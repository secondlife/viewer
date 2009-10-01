/** 
 * @file llvovolume.cpp
 * @brief LLVOVolume class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
#include "llmediadataresponder.h"
#include "llpluginclassmedia.h" // for code in the mediaEvent handler
#include "object_flags.h"
#include "llagentconstants.h"
#include "lldrawable.h"
#include "lldrawpoolbump.h"
#include "llface.h"
#include "llspatialpartition.h"

// TEMP HACK ventrella
#include "llhudmanager.h"
#include "llflexibleobject.h"

#include "llsky.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerregion.h"
#include "llviewertextureanim.h"
#include "llworld.h"
#include "llselectmgr.h"
#include "pipeline.h"
#include "llsdutil.h"
#include "llmediaentry.h"
#include "llmediadatafetcher.h"
#include "llagent.h"

const S32 MIN_QUIET_FRAMES_COALESCE = 30;
const F32 FORCE_SIMPLE_RENDER_AREA = 512.f;
const F32 FORCE_CULL_AREA = 8.f;
const F32 MAX_LOD_DISTANCE = 24.f;
const S32 MAX_SCULPT_REZ = 128;


BOOL gAnimateTextures = TRUE;
//extern BOOL gHideSelectedObjects;

F32 LLVOVolume::sLODFactor = 1.f;
F32	LLVOVolume::sLODSlopDistanceFactor = 0.5f; //Changing this to zero, effectively disables the LOD transition slop 
F32 LLVOVolume::sDistanceFactor = 1.0f;
S32 LLVOVolume::sNumLODChanges = 0;

static LLFastTimer::DeclareTimer FTM_GEN_TRIANGLES("Generate Triangles");
static LLFastTimer::DeclareTimer FTM_GEN_VOLUME("Generate Volumes");

LLVOVolume::LLVOVolume(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
	: LLViewerObject(id, pcode, regionp),
	  mVolumeImpl(NULL)
{
	mTexAnimMode = 0;
	mRelativeXform.setIdentity();
	mRelativeXformInvTrans.setIdentity();

	mLOD = MIN_LOD;
	mSculptLevel = -2;
	mTextureAnimp = NULL;
	mVObjRadius = LLVector3(1,1,0.5f).length();
	mNumFaces = 0;
	mLODChanged = FALSE;
	mSculptChanged = FALSE;
	mSpotLightPriority = 0.f;

	mMediaImplList.resize(getNumTEs());
}

LLVOVolume::~LLVOVolume()
{
	delete mTextureAnimp;
	mTextureAnimp = NULL;
	delete mVolumeImpl;
	mVolumeImpl = NULL;

	if(!mMediaImplList.empty())
	{
		for(U32 i = 0 ; i < mMediaImplList.size() ; i++)
		{
			if(mMediaImplList[i].notNull())
			{
				mMediaImplList[i]->removeObject(this) ;
			}
		}
	}
}


// static
void LLVOVolume::initClass()
{
	LLMediaDataFetcher::initClass();
}

// static
void LLVOVolume::cleanupClass()
{
	LLMediaDataFetcher::cleanupClass();
}

U32 LLVOVolume::processUpdateMessage(LLMessageSystem *mesgsys,
										  void **user_data,
										  U32 block_num, EObjectUpdateType update_type,
										  LLDataPacker *dp)
{
	LLColor4U color;
	const S32 teDirtyBits = (TEM_CHANGE_TEXTURE|TEM_CHANGE_COLOR|TEM_CHANGE_MEDIA);

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
		S32 result = unpackTEMessage(mesgsys, _PREHASH_ObjectData, block_num);
		if (result & teDirtyBits)
		{
			updateTEData();
		}
		if (result & TEM_CHANGE_MEDIA)
		{
			retval |= MEDIA_FLAGS_CHANGED;
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
			else 
			{
				if (res2 & teDirtyBits) 
				{
					updateTEData();
				}
				if (res2 & TEM_CHANGE_MEDIA)
				{
					retval |= MEDIA_FLAGS_CHANGED;
				}
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
				S32 result = unpackTEMessage(tdp);
				if (result & teDirtyBits)
				{
					updateTEData();
				}
				if (result & TEM_CHANGE_MEDIA)
				{
					retval |= MEDIA_FLAGS_CHANGED;
				}
			}
		}
	}
	if (retval & (MEDIA_URL_REMOVED | MEDIA_URL_ADDED | MEDIA_URL_UPDATED | MEDIA_FLAGS_CHANGED)) {
		// If the media changed at all, request new media data
		if(mMedia)
		{
			llinfos << "Media URL: " << mMedia->mMediaURL << llendl;
		}
		requestMediaDataUpdate();
	}
	// ...and clean up any media impls
	cleanUpMediaImpls();

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

			if (!facep->mTextureMatrix)
			{
				facep->mTextureMatrix = new LLMatrix4();
			}

			LLMatrix4& tex_mat = *facep->mTextureMatrix;
			tex_mat.setIdentity();
			LLVector3 trans ;

			if(facep->isAtlasInUse())
			{
				//
				//if use atlas for animated texture
				//apply the following transform to the animation matrix.
				//

				F32 tcoord_xoffset = 0.f ;
				F32 tcoord_yoffset = 0.f ;
				F32 tcoord_xscale = 1.f ;
				F32 tcoord_yscale = 1.f ;			
				if(facep->isAtlasInUse())
				{
					const LLVector2* tmp = facep->getTexCoordOffset() ;
					tcoord_xoffset = tmp->mV[0] ; 
					tcoord_yoffset = tmp->mV[1] ;

					tmp = facep->getTexCoordScale() ;
					tcoord_xscale = tmp->mV[0] ; 
					tcoord_yscale = tmp->mV[1] ;	
				}
				trans.set(LLVector3(tcoord_xoffset + tcoord_xscale * (off_s+0.5f), tcoord_yoffset + tcoord_yscale * (off_t+0.5f), 0.f));

				tex_mat.translate(LLVector3(-(tcoord_xoffset + tcoord_xscale * 0.5f), -(tcoord_yoffset + tcoord_yscale * 0.5f), 0.f));
			}
			else	//non atlas
			{
				trans.set(LLVector3(off_s+0.5f, off_t+0.5f, 0.f));			
				tex_mat.translate(LLVector3(-0.5f, -0.5f, 0.f));
			}

			LLVector3 scale(scale_s, scale_t, 1.f);			
			LLQuaternion quat;
			quat.setQuat(rot, 0, 0, -1.f);
		
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
	if (mDrawable.notNull() && mTextureUpdateTimer.getElapsedTimeF32() > TEXTURE_AREA_REFRESH_TIME)
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
	
	if (LLViewerTexture::sDontLoadVolumeTextures || mDrawable.isNull()) // || !mDrawable->isVisible())
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
		LLViewerTexture *imagep = face->getTexture();
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
			imagep->setBoostLevel(LLViewerTexture::BOOST_HUD);
 			face->setPixelArea(area); // treat as full screen
		}
		else
		{
			vsize = getTextureVirtualSize(face);
		}

		mPixelArea = llmax(mPixelArea, face->getPixelArea());

		F32 old_size = face->getVirtualSize();

		if (face->mTextureMatrix != NULL)
		{
			if ((vsize < MIN_TEX_ANIM_SIZE && old_size > MIN_TEX_ANIM_SIZE) ||
				(vsize > MIN_TEX_ANIM_SIZE && old_size < MIN_TEX_ANIM_SIZE))
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
			LLViewerFetchedTexture* img = LLViewerTextureManager::staticCastToFetchedTexture(imagep) ;
			if(img)
			{
				F32 pri = img->getDecodePriority();
				pri = llmax(pri, 0.0f);
				if (pri < min_vsize) min_vsize = pri;
				if (pri > max_vsize) max_vsize = pri;
			}
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
		mSculptTexture = LLViewerTextureManager::getFetchedTexture(id, TRUE, FALSE, LLViewerTexture::LOD_TEXTURE);
		if (mSculptTexture.notNull())
		{
			S32 lod = llmin(mLOD, 3);
			F32 lodf = ((F32)(lod + 1.0f)/4.f); 
			F32 tex_size = lodf * MAX_SCULPT_REZ;
			mSculptTexture->addTextureStats(2.f * tex_size * tex_size);
			mSculptTexture->setBoostLevel(llmax((S32)mSculptTexture->getBoostLevel(),
												(S32)LLViewerTexture::BOOST_SCULPTED));
			mSculptTexture->setForSculpt() ;
		}

		S32 texture_discard = mSculptTexture->getDiscardLevel(); //try to match the texture
		S32 current_discard = mSculptLevel;

		if (texture_discard >= 0 && //texture has some data available
			(texture_discard < current_discard || //texture has more data than last rebuild
			current_discard < 0)) //no previous rebuild
		{
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, FALSE);
			mSculptChanged = TRUE;
		}

		if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SCULPTED))
			{
				setDebugText(llformat("T%d C%d V%d\n%dx%d",
									  texture_discard, current_discard, getVolume()->getSculptLevel(),
									  mSculptTexture->getHeight(), mSculptTexture->getWidth()));
			}
	}

	if (getLightTextureID().notNull())
	{
		LLLightImageParams* params = (LLLightImageParams*) getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
		LLUUID id = params->getLightTexture();
		mLightTexture = LLViewerTextureManager::getFetchedTexture(id);
		if (mLightTexture.notNull())
		{
			F32 rad = getLightRadius();
			mLightTexture->addTextureStats(gPipeline.calcPixelArea(getPositionAgent(), 
																	LLVector3(rad,rad,rad),
																	*LLViewerCamera::getInstance()));
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
	F32 texel_area = (tdim * 0.5f).lengthSquared()*3.14159f;
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
	gGL.getTexUnit(0)->bind(getTEImage(face));
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
	LLViewerTexture* imagep = getTEImage(f);
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
	bool force_update = true; // avoid non-alpha mDistance update being optimized away
	mDrawable->updateDistance(*LLViewerCamera::getInstance(), force_update);

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
			mSculptTexture = LLViewerTextureManager::getFetchedTexture(volume_params.getSculptID(), TRUE, FALSE, LLViewerTexture::LOD_TEXTURE);
			if (mSculptTexture.notNull())
			{
				//ignore sculpt GL usage since bao fixed this in a separate branch
				if (!gGLActive)
				{
					gGLActive = TRUE;
					sculpt();
					gGLActive = FALSE;
				}
				else
				{
					sculpt();
				}
				mSculptLevel = getVolume()->getSculptLevel();
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
		else if (current_discard > MAX_DISCARD_LEVEL)
		{
			llwarns << "WARNING!!: Current discard of sculpty at " << current_discard 
				<< " is more than than allowed max of " << MAX_DISCARD_LEVEL << llendl;
			
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

			if(LLViewerTextureManager::sTesterp)
			{
				LLViewerTextureManager::sTesterp->updateGrayTextureBinding();
			}
		}
		else
		{
			if (raw_image->getDataSize() < sculpt_height * sculpt_width * sculpt_components)
				llerrs << "Sculpt: image data size = " << raw_image->getDataSize()
					   << " < " << sculpt_height << " x " << sculpt_width << " x " <<sculpt_components << llendl;
					   
			sculpt_data = raw_image->getData();

			if(LLViewerTextureManager::sTesterp)
			{
				mSculptTexture->updateBindStatsForTester() ;
			}
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
	//if (mDrawable->isVisible())
	//{
	//	updateTextures();
	//}

	S32 cur_detail = 0;
	
	F32 radius = getVolume()->mLODScaleBias.scaledVec(getScale()).length();
	F32 distance = llmin(mDrawable->mDistanceWRTCamera, MAX_LOD_DISTANCE);
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

BOOL LLVOVolume::setParent(LLViewerObject* parent)
{
	BOOL ret = FALSE ;
	if (parent != getParent())
	{
		ret = LLViewerObject::setParent(parent);
		if (ret && mDrawable)
		{
			gPipeline.markMoved(mDrawable);
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
		}
	}

	return ret ;
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

static LLFastTimer::DeclareTimer FTM_GEN_FLEX("Generate Flexies");
static LLFastTimer::DeclareTimer FTM_UPDATE_PRIMITIVES("Update Primitives");

BOOL LLVOVolume::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer t(FTM_UPDATE_PRIMITIVES);
	
	if (mVolumeImpl != NULL)
	{
		BOOL res;
		{
			LLFastTimer t(FTM_GEN_FLEX);
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
			LLFastTimer ftm(FTM_GEN_VOLUME);
			LLVolumeParams volume_params = getVolume()->getParams();
			setVolume(volume_params, 0);
			drawable->setState(LLDrawable::REBUILD_VOLUME);
		}

		{
			LLFastTimer t(FTM_GEN_TRIANGLES);
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
			LLFastTimer ftm(FTM_GEN_VOLUME);
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
				LLFastTimer t(FTM_GEN_TRIANGLES);
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
		LLFastTimer t(FTM_GEN_TRIANGLES);
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

//virtual
void LLVOVolume::setNumTEs(const U8 num_tes)
{
	const U8 old_num_tes = getNumTEs() ;
	
	if(old_num_tes && old_num_tes < num_tes) //new faces added
	{
		LLViewerObject::setNumTEs(num_tes) ;

		if(mMediaImplList.size() >= old_num_tes && mMediaImplList[old_num_tes -1].notNull())//duplicate the last media textures if exists.
		{
			mMediaImplList.resize(num_tes) ;
			const LLTextureEntry* te = getTE(old_num_tes - 1) ;
			for(U8 i = old_num_tes; i < num_tes ; i++)
			{
				setTE(i, *te) ;
				mMediaImplList[i] = mMediaImplList[old_num_tes -1] ;
			}
			mMediaImplList[old_num_tes -1]->setUpdated(TRUE) ;
		}
	}
	else if(old_num_tes > num_tes && mMediaImplList.size() > num_tes) //old faces removed
	{
		U8 end = mMediaImplList.size() ;
		for(U8 i = num_tes; i < end ; i++)
		{
			removeMediaImpl(i) ;				
		}
		mMediaImplList.resize(num_tes) ;

		LLViewerObject::setNumTEs(num_tes) ;
	}
	else
	{
		LLViewerObject::setNumTEs(num_tes) ;
	}

	return ;
}

void LLVOVolume::setTEImage(const U8 te, LLViewerTexture *imagep)
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
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		llwarns << "No texture entry for te " << (S32)te << ", object " << mID << llendl;
	}
	else if (color != tep->getColor())
	{
		if (color.mV[3] != tep->getColor().mV[3])
		{
			gPipeline.markTextured(mDrawable);
		}
		retval = LLPrimitive::setTEColor(te, color);
		if (mDrawable.notNull() && retval)
		{
			// These should only happen on updates which are not the initial update.
			mDrawable->setState(LLDrawable::REBUILD_COLOR);
			dirtyMesh();
		}
	}

	return  retval;
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

S32 LLVOVolume::setTEMediaTexGen(const U8 te, const U8 media)
{
	S32 res = LLViewerObject::setTEMediaTexGen(te, media);
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

S32 LLVOVolume::setTEBumpShinyFullbright(const U8 te, const U8 bump)
{
	S32 res = LLViewerObject::setTEBumpShinyFullbright(te, bump);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
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
	/*if (mDrawable.notNull())
	{
		mFaceMappingChanged = TRUE;
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_MATERIAL, TRUE);
	}*/
}

bool LLVOVolume::hasMedia() const
{
	bool result = false;
	const U8 numTEs = getNumTEs();
	for (U8 i = 0; i < numTEs; i++)
	{
		const LLTextureEntry* te = getTE(i);
		if(te->hasMedia())
		{
			result = true;
			break;
		}
	}
	return result;
}

void LLVOVolume::requestMediaDataUpdate()
{
	LLMediaDataFetcher::fetchMedia(this);
}

void LLVOVolume::cleanUpMediaImpls()
{
	// Iterate through our TEs and remove any Impls that are no longer used
	const U8 numTEs = getNumTEs();
	for (U8 i = 0; i < numTEs; i++)
	{
		const LLTextureEntry* te = getTE(i);
		if( ! te->hasMedia())
		{
			// Delete the media IMPL!
			removeMediaImpl(i) ;
		}
	}
}

void LLVOVolume::updateObjectMediaData(const LLSD &media_data_array)
{
	// media_data_array is an array of media entry maps

	//llinfos << "updating:" << this->getID() << " " << ll_pretty_print_sd(media_data_array) << llendl;

	LLSD::array_const_iterator iter = media_data_array.beginArray();
	LLSD::array_const_iterator end = media_data_array.endArray();
	U8 texture_index = 0;
	for (; iter != end; ++iter, ++texture_index)
	{
		syncMediaData(texture_index, *iter, false/*merge*/, false/*ignore_agent*/);
	}
}

void LLVOVolume::syncMediaData(S32 texture_index, const LLSD &media_data, bool merge, bool ignore_agent)
{
	LLTextureEntry *te = getTE(texture_index);
	//llinfos << "BEFORE: texture_index = " << texture_index
	//	<< " hasMedia = " << te->hasMedia() << " : " 
	//	<< ((NULL == te->getMediaData()) ? "NULL MEDIA DATA" : ll_pretty_print_sd(te->getMediaData()->asLLSD())) << llendl;

	std::string previous_url;
	LLMediaEntry* mep = te->getMediaData();
	if(mep)
	{
		// Save the "current url" from before the update so we can tell if
		// it changes. 
		previous_url = mep->getCurrentURL();
	}

	if (merge)
	{
		te->mergeIntoMediaData(media_data);
	}
	else {
		// XXX Question: what if the media data is undefined LLSD, but the
		// update we got above said that we have media flags??	Here we clobber
		// that, assuming the data from the service is more up-to-date. 
		te->updateMediaData(media_data);
	}

	mep = te->getMediaData();
	if(mep)
	{
		bool update_from_self = false;
		if (!ignore_agent) 
		{
			LLUUID updating_agent = LLTextureEntry::getAgentIDFromMediaVersionString(getMediaURL());
			update_from_self = (updating_agent == gAgent.getID());
		}
		viewer_media_t media_impl = LLViewerMedia::updateMediaImpl(mep, previous_url, update_from_self);
			
		addMediaImpl(media_impl, texture_index) ;
	}

	//llinfos << "AFTER: texture_index = " << texture_index
	//	<< " hasMedia = " << te->hasMedia() << " : " 
	//	<< ((NULL == te->getMediaData()) ? "NULL MEDIA DATA" : ll_pretty_print_sd(te->getMediaData()->asLLSD())) << llendl;
}

void LLVOVolume::mediaEvent(LLViewerMediaImpl *impl, LLPluginClassMedia* plugin, LLViewerMediaObserver::EMediaEvent event)
{
	switch(event)
	{
		
		case LLViewerMediaObserver::MEDIA_EVENT_LOCATION_CHANGED:
		{			
			switch(impl->getNavState())
			{
				case LLViewerMediaImpl::MEDIANAVSTATE_FIRST_LOCATION_CHANGED:
				{
					// This is the first location changed event after the start of a non-server-directed nav.  It may need to be broadcast.

					bool block_navigation = false;
					// FIXME: if/when we allow the same media impl to be used by multiple faces, the logic here will need to be fixed
					// to deal with multiple face indices.
					int face_index = getFaceIndexWithMediaImpl(impl, -1);
					std::string new_location = plugin->getLocation();
					
					// Find the media entry for this navigate
					LLMediaEntry* mep = NULL;
					LLTextureEntry *te = getTE(face_index);
					if(te)
					{
						mep = te->getMediaData();
					}
					
					if(mep)
					{
						if(!mep->checkCandidateUrl(new_location))
						{
							block_navigation = true;
						}
					}
					else
					{
						llwarns << "Couldn't find media entry!" << llendl;
					}
										
					if(block_navigation)
					{
						llinfos << "blocking navigate to URI " << new_location << llendl;

						// "bounce back" to the current URL from the media entry
						// NOTE: the only way block_navigation can be true is if we found the media entry, so we're guaranteed here that mep is not NULL.
						impl->navigateTo(mep->getCurrentURL());
					}
					else
					{
						
						llinfos << "broadcasting navigate with URI " << new_location << llendl;

						// Post the navigate to the cap
						std::string cap = getRegion()->getCapability("ObjectMediaNavigate");
						if(cap.empty())
						{
							// XXX *TODO: deal with no cap!	 It may happen! (retry?)
							LL_WARNS("Media") << "Can't broadcast navigate event -- ObjectMediaNavigate cap is not available" << LL_ENDL;
							return;
						}
						
						// If we got here, the cap is available.  Index through all faces that have this media and send the navigate message.
						LLSD sd;
						sd["object_id"] = mID;
						sd["current_url"] = new_location;
						sd["texture_index"] = face_index;
						LLHTTPClient::post(cap, sd, new LLMediaDataResponder("ObjectMediaNavigate", sd, this));
					}
				}
				break;
				
				case LLViewerMediaImpl::MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED:
					// This is the first location changed event after the start of a server-directed nav.  Don't broadcast it.
					llinfos << "	NOT broadcasting navigate (server-directed)" << llendl;
				break;
				
				default:
					// This is a subsequent location-changed due to a redirect.	 Don't broadcast.
					llinfos << "	NOT broadcasting navigate (redirect)" << llendl;
				break;
			}
		}
		break;
		
		default:
		break;
	}

}

void LLVOVolume::sendMediaDataUpdate() const
{
	std::string url = getRegion()->getCapability("ObjectMedia");
	if (!url.empty())
	{
		LLSD sd_payload;
		sd_payload["verb"] = "UPDATE";
		sd_payload[LLTextureEntry::OBJECT_ID_KEY] = mID;
		LLSD object_media_data;
		for (int i=0; i < getNumTEs(); i++) {
			LLTextureEntry *texture_entry = getTE(i);
			llassert((texture_entry->getMediaData() != NULL) == texture_entry->hasMedia());
			const LLSD &media_data =  
				(texture_entry->getMediaData() == NULL) ? LLSD() : texture_entry->getMediaData()->asLLSD();
			object_media_data.append(media_data);
		}
		sd_payload[LLTextureEntry::OBJECT_MEDIA_DATA_KEY] = object_media_data;

		llinfos << "Sending media data: " << getID() << " " << ll_pretty_print_sd(sd_payload) << llendl;

		LLHTTPClient::post(url, sd_payload, new LLMediaDataResponder("ObjectMedia", sd_payload, this));
	}
	// XXX *TODO: deal with no cap!	 It may happen! (retry?)
}

void LLVOVolume::removeMediaImpl(S32 texture_index)
{
	if(mMediaImplList.size() <= (U32)texture_index || mMediaImplList[texture_index].isNull())
	{
		return ;
	}

	//make the face referencing to mMediaImplList[texture_index] to point back to the old texture.
	if(mDrawable)
	{
		LLFace* facep = mDrawable->getFace(texture_index) ;
		if(facep)
		{
			LLViewerMediaTexture* media_tex = LLViewerTextureManager::findMediaTexture(mMediaImplList[texture_index]->getMediaTextureID()) ;
			if(media_tex)
			{
				media_tex->removeMediaFromFace(facep) ;
			}
		}
	}		
	
	//check if some other face(s) of this object reference(s)to this media impl.
	S32 i ;
	S32 end = (S32)mMediaImplList.size() ;
	for(i = 0; i < end ; i++)
	{
		if( i != texture_index && mMediaImplList[i] == mMediaImplList[texture_index])
		{
			break ;
		}
	}

	if(i == end) //this object does not need this media impl.
	{
		mMediaImplList[texture_index]->removeObject(this) ;
	}

	mMediaImplList[texture_index] = NULL ;
	return ;
}

void LLVOVolume::addMediaImpl(LLViewerMediaImpl* media_impl, S32 texture_index)
{
	if((S32)mMediaImplList.size() < texture_index + 1)
	{
		mMediaImplList.resize(texture_index + 1) ;
	}
	
	if(mMediaImplList[texture_index].notNull())
	{
		if(mMediaImplList[texture_index] == media_impl)
		{
			return ;
		}

		removeMediaImpl(texture_index) ;
	}

	mMediaImplList[texture_index] = media_impl;
	media_impl->addObject(this) ;	

	//add the face to show the media if it is in playing
	if(mDrawable)
	{
		LLFace* facep = mDrawable->getFace(texture_index) ;
		if(facep)
		{
			LLViewerMediaTexture* media_tex = LLViewerTextureManager::findMediaTexture(mMediaImplList[texture_index]->getMediaTextureID()) ;
			if(media_tex)
			{
				media_tex->addMediaToFace(facep) ;
			}
		}
		else //the face is not available now, start media on this face later.
		{
			media_impl->setUpdated(TRUE) ;
		}
	}
	return ;
}

viewer_media_t LLVOVolume::getMediaImpl(U8 face_id) const
{
	if(mMediaImplList.size() > face_id)
	{
		return mMediaImplList[face_id];
	}
	return NULL;
}

S32 LLVOVolume::getFaceIndexWithMediaImpl(const LLViewerMediaImpl* media_impl, S32 start_face_id)
{
	S32 end = (S32)mMediaImplList.size() ;
	for(S32 face_id = start_face_id + 1; face_id < end; face_id++)
	{
		if(mMediaImplList[face_id] == media_impl)
		{
			return face_id ;
		}
	}
	return -1 ;
}

//----------------------------------------------------------------------------

void LLVOVolume::setLightTextureID(LLUUID id)
{
	if (id.notNull())
	{
		if (!hasLightTexture())
		{
			setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE, TRUE, true);
		}
		LLLightImageParams* param_block = (LLLightImageParams*) getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
		if (param_block && param_block->getLightTexture() != id)
		{
			param_block->setLightTexture(id);
			parameterChanged(LLNetworkData::PARAMS_LIGHT_IMAGE, true);
		}
	}
	else
	{
		if (hasLightTexture())
		{
			setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE, FALSE, true);
			mLightTexture = NULL;
		}
	}		
}

void LLVOVolume::setSpotLightParams(LLVector3 params)
{
	LLLightImageParams* param_block = (LLLightImageParams*) getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
	if (param_block && param_block->getParams() != params)
	{
		param_block->setParams(params);
		parameterChanged(LLNetworkData::PARAMS_LIGHT_IMAGE, true);
	}
}
		
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

LLUUID LLVOVolume::getLightTextureID() const
{
	if (getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE))
	{
		const LLLightImageParams *param_block = (const LLLightImageParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
		if (param_block)
		{
			return param_block->getLightTexture();
		}
	}
	
	return LLUUID::null;
}


LLVector3 LLVOVolume::getSpotLightParams() const
{
	if (getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE))
	{
		const LLLightImageParams *param_block = (const LLLightImageParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
		if (param_block)
		{
			return param_block->getParams();
		}
	}
	
	return LLVector3();
}

F32 LLVOVolume::getSpotLightPriority() const
{
	return mSpotLightPriority;
}

void LLVOVolume::updateSpotLightPriority()
{
	LLVector3 pos = mDrawable->getPositionAgent();
	LLVector3 at(0,0,-1);
	at *= getRenderRotation();

	F32 r = getLightRadius()*0.5f;

	pos += at * r;

	at = LLViewerCamera::getInstance()->getAtAxis();

	pos -= at * r;
	
	mSpotLightPriority = gPipeline.calcPixelArea(pos, LLVector3(r,r,r), *LLViewerCamera::getInstance());

	if (mLightTexture.notNull())
	{
		mLightTexture->addTextureStats(mSpotLightPriority);
		mLightTexture->setBoostLevel(LLViewerTexture::BOOST_CLOUDS);
	}
}


LLViewerTexture* LLVOVolume::getLightTexture()
{
	LLUUID id = getLightTextureID();

	if (id.notNull())
	{
		if (mLightTexture.isNull() || id != mLightTexture->getID())
		{
			mLightTexture = LLViewerTextureManager::getFetchedTexture(id);
		}
	}
	else
	{
		mLightTexture = NULL;
	}

	return mLightTexture;
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

BOOL LLVOVolume::hasLightTexture() const
{
	if (getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE))
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
		mDrawable->mDistanceWRTCamera = view_vector.length();
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

		volume->generateSilhouetteVertices(nodep->mSilhouetteVertices, nodep->mSilhouetteNormals, nodep->mSilhouetteSegments, view_vector, trans_mat, mRelativeXformInvTrans, nodep->getTESelectMask());

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
	
	mVObjRadius = getScale().length();
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
				(!LLPipeline::sFastAlpha || 
				face->getFaceColor().mV[3] != 1.f ||
				!face->getTexture()->getIsAlphaMask()))
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
		radius = (ext[1]-ext[0]).length()*0.5f;
	}
	else if (mDrawable->isStatic())
	{
		/*if (mDrawable->getRadius() < 2.0f)
		{
			radius = 16.f;
		}
		else
		{
			radius = llmax(mDrawable->getRadius(), 32.f);
		}*/

		radius = (((S32) mDrawable->getRadius())/2+1)*8;
	}
	else if (mDrawable->getVObj()->isAttachment())
	{
		radius = (((S32) (mDrawable->getRadius()*4)+1))*2;
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
	LLVector3 ret = dir * ~getRenderRotation();
	
	LLVector3 objScale = isVolumeGlobal() ? LLVector3(1,1,1) : getScale();
	ret.scaleVec(objScale);

	return ret;
}

LLVector3 LLVOVolume::volumePositionToAgent(const LLVector3& dir) const
{
	LLVector3 ret = dir;
	LLVector3 objScale = isVolumeGlobal() ? LLVector3(1,1,1) : getScale();
	ret.scaleVec(objScale);
	ret = ret * getRenderRotation();
	ret += getRenderPosition();
	
	return ret;
}

LLVector3 LLVOVolume::volumeDirectionToAgent(const LLVector3& dir) const
{
	LLVector3 ret = dir;
	LLVector3 objScale = isVolumeGlobal() ? LLVector3(1,1,1) : getScale();
	LLVector3 invObjScale(1.f / objScale.mV[VX], 1.f / objScale.mV[VY], 1.f / objScale.mV[VZ]);
	ret.scaleVec(invObjScale);
	ret = ret * getRenderRotation();

	return ret;
}


BOOL LLVOVolume::lineSegmentIntersect(const LLVector3& start, const LLVector3& end, S32 face, BOOL pick_transparent, S32 *face_hitp,
									  LLVector3* intersection,LLVector2* tex_coord, LLVector3* normal, LLVector3* bi_normal)
	
{
	if (!mbCanSelect 
		|| mDrawable->isDead() 
		|| !gPipeline.hasRenderType(mDrawable->getRenderType()))
	{
		return FALSE;
	}

	BOOL ret = FALSE;

	LLVolume* volume = getVolume();
	if (volume)
	{	
		LLVector3 v_start, v_end, v_dir;
	
		v_start = agentPositionToVolume(start);
		v_end = agentPositionToVolume(end);
		
		LLVector3 p;
		LLVector3 n;
		LLVector2 tc;
		LLVector3 bn;

		if (intersection != NULL)
		{
			p = *intersection;
		}

		if (tex_coord != NULL)
		{
			tc = *tex_coord;
		}

		if (normal != NULL)
		{
			n = *normal;
		}

		if (bi_normal != NULL)
		{
			bn = *bi_normal;
		}

		S32 face_hit = -1;

		S32 start_face, end_face;
		if (face == -1)
		{
			start_face = 0;
			end_face = volume->getNumFaces();
		}
		else
		{
			start_face = face;
			end_face = face+1;
		}

		for (S32 i = start_face; i < end_face; ++i)
		{
			face_hit = volume->lineSegmentIntersect(v_start, v_end, i,
													&p, &tc, &n, &bn);
			
			if (face_hit >= 0 && mDrawable->getNumFaces() > face_hit)
			{
				LLFace* face = mDrawable->getFace(face_hit);
			
				if (pick_transparent || !face->getTexture() || !face->getTexture()->hasGLTexture() || face->getTexture()->getMask(face->surfaceToTexture(tc, p, n)))
				{
					v_end = p;
					if (face_hitp != NULL)
					{
						*face_hitp = face_hit;
					}
					
					if (intersection != NULL)
					{
						*intersection = volumePositionToAgent(p);  // must map back to agent space
					}

					if (normal != NULL)
					{
						*normal = volumeDirectionToAgent(n);
						(*normal).normVec();
					}

					if (bi_normal != NULL)
					{
						*bi_normal = volumeDirectionToAgent(bn);
						(*bi_normal).normVec();
					}

					if (tex_coord != NULL)
					{
						*tex_coord = tc;
					}
					
					ret = TRUE;
				}
			}
		}
	}
		
	return ret;
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
: LLSpatialPartition(LLVOVolume::VERTEX_DATA_MASK, TRUE, GL_DYNAMIC_DRAW_ARB)
{
	mLODPeriod = 32;
	mDepthMask = FALSE;
	mDrawableType = LLPipeline::RENDER_TYPE_VOLUME;
	mPartitionType = LLViewerRegion::PARTITION_VOLUME;
	mSlopRatio = 0.25f;
	mBufferUsage = GL_DYNAMIC_DRAW_ARB;
}

LLVolumeBridge::LLVolumeBridge(LLDrawable* drawablep)
: LLSpatialBridge(drawablep, TRUE, LLVOVolume::VERTEX_DATA_MASK)
{
	mDepthMask = FALSE;
	mLODPeriod = 32;
	mDrawableType = LLPipeline::RENDER_TYPE_VOLUME;
	mPartitionType = LLViewerRegion::PARTITION_BRIDGE;
	
	mBufferUsage = GL_DYNAMIC_DRAW_ARB;

	mSlopRatio = 0.25f;
}

void LLVolumeGeometryManager::registerFace(LLSpatialGroup* group, LLFace* facep, U32 type)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);

	if (facep->getViewerObject()->isSelected() && LLSelectMgr::getInstance()->mHideSelectedObjects)
	{
		return;
	}

	//add face to drawmap
	LLSpatialGroup::drawmap_elem_t& draw_vec = group->mDrawMap[type];	

	S32 idx = draw_vec.size()-1;


	BOOL fullbright = (type == LLRenderPass::PASS_FULLBRIGHT) ||
					  (type == LLRenderPass::PASS_INVISIBLE) ||
					  (type == LLRenderPass::PASS_ALPHA ? facep->isState(LLFace::FULLBRIGHT) : FALSE);

	if (!fullbright && type != LLRenderPass::PASS_GLOW && !facep->mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_NORMAL))
	{
		llwarns << "Non fullbright face has no normals!" << llendl;
		return;
	}

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
	
	LLViewerTexture* tex = facep->getTexture();

	U8 glow = 0;
		
	if (type == LLRenderPass::PASS_GLOW)
	{
		glow = (U8) (facep->getTextureEntry()->getGlow() * 255);
	}

	if (facep->mVertexBuffer.isNull())
	{
		llerrs << "WTF?" << llendl;
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
		update_min_max(draw_vec[idx]->mExtents[0], draw_vec[idx]->mExtents[1], facep->mExtents[0]);
		update_min_max(draw_vec[idx]->mExtents[0], draw_vec[idx]->mExtents[1], facep->mExtents[1]);
	}
	else
	{
		U32 start = facep->getGeomIndex();
		U32 end = start + facep->getGeomCount()-1;
		U32 offset = facep->getIndicesStart();
		U32 count = facep->getIndicesCount();
		LLPointer<LLDrawInfo> draw_info = new LLDrawInfo(start,end,count,offset, tex, 
			facep->mVertexBuffer, fullbright, bump); 
		draw_info->mGroup = group;
		draw_info->mVSize = facep->getVirtualSize();
		draw_vec.push_back(draw_info);
		draw_info->mTextureMatrix = tex_mat;
		draw_info->mModelMatrix = model_mat;
		draw_info->mGlowColor.setVec(0,0,0,glow);
		if (type == LLRenderPass::PASS_ALPHA)
		{ //for alpha sorting
			facep->setDrawInfo(draw_info);
		}
		draw_info->mExtents[0] = facep->mExtents[0];
		draw_info->mExtents[1] = facep->mExtents[1];
		validate_draw_info(*draw_info);
	}
}

void LLVolumeGeometryManager::getGeometry(LLSpatialGroup* group)
{

}

static LLFastTimer::DeclareTimer FTM_REBUILD_VOLUME_VB("Volume");
static LLFastTimer::DeclareTimer FTM_REBUILD_VBO("VBO Rebuilt");

void LLVolumeGeometryManager::rebuildGeom(LLSpatialGroup* group)
{
	if (group->changeLOD())
	{
		group->mLastUpdateDistance = group->mDistance;
	}

	group->mLastUpdateViewAngle = group->mViewAngle;

	if (!group->isState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::ALPHA_DIRTY))
	{
		if (group->isState(LLSpatialGroup::MESH_DIRTY) && !LLPipeline::sDelayVBUpdate)
		{
			LLFastTimer ftm(FTM_REBUILD_VBO);	
			LLFastTimer ftm2(FTM_REBUILD_VOLUME_VB);
		
			rebuildMesh(group);
		}
		return;
	}

	group->mBuilt = 1.f;
	LLFastTimer ftm(FTM_REBUILD_VBO);	

	LLFastTimer ftm2(FTM_REBUILD_VOLUME_VB);

	group->clearDrawMap();

	mFaceList.clear();

	std::vector<LLFace*> fullbright_faces;
	std::vector<LLFace*> bump_faces;
	std::vector<LLFace*> simple_faces;

	std::vector<LLFace*> alpha_faces;
	U32 useage = group->mSpatialPartition->mBufferUsage;

	U32 max_vertices = (gSavedSettings.getS32("RenderMaxVBOSize")*1024)/LLVertexBuffer::calcStride(group->mSpatialPartition->mVertexDataMask);
	U32 max_total = (gSavedSettings.getS32("RenderMaxNodeSize")*1024)/LLVertexBuffer::calcStride(group->mSpatialPartition->mVertexDataMask);
	max_vertices = llmin(max_vertices, (U32) 65535);

	U32 cur_total = 0;

	//get all the faces into a list
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
		llassert_always(vobj);
		vobj->updateTextures();
		vobj->preRebuild();

		drawablep->clearState(LLDrawable::HAS_ALPHA);

		//for each face
		for (S32 i = 0; i < drawablep->getNumFaces(); i++)
		{
			//sum up face verts and indices
			drawablep->updateFaceSize(i);
			LLFace* facep = drawablep->getFace(i);

			if (cur_total > max_total)
			{
				facep->mVertexBuffer = NULL;
				facep->mLastVertexBuffer = NULL;
				continue;
			}

			cur_total += facep->getGeomCount();

			if (facep->hasGeometry() && facep->mPixelArea > FORCE_CULL_AREA)
			{
				const LLTextureEntry* te = facep->getTextureEntry();
				LLViewerTexture* tex = facep->getTexture();

				if (facep->isState(LLFace::TEXTURE_ANIM))
				{
					if (!vobj->mTexAnimMode)
					{
						facep->clearState(LLFace::TEXTURE_ANIM);
					}
				}

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
					if (LLPipeline::sFastAlpha &&
					    (te->getColor().mV[VW] == 1.0f) &&
					    (!te->getFullbright()) && // hack: alpha masking renders fullbright faces invisible, need to figure out why - for now, avoid
					    facep->getTexture()->getIsAlphaMask())
					{ //can be treated as alpha mask
						simple_faces.push_back(facep);
					}
					else
					{
						drawablep->setState(LLDrawable::HAS_ALPHA);
						alpha_faces.push_back(facep);
					}
				}
				else
				{
					if (drawablep->isState(LLDrawable::REBUILD_VOLUME))
					{
						facep->mLastUpdateTime = gFrameTimeSeconds;
					}

					if (gPipeline.canUseWindLightShadersOnObjects()
						&& LLPipeline::sRenderBump)
					{
						if (te->getBumpmap())
						{ //needs normal + binormal
							bump_faces.push_back(facep);
						}
						else if (te->getShiny() || !te->getFullbright())
						{ //needs normal
							simple_faces.push_back(facep);
						}
						else 
						{ //doesn't need normal
							facep->setState(LLFace::FULLBRIGHT);
							fullbright_faces.push_back(facep);
						}
					}
					else
					{
						if (te->getBumpmap() && LLPipeline::sRenderBump)
						{ //needs normal + binormal
							bump_faces.push_back(facep);
						}
						else if ((te->getShiny() && LLPipeline::sRenderBump) ||
							!te->getFullbright())
						{ //needs normal
							simple_faces.push_back(facep);
						}
						else 
						{ //doesn't need normal
							facep->setState(LLFace::FULLBRIGHT);
							fullbright_faces.push_back(facep);
						}
					}
				}
			}
			else
			{	//face has no renderable geometry
				facep->mVertexBuffer = NULL;
				facep->mLastVertexBuffer = NULL;
			}		
		}
	}

	group->mBufferUsage = useage;

	//PROCESS NON-ALPHA FACES
	U32 simple_mask = LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR;
	U32 alpha_mask = simple_mask | 0x80000000; //hack to give alpha verts their own VBO
	U32 bump_mask = LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR;
	U32 fullbright_mask = LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR;

	if (LLPipeline::sRenderDeferred)
	{
		bump_mask |= LLVertexBuffer::MAP_BINORMAL;
	}

	genDrawInfo(group, simple_mask, simple_faces);
	genDrawInfo(group, bump_mask, bump_faces);
	genDrawInfo(group, fullbright_mask, fullbright_faces);
	genDrawInfo(group, alpha_mask, alpha_faces, TRUE);

	if (!LLPipeline::sDelayVBUpdate)
	{
		//drawables have been rebuilt, clear rebuild status
		for (LLSpatialGroup::element_iter drawable_iter = group->getData().begin(); drawable_iter != group->getData().end(); ++drawable_iter)
		{
			LLDrawable* drawablep = *drawable_iter;
			drawablep->clearState(LLDrawable::REBUILD_ALL);
		}
	}

	group->mLastUpdateTime = gFrameTimeSeconds;
	group->mBuilt = 1.f;
	group->clearState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::ALPHA_DIRTY);

	if (LLPipeline::sDelayVBUpdate)
	{
		group->setState(LLSpatialGroup::MESH_DIRTY | LLSpatialGroup::NEW_DRAWINFO);
	}

	mFaceList.clear();
}

static LLFastTimer::DeclareTimer FTM_VOLUME_GEOM("Volume Geometry");
void LLVolumeGeometryManager::rebuildMesh(LLSpatialGroup* group)
{
	if (group->isState(LLSpatialGroup::MESH_DIRTY) && !group->isState(LLSpatialGroup::GEOM_DIRTY))
	{
		LLFastTimer tm(FTM_VOLUME_GEOM);
		S32 num_mapped_veretx_buffer = LLVertexBuffer::sMappedCount ;

		group->mBuilt = 1.f;
		
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
			LLSpatialGroup::buffer_texture_map_t& map = i->second;
			for (LLSpatialGroup::buffer_texture_map_t::iterator j = map.begin(); j != map.end(); ++j)
			{
				LLSpatialGroup::buffer_list_t& list = j->second;
				for (LLSpatialGroup::buffer_list_t::iterator k = list.begin(); k != list.end(); ++k)
				{
					LLVertexBuffer* buffer = *k;
					if (buffer->isLocked())
					{
						buffer->setBuffer(0);
					}
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

		group->clearState(LLSpatialGroup::MESH_DIRTY | LLSpatialGroup::NEW_DRAWINFO);
	}

	if (group->isState(LLSpatialGroup::NEW_DRAWINFO))
	{
		llerrs << "WTF?" << llendl;
	}
}

void LLVolumeGeometryManager::genDrawInfo(LLSpatialGroup* group, U32 mask, std::vector<LLFace*>& faces, BOOL distance_sort)
{
	//calculate maximum number of vertices to store in a single buffer
	U32 max_vertices = (gSavedSettings.getS32("RenderMaxVBOSize")*1024)/LLVertexBuffer::calcStride(group->mSpatialPartition->mVertexDataMask);
	max_vertices = llmin(max_vertices, (U32) 65535);

	if (!distance_sort)
	{
		//sort faces by things that break batches
		std::sort(faces.begin(), faces.end(), LLFace::CompareBatchBreaker());
	}
	else
	{
		//sort faces by distance
		std::sort(faces.begin(), faces.end(), LLFace::CompareDistanceGreater());
	}
				
	std::vector<LLFace*>::iterator face_iter = faces.begin();
	
	LLSpatialGroup::buffer_map_t buffer_map;

	LLViewerTexture* last_tex = NULL;
	S32 buffer_index = 0;

	if (distance_sort)
	{
		buffer_index = -1;
	}

	while (face_iter != faces.end())
	{
		//pull off next face
		LLFace* facep = *face_iter;
		LLViewerTexture* tex = facep->getTexture();

		if (distance_sort)
		{
			tex = NULL;
		}

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
		
		while (i != faces.end() && 
			(LLPipeline::sTextureBindTest || (distance_sort || (*i)->getTexture() == tex)))
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
		LLSpatialGroup::buffer_texture_map_t::iterator found_iter = group->mBufferMap[mask].find(tex);
		
		if (found_iter != group->mBufferMap[mask].end())
		{
			if ((U32) buffer_index < found_iter->second.size())
			{
				buffer = found_iter->second[buffer_index];
			}
		}
						
		if (!buffer)
		{ //create new buffer if needed
			buffer = createVertexBuffer(mask, 
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

		buffer_map[mask][tex].push_back(buffer);

		//add face geometry

		U32 indices_index = 0;
		U16 index_offset = 0;

		while (face_iter < i)
		{
			facep = *face_iter;
			facep->mIndicesIndex = indices_index;
			facep->mGeomIndex = index_offset;
			facep->mVertexBuffer = buffer;
			{
				facep->updateRebuildFlags();
				if (!LLPipeline::sDelayVBUpdate)
				{
					LLDrawable* drawablep = facep->getDrawable();
					LLVOVolume* vobj = drawablep->getVOVolume();
					LLVolume* volume = vobj->getVolume();

					U32 te_idx = facep->getTEOffset();

					if (facep->getGeometryVolume(*volume, te_idx, 
						vobj->getRelativeXform(), vobj->getRelativeXformInvTrans(), index_offset))
					{
						buffer->markDirty(facep->getGeomIndex(), facep->getGeomCount(), 
							facep->getIndicesStart(), facep->getIndicesCount());
					}
				}
			}

			index_offset += facep->getGeomCount();
			indices_index += facep->mIndicesCount;

			BOOL force_simple = facep->mPixelArea < FORCE_SIMPLE_RENDER_AREA;
			BOOL fullbright = facep->isState(LLFace::FULLBRIGHT);
			if ((mask & LLVertexBuffer::MAP_NORMAL) == 0)
			{ //paranoia check to make sure GL doesn't try to read non-existant normals
				fullbright = TRUE;
			}

			const LLTextureEntry* te = facep->getTextureEntry();

			BOOL is_alpha = facep->getPoolType() == LLDrawPool::POOL_ALPHA ? TRUE : FALSE;
		
			if (is_alpha)
			{
				// can we safely treat this as an alpha mask?
				if (LLPipeline::sFastAlpha &&
				    (te->getColor().mV[VW] == 1.0f) &&
				    (!te->getFullbright()) && // hack: alpha masking renders fullbright faces invisible, need to figure out why - for now, avoid
				    facep->getTexture()->getIsAlphaMask())
				{
					if (te->getFullbright())
					{
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK);
					}
					else
					{
						registerFace(group, facep, LLRenderPass::PASS_ALPHA_MASK);
					}
				}
				else
				{
					registerFace(group, facep, LLRenderPass::PASS_ALPHA);
				}

				if (LLPipeline::sRenderDeferred)
				{
					registerFace(group, facep, LLRenderPass::PASS_ALPHA_SHADOW);
				}
			}
			else if (gPipeline.canUseVertexShaders()
				&& group->mSpatialPartition->mPartitionType != LLViewerRegion::PARTITION_HUD 
				&& LLPipeline::sRenderBump 
				&& te->getShiny())
			{
				if (tex->getPrimaryFormat() == GL_ALPHA)
				{
					registerFace(group, facep, LLRenderPass::PASS_INVISI_SHINY);
					registerFace(group, facep, LLRenderPass::PASS_INVISIBLE);
				}
				else if (LLPipeline::sRenderDeferred)
				{
					if (te->getBumpmap())
					{
						registerFace(group, facep, LLRenderPass::PASS_BUMP);
					}
					else if (te->getFullbright())
					{
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_SHINY);
					}
					else
					{
						llassert(mask & LLVertexBuffer::MAP_NORMAL);
						registerFace(group, facep, LLRenderPass::PASS_SIMPLE);
					}
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
					if (LLPipeline::sRenderDeferred && te->getBumpmap())
					{
						registerFace(group, facep, LLRenderPass::PASS_BUMP);
					}
					else
					{
						llassert(mask & LLVertexBuffer::MAP_NORMAL);
						registerFace(group, facep, LLRenderPass::PASS_SIMPLE);
					}
				}
				
				if (!is_alpha && te->getShiny() && LLPipeline::sRenderBump)
				{
					registerFace(group, facep, LLRenderPass::PASS_SHINY);
				}
			}
			
			if (!is_alpha && !LLPipeline::sRenderDeferred)
			{
				llassert((mask & LLVertexBuffer::MAP_NORMAL) || fullbright);
				facep->setPoolType((fullbright) ? LLDrawPool::POOL_FULLBRIGHT : LLDrawPool::POOL_SIMPLE);
				
				if (!force_simple && te->getBumpmap() && LLPipeline::sRenderBump)
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

	group->mBufferMap[mask].clear();
	for (LLSpatialGroup::buffer_texture_map_t::iterator i = buffer_map[mask].begin(); i != buffer_map[mask].end(); ++i)
	{
		group->mBufferMap[mask][i->first] = i->second;
	}
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
	mLODPeriod = 1;
}

void LLHUDPartition::shift(const LLVector3 &offset)
{
	//HUD objects don't shift with region crossing.  That would be silly.
}


