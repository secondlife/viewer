/** 
 * @file llvovolume.cpp
 * @brief LLVOVolume class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

// A "volume" is a box, cylinder, sphere, or other primitive shape.

#include "llviewerprecompiledheaders.h"

#include "llvovolume.h"

#include "llviewercontrol.h"
#include "lldir.h"
#include "llflexibleobject.h"
#include "llmaterialtable.h"
#include "llprimitive.h"
#include "llvolume.h"
#include "llvolumemgr.h"
#include "llvolumemessage.h"
#include "material_codes.h"
#include "message.h"
#include "object_flags.h"
#include "llagent.h"
#include "lldrawable.h"
#include "lldrawpoolbump.h"
#include "llface.h"
#include "llspatialpartition.h"

// TEMP HACK ventrella
#include "llhudmanager.h"
#include "llflexibleobject.h"

#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewerregion.h"
#include "llviewertextureanim.h"
#include "llworld.h"
#include "llselectmgr.h"
#include "pipeline.h"

const S32 MIN_QUIET_FRAMES_COALESCE = 30;
const F32 FORCE_SIMPLE_RENDER_AREA = 512.f;
const F32 FORCE_CULL_AREA = 8.f;
const S32 SCULPT_REZ = 128;

BOOL gAnimateTextures = TRUE;
extern BOOL gHideSelectedObjects;

F32 LLVOVolume::sLODFactor = 1.f;
F32	LLVOVolume::sLODSlopDistanceFactor = 0.5f; //Changing this to zero, effectively disables the LOD transition slop 
F32 LLVOVolume::sDistanceFactor = 1.0f;
S32 LLVOVolume::sNumLODChanges = 0;

LLVOVolume::LLVOVolume(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
	: LLViewerObject(id, pcode, regionp),
	  mVolumeImpl(NULL)
{
	mTexAnimMode = 0;
	mRelativeXform.setIdentity();
	mRelativeXformInvTrans.setIdentity();

	mLOD = MIN_LOD;
	mTextureAnimp = NULL;
	mVObjRadius = LLVector3(1,1,0.5f).magVec();
	mNumFaces = 0;
	mLODChanged = FALSE;
	mSculptChanged = FALSE;
}

LLVOVolume::~LLVOVolume()
{
	delete mTextureAnimp;
	mTextureAnimp = NULL;
	delete mVolumeImpl;
	mVolumeImpl = NULL;
}


// static
void LLVOVolume::initClass()
{
}


U32 LLVOVolume::processUpdateMessage(LLMessageSystem *mesgsys,
										  void **user_data,
										  U32 block_num, EObjectUpdateType update_type,
										  LLDataPacker *dp)
{
	LLColor4U color;

	// Do base class updates...
	U32 retval = LLViewerObject::processUpdateMessage(mesgsys, user_data, block_num, update_type, dp);

	LLUUID sculpt_id;
	U8 sculpt_type = 0;
	if (isSculpted())
	{
		LLSculptParams *sculpt_params = (LLSculptParams *)getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		sculpt_id = sculpt_params->getSculptTexture();
		sculpt_type = sculpt_params->getSculptType();
	}

	if (!dp)
	{
		if (update_type == OUT_FULL)
		{
			////////////////////////////////
			//
			// Unpack texture animation data
			//
			//

			if (mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_TextureAnim))
			{
				if (!mTextureAnimp)
				{
					mTextureAnimp = new LLViewerTextureAnim();
				}
				else
				{
					if (!(mTextureAnimp->mMode & LLTextureAnim::SMOOTH))
					{
						mTextureAnimp->reset();
					}
				}
				mTexAnimMode = 0;
				mTextureAnimp->unpackTAMessage(mesgsys, block_num);
			}
			else
			{
				if (mTextureAnimp)
				{
					delete mTextureAnimp;
					mTextureAnimp = NULL;
					gPipeline.markTextured(mDrawable);
					mFaceMappingChanged = TRUE;
					mTexAnimMode = 0;
				}
			}

			// Unpack volume data
			LLVolumeParams volume_params;
			LLVolumeMessage::unpackVolumeParams(&volume_params, mesgsys, _PREHASH_ObjectData, block_num);
			volume_params.setSculptID(sculpt_id, sculpt_type);

			if (setVolume(volume_params, 0))
			{
				markForUpdate(TRUE);
			}
		}

		// Sigh, this needs to be done AFTER the volume is set as well, otherwise bad stuff happens...
		////////////////////////////
		//
		// Unpack texture entry data
		//
		if (unpackTEMessage(mesgsys, _PREHASH_ObjectData, block_num) & (TEM_CHANGE_TEXTURE|TEM_CHANGE_COLOR))
		{
			updateTEData();
		}
	}
	else
	{
		// CORY TO DO: Figure out how to get the value here
		if (update_type != OUT_TERSE_IMPROVED)
		{
			LLVolumeParams volume_params;
			BOOL res = LLVolumeMessage::unpackVolumeParams(&volume_params, *dp);
			if (!res)
			{
				llwarns << "Bogus volume parameters in object " << getID() << llendl;
				llwarns << getRegion()->getOriginGlobal() << llendl;
			}

			volume_params.setSculptID(sculpt_id, sculpt_type);

			if (setVolume(volume_params, 0))
			{
				markForUpdate(TRUE);
			}
			S32 res2 = unpackTEMessage(*dp);
			if (TEM_INVALID == res2)
			{
				// Well, crap, there's something bogus in the data that we're unpacking.
				dp->dumpBufferToLog();
				llwarns << "Flushing cache files" << llendl;
				std::string mask;
				mask = gDirUtilp->getDirDelimiter() + "*.slc";
				gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE,""), mask);
// 				llerrs << "Bogus TE data in " << getID() << ", crashing!" << llendl;
				llwarns << "Bogus TE data in " << getID() << llendl;
			}
			else if (res2 & (TEM_CHANGE_TEXTURE|TEM_CHANGE_COLOR))
			{
				updateTEData();
			}

			U32 value = dp->getPassFlags();

			if (value & 0x40)
			{
				if (!mTextureAnimp)
				{
					mTextureAnimp = new LLViewerTextureAnim();
				}
				else
				{
					if (!(mTextureAnimp->mMode & LLTextureAnim::SMOOTH))
					{
						mTextureAnimp->reset();
					}
				}
				mTexAnimMode = 0;
				mTextureAnimp->unpackTAMessage(*dp);
			}
			else if (mTextureAnimp)
			{
				delete mTextureAnimp;
				mTextureAnimp = NULL;
				gPipeline.markTextured(mDrawable);
				mFaceMappingChanged = TRUE;
				mTexAnimMode = 0;
			}
		}
		else
		{
			S32 texture_length = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_TextureEntry);
			if (texture_length)
			{
				U8							tdpbuffer[1024];
				LLDataPackerBinaryBuffer	tdp(tdpbuffer, 1024);
				mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_TextureEntry, tdpbuffer, 0, block_num);
				if ( unpackTEMessage(tdp) & (TEM_CHANGE_TEXTURE|TEM_CHANGE_COLOR))
				{
					updateTEData();
				}
			}
		}
	}
	
	return retval;
}


void LLVOVolume::animateTextures()
{
	F32 off_s = 0.f, off_t = 0.f, scale_s = 1.f, scale_t = 1.f, rot = 0.f;
	S32 result = mTextureAnimp->animateTextures(off_s, off_t, scale_s, scale_t, rot);
	
	if (result)
	{
		if (!mTexAnimMode)
		{
			mFaceMappingChanged = TRUE;
			gPipeline.markTextured(mDrawable);
		}
		mTexAnimMode = result | mTextureAnimp->mMode;
				
		S32 start=0, end=mDrawable->getNumFaces()-1;
		if (mTextureAnimp->mFace >= 0 && mTextureAnimp->mFace <= end)
		{
			start = end = mTextureAnimp->mFace;
		}
		
		for (S32 i = start; i <= end; i++)
		{
			LLFace* facep = mDrawable->getFace(i);
			if(facep->getVirtualSize() <= MIN_TEX_ANIM_SIZE && facep->mTextureMatrix) continue;

			const LLTextureEntry* te = facep->getTextureEntry();
			
			if (!te)
			{
				continue;
			}
		
			if (!(result & LLViewerTextureAnim::ROTATE))
			{
				te->getRotation(&rot);
			}
			if (!(result & LLViewerTextureAnim::TRANSLATE))
			{
				te->getOffset(&off_s,&off_t);
			}			
			if (!(result & LLViewerTextureAnim::SCALE))
			{
				te->getScale(&scale_s, &scale_t);
			}

			LLVector3 scale(scale_s, scale_t, 1.f);
			LLVector3 trans(off_s+0.5f, off_t+0.5f, 0.f);
			LLQuaternion quat;
			quat.setQuat(rot, 0, 0, -1.f);
		
			if (!facep->mTextureMatrix)
			{
				facep->mTextureMatrix = new LLMatrix4();
			}

			LLMatrix4& tex_mat = *facep->mTextureMatrix;
			tex_mat.setIdentity();
			tex_mat.translate(LLVector3(-0.5f, -0.5f, 0.f));
			tex_mat.rotate(quat);				

			LLMatrix4 mat;
			mat.initAll(scale, LLQuaternion(), LLVector3());
			tex_mat *= mat;
		
			tex_mat.translate(trans);
		}
	}
	else
	{
		if (mTexAnimMode && mTextureAnimp->mRate == 0)
		{
			U8 start, count;

			if (mTextureAnimp->mFace == -1)
			{
				start = 0;
				count = getNumTEs();
			}
			else
			{
				start = (U8) mTextureAnimp->mFace;
				count = 1;
			}

			for (S32 i = start; i < start + count; i++)
			{
				if (mTexAnimMode & LLViewerTextureAnim::TRANSLATE)
				{
					setTEOffset(i, mTextureAnimp->mOffS, mTextureAnimp->mOffT);				
				}
				if (mTexAnimMode & LLViewerTextureAnim::SCALE)
				{
					setTEScale(i, mTextureAnimp->mScaleS, mTextureAnimp->mScaleT);	
				}
				if (mTexAnimMode & LLViewerTextureAnim::ROTATE)
				{
					setTERotation(i, mTextureAnimp->mRot);
				}
			}

			gPipeline.markTextured(mDrawable);
			mFaceMappingChanged = TRUE;
			mTexAnimMode = 0;
		}
	}
}
BOOL LLVOVolume::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	LLViewerObject::idleUpdate(agent, world, time);

	if (mDead || mDrawable.isNull())
	{
		return TRUE;
	}
	
	///////////////////////
	//
	// Do texture animation stuff
	//

	if (mTextureAnimp && gAnimateTextures)
	{
		animateTextures();
	}

	// Dispatch to implementation
	if (mVolumeImpl)
	{
		mVolumeImpl->doIdleUpdate(agent, world, time);
	}

	return TRUE;
}

void LLVOVolume::updateTextures(LLAgent &agent)
{
	const F32 TEXTURE_AREA_REFRESH_TIME = 5.f; // seconds
	if (mTextureUpdateTimer.getElapsedTimeF32() > TEXTURE_AREA_REFRESH_TIME)
	{
		if (mDrawable->isVisible())
		{
			updateTextures();
		}
	}
}

void LLVOVolume::updateTextures()
{
	// Update the pixel area of all faces

	if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SIMPLE))
	{
		return;
	}
	
	if (LLViewerImage::sDontLoadVolumeTextures || mDrawable.isNull()) // || !mDrawable->isVisible())
	{
		return;
	}

	mTextureUpdateTimer.reset();
	
	F32 old_area = mPixelArea;
	mPixelArea = 0.f;

	const S32 num_faces = mDrawable->getNumFaces();
	F32 min_vsize=999999999.f, max_vsize=0.f;
	for (S32 i = 0; i < num_faces; i++)
	{
		LLFace* face = mDrawable->getFace(i);
		const LLTextureEntry *te = face->getTextureEntry();
		LLViewerImage *imagep = face->getTexture();
		if (!imagep || !te ||
			face->mExtents[0] == face->mExtents[1])
		{
			continue;
		}
		
		F32 vsize;
		
		if (isHUDAttachment())
		{
			F32 area = (F32) LLViewerCamera::getInstance()->getScreenPixelArea();
			vsize = area;
			imagep->setBoostLevel(LLViewerImage::BOOST_HUD);
 			face->setPixelArea(area); // treat as full screen
		}
		else
		{
			vsize = getTextureVirtualSize(face);
		}

		mPixelArea = llmax(mPixelArea, face->getPixelArea());

		F32 old_size = face->getVirtualSize();

		if (face->getPoolType() == LLDrawPool::POOL_ALPHA)
		{
			
			if (LLPipeline::sFastAlpha &&
				vsize < MIN_ALPHA_SIZE && old_size > MIN_ALPHA_SIZE ||
				vsize > MIN_ALPHA_SIZE && old_size < MIN_ALPHA_SIZE)
			{
				gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_COLOR, FALSE);
			}
		}

		if (face->mTextureMatrix != NULL)
		{
			if (vsize < MIN_TEX_ANIM_SIZE && old_size > MIN_TEX_ANIM_SIZE ||
				vsize > MIN_TEX_ANIM_SIZE && old_size < MIN_TEX_ANIM_SIZE)
			{
				gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD, FALSE);
			}
		}
		
		face->setVirtualSize(vsize);
		imagep->addTextureStats(vsize);
		if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
		{
			if (vsize < min_vsize) min_vsize = vsize;
			if (vsize > max_vsize) max_vsize = vsize;
		}
		else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
		{
			F32 pri = imagep->getDecodePriority();
			if (pri < min_vsize) min_vsize = pri;
			if (pri > max_vsize) max_vsize = pri;
		}
		else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_FACE_AREA))
		{
			F32 pri = mPixelArea;
			if (pri < min_vsize) min_vsize = pri;
			if (pri > max_vsize) max_vsize = pri;
		}	
	}
	
	if (isSculpted())
	{
		LLSculptParams *sculpt_params = (LLSculptParams *)getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		LLUUID id =  sculpt_params->getSculptTexture(); 
		mSculptTexture = gImageList.getImage(id);
		if (mSculptTexture.notNull())
		{
			mSculptTexture->addTextureStats(SCULPT_REZ * SCULPT_REZ);
			mSculptTexture->setBoostLevel(LLViewerImage::BOOST_SCULPTED);
		}

		S32 texture_discard = mSculptTexture->getDiscardLevel(); //try to match the texture
		S32 current_discard = getVolume()->getSculptLevel();
		
		if (texture_discard >= 0 && //texture has some data available
			(texture_discard < current_discard || //texture has more data than last rebuild
			current_discard < 0)) //no previous rebuild
		{
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, FALSE);
			mSculptChanged = TRUE;
		}
		
	}


	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
	{
		setDebugText(llformat("%.0f:%.0f", fsqrtf(min_vsize),fsqrtf(max_vsize)));
	}
	else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
	{
		setDebugText(llformat("%.0f:%.0f", fsqrtf(min_vsize),fsqrtf(max_vsize)));
	}
	else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_FACE_AREA))
	{
		setDebugText(llformat("%.0f:%.0f", fsqrtf(min_vsize),fsqrtf(max_vsize)));
	}

	if (mPixelArea == 0)
	{ //flexi phasing issues make this happen
		mPixelArea = old_area;
	}
}

F32 LLVOVolume::getTextureVirtualSize(LLFace* face)
{
	//get area of circle around face
	LLVector3 center = face->getPositionAgent();
	LLVector3 size = (face->mExtents[1] - face->mExtents[0]) * 0.5f;
	
	F32 face_area = LLPipeline::calcPixelArea(center, size, *LLViewerCamera::getInstance());

	face->setPixelArea(face_area);

	if (face_area <= 0)
	{
		return 0.f;
	}

	//get area of circle in texture space
	LLVector2 tdim = face->mTexExtents[1] - face->mTexExtents[0];
	F32 texel_area = (tdim * 0.5f).magVecSquared()*3.14159f;
	if (texel_area <= 0)
	{
		// Probably animated, use default
		texel_area = 1.f;
	}

	//apply texel area to face area to get accurate ratio
	face_area /= llclamp(texel_area, 1.f/64.f, 16.f);

	return face_area;
}

BOOL LLVOVolume::isActive() const
{
	return !mStatic || mTextureAnimp || (mVolumeImpl && mVolumeImpl->isActive());
}

BOOL LLVOVolume::setMaterial(const U8 material)
{
	BOOL res = LLViewerObject::setMaterial(material);
	
	return res;
}

void LLVOVolume::setTexture(const S32 face)
{
	llassert(face < getNumTEs());
	LLViewerImage::bindTexture(getTEImage(face));
}

void LLVOVolume::setScale(const LLVector3 &scale, BOOL damped)
{
	if (scale != getScale())
	{
		// store local radius
		LLViewerObject::setScale(scale);

		if (mVolumeImpl)
		{
			mVolumeImpl->onSetScale(scale, damped);
		}
		
		updateRadius();

		//since drawable transforms do not include scale, changing volume scale
		//requires an immediate rebuild of volume verts.
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_POSITION, TRUE);
	}
}

LLFace* LLVOVolume::addFace(S32 f)
{
	const LLTextureEntry* te = getTE(f);
	LLViewerImage* imagep = getTEImage(f);
	return mDrawable->addFace(te, imagep);
}

LLDrawable *LLVOVolume::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
		
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_VOLUME);

	S32 max_tes_to_set = getNumTEs();
	for (S32 i = 0; i < max_tes_to_set; i++)
	{
		addFace(i);
	}
	mNumFaces = max_tes_to_set;

	if (isAttachment())
	{
		mDrawable->makeActive();
	}

	if (getIsLight())
	{
		// Add it to the pipeline mLightSet
		gPipeline.setLight(mDrawable, TRUE);
	}
	
	updateRadius();
	mDrawable->updateDistance(*LLViewerCamera::getInstance());

	return mDrawable;
}


BOOL LLVOVolume::setVolume(const LLVolumeParams &volume_params, const S32 detail, bool unique_volume)
{
	// Check if we need to change implementations
	bool is_flexible = (volume_params.getPathParams().getCurveType() == LL_PCODE_PATH_FLEXIBLE);
	if (is_flexible)
	{
		setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, TRUE, false);
		if (!mVolumeImpl)
		{
			LLFlexibleObjectData* data = (LLFlexibleObjectData*)getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
			mVolumeImpl = new LLVolumeImplFlexible(this, data);
		}
	}
	else
	{
		// Mark the parameter not in use
		setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, FALSE, false);
		if (mVolumeImpl)
		{
			delete mVolumeImpl;
			mVolumeImpl = NULL;
			if (mDrawable.notNull())
			{
				// Undo the damage we did to this matrix
				mDrawable->updateXform(FALSE);
			}
		}
	}
	
	if ((LLPrimitive::setVolume(volume_params, mLOD, (mVolumeImpl && mVolumeImpl->isVolumeUnique()))) || mSculptChanged)
	{
		mFaceMappingChanged = TRUE;
		
		if (mVolumeImpl)
		{
			mVolumeImpl->onSetVolume(volume_params, detail);
		}
		
		if (isSculpted())
		{
			mSculptTexture = gImageList.getImage(volume_params.getSculptID());
			if (mSculptTexture.notNull())
			{
				sculpt();
			}
		}
		else
		{
			mSculptTexture = NULL;
		}

		return TRUE;
	}
	return FALSE;
}

// sculpt replaces generate() for sculpted surfaces
void LLVOVolume::sculpt()
{
	U16 sculpt_height = 0;
	U16 sculpt_width = 0;
	S8 sculpt_components = 0;
	const U8* sculpt_data = NULL;

	if (mSculptTexture.notNull())
	{
		S32 discard_level;
		S32 desired_discard = 0; // lower discard levels have MUCH less resolution 

		discard_level = desired_discard;
		
		S32 max_discard = mSculptTexture->getMaxDiscardLevel();
		if (discard_level > max_discard)
			discard_level = max_discard;    // clamp to the best we can do

		S32 best_discard = mSculptTexture->getDiscardLevel();
		if (discard_level < best_discard)
			discard_level = best_discard;   // clamp to what we have

		if (best_discard == -1)
			discard_level = -1;  // and if we have nothing, set to nothing

		
		S32 current_discard = getVolume()->getSculptLevel();
		if(current_discard < -2)
		{
			llwarns << "WARNING!!: Current discard of sculpty at " << current_discard 
				<< " is less than -2." << llendl;
			
			// corrupted volume... don't update the sculpty
			return;
		}
		else if (current_discard > max_discard)
		{
			llwarns << "WARNING!!: Current discard of sculpty at " << current_discard 
				<< " is more than than allowed max of " << max_discard << llendl;
			
			// corrupted volume... don't update the sculpty			
			return;
		}

		if (current_discard == discard_level)  // no work to do here
			return;
		
		LLPointer<LLImageRaw> raw_image = new LLImageRaw();
		BOOL is_valid = mSculptTexture->readBackRaw(discard_level, raw_image, FALSE);

		sculpt_height = raw_image->getHeight();
		sculpt_width = raw_image->getWidth();
		sculpt_components = raw_image->getComponents();		

		if(is_valid)
		{
			is_valid = mSculptTexture->isValidForSculpt(discard_level, sculpt_width, sculpt_height, sculpt_components) ;
		}
		if(!is_valid)
		{
			sculpt_width = 0;
			sculpt_height = 0;
			sculpt_data = NULL ;
		}
		else
		{
			if (raw_image->getDataSize() < sculpt_height * sculpt_width * sculpt_components)
				llerrs << "Sculpt: image data size = " << raw_image->getDataSize()
					   << " < " << sculpt_height << " x " << sculpt_width << " x " <<sculpt_components << llendl;
					   
			sculpt_data = raw_image->getData();
		}
		getVolume()->sculpt(sculpt_width, sculpt_height, sculpt_components, sculpt_data, discard_level);
	}
}

S32	LLVOVolume::computeLODDetail(F32 distance, F32 radius)
{
	S32	cur_detail;
	if (LLPipeline::sDynamicLOD)
	{
		// We've got LOD in the profile, and in the twist.  Use radius.
		F32 tan_angle = (LLVOVolume::sLODFactor*radius)/distance;
		cur_detail = LLVolumeLODGroup::getDetailFromTan(llround(tan_angle, 0.01f));
	}
	else
	{
		cur_detail = llclamp((S32) (sqrtf(radius)*LLVOVolume::sLODFactor*4.f), 0, 3);		
	}
	return cur_detail;
}

BOOL LLVOVolume::calcLOD()
{
	if (mDrawable.isNull())
	{
		return FALSE;
	}

	//update face texture sizes on lod calculation
	updateTextures();

	S32 cur_detail = 0;
	
	F32 radius = getVolume()->mLODScaleBias.scaledVec(getScale()).magVec();
	F32 distance = mDrawable->mDistanceWRTCamera;
	distance *= sDistanceFactor;
			
	F32 rampDist = LLVOVolume::sLODFactor * 2;
	
	if (distance < rampDist)
	{
		// Boost LOD when you're REALLY close
		distance *= 1.0f/rampDist;
		distance *= distance;
		distance *= rampDist;
	}
	
	// DON'T Compensate for field of view changing on FOV zoom.
	distance *= 3.14159f/3.f;

	cur_detail = computeLODDetail(llround(distance, 0.01f), 
									llround(radius, 0.01f));

	if (cur_detail != mLOD)
	{
		mAppAngle = llround((F32) atan2( mDrawable->getRadius(), mDrawable->mDistanceWRTCamera) * RAD_TO_DEG, 0.01f);
		mLOD = cur_detail;		
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL LLVOVolume::updateLOD()
{
	if (mDrawable.isNull())
	{
		return FALSE;
	}
	
	BOOL lod_changed = calcLOD();

	if (lod_changed)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, FALSE);
		mLODChanged = TRUE;
	}

	lod_changed |= LLViewerObject::updateLOD();
	
	return lod_changed;
}

BOOL LLVOVolume::setDrawableParent(LLDrawable* parentp)
{
	if (!LLViewerObject::setDrawableParent(parentp))
	{
		// no change in drawable parent
		return FALSE;
	}

	if (!mDrawable->isRoot())
	{
		// rebuild vertices in parent relative space
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);

		if (mDrawable->isActive() && !parentp->isActive())
		{
			parentp->makeActive();
		}
		else if (mDrawable->isStatic() && parentp->isActive())
		{
			mDrawable->makeActive();
		}
	}
	
	return TRUE;
}

void LLVOVolume::updateFaceFlags()
{
	for (S32 i = 0; i < getVolume()->getNumFaces(); i++)
	{
		LLFace *face = mDrawable->getFace(i);
		BOOL fullbright = getTE(i)->getFullbright();
		face->clearState(LLFace::FULLBRIGHT | LLFace::HUD_RENDER | LLFace::LIGHT);

		if (fullbright || (mMaterial == LL_MCODE_LIGHT))
		{
			face->setState(LLFace::FULLBRIGHT);
		}
		if (mDrawable->isLight())
		{
			face->setState(LLFace::LIGHT);
		}
		if (isHUDAttachment())
		{
			face->setState(LLFace::HUD_RENDER);
		}
	}
}

void LLVOVolume::setParent(LLViewerObject* parent)
{
	if (parent != getParent())
	{
		LLViewerObject::setParent(parent);
		if (mDrawable)
		{
			gPipeline.markMoved(mDrawable);
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
		}
	}
}

// NOTE: regenFaces() MUST be followed by genTriangles()!
void LLVOVolume::regenFaces()
{
	// remove existing faces
	BOOL count_changed = mNumFaces != getNumTEs();
	
	if (count_changed)
	{
		deleteFaces();		
		// add new faces
		mNumFaces = getNumTEs();
	}
		
	for (S32 i = 0; i < mNumFaces; i++)
	{
		LLFace* facep = count_changed ? addFace(i) : mDrawable->getFace(i);
		facep->setTEOffset(i);
		facep->setTexture(getTEImage(i));
		facep->setViewerObject(this);
	}
	
	if (!count_changed)
	{
		updateFaceFlags();
	}
}

BOOL LLVOVolume::genBBoxes(BOOL force_global)
{
	BOOL res = TRUE;

	LLVector3 min,max;

	BOOL rebuild = mDrawable->isState(LLDrawable::REBUILD_VOLUME | LLDrawable::REBUILD_POSITION);

	for (S32 i = 0; i < getVolume()->getNumFaces(); i++)
	{
		LLFace *face = mDrawable->getFace(i);
		res &= face->genVolumeBBoxes(*getVolume(), i,
										mRelativeXform, mRelativeXformInvTrans,
										(mVolumeImpl && mVolumeImpl->isVolumeGlobal()) || force_global);
		
		if (rebuild)
		{
			if (i == 0)
			{
				min = face->mExtents[0];
				max = face->mExtents[1];
			}
			else
			{
				for (U32 i = 0; i < 3; i++)
				{
					if (face->mExtents[0].mV[i] < min.mV[i])
					{
						min.mV[i] = face->mExtents[0].mV[i];
					}
					if (face->mExtents[1].mV[i] > max.mV[i])
					{
						max.mV[i] = face->mExtents[1].mV[i];
					}
				}
			}
		}
	}
	
	if (rebuild)
	{
		mDrawable->setSpatialExtents(min,max);
		mDrawable->setPositionGroup((min+max)*0.5f);	
	}

	updateRadius();
	mDrawable->movePartition();
			
	return res;
}

void LLVOVolume::preRebuild()
{
	if (mVolumeImpl != NULL)
	{
		mVolumeImpl->preRebuild();
	}
}

void LLVOVolume::updateRelativeXform()
{
	if (mVolumeImpl)
	{
		mVolumeImpl->updateRelativeXform();
		return;
	}
	
	LLDrawable* drawable = mDrawable;
	
	if (drawable->isActive())
	{				
		// setup relative transforms
		LLQuaternion delta_rot;
		LLVector3 delta_pos, delta_scale;
		
		//matrix from local space to parent relative/global space
		delta_rot = drawable->isSpatialRoot() ? LLQuaternion() : mDrawable->getRotation();
		delta_pos = drawable->isSpatialRoot() ? LLVector3(0,0,0) : mDrawable->getPosition();
		delta_scale = mDrawable->getScale();

		// Vertex transform (4x4)
		LLVector3 x_axis = LLVector3(delta_scale.mV[VX], 0.f, 0.f) * delta_rot;
		LLVector3 y_axis = LLVector3(0.f, delta_scale.mV[VY], 0.f) * delta_rot;
		LLVector3 z_axis = LLVector3(0.f, 0.f, delta_scale.mV[VZ]) * delta_rot;

		mRelativeXform.initRows(LLVector4(x_axis, 0.f),
								LLVector4(y_axis, 0.f),
								LLVector4(z_axis, 0.f),
								LLVector4(delta_pos, 1.f));

		
		// compute inverse transpose for normals
		// mRelativeXformInvTrans.setRows(x_axis, y_axis, z_axis);
		// mRelativeXformInvTrans.invert(); 
		// mRelativeXformInvTrans.setRows(x_axis, y_axis, z_axis);
		// grumble - invert is NOT a matrix invert, so we do it by hand:

		LLMatrix3 rot_inverse = LLMatrix3(~delta_rot);

		LLMatrix3 scale_inverse;
		scale_inverse.setRows(LLVector3(1.0, 0.0, 0.0) / delta_scale.mV[VX],
							  LLVector3(0.0, 1.0, 0.0) / delta_scale.mV[VY],
							  LLVector3(0.0, 0.0, 1.0) / delta_scale.mV[VZ]);
							   
		
		mRelativeXformInvTrans = rot_inverse * scale_inverse;

		mRelativeXformInvTrans.transpose();
	}
	else
	{
		LLVector3 pos = getPosition();
		LLVector3 scale = getScale();
		LLQuaternion rot = getRotation();
	
		if (mParent)
		{
			pos *= mParent->getRotation();
			pos += mParent->getPosition();
			rot *= mParent->getRotation();
		}
		
		//LLViewerRegion* region = getRegion();
		//pos += region->getOriginAgent();
		
		LLVector3 x_axis = LLVector3(scale.mV[VX], 0.f, 0.f) * rot;
		LLVector3 y_axis = LLVector3(0.f, scale.mV[VY], 0.f) * rot;
		LLVector3 z_axis = LLVector3(0.f, 0.f, scale.mV[VZ]) * rot;

		mRelativeXform.initRows(LLVector4(x_axis, 0.f),
								LLVector4(y_axis, 0.f),
								LLVector4(z_axis, 0.f),
								LLVector4(pos, 1.f));

		// compute inverse transpose for normals
		LLMatrix3 rot_inverse = LLMatrix3(~rot);

		LLMatrix3 scale_inverse;
		scale_inverse.setRows(LLVector3(1.0, 0.0, 0.0) / scale.mV[VX],
							  LLVector3(0.0, 1.0, 0.0) / scale.mV[VY],
							  LLVector3(0.0, 0.0, 1.0) / scale.mV[VZ]);
							   
		
		mRelativeXformInvTrans = rot_inverse * scale_inverse;

		mRelativeXformInvTrans.transpose();
	}
}

BOOL LLVOVolume::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer t(LLFastTimer::FTM_UPDATE_PRIMITIVES);
	
	if (mVolumeImpl != NULL)
	{
		BOOL res;
		{
			LLFastTimer t(LLFastTimer::FTM_GEN_FLEX);
			res = mVolumeImpl->doUpdateGeometry(drawable);
		}
		updateFaceFlags();
		return res;
	}
	
	dirtySpatialGroup();

	BOOL compiled = FALSE;
			
	updateRelativeXform();
	
	if (mDrawable.isNull()) // Not sure why this is happening, but it is...
	{
		return TRUE; // No update to complete
	}

	if (mVolumeChanged || mFaceMappingChanged )
	{
		compiled = TRUE;

		if (mVolumeChanged)
		{
			LLFastTimer ftm(LLFastTimer::FTM_GEN_VOLUME);
			LLVolumeParams volume_params = getVolume()->getParams();
			setVolume(volume_params, 0);
			drawable->setState(LLDrawable::REBUILD_VOLUME);
		}

		{
			LLFastTimer t(LLFastTimer::FTM_GEN_TRIANGLES);
			regenFaces();
			genBBoxes(FALSE);
		}
	}
	else if ((mLODChanged) || (mSculptChanged))
	{
		LLVolume *old_volumep, *new_volumep;
		F32 old_lod, new_lod;
		S32 old_num_faces, new_num_faces ;

		old_volumep = getVolume();
		old_lod = old_volumep->getDetail();
		old_num_faces = old_volumep->getNumFaces() ;
		old_volumep = NULL ;

		{
			LLFastTimer ftm(LLFastTimer::FTM_GEN_VOLUME);
			LLVolumeParams volume_params = getVolume()->getParams();
			setVolume(volume_params, 0);
		}

		new_volumep = getVolume();
		new_lod = new_volumep->getDetail();
		new_num_faces = new_volumep->getNumFaces() ;
		new_volumep = NULL ;

		if ((new_lod != old_lod) || mSculptChanged)
		{
			compiled = TRUE;
			sNumLODChanges += new_num_faces ;
	
			drawable->setState(LLDrawable::REBUILD_VOLUME); // for face->genVolumeTriangles()

			{
				LLFastTimer t(LLFastTimer::FTM_GEN_TRIANGLES);
				if (new_num_faces != old_num_faces)
				{
					regenFaces();
				}
				genBBoxes(FALSE);
			}
		}
	}
	// it has its own drawable (it's moved) or it has changed UVs or it has changed xforms from global<->local
	else
	{
		compiled = TRUE;
		// All it did was move or we changed the texture coordinate offset
		LLFastTimer t(LLFastTimer::FTM_GEN_TRIANGLES);
		genBBoxes(FALSE);
	}

	// Update face flags
	updateFaceFlags();
	
	if(compiled)
	{
		LLPipeline::sCompiles++;
	}
	
	mVolumeChanged = FALSE;
	mLODChanged = FALSE;
	mSculptChanged = FALSE;
	mFaceMappingChanged = FALSE;

	return LLViewerObject::updateGeometry(drawable);
}

void LLVOVolume::updateFaceSize(S32 idx)
{
	LLFace* facep = mDrawable->getFace(idx);
	if (idx >= getVolume()->getNumVolumeFaces())
	{
		facep->setSize(0,0);
	}
	else
	{
		const LLVolumeFace& vol_face = getVolume()->getVolumeFace(idx);
		facep->setSize(vol_face.mVertices.size(), vol_face.mIndices.size());
	}
}

BOOL LLVOVolume::isRootEdit() const
{
	if (mParent && !((LLViewerObject*)mParent)->isAvatar())
	{
		return FALSE;
	}
	return TRUE;
}

void LLVOVolume::setTEImage(const U8 te, LLViewerImage *imagep)
{
	BOOL changed = (mTEImages[te] != imagep);
	LLViewerObject::setTEImage(te, imagep);
	if (changed)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
}

S32 LLVOVolume::setTETexture(const U8 te, const LLUUID &uuid)
{
	S32 res = LLViewerObject::setTETexture(te, uuid);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}

S32 LLVOVolume::setTEColor(const U8 te, const LLColor3& color)
{
	return setTEColor(te, LLColor4(color));
}

S32 LLVOVolume::setTEColor(const U8 te, const LLColor4& color)
{
	S32 res = LLViewerObject::setTEColor(te, color);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTEBumpmap(const U8 te, const U8 bumpmap)
{
	S32 res = LLViewerObject::setTEBumpmap(te, bumpmap);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTETexGen(const U8 te, const U8 texgen)
{
	S32 res = LLViewerObject::setTETexGen(te, texgen);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTEShiny(const U8 te, const U8 shiny)
{
	S32 res = LLViewerObject::setTEShiny(te, shiny);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTEFullbright(const U8 te, const U8 fullbright)
{
	S32 res = LLViewerObject::setTEFullbright(te, fullbright);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTEMediaFlags(const U8 te, const U8 media_flags)
{
	S32 res = LLViewerObject::setTEMediaFlags(te, media_flags);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTEGlow(const U8 te, const F32 glow)
{
	S32 res = LLViewerObject::setTEGlow(te, glow);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTEScale(const U8 te, const F32 s, const F32 t)
{
	S32 res = LLViewerObject::setTEScale(te, s, t);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}

S32 LLVOVolume::setTEScaleS(const U8 te, const F32 s)
{
	S32 res = LLViewerObject::setTEScaleS(te, s);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}

S32 LLVOVolume::setTEScaleT(const U8 te, const F32 t)
{
	S32 res = LLViewerObject::setTEScaleT(te, t);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}

void LLVOVolume::updateTEData()
{
	if (mDrawable.notNull())
	{
		mFaceMappingChanged = TRUE;
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_MATERIAL, TRUE);
	}
}

//----------------------------------------------------------------------------

void LLVOVolume::setIsLight(BOOL is_light)
{
	if (is_light != getIsLight())
	{
		if (is_light)
		{
			setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT, TRUE, true);
		}
		else
		{
			setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT, FALSE, true);
		}

		if (is_light)
		{
			// Add it to the pipeline mLightSet
			gPipeline.setLight(mDrawable, TRUE);
		}
		else
		{
			// Not a light.  Remove it from the pipeline's light set.
			gPipeline.setLight(mDrawable, FALSE);
		}
	}
}

void LLVOVolume::setLightColor(const LLColor3& color)
{
	LLLightParams *param_block = (LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		if (param_block->getColor() != color)
		{
			param_block->setColor(LLColor4(color, param_block->getColor().mV[3]));
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
			gPipeline.markTextured(mDrawable);
			mFaceMappingChanged = TRUE;
		}
	}
}

void LLVOVolume::setLightIntensity(F32 intensity)
{
	LLLightParams *param_block = (LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		if (param_block->getColor().mV[3] != intensity)
		{
			param_block->setColor(LLColor4(LLColor3(param_block->getColor()), intensity));
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
		}
	}
}

void LLVOVolume::setLightRadius(F32 radius)
{
	LLLightParams *param_block = (LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		if (param_block->getRadius() != radius)
		{
			param_block->setRadius(radius);
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
		}
	}
}

void LLVOVolume::setLightFalloff(F32 falloff)
{
	LLLightParams *param_block = (LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		if (param_block->getFalloff() != falloff)
		{
			param_block->setFalloff(falloff);
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
		}
	}
}

void LLVOVolume::setLightCutoff(F32 cutoff)
{
	LLLightParams *param_block = (LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		if (param_block->getCutoff() != cutoff)
		{
			param_block->setCutoff(cutoff);
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
		}
	}
}

//----------------------------------------------------------------------------

BOOL LLVOVolume::getIsLight() const
{
	return getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT);
}

LLColor3 LLVOVolume::getLightBaseColor() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return LLColor3(param_block->getColor());
	}
	else
	{
		return LLColor3(1,1,1);
	}
}

LLColor3 LLVOVolume::getLightColor() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return LLColor3(param_block->getColor()) * param_block->getColor().mV[3];
	}
	else
	{
		return LLColor3(1,1,1);
	}
}

F32 LLVOVolume::getLightIntensity() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return param_block->getColor().mV[3];
	}
	else
	{
		return 1.f;
	}
}

F32 LLVOVolume::getLightRadius() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return param_block->getRadius();
	}
	else
	{
		return 0.f;
	}
}

F32 LLVOVolume::getLightFalloff() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return param_block->getFalloff();
	}
	else
	{
		return 0.f;
	}
}

F32 LLVOVolume::getLightCutoff() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return param_block->getCutoff();
	}
	else
	{
		return 0.f;
	}
}

U32 LLVOVolume::getVolumeInterfaceID() const
{
	if (mVolumeImpl)
	{
		return mVolumeImpl->getID();
	}

	return 0;
}

BOOL LLVOVolume::isFlexible() const
{
	if (getParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE))
	{
		LLVolume* volume = getVolume();
		if (volume && volume->getParams().getPathParams().getCurveType() != LL_PCODE_PATH_FLEXIBLE)
		{
			LLVolumeParams volume_params = getVolume()->getParams();
			U8 profile_and_hole = volume_params.getProfileParams().getCurveType();
			volume_params.setType(profile_and_hole, LL_PCODE_PATH_FLEXIBLE);
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL LLVOVolume::isSculpted() const
{
	if (getParameterEntryInUse(LLNetworkData::PARAMS_SCULPT))
	{
		return TRUE;
	}
	
	return FALSE;
}

BOOL LLVOVolume::isVolumeGlobal() const
{
	if (mVolumeImpl)
	{
		return mVolumeImpl->isVolumeGlobal() ? TRUE : FALSE;
	}
	return FALSE;
}

BOOL LLVOVolume::canBeFlexible() const
{
	U8 path = getVolume()->getParams().getPathParams().getCurveType();
	return (path == LL_PCODE_PATH_FLEXIBLE || path == LL_PCODE_PATH_LINE);
}

BOOL LLVOVolume::setIsFlexible(BOOL is_flexible)
{
	BOOL res = FALSE;
	BOOL was_flexible = isFlexible();
	LLVolumeParams volume_params;
	if (is_flexible)
	{
		if (!was_flexible)
		{
			volume_params = getVolume()->getParams();
			U8 profile_and_hole = volume_params.getProfileParams().getCurveType();
			volume_params.setType(profile_and_hole, LL_PCODE_PATH_FLEXIBLE);
			res = TRUE;
			setFlags(FLAGS_USE_PHYSICS, FALSE);
			setFlags(FLAGS_PHANTOM, TRUE);
			setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, TRUE, true);
			if (mDrawable)
			{
				mDrawable->makeActive();
			}
		}
	}
	else
	{
		if (was_flexible)
		{
			volume_params = getVolume()->getParams();
			U8 profile_and_hole = volume_params.getProfileParams().getCurveType();
			volume_params.setType(profile_and_hole, LL_PCODE_PATH_LINE);
			res = TRUE;
			setFlags(FLAGS_PHANTOM, FALSE);
			setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, FALSE, true);
		}
	}
	if (res)
	{
		res = setVolume(volume_params, 1);
		if (res)
		{
			markForUpdate(TRUE);
		}
	}
	return res;
}

//----------------------------------------------------------------------------

void LLVOVolume::generateSilhouette(LLSelectNode* nodep, const LLVector3& view_point)
{
	LLVolume *volume = getVolume();

	if (volume)
	{
		LLVector3 view_vector;
		view_vector = view_point; 

		//transform view vector into volume space
		view_vector -= getRenderPosition();
		mDrawable->mDistanceWRTCamera = view_vector.magVec();
		LLQuaternion worldRot = getRenderRotation();
		view_vector = view_vector * ~worldRot;
		if (!isVolumeGlobal())
		{
			LLVector3 objScale = getScale();
			LLVector3 invObjScale(1.f / objScale.mV[VX], 1.f / objScale.mV[VY], 1.f / objScale.mV[VZ]);
			view_vector.scaleVec(invObjScale);
		}
		
		updateRelativeXform();
		LLMatrix4 trans_mat = mRelativeXform;
		if (mDrawable->isStatic())
		{
			trans_mat.translate(getRegion()->getOriginAgent());
		}

		volume->generateSilhouetteVertices(nodep->mSilhouetteVertices, nodep->mSilhouetteNormals, nodep->mSilhouetteSegments, view_vector, trans_mat, mRelativeXformInvTrans);

		nodep->mSilhouetteExists = TRUE;
	}
}

void LLVOVolume::deleteFaces()
{
	S32 face_count = mNumFaces;
	if (mDrawable.notNull())
	{
		mDrawable->deleteFaces(0, face_count);
	}

	mNumFaces = 0;
}

void LLVOVolume::updateRadius()
{
	if (mDrawable.isNull())
	{
		return;
	}
	
	mVObjRadius = getScale().magVec();
	mDrawable->setRadius(mVObjRadius);
}


BOOL LLVOVolume::isAttachment() const
{
	if (mState == 0)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

BOOL LLVOVolume::isHUDAttachment() const
{
	// *NOTE: we assume hud attachment points are in defined range
	// since this range is constant for backwards compatibility
	// reasons this is probably a reasonable assumption to make
	S32 attachment_id = ATTACHMENT_ID_FROM_STATE(mState);
	return ( attachment_id >= 31 && attachment_id <= 38 );
}


const LLMatrix4 LLVOVolume::getRenderMatrix() const
{
	if (mDrawable->isActive() && !mDrawable->isRoot())
	{
		return mDrawable->getParent()->getWorldMatrix();
	}
	return mDrawable->getWorldMatrix();
}

//static
void LLVOVolume::preUpdateGeom()
{
	sNumLODChanges = 0;
}

void LLVOVolume::parameterChanged(U16 param_type, bool local_origin)
{
	LLViewerObject::parameterChanged(param_type, local_origin);
}

void LLVOVolume::parameterChanged(U16 param_type, LLNetworkData* data, BOOL in_use, bool local_origin)
{
	LLViewerObject::parameterChanged(param_type, data, in_use, local_origin);
	if (mVolumeImpl)
	{
		mVolumeImpl->onParameterChanged(param_type, data, in_use, local_origin);
	}
	if (mDrawable.notNull())
	{
		BOOL is_light = getIsLight();
		if (is_light != mDrawable->isState(LLDrawable::LIGHT))
		{
			gPipeline.setLight(mDrawable, is_light);
		}
	}
}

void LLVOVolume::setSelected(BOOL sel)
{
	LLViewerObject::setSelected(sel);
	if (mDrawable.notNull())
	{
		markForUpdate(TRUE);
	}
}

void LLVOVolume::updateSpatialExtents(LLVector3& newMin, LLVector3& newMax)
{		
}

F32 LLVOVolume::getBinRadius()
{
	F32 radius;
	
	const LLVector3* ext = mDrawable->getSpatialExtents();
	
	BOOL shrink_wrap = mDrawable->isAnimating();
	BOOL alpha_wrap = FALSE;

	if (!isHUDAttachment())
	{
		for (S32 i = 0; i < mDrawable->getNumFaces(); i++)
		{
			LLFace* face = mDrawable->getFace(i);
			if (face->getPoolType() == LLDrawPool::POOL_ALPHA &&
				(!LLPipeline::sFastAlpha || face->getVirtualSize() > MIN_ALPHA_SIZE))
			{
				alpha_wrap = TRUE;
				break;
			}
		}
	}
	else
	{
		shrink_wrap = FALSE;
	}

	if (alpha_wrap)
	{
		LLVector3 bounds = getScale();
		radius = llmin(bounds.mV[1], bounds.mV[2]);
		radius = llmin(radius, bounds.mV[0]);
		radius *= 0.5f;
	}
	else if (shrink_wrap)
	{
		radius = (ext[1]-ext[0]).magVec()*0.5f;
	}
	else if (mDrawable->isStatic())
	{
		if (mDrawable->getRadius() < 2.0f)
		{
			radius = 16.f;
		}
		else
		{
			radius = llmax(mDrawable->getRadius(), 32.f);
		}
	}
	else
	{
		radius = 8.f;
	}

	return llclamp(radius, 0.5f, 256.f);
}

const LLVector3 LLVOVolume::getPivotPositionAgent() const
{
	if (mVolumeImpl)
	{
		return mVolumeImpl->getPivotPosition();
	}
	return LLViewerObject::getPivotPositionAgent();
}

void LLVOVolume::onShift(const LLVector3 &shift_vector)
{
	if (mVolumeImpl)
	{
		mVolumeImpl->onShift(shift_vector);
	}

	updateRelativeXform();
}

const LLMatrix4& LLVOVolume::getWorldMatrix(LLXformMatrix* xform) const
{
	if (mVolumeImpl)
	{
		return mVolumeImpl->getWorldMatrix(xform);
	}
	return xform->getWorldMatrix();
}

LLVector3 LLVOVolume::agentPositionToVolume(const LLVector3& pos) const
{
	LLVector3 ret = pos - getRenderPosition();
	ret = ret * ~getRenderRotation();
	LLVector3 objScale = isVolumeGlobal() ? LLVector3(1,1,1) : getScale();
	LLVector3 invObjScale(1.f / objScale.mV[VX], 1.f / objScale.mV[VY], 1.f / objScale.mV[VZ]);
	ret.scaleVec(invObjScale);
	
	return ret;
}

LLVector3 LLVOVolume::agentDirectionToVolume(const LLVector3& dir) const
{
	return dir * ~getRenderRotation();
}

LLVector3 LLVOVolume::volumePositionToAgent(const LLVector3& dir) const
{
	LLVector3 ret = dir;
	ret.scaleVec(getScale());
	ret = ret * getRenderRotation();
	ret += getRenderPosition();
	
	return ret;
}

BOOL LLVOVolume::lineSegmentIntersect(const LLVector3& start, LLVector3& end) const
{
	return FALSE;
	
#if 0 // needs to be rewritten to use face extents instead of volume bounds
	LLVolume* volume = getVolume();
	BOOL ret = FALSE;
	if (volume)
	{	
		LLVector3 v_start, v_end, v_dir;
	
		v_start = agentPositionToVolume(start);
		v_end = agentPositionToVolume(end);
		
		if (LLLineSegmentAABB(v_start, v_end, volume->mBounds[0], volume->mBounds[1]))
		{
			if (volume->lineSegmentIntersect(v_start, v_end) >= 0)
			{
				end = volumePositionToAgent(v_end);
				ret = TRUE;
			}
		}
	}
	return ret;
#endif
}

U32 LLVOVolume::getPartitionType() const
{
	if (isHUDAttachment())
	{
		return LLViewerRegion::PARTITION_HUD;
	}

	return LLViewerRegion::PARTITION_VOLUME;
}

LLVolumePartition::LLVolumePartition()
: LLSpatialPartition(LLVOVolume::VERTEX_DATA_MASK, FALSE)
{
	mLODPeriod = 16;
	mDepthMask = FALSE;
	mDrawableType = LLPipeline::RENDER_TYPE_VOLUME;
	mPartitionType = LLViewerRegion::PARTITION_VOLUME;
	mSlopRatio = 0.25f;
	mBufferUsage = GL_DYNAMIC_DRAW_ARB;
}

LLVolumeBridge::LLVolumeBridge(LLDrawable* drawablep)
: LLSpatialBridge(drawablep, LLVOVolume::VERTEX_DATA_MASK)
{
	mDepthMask = FALSE;
	mLODPeriod = 16;
	mDrawableType = LLPipeline::RENDER_TYPE_VOLUME;
	mPartitionType = LLViewerRegion::PARTITION_BRIDGE;
	
	mBufferUsage = GL_DYNAMIC_DRAW_ARB;

	mSlopRatio = 0.25f;
}

void LLVolumeGeometryManager::registerFace(LLSpatialGroup* group, LLFace* facep, U32 type)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);

	if (facep->getViewerObject()->isSelected() && gHideSelectedObjects)
	{
		return;
	}

	//add face to drawmap
	LLSpatialGroup::drawmap_elem_t& draw_vec = group->mDrawMap[type];	

	S32 idx = draw_vec.size()-1;


	BOOL fullbright = (type == LLRenderPass::PASS_FULLBRIGHT ||
					  type == LLRenderPass::PASS_ALPHA) ? facep->isState(LLFace::FULLBRIGHT) : FALSE;

	const LLMatrix4* tex_mat = NULL;
	if (facep->isState(LLFace::TEXTURE_ANIM) && facep->getVirtualSize() > MIN_TEX_ANIM_SIZE)
	{
		tex_mat = facep->mTextureMatrix;	
	}

	const LLMatrix4* model_mat = NULL;

	LLDrawable* drawable = facep->getDrawable();
	if (drawable->isActive())
	{
		model_mat = &(drawable->getRenderMatrix());
	}
	else
	{
		model_mat = &(drawable->getRegion()->mRenderMatrix);
	}

	U8 bump = (type == LLRenderPass::PASS_BUMP ? facep->getTextureEntry()->getBumpmap() : 0);
	
	LLViewerImage* tex = facep->getTexture();

	U8 glow = 0;
		
	if (type == LLRenderPass::PASS_GLOW)
	{
		glow = (U8) (facep->getTextureEntry()->getGlow() * 255);
	}

	if (idx >= 0 && 
		draw_vec[idx]->mVertexBuffer == facep->mVertexBuffer &&
		draw_vec[idx]->mEnd == facep->getGeomIndex()-1 &&
		(LLPipeline::sTextureBindTest || draw_vec[idx]->mTexture == tex) &&
#if LL_DARWIN
		draw_vec[idx]->mEnd - draw_vec[idx]->mStart + facep->getGeomCount() <= (U32) gGLManager.mGLMaxVertexRange &&
		draw_vec[idx]->mCount + facep->getIndicesCount() <= (U32) gGLManager.mGLMaxIndexRange &&
#endif
		draw_vec[idx]->mGlowColor.mV[3] == glow &&
		draw_vec[idx]->mFullbright == fullbright &&
		draw_vec[idx]->mBump == bump &&
		draw_vec[idx]->mTextureMatrix == tex_mat &&
		draw_vec[idx]->mModelMatrix == model_mat)
	{
		draw_vec[idx]->mCount += facep->getIndicesCount();
		draw_vec[idx]->mEnd += facep->getGeomCount();
		draw_vec[idx]->mVSize = llmax(draw_vec[idx]->mVSize, facep->getVirtualSize());
		validate_draw_info(*draw_vec[idx]);
	}
	else
	{
		U32 start = facep->getGeomIndex();
		U32 end = start + facep->getGeomCount()-1;
		U32 offset = facep->getIndicesStart();
		U32 count = facep->getIndicesCount();
		LLPointer<LLDrawInfo> draw_info = new LLDrawInfo(start,end,count,offset,tex, 
			facep->mVertexBuffer, fullbright, bump); 
		draw_info->mGroup = group;
		draw_info->mVSize = facep->getVirtualSize();
		draw_vec.push_back(draw_info);
		draw_info->mTextureMatrix = tex_mat;
		draw_info->mModelMatrix = model_mat;
		draw_info->mGlowColor.setVec(0,0,0,glow);
		validate_draw_info(*draw_info);
	}
}

void LLVolumeGeometryManager::getGeometry(LLSpatialGroup* group)
{

}

void LLVolumeGeometryManager::rebuildGeom(LLSpatialGroup* group)
{
	if (LLPipeline::sSkipUpdate)
	{
		return;
	}

	if (group->changeLOD())
	{
		group->mLastUpdateDistance = group->mDistance;
	}

	group->mLastUpdateViewAngle = group->mViewAngle;

	if (!group->isState(LLSpatialGroup::GEOM_DIRTY |
						LLSpatialGroup::ALPHA_DIRTY))
	{
		if (group->isState(LLSpatialGroup::MESH_DIRTY))
		{
			S32 num_mapped_veretx_buffer = LLVertexBuffer::sMappedCount ;

			group->mBuilt = 1.f;
			LLFastTimer ftm(LLFastTimer::FTM_REBUILD_VBO);	

			LLFastTimer ftm2(LLFastTimer::FTM_REBUILD_VOLUME_VB);

			for (LLSpatialGroup::element_iter drawable_iter = group->getData().begin(); drawable_iter != group->getData().end(); ++drawable_iter)
			{
				LLDrawable* drawablep = *drawable_iter;

				if (drawablep->isDead() || drawablep->isState(LLDrawable::FORCE_INVISIBLE) )
				{
					continue;
				}

				if (drawablep->isState(LLDrawable::REBUILD_ALL))
				{
					LLVOVolume* vobj = drawablep->getVOVolume();
					vobj->preRebuild();
					LLVolume* volume = vobj->getVolume();
					for (S32 i = 0; i < drawablep->getNumFaces(); ++i)
					{
						LLFace* face = drawablep->getFace(i);
						if (face && face->mVertexBuffer.notNull())
						{
							face->getGeometryVolume(*volume, face->getTEOffset(), 
								vobj->getRelativeXform(), vobj->getRelativeXformInvTrans(), face->getGeomIndex());
						}
					}

					drawablep->clearState(LLDrawable::REBUILD_ALL);
				}
			}
			
			//unmap all the buffers
			for (LLSpatialGroup::buffer_map_t::iterator i = group->mBufferMap.begin(); i != group->mBufferMap.end(); ++i)
			{
				LLSpatialGroup::buffer_list_t& list = i->second;
				for (LLSpatialGroup::buffer_list_t::iterator j = list.begin(); j != list.end(); ++j)
				{
					LLVertexBuffer* buffer = *j;
					if (buffer->isLocked())
					{
						buffer->setBuffer(0);
					}
				}
			}
			
			// don't forget alpha
			if(	group != NULL && 
				!group->mVertexBuffer.isNull() && 
				group->mVertexBuffer->isLocked())
			{
				group->mVertexBuffer->setBuffer(0);
			}

			//if not all buffers are unmapped
			if(num_mapped_veretx_buffer != LLVertexBuffer::sMappedCount) 
			{
				llwarns << "Not all mapped vertex buffers are unmapped!" << llendl ; 
				for (LLSpatialGroup::element_iter drawable_iter = group->getData().begin(); drawable_iter != group->getData().end(); ++drawable_iter)
				{
					LLDrawable* drawablep = *drawable_iter;
					for (S32 i = 0; i < drawablep->getNumFaces(); ++i)
					{
						LLFace* face = drawablep->getFace(i);
						if (face && face->mVertexBuffer.notNull() && face->mVertexBuffer->isLocked())
						{
							face->mVertexBuffer->setBuffer(0) ;
						}
					}
				} 
			}

			group->clearState(LLSpatialGroup::MESH_DIRTY);
		}

		return;
	}

	group->mBuilt = 1.f;
	LLFastTimer ftm(LLFastTimer::FTM_REBUILD_VBO);	

	LLFastTimer ftm2(LLFastTimer::FTM_REBUILD_VOLUME_VB);

	group->clearDrawMap();

	mFaceList.clear();

	std::vector<LLFace*> alpha_faces;
	U32 vertex_count = 0;
	U32 index_count = 0;
	U32 useage = group->mSpatialPartition->mBufferUsage;

	U32 max_vertices = (gSavedSettings.getS32("RenderMaxVBOSize")*1024)/LLVertexBuffer::calcStride(group->mSpatialPartition->mVertexDataMask);
	max_vertices = llmin(max_vertices, (U32) 65535);

	//get all the faces into a list, putting alpha faces in their own list
	for (LLSpatialGroup::element_iter drawable_iter = group->getData().begin(); drawable_iter != group->getData().end(); ++drawable_iter)
	{
		LLDrawable* drawablep = *drawable_iter;
		
		if (drawablep->isDead() || drawablep->isState(LLDrawable::FORCE_INVISIBLE) )
		{
			continue;
		}
	
		if (drawablep->isAnimating())
		{ //fall back to stream draw for animating verts
			useage = GL_STREAM_DRAW_ARB;
		}

		LLVOVolume* vobj = drawablep->getVOVolume();
		vobj->updateTextures();
		vobj->preRebuild();

		//for each face
		for (S32 i = 0; i < drawablep->getNumFaces(); i++)
		{
			//sum up face verts and indices
			drawablep->updateFaceSize(i);
			LLFace* facep = drawablep->getFace(i);
			if (facep->hasGeometry() && facep->mPixelArea > FORCE_CULL_AREA)
			{
				const LLTextureEntry* te = facep->getTextureEntry();
				LLViewerImage* tex = facep->getTexture();

				BOOL force_simple = (facep->mPixelArea < FORCE_SIMPLE_RENDER_AREA);
				U32 type = gPipeline.getPoolTypeFromTE(te, tex);
				if (type != LLDrawPool::POOL_ALPHA && force_simple)
				{
					type = LLDrawPool::POOL_SIMPLE;
				}
				facep->setPoolType(type);

				if (vobj->isHUDAttachment())
				{
					facep->setState(LLFace::FULLBRIGHT);
				}

				if (vobj->mTextureAnimp && vobj->mTexAnimMode)
				{
					if (vobj->mTextureAnimp->mFace <= -1)
					{
						S32 face;
						for (face = 0; face < vobj->getNumTEs(); face++)
						{
							drawablep->getFace(face)->setState(LLFace::TEXTURE_ANIM);
						}
					}
					else if (vobj->mTextureAnimp->mFace < vobj->getNumTEs())
					{
						drawablep->getFace(vobj->mTextureAnimp->mFace)->setState(LLFace::TEXTURE_ANIM);
					}
				}

				if (type == LLDrawPool::POOL_ALPHA)
				{
					BOOL alpha_opt = LLPipeline::sFastAlpha && gPipeline.canUseWindLightShadersOnObjects() && facep->getVirtualSize() < MIN_ALPHA_SIZE;

					const LLColor4& col = facep->getTextureEntry()->getColor();

					if (alpha_opt)
					{ //if we're applying the alpha optimization, only blend faces that have alpha (0.15, 0.5]
					  //for faces with alpha (0.5, 1.0], render with an alpha mask
					  //for faces with alpha [0.0, 0.15], don't render
						if (col.mV[3] > 0.5f)
						{
							mFaceList.push_back(facep);
						}
						else if (col.mV[3] > 0.15f)
						{
							vertex_count += facep->getGeomCount();
							index_count += facep->getIndicesCount();
							alpha_faces.push_back(facep);
						}
						else
						{	//face has no renderable geometry
							facep->mVertexBuffer = NULL;
							facep->mLastVertexBuffer = NULL;
							//don't alpha wrap drawables that have only tiny tiny alpha faces
							facep->setPoolType(LLDrawPool::POOL_SIMPLE);
						}
					}
					else
					{
						vertex_count += facep->getGeomCount();
						index_count += facep->getIndicesCount();
						alpha_faces.push_back(facep);
					}
				}
				else
				{
					if (drawablep->isState(LLDrawable::REBUILD_VOLUME))
					{
						facep->mLastUpdateTime = gFrameTimeSeconds;
					}
					mFaceList.push_back(facep);
				}
			}
			else
			{	//face has no renderable geometry
				facep->mVertexBuffer = NULL;
				facep->mLastVertexBuffer = NULL;
				//don't alpha wrap drawables that have only tiny tiny alpha faces
				facep->setPoolType(LLDrawPool::POOL_SIMPLE);
			}		
		}
	}

	U16 alpha_vertex_count = vertex_count > 65535 ? 65535 : vertex_count;
	U32 alpha_index_count = index_count;

	group->mBufferUsage = useage;

	//PROCESS NON-ALPHA FACES
	{
		//sort faces by things that break batches
		std::sort(mFaceList.begin(), mFaceList.end(), LLFace::CompareBatchBreaker());
					
		std::vector<LLFace*>::iterator face_iter = mFaceList.begin();
		
		LLSpatialGroup::buffer_map_t buffer_map;

		LLViewerImage* last_tex = NULL;
		U32 buffer_index = 0;

		while (face_iter != mFaceList.end())
		{
			//pull off next face
			LLFace* facep = *face_iter;
			LLViewerImage* tex = facep->getTexture();

			if (last_tex == tex)
			{
				buffer_index++;
			}
			else
			{
				last_tex = tex;
				buffer_index = 0;
			}

			U32 index_count = facep->getIndicesCount();
			U32 geom_count = facep->getGeomCount();

			//sum up vertices needed for this texture
			std::vector<LLFace*>::iterator i = face_iter;
			++i;
			
			while (i != mFaceList.end() && 
				(LLPipeline::sTextureBindTest || (*i)->getTexture() == tex))
			{
				facep = *i;
				
				if (geom_count + facep->getGeomCount() > max_vertices)
				{ //cut vertex buffers on geom count too big
					break;
				}

				++i;
				index_count += facep->getIndicesCount();
				geom_count += facep->getGeomCount();
			}
		
			//create/delete/resize vertex buffer if needed
			LLVertexBuffer* buffer = NULL;
			LLSpatialGroup::buffer_map_t::iterator found_iter = group->mBufferMap.find(tex);
			if (found_iter != group->mBufferMap.end())
			{
				if (buffer_index < found_iter->second.size())
				{
					buffer = found_iter->second[buffer_index];
				}
			}
							
			if (!buffer)
			{ //create new buffer if needed
				buffer = createVertexBuffer(group->mSpatialPartition->mVertexDataMask, 
												group->mBufferUsage);
				buffer->allocateBuffer(geom_count, index_count, TRUE);
			}
			else 
			{
				if (LLVertexBuffer::sEnableVBOs && buffer->getUsage() != group->mBufferUsage)
				{
					buffer = createVertexBuffer(group->mSpatialPartition->mVertexDataMask, 
												group->mBufferUsage);
					buffer->allocateBuffer(geom_count, index_count, TRUE);
				}
				else
				{
					buffer->resizeBuffer(geom_count, index_count);
				}
			}

			buffer_map[tex].push_back(buffer);

			//add face geometry

			U32 indices_index = 0;
			U16 index_offset = 0;

			while (face_iter < i)
			{
				facep = *face_iter;
				LLDrawable* drawablep = facep->getDrawable();
				LLVOVolume* vobj = drawablep->getVOVolume();
				LLVolume* volume = vobj->getVolume();

				U32 te_idx = facep->getTEOffset();
				facep->mIndicesIndex = indices_index;
				facep->mGeomIndex = index_offset;
				facep->mVertexBuffer = buffer;
				{
					if (facep->getGeometryVolume(*volume, te_idx, 
						vobj->getRelativeXform(), vobj->getRelativeXformInvTrans(), index_offset))
					{
						buffer->markDirty(facep->getGeomIndex(), facep->getGeomCount(), 
							facep->getIndicesStart(), facep->getIndicesCount());
					}
				}

				index_offset += facep->getGeomCount();
				indices_index += facep->mIndicesCount;

				BOOL force_simple = facep->mPixelArea < FORCE_SIMPLE_RENDER_AREA;
				BOOL fullbright = facep->isState(LLFace::FULLBRIGHT);
				const LLTextureEntry* te = facep->getTextureEntry();

				BOOL is_alpha = facep->getPoolType() == LLDrawPool::POOL_ALPHA ? TRUE : FALSE;

				if (!is_alpha 
					&& gPipeline.canUseWindLightShadersOnObjects()
					&& LLPipeline::sRenderBump 
					&& te->getShiny())
				{
					if (tex->getPrimaryFormat() == GL_ALPHA)
					{
						registerFace(group, facep, LLRenderPass::PASS_INVISI_SHINY);
						registerFace(group, facep, LLRenderPass::PASS_INVISIBLE);
					}
					else if (fullbright)
					{						
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_SHINY);
					}
					else
					{
						registerFace(group, facep, LLRenderPass::PASS_SHINY);
					}
				}
				else
				{
					if (!is_alpha && tex->getPrimaryFormat() == GL_ALPHA)
					{
						registerFace(group, facep, LLRenderPass::PASS_INVISIBLE);
					}
					else if (fullbright)
					{
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT);
					}
					else
					{
						registerFace(group, facep, LLRenderPass::PASS_SIMPLE);
					}
					
					if (!is_alpha && te->getShiny())
					{
						registerFace(group, facep, LLRenderPass::PASS_SHINY);
					}
				}
				
				if (!is_alpha)
				{
					facep->setPoolType(LLDrawPool::POOL_SIMPLE);
					
					if (!force_simple && te->getBumpmap())
					{
						registerFace(group, facep, LLRenderPass::PASS_BUMP);
					}
				}

				if (LLPipeline::sRenderGlow && te->getGlow() > 0.f)
				{
					registerFace(group, facep, LLRenderPass::PASS_GLOW);
				}
							
				++face_iter;
			}

			buffer->setBuffer(0);
		}

		group->mBufferMap.clear();
		for (LLSpatialGroup::buffer_map_t::iterator i = buffer_map.begin(); i != buffer_map.end(); ++i)
		{
			group->mBufferMap[i->first] = i->second;
		}
	}

	//PROCESS ALPHA FACES
	if (!alpha_faces.empty())
	{
		//sort alpha faces by distance
		std::sort(alpha_faces.begin(), alpha_faces.end(), LLFace::CompareDistanceGreater());

		//store alpha faces in root vertex buffer
		if (group->mVertexBuffer.isNull() || (LLVertexBuffer::sEnableVBOs && group->mBufferUsage != group->mVertexBuffer->getUsage()))
		{
			group->mVertexBuffer = createVertexBuffer(group->mSpatialPartition->mVertexDataMask, 
													  group->mBufferUsage);
			group->mVertexBuffer->allocateBuffer(alpha_vertex_count, alpha_index_count, true);
			stop_glerror();
		}
		else
		{
			group->mVertexBuffer->resizeBuffer(alpha_vertex_count, alpha_index_count);
			stop_glerror();
		}

		//get vertex buffer striders
		LLVertexBuffer* buffer = group->mVertexBuffer;

		U32 index_offset = 0;
		U32 indices_index = 0;

		for (std::vector<LLFace*>::iterator i = alpha_faces.begin(); i != alpha_faces.end(); ++i)
		{
			LLFace* facep = *i;

			if (facep->mGeomCount + index_offset > 65535)
			{ //cut off alpha nodes at 64k vertices
				facep->mVertexBuffer = NULL ;
				facep->mLastVertexBuffer = NULL ;
				continue ;
			}

			LLDrawable* drawablep = facep->getDrawable();
			LLVOVolume* vobj = drawablep->getVOVolume();
			LLVolume* volume = vobj->getVolume();

			U32 te_idx = facep->getTEOffset();
			facep->mIndicesIndex = indices_index;
			facep->mGeomIndex = index_offset;
			facep->mVertexBuffer = group->mVertexBuffer;
			if (facep->getGeometryVolume(*volume, te_idx, 
										vobj->getRelativeXform(), vobj->getRelativeXformInvTrans(), 
										index_offset))
			{
				buffer->markDirty(facep->getGeomIndex(), facep->getGeomCount(), 
					facep->getIndicesStart(), facep->getIndicesCount());
			}

			index_offset += facep->getGeomCount();
			indices_index += facep->mIndicesCount;

			registerFace(group, facep, LLRenderPass::PASS_ALPHA);

			if (LLPipeline::sRenderGlow && facep->getTextureEntry()->getGlow() > 0.f)
			{
				registerFace(group, facep, LLRenderPass::PASS_GLOW);
			}				
		}

		buffer->setBuffer(0);
	}
	else
	{
		group->mVertexBuffer = NULL;
	}

	//drawables have been rebuilt, clear rebuild status
	for (LLSpatialGroup::element_iter drawable_iter = group->getData().begin(); drawable_iter != group->getData().end(); ++drawable_iter)
	{
		LLDrawable* drawablep = *drawable_iter;
		drawablep->clearState(LLDrawable::REBUILD_ALL);
	}

	group->mLastUpdateTime = gFrameTimeSeconds;
	group->mBuilt = 1.f;
	group->clearState(LLSpatialGroup::GEOM_DIRTY |
						LLSpatialGroup::ALPHA_DIRTY | LLSpatialGroup::MESH_DIRTY);

	mFaceList.clear();
}

void LLGeometryManager::addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32 &index_count)
{	
	//initialize to default usage for this partition
	U32 usage = group->mSpatialPartition->mBufferUsage;
	
	//clear off any old faces
	mFaceList.clear();

	//for each drawable
	for (LLSpatialGroup::element_iter drawable_iter = group->getData().begin(); drawable_iter != group->getData().end(); ++drawable_iter)
	{
		LLDrawable* drawablep = *drawable_iter;
		
		if (drawablep->isDead())
		{
			continue;
		}
	
		if (drawablep->isAnimating())
		{ //fall back to stream draw for animating verts
			usage = GL_STREAM_DRAW_ARB;
		}

		//for each face
		for (S32 i = 0; i < drawablep->getNumFaces(); i++)
		{
			//sum up face verts and indices
			drawablep->updateFaceSize(i);
			LLFace* facep = drawablep->getFace(i);
			if (facep->hasGeometry() && facep->mPixelArea > FORCE_CULL_AREA)
			{
				vertex_count += facep->getGeomCount();
				index_count += facep->getIndicesCount();

				//remember face (for sorting)
				mFaceList.push_back(facep);
			}
			else
			{
				facep->mVertexBuffer = NULL;
				facep->mLastVertexBuffer = NULL;
			}
		}
	}
	
	group->mBufferUsage = usage;
}

LLHUDPartition::LLHUDPartition()
{
	mPartitionType = LLViewerRegion::PARTITION_HUD;
	mDrawableType = LLPipeline::RENDER_TYPE_HUD;
	mSlopRatio = 0.f;
	mLODPeriod = 16;
}

void LLHUDPartition::shift(const LLVector3 &offset)
{
	//HUD objects don't shift with region crossing.  That would be silly.
}


