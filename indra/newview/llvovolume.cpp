/** 
 * @file llvovolume.cpp
 * @brief LLVOVolume class implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
#include "lldrawpoolsimple.h"
#include "lldrawpoolbump.h"
#include "llface.h"

// TEMP HACK ventrella
#include "llhudmanager.h"
#include "llflexibleobject.h"
#include "llanimalcontrols.h"

#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewerregion.h"
#include "llviewertextureanim.h"
#include "llworld.h"
#include "llselectmgr.h"
#include "pipeline.h"

const S32 MIN_QUIET_FRAMES_COALESCE = 30;

//#define	LLDEBUG_DISPLAY_LODS 1

BOOL gAnimateTextures = TRUE;

F32 LLVOVolume::sLODFactor = 1.f;
F32	LLVOVolume::sLODSlopDistanceFactor = 0.5f; //Changing this to zero, effectively disables the LOD transition slop 
F32	LLVOVolume::sLODComplexityDistanceBias = 0.0f;//Changing this to zero makes all prims LOD equally regardless of complexity
F32 LLVOVolume::sDistanceFactor = 1.0f;
S32 LLVOVolume::sNumLODChanges = 0;

LLVOVolume::LLVOVolume(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
	: LLViewerObject(id, pcode, regionp),
	  mVolumeImpl(NULL)
{
	mRelativeXform.identity();
	mRelativeXformInvTrans.identity();

	mLOD = MIN_LOD;
	mInited = FALSE;
	mAllTEsSame = FALSE;
	mTextureAnimp = NULL;
	mGlobalVolume = FALSE;

	mTextureAnimp = NULL;
	mAllTEsSame = FALSE;
	mVObjRadius = LLVector3(1,1,0.5f).magVec();
	mNumFaces = 0;
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
				mTextureAnimp->unpackTAMessage(mesgsys, block_num);
			}
			else
			{
				delete mTextureAnimp;
				mTextureAnimp = NULL;
			}

			// Unpack volume data
			LLVolumeParams volume_params;
			LLVolumeMessage::unpackVolumeParams(&volume_params, mesgsys, _PREHASH_ObjectData, block_num);

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
				char mask[LL_MAX_PATH];
				sprintf(mask, "%s*.slc", gDirUtilp->getDirDelimiter().c_str());
				gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"").c_str(),mask);
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
				mTextureAnimp->unpackTAMessage(*dp);
			}
			else
			{
				delete mTextureAnimp;
				mTextureAnimp = NULL;
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


BOOL LLVOVolume::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	LLViewerObject::idleUpdate(agent, world, time);

	///////////////////////
	//
	// Do texture animation stuff
	//

	if (mTextureAnimp && gAnimateTextures)
	{
		F32 off_s, off_t, scale_s, scale_t, rot;
		S32 result;
		if ((result = mTextureAnimp->animateTextures(off_s, off_t, scale_s, scale_t, rot)))
		{
			U8 has_bump = 0;
			if (mTextureAnimp->mFace <= -1)
			{
				S32 face;
				for (face = 0; face < getNumTEs(); face++)
				{
					if (result & LLViewerTextureAnim::TRANSLATE)
					{
						setTEOffset(face, off_s, off_t);
					}
					if (result & LLViewerTextureAnim::SCALE)
					{
						setTEScale(face, scale_s, scale_t);
					}
					if (result & LLViewerTextureAnim::ROTATE)
					{
						setTERotation(face, rot);
					}
					has_bump |= getTE(face)->getBumpmap();
				}
			}
			else if (mTextureAnimp->mFace < getNumTEs())
			{
				if (result & LLViewerTextureAnim::TRANSLATE)
				{
					setTEOffset(mTextureAnimp->mFace, off_s, off_t);
				}
				if (result & LLViewerTextureAnim::SCALE)
				{
					setTEScale(mTextureAnimp->mFace, scale_s, scale_t);
				}
				if (result & LLViewerTextureAnim::ROTATE)
				{
					setTERotation(mTextureAnimp->mFace, rot);
				}
				has_bump |= getTE(mTextureAnimp->mFace)->getBumpmap();
			}
// 			mFaceMappingChanged = TRUE;
			if (mDrawable->isVisible())
			{
				gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD, TRUE);
			}
		}
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

}

//static
F32 LLVOVolume::getTextureVirtualSize(const LLFace* face)
{
	//LLVector2 tdim = face->mTexExtents[1] - face->mTexExtents[0];
	//F32 pixel_area = 1.f/llmin(llmax(tdim.mV[0] * tdim.mV[1], 1.f), 10.f);
	LLVector3 cross_vec = (face->mExtents[1] - face->mExtents[0]);
	

	LLVector3 lookAt = (face->getPositionAgent()-gCamera->getOrigin());
	F32 dist = lookAt.normVec();

	F32 face_area;	
	
	if (face->isState(LLFace::GLOBAL))
	{
		face_area = cross_vec.mV[0]*cross_vec.mV[1]*fabsf(lookAt.mV[2]) +  
					cross_vec.mV[1]*cross_vec.mV[2]*fabsf(lookAt.mV[0]) +
					cross_vec.mV[0]*cross_vec.mV[2]*fabsf(lookAt.mV[1]);
	}
	else
	{
		face_area = cross_vec.mV[0]*cross_vec.mV[1] + 
					cross_vec.mV[1]*cross_vec.mV[2] +
					cross_vec.mV[0]*cross_vec.mV[2];
	}

	if (face_area <= 0)
	{
		return 0.f;
	}

	F32 view = llmax(lookAt*gCamera->getAtAxis(), 0.5f);
	F32 dist_ramp = dist * view/face_area;
	//ramp down distance for things closer than 16 m * lookAt
	dist /= dist_ramp;
	dist *= dist;
	dist *= dist_ramp;

	F32 dist_ratio = face_area / llmax(dist, 0.1f);
	F32 pixel_area = dist_ratio*gCamera->getScreenPixelArea();
	
	return view*pixel_area;
}

void LLVOVolume::updateTextures(S32 lod)
{
	// Update the image levels of all textures...
	// First we do some quick checks.

	// This doesn't take into account whether the object is in front
	// or behind...

	if (LLViewerImage::sDontLoadVolumeTextures || mDrawable.isNull() || !mDrawable->isVisible())
	{
		return;
	}
		
	const S32 num_faces = mDrawable->getNumFaces();
	
	for (S32 i = 0; i < num_faces; i++)
	{
		const LLFace* face = mDrawable->getFace(i);
		const LLTextureEntry *te = face->getTextureEntry();
		LLViewerImage *imagep = face->getTexture();

		if (!imagep || !te)
		{
			continue;
		}
		
		F32 vsize;
		
		if (isHUDAttachment())
		{
			vsize = (F32) (imagep->getWidth(0) * imagep->getHeight(0));
			imagep->setBoostLevel(LLViewerImage::BOOST_HUD);
		}
		else
		{
			vsize = getTextureVirtualSize(face);
		}

		imagep->addTextureStats(vsize);


		U8 bump = te->getBumpmap();
		if( te && bump)
		{
			gBumpImageList.addTextureStats( bump, imagep->getID(), vsize, 1, 1);
		}
	}
}

BOOL LLVOVolume::isActive() const
{
	return !mStatic || mTextureAnimp || isAttachment() || (mVolumeImpl && mVolumeImpl->isActive());
}

BOOL LLVOVolume::setMaterial(const U8 material)
{
	BOOL res = LLViewerObject::setMaterial(material);
	if (res)
	{
		// for deprecated LL_MCODE_LIGHT
		if (mDrawable.notNull())
		{
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_LIGHTING, TRUE);
		}
	}
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
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	}
}

LLFace* LLVOVolume::addFace(S32 f)
{
	const LLTextureEntry* te = getTE(f);
	LLViewerImage* imagep = getTEImage(f);
	LLDrawPool* poolp;
	if (isHUDAttachment())
	{
		poolp = gPipeline.getPool(LLDrawPool::POOL_HUD);
	}
	else
	{
		poolp = LLPipeline::getPoolFromTE(te, imagep);
	}
	return mDrawable->addFace(poolp, imagep);
}

LLDrawable *LLVOVolume::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_VOLUME);

	S32 max_tes_to_set = calcAllTEsSame() ? 1 : getNumTEs();
	for (S32 i = 0; i < max_tes_to_set; i++)
	{
		LLFace* face = addFace(i);
		// JC - should there be a setViewerObject(this) call here?
		face->setTEOffset(i);
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
	mDrawable->updateDistance(*gCamera);

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
	mGlobalVolume = (mVolumeImpl && mVolumeImpl->isVolumeGlobal());
	
	//MSMSM Recompute LOD here in case the object was just created,
	// its LOD might be incorrectly set to minumum detail...
	calcLOD();

	if (LLPrimitive::setVolume(volume_params, mLOD, (mVolumeImpl && mVolumeImpl->isVolumeUnique())))
	{
		mFaceMappingChanged = TRUE;

		if (mVolumeImpl)
		{
			mVolumeImpl->onSetVolume(volume_params, detail);
		}

		return TRUE;
	}
	return FALSE;
}


F32	LLVOVolume::computeLODProfilePathComplexityBias(){
	//compute a complexity cost from 0 to 1.0 where the 'simplest' prim has a cost of 0.0
	// and the 'heaviest' prim has a cost of 1.0
//	LLVolume*	volume = getVolume();
	F32			complexity = 0.0f;
//	const LLVolumeParams&	params = volume->getParams();
//	U8 type = volume->getPathType();
//	U8 pcode = this->getPCode();
//	U8 proftype = volume->getProfileType();

	//if(params.getHollow()>0.0f){// || (proftype == 1) || (proftype == 0)){
		//If it is hollow, or a cube/pyramid(subdivided), the complexity is roughly doubled
	//	complexity+=0.5f;
	//}

	if(this->getVolume()->getProfile().mParams.getCurveType()==LL_PCODE_PROFILE_SQUARE &&
		this->getVolume()->getPath().mParams.getCurveType()==LL_PCODE_PATH_LINE)
	{
		//Object is a cube so bias it heavily since cubes are subdivided alot.
//		this->setDebugText("CUBE");
		complexity += 1.0f;
	}

//	if(params.getTwist() != params.getTwistBegin()){
		//if there is twist.. the complexity is bumped
//		complexity+=0.25f;
//	}
//	if(type != LL_PCODE_PATH_LINE)//If the path is not a line it is more complex
//		complexity+=0.2f;
	return complexity * sLODComplexityDistanceBias;
}

S32	LLVOVolume::computeLODDetail(F32 distance, F32 radius)
{
	S32	cur_detail;
	// We've got LOD in the profile, and in the twist.  Use radius.
	F32 tan_angle = (LLVOVolume::sLODFactor*radius)/distance;
	cur_detail = LLVolumeLODGroup::getDetailFromTan(tan_angle);
	return cur_detail;
}

BOOL LLVOVolume::calcLOD()
{
	if (mDrawable.isNull())
	{
		return FALSE;
	}
	S32 cur_detail = 0;
	/*if (isHUDAttachment())
	{
		cur_detail = LLVolumeLODGroup::NUM_LODS-1; // max detail
	}
	else*/
	{	
		F32 radius = (mVolumep->mLODScaleBias.scaledVec(getScale())).magVec();
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
		else
		{
			//Now adjust the computed distance by some factor based on the geometric complexity of the primitive
			distance += computeLODProfilePathComplexityBias();
		}
		// Compensate for field of view changing on FOV zoom.
		distance *= gCamera->getView();

		cur_detail = computeLODDetail(distance, radius);

		//update textures with what the real LOD is
		updateTextures(cur_detail);

		if(cur_detail != mLOD)
		{
			// Here we test whether the LOD is increasing or decreasing to introduce a slop factor
			if(cur_detail < mLOD)
			{
				// Viewer is moving away from the object
				// so bias our LOD by adding a fixed amount to the distance.
				// This will reduce the problem of LOD twitching when the 
				// user makes slight movements near the LOD transition threshhold.
				F32	test_distance = distance - (distance*sLODSlopDistanceFactor/(1.0f+sLODFactor));
				if(test_distance < 0.0f) test_distance = 0.0f;
				S32	potential_detail = computeLODDetail( test_distance, radius );
				if(potential_detail >= mLOD )
				{	//The LOD has truly not changed
					cur_detail = mLOD;
				}
			}
		}
	}

	if (cur_detail != mLOD)
	{
		mAppAngle = (F32) atan2( mDrawable->getRadius(), mDrawable->mDistanceWRTCamera) * RAD_TO_DEG;
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
	
#if LLDEBUG_DISPLAY_LODS
	//MS Enable this to display LOD numbers on objects
	std::ostringstream msg;
	msg << cur_detail;//((cur_detail<mLOD)?"-":cur_detail==mLOD?"=":"+") << (int)cur_detail << " , " << mDrawable->mDistanceWRTCamera << " , " << ((LLVOVolume::sLODFactor*mVObjRadius)/mDrawable->mDistanceWRTCamera);
	this->setDebugText(msg.str());
#endif
	
	if (lod_changed)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, FALSE);
		mLODChanged = TRUE;
	}

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
		// parent is dynamic, so I'll need to share its drawable, must rebuild to share drawables
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, TRUE);

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
		LLFace *face = mDrawable->getFace(i + mFaceIndexOffset);
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
		if (getAllTEsSame())
		{
			break; // only 1 face
		}
	}
}

// NOTE: regenFaces() MUST be followed by genTriangles()!
void LLVOVolume::regenFaces()
{
	// remove existing faces
	// use mDrawable->getVOVolume() in case of shared drawables
	mDrawable->getVOVolume()->deleteFaces(this);
	mFaceIndexOffset = mDrawable->getNumFaces();
	// add new faces
	mNumFaces = getAllTEsSame() ? 1 : getNumTEs();
	for (S32 i = 0; i < mNumFaces; i++)
	{
		LLFace* facep = addFace(i);
		facep->setViewerObject(this);
		facep->setTEOffset(i);
	}
	// Need to do this as texture entries may not correspond to faces any more!
	mDrawable->updateTexture();
	gPipeline.markMaterialed(mDrawable);
}

BOOL LLVOVolume::genTriangles(BOOL force_global)
{
	BOOL res = TRUE;

	LLVector3 min,max;

	if (getAllTEsSame())
	{
		setupSingleFace(mFaceIndexOffset);
		LLFace *face = mDrawable->getFace(mFaceIndexOffset);
		S32 num_faces = getVolume()->getNumFaces();
		res = face->genVolumeTriangles(*getVolume(), 0, num_faces-1,
									   mRelativeXform, mRelativeXformInvTrans,
									   mGlobalVolume | force_global);
		
		if (mDrawable->isState(LLDrawable::REBUILD_VOLUME))
		{
			min = face->mExtents[0];
			max = face->mExtents[1];
		}
		mWereAllTEsSame = TRUE;
	}
	else
	{
		for (S32 i = 0; i < getVolume()->getNumFaces(); i++)
		{
			LLFace *face = mDrawable->getFace(i + mFaceIndexOffset);
			res &= face->genVolumeTriangles(*getVolume(), i,
											mRelativeXform, mRelativeXformInvTrans,
											mGlobalVolume | force_global);
			
			if (mDrawable->isState(LLDrawable::REBUILD_VOLUME))
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
		mWereAllTEsSame = FALSE;
	}

	if (mDrawable->isState(LLDrawable::REBUILD_VOLUME))
	{
		mDrawable->setSpatialExtents(min,max);
		if (!isVolumeGlobal())
		{
			mDrawable->setPositionGroup((min+max)*0.5f);	
		}
		else
		{
			mDrawable->setPositionGroup(getPosition());
		}

		updateRadius();
		mDrawable->updateBinRadius();
		mDrawable->movePartition();
	}
	
	return res;
}

void LLVOVolume::updateRelativeXform(BOOL global_volume)
{
	if (mVolumeImpl)
	{
		mVolumeImpl->updateRelativeXform(global_volume);
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
				
		x_axis.normVec();
		y_axis.normVec();
		z_axis.normVec();
		
		mRelativeXformInvTrans.setRows(x_axis, y_axis, z_axis);
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
		
		LLViewerRegion* region = getRegion();
		pos += region->getOriginAgent();
		
		LLVector3 x_axis = LLVector3(scale.mV[VX], 0.f, 0.f) * rot;
		LLVector3 y_axis = LLVector3(0.f, scale.mV[VY], 0.f) * rot;
		LLVector3 z_axis = LLVector3(0.f, 0.f, scale.mV[VZ]) * rot;

		mRelativeXform.initRows(LLVector4(x_axis, 0.f),
								LLVector4(y_axis, 0.f),
								LLVector4(z_axis, 0.f),
								LLVector4(pos, 1.f));

		x_axis.normVec();
		y_axis.normVec();
		z_axis.normVec();
																					
		mRelativeXformInvTrans.setRows(x_axis, y_axis, z_axis);
	}
}

BOOL LLVOVolume::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer t(LLFastTimer::FTM_UPDATE_PRIMITIVES);

	if (mVolumeImpl != NULL)
	{
		LLFastTimer t(LLFastTimer::FTM_GEN_FLEX);
		BOOL res = mVolumeImpl->doUpdateGeometry(drawable);
		updateFaceFlags();
		if (res)
		{
			drawable->clearState(LLDrawable::REBUILD_GEOMETRY);
		}

		return res;
	}
	
	BOOL compiled = FALSE;
	BOOL change_shared = FALSE;
		
	updateRelativeXform();
	
	if (mDrawable.isNull()) // Not sure why this is happening, but it is...
	{
		return TRUE; // No update to complete
	}

	calcAllTEsSame();
	
	if (mVolumeChanged || mFaceMappingChanged || change_shared)
	{
		compiled = TRUE;
		mInited = TRUE;

		{
			LLFastTimer ftm(LLFastTimer::FTM_GEN_VOLUME);
			LLVolumeParams volume_params = getVolume()->getParams();
			setVolume(volume_params, 0);
		}
		drawable->setState(LLDrawable::REBUILD_GEOMETRY);
		if (mVolumeChanged || change_shared)
		{
			drawable->setState(LLDrawable::REBUILD_LIGHTING);
		}

		{
			LLFastTimer t(LLFastTimer::FTM_GEN_TRIANGLES);
			regenFaces();
			genTriangles(FALSE);
		}
	}
	else if (mLODChanged)
	{
		LLPointer<LLVolume> old_volumep, new_volumep;
		F32 old_lod, new_lod;

		old_volumep = getVolume();
		old_lod = old_volumep->getDetail();

		{
			LLFastTimer ftm(LLFastTimer::FTM_GEN_VOLUME);
			LLVolumeParams volume_params = getVolume()->getParams();
			setVolume(volume_params, 0);
		}
		new_volumep = getVolume();
		new_lod = new_volumep->getDetail();

		if (new_lod != old_lod)
		{
			compiled = TRUE;
			sNumLODChanges += (getAllTEsSame() ? 1 : getVolume()->getNumFaces());
	
			drawable->setState(LLDrawable::REBUILD_ALL); // for face->genVolumeTriangles()

			{
				LLFastTimer t(LLFastTimer::FTM_GEN_TRIANGLES);
				if (new_volumep->getNumFaces() != old_volumep->getNumFaces())
				{
					regenFaces();
				}
				genTriangles(FALSE);
			}
		}
	}
	// it has its own drawable (it's moved) or it has changed UVs or it has changed xforms from global<->local
	else
	{
		compiled = TRUE;
		// All it did was move or we changed the texture coordinate offset
		LLFastTimer t(LLFastTimer::FTM_GEN_TRIANGLES);
		genTriangles(FALSE);
	}

	// Update face flags
	updateFaceFlags();
	
	if(compiled)
	{
		LLPipeline::sCompiles++;
	}
	
	mVolumeChanged = FALSE;
	mLODChanged = FALSE;
	mFaceMappingChanged = FALSE;

	drawable->clearState(LLDrawable::REBUILD_GEOMETRY);

	return TRUE;
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
//	llinfos << "SetTEImage:" << llendl;
	BOOL changed = (mTEImages[te] != imagep);
	LLViewerObject::setTEImage(te, imagep);
	if (mDrawable.notNull())
	{
		if (changed)
		{
			calcAllTEsSame();
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, TRUE);
			mFaceMappingChanged = TRUE;
		}
	}
}

S32 LLVOVolume::setTETexture(const U8 te, const LLUUID &uuid)
{
	BOOL changed = (uuid != getTE(te)->getID() || (uuid == LLUUID::null));

	S32 res = LLViewerObject::setTETexture(te, uuid);
	if (mDrawable.notNull())
	{
		if (changed)
		{
			calcAllTEsSame();
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, TRUE);
			mFaceMappingChanged = TRUE;
		}
	}
	return res;
}

S32 LLVOVolume::setTEColor(const U8 te, const LLColor4 &color)
{
	BOOL changed = (color != getTE(te)->getColor());
	S32 res = LLViewerObject::setTEColor(te, color);
	if (mDrawable.notNull())
	{
		if (changed)
		{
			calcAllTEsSame();
// 			mFaceMappingChanged = TRUE;
		}
	}
	return  res;
}

S32 LLVOVolume::setTEBumpmap(const U8 te, const U8 bumpmap)
{
	BOOL changed = (bumpmap != getTE(te)->getBumpmap());
	S32 res = LLViewerObject::setTEBumpmap(te, bumpmap);
	if (mDrawable.notNull())
	{
		if (changed)
		{
			calcAllTEsSame();
			mFaceMappingChanged = TRUE;
		}
	}
	return  res;
}

S32 LLVOVolume::setTETexGen(const U8 te, const U8 texgen)
{
	BOOL changed = (texgen != getTE(te)->getTexGen());
	S32 res = LLViewerObject::setTETexGen(te, texgen);
	if (mDrawable.notNull())
	{
		if (changed)
		{
			calcAllTEsSame();
			mFaceMappingChanged = TRUE;
		}
	}
	return  res;
}

S32 LLVOVolume::setTEShiny(const U8 te, const U8 shiny)
{
	BOOL changed = (shiny != getTE(te)->getShiny());
	S32 res = LLViewerObject::setTEShiny(te, shiny);
	if (mDrawable.notNull())
	{
		if (changed)
		{
			calcAllTEsSame();
			mFaceMappingChanged = TRUE;
		}
	}
	return  res;
}

S32 LLVOVolume::setTEFullbright(const U8 te, const U8 fullbright)
{
	BOOL changed = (fullbright != getTE(te)->getFullbright());
	S32 res = LLViewerObject::setTEFullbright(te, fullbright);
	if (mDrawable.notNull())
	{
		if (changed)
		{
			calcAllTEsSame();
			if (!mDrawable->isState(LLDrawable::REBUILD_VOLUME))
			{
				updateFaceFlags();
			}
			mFaceMappingChanged = TRUE;
		}
	}
	return  res;
}

S32 LLVOVolume::setTEMediaFlags(const U8 te, const U8 media_flags)
{
	bool changed = (media_flags != getTE(te)->getMediaFlags());
	S32 res = LLViewerObject::setTEMediaFlags(te, media_flags);
	if (mDrawable.notNull())
	{
		if (changed)
		{
			calcAllTEsSame();
			mFaceMappingChanged = TRUE;
		}
	}
	return  res;
}

S32 LLVOVolume::setTEScale(const U8 te, const F32 s, const F32 t)
{
	F32 olds,oldt;
	getTE(te)->getScale(&olds, &oldt);
	bool changed = (s != olds || t != oldt);
	S32 res = LLViewerObject::setTEScale(te, s, t);
	if (mDrawable.notNull())
	{
		if (changed)
		{
			calcAllTEsSame();
			mFaceMappingChanged = TRUE;
		}
	}
	return res;
}

S32 LLVOVolume::setTEScaleS(const U8 te, const F32 s)
{
	F32 olds,oldt;
	getTE(te)->getScale(&olds, &oldt);
	bool changed = (s != olds);
	S32 res = LLViewerObject::setTEScaleS(te, s);
	if (mDrawable.notNull())
	{
		if (changed)
		{
			calcAllTEsSame();
			mFaceMappingChanged = TRUE;
		}
	}
	return res;
}

S32 LLVOVolume::setTEScaleT(const U8 te, const F32 t)
{
	F32 olds,oldt;
	getTE(te)->getScale(&olds, &oldt);
	bool changed = (t != oldt);
	S32 res = LLViewerObject::setTEScaleT(te, t);
	if (mDrawable.notNull())
	{
		if (changed)
		{
			calcAllTEsSame();
			mFaceMappingChanged = TRUE;
		}
	}
	return res;
}

void LLVOVolume::updateTEData()
{
	if (mDrawable.notNull())
	{
		calcAllTEsSame();
		mFaceMappingChanged = TRUE;
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, TRUE);
	}
}

BOOL LLVOVolume::calcAllTEsSame()
{
	BOOL is_alpha = FALSE;
	BOOL was_same = mAllTEsSame;
	BOOL all_same = TRUE;
	S32 num_tes = getNumTEs();

	LLViewerImage *first_texturep = getTEImage(0);
	if (!first_texturep)
	{
		return FALSE;
	}

	const LLTextureEntry *tep = getTE(0);
	if (!tep)
	{
		llwarns << "Volume with zero textures!" << llendl;
		return FALSE;
	}

	if (tep->getColor().mV[3] != 1.f)
	{
		is_alpha = TRUE;
	}
	const LLColor4 first_color = tep->getColor();
	const U8 first_bump = tep->getBumpShinyFullbright();
	const U8 first_media_flags = tep->getMediaTexGen();

	if (first_texturep->getComponents() == 4)
	{
		is_alpha = TRUE;
	}

	F32 s_scale, t_scale;
	tep->getScale(&s_scale, &t_scale);

	for (S32 f = 1; f < num_tes; f++)
	{
		LLViewerImage *texturep = getTEImage(f);
		if (texturep != first_texturep)
		{
			all_same = FALSE;
			break;
		}

		tep = getTE(f);

		if( tep->getBumpShinyFullbright() != first_bump )
		{
			all_same = FALSE;
			break;
		}

		if (first_bump)
		{
			F32 cur_s, cur_t;
			tep->getScale(&cur_s, &cur_t);
			if ((cur_s != s_scale) || (cur_t != t_scale))
			{
				all_same = FALSE;
				break;
			}
		}

		if ((texturep->getComponents() == 4) || (tep->getColor().mV[3] != 1.f))
		{
			if (!is_alpha)
			{
				all_same = FALSE;
				break;
			}
		}
		else if (is_alpha)
		{
			all_same = FALSE;
			break;
		}

		if (tep->getColor() != first_color)
		{
			all_same = FALSE;
			break;
		}

		if (tep->getMediaTexGen() != first_media_flags)
		{
			all_same = FALSE;
			break;
		}
	}

	mAllTEsSame = all_same;
	if (was_same != all_same)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE); // rebuild NOW
		mFaceMappingChanged = TRUE;
	}
	return mAllTEsSame;
}

void LLVOVolume::setupSingleFace(S32 face_offset)
{
	S32 num_indices = 0;
	S32 num_vertices = 0;

	if (mDrawable.isNull())
	{
		llerrs << "setupSingleFace called with NULL mDrawable" << llendl;
	}
	if (face_offset >= mDrawable->getNumFaces())
	{
		llerrs << "setupSingleFace called with invalid face_offset" << llendl;
	}
	
	const S32 num_faces = getVolume()->getNumFaces();
	for (S32 i = 0; i < num_faces; i++)
	{
		const LLVolumeFace &vf = getVolume()->getVolumeFace(i);
		num_vertices += vf.mVertices.size();
		num_indices += vf.mIndices.size();
	}
	LLFace *facep = mDrawable->getFace(face_offset);
	facep->setSize(num_vertices, num_indices);
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
			
			// Remove this object from any object which has it as a light
			if (mDrawable)
			{
				mDrawable->clearLightSet();
			}
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

//----------------------------------------------------------------------------

// returns < 0 if inside radius
F32 LLVOVolume::getLightDistance(const LLVector3& pos) const
{
	LLVector3 dpos = getRenderPosition() - pos;
	F32 dist = dpos.magVec() - getLightRadius();
	return dist;
}

// returns intensity, modifies color in result
F32 LLVOVolume::calcLightAtPoint(const LLVector3& pos, const LLVector3& norm, LLColor4& result)
{
	if (!getIsLight())
	{
		return 0.0f;
	}
	F32 light_radius = getLightRadius();
	LLVector3 light_pos = getRenderPosition();
	LLVector3 light_dir = light_pos - pos;
	F32 dist = light_dir.normVec();
	F32 dp = norm * light_dir;
	if ((gPipeline.getVertexShaderLevel(LLPipeline::SHADER_OBJECT) >= LLDrawPoolSimple::SHADER_LEVEL_LOCAL_LIGHTS))
	{
		if (dp <= 0)
		{
			result *= 0;
			return 0;
		}

		if (dist >= light_radius)
		{
			result *= 0;
			return 0;
		}

		F32 mag = 1.0f-(dist/light_radius);
		mag = powf(mag, 0.75f);
		mag *= dp;
		result = getLightColor() * mag;
		return mag;
	}
	else
	{
		F32 light_radius = getLightRadius();
		LLVector3 light_pos = getRenderPosition();
		LLVector3 light_dir = light_pos - pos;
		F32 dist = light_dir.normVec();
		F32 dp = norm * light_dir;
		F32 atten = (1.f/.2f) / (light_radius); // 20% of brightness at radius
		F32 falloff = 1.f / (dist * atten);
		F32 mag = falloff * dp;
		mag = llmax(mag, 0.0f);
		result = getLightColor() * mag;
		return mag;
	}
}

BOOL LLVOVolume::updateLighting(BOOL do_lighting)
{
	LLMemType mt1(LLMemType::MTYPE_DRAWABLE);

	if (mDrawable->isStatic())
	{
		do_lighting = FALSE;
	}

	const LLMatrix4& mat_vert  = mDrawable->getWorldMatrix();
	const LLMatrix3& mat_normal = LLMatrix3(mDrawable->getWorldRotation());
	
	LLVolume* volume = getVolume();
	if (getAllTEsSame())
	{
		LLFace *face = mDrawable->getFace(mFaceIndexOffset);
		S32 num_faces = volume->getNumFaces();
		if (face && face->getGeomCount())
		{
			face->genLighting(volume, mDrawable, 0, num_faces-1, mat_vert, mat_normal, do_lighting);
		}
	}
	else
	{
		for (S32 i = 0; i < volume->getNumFaces(); i++)
		{
			LLFace *face = mDrawable->getFace(i + mFaceIndexOffset);
			if (face && face->getGeomCount())
			{
				face->genLighting(volume, mDrawable, i, i, mat_vert, mat_normal, do_lighting);
			}
		}
	}		
	return TRUE;
}

//----------------------------------------------------------------------------

BOOL LLVOVolume::isFlexible() const
{
	if (getParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE))
	{
		if (getVolume()->getParams().getPathParams().getCurveType() != LL_PCODE_PATH_FLEXIBLE)
		{
			llwarns << "wtf" << llendl;
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

		if (!isVolumeGlobal())
		{	//transform view vector into volume space
			view_vector -= getRenderPosition();
			LLQuaternion worldRot = getRenderRotation();
			view_vector = view_vector * ~worldRot;
			LLVector3 objScale = getScale();
			LLVector3 invObjScale(1.f / objScale.mV[VX], 1.f / objScale.mV[VY], 1.f / objScale.mV[VZ]);
			view_vector.scaleVec(invObjScale);
		}

		updateRelativeXform();
		volume->generateSilhouetteVertices(nodep->mSilhouetteVertices, nodep->mSilhouetteNormals, nodep->mSilhouetteSegments, view_vector, mRelativeXform, mRelativeXformInvTrans);

		nodep->mSilhouetteExists = TRUE;
	}
}

void LLVOVolume::deleteFaces(LLVOVolume* childp)
{
	S32 face_count = childp->mNumFaces;
	S32 start_index = childp->mFaceIndexOffset;
	if (mDrawable.notNull())
	{
		mDrawable->deleteFaces(start_index, face_count);
	}
	if (mFaceIndexOffset > start_index)
	{
		mFaceIndexOffset -= face_count;
	}

	for (U32 i = 0; i < mChildList.size(); i++)
	{
		LLViewerObject* siblingp = mChildList[i];
		if (siblingp != childp)
		{
			if (siblingp->getPCode() == LL_PCODE_VOLUME && 
				((LLVOVolume*)siblingp)->mFaceIndexOffset > start_index)
			{
				((LLVOVolume*)siblingp)->mFaceIndexOffset -= face_count;
			}
		}
	}
	childp->mFaceIndexOffset = 0;
	childp->mNumFaces = 0;
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

void LLVOVolume::writeCAL3D(apr_file_t* fp, std::string& path, std::string& file_base, S32 joint_num, LLVector3& pos, LLQuaternion& rot, S32& material_index, S32& texture_index, std::multimap<LLUUID, LLMaterialExportInfo*>& material_map)
{
	LLPointer<LLImageTGA> tga_image = new LLImageTGA;

	if (mDrawable.isNull())
	{
		return;
	}

	LLVector3 final_pos = getPosition();
	final_pos *= 100.f;

	final_pos = final_pos * rot;
	final_pos += pos;
	LLQuaternion final_rot;
	final_rot = getRotation() * rot;
	LLMatrix4 transform;
	transform.initAll(getScale(), final_rot, final_pos);

	LLMatrix4 int_transpose_transform;
	int_transpose_transform.initAll(LLVector3(1.f / getScale().mV[VX], 1.f / getScale().mV[VY], 1.f / getScale().mV[VZ]), final_rot, LLVector3::zero);

	for (S32 i = 0; i < mDrawable->getNumFaces(); i++)
	{
		S32 vert_num = 0;
		LLFace* facep = mDrawable->getFace(i);
		LLDrawPool* poolp = facep->getPool();

		const LLTextureEntry* tep = facep->getTextureEntry();
		if (!tep)
		{
			continue;
		}

		S32 my_material = -1;
		S32 my_texture = -1;
		LLColor4 face_color = tep->getColor();

		typedef std::multimap<LLUUID, LLMaterialExportInfo*>::iterator material_it_t;
		std::pair<material_it_t, material_it_t> found_range = material_map.equal_range(tep->getID());
		material_it_t material_it = found_range.first;

		LLMaterialExportInfo* material_info = NULL;

		while(material_it != material_map.end() && material_it != found_range.second)
		{
			// we've at least found a matching texture, so reuse it
			my_texture = material_it->second->mTextureIndex;
			if (material_it->second->mColor == face_color)
			{
				// we've found a matching material
				material_info = material_it->second;
			}
			++material_it;
		}

		if (material_info)
		{
			// material already exported, just reuse it
			my_material = material_info->mMaterialIndex;
			my_texture = material_info->mTextureIndex;
		}
		else
		{
			// reserve new material number
			my_material = material_index++;

			// if we didn't already find a matching texture...
			if (my_texture == -1)
			{
				//...use the next available slot...
				my_texture = texture_index++;

				//...and export texture as image file
				char filename[MAX_PATH];
				sprintf(filename, "%s\\%s_material_tex_%d.tga", path.c_str(), file_base.c_str(), my_texture);

				LLViewerImage* imagep = facep->getTexture();
				if (imagep->getTexName() == 0)
				{
					llinfos << "No image data available for " << filename << llendl;
					continue;
				}
				LLPointer<LLImageRaw> raw_image = new LLImageRaw;
				imagep->readBackRaw(-1, raw_image);
				BOOL success = tga_image->encode(raw_image);
				success = tga_image->save(filename);
			}

			material_info = new LLMaterialExportInfo(my_material, my_texture, face_color);
			material_map.insert(std::make_pair<LLUUID, LLMaterialExportInfo*>(tep->getID(), material_info));
		}

		apr_file_printf(fp, "\t<SUBMESH NUMVERTICES=\"%d\" NUMFACES=\"%d\" MATERIAL=\"%d\" NUMLODSTEPS=\"0\" NUMSPRINGS=\"0\" NUMTEXCOORDS=\"1\">\n", 
			facep->getGeomCount(), facep->getIndicesCount() / 3, my_material);

		for (S32 vert_index = 0; vert_index < facep->getGeomCount(); vert_index++)
		{
			LLVector3 vert_pos = poolp->getVertex(facep->getGeomStart() + vert_index);
			vert_pos *= 100.f;
			vert_pos = vert_pos * transform;
			LLVector3 vert_norm = poolp->getNormal(facep->getGeomStart() + vert_index);
			vert_norm = vert_norm * int_transpose_transform;
			LLVector2 vert_tc = poolp->getTexCoord(facep->getGeomStart() + vert_index, 0);
			apr_file_printf(fp, "		<VERTEX ID=\"%d\" NUMINFLUENCES=\"1\">\n", vert_num++);
			apr_file_printf(fp, "			<POS>%.4f %.4f %.4f</POS>\n", vert_pos.mV[VX], vert_pos.mV[VY], vert_pos.mV[VZ]);
			apr_file_printf(fp, "			<NORM>%.6f %.6f %.6f</NORM>\n", vert_norm.mV[VX], vert_norm.mV[VY], vert_norm.mV[VZ]);
			apr_file_printf(fp, "			<TEXCOORD>%.6f %.6f</TEXCOORD>\n", vert_tc.mV[VX], 1.f - vert_tc.mV[VY]);
			apr_file_printf(fp, "			<INFLUENCE ID=\"%d\">1.0</INFLUENCE>\n", joint_num + 1);
			apr_file_printf(fp, "		</VERTEX>\n");
		}

		for (U32 index_i = 0; index_i < facep->getIndicesCount(); index_i += 3)
		{
			U32 index_a = poolp->getIndex(facep->getIndicesStart() + index_i) - facep->getGeomStart();
			U32 index_b = poolp->getIndex(facep->getIndicesStart() + index_i + 1) - facep->getGeomStart();
			U32 index_c = poolp->getIndex(facep->getIndicesStart() + index_i + 2) - facep->getGeomStart();
			apr_file_printf(fp, "		<FACE VERTEXID=\"%d %d %d\" />\n", index_a, index_b, index_c);
		}

		apr_file_printf(fp, "	</SUBMESH>\n");
	}
	
	for (U32 i = 0; i < mChildList.size(); i++)
	{
		((LLVOVolume*)(LLViewerObject*)mChildList[i])->writeCAL3D(fp, path, file_base, joint_num, final_pos, final_rot, material_index, texture_index, material_map);
	}
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

void LLVOVolume::updateSpatialExtents(LLVector3& newMin, LLVector3& newMax)
{		
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
	if (isVolumeGlobal())
	{
		return pos;
	}
	
	LLVector3 ret = pos - getRenderPosition();
	ret = ret * ~getRenderRotation();
	LLVector3 objScale = getScale();
	LLVector3 invObjScale(1.f / objScale.mV[VX], 1.f / objScale.mV[VY], 1.f / objScale.mV[VZ]);
	ret.scaleVec(invObjScale);
	
	return ret;
}

LLVector3 LLVOVolume::agentDirectionToVolume(const LLVector3& dir) const
{
	return isVolumeGlobal() ? dir : (dir * ~getRenderRotation());
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
}
