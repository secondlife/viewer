/** 
 * @file llvovolume.cpp
 * @brief LLVOVolume class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

// A "volume" is a box, cylinder, sphere, or other primitive shape.

#include "llviewerprecompiledheaders.h"

#include "llvovolume.h"

#include <sstream>

#include "llviewercontrol.h"
#include "lldir.h"
#include "llflexibleobject.h"
#include "llfloatertools.h"
#include "llmaterialid.h"
#include "llmaterialtable.h"
#include "llprimitive.h"
#include "llvolume.h"
#include "llvolumeoctree.h"
#include "llvolumemgr.h"
#include "llvolumemessage.h"
#include "material_codes.h"
#include "message.h"
#include "llpluginclassmedia.h" // for code in the mediaEvent handler
#include "object_flags.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "llface.h"
#include "llspatialpartition.h"
#include "llhudmanager.h"
#include "llflexibleobject.h"
#include "llskinningutil.h"
#include "llsky.h"
#include "lltexturefetch.h"
#include "llvector4a.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertextureanim.h"
#include "llworld.h"
#include "llselectmgr.h"
#include "pipeline.h"
#include "llsdutil.h"
#include "llmatrix4a.h"
#include "llmediaentry.h"
#include "llmediadataclient.h"
#include "llmeshrepository.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llagent.h"
#include "llviewermediafocus.h"
#include "lldatapacker.h"
#include "llviewershadermgr.h"
#include "llvoavatar.h"
#include "llcontrolavatar.h"
#include "llvoavatarself.h"
#include "llvocache.h"
#include "llmaterialmgr.h"
#include "llanimationstates.h"
#include "llinventorytype.h"
#include "llviewerinventory.h"
#include "llcallstack.h"
#include "llsculptidsize.h"
#include "llavatarappearancedefines.h"
#include "llgltfmateriallist.h"

const F32 FORCE_SIMPLE_RENDER_AREA = 512.f;
const F32 FORCE_CULL_AREA = 8.f;
U32 JOINT_COUNT_REQUIRED_FOR_FULLRIG = 1;

BOOL gAnimateTextures = TRUE;

F32 LLVOVolume::sLODFactor = 1.f;
F32	LLVOVolume::sLODSlopDistanceFactor = 0.5f; //Changing this to zero, effectively disables the LOD transition slop 
F32 LLVOVolume::sDistanceFactor = 1.0f;
S32 LLVOVolume::sNumLODChanges = 0;
S32 LLVOVolume::mRenderComplexity_last = 0;
S32 LLVOVolume::mRenderComplexity_current = 0;
LLPointer<LLObjectMediaDataClient> LLVOVolume::sObjectMediaClient = NULL;
LLPointer<LLObjectMediaNavigateClient> LLVOVolume::sObjectMediaNavigateClient = NULL;

extern BOOL gCubeSnapshot;

// Implementation class of LLMediaDataClientObject.  See llmediadataclient.h
class LLMediaDataClientObjectImpl : public LLMediaDataClientObject
{
public:
	LLMediaDataClientObjectImpl(LLVOVolume *obj, bool isNew) : mObject(obj), mNew(isNew) 
	{
		mObject->addMDCImpl();
	}
	~LLMediaDataClientObjectImpl()
	{
		mObject->removeMDCImpl();
	}
	
	virtual U8 getMediaDataCount() const 
		{ return mObject->getNumTEs(); }

	virtual LLSD getMediaDataLLSD(U8 index) const 
		{
			LLSD result;
			LLTextureEntry *te = mObject->getTE(index); 
			if (NULL != te)
			{
				llassert((te->getMediaData() != NULL) == te->hasMedia());
				if (te->getMediaData() != NULL)
				{
					result = te->getMediaData()->asLLSD();
					// XXX HACK: workaround bug in asLLSD() where whitelist is not set properly
					// See DEV-41949
					if (!result.has(LLMediaEntry::WHITELIST_KEY))
					{
						result[LLMediaEntry::WHITELIST_KEY] = LLSD::emptyArray();
					}
				}
			}
			return result;
		}
	virtual bool isCurrentMediaUrl(U8 index, const std::string &url) const
		{
			LLTextureEntry *te = mObject->getTE(index); 
			if (te)
			{
				if (te->getMediaData())
				{
					return (te->getMediaData()->getCurrentURL() == url);
				}
			}
			return url.empty();
		}

	virtual LLUUID getID() const
		{ return mObject->getID(); }

	virtual void mediaNavigateBounceBack(U8 index)
		{ mObject->mediaNavigateBounceBack(index); }
	
	virtual bool hasMedia() const
		{ return mObject->hasMedia(); }
	
	virtual void updateObjectMediaData(LLSD const &data, const std::string &version_string) 
		{ mObject->updateObjectMediaData(data, version_string); }
	
	virtual F64 getMediaInterest() const 
		{ 
			F64 interest = mObject->getTotalMediaInterest();
			if (interest < (F64)0.0)
			{
				// media interest not valid yet, try pixel area
				interest = mObject->getPixelArea();
				// HACK: force recalculation of pixel area if interest is the "magic default" of 1024.
				if (interest == 1024.f)
				{
					const_cast<LLVOVolume*>(static_cast<LLVOVolume*>(mObject))->setPixelAreaAndAngle(gAgent);
					interest = mObject->getPixelArea();
				}
			}
			return interest; 
		}
	
	virtual bool isInterestingEnough() const
		{
			return LLViewerMedia::getInstance()->isInterestingEnough(mObject, getMediaInterest());
		}

	virtual std::string getCapabilityUrl(const std::string &name) const
		{ return mObject->getRegion()->getCapability(name); }
	
	virtual bool isDead() const
		{ return mObject->isDead(); }
	
	virtual U32 getMediaVersion() const
		{ return LLTextureEntry::getVersionFromMediaVersionString(mObject->getMediaURL()); }
	
	virtual bool isNew() const
		{ return mNew; }

private:
	LLPointer<LLVOVolume> mObject;
	bool mNew;
};


LLVOVolume::LLVOVolume(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
	: LLViewerObject(id, pcode, regionp),
	  mVolumeImpl(NULL)
{
	mTexAnimMode = 0;
	mRelativeXform.setIdentity();
	mRelativeXformInvTrans.setIdentity();

	mFaceMappingChanged = FALSE;
	mLOD = MIN_LOD;
    mLODDistance = 0.0f;
    mLODAdjustedDistance = 0.0f;
    mLODRadius = 0.0f;
	mTextureAnimp = NULL;
	mVolumeChanged = FALSE;
	mVObjRadius = LLVector3(1,1,0.5f).length();
	mNumFaces = 0;
	mLODChanged = FALSE;
	mSculptChanged = FALSE;
    mColorChanged = FALSE;
	mSpotLightPriority = 0.f;

	mSkinInfoFailed = false;
	mSkinInfo = NULL;

	mMediaImplList.resize(getNumTEs());
	mLastFetchedMediaVersion = -1;
    mServerDrawableUpdateCount = 0;
	memset(&mIndexInTex, 0, sizeof(S32) * LLRender::NUM_VOLUME_TEXTURE_CHANNELS);
	mMDCImplCount = 0;
	mLastRiggingInfoLOD = -1;
	mResetDebugText = false;
}

LLVOVolume::~LLVOVolume()
{
	delete mTextureAnimp;
	mTextureAnimp = NULL;
	delete mVolumeImpl;
	mVolumeImpl = NULL;

	gMeshRepo.unregisterMesh(this);

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

void LLVOVolume::markDead()
{
	if (!mDead)
	{
        if (getVolume())
        {
            LLSculptIDSize::instance().rem(getVolume()->getParams().getSculptID());
        }

		if(getMDCImplCount() > 0)
		{
			LLMediaDataClientObject::ptr_t obj = new LLMediaDataClientObjectImpl(const_cast<LLVOVolume*>(this), false);
			if (sObjectMediaClient) sObjectMediaClient->removeFromQueue(obj);
			if (sObjectMediaNavigateClient) sObjectMediaNavigateClient->removeFromQueue(obj);
		}
		
		// Detach all media impls from this object
		for(U32 i = 0 ; i < mMediaImplList.size() ; i++)
		{
			removeMediaImpl(i);
		}

		if (mSculptTexture.notNull())
		{
			mSculptTexture->removeVolume(LLRender::SCULPT_TEX, this);
		}

		if (mLightTexture.notNull())
		{
			mLightTexture->removeVolume(LLRender::LIGHT_TEX, this);
		}
	}
	
	LLViewerObject::markDead();
}


// static
void LLVOVolume::initClass()
{
	// gSavedSettings better be around
	if (gSavedSettings.getBOOL("PrimMediaMasterEnabled"))
	{
		const F32 queue_timer_delay = gSavedSettings.getF32("PrimMediaRequestQueueDelay");
		const F32 retry_timer_delay = gSavedSettings.getF32("PrimMediaRetryTimerDelay");
		const U32 max_retries = gSavedSettings.getU32("PrimMediaMaxRetries");
		const U32 max_sorted_queue_size = gSavedSettings.getU32("PrimMediaMaxSortedQueueSize");
		const U32 max_round_robin_queue_size = gSavedSettings.getU32("PrimMediaMaxRoundRobinQueueSize");
		sObjectMediaClient = new LLObjectMediaDataClient(queue_timer_delay, retry_timer_delay, max_retries, 
														 max_sorted_queue_size, max_round_robin_queue_size);
		sObjectMediaNavigateClient = new LLObjectMediaNavigateClient(queue_timer_delay, retry_timer_delay, 
																	 max_retries, max_sorted_queue_size, max_round_robin_queue_size);
	}
}

// static
void LLVOVolume::cleanupClass()
{
    sObjectMediaClient = NULL;
    sObjectMediaNavigateClient = NULL;
}

U32 LLVOVolume::processUpdateMessage(LLMessageSystem *mesgsys,
										  void **user_data,
										  U32 block_num, EObjectUpdateType update_type,
										  LLDataPacker *dp)
{
	 	
	LLColor4U color;
	const S32 teDirtyBits = (TEM_CHANGE_TEXTURE|TEM_CHANGE_COLOR|TEM_CHANGE_MEDIA);
    const bool previously_volume_changed = mVolumeChanged;
    const bool previously_face_mapping_changed = mFaceMappingChanged;
    const bool previously_color_changed = mColorChanged;

	// Do base class updates...
	U32 retval = LLViewerObject::processUpdateMessage(mesgsys, user_data, block_num, update_type, dp);

	LLUUID sculpt_id;
	U8 sculpt_type = 0;
	if (isSculpted())
	{
		LLSculptParams *sculpt_params = (LLSculptParams *)getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		sculpt_id = sculpt_params->getSculptTexture();
		sculpt_type = sculpt_params->getSculptType();

        LL_DEBUGS("ObjectUpdate") << "uuid " << mID << " set sculpt_id " << sculpt_id << LL_ENDL;
        dumpStack("ObjectUpdateStack");
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
					mTextureAnimp = new LLViewerTextureAnim(this);
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

                    for (S32 i = 0; i < getNumTEs(); i++)
                    {
                        LLFace* facep = mDrawable->getFace(i);
                        if (facep && facep->mTextureMatrix)
                        {
                            // delete or reset
                            delete facep->mTextureMatrix;
                            facep->mTextureMatrix = NULL;
                        }
                    }

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

		S32 result = unpackTEMessage(mesgsys, _PREHASH_ObjectData, (S32) block_num);
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
		if (update_type != OUT_TERSE_IMPROVED)
		{
			LLVolumeParams volume_params;
			BOOL res = LLVolumeMessage::unpackVolumeParams(&volume_params, *dp);
			if (!res)
			{
				LL_WARNS() << "Bogus volume parameters in object " << getID() << LL_ENDL;
				LL_WARNS() << getRegion()->getOriginGlobal() << LL_ENDL;
			}

			volume_params.setSculptID(sculpt_id, sculpt_type);

			if (setVolume(volume_params, 0))
			{
				markForUpdate(TRUE);
			}
			S32 res2 = unpackTEMessage(*dp);
			if (TEM_INVALID == res2)
			{
				// There's something bogus in the data that we're unpacking.
				dp->dumpBufferToLog();
				LL_WARNS() << "Flushing cache files" << LL_ENDL;

				if(LLVOCache::instanceExists() && getRegion())
				{
					LLVOCache::getInstance()->removeEntry(getRegion()->getHandle()) ;
				}
				
				LL_WARNS() << "Bogus TE data in " << getID() << LL_ENDL;
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
					mTextureAnimp = new LLViewerTextureAnim(this);
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

                for (S32 i = 0; i < getNumTEs(); i++)
                {
                    LLFace* facep = mDrawable->getFace(i);
                    if (facep && facep->mTextureMatrix)
                    {
                        // delete or reset
                        delete facep->mTextureMatrix;
                        facep->mTextureMatrix = NULL;
                    }
                }

				gPipeline.markTextured(mDrawable);
				mFaceMappingChanged = TRUE;
				mTexAnimMode = 0;
			}

			if (value & 0x400)
			{ //particle system (new)
				unpackParticleSource(*dp, mOwnerID, false);
			}
		}
		else
		{
			S32 texture_length = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_TextureEntry);
			if (texture_length)
			{
				U8							tdpbuffer[1024];
				LLDataPackerBinaryBuffer	tdp(tdpbuffer, 1024);
				mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_TextureEntry, tdpbuffer, 0, block_num, 1024);
				S32 result = unpackTEMessage(tdp);
				if (result & teDirtyBits)
				{
					updateTEData();
                    if (mDrawable)
                    { //on the fly TE updates break batches, isolate in octree
                        shrinkWrap();
                    }
				}
				if (result & TEM_CHANGE_MEDIA)
				{
					retval |= MEDIA_FLAGS_CHANGED;
				}
			}
		}
	}
	if (retval & (MEDIA_URL_REMOVED | MEDIA_URL_ADDED | MEDIA_URL_UPDATED | MEDIA_FLAGS_CHANGED)) 
	{
		// If only the media URL changed, and it isn't a media version URL,
		// ignore it
		if ( ! ( retval & (MEDIA_URL_ADDED | MEDIA_URL_UPDATED) &&
				 mMedia && ! mMedia->mMediaURL.empty() &&
				 ! LLTextureEntry::isMediaVersionString(mMedia->mMediaURL) ) )
		{
			// If the media changed at all, request new media data
			LL_DEBUGS("MediaOnAPrim") << "Media update: " << getID() << ": retval=" << retval << " Media URL: " <<
                ((mMedia) ?  mMedia->mMediaURL : std::string("")) << LL_ENDL;
			requestMediaDataUpdate(retval & MEDIA_FLAGS_CHANGED);
		}
        else {
            LL_INFOS("MediaOnAPrim") << "Ignoring media update for: " << getID() << " Media URL: " <<
                ((mMedia) ?  mMedia->mMediaURL : std::string("")) << LL_ENDL;
        }
	}
	// ...and clean up any media impls
	cleanUpMediaImpls();

    if ((
            (mVolumeChanged && !previously_volume_changed) ||
            (mFaceMappingChanged && !previously_face_mapping_changed) ||
            (mColorChanged && !previously_color_changed)
        )
        && !mLODChanged) {
        onDrawableUpdateFromServer();
    }

	return retval;
}

// Called when a volume, material, etc is updated by the server, possibly by a
// script. If this occurs too often for this object, mark it as active so that
// it doesn't disrupt the octree/render batches, thereby potentially causing a
// big performance penalty.
void LLVOVolume::onDrawableUpdateFromServer()
{
    constexpr U32 UPDATES_UNTIL_ACTIVE = 8;
    ++mServerDrawableUpdateCount;
    if (mDrawable && !mDrawable->isActive() && mServerDrawableUpdateCount > UPDATES_UNTIL_ACTIVE)
    {
        mDrawable->makeActive();
    }
}

void LLVOVolume::animateTextures()
{
	if (!mDead)
	{
        shrinkWrap();
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
				if (!facep) continue;
				if(facep->getVirtualSize() <= MIN_TEX_ANIM_SIZE && facep->mTextureMatrix) continue;

				const LLTextureEntry* te = facep->getTextureEntry();
			
				if (!te)
				{
					continue;
				}
		
                LLGLTFMaterial *gltf_mat = te->getGLTFRenderMaterial();
                const bool is_pbr = gltf_mat != nullptr;

                if (!is_pbr)
                {
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
                }

				if (!facep->mTextureMatrix)
				{
					facep->mTextureMatrix = new LLMatrix4();
				}

				LLMatrix4& tex_mat = *facep->mTextureMatrix;
				tex_mat.setIdentity();

				LLVector3 trans ;

					trans.set(LLVector3(off_s+0.5f, off_t+0.5f, 0.f));			
					tex_mat.translate(LLVector3(-0.5f, -0.5f, 0.f));

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
}

void LLVOVolume::updateTextures()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	//const F32 TEXTURE_AREA_REFRESH_TIME = 1.f; // seconds
	//if (mTextureUpdateTimer.getElapsedTimeF32() > TEXTURE_AREA_REFRESH_TIME)
	{
		updateTextureVirtualSize();

		/*if (mDrawable.notNull() && !isVisible() && !mDrawable->isActive())
		{ //delete vertex buffer to free up some VRAM
			LLSpatialGroup* group  = mDrawable->getSpatialGroup();
			if (group && (group->mVertexBuffer.notNull() || !group->mBufferMap.empty() || !group->mDrawMap.empty()))
			{
				group->destroyGL(true);

				//flag the group as having changed geometry so it gets a rebuild next time
				//it becomes visible
				group->setState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::MESH_DIRTY | LLSpatialGroup::NEW_DRAWINFO);
			}
		}*/
    }
}

BOOL LLVOVolume::isVisible() const 
{
	if(mDrawable.notNull() && mDrawable->isVisible())
	{
		return TRUE ;
	}

	if(isAttachment())
	{
		LLViewerObject* objp = (LLViewerObject*)getParent() ;
		while(objp && !objp->isAvatar())
		{
			objp = (LLViewerObject*)objp->getParent() ;
		}

		return objp && objp->mDrawable.notNull() && objp->mDrawable->isVisible() ;
	}

	return FALSE ;
}

void LLVOVolume::updateTextureVirtualSize(bool forced)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;
	// Update the pixel area of all faces

    if (mDrawable.isNull() || gCubeSnapshot)
    {
        return;
    }

	if(!forced)
	{
		if(!isVisible())
		{ //don't load textures for non-visible faces
			const S32 num_faces = mDrawable->getNumFaces();
			for (S32 i = 0; i < num_faces; i++)
			{
				LLFace* face = mDrawable->getFace(i);
				if (face)
				{
					face->setPixelArea(0.f); 
					face->setVirtualSize(0.f);
				}
			}

			return ;
		}

		if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SIMPLE))
		{
			return;
		}
	}

	static LLCachedControl<bool> dont_load_textures(gSavedSettings,"TextureDisable", false);
		
	if (dont_load_textures || LLAppViewer::getTextureFetch()->mDebugPause) // || !mDrawable->isVisible())
	{
		return;
	}

	mTextureUpdateTimer.reset();
	
	F32 old_area = mPixelArea;
	mPixelArea = 0.f;

	const S32 num_faces = mDrawable->getNumFaces();
	F32 min_vsize=999999999.f, max_vsize=0.f;
	LLViewerCamera* camera = LLViewerCamera::getInstance();
    std::stringstream debug_text;
	for (S32 i = 0; i < num_faces; i++)
	{
		LLFace* face = mDrawable->getFace(i);
		if (!face) continue;
		const LLTextureEntry *te = face->getTextureEntry();
		LLViewerTexture *imagep = face->getTexture();
		if (!imagep || !te ||			
			face->mExtents[0].equals3(face->mExtents[1]))
		{
			continue;
		}

        // clear out boost selected periodically
        if (imagep->getBoostLevel() == LLGLTexture::BOOST_SELECTED)
        {
            imagep->setBoostLevel(LLGLTexture::BOOST_NONE);
        }

		F32 vsize;
		F32 old_size = face->getVirtualSize();

		if (isHUDAttachment())
		{
			F32 area = (F32) camera->getScreenPixelArea();
			vsize = area;
			imagep->setBoostLevel(LLGLTexture::BOOST_HUD);
 			face->setPixelArea(area); // treat as full screen
			face->setVirtualSize(vsize);
		}
		else
		{
			vsize = face->getTextureVirtualSize();
            imagep->addTextureStats(vsize);
		}

		mPixelArea = llmax(mPixelArea, face->getPixelArea());

        // if the face has gotten small enough to turn off texture animation and texture
        // animation is running, rebuild the render batch for this face to turn off
        // texture animation
		if (face->mTextureMatrix != NULL)
		{
			if ((vsize < MIN_TEX_ANIM_SIZE && old_size > MIN_TEX_ANIM_SIZE) ||
				(vsize > MIN_TEX_ANIM_SIZE && old_size < MIN_TEX_ANIM_SIZE))
			{
				gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD, FALSE);
			}
		}
				
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
                debug_text << img->getDiscardLevel() << ":" << img->getDesiredDiscardLevel() << ":" << img->getWidth() << ":" << (S32) sqrtf(vsize) << ":" << (S32) sqrtf(img->getMaxVirtualSize()) << "\n";
				/*F32 pri = img->getDecodePriority();
				pri = llmax(pri, 0.0f);
				if (pri < min_vsize) min_vsize = pri;
				if (pri > max_vsize) max_vsize = pri;*/
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
        updateSculptTexture();
		
		

		if (mSculptTexture.notNull())
		{
			mSculptTexture->setBoostLevel(llmax((S32)mSculptTexture->getBoostLevel(),
												(S32)LLGLTexture::BOOST_SCULPTED));
			mSculptTexture->setForSculpt() ;
			
			if(!mSculptTexture->isCachedRawImageReady())
			{
				S32 lod = llmin(mLOD, 3);
				F32 lodf = ((F32)(lod + 1.0f)/4.f);
				F32 tex_size = lodf * LLViewerTexture::sMaxSculptRez ;
				mSculptTexture->addTextureStats(2.f * tex_size * tex_size, FALSE);
			}
	
			S32 texture_discard = mSculptTexture->getCachedRawImageLevel(); //try to match the texture
			S32 current_discard = getVolume() ? getVolume()->getSculptLevel() : -2 ;

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

	}

	if (getLightTextureID().notNull())
	{
		LLLightImageParams* params = (LLLightImageParams*) getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
		LLUUID id = params->getLightTexture();
		mLightTexture = LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_ALM);
		if (mLightTexture.notNull())
		{
			F32 rad = getLightRadius();
			mLightTexture->addTextureStats(gPipeline.calcPixelArea(getPositionAgent(), 
																	LLVector3(rad,rad,rad),
																	*camera));
		}	
	}
	
	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
	{
		setDebugText(llformat("%.0f:%.0f", (F32) sqrt(min_vsize),(F32) sqrt(max_vsize)));
	}
 	else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
 	{
 		//setDebugText(llformat("%.0f:%.0f", (F32) sqrt(min_vsize),(F32) sqrt(max_vsize)));
        setDebugText(debug_text.str());
 	}
	else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_FACE_AREA))
	{
		setDebugText(llformat("%.0f:%.0f", (F32) sqrt(min_vsize),(F32) sqrt(max_vsize)));
	}

	if (mPixelArea == 0)
	{ //flexi phasing issues make this happen
		mPixelArea = old_area;
	}
}

BOOL LLVOVolume::isActive() const
{
	return !mStatic;
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

        if (mDrawable)
        {
            shrinkWrap();
        }
	}
}

LLFace* LLVOVolume::addFace(S32 f)
{
	const LLTextureEntry* te = getTE(f);
	LLViewerTexture* imagep = getTEImage(f);
	if (te->getMaterialParams().notNull())
	{
		LLViewerTexture* normalp = getTENormalMap(f);
		LLViewerTexture* specularp = getTESpecularMap(f);
		return mDrawable->addFace(te, imagep, normalp, specularp);
	}
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

    if (isReflectionProbe())
    {
        updateReflectionProbePtr();
    }

	updateRadius();
	bool force_update = true; // avoid non-alpha mDistance update being optimized away
	mDrawable->updateDistance(*LLViewerCamera::getInstance(), force_update);

	return mDrawable;
}

BOOL LLVOVolume::setVolume(const LLVolumeParams &params_in, const S32 detail, bool unique_volume)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;
	LLVolumeParams volume_params = params_in;

	S32 last_lod = mVolumep.notNull() ? LLVolumeLODGroup::getVolumeDetailFromScale(mVolumep->getDetail()) : -1;
	S32 lod = mLOD;

	BOOL is404 = FALSE;
	
	if (isSculpted())
	{
		// if it's a mesh
		if ((volume_params.getSculptType() & LL_SCULPT_TYPE_MASK) == LL_SCULPT_TYPE_MESH)
		{ //meshes might not have all LODs, get the force detail to best existing LOD
			if (NO_LOD != lod)
			{
				lod = gMeshRepo.getActualMeshLOD(volume_params, lod);
				if (lod == -1)
				{
					is404 = TRUE;
					lod = 0;
				}
			}
		}
	}

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
	
	if (is404)
	{
		setIcon(LLViewerTextureManager::getFetchedTextureFromFile("icons/Inv_Mesh.png", FTT_LOCAL_FILE, TRUE, LLGLTexture::BOOST_UI));
		//render prim proxy when mesh loading attempts give up
		volume_params.setSculptID(LLUUID::null, LL_SCULPT_TYPE_NONE);

	}

	if ((LLPrimitive::setVolume(volume_params, lod, (mVolumeImpl && mVolumeImpl->isVolumeUnique()))) || mSculptChanged)
	{
		mFaceMappingChanged = TRUE;
		
		if (mVolumeImpl)
		{
			mVolumeImpl->onSetVolume(volume_params, mLOD);
		}
	
		updateSculptTexture();

		if (isSculpted())
		{
			updateSculptTexture();
			// if it's a mesh
			if ((volume_params.getSculptType() & LL_SCULPT_TYPE_MASK) == LL_SCULPT_TYPE_MESH)
			{
				if (mSkinInfo && mSkinInfo->mMeshID != volume_params.getSculptID())
				{
					mSkinInfo = NULL;
					mSkinInfoFailed = false;
				}

				if (!getVolume()->isMeshAssetLoaded())
				{ 
					//load request not yet issued, request pipeline load this mesh
					S32 available_lod = gMeshRepo.loadMesh(this, volume_params, lod, last_lod);
					if (available_lod != lod)
					{
						LLPrimitive::setVolume(volume_params, available_lod);
					}
				}
				
				if (!mSkinInfo && !mSkinInfoFailed)
				{
					const LLMeshSkinInfo* skin_info = gMeshRepo.getSkinInfo(volume_params.getSculptID(), this);
					if (skin_info)
					{
						notifySkinInfoLoaded(skin_info);
					}
				}
			}
			else // otherwise is sculptie
			{
				if (mSculptTexture.notNull())
				{
					sculpt();
				}
			}
		}

        return TRUE;
	}
	else if (NO_LOD == lod) 
	{
		LLSculptIDSize::instance().resetSizeSum(volume_params.getSculptID());
	}

	return FALSE;
}

void LLVOVolume::updateSculptTexture()
{
	LLPointer<LLViewerFetchedTexture> old_sculpt = mSculptTexture;

	if (isSculpted() && !isMesh())
	{
		LLSculptParams *sculpt_params = (LLSculptParams *)getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		LLUUID id =  sculpt_params->getSculptTexture();
		if (id.notNull())
		{
			mSculptTexture = LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
		}

		mSkinInfoFailed = false;
		mSkinInfo = NULL;
	}
	else
	{
		mSculptTexture = NULL;
	}

	if (mSculptTexture != old_sculpt)
	{
		if (old_sculpt.notNull())
		{
			old_sculpt->removeVolume(LLRender::SCULPT_TEX, this);
		}
		if (mSculptTexture.notNull())
		{
			mSculptTexture->addVolume(LLRender::SCULPT_TEX, this);
		}
	}
	
}

void LLVOVolume::updateVisualComplexity()
{
    LLVOAvatar* avatar = getAvatarAncestor();
    if (avatar)
    {
        avatar->updateVisualComplexity();
    }
    LLVOAvatar* rigged_avatar = getAvatar();
    if(rigged_avatar && (rigged_avatar != avatar))
    {
        rigged_avatar->updateVisualComplexity();
    }
}

void LLVOVolume::notifyMeshLoaded()
{ 
	mSculptChanged = TRUE;
	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, TRUE);

    LLVOAvatar *av = getAvatar();
    if (av && !isAnimatedObject())
    {
        av->addAttachmentOverridesForObject(this);
        av->notifyAttachmentMeshLoaded();
    }
    LLControlAvatar *cav = getControlAvatar();
    if (cav && isAnimatedObject())
    {
        cav->addAttachmentOverridesForObject(this);
        cav->notifyAttachmentMeshLoaded();
    }
    updateVisualComplexity();
}

void LLVOVolume::notifySkinInfoLoaded(const LLMeshSkinInfo* skin)
{
	mSkinInfoFailed = false;
	mSkinInfo = skin;

	notifyMeshLoaded();
}

void LLVOVolume::notifySkinInfoUnavailable()
{
	mSkinInfoFailed = true;
	mSkinInfo = nullptr;
}

// sculpt replaces generate() for sculpted surfaces
void LLVOVolume::sculpt()
{	
	if (mSculptTexture.notNull())
	{				
		U16 sculpt_height = 0;
		U16 sculpt_width = 0;
		S8 sculpt_components = 0;
		const U8* sculpt_data = NULL;
	
		S32 discard_level = mSculptTexture->getCachedRawImageLevel() ;
		LLImageRaw* raw_image = mSculptTexture->getCachedRawImage() ;
		
		S32 max_discard = mSculptTexture->getMaxDiscardLevel();
		if (discard_level > max_discard)
		{
			discard_level = max_discard;    // clamp to the best we can do			
		}
		if(discard_level > MAX_DISCARD_LEVEL)
		{
			return; //we think data is not ready yet.
		}

		S32 current_discard = getVolume()->getSculptLevel() ;
		if(current_discard < -2)
		{
			static S32 low_sculpty_discard_warning_count = 1;
			S32 exponent = llmax(1, llfloor( log10((F64) low_sculpty_discard_warning_count) ));
			S32 interval = pow(10.0, exponent);
			if ( low_sculpty_discard_warning_count < 10 ||
				(low_sculpty_discard_warning_count % interval) == 0)
			{	// Log first 10 time, then decreasing intervals afterwards otherwise this can flood the logs
				LL_WARNS() << "WARNING!!: Current discard for sculpty " << mSculptTexture->getID() 
					<< " at " << current_discard 
					<< " is less than -2." 
					<< " Hit this " << low_sculpty_discard_warning_count << " times"
					<< LL_ENDL;
			}
			low_sculpty_discard_warning_count++;
			
			// corrupted volume... don't update the sculpty
			return;
		}
		else if (current_discard > MAX_DISCARD_LEVEL)
		{
			static S32 high_sculpty_discard_warning_count = 1;
			S32 exponent = llmax(1, llfloor( log10((F64) high_sculpty_discard_warning_count) ));
			S32 interval = pow(10.0, exponent);
			if ( high_sculpty_discard_warning_count < 10 ||
				(high_sculpty_discard_warning_count % interval) == 0)
			{	// Log first 10 time, then decreasing intervals afterwards otherwise this can flood the logs
				LL_WARNS() << "WARNING!!: Current discard for sculpty " << mSculptTexture->getID() 
					<< " at " << current_discard 
					<< " is more than than allowed max of " << MAX_DISCARD_LEVEL
					<< ".  Hit this " << high_sculpty_discard_warning_count << " times"
					<< LL_ENDL;
			}
			high_sculpty_discard_warning_count++;

			// corrupted volume... don't update the sculpty			
			return;
		}

		if (current_discard == discard_level)  // no work to do here
			return;
		
		if(!raw_image)
		{
			llassert(discard_level < 0) ;

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
			sculpt_height = raw_image->getHeight();
			sculpt_width = raw_image->getWidth();
			sculpt_components = raw_image->getComponents();		
					   
			sculpt_data = raw_image->getData();

			if(LLViewerTextureManager::sTesterp)
			{
				mSculptTexture->updateBindStatsForTester() ;
			}
		}
		getVolume()->sculpt(sculpt_width, sculpt_height, sculpt_components, sculpt_data, discard_level, mSculptTexture->isMissingAsset());

		//notify rebuild any other VOVolumes that reference this sculpty volume
		for (S32 i = 0; i < mSculptTexture->getNumVolumes(LLRender::SCULPT_TEX); ++i)
		{
			LLVOVolume* volume = (*(mSculptTexture->getVolumeList(LLRender::SCULPT_TEX)))[i];
			if (volume != this && volume->getVolume() == getVolume())
			{
				gPipeline.markRebuild(volume->mDrawable, LLDrawable::REBUILD_GEOMETRY, FALSE);
			}
		}
	}
}

S32	LLVOVolume::computeLODDetail(F32 distance, F32 radius, F32 lod_factor)
{
	S32	cur_detail;
	if (LLPipeline::sDynamicLOD)
	{
		// We've got LOD in the profile, and in the twist.  Use radius.
		F32 tan_angle = (lod_factor*radius)/distance;
		cur_detail = LLVolumeLODGroup::getDetailFromTan(ll_round(tan_angle, 0.01f));
	}
	else
	{
		cur_detail = llclamp((S32) (sqrtf(radius)*lod_factor*4.f), 0, 3);
	}
	return cur_detail;
}

std::string get_debug_object_lod_text(LLVOVolume *rootp)
{
    std::string cam_dist_string = "";
    cam_dist_string += LLStringOps::getReadableNumber(rootp->mLODDistance) +  " ";
    std::string lod_string = llformat("%d",rootp->getLOD());
    F32 lod_radius = rootp->mLODRadius;
    S32 cam_dist_count = 0;
    LLViewerObject::const_child_list_t& child_list = rootp->getChildren();
    for (LLViewerObject::const_child_list_t::const_iterator iter = child_list.begin();
         iter != child_list.end(); ++iter)
    {
        LLViewerObject *childp = *iter;
        LLVOVolume *volp = dynamic_cast<LLVOVolume*>(childp);
        if (volp)
        {
            lod_string += llformat("%d",volp->getLOD());
            if (volp->isRiggedMesh())
            {
                // Rigged/animatable mesh. This is computed from the
                // avatar dynamic box, so value from any vol will be
                // the same.
                lod_radius = volp->mLODRadius;
            }
            if (volp->mDrawable)
            {
                if (cam_dist_count < 4)
                {
                    cam_dist_string += LLStringOps::getReadableNumber(volp->mLODDistance) +  " ";
                    cam_dist_count++;
                }
            }
        }
    }
    std::string result = llformat("lod_radius %s dists %s lods %s",
                                  LLStringOps::getReadableNumber(lod_radius).c_str(),
                                  cam_dist_string.c_str(),
                                  lod_string.c_str());
    return result;
}

BOOL LLVOVolume::calcLOD()
{
	if (mDrawable.isNull())
	{
		return FALSE;
	}

	S32 cur_detail = 0;
	
	F32 radius;
	F32 distance;
	F32 lod_factor = LLVOVolume::sLODFactor;

	if (mDrawable->isState(LLDrawable::RIGGED))
	{
		LLVOAvatar* avatar = getAvatar(); 
		
		// Not sure how this can really happen, but alas it does. Better exit here than crashing.
		if( !avatar || !avatar->mDrawable )
		{
			return FALSE;
		}

		distance = avatar->mDrawable->mDistanceWRTCamera;


        if (avatar->isControlAvatar())
        {
            // MAINT-7926 Handle volumes in an animated object as a special case
            const LLVector3* box = avatar->getLastAnimExtents();
            LLVector3 diag = box[1] - box[0];
            radius = diag.magVec() * 0.5f;
            LL_DEBUGS("DynamicBox") << avatar->getFullname() << " diag " << diag << " radius " << radius << LL_ENDL;
        }
        else
        {
            // Volume in a rigged mesh attached to a regular avatar.
            // Note this isn't really a radius, so distance calcs are off by factor of 2
            //radius = avatar->getBinRadius();
            // SL-937: add dynamic box handling for rigged mesh on regular avatars.
            const LLVector3* box = avatar->getLastAnimExtents();
            LLVector3 diag = box[1] - box[0];
            radius = diag.magVec(); // preserve old BinRadius behavior - 2x off
            LL_DEBUGS("DynamicBox") << avatar->getFullname() << " diag " << diag << " radius " << radius << LL_ENDL;
        }
        if (distance <= 0.f || radius <= 0.f)
        {
            LL_DEBUGS("DynamicBox","CalcLOD") << "avatar distance/radius uninitialized, skipping" << LL_ENDL;
            return FALSE;
        }
	}
	else
	{
		distance = mDrawable->mDistanceWRTCamera;
		radius = getVolume() ? getVolume()->mLODScaleBias.scaledVec(getScale()).length() : getScale().length();
        if (distance <= 0.f || radius <= 0.f)
        {
            LL_DEBUGS("DynamicBox","CalcLOD") << "non-avatar distance/radius uninitialized, skipping" << LL_ENDL;
            return FALSE;
        }
	}
	
	//hold onto unmodified distance for debugging
	//F32 debug_distance = distance;

    mLODDistance = distance;
    mLODRadius = radius;

    static LLCachedControl<bool> debug_lods(gSavedSettings, "DebugObjectLODs", false);
    if (debug_lods)
    {
        if (getAvatar() && isRootEdit())
        {
            std::string debug_object_text = get_debug_object_lod_text(this);
            setDebugText(debug_object_text);
            mResetDebugText = true;
        }
    }
    else
    {
        if (mResetDebugText)
        {
            restoreHudText();
            mResetDebugText = false;
        }
    }

    distance *= sDistanceFactor;

	F32 rampDist = LLVOVolume::sLODFactor * 2;
	
	if (distance < rampDist)
	{
		// Boost LOD when you're REALLY close
		distance *= 1.0f/rampDist;
		distance *= distance;
		distance *= rampDist;
	}
	

	distance *= F_PI/3.f;

	static LLCachedControl<bool> ignore_fov_zoom(gSavedSettings,"IgnoreFOVZoomForLODs");
	if(!ignore_fov_zoom)
	{
		lod_factor *= DEFAULT_FIELD_OF_VIEW / LLViewerCamera::getInstance()->getDefaultFOV();
	}

    mLODAdjustedDistance = distance;

    if (isHUDAttachment())
    {
        // HUDs always show at highest detail
        cur_detail = 3;
    }
    else
    {
        cur_detail = computeLODDetail(ll_round(distance, 0.01f), ll_round(radius, 0.01f), lod_factor);
    }

    if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TRIANGLE_COUNT) && mDrawable->getFace(0))
    {
        if (isRootEdit())
        {
            S32 total_tris = recursiveGetTriangleCount();
            S32 est_max_tris = recursiveGetEstTrianglesMax();
            setDebugText(llformat("TRIS SHOWN %d EST %d", total_tris, est_max_tris));
        }
    }
	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_LOD_INFO) &&
		mDrawable->getFace(0))
	{
        // This is a debug display for LODs. Please don't put the texture index here.
        setDebugText(llformat("%d", cur_detail));
	}

	if (cur_detail != mLOD)
	{
        LL_DEBUGS("DynamicBox","CalcLOD") << "new LOD " << cur_detail << " change from " << mLOD 
                             << " distance " << distance << " radius " << radius << " rampDist " << rampDist
                             << " drawable rigged? " << (mDrawable ? (S32) mDrawable->isState(LLDrawable::RIGGED) : (S32) -1)
							 << " mRiggedVolume " << (void*)getRiggedVolume()
                             << " distanceWRTCamera " << (mDrawable ? mDrawable->mDistanceWRTCamera : -1.f)
                             << LL_ENDL;
        
		mAppAngle = ll_round((F32) atan2( mDrawable->getRadius(), mDrawable->mDistanceWRTCamera) * RAD_TO_DEG, 0.01f);
		mLOD = cur_detail;		

        return TRUE;
	}

	return FALSE;
}

BOOL LLVOVolume::updateLOD()
{
	if (mDrawable.isNull())
	{
		return FALSE;
	}
	
	BOOL lod_changed = FALSE;

	if (!LLSculptIDSize::instance().isUnloaded(getVolume()->getParams().getSculptID())) 
	{
		lod_changed = calcLOD();
	}
	else
	{
		return FALSE;
	}

	if (lod_changed)
	{
        if (debugLoggingEnabled("AnimatedObjectsLinkset"))
        {
            if (isAnimatedObject() && isRiggedMesh())
            {
                std::string vobj_name = llformat("Vol%p", this);
                F32 est_tris = getEstTrianglesMax();
                LL_DEBUGS("AnimatedObjectsLinkset") << vobj_name << " updateLOD to " << getLOD() << ", tris " << est_tris << LL_ENDL; 
            }
        }

		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, FALSE);
		mLODChanged = TRUE;
	}
	else
	{
		F32 new_radius = getBinRadius();
		F32 old_radius = mDrawable->getBinRadius();
		if (new_radius < old_radius * 0.9f || new_radius > old_radius*1.1f)
		{
			gPipeline.markPartitionMove(mDrawable);
		}
	}

	lod_changed = lod_changed || LLViewerObject::updateLOD();
	
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
	// There's no guarantee that getVolume()->getNumFaces() == mDrawable->getNumFaces()
	for (S32 i = 0; i < getVolume()->getNumFaces() && i < mDrawable->getNumFaces(); i++)
	{
		LLFace *face = mDrawable->getFace(i);
		if (face)
		{
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
}

BOOL LLVOVolume::setParent(LLViewerObject* parent)
{
	BOOL ret = FALSE ;
    LLViewerObject *old_parent = (LLViewerObject*) getParent();
	if (parent != old_parent)
	{
		ret = LLViewerObject::setParent(parent);
		if (ret && mDrawable)
		{
			gPipeline.markMoved(mDrawable);
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
		}
        onReparent(old_parent, parent);
	}

	return ret ;
}

// NOTE: regenFaces() MUST be followed by genTriangles()!
void LLVOVolume::regenFaces()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;
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
		if (!facep) continue;

		facep->setTEOffset(i);
		facep->setTexture(getTEImage(i));
		if (facep->getTextureEntry()->getMaterialParams().notNull())
		{
			facep->setNormalMap(getTENormalMap(i));
			facep->setSpecularMap(getTESpecularMap(i));
		}
		facep->setViewerObject(this);
		
		// If the face had media on it, this will have broken the link between the LLViewerMediaTexture and the face.
		// Re-establish the link.
		if((int)mMediaImplList.size() > i)
		{
			if(mMediaImplList[i])
			{
				LLViewerMediaTexture* media_tex = LLViewerTextureManager::findMediaTexture(mMediaImplList[i]->getMediaTextureID()) ;
				if(media_tex)
				{
					media_tex->addMediaToFace(facep) ;
				}
			}
		}
	}
	
	if (!count_changed)
	{
		updateFaceFlags();
	}
}

BOOL LLVOVolume::genBBoxes(BOOL force_global, BOOL should_update_octree_bounds)
{
    LL_PROFILE_ZONE_SCOPED;
    BOOL res = TRUE;

    LLVector4a min, max;

    min.clear();
    max.clear();

    BOOL rebuild = mDrawable->isState(LLDrawable::REBUILD_VOLUME | LLDrawable::REBUILD_POSITION | LLDrawable::REBUILD_RIGGED);

    if (getRiggedVolume())
    {
        // MAINT-8264 - better to use the existing call in calling
        // func LLVOVolume::updateGeometry() if we can detect when
        // updates needed, set REBUILD_RIGGED accordingly.

        // Without the flag, this will remove unused rigged volumes, which we are not currently very aggressive about.
        updateRiggedVolume(false);
    }

    LLVolume* volume = mRiggedVolume;
    if (!volume)
    {
        volume = getVolume();
    }

    bool any_valid_boxes = false;

    if (getRiggedVolume())
    {
        LL_DEBUGS("RiggedBox") << "rebuilding box, volume face count " << getVolume()->getNumVolumeFaces() << " drawable face count " << mDrawable->getNumFaces() << LL_ENDL;
    }

    // There's no guarantee that getVolume()->getNumFaces() == mDrawable->getNumFaces()
    for (S32 i = 0;
        i < getVolume()->getNumVolumeFaces() && i < mDrawable->getNumFaces() && i < getNumTEs();
        i++)
    {
        LLFace* face = mDrawable->getFace(i);
        if (!face)
        {
            continue;
        }

        BOOL face_res = face->genVolumeBBoxes(*volume, i,
            mRelativeXform,
            (mVolumeImpl && mVolumeImpl->isVolumeGlobal()) || force_global);
        res &= face_res; // note that this result is never used

        // MAINT-8264 - ignore bboxes of ill-formed faces.
        if (!face_res)
        {
            continue;
        }
        if (rebuild)
        {
            if (getRiggedVolume())
            {
                LL_DEBUGS("RiggedBox") << "rebuilding box, face " << i << " extents " << face->mExtents[0] << ", " << face->mExtents[1] << LL_ENDL;
            }
            if (!any_valid_boxes)
            {
                min = face->mExtents[0];
                max = face->mExtents[1];
                any_valid_boxes = true;
            }
            else
            {
                min.setMin(min, face->mExtents[0]);
                max.setMax(max, face->mExtents[1]);
            }
        }
    }

    if (any_valid_boxes)
    {
        if (rebuild && should_update_octree_bounds)
        {
            //get the Avatar associated with this object if it's rigged
            LLVOAvatar* avatar = nullptr;
            if (isRiggedMesh())
            {
                if (!isAnimatedObject())
                {
                    if (isAttachment())
                    {
                        avatar = getAvatar();
                    }
                }
                else
                {
                    LLControlAvatar* controlAvatar = getControlAvatar();
                    if (controlAvatar && controlAvatar->mPlaying)
                    {
                        avatar = controlAvatar;
                    }
                }
            }

            mDrawable->setSpatialExtents(min, max);

            if (avatar)
            {
                // put all rigged drawables in the same octree node for better batching
                mDrawable->setPositionGroup(LLVector4a(0, 0, 0));
            }
            else
            {
                min.add(max);
                min.mul(0.5f);
                mDrawable->setPositionGroup(min);
            }
        }
        
        updateRadius();
        mDrawable->movePartition();
    }
    else
    {
        LL_DEBUGS("RiggedBox") << "genBBoxes failed to find any valid face boxes" << LL_ENDL;
    }

    return res;
}

void LLVOVolume::preRebuild()
{
	if (mVolumeImpl != NULL)
	{
		mVolumeImpl->preRebuild();
	}
}

void LLVOVolume::updateRelativeXform(bool force_identity)
{
	if (mVolumeImpl)
	{
		mVolumeImpl->updateRelativeXform(force_identity);
		return;
	}
	
	LLDrawable* drawable = mDrawable;
	
	if (drawable->isState(LLDrawable::RIGGED) && mRiggedVolume.notNull())
	{ //rigged volume (which is in agent space) is used for generating bounding boxes etc
	  //inverse of render matrix should go to partition space
		mRelativeXform = getRenderMatrix();

		F32* dst = (F32*) mRelativeXformInvTrans.mMatrix;
		F32* src = (F32*) mRelativeXform.mMatrix;
		dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
		dst[3] = src[4]; dst[4] = src[5]; dst[5] = src[6];
		dst[6] = src[8]; dst[7] = src[9]; dst[8] = src[10];
		
		mRelativeXform.invert();
		mRelativeXformInvTrans.transpose();
	}
	else if (drawable->isActive() || force_identity)
	{				
		// setup relative transforms
		LLQuaternion delta_rot;
		LLVector3 delta_pos, delta_scale;
		
		//matrix from local space to parent relative/global space
		bool use_identity = force_identity || drawable->isSpatialRoot();
		delta_rot = use_identity ? LLQuaternion() : mDrawable->getRotation();
		delta_pos = use_identity ? LLVector3(0,0,0) : mDrawable->getPosition();
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

bool LLVOVolume::lodOrSculptChanged(LLDrawable *drawable, BOOL &compiled, BOOL &should_update_octree_bounds)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;
	bool regen_faces = false;

	LLVolume *old_volumep, *new_volumep;
	F32 old_lod, new_lod;
	S32 old_num_faces, new_num_faces;

	old_volumep = getVolume();
	old_lod = old_volumep->getDetail();
	old_num_faces = old_volumep->getNumFaces();
	old_volumep = NULL;

	{
		const LLVolumeParams &volume_params = getVolume()->getParams();
		setVolume(volume_params, 0);
	}

	new_volumep = getVolume();
	new_lod = new_volumep->getDetail();
	new_num_faces = new_volumep->getNumFaces();
	new_volumep = NULL;

	if ((new_lod != old_lod) || mSculptChanged)
	{
        if (mDrawable->isState(LLDrawable::RIGGED))
        {
            updateVisualComplexity();
        }

		compiled = TRUE;
        // new_lod > old_lod breaks a feedback loop between LOD updates and
        // bounding box updates.
        should_update_octree_bounds = should_update_octree_bounds || mSculptChanged || new_lod > old_lod;
		sNumLODChanges += new_num_faces;

		if ((S32)getNumTEs() != getVolume()->getNumFaces())
		{
			setNumTEs(getVolume()->getNumFaces()); //mesh loading may change number of faces.
		}

		drawable->setState(LLDrawable::REBUILD_VOLUME); // for face->genVolumeTriangles()

		{
			regen_faces = new_num_faces != old_num_faces || mNumFaces != (S32)getNumTEs();
			if (regen_faces)
			{
				regenFaces();
			}

			if (mSculptChanged)
			{ //changes in sculpt maps can thrash an object bounding box without 
				//triggering a spatial group bounding box update -- force spatial group
				//to update bounding boxes
				LLSpatialGroup* group = mDrawable->getSpatialGroup();
				if (group)
				{
					group->unbound();
				}
			}
		}
	}

	return regen_faces;
}

BOOL LLVOVolume::updateGeometry(LLDrawable *drawable)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;
	
	if (mDrawable->isState(LLDrawable::REBUILD_RIGGED))
	{
        updateRiggedVolume(false);
		genBBoxes(FALSE);
		mDrawable->clearState(LLDrawable::REBUILD_RIGGED);
	}

	if (mVolumeImpl != NULL)
	{
		BOOL res;
		{
			res = mVolumeImpl->doUpdateGeometry(drawable);
		}
		updateFaceFlags();
		return res;
	}
	
	LLSpatialGroup* group = drawable->getSpatialGroup();
	if (group)
	{
		group->dirtyMesh();
	}

	updateRelativeXform();
	
	if (mDrawable.isNull()) // Not sure why this is happening, but it is...
	{
		return TRUE; // No update to complete
	}

	BOOL compiled = FALSE;
    // This should be true in most cases, unless we're sure no octree update is
    // needed.
    BOOL should_update_octree_bounds = bool(getRiggedVolume()) || mDrawable->isState(LLDrawable::REBUILD_POSITION) || !mDrawable->getSpatialExtents()->isFinite3();

	if (mVolumeChanged || mFaceMappingChanged)
	{
		dirtySpatialGroup(drawable->isState(LLDrawable::IN_REBUILD_Q1));

		bool was_regen_faces = false;
        should_update_octree_bounds = true;

		if (mVolumeChanged)
		{
            was_regen_faces = lodOrSculptChanged(drawable, compiled, should_update_octree_bounds);
			drawable->setState(LLDrawable::REBUILD_VOLUME);
		}
		else if (mSculptChanged || mLODChanged || mColorChanged)
		{
			compiled = TRUE;
            was_regen_faces = lodOrSculptChanged(drawable, compiled, should_update_octree_bounds);
		}

		if (!was_regen_faces) {
			regenFaces();
		}
	}
	else if (mLODChanged || mSculptChanged || mColorChanged)
	{
		dirtySpatialGroup(drawable->isState(LLDrawable::IN_REBUILD_Q1));
		compiled = TRUE;
        lodOrSculptChanged(drawable, compiled, should_update_octree_bounds);
		
		if(drawable->isState(LLDrawable::REBUILD_RIGGED | LLDrawable::RIGGED)) 
		{
			updateRiggedVolume(false);
		}
	}
	// it has its own drawable (it's moved) or it has changed UVs or it has changed xforms from global<->local
	else
	{
		compiled = TRUE;
		// All it did was move or we changed the texture coordinate offset
	}

    // Generate bounding boxes if needed, and update the object's size in the
    // octree
    genBBoxes(FALSE, should_update_octree_bounds);

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
    mColorChanged = FALSE;
	
	return LLViewerObject::updateGeometry(drawable);
}

void LLVOVolume::updateFaceSize(S32 idx)
{
	if( mDrawable->getNumFaces() <= idx )
	{
		return;
	}

	LLFace* facep = mDrawable->getFace(idx);
	if (facep)
	{
		if (idx >= getVolume()->getNumVolumeFaces())
		{
			facep->setSize(0,0, true);
		}
		else
		{
			const LLVolumeFace& vol_face = getVolume()->getVolumeFace(idx);
			facep->setSize(vol_face.mNumVertices, vol_face.mNumIndices, 
							true); // <--- volume faces should be padded for 16-byte alignment
		
		}
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
		U8 end = (U8)(mMediaImplList.size()) ;
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


//virtual
void LLVOVolume::changeTEImage(S32 index, LLViewerTexture* imagep)
{
	BOOL changed = (mTEImages[index] != imagep);
	LLViewerObject::changeTEImage(index, imagep);
	if (changed)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
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
        if (mDrawable)
        {
            // dynamic texture changes break batches, isolate in octree
            shrinkWrap();
            gPipeline.markTextured(mDrawable);
        }
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
		LL_WARNS("MaterialTEs") << "No texture entry for te " << (S32)te << ", object " << mID << LL_ENDL;
	}
	else if (color != tep->getColor())
	{
		F32 old_alpha = tep->getColor().mV[3];
		if (color.mV[3] != old_alpha)
		{
			gPipeline.markTextured(mDrawable);
			//treat this alpha change as an LoD update since render batches may need to get rebuilt
			mLODChanged = TRUE;
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, FALSE);
		}
		retval = LLPrimitive::setTEColor(te, color);
		if (mDrawable.notNull() && retval)
		{
			// These should only happen on updates which are not the initial update.
            mColorChanged = TRUE;
			mDrawable->setState(LLDrawable::REBUILD_COLOR);
            shrinkWrap();
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
        if (mDrawable)
        {
            gPipeline.markTextured(mDrawable);
            shrinkWrap();
        }
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

void LLVOVolume::setTEMaterialParamsCallbackTE(const LLUUID& objectID, const LLMaterialID &pMaterialID, const LLMaterialPtr pMaterialParams, U32 te)
{
	LLVOVolume* pVol = (LLVOVolume*)gObjectList.findObject(objectID);
	if (pVol)
	{
		LL_DEBUGS("MaterialTEs") << "materialid " << pMaterialID.asString() << " to TE " << te << LL_ENDL;
		if (te >= pVol->getNumTEs())
			return;

		LLTextureEntry* texture_entry = pVol->getTE(te);
		if (texture_entry && (texture_entry->getMaterialID() == pMaterialID))
		{
			pVol->setTEMaterialParams(te, pMaterialParams);
		}
	}
}

S32 LLVOVolume::setTEMaterialID(const U8 te, const LLMaterialID& pMaterialID)
{
	S32 res = LLViewerObject::setTEMaterialID(te, pMaterialID);
	LL_DEBUGS("MaterialTEs") << "te "<< (S32)te << " materialid " << pMaterialID.asString() << " res " << res
								<< ( LLSelectMgr::getInstance()->getSelection()->contains(const_cast<LLVOVolume*>(this), te) ? " selected" : " not selected" )
								<< LL_ENDL;
		
	LL_DEBUGS("MaterialTEs") << " " << pMaterialID.asString() << LL_ENDL;
	if (res)
	{
		LLMaterialMgr::instance().getTE(getRegion()->getRegionID(), pMaterialID, te, boost::bind(&LLVOVolume::setTEMaterialParamsCallbackTE, getID(), _1, _2, _3));

		setChanged(ALL_CHANGED);
		if (!mDrawable.isNull())
		{
			gPipeline.markTextured(mDrawable);
			gPipeline.markRebuild(mDrawable,LLDrawable::REBUILD_ALL);
		}
		mFaceMappingChanged = TRUE;
	}
	return res;
}

S32 LLVOVolume::setTEMaterialParams(const U8 te, const LLMaterialPtr pMaterialParams)
{
	S32 res = LLViewerObject::setTEMaterialParams(te, pMaterialParams);

	LL_DEBUGS("MaterialTEs") << "te " << (S32)te << " material " << ((pMaterialParams) ? pMaterialParams->asLLSD() : LLSD("null")) << " res " << res
							 << ( LLSelectMgr::getInstance()->getSelection()->contains(const_cast<LLVOVolume*>(this), te) ? " selected" : " not selected" )
							 << LL_ENDL;
	setChanged(ALL_CHANGED);
	if (!mDrawable.isNull())
	{
		gPipeline.markTextured(mDrawable);
		gPipeline.markRebuild(mDrawable,LLDrawable::REBUILD_ALL);
	}
	mFaceMappingChanged = TRUE;
	return TEM_CHANGE_TEXTURE;
}

S32 LLVOVolume::setTEGLTFMaterialOverride(U8 te, LLGLTFMaterial* mat)
{
    S32 retval = LLViewerObject::setTEGLTFMaterialOverride(te, mat);

    if (retval == TEM_CHANGE_TEXTURE)
    {
        if (!mDrawable.isNull())
        {
            gPipeline.markTextured(mDrawable);
            gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL);
        }
        mFaceMappingChanged = TRUE;
    }

    return retval;
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

LLVector3 LLVOVolume::getApproximateFaceNormal(U8 face_id)
{
	LLVolume* volume = getVolume();
	LLVector4a result;
	result.clear();

	LLVector3 ret;

	if (volume && face_id < volume->getNumVolumeFaces())
	{
		const LLVolumeFace& face = volume->getVolumeFace(face_id);
		for (S32 i = 0; i < (S32)face.mNumVertices; ++i)
		{
			result.add(face.mNormals[i]);
		}

		LLVector3 ret(result.getF32ptr());
		ret = volumeDirectionToAgent(ret);
		ret.normVec();
	}
	
	return ret;
}

void LLVOVolume::requestMediaDataUpdate(bool isNew)
{
    if (sObjectMediaClient)
		sObjectMediaClient->fetchMedia(new LLMediaDataClientObjectImpl(this, isNew));
}

bool LLVOVolume::isMediaDataBeingFetched() const
{
	// I know what I'm doing by const_casting this away: this is just 
	// a wrapper class that is only going to do a lookup.
	return (sObjectMediaClient) ? sObjectMediaClient->isInQueue(new LLMediaDataClientObjectImpl(const_cast<LLVOVolume*>(this), false)) : false;
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

void LLVOVolume::updateObjectMediaData(const LLSD &media_data_array, const std::string &media_version)
{
	// media_data_array is an array of media entry maps
	// media_version is the version string in the response.
	U32 fetched_version = LLTextureEntry::getVersionFromMediaVersionString(media_version);

	// Only update it if it is newer!
	if ( (S32)fetched_version > mLastFetchedMediaVersion)
	{
		mLastFetchedMediaVersion = fetched_version;
		//LL_INFOS() << "updating:" << this->getID() << " " << ll_pretty_print_sd(media_data_array) << LL_ENDL;
		
		LLSD::array_const_iterator iter = media_data_array.beginArray();
		LLSD::array_const_iterator end = media_data_array.endArray();
		U8 texture_index = 0;
		for (; iter != end; ++iter, ++texture_index)
		{
			syncMediaData(texture_index, *iter, false/*merge*/, false/*ignore_agent*/);
		}
	}
}

void LLVOVolume::syncMediaData(S32 texture_index, const LLSD &media_data, bool merge, bool ignore_agent)
{
	if(mDead)
	{
		// If the object has been marked dead, don't process media updates.
		return;
	}
	
	LLTextureEntry *te = getTE(texture_index);
	if(!te)
	{
		return ;
	}

	LL_DEBUGS("MediaOnAPrim") << "BEFORE: texture_index = " << texture_index
		<< " hasMedia = " << te->hasMedia() << " : " 
		<< ((NULL == te->getMediaData()) ? "NULL MEDIA DATA" : ll_pretty_print_sd(te->getMediaData()->asLLSD())) << LL_ENDL;

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
		viewer_media_t media_impl = LLViewerMedia::getInstance()->updateMediaImpl(mep, previous_url, update_from_self);
			
		addMediaImpl(media_impl, texture_index) ;
	}
	else
	{
		removeMediaImpl(texture_index);
	}

	LL_DEBUGS("MediaOnAPrim") << "AFTER: texture_index = " << texture_index
		<< " hasMedia = " << te->hasMedia() << " : " 
		<< ((NULL == te->getMediaData()) ? "NULL MEDIA DATA" : ll_pretty_print_sd(te->getMediaData()->asLLSD())) << LL_ENDL;
}

void LLVOVolume::mediaNavigateBounceBack(U8 texture_index)
{
	// Find the media entry for this navigate
	const LLMediaEntry* mep = NULL;
	viewer_media_t impl = getMediaImpl(texture_index);
	LLTextureEntry *te = getTE(texture_index);
	if(te)
	{
		mep = te->getMediaData();
	}
	
	if (mep && impl)
	{
        std::string url = mep->getCurrentURL();
		// Look for a ":", if not there, assume "http://"
		if (!url.empty() && std::string::npos == url.find(':')) 
		{
			url = "http://" + url;
		}
		// If the url we're trying to "bounce back" to is either empty or not
		// allowed by the whitelist, try the home url.  If *that* doesn't work,
		// set the media as failed and unload it
        if (url.empty() || !mep->checkCandidateUrl(url))
        {
            url = mep->getHomeURL();
			// Look for a ":", if not there, assume "http://"
			if (!url.empty() && std::string::npos == url.find(':')) 
			{
				url = "http://" + url;
			}
        }
        if (url.empty() || !mep->checkCandidateUrl(url))
		{
			// The url to navigate back to is not good, and we have nowhere else
			// to go.
			LL_WARNS("MediaOnAPrim") << "FAILED to bounce back URL \"" << url << "\" -- unloading impl" << LL_ENDL;
			impl->setMediaFailed(true);
		}
		// Make sure we are not bouncing to url we came from
		else if (impl->getCurrentMediaURL() != url) 
		{
			// Okay, navigate now
            LL_INFOS("MediaOnAPrim") << "bouncing back to URL: " << url << LL_ENDL;
            impl->navigateTo(url, "", false, true);
        }
    }
}

bool LLVOVolume::hasMediaPermission(const LLMediaEntry* media_entry, MediaPermType perm_type)
{
    // NOTE: This logic ALMOST duplicates the logic in the server (in particular, in llmediaservice.cpp).
    if (NULL == media_entry ) return false; // XXX should we assert here?
    
    // The agent has permissions if:
    // - world permissions are on, or
    // - group permissions are on, and agent_id is in the group, or
    // - agent permissions are on, and agent_id is the owner
    
	// *NOTE: We *used* to check for modify permissions here (i.e. permissions were
	// granted if permModify() was true).  However, this doesn't make sense in the
	// viewer: we don't want to show controls or allow interaction if the author
	// has deemed it so.  See DEV-42115.
	
    U8 media_perms = (perm_type == MEDIA_PERM_INTERACT) ? media_entry->getPermsInteract() : media_entry->getPermsControl();
    
    // World permissions
    if (0 != (media_perms & LLMediaEntry::PERM_ANYONE)) 
    {
        return true;
    }
    
    // Group permissions
    else if (0 != (media_perms & LLMediaEntry::PERM_GROUP))
    {
		LLPermissions* obj_perm = LLSelectMgr::getInstance()->findObjectPermissions(this);
		if (obj_perm && gAgent.isInGroup(obj_perm->getGroup()))
		{
			return true;
		}
    }
    
    // Owner permissions
    else if (0 != (media_perms & LLMediaEntry::PERM_OWNER) && permYouOwner()) 
    {
        return true;
    }
    
    return false;
    
}

void LLVOVolume::mediaNavigated(LLViewerMediaImpl *impl, LLPluginClassMedia* plugin, std::string new_location)
{
	bool block_navigation = false;
	// FIXME: if/when we allow the same media impl to be used by multiple faces, the logic here will need to be fixed
	// to deal with multiple face indices.
	int face_index = getFaceIndexWithMediaImpl(impl, -1);
	
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
		if (!block_navigation && !hasMediaPermission(mep, MEDIA_PERM_INTERACT))
		{
			block_navigation = true;
		}
	}
	else
	{
		LL_WARNS("MediaOnAPrim") << "Couldn't find media entry!" << LL_ENDL;
	}
						
	if(block_navigation)
	{
		LL_INFOS("MediaOnAPrim") << "blocking navigate to URI " << new_location << LL_ENDL;

		// "bounce back" to the current URL from the media entry
		mediaNavigateBounceBack(face_index);
	}
	else if (sObjectMediaNavigateClient)
	{
		
		LL_DEBUGS("MediaOnAPrim") << "broadcasting navigate with URI " << new_location << LL_ENDL;

		sObjectMediaNavigateClient->navigate(new LLMediaDataClientObjectImpl(this, false), face_index, new_location);
	}
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
					// This is the first location changed event after the start of a non-server-directed nav.  It may need to be broadcast or bounced back.
					mediaNavigated(impl, plugin, plugin->getLocation());
				}
				break;
				
				case LLViewerMediaImpl::MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS:
					// This navigate didn't change the current URL.  
					LL_DEBUGS("MediaOnAPrim") << "	NOT broadcasting navigate (spurious)" << LL_ENDL;
				break;
				
				case LLViewerMediaImpl::MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED:
					// This is the first location changed event after the start of a server-directed nav.  Don't broadcast it.
					LL_INFOS("MediaOnAPrim") << "	NOT broadcasting navigate (server-directed)" << LL_ENDL;
				break;
				
				default:
					// This is a subsequent location-changed due to a redirect.	 Don't broadcast.
					LL_INFOS("MediaOnAPrim") << "	NOT broadcasting navigate (redirect)" << LL_ENDL;
				break;
			}
		}
		break;
		
		case LLViewerMediaObserver::MEDIA_EVENT_NAVIGATE_COMPLETE:
		{
			switch(impl->getNavState())
			{
				case LLViewerMediaImpl::MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED:
				{
					// This is the first location changed event after the start of a non-server-directed nav.  It may need to be broadcast or bounced back.
					mediaNavigated(impl, plugin, plugin->getNavigateURI());
				}
				break;
				
				case LLViewerMediaImpl::MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS:
					// This navigate didn't change the current URL.  
					LL_DEBUGS("MediaOnAPrim") << "	NOT broadcasting navigate (spurious)" << LL_ENDL;
				break;

				case LLViewerMediaImpl::MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED:
					// This is the the navigate complete event from a server-directed nav.  Don't broadcast it.
					LL_INFOS("MediaOnAPrim") << "	NOT broadcasting navigate (server-directed)" << LL_ENDL;
				break;
				
				default:
					// For all other states, the navigate should have been handled by LOCATION_CHANGED events already.
				break;
			}
		}
		break;

        case LLViewerMediaObserver::MEDIA_EVENT_FILE_DOWNLOAD:
        {
            // Media might be blocked, waiting for a file,
            // send an empty response to unblock it
            const std::vector<std::string> empty_response;
            plugin->sendPickFileResponse(empty_response);

            LLNotificationsUtil::add("MediaFileDownloadUnsupported");
        }
        break;
		
		default:
		break;
	}

}

void LLVOVolume::sendMediaDataUpdate()
{
    if (sObjectMediaClient)
		sObjectMediaClient->updateMedia(new LLMediaDataClientObjectImpl(this, false));
}

void LLVOVolume::removeMediaImpl(S32 texture_index)
{
	if(mMediaImplList.size() <= (U32)texture_index || mMediaImplList[texture_index].isNull())
	{
		return ;
	}

	//make the face referencing to mMediaImplList[texture_index] to point back to the old texture.
	if(mDrawable && texture_index < mDrawable->getNumFaces())
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
		LLFace* facep(NULL);
		if( texture_index < mDrawable->getNumFaces() )
		{
			facep = mDrawable->getFace(texture_index) ;
		}

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

F64 LLVOVolume::getTotalMediaInterest() const
{
	// If this object is currently focused, this object has "high" interest
	if (LLViewerMediaFocus::getInstance()->getFocusedObjectID() == getID())
		return F64_MAX;
	
	F64 interest = (F64)-1.0;  // means not interested;
    
	// If this object is selected, this object has "high" interest, but since 
	// there can be more than one, we still add in calculated impl interest
	// XXX Sadly, 'contains()' doesn't take a const :(
	if (LLSelectMgr::getInstance()->getSelection()->contains(const_cast<LLVOVolume*>(this)))
		interest = F64_MAX / 2.0;
	
	int i = 0;
	const int end = getNumTEs();
	for ( ; i < end; ++i)
	{
		const viewer_media_t &impl = getMediaImpl(i);
		if (!impl.isNull())
		{
			if (interest == (F64)-1.0) interest = (F64)0.0;
			interest += impl->getInterest();
		}
	}
	return interest;
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
	LLViewerTexture* old_texturep = getLightTexture(); // same as mLightTexture, but inits if nessesary
	if (id.notNull())
	{
		if (!hasLightTexture())
		{
			setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE, TRUE, true);
		}
		else if (old_texturep)
		{	
			old_texturep->removeVolume(LLRender::LIGHT_TEX, this);
		}
		LLLightImageParams* param_block = (LLLightImageParams*) getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
		if (param_block && param_block->getLightTexture() != id)
		{
			param_block->setLightTexture(id);
			parameterChanged(LLNetworkData::PARAMS_LIGHT_IMAGE, true);
		}
		LLViewerTexture* tex = getLightTexture();
		if (tex)
		{
			tex->addVolume(LLRender::LIGHT_TEX, this); // new texture
		}
		else
		{
			LL_WARNS() << "Can't get light texture for ID " << id.asString() << LL_ENDL;
		}
	}
	else if (hasLightTexture())
	{
		if (old_texturep)
		{
			old_texturep->removeVolume(LLRender::LIGHT_TEX, this);
		}
		setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE, FALSE, true);
		parameterChanged(LLNetworkData::PARAMS_LIGHT_IMAGE, true);
		mLightTexture = NULL;
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
	BOOL was_light = getIsLight();
	if (is_light != was_light)
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

void LLVOVolume::setLightSRGBColor(const LLColor3& color)
{
    setLightLinearColor(linearColor3(color));
}

void LLVOVolume::setLightLinearColor(const LLColor3& color)
{
	LLLightParams *param_block = (LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		if (param_block->getLinearColor() != color)
		{
			param_block->setLinearColor(LLColor4(color, param_block->getLinearColor().mV[3]));
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
		if (param_block->getLinearColor().mV[3] != intensity)
		{
			param_block->setLinearColor(LLColor4(LLColor3(param_block->getLinearColor()), intensity));
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

LLColor3 LLVOVolume::getLightSRGBBaseColor() const
{
    return srgbColor3(getLightLinearBaseColor());
}

LLColor3 LLVOVolume::getLightLinearBaseColor() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return LLColor3(param_block->getLinearColor());
	}
	else
	{
		return LLColor3(1,1,1);
	}
}

LLColor3 LLVOVolume::getLightLinearColor() const
{
    const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
    if (param_block)
    {
        return LLColor3(param_block->getLinearColor()) * param_block->getLinearColor().mV[3];
    }
    else
    {
        return LLColor3(1, 1, 1);
    }
}

LLColor3 LLVOVolume::getLightSRGBColor() const
{
    LLColor3 ret = getLightLinearColor();
    ret = srgbColor3(ret);
    return ret;
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
    if (gCubeSnapshot)
    {
        return;
    }

    F32 r = getLightRadius();
	LLVector3 pos = mDrawable->getPositionAgent();

	LLVector3 at(0,0,-1);
	at *= getRenderRotation();
	pos += at * r;

	at = LLViewerCamera::getInstance()->getAtAxis();
	pos -= at * r;

	mSpotLightPriority = gPipeline.calcPixelArea(pos, LLVector3(r,r,r), *LLViewerCamera::getInstance());

	if (mLightTexture.notNull())
	{
		mLightTexture->addTextureStats(mSpotLightPriority);
		mLightTexture->setBoostLevel(LLGLTexture::BOOST_CLOUDS);
	}
}


bool LLVOVolume::isLightSpotlight() const
{
	LLLightImageParams* params = (LLLightImageParams*) getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
	if (params && getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE))
	{
		return params->isLightSpotlight();
	}
	return false;
}


LLViewerTexture* LLVOVolume::getLightTexture()
{
	LLUUID id = getLightTextureID();

	if (id.notNull())
	{
		if (mLightTexture.isNull() || id != mLightTexture->getID())
		{
			mLightTexture = LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_ALM);
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
		return param_block->getLinearColor().mV[3];
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

F32 LLVOVolume::getLightFalloff(const F32 fudge_factor) const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return param_block->getFalloff() * fudge_factor;
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

BOOL LLVOVolume::isReflectionProbe() const
{
    return getParameterEntryInUse(LLNetworkData::PARAMS_REFLECTION_PROBE);
}

bool LLVOVolume::setIsReflectionProbe(BOOL is_probe)
{
    BOOL was_probe = isReflectionProbe();
    if (is_probe != was_probe)
    {
        if (is_probe)
        {
            setParameterEntryInUse(LLNetworkData::PARAMS_REFLECTION_PROBE, TRUE, true);
        }
        else
        {
            setParameterEntryInUse(LLNetworkData::PARAMS_REFLECTION_PROBE, FALSE, true);
        }
    }

    updateReflectionProbePtr();

    return was_probe != is_probe;
}

bool LLVOVolume::setReflectionProbeAmbiance(F32 ambiance)
{
    LLReflectionProbeParams* param_block = (LLReflectionProbeParams*)getParameterEntry(LLNetworkData::PARAMS_REFLECTION_PROBE);
    if (param_block)
    {
        if (param_block->getAmbiance() != ambiance)
        {
            param_block->setAmbiance(ambiance);
            parameterChanged(LLNetworkData::PARAMS_REFLECTION_PROBE, true);
            return true;
        }
    }

    return false;
}

bool LLVOVolume::setReflectionProbeNearClip(F32 near_clip)
{
    LLReflectionProbeParams* param_block = (LLReflectionProbeParams*)getParameterEntry(LLNetworkData::PARAMS_REFLECTION_PROBE);
    if (param_block)
    {
        if (param_block->getClipDistance() != near_clip)
        {
            param_block->setClipDistance(near_clip);
            parameterChanged(LLNetworkData::PARAMS_REFLECTION_PROBE, true);
            return true;
        }
    }

    return false;
}

bool LLVOVolume::setReflectionProbeIsBox(bool is_box)
{
    LLReflectionProbeParams* param_block = (LLReflectionProbeParams*)getParameterEntry(LLNetworkData::PARAMS_REFLECTION_PROBE);
    if (param_block)
    {
        if (param_block->getIsBox() != is_box)
        {
            param_block->setIsBox(is_box);
            parameterChanged(LLNetworkData::PARAMS_REFLECTION_PROBE, true);
            return true;
        }
    }

    return false;
}

bool LLVOVolume::setReflectionProbeIsDynamic(bool is_dynamic)
{
    LLReflectionProbeParams* param_block = (LLReflectionProbeParams*)getParameterEntry(LLNetworkData::PARAMS_REFLECTION_PROBE);
    if (param_block)
    {
        if (param_block->getIsDynamic() != is_dynamic)
        {
            param_block->setIsDynamic(is_dynamic);
            parameterChanged(LLNetworkData::PARAMS_REFLECTION_PROBE, true);
            return true;
        }
    }

    return false;
}

F32 LLVOVolume::getReflectionProbeAmbiance() const
{
    const LLReflectionProbeParams* param_block = (const LLReflectionProbeParams*)getParameterEntry(LLNetworkData::PARAMS_REFLECTION_PROBE);
    if (param_block)
    {
        return param_block->getAmbiance();
    }
    else
    {
        return 0.f;
    }
}

F32 LLVOVolume::getReflectionProbeNearClip() const
{
    const LLReflectionProbeParams* param_block = (const LLReflectionProbeParams*)getParameterEntry(LLNetworkData::PARAMS_REFLECTION_PROBE);
    if (param_block)
    {
        return param_block->getClipDistance();
    }
    else
    {
        return 0.f;
    }
}

bool LLVOVolume::getReflectionProbeIsBox() const
{
    const LLReflectionProbeParams* param_block = (const LLReflectionProbeParams*)getParameterEntry(LLNetworkData::PARAMS_REFLECTION_PROBE);
    if (param_block)
    {
        return param_block->getIsBox();
    }
    
    return false;
}

bool LLVOVolume::getReflectionProbeIsDynamic() const
{
    const LLReflectionProbeParams* param_block = (const LLReflectionProbeParams*)getParameterEntry(LLNetworkData::PARAMS_REFLECTION_PROBE);
    if (param_block)
    {
        return param_block->getIsDynamic();
    }

    return false;
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

BOOL LLVOVolume::isMesh() const
{
	if (isSculpted())
	{
		LLSculptParams *sculpt_params = (LLSculptParams *)getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		U8 sculpt_type = sculpt_params->getSculptType();

		if ((sculpt_type & LL_SCULPT_TYPE_MASK) == LL_SCULPT_TYPE_MESH)
			// mesh is a mesh
		{
			return TRUE;	
		}
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
	else if (mRiggedVolume.notNull())
	{
		return TRUE;
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

const LLMeshSkinInfo* LLVOVolume::getSkinInfo() const
{
    if (getVolume())
    {
         return mSkinInfo;
    }
    else
    {
        return NULL;
    }
}

// virtual
BOOL LLVOVolume::isRiggedMesh() const
{
    return isMesh() && getSkinInfo();
}

//----------------------------------------------------------------------------
U32 LLVOVolume::getExtendedMeshFlags() const
{
	const LLExtendedMeshParams *param_block = 
        (const LLExtendedMeshParams *)getParameterEntry(LLNetworkData::PARAMS_EXTENDED_MESH);
	if (param_block)
	{
		return param_block->getFlags();
	}
	else
	{
		return 0;
	}
}

void LLVOVolume::onSetExtendedMeshFlags(U32 flags)
{

    // The isAnySelected() check was needed at one point to prevent
    // graphics problems. These are now believed to be fixed so the
    // check has been disabled.
	if (/*!getRootEdit()->isAnySelected() &&*/ mDrawable.notNull())
    {
        // Need to trigger rebuildGeom(), which is where control avatars get created/removed
        getRootEdit()->recursiveMarkForUpdate(TRUE);
    }
    if (isAttachment() && getAvatarAncestor())
    {
        updateVisualComplexity();
        if (flags & LLExtendedMeshParams::ANIMATED_MESH_ENABLED_FLAG)
        {
            // Making a rigged mesh into an animated object
            getAvatarAncestor()->updateAttachmentOverrides();
        }
        else
        {
            // Making an animated object into a rigged mesh
            getAvatarAncestor()->updateAttachmentOverrides();
        }
    }
}

void LLVOVolume::setExtendedMeshFlags(U32 flags)
{
    U32 curr_flags = getExtendedMeshFlags();
    if (curr_flags != flags)
    {
        bool in_use = true;
        setParameterEntryInUse(LLNetworkData::PARAMS_EXTENDED_MESH, in_use, true);
        LLExtendedMeshParams *param_block = 
            (LLExtendedMeshParams *)getParameterEntry(LLNetworkData::PARAMS_EXTENDED_MESH);
        if (param_block)
        {
            param_block->setFlags(flags);
        }
        parameterChanged(LLNetworkData::PARAMS_EXTENDED_MESH, true);
        LL_DEBUGS("AnimatedObjects") << this
                                     << " new flags " << flags << " curr_flags " << curr_flags
                                     << ", calling onSetExtendedMeshFlags()"
                                     << LL_ENDL;
        onSetExtendedMeshFlags(flags);
    }
}

bool LLVOVolume::canBeAnimatedObject() const
{
    F32 est_tris = recursiveGetEstTrianglesMax();
    if (est_tris < 0 || est_tris > getAnimatedObjectMaxTris())
    {
        return false;
    }
    return true;
}

bool LLVOVolume::isAnimatedObject() const
{
    LLVOVolume *root_vol = (LLVOVolume*)getRootEdit();
    bool root_is_animated_flag = root_vol->getExtendedMeshFlags() & LLExtendedMeshParams::ANIMATED_MESH_ENABLED_FLAG;
    return root_is_animated_flag;
}

// Called any time parenting changes for a volume. Update flags and
// control av accordingly.  This is called after parent has been
// changed to new_parent, but before new_parent's mChildList has changed.

// virtual
void LLVOVolume::onReparent(LLViewerObject *old_parent, LLViewerObject *new_parent)
{
    LLVOVolume *old_volp = dynamic_cast<LLVOVolume*>(old_parent);

    if (new_parent && !new_parent->isAvatar())
    {
        if (mControlAvatar.notNull())
        {
            // Here an animated object is being made the child of some
            // other prim. Should remove the control av from the child.
            LLControlAvatar *av = mControlAvatar;
            mControlAvatar = NULL;
            av->markForDeath();
        }
    }
    if (old_volp && old_volp->isAnimatedObject())
    {
        if (old_volp->getControlAvatar())
        {
            // We have been removed from an animated object, need to do cleanup.
            old_volp->getControlAvatar()->updateAttachmentOverrides();
            old_volp->getControlAvatar()->updateAnimations();
        }
    }
}

// This needs to be called after onReparent(), because mChildList is
// not updated until the end of LLViewerObject::addChild()

// virtual
void LLVOVolume::afterReparent()
{
    {
        LL_DEBUGS("AnimatedObjects") << "new child added for parent " 
            << ((LLViewerObject*)getParent())->getID() << LL_ENDL;
    }
                                                                                             
    if (isAnimatedObject() && getControlAvatar())
    {
        LL_DEBUGS("AnimatedObjects") << "adding attachment overrides, parent is animated object " 
            << ((LLViewerObject*)getParent())->getID() << LL_ENDL;

        // MAINT-8239 - doing a full rebuild whenever parent is set
        // makes the joint overrides load more robustly. In theory,
        // addAttachmentOverrides should be sufficient, but in
        // practice doing a full rebuild helps compensate for
        // notifyMeshLoaded() not being called reliably enough.
        
        // was: getControlAvatar()->addAttachmentOverridesForObject(this);
        //getControlAvatar()->rebuildAttachmentOverrides();
        getControlAvatar()->updateAnimations();
    }
    else
    {
        LL_DEBUGS("AnimatedObjects") << "not adding overrides, parent: " 
                                     << ((LLViewerObject*)getParent())->getID() 
                                     << " isAnimated: "  << isAnimatedObject() << " cav "
                                     << getControlAvatar() << LL_ENDL;
    }
}

//----------------------------------------------------------------------------
void LLVOVolume::updateRiggingInfo()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;
    if (isRiggedMesh())
    {
        const LLMeshSkinInfo* skin = getSkinInfo();
        LLVOAvatar *avatar = getAvatar();
        LLVolume *volume = getVolume();
        if (skin && avatar && volume)
        {
            LL_DEBUGS("RigSpammish") << "starting, vovol " << this << " lod " << getLOD() << " last " << mLastRiggingInfoLOD << LL_ENDL;
            if (getLOD()>mLastRiggingInfoLOD || getLOD()==3)
            {
                // Rigging info may need update
                mJointRiggingInfoTab.clear();
                for (S32 f = 0; f < volume->getNumVolumeFaces(); ++f)
                {
                    LLVolumeFace& vol_face = volume->getVolumeFace(f);
                    LLSkinningUtil::updateRiggingInfo(skin, avatar, vol_face);
                    if (vol_face.mJointRiggingInfoTab.size()>0)
                    {
                        mJointRiggingInfoTab.merge(vol_face.mJointRiggingInfoTab);
                    }
                }
                // Keep the highest LOD info available.
                mLastRiggingInfoLOD = getLOD();
                LL_DEBUGS("RigSpammish") << "updated rigging info for LLVOVolume " 
                                         << this << " lod " << mLastRiggingInfoLOD 
                                         << LL_ENDL;
            }
        }
    }
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
		//mDrawable->mDistanceWRTCamera = view_vector.length();
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

		volume->generateSilhouetteVertices(nodep->mSilhouetteVertices, nodep->mSilhouetteNormals, view_vector, trans_mat, mRelativeXformInvTrans, nodep->getTESelectMask());

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
	return mAttachmentState != 0 ;
}

BOOL LLVOVolume::isHUDAttachment() const
{
	// *NOTE: we assume hud attachment points are in defined range
	// since this range is constant for backwards compatibility
	// reasons this is probably a reasonable assumption to make
	S32 attachment_id = ATTACHMENT_ID_FROM_STATE(mAttachmentState);
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

// Returns a base cost and adds textures to passed in set.
// total cost is returned value + 5 * size of the resulting set.
// Cannot include cost of textures, as they may be re-used in linked
// children, and cost should only be increased for unique textures  -Nyx
U32 LLVOVolume::getRenderCost(texture_cost_t &textures) const
{
    /*****************************************************************
     * This calculation should not be modified by third party viewers,
     * since it is used to limit rendering and should be uniform for
     * everyone. If you have suggested improvements, submit them to
     * the official viewer for consideration.
     *****************************************************************/

	// Get access to params we'll need at various points.  
	// Skip if this is object doesn't have a volume (e.g. is an avatar).
	BOOL has_volume = (getVolume() != NULL);
	LLVolumeParams volume_params;
	LLPathParams path_params;
	LLProfileParams profile_params;

	U32 num_triangles = 0;

	// per-prim costs
	static const U32 ARC_PARTICLE_COST = 1; // determined experimentally
	static const U32 ARC_PARTICLE_MAX = 2048; // default values
	static const U32 ARC_TEXTURE_COST = 16; // multiplier for texture resolution - performance tested
	static const U32 ARC_LIGHT_COST = 500; // static cost for light-producing prims 
	static const U32 ARC_MEDIA_FACE_COST = 1500; // static cost per media-enabled face 


	// per-prim multipliers
	static const F32 ARC_GLOW_MULT = 1.5f; // tested based on performance
	static const F32 ARC_BUMP_MULT = 1.25f; // tested based on performance
	static const F32 ARC_FLEXI_MULT = 5; // tested based on performance
	static const F32 ARC_SHINY_MULT = 1.6f; // tested based on performance
	static const F32 ARC_INVISI_COST = 1.2f; // tested based on performance
	static const F32 ARC_WEIGHTED_MESH = 1.2f; // tested based on performance

	static const F32 ARC_PLANAR_COST = 1.0f; // tested based on performance to have negligible impact
	static const F32 ARC_ANIM_TEX_COST = 4.f; // tested based on performance
	static const F32 ARC_ALPHA_COST = 4.f; // 4x max - based on performance

	F32 shame = 0;

	U32 invisi = 0;
	U32 shiny = 0;
	U32 glow = 0;
	U32 alpha = 0;
	U32 flexi = 0;
	U32 animtex = 0;
	U32 particles = 0;
	U32 bump = 0;
	U32 planar = 0;
	U32 weighted_mesh = 0;
	U32 produces_light = 0;
	U32 media_faces = 0;

	const LLDrawable* drawablep = mDrawable;
	U32 num_faces = drawablep->getNumFaces();

	if (has_volume)
	{
		volume_params = getVolume()->getParams();
		path_params = volume_params.getPathParams();
		profile_params = volume_params.getProfileParams();

        LLMeshCostData costs;
		if (getCostData(costs))
		{
            if (isAnimatedObject() && isRiggedMesh())
            {
                // Scaling here is to make animated object vs
                // non-animated object ARC proportional to the
                // corresponding calculations for streaming cost.
                num_triangles = (ANIMATED_OBJECT_COST_PER_KTRI * 0.001 * costs.getEstTrisForStreamingCost())/0.06;
            }
            else
            {
                F32 radius = getScale().length()*0.5f;
                num_triangles = costs.getRadiusWeightedTris(radius);
            }
		}
	}

	if (num_triangles <= 0)
	{
		num_triangles = 4;
	}

	if (isSculpted())
	{
		if (isMesh())
		{
			// base cost is dependent on mesh complexity
			// note that 3 is the highest LOD as of the time of this coding.
			S32 size = gMeshRepo.getMeshSize(volume_params.getSculptID(), getLOD());
			if ( size > 0)
			{
				if (isRiggedMesh())
				{
					// weighted attachment - 1 point for every 3 bytes
					weighted_mesh = 1;
				}
			}
			else
			{
				// something went wrong - user should know their content isn't render-free
				return 0;
			}
		}
		else
		{
			const LLSculptParams *sculpt_params = (LLSculptParams *) getParameterEntry(LLNetworkData::PARAMS_SCULPT);
			LLUUID sculpt_id = sculpt_params->getSculptTexture();
			if (textures.find(sculpt_id) == textures.end())
			{
				LLViewerFetchedTexture *texture = LLViewerTextureManager::getFetchedTexture(sculpt_id);
				if (texture)
				{
					S32 texture_cost = 256 + (S32)(ARC_TEXTURE_COST * (texture->getFullHeight() / 128.f + texture->getFullWidth() / 128.f));
					textures.insert(texture_cost_t::value_type(sculpt_id, texture_cost));
				}
			}
		}
	}

	if (isFlexible())
	{
		flexi = 1;
	}
	if (isParticleSource())
	{
		particles = 1;
	}

	if (getIsLight())
	{
		produces_light = 1;
	}

	for (S32 i = 0; i < num_faces; ++i)
	{
		const LLFace* face = drawablep->getFace(i);
		if (!face) continue;
		const LLTextureEntry* te = face->getTextureEntry();
		const LLViewerTexture* img = face->getTexture();

		if (img)
		{
			if (textures.find(img->getID()) == textures.end())
			{
                S32 texture_cost = 0;
                S8 type = img->getType();
                if (type == LLViewerTexture::FETCHED_TEXTURE || type == LLViewerTexture::LOD_TEXTURE)
                {
                    const LLViewerFetchedTexture* fetched_texturep = static_cast<const LLViewerFetchedTexture*>(img);
                    if (fetched_texturep
                        && fetched_texturep->getFTType() == FTT_LOCAL_FILE
                        && (img->getID() == IMG_ALPHA_GRAD_2D || img->getID() == IMG_ALPHA_GRAD)
                        )
                    {
                        // These two textures appear to switch between each other, but are of different sizes (4x256 and 256x256).
                        // Hardcode cost from larger one to not cause random complexity changes
                        texture_cost = 320;
                    }
                }
                if (texture_cost == 0)
                {
                    texture_cost = 256 + (S32)(ARC_TEXTURE_COST * (img->getFullHeight() / 128.f + img->getFullWidth() / 128.f));
                }
				textures.insert(texture_cost_t::value_type(img->getID(), texture_cost));
			}
		}

		if (face->isInAlphaPool())
		{
			alpha = 1;
		}
		else if (img && img->getPrimaryFormat() == GL_ALPHA)
		{
			invisi = 1;
		}
		if (face->hasMedia())
		{
			media_faces++;
		}

		if (te)
		{
			if (te->getBumpmap())
			{
				// bump is a multiplier, don't add per-face
				bump = 1;
			}
			if (te->getShiny())
			{
				// shiny is a multiplier, don't add per-face
				shiny = 1;
			}
			if (te->getGlow() > 0.f)
			{
				// glow is a multiplier, don't add per-face
				glow = 1;
			}
			if (face->mTextureMatrix != NULL)
			{
				animtex = 1;
			}
			if (te->getTexGen())
			{
				planar = 1;
			}
		}
	}

	// shame currently has the "base" cost of 1 point per 15 triangles, min 2.
	shame = num_triangles  * 5.f;
	shame = shame < 2.f ? 2.f : shame;

	// multiply by per-face modifiers
	if (planar)
	{
		shame *= planar * ARC_PLANAR_COST;
	}

	if (animtex)
	{
		shame *= animtex * ARC_ANIM_TEX_COST;
	}

	if (alpha)
	{
		shame *= alpha * ARC_ALPHA_COST;
	}

	if(invisi)
	{
		shame *= invisi * ARC_INVISI_COST;
	}

	if (glow)
	{
		shame *= glow * ARC_GLOW_MULT;
	}

	if (bump)
	{
		shame *= bump * ARC_BUMP_MULT;
	}

	if (shiny)
	{
		shame *= shiny * ARC_SHINY_MULT;
	}


	// multiply shame by multipliers
	if (weighted_mesh)
	{
		shame *= weighted_mesh * ARC_WEIGHTED_MESH;
	}

	if (flexi)
	{
		shame *= flexi * ARC_FLEXI_MULT;
	}


	// add additional costs
	if (particles)
	{
		const LLPartSysData *part_sys_data = &(mPartSourcep->mPartSysData);
		const LLPartData *part_data = &(part_sys_data->mPartData);
		U32 num_particles = (U32)(part_sys_data->mBurstPartCount * llceil( part_data->mMaxAge / part_sys_data->mBurstRate));
		num_particles = num_particles > ARC_PARTICLE_MAX ? ARC_PARTICLE_MAX : num_particles;
		F32 part_size = (llmax(part_data->mStartScale[0], part_data->mEndScale[0]) + llmax(part_data->mStartScale[1], part_data->mEndScale[1])) / 2.f;
		shame += num_particles * part_size * ARC_PARTICLE_COST;
	}

	if (produces_light)
	{
		shame += ARC_LIGHT_COST;
	}

	if (media_faces)
	{
		shame += media_faces * ARC_MEDIA_FACE_COST;
	}

    // Streaming cost for animated objects includes a fixed cost
    // per linkset. Add a corresponding charge here translated into
    // triangles, but not weighted by any graphics properties.
    if (isAnimatedObject() && isRootEdit())
    {
        shame += (ANIMATED_OBJECT_BASE_COST/0.06) * 5.0f;
    }

	if (shame > mRenderComplexity_current)
	{
		mRenderComplexity_current = (S32)shame;
	}

	return (U32)shame;
}

F32 LLVOVolume::getEstTrianglesMax() const
{
	if (isMesh() && getVolume())
	{
		return gMeshRepo.getEstTrianglesMax(getVolume()->getParams().getSculptID());
	}
    return 0.f;
}

F32 LLVOVolume::getEstTrianglesStreamingCost() const
{
	if (isMesh() && getVolume())
	{
		return gMeshRepo.getEstTrianglesStreamingCost(getVolume()->getParams().getSculptID());
	}
    return 0.f;
}

F32 LLVOVolume::getStreamingCost() const
{
	F32 radius = getScale().length()*0.5f;
    F32 linkset_base_cost = 0.f;

    LLMeshCostData costs;
    if (getCostData(costs))
    {
        if (isAnimatedObject() && isRootEdit())
        {
            // Root object of an animated object has this to account for skeleton overhead.
            linkset_base_cost = ANIMATED_OBJECT_BASE_COST;
        }
        if (isMesh())
        {
            if (isAnimatedObject() && isRiggedMesh())
            {
                return linkset_base_cost + costs.getTriangleBasedStreamingCost();
            }
            else
            {
                return linkset_base_cost + costs.getRadiusBasedStreamingCost(radius);
            }
        }
        else
        {
            return linkset_base_cost + costs.getRadiusBasedStreamingCost(radius);
        }
    }
    else
    {
        return 0.f;
    }
}

// virtual
bool LLVOVolume::getCostData(LLMeshCostData& costs) const
{
    if (isMesh())
    {
        return gMeshRepo.getCostData(getVolume()->getParams().getSculptID(), costs);
    }
    else
    {
		LLVolume* volume = getVolume();
		S32 counts[4];
		LLVolume::getLoDTriangleCounts(volume->getParams(), counts);

		LLSD header;
		header["lowest_lod"]["size"] = counts[0] * 10;
		header["low_lod"]["size"] = counts[1] * 10;
		header["medium_lod"]["size"] = counts[2] * 10;
		header["high_lod"]["size"] = counts[3] * 10;

		return gMeshRepo.getCostData(header, costs);
    }
}

//static 
void LLVOVolume::updateRenderComplexity()
{
	mRenderComplexity_last = mRenderComplexity_current;
	mRenderComplexity_current = 0;
}

U32 LLVOVolume::getTriangleCount(S32* vcount) const
{
	U32 count = 0;
	LLVolume* volume = getVolume();
	if (volume)
	{
		count = volume->getNumTriangles(vcount);
	}

	return count;
}

U32 LLVOVolume::getHighLODTriangleCount()
{
	U32 ret = 0;

	LLVolume* volume = getVolume();

	if (!isSculpted())
	{
		LLVolume* ref = LLPrimitive::getVolumeManager()->refVolume(volume->getParams(), 3);
		ret = ref->getNumTriangles();
		LLPrimitive::getVolumeManager()->unrefVolume(ref);
	}
	else if (isMesh())
	{
		LLVolume* ref = LLPrimitive::getVolumeManager()->refVolume(volume->getParams(), 3);
		if (!ref->isMeshAssetLoaded() || ref->getNumVolumeFaces() == 0)
		{
			gMeshRepo.loadMesh(this, volume->getParams(), LLModel::LOD_HIGH);
		}
		ret = ref->getNumTriangles();
		LLPrimitive::getVolumeManager()->unrefVolume(ref);
	}
	else
	{ //default sculpts have a constant number of triangles
		ret = 31*2*31;  //31 rows of 31 columns of quads for a 32x32 vertex patch
	}

	return ret;
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
    if (!local_origin && param_type == LLNetworkData::PARAMS_EXTENDED_MESH)
    {
        U32 extended_mesh_flags = getExtendedMeshFlags();
        bool enabled =  (extended_mesh_flags & LLExtendedMeshParams::ANIMATED_MESH_ENABLED_FLAG);
        bool was_enabled = (getControlAvatar() != NULL);
        if (enabled != was_enabled)
        {
            LL_DEBUGS("AnimatedObjects") << this
                                         << " calling onSetExtendedMeshFlags, enabled " << (U32) enabled
                                         << " was_enabled " << (U32) was_enabled
                                         << " local_origin " << (U32) local_origin
                                         << LL_ENDL;
            onSetExtendedMeshFlags(extended_mesh_flags);
        }
    }
	if (mDrawable.notNull())
	{
		BOOL is_light = getIsLight();
		if (is_light != mDrawable->isState(LLDrawable::LIGHT))
		{
			gPipeline.setLight(mDrawable, is_light);
		}
	}
   
    updateReflectionProbePtr();
}

void LLVOVolume::updateReflectionProbePtr()
{
    if (isReflectionProbe())
    {
        if (mReflectionProbe.isNull())
        {
            mReflectionProbe = gPipeline.mReflectionMapManager.registerViewerObject(this);
        }
    }
    else if (mReflectionProbe.notNull())
    {
        mReflectionProbe = nullptr;
    }
}

void LLVOVolume::setSelected(BOOL sel)
{
	LLViewerObject::setSelected(sel);
    if (isAnimatedObject())
    {
        getRootEdit()->recursiveMarkForUpdate(TRUE);
    }
    else
    {
        if (mDrawable.notNull())
        {
            markForUpdate(TRUE);
        }
    }
}

void LLVOVolume::updateSpatialExtents(LLVector4a& newMin, LLVector4a& newMax)
{		
}

F32 LLVOVolume::getBinRadius()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;
    F32 radius;

    static LLCachedControl<S32> octree_size_factor(gSavedSettings, "OctreeStaticObjectSizeFactor", 3);
    static LLCachedControl<S32> octree_attachment_size_factor(gSavedSettings, "OctreeAttachmentSizeFactor", 4);
    static LLCachedControl<LLVector3> octree_distance_factor(gSavedSettings, "OctreeDistanceFactor", LLVector3(0.01f, 0.f, 0.f));
    static LLCachedControl<LLVector3> octree_alpha_distance_factor(gSavedSettings, "OctreeAlphaDistanceFactor", LLVector3(0.1f, 0.f, 0.f));

    S32 size_factor = llmax((S32)octree_size_factor, 1);
    LLVector3 alpha_distance_factor = octree_alpha_distance_factor;

    //const LLVector4a* ext = mDrawable->getSpatialExtents();

    bool shrink_wrap = mShouldShrinkWrap || mDrawable->isAnimating();
    bool alpha_wrap = FALSE;

    if (!isHUDAttachment() && mDrawable->mDistanceWRTCamera < alpha_distance_factor[2])
    {
        for (S32 i = 0; i < mDrawable->getNumFaces(); i++)
        {
            LLFace* face = mDrawable->getFace(i);
            if (!face) continue;
            if (face->isInAlphaPool() &&
                !face->canRenderAsMask())
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
        //radius *= 1.f+mDrawable->mDistanceWRTCamera*alpha_distance_factor[1];
        //radius += mDrawable->mDistanceWRTCamera*alpha_distance_factor[0];
    }
    else if (shrink_wrap)
    {
        radius = mDrawable->getRadius() * 0.25f;
    }
    else
    {
        F32 szf = size_factor;
        radius = llmax(mDrawable->getRadius(), szf);
        //radius = llmax(radius, mDrawable->mDistanceWRTCamera * distance_factor[0]);
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

void LLVOVolume::onShift(const LLVector4a &shift_vector)
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

void LLVOVolume::markForUpdate(BOOL priority)
{ 
    if (debugLoggingEnabled("AnimatedObjectsLinkset"))
    {
        if (isAnimatedObject() && isRiggedMesh())
        {
            std::string vobj_name = llformat("Vol%p", this);
            F32 est_tris = getEstTrianglesMax();
            LL_DEBUGS("AnimatedObjectsLinkset") << vobj_name << " markForUpdate, tris " << est_tris << LL_ENDL; 
        }
    }

    if (mDrawable)
    {
        shrinkWrap();
    }

    LLViewerObject::markForUpdate(priority); 
    mVolumeChanged = TRUE; 
}

LLVector3 LLVOVolume::agentPositionToVolume(const LLVector3& pos) const
{
	LLVector3 ret = pos - getRenderPosition();
	ret = ret * ~getRenderRotation();
	if (!isVolumeGlobal())
	{
		LLVector3 objScale = getScale();
		LLVector3 invObjScale(1.f / objScale.mV[VX], 1.f / objScale.mV[VY], 1.f / objScale.mV[VZ]);
		ret.scaleVec(invObjScale);
	}
	
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
	if (!isVolumeGlobal())
	{
		LLVector3 objScale = getScale();
		ret.scaleVec(objScale);
	}

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


BOOL LLVOVolume::lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end, S32 face, BOOL pick_transparent, BOOL pick_rigged, BOOL pick_unselectable, S32 *face_hitp,
									  LLVector4a* intersection,LLVector2* tex_coord, LLVector4a* normal, LLVector4a* tangent)
	
{
	if (!mbCanSelect 
		|| mDrawable->isDead() 
		|| !gPipeline.hasRenderType(mDrawable->getRenderType()))
	{
		return FALSE;
	}

    if (!pick_unselectable)
    {
        if (!LLSelectMgr::instance().canSelectObject(this))
        {
            return FALSE;
        }
    }

	BOOL ret = FALSE;

	LLVolume* volume = getVolume();

	bool transform = true;

	if (mDrawable->isState(LLDrawable::RIGGED))
	{
		if ((pick_rigged) || (getAvatar() && (getAvatar()->isSelf()) && (LLFloater::isVisible(gFloaterTools))))
		{
            updateRiggedVolume(true, LLRiggedVolume::DO_NOT_UPDATE_FACES);
			volume = mRiggedVolume;
			transform = false;
		}
		else
		{ //cannot pick rigged attachments on other avatars or when not in build mode
			return FALSE;
		}
	}
	
	if (volume)
	{	
		LLVector4a local_start = start;
		LLVector4a local_end = end;
	
		if (transform)
		{
			LLVector3 v_start(start.getF32ptr());
			LLVector3 v_end(end.getF32ptr());
		
			v_start = agentPositionToVolume(v_start);
			v_end = agentPositionToVolume(v_end);

			local_start.load3(v_start.mV);
			local_end.load3(v_end.mV);
		}
		
		LLVector4a p;
		LLVector4a n;
		LLVector2 tc;
		LLVector4a tn;

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

		if (tangent != NULL)
		{
			tn = *tangent;
		}

		S32 face_hit = -1;

		S32 start_face, end_face;
		if (face == -1)
		{
			start_face = 0;
			end_face = volume->getNumVolumeFaces();
		}
		else
		{
			start_face = face;
			end_face = face+1;
		}
		pick_transparent |= isHiglightedOrBeacon();
		bool special_cursor = specialHoverCursor();
		for (S32 i = start_face; i < end_face; ++i)
		{
			if (!special_cursor && !pick_transparent && getTE(i) && getTE(i)->getColor().mV[3] == 0.f)
			{ //don't attempt to pick completely transparent faces unless
				//pick_transparent is true
				continue;
			}

            // This calculates the bounding box of the skinned mesh from scratch. It's actually quite expensive, but not nearly as expensive as building a full octree.
            // rebuild_face_octrees = false because an octree for this face will be built later only if needed for narrow phase picking.
            updateRiggedVolume(true, i, false);
			face_hit = volume->lineSegmentIntersect(local_start, local_end, i,
													&p, &tc, &n, &tn);
			
			if (face_hit >= 0 && mDrawable->getNumFaces() > face_hit)
			{
				LLFace* face = mDrawable->getFace(face_hit);				

				bool ignore_alpha = false;

				const LLTextureEntry* te = face->getTextureEntry();
				if (te)
				{
					LLMaterial* mat = te->getMaterialParams();
					if (mat)
					{
						U8 mode = mat->getDiffuseAlphaMode();

						if (mode == LLMaterial::DIFFUSE_ALPHA_MODE_EMISSIVE
							|| mode == LLMaterial::DIFFUSE_ALPHA_MODE_NONE
							|| (mode == LLMaterial::DIFFUSE_ALPHA_MODE_MASK && mat->getAlphaMaskCutoff() == 0))
						{
							ignore_alpha = true;
						}
					}
				}

                BOOL no_texture = !face->getTexture() || !face->getTexture()->hasGLTexture();
                BOOL mask       = no_texture ? FALSE : face->getTexture()->getMask(face->surfaceToTexture(tc, p, n));
				if (face &&
					(ignore_alpha || pick_transparent || no_texture || mask))
				{
					local_end = p;
					if (face_hitp != NULL)
					{
						*face_hitp = face_hit;
					}
					
					if (intersection != NULL)
					{
						if (transform)
						{
							LLVector3 v_p(p.getF32ptr());

							intersection->load3(volumePositionToAgent(v_p).mV);  // must map back to agent space
						}
						else
						{
							*intersection = p;
						}
					}

					if (normal != NULL)
					{
						if (transform)
						{
							LLVector3 v_n(n.getF32ptr());
							normal->load3(volumeDirectionToAgent(v_n).mV);
						}
						else
						{
							*normal = n;
						}
						(*normal).normalize3fast();
					}

					if (tangent != NULL)
					{
						if (transform)
						{
							LLVector3 v_tn(tn.getF32ptr());

							LLVector4a trans_tangent;
							trans_tangent.load3(volumeDirectionToAgent(v_tn).mV);

							LLVector4Logical mask;
							mask.clear();
							mask.setElement<3>();

							tangent->setSelectWithMask(mask, tn, trans_tangent);
						}
						else
						{
							*tangent = tn;
						}
						(*tangent).normalize3fast();
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

bool LLVOVolume::treatAsRigged()
{
	return isSelected() &&
        (isAttachment() || isAnimatedObject()) &&
        mDrawable.notNull() &&
        mDrawable->isState(LLDrawable::RIGGED);
}

LLRiggedVolume* LLVOVolume::getRiggedVolume()
{
	return mRiggedVolume;
}

void LLVOVolume::clearRiggedVolume()
{
	if (mRiggedVolume.notNull())
	{
		mRiggedVolume = NULL;
		updateRelativeXform();
	}
}

void LLVOVolume::updateRiggedVolume(bool force_treat_as_rigged, LLRiggedVolume::FaceIndex face_index, bool rebuild_face_octrees)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;
	//Update mRiggedVolume to match current animation frame of avatar. 
	//Also update position/size in octree.  

    if ((!force_treat_as_rigged) && (!treatAsRigged()))
	{
		clearRiggedVolume();
		
		return;
	}

	LLVolume* volume = getVolume();
	const LLMeshSkinInfo* skin = getSkinInfo();
	if (!skin)
	{
		clearRiggedVolume();
		return;
	}

	LLVOAvatar* avatar = getAvatar();
	if (!avatar)
	{
		clearRiggedVolume();
		return;
	}

	if (!mRiggedVolume)
	{
		LLVolumeParams p;
		mRiggedVolume = new LLRiggedVolume(p);
		updateRelativeXform();
	}

    mRiggedVolume->update(skin, avatar, volume, face_index, rebuild_face_octrees);
}

void LLRiggedVolume::update(
    const LLMeshSkinInfo* skin,
    LLVOAvatar* avatar,
    const LLVolume* volume,
    FaceIndex face_index,
    bool rebuild_face_octrees)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;
	bool copy = false;
	if (volume->getNumVolumeFaces() != getNumVolumeFaces())
	{ 
		copy = true;
	}

	for (S32 i = 0; i < volume->getNumVolumeFaces() && !copy; ++i)
	{
		const LLVolumeFace& src_face = volume->getVolumeFace(i);
		const LLVolumeFace& dst_face = getVolumeFace(i);

		if (src_face.mNumIndices != dst_face.mNumIndices ||
			src_face.mNumVertices != dst_face.mNumVertices)
		{
			copy = true;
		}
	}

	if (copy)
	{
		copyVolumeFaces(volume);
	}
    else
    {
        bool is_paused = avatar && avatar->areAnimationsPaused();
		if (is_paused)
		{
            S32 frames_paused = LLFrameTimer::getFrameCount() - avatar->getMotionController().getPausedFrame();
            if (frames_paused > 1)
            {
                return;
            }
		}
    }


	//build matrix palette
	static const size_t kMaxJoints = LL_MAX_JOINTS_PER_MESH_OBJECT;

	LLMatrix4a mat[kMaxJoints];
	U32 maxJoints = LLSkinningUtil::getMeshJointCount(skin);
    LLSkinningUtil::initSkinningMatrixPalette(mat, maxJoints, skin, avatar);
    const LLMatrix4a bind_shape_matrix = skin->mBindShapeMatrix;

    S32 rigged_vert_count = 0;
    S32 rigged_face_count = 0;
    LLVector4a box_min, box_max;
    S32 face_begin;
    S32 face_end;
    if (face_index == DO_NOT_UPDATE_FACES)
    {
        face_begin = 0;
        face_end = 0;
    }
    else if (face_index == UPDATE_ALL_FACES)
    {
        face_begin = 0;
        face_end = volume->getNumVolumeFaces();
    }
    else
    {
        face_begin = face_index;
        face_end = face_begin + 1;
    }
    for (S32 i = face_begin; i < face_end; ++i)
	{
		const LLVolumeFace& vol_face = volume->getVolumeFace(i);
		
		LLVolumeFace& dst_face = mVolumeFaces[i];
		
		LLVector4a* weight = vol_face.mWeights;

		if ( weight )
		{
            LLSkinningUtil::checkSkinWeights(weight, dst_face.mNumVertices, skin);

			LLVector4a* pos = dst_face.mPositions;

			if (pos && dst_face.mExtents)
			{
                U32 max_joints = LLSkinningUtil::getMaxJointCount();
                rigged_vert_count += dst_face.mNumVertices;
                rigged_face_count++;

            #if USE_SEPARATE_JOINT_INDICES_AND_WEIGHTS
                if (vol_face.mJointIndices) // fast path with preconditioned joint indices
                {
                    LLMatrix4a src[4];
                    U8* joint_indices_cursor = vol_face.mJointIndices;
                    LLVector4a* just_weights = vol_face.mJustWeights;
                    for (U32 j = 0; j < dst_face.mNumVertices; ++j)
				    {
					    LLMatrix4a final_mat;
                        F32* w = just_weights[j].getF32ptr();
                        LLSkinningUtil::getPerVertexSkinMatrixWithIndices(w, joint_indices_cursor, mat, final_mat, src);
                        joint_indices_cursor += 4;

					    LLVector4a& v = vol_face.mPositions[j];
					    LLVector4a t;
					    LLVector4a dst;
					    bind_shape_matrix.affineTransform(v, t);
					    final_mat.affineTransform(t, dst);
					    pos[j] = dst;
				    }
                }
                else
            #endif
                {
				    for (U32 j = 0; j < dst_face.mNumVertices; ++j)
				    {
					    LLMatrix4a final_mat;
                        LLSkinningUtil::getPerVertexSkinMatrix(weight[j].getF32ptr(), mat, false, final_mat, max_joints);
				
					    LLVector4a& v = vol_face.mPositions[j];
					    LLVector4a t;
					    LLVector4a dst;
					    bind_shape_matrix.affineTransform(v, t);
					    final_mat.affineTransform(t, dst);
					    pos[j] = dst;
				    }
                }

				//update bounding box
				// VFExtents change
				LLVector4a& min = dst_face.mExtents[0];
				LLVector4a& max = dst_face.mExtents[1];

				min = pos[0];
				max = pos[1];
                if (i==0)
                {
                    box_min = min;
                    box_max = max;
                }

				for (U32 j = 1; j < dst_face.mNumVertices; ++j)
				{
					min.setMin(min, pos[j]);
					max.setMax(max, pos[j]);
				}

                box_min.setMin(min,box_min);
                box_max.setMax(max,box_max);

				dst_face.mCenter->setAdd(dst_face.mExtents[0], dst_face.mExtents[1]);
				dst_face.mCenter->mul(0.5f);

			}

            if (rebuild_face_octrees)
			{
                dst_face.destroyOctree();
                dst_face.createOctree();
			}
		}
	}
    mExtraDebugText = llformat("rigged %d/%d - box (%f %f %f) (%f %f %f)",
                               rigged_face_count, rigged_vert_count,
                               box_min[0], box_min[1], box_min[2],
                               box_max[0], box_max[1], box_max[2]);
}

U32 LLVOVolume::getPartitionType() const
{
	if (isHUDAttachment())
	{
		return LLViewerRegion::PARTITION_HUD;
	}
	if (isAnimatedObject() && getControlAvatar())
	{
		return LLViewerRegion::PARTITION_CONTROL_AV;
	}
	if (isAttachment())
	{
		return LLViewerRegion::PARTITION_AVATAR;
	}

	return LLViewerRegion::PARTITION_VOLUME;
}

LLVolumePartition::LLVolumePartition(LLViewerRegion* regionp)
: LLSpatialPartition(LLVOVolume::VERTEX_DATA_MASK, TRUE, regionp),
LLVolumeGeometryManager()
{
	mLODPeriod = 32;
	mDepthMask = FALSE;
	mDrawableType = LLPipeline::RENDER_TYPE_VOLUME;
	mPartitionType = LLViewerRegion::PARTITION_VOLUME;
	mSlopRatio = 0.25f;
}

LLVolumeBridge::LLVolumeBridge(LLDrawable* drawablep, LLViewerRegion* regionp)
: LLSpatialBridge(drawablep, TRUE, LLVOVolume::VERTEX_DATA_MASK, regionp),
LLVolumeGeometryManager()
{
	mDepthMask = FALSE;
	mLODPeriod = 32;
	mDrawableType = LLPipeline::RENDER_TYPE_VOLUME;
	mPartitionType = LLViewerRegion::PARTITION_BRIDGE;
	
	mSlopRatio = 0.25f;
}

LLAvatarBridge::LLAvatarBridge(LLDrawable* drawablep, LLViewerRegion* regionp)
	: LLVolumeBridge(drawablep, regionp)
{
	mDrawableType = LLPipeline::RENDER_TYPE_AVATAR;
	mPartitionType = LLViewerRegion::PARTITION_AVATAR;
}

LLControlAVBridge::LLControlAVBridge(LLDrawable* drawablep, LLViewerRegion* regionp)
	: LLVolumeBridge(drawablep, regionp)
{
	mDrawableType = LLPipeline::RENDER_TYPE_CONTROL_AV;
	mPartitionType = LLViewerRegion::PARTITION_CONTROL_AV;
}

bool can_batch_texture(LLFace* facep)
{
	if (facep->getTextureEntry()->getBumpmap())
	{ //bump maps aren't worked into texture batching yet
		return false;
	}

	if (facep->getTextureEntry()->getMaterialParams().notNull())
	{ //materials don't work with texture batching yet
		return false;
	}

	if (facep->getTexture() && facep->getTexture()->getPrimaryFormat() == GL_ALPHA)
	{ //can't batch invisiprims
		return false;
	}

	if (facep->isState(LLFace::TEXTURE_ANIM) && facep->getVirtualSize() > MIN_TEX_ANIM_SIZE)
	{ //texture animation breaks batches
		return false;
	}
	
    if (facep->getTextureEntry()->getGLTFRenderMaterial() != nullptr)
    { // PBR materials break indexed texture batching
        return false;
    }

	return true;
}

const static U32 MAX_FACE_COUNT = 4096U;
int32_t LLVolumeGeometryManager::sInstanceCount = 0;
LLFace** LLVolumeGeometryManager::sFullbrightFaces[2] = { NULL };
LLFace** LLVolumeGeometryManager::sBumpFaces[2] = { NULL };
LLFace** LLVolumeGeometryManager::sSimpleFaces[2] = { NULL };
LLFace** LLVolumeGeometryManager::sNormFaces[2] = { NULL };
LLFace** LLVolumeGeometryManager::sSpecFaces[2] = { NULL };
LLFace** LLVolumeGeometryManager::sNormSpecFaces[2] = { NULL };
LLFace** LLVolumeGeometryManager::sAlphaFaces[2] = { NULL };

LLVolumeGeometryManager::LLVolumeGeometryManager()
	: LLGeometryManager()
{
	llassert(sInstanceCount >= 0);
	if (sInstanceCount == 0)
	{
		allocateFaces(MAX_FACE_COUNT);
	}

	++sInstanceCount;
}

LLVolumeGeometryManager::~LLVolumeGeometryManager()
{
	llassert(sInstanceCount > 0);
	--sInstanceCount;

	if (sInstanceCount <= 0)
	{
		freeFaces();
		sInstanceCount = 0;
	}
}

void LLVolumeGeometryManager::allocateFaces(U32 pMaxFaceCount)
{
    for (int i = 0; i < 2; ++i)
    {
        sFullbrightFaces[i] = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount * sizeof(LLFace*)));
        sBumpFaces[i] = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount * sizeof(LLFace*)));
        sSimpleFaces[i] = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount * sizeof(LLFace*)));
        sNormFaces[i] = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount * sizeof(LLFace*)));
        sSpecFaces[i] = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount * sizeof(LLFace*)));
        sNormSpecFaces[i] = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount * sizeof(LLFace*)));
        sAlphaFaces[i] = static_cast<LLFace**>(ll_aligned_malloc<64>(pMaxFaceCount * sizeof(LLFace*)));
    }
}

void LLVolumeGeometryManager::freeFaces()
{
    for (int i = 0; i < 2; ++i)
    {
        ll_aligned_free<64>(sFullbrightFaces[i]);
        ll_aligned_free<64>(sBumpFaces[i]);
        ll_aligned_free<64>(sSimpleFaces[i]);
        ll_aligned_free<64>(sNormFaces[i]);
        ll_aligned_free<64>(sSpecFaces[i]);
        ll_aligned_free<64>(sNormSpecFaces[i]);
        ll_aligned_free<64>(sAlphaFaces[i]);

        sFullbrightFaces[i] = NULL;
        sBumpFaces[i] = NULL;
        sSimpleFaces[i] = NULL;
        sNormFaces[i] = NULL;
        sSpecFaces[i] = NULL;
        sNormSpecFaces[i] = NULL;
        sAlphaFaces[i] = NULL;
    }
}

void LLVolumeGeometryManager::registerFace(LLSpatialGroup* group, LLFace* facep, U32 type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;
	if (   type == LLRenderPass::PASS_ALPHA 
		&& facep->getTextureEntry()->getMaterialParams().notNull() 
		&& !facep->getVertexBuffer()->hasDataType(LLVertexBuffer::TYPE_TANGENT)
		&& LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT) > 1)
	{
		LL_WARNS_ONCE("RenderMaterials") << "Oh no! No binormals for this alpha blended face!" << LL_ENDL;
	}

	bool selected = facep->getViewerObject()->isSelected();

	if (selected && LLSelectMgr::getInstance()->mHideSelectedObjects)
	{
		return;
	}

	LL_LABEL_VERTEX_BUFFER(facep->getVertexBuffer(), LLRenderPass::lookupPassName(type));

    U32 passType = type;

    bool rigged = facep->isState(LLFace::RIGGED);

    if (rigged)
    {
        // hacky, should probably clean up -- if this face is rigged, put it in "type + 1"
        // See LLRenderPass PASS_foo enum
        passType += 1;
    }
	//add face to drawmap
	LLSpatialGroup::drawmap_elem_t& draw_vec = group->mDrawMap[passType];

	S32 idx = draw_vec.size()-1;

	bool fullbright = (type == LLRenderPass::PASS_FULLBRIGHT) ||
		(type == LLRenderPass::PASS_INVISIBLE) ||
		(type == LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK) ||
		(type == LLRenderPass::PASS_ALPHA && facep->isState(LLFace::FULLBRIGHT)) ||
		(facep->getTextureEntry()->getFullbright());
	
	if (!fullbright && 
        type != LLRenderPass::PASS_GLOW && 
        !facep->getVertexBuffer()->hasDataType(LLVertexBuffer::TYPE_NORMAL))
	{
        llassert(false);
		LL_WARNS() << "Non fullbright face has no normals!" << LL_ENDL;
		return;
	}

	const LLMatrix4* tex_mat = NULL;
	if (facep->isState(LLFace::TEXTURE_ANIM) && facep->getVirtualSize() > MIN_TEX_ANIM_SIZE)
	{
		tex_mat = facep->mTextureMatrix;	
	}

	const LLMatrix4* model_mat = NULL;

	LLDrawable* drawable = facep->getDrawable();
	
    if (rigged)
    {
        // rigged meshes ignore their model matrix
        model_mat = nullptr;
    }
	else if (drawable->isState(LLDrawable::ANIMATED_CHILD))
	{
		model_mat = &drawable->getWorldMatrix();
	}
	else if (drawable->isActive())
	{
		model_mat = &drawable->getRenderMatrix();
	}
	else
	{
		model_mat = &(drawable->getRegion()->mRenderMatrix);
	}

	//drawable->getVObj()->setDebugText(llformat("%d", drawable->isState(LLDrawable::ANIMATED_CHILD)));

	U8 bump = (type == LLRenderPass::PASS_BUMP || type == LLRenderPass::PASS_POST_BUMP) ? facep->getTextureEntry()->getBumpmap() : 0;
	U8 shiny = facep->getTextureEntry()->getShiny();
	
	LLViewerTexture* tex = facep->getTexture();

	U8 index = facep->getTextureIndex();

    LLMaterial* mat = nullptr;
    
    LLUUID mat_id;

    auto* gltf_mat = (LLFetchedGLTFMaterial*) facep->getTextureEntry()->getGLTFRenderMaterial();
    if (gltf_mat != nullptr)
    {
        mat_id = gltf_mat->getHash(); // TODO: cache this hash
        if (!facep->hasMedia())
        { // no media texture, face texture will be unused
            tex = nullptr;
        }
    }
    else
    {
        mat = facep->getTextureEntry()->getMaterialParams().get();
        if (mat)
        {
            mat_id = facep->getTextureEntry()->getMaterialParams()->getHash();
        }
    }

	bool batchable = false;

	U32 shader_mask = 0xFFFFFFFF; //no shader

	if (mat)
	{
		if (type == LLRenderPass::PASS_ALPHA)
		{
			shader_mask = mat->getShaderMask(LLMaterial::DIFFUSE_ALPHA_MODE_BLEND);
		}
		else
		{
			shader_mask = mat->getShaderMask();
		}
	}

	if (index < FACE_DO_NOT_BATCH_TEXTURES && idx >= 0)
	{
		if (mat || gltf_mat || draw_vec[idx]->mMaterial)
		{ //can't batch textures when materials are present (yet)
			batchable = false;
		}
		else if (index < draw_vec[idx]->mTextureList.size())
		{
			if (draw_vec[idx]->mTextureList[index].isNull())
			{
				batchable = true;
				draw_vec[idx]->mTextureList[index] = tex;
			}
			else if (draw_vec[idx]->mTextureList[index] == tex)
			{ //this face's texture index can be used with this batch
				batchable = true;
			}
		}
		else
		{ //texture list can be expanded to fit this texture index
			batchable = true;
		}
	}

    LLDrawInfo* info = idx >= 0 ? draw_vec[idx] : nullptr;

	if (info && 
		info->mVertexBuffer == facep->getVertexBuffer() &&
		info->mEnd == facep->getGeomIndex()-1 &&
		(LLPipeline::sTextureBindTest || draw_vec[idx]->mTexture == tex || batchable) &&
#if LL_DARWIN
		info->mEnd - draw_vec[idx]->mStart + facep->getGeomCount() <= (U32) gGLManager.mGLMaxVertexRange &&
		info->mCount + facep->getIndicesCount() <= (U32) gGLManager.mGLMaxIndexRange &&
#endif
		info->mMaterialID == mat_id &&
		info->mFullbright == fullbright &&
		info->mBump == bump &&
		(!mat || (info->mShiny == shiny)) && // need to break batches when a material is shared, but legacy settings are different
		info->mTextureMatrix == tex_mat &&
		info->mModelMatrix == model_mat &&
		info->mShaderMask == shader_mask &&
        info->mAvatar == facep->mAvatar &&
        info->getSkinHash() == facep->getSkinHash())
	{
		info->mCount += facep->getIndicesCount();
		info->mEnd += facep->getGeomCount();

		if (index < FACE_DO_NOT_BATCH_TEXTURES && index >= info->mTextureList.size())
		{
			info->mTextureList.resize(index+1);
			info->mTextureList[index] = tex;
		}
		info->validate();
	}
	else
	{
		U32 start = facep->getGeomIndex();
		U32 end = start + facep->getGeomCount()-1;
		U32 offset = facep->getIndicesStart();
		U32 count = facep->getIndicesCount();
		LLPointer<LLDrawInfo> draw_info = new LLDrawInfo(start,end,count,offset, tex,
			facep->getVertexBuffer(), fullbright, bump);

        info = draw_info;

		draw_vec.push_back(draw_info);
		draw_info->mTextureMatrix = tex_mat;
		draw_info->mModelMatrix = model_mat;
		
		draw_info->mBump  = bump;
		draw_info->mShiny = shiny;

		static const float alpha[4] =
		{
			0.00f,
			0.25f,
			0.5f,
			0.75f
		};
		float spec = alpha[shiny & TEM_SHINY_MASK];
		LLVector4 specColor(spec, spec, spec, spec);
		draw_info->mSpecColor = specColor;
		draw_info->mEnvIntensity = spec;
		draw_info->mSpecularMap = NULL;
		draw_info->mMaterial = mat;
        draw_info->mGLTFMaterial = gltf_mat;
		draw_info->mShaderMask = shader_mask;
        draw_info->mAvatar = facep->mAvatar;
        draw_info->mSkinInfo = facep->mSkinInfo;

        if (gltf_mat)
        {
            // nothing to do, render pools will reference the GLTF material
        }
        else if (mat)
		{
			draw_info->mMaterialID = mat_id;

			// We have a material.  Update our draw info accordingly.
				
			if (!mat->getSpecularID().isNull())
			{
				LLVector4 specColor;
				specColor.mV[0] = mat->getSpecularLightColor().mV[0] * (1.f / 255.f);
				specColor.mV[1] = mat->getSpecularLightColor().mV[1] * (1.f / 255.f);
				specColor.mV[2] = mat->getSpecularLightColor().mV[2] * (1.f / 255.f);
				specColor.mV[3] = mat->getSpecularLightExponent() * (1.f / 255.f);
				draw_info->mSpecColor = specColor;
				draw_info->mEnvIntensity = mat->getEnvironmentIntensity() * (1.f / 255.f);
				draw_info->mSpecularMap = facep->getViewerObject()->getTESpecularMap(facep->getTEOffset());
			}

			draw_info->mAlphaMaskCutoff = mat->getAlphaMaskCutoff() * (1.f / 255.f);
			draw_info->mDiffuseAlphaMode = mat->getDiffuseAlphaMode();
			draw_info->mNormalMap = facep->getViewerObject()->getTENormalMap(facep->getTEOffset());
		}
		else 
		{
			if (type == LLRenderPass::PASS_GRASS)
			{
				draw_info->mAlphaMaskCutoff = 0.5f;
			}
			else
			{
				draw_info->mAlphaMaskCutoff = 0.33f;
			}
		}
		
		if (type == LLRenderPass::PASS_ALPHA)
		{ //for alpha sorting
			facep->setDrawInfo(draw_info);
		}

		if (index < FACE_DO_NOT_BATCH_TEXTURES)
		{ //initialize texture list for texture batching
			draw_info->mTextureList.resize(index+1);
			draw_info->mTextureList[index] = tex;
		}
		draw_info->validate();
	}

    llassert(type != LLPipeline::RENDER_TYPE_PASS_GLTF_PBR || info->mGLTFMaterial != nullptr);
    llassert(type != LLPipeline::RENDER_TYPE_PASS_GLTF_PBR_RIGGED || info->mGLTFMaterial != nullptr);
    llassert(type != LLPipeline::RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK || info->mGLTFMaterial != nullptr);
    llassert(type != LLPipeline::RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK_RIGGED || info->mGLTFMaterial != nullptr);

    llassert(type != LLRenderPass::PASS_NORMSPEC || info->mNormalMap.notNull());
}

void LLVolumeGeometryManager::getGeometry(LLSpatialGroup* group)
{

}

void handleRenderAutoMuteByteLimitChanged(const LLSD& new_value)
{
	static LLCachedControl<U32> render_auto_mute_byte_limit(gSavedSettings, "RenderAutoMuteByteLimit", 0U);

	if (0 != render_auto_mute_byte_limit)
	{
		//for unload
		LLSculptIDSize::container_BY_SIZE_view::iterator
			itL = LLSculptIDSize::instance().getSizeInfo().get<LLSculptIDSize::tag_BY_SIZE>().lower_bound(render_auto_mute_byte_limit),
			itU = LLSculptIDSize::instance().getSizeInfo().get<LLSculptIDSize::tag_BY_SIZE>().end();

		for (; itL != itU; ++itL)
		{
			const LLSculptIDSize::Info &nfo = *itL;
			LLVOVolume *pVVol = nfo.getPtrLLDrawable()->getVOVolume();
			if (pVVol
				&& !pVVol->isDead()
				&& pVVol->isAttachment()
				&& !pVVol->getAvatar()->isSelf()
				&& LLVOVolume::NO_LOD != pVVol->getLOD()
				)
			{
				//postponed
				pVVol->markForUnload();
				LLSculptIDSize::instance().addToUnloaded(nfo.getSculptId());
			}
		}

		//for load if it was unload
		itL = LLSculptIDSize::instance().getSizeInfo().get<LLSculptIDSize::tag_BY_SIZE>().begin();
		itU = LLSculptIDSize::instance().getSizeInfo().get<LLSculptIDSize::tag_BY_SIZE>().upper_bound(render_auto_mute_byte_limit);

		for (; itL != itU; ++itL)
		{
			const LLSculptIDSize::Info &nfo = *itL;
			LLVOVolume *pVVol = nfo.getPtrLLDrawable()->getVOVolume();
			if (pVVol
				&& !pVVol->isDead()
				&& pVVol->isAttachment()
				&& !pVVol->getAvatar()->isSelf()
				&& LLVOVolume::NO_LOD == pVVol->getLOD()
				)
			{
				LLSculptIDSize::instance().remFromUnloaded(nfo.getSculptId());
				pVVol->updateLOD();
				pVVol->markForUpdate(TRUE);
			}
		}
	}
	else
	{
		LLSculptIDSize::instance().clearUnloaded();

		LLSculptIDSize::container_BY_SIZE_view::iterator
			itL = LLSculptIDSize::instance().getSizeInfo().get<LLSculptIDSize::tag_BY_SIZE>().begin(),
			itU = LLSculptIDSize::instance().getSizeInfo().get<LLSculptIDSize::tag_BY_SIZE>().end();

		for (; itL != itU; ++itL)
		{
			const LLSculptIDSize::Info &nfo = *itL;
			LLVOVolume *pVVol = nfo.getPtrLLDrawable()->getVOVolume();
			if (pVVol
				&& !pVVol->isDead()
				&& pVVol->isAttachment()
				&& !pVVol->getAvatar()->isSelf()
				&& LLVOVolume::NO_LOD == pVVol->getLOD()
				) 
			{
				pVVol->updateLOD();
				pVVol->markForUpdate(TRUE);
			}
		}
	}
}

// add a face pointer to a list of face pointers without going over MAX_COUNT faces
template<typename T>
static inline void add_face(T*** list, U32* count, T* face)
{
    if (face->isState(LLFace::RIGGED))
    {
        if (count[1] < MAX_FACE_COUNT)
        {
            face->setDrawOrderIndex(count[1]);
            list[1][count[1]++] = face;
        }
    }
    else
    {
        if (count[0] < MAX_FACE_COUNT)
        {
            face->setDrawOrderIndex(count[0]);
            list[0][count[0]++] = face;
        }
    }
}

void LLVolumeGeometryManager::rebuildGeom(LLSpatialGroup* group)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;
    llassert(!gCubeSnapshot);

	if (group->changeLOD())
	{
		group->mLastUpdateDistance = group->mDistance;
	}

	group->mLastUpdateViewAngle = group->mViewAngle;

	if (!group->hasState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::ALPHA_DIRTY))
	{
		if (group->hasState(LLSpatialGroup::MESH_DIRTY) && !LLPipeline::sDelayVBUpdate)
		{
			rebuildMesh(group);
		}
		return;
	}

	group->mBuilt = 1.f;
	
	LLSpatialBridge* bridge = group->getSpatialPartition()->asBridge();
    LLViewerObject *vobj = NULL;
    LLVOVolume *vol_obj = NULL;

	if (bridge)
	{
        vobj = bridge->mDrawable->getVObj();
        vol_obj = dynamic_cast<LLVOVolume*>(vobj);
	}
    if (vol_obj)
    {
        vol_obj->updateVisualComplexity();
    }

	group->mGeometryBytes = 0;
	group->mSurfaceArea = 0;
	
	//cache object box size since it might be used for determining visibility
	const LLVector4a* bounds = group->getObjectBounds();
	group->mObjectBoxSize = bounds[1].getLength3().getF32();

	group->clearDrawMap();

    U32 fullbright_count[2] = { 0 };
	U32 bump_count[2] = { 0 };
	U32 simple_count[2] = { 0 };
	U32 alpha_count[2] = { 0 };
	U32 norm_count[2] = { 0 };
	U32 spec_count[2] = { 0 };
	U32 normspec_count[2] = { 0 };

	static LLCachedControl<S32> max_vbo_size(gSavedSettings, "RenderMaxVBOSize", 512);
	static LLCachedControl<S32> max_node_size(gSavedSettings, "RenderMaxNodeSize", 65536);
	U32 max_vertices = (max_vbo_size * 1024)/LLVertexBuffer::calcVertexSize(group->getSpatialPartition()->mVertexDataMask);
	U32 max_total = (max_node_size * 1024) / LLVertexBuffer::calcVertexSize(group->getSpatialPartition()->mVertexDataMask);
	max_vertices = llmin(max_vertices, (U32) 65535);

	U32 cur_total = 0;

	bool emissive = false;

	//Determine if we've received skininfo that contains an
	//alternate bind matrix - if it does then apply the translational component
	//to the joints of the avatar.
#if 0
	bool pelvisGotSet = false;
#endif

	{
        LL_PROFILE_ZONE_NAMED("rebuildGeom - face list");

		//get all the faces into a list
		for (LLSpatialGroup::element_iter drawable_iter = group->getDataBegin(); 
             drawable_iter != group->getDataEnd(); ++drawable_iter)
		{
			LLDrawable* drawablep = (LLDrawable*)(*drawable_iter)->getDrawable();
		
			if (!drawablep || drawablep->isDead() || drawablep->isState(LLDrawable::FORCE_INVISIBLE) )
			{
				continue;
			}
	
			LLVOVolume* vobj = drawablep->getVOVolume();
            
			if (!vobj)
			{
				continue;
			}

            // apply any pending material overrides
            gGLTFMaterialList.applyQueuedOverrides(vobj);

            std::string vobj_name = llformat("Vol%p", vobj);

            bool is_mesh = vobj->isMesh();
			if (is_mesh &&
				((vobj->getVolume() && !vobj->getVolume()->isMeshAssetLoaded()) || !gMeshRepo.meshRezEnabled()))
			{
				continue;
			}

			LLVolume* volume = vobj->getVolume();
			if (volume)
			{
				const LLVector3& scale = vobj->getScale();
				group->mSurfaceArea += volume->getSurfaceArea() * llmax(llmax(scale.mV[0], scale.mV[1]), scale.mV[2]);
			}

            
            F32 est_tris = vobj->getEstTrianglesMax();

            vobj->updateControlAvatar();
            
            LL_DEBUGS("AnimatedObjectsLinkset") << vobj_name << " rebuilding, isAttachment: " << (U32) vobj->isAttachment()
                                                << " is_mesh " << is_mesh
                                                << " est_tris " << est_tris
                                                << " is_animated " << vobj->isAnimatedObject()
                                                << " can_animate " << vobj->canBeAnimatedObject() 
                                                << " cav " << vobj->getControlAvatar() 
                                                << " lod " << vobj->getLOD()
                                                << " drawable rigged " << (drawablep->isState(LLDrawable::RIGGED))
                                                << " drawable state " << drawablep->getState()
                                                << " playing " << (U32) (vobj->getControlAvatar() ? vobj->getControlAvatar()->mPlaying : false)
                                                << " frame " << LLFrameTimer::getFrameCount()
                                                << LL_ENDL;

			llassert_always(vobj);
			vobj->updateTextureVirtualSize(true);
			vobj->preRebuild();

			drawablep->clearState(LLDrawable::HAS_ALPHA);

            LLVOAvatar* avatar = nullptr;
            const LLMeshSkinInfo* skinInfo = nullptr;
            if (is_mesh)
            {
                skinInfo = vobj->getSkinInfo();
            }

            if (skinInfo)
            {
                if (vobj->isAnimatedObject())
                {
                    avatar = vobj->getControlAvatar();
                }
                else
                {
                    avatar = vobj->getAvatar();
                }
            }

            if (avatar != nullptr)
            {
                avatar->addAttachmentOverridesForObject(vobj, NULL, false);
            }

            // Standard rigged mesh attachments: 
			bool rigged = !vobj->isAnimatedObject() && skinInfo && vobj->isAttachment();
            // Animated objects. Have to check for isRiggedMesh() to
            // exclude static objects in animated object linksets.
			rigged = rigged || (vobj->isAnimatedObject() && vobj->isRiggedMesh() &&
                vobj->getControlAvatar() && vobj->getControlAvatar()->mPlaying);

			bool bake_sunlight = LLPipeline::sBakeSunlight && drawablep->isStatic();
			bool any_rigged_face = false;

			//for each face
			for (S32 i = 0; i < drawablep->getNumFaces(); i++)
			{
				LLFace* facep = drawablep->getFace(i);
				if (!facep)
				{
					continue;
				}

                // HACK -- brute force this check every time a drawable gets rebuilt
                vobj->updateTEMaterialTextures(i);
#if 0
#if LL_RELEASE_WITH_DEBUG_INFO
                const LLUUID pbr_id( "49c88210-7238-2a6b-70ac-92d4f35963cf" );
                const LLUUID obj_id( vobj->getID() );
                bool is_pbr = (obj_id == pbr_id);
#else
                bool is_pbr = false;
#endif
#else
                LLGLTFMaterial *gltf_mat = facep->getTextureEntry()->getGLTFRenderMaterial();
                bool is_pbr = gltf_mat != nullptr;
#endif

				//ALWAYS null out vertex buffer on rebuild -- if the face lands in a render
				// batch, it will recover its vertex buffer reference from the spatial group
				facep->setVertexBuffer(NULL);
			
				//sum up face verts and indices
				drawablep->updateFaceSize(i);

                if (rigged)
                {
                    if (!facep->isState(LLFace::RIGGED))
                    { //completely reset vertex buffer
                        facep->clearVertexBuffer();
                    }

                    facep->setState(LLFace::RIGGED);
                    facep->mSkinInfo = (LLMeshSkinInfo*) skinInfo; // TODO -- fix ugly de-consting here
                    facep->mAvatar = avatar;
                    any_rigged_face = true;
                }
                else
                {
                    if (facep->isState(LLFace::RIGGED))
                    { 
                        //face is not rigged but used to be, remove from rigged face pool
                        LLDrawPoolAvatar* pool = (LLDrawPoolAvatar*)facep->getPool();
                        if (pool)
                        {
                            pool->removeFace(facep);
                        }
                        facep->clearState(LLFace::RIGGED);
                        facep->mAvatar = NULL;
                        facep->mSkinInfo = NULL;
                    }
                }

				if (cur_total > max_total || facep->getIndicesCount() <= 0 || facep->getGeomCount() <= 0)
				{
					facep->clearVertexBuffer();
					continue;
				}

				if (facep->hasGeometry() &&
                    (rigged ||  // <-- HACK FIXME -- getPixelArea might be incorrect for rigged objects
                        facep->getPixelArea() > FORCE_CULL_AREA)) // <-- don't render tiny faces
				{
                    cur_total += facep->getGeomCount();

					const LLTextureEntry* te = facep->getTextureEntry();
					LLViewerTexture* tex = facep->getTexture();

					if (te->getGlow() >= 1.f/255.f)
					{
						emissive = true;
					}

					if (facep->isState(LLFace::TEXTURE_ANIM))
					{
						if (!vobj->mTexAnimMode)
						{
							facep->clearState(LLFace::TEXTURE_ANIM);
						}
					}

					BOOL force_simple = (facep->getPixelArea() < FORCE_SIMPLE_RENDER_AREA);
					U32 type = gPipeline.getPoolTypeFromTE(te, tex);
                    if (is_pbr && gltf_mat && gltf_mat->mAlphaMode != LLGLTFMaterial::ALPHA_MODE_BLEND)
                    {
                        type = LLDrawPool::POOL_GLTF_PBR;
                    }
                    else
					if (type != LLDrawPool::POOL_ALPHA && force_simple)
					{
						type = LLDrawPool::POOL_SIMPLE;
					}
					facep->setPoolType(type);

					if (vobj->isHUDAttachment() && !is_pbr)
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
								LLFace * facep = drawablep->getFace(face);
								if (facep)
								{
									facep->setState(LLFace::TEXTURE_ANIM);
								}
							}
						}
						else if (vobj->mTextureAnimp->mFace < vobj->getNumTEs())
						{
							LLFace * facep = drawablep->getFace(vobj->mTextureAnimp->mFace);
							if (facep)
							{
								facep->setState(LLFace::TEXTURE_ANIM);
							}
						}
					}

					if (type == LLDrawPool::POOL_ALPHA)
					{
						if (facep->canRenderAsMask())
						{ //can be treated as alpha mask
                            add_face(sSimpleFaces, simple_count, facep);
						}
						else
						{
                            if (te->getColor().mV[3] > 0.f || te->getGlow() > 0.f)
                            { //only treat as alpha in the pipeline if < 100% transparent
                                drawablep->setState(LLDrawable::HAS_ALPHA);
                                add_face(sAlphaFaces, alpha_count, facep);
                            }
                            else if (LLDrawPoolAlpha::sShowDebugAlpha)
                            {
                                add_face(sAlphaFaces, alpha_count, facep);
                            }
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
                            LLGLTFMaterial* gltf_mat = te->getGLTFRenderMaterial();

							if (LLPipeline::sRenderDeferred && 
                                (gltf_mat != nullptr || (te->getMaterialParams().notNull()  && !te->getMaterialID().isNull())))
							{
                                if (gltf_mat != nullptr)
                                {
                                    // all gltf materials have all vertex attributes for now
                                    add_face(sNormSpecFaces, normspec_count, facep);
                                }
                                else
                                {
                                    LLMaterial* mat = te->getMaterialParams().get();
                                    if (mat->getNormalID().notNull())
                                    {
                                        if (mat->getSpecularID().notNull())
                                        { //has normal and specular maps (needs texcoord1, texcoord2, and tangent)
                                            add_face(sNormSpecFaces, normspec_count, facep);
                                        }
                                        else
                                        { //has normal map (needs texcoord1 and tangent)
                                            add_face(sNormFaces, norm_count, facep);
                                        }
                                    }
                                    else if (mat->getSpecularID().notNull())
                                    { //has specular map but no normal map, needs texcoord2
                                        add_face(sSpecFaces, spec_count, facep);
                                    }
                                    else
                                    { //has neither specular map nor normal map, only needs texcoord0
                                        add_face(sSimpleFaces, simple_count, facep);
                                    }
                                }
							}
							else if (te->getBumpmap())
							{ //needs normal + tangent
                                add_face(sBumpFaces, bump_count, facep);
							}
							else if (te->getShiny() || !te->getFullbright())
							{ //needs normal
                                add_face(sSimpleFaces, simple_count, facep);
							}
							else 
							{ //doesn't need normal
								facep->setState(LLFace::FULLBRIGHT);
                                add_face(sFullbrightFaces, fullbright_count, facep);
							}
						}
						else
						{
							if (te->getBumpmap() && LLPipeline::sRenderBump)
							{ //needs normal + tangent
                                add_face(sBumpFaces, bump_count, facep);
							}
							else if ((te->getShiny() && LLPipeline::sRenderBump) ||
								!(te->getFullbright() || bake_sunlight))
							{ //needs normal
                                add_face(sSimpleFaces, simple_count, facep);
							}
							else 
							{ //doesn't need normal
								facep->setState(LLFace::FULLBRIGHT);
                                add_face(sFullbrightFaces, fullbright_count, facep);
							}
						}
					}
				}
				else
				{	//face has no renderable geometry
					facep->clearVertexBuffer();
				}		
			}
			
			if (any_rigged_face)
			{
				if (!drawablep->isState(LLDrawable::RIGGED))
				{
					drawablep->setState(LLDrawable::RIGGED);
                    LLDrawable* root = drawablep->getRoot();
                    if (root != drawablep)
                    {
                        root->setState(LLDrawable::RIGGED_CHILD);
                    }

					//first time this is drawable is being marked as rigged,
					// do another LoD update to use avatar bounding box
					vobj->updateLOD();
				}
			}
			else
			{
				drawablep->clearState(LLDrawable::RIGGED);
                vobj->updateRiggedVolume(false);
			}
		}
	}

	//PROCESS NON-ALPHA FACES
	U32 simple_mask = LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR;
	U32 alpha_mask = simple_mask | 0x80000000; //hack to give alpha verts their own VBO
	U32 bump_mask = LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR;
	U32 fullbright_mask = LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR;

	U32 norm_mask = simple_mask | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TANGENT;
	U32 normspec_mask = norm_mask | LLVertexBuffer::MAP_TEXCOORD2;
	U32 spec_mask = simple_mask | LLVertexBuffer::MAP_TEXCOORD2;

	if (emissive)
	{ //emissive faces are present, include emissive byte to preserve batching
		simple_mask = simple_mask | LLVertexBuffer::MAP_EMISSIVE;
		alpha_mask = alpha_mask | LLVertexBuffer::MAP_EMISSIVE;
		bump_mask = bump_mask | LLVertexBuffer::MAP_EMISSIVE;
		fullbright_mask = fullbright_mask | LLVertexBuffer::MAP_EMISSIVE;
		norm_mask = norm_mask | LLVertexBuffer::MAP_EMISSIVE;
		normspec_mask = normspec_mask | LLVertexBuffer::MAP_EMISSIVE;
		spec_mask = spec_mask | LLVertexBuffer::MAP_EMISSIVE;
	}

	BOOL batch_textures = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT) > 1;

    // add extra vertex data for deferred rendering (not necessarily for batching textures)
	if (batch_textures)
	{
		bump_mask = bump_mask | LLVertexBuffer::MAP_TANGENT;
		simple_mask = simple_mask | LLVertexBuffer::MAP_TEXTURE_INDEX;
		alpha_mask = alpha_mask | LLVertexBuffer::MAP_TEXTURE_INDEX | LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2;
		fullbright_mask = fullbright_mask | LLVertexBuffer::MAP_TEXTURE_INDEX;
	}

	group->mGeometryBytes = 0;

	U32 geometryBytes = 0;

    // generate render batches for static geometry
    U32 extra_mask = LLVertexBuffer::MAP_TEXTURE_INDEX;
    BOOL alpha_sort = TRUE;
    BOOL rigged = FALSE;
    for (int i = 0; i < 2; ++i) //two sets, static and rigged)
    {
        geometryBytes += genDrawInfo(group, simple_mask | extra_mask, sSimpleFaces[i], simple_count[i], FALSE, batch_textures, rigged);
        geometryBytes += genDrawInfo(group, fullbright_mask | extra_mask, sFullbrightFaces[i], fullbright_count[i], FALSE, batch_textures, rigged);
        geometryBytes += genDrawInfo(group, alpha_mask | extra_mask, sAlphaFaces[i], alpha_count[i], alpha_sort, batch_textures, rigged);
        geometryBytes += genDrawInfo(group, bump_mask | extra_mask, sBumpFaces[i], bump_count[i], FALSE, FALSE, rigged);
        geometryBytes += genDrawInfo(group, norm_mask | extra_mask, sNormFaces[i], norm_count[i], FALSE, FALSE, rigged);
        geometryBytes += genDrawInfo(group, spec_mask | extra_mask, sSpecFaces[i], spec_count[i], FALSE, FALSE, rigged);
        geometryBytes += genDrawInfo(group, normspec_mask | extra_mask, sNormSpecFaces[i], normspec_count[i], FALSE, FALSE, rigged);

        // for rigged set, add weights and disable alpha sorting (rigged items use depth buffer)
        extra_mask |= LLVertexBuffer::MAP_WEIGHT4;
        rigged = TRUE;
    }

	group->mGeometryBytes = geometryBytes;

	if (!LLPipeline::sDelayVBUpdate)
	{
		//drawables have been rebuilt, clear rebuild status
		for (LLSpatialGroup::element_iter drawable_iter = group->getDataBegin(); drawable_iter != group->getDataEnd(); ++drawable_iter)
		{
			LLDrawable* drawablep = (LLDrawable*)(*drawable_iter)->getDrawable();
			if(drawablep)
			{
                drawablep->clearState(LLDrawable::REBUILD_ALL);
            }
        }
	}

	group->mLastUpdateTime = gFrameTimeSeconds;
	group->mBuilt = 1.f;
	group->clearState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::ALPHA_DIRTY);

	if (LLPipeline::sDelayVBUpdate)
	{
		group->setState(LLSpatialGroup::MESH_DIRTY | LLSpatialGroup::NEW_DRAWINFO);
	}

}

void LLVolumeGeometryManager::rebuildMesh(LLSpatialGroup* group)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;
	llassert(group);
	if (group && group->hasState(LLSpatialGroup::MESH_DIRTY) && !group->hasState(LLSpatialGroup::GEOM_DIRTY))
	{
		{
            LL_PROFILE_ZONE_NAMED("rebuildMesh - gen draw info");

            group->mBuilt = 1.f;
		
			const U32 MAX_BUFFER_COUNT = 4096;
			LLVertexBuffer* locked_buffer[MAX_BUFFER_COUNT];

			U32 buffer_count = 0;

			for (LLSpatialGroup::element_iter drawable_iter = group->getDataBegin(); drawable_iter != group->getDataEnd(); ++drawable_iter)
			{
				LLDrawable* drawablep = (LLDrawable*)(*drawable_iter)->getDrawable();

				if (drawablep && !drawablep->isDead() && drawablep->isState(LLDrawable::REBUILD_ALL))
				{
					LLVOVolume* vobj = drawablep->getVOVolume();
					
					if (!vobj) continue;
					
					if (debugLoggingEnabled("AnimatedObjectsLinkset"))
					{
						if (vobj->isAnimatedObject() && vobj->isRiggedMesh())
						{
							std::string vobj_name = llformat("Vol%p", vobj);
							F32 est_tris = vobj->getEstTrianglesMax();
							LL_DEBUGS("AnimatedObjectsLinkset") << vobj_name << " rebuildMesh, tris " << est_tris << LL_ENDL;
						}
					}
					if (vobj->isNoLOD()) continue;

					vobj->preRebuild();

					if (drawablep->isState(LLDrawable::ANIMATED_CHILD))
					{
						vobj->updateRelativeXform(true);
					}

					LLVolume* volume = vobj->getVolume();
					if (!volume) continue;
					for (S32 i = 0; i < drawablep->getNumFaces(); ++i)
					{
						LLFace* face = drawablep->getFace(i);
						if (face)
						{
							LLVertexBuffer* buff = face->getVertexBuffer();
							if (buff)
							{
								if (!face->getGeometryVolume(*volume, face->getTEOffset(), 
									vobj->getRelativeXform(), vobj->getRelativeXformInvTrans(), face->getGeomIndex()))
								{ //something's gone wrong with the vertex buffer accounting, rebuild this group 
									group->dirtyGeom();
									gPipeline.markRebuild(group, TRUE);
								}

                                buff->unmapBuffer();
							}
						}
					}

					if (drawablep->isState(LLDrawable::ANIMATED_CHILD))
					{
						vobj->updateRelativeXform();
					}

					drawablep->clearState(LLDrawable::REBUILD_ALL);
				}
			}

			{
                LL_PROFILE_ZONE_NAMED("rebuildMesh - flush");
				for (LLVertexBuffer** iter = locked_buffer, ** end_iter = locked_buffer+buffer_count; iter != end_iter; ++iter)
				{
					(*iter)->unmapBuffer();
				}

				// don't forget alpha
				if(group != NULL &&
				   !group->mVertexBuffer.isNull())
				{
					group->mVertexBuffer->unmapBuffer();
				}
			}
			
			group->clearState(LLSpatialGroup::MESH_DIRTY | LLSpatialGroup::NEW_DRAWINFO);
		}
	} 
}

struct CompareBatchBreaker
{
	bool operator()(const LLFace* const& lhs, const LLFace* const& rhs)
	{
		const LLTextureEntry* lte = lhs->getTextureEntry();
		const LLTextureEntry* rte = rhs->getTextureEntry();

        if (lte->getBumpmap() != rte->getBumpmap())
		{
			return lte->getBumpmap() < rte->getBumpmap();
		}
		else if (lte->getFullbright() != rte->getFullbright())
		{
			return lte->getFullbright() < rte->getFullbright();
		}
        else if (LLPipeline::sRenderDeferred && lte->getMaterialID() != rte->getMaterialID())
        {
            return lte->getMaterialID() < rte->getMaterialID();
        }
		else if (lte->getShiny() != rte->getShiny())
		{
			return lte->getShiny() < rte->getShiny();
		}
        else if (lhs->getTexture() != rhs->getTexture())
		{
			return lhs->getTexture() < rhs->getTexture();
		}
        else 
        {
            // all else being equal, maintain consistent draw order
            return lhs->getDrawOrderIndex() < rhs->getDrawOrderIndex();
        }
	}
};

struct CompareBatchBreakerRigged
{
    bool operator()(const LLFace* const& lhs, const LLFace* const& rhs)
    {
        if (lhs->mAvatar != rhs->mAvatar)
        {
            return lhs->mAvatar < rhs->mAvatar;
        }
        else if (lhs->mSkinInfo->mHash != rhs->mSkinInfo->mHash)
        {
            return lhs->mSkinInfo->mHash < rhs->mSkinInfo->mHash;
        }
        else
        {
            // "inherit" non-rigged behavior
            CompareBatchBreaker comp;
            return comp(lhs, rhs);
        }
    }
};

U32 LLVolumeGeometryManager::genDrawInfo(LLSpatialGroup* group, U32 mask, LLFace** faces, U32 face_count, BOOL distance_sort, BOOL batch_textures, BOOL rigged)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;

	U32 geometryBytes = 0;
	
	//calculate maximum number of vertices to store in a single buffer
	static LLCachedControl<S32> max_vbo_size(gSavedSettings, "RenderMaxVBOSize", 512);
	U32 max_vertices = (max_vbo_size * 1024)/LLVertexBuffer::calcVertexSize(group->getSpatialPartition()->mVertexDataMask);
	max_vertices = llmin(max_vertices, (U32) 65535);

	{
        LL_PROFILE_ZONE_NAMED("genDrawInfo - sort");
        
        if (rigged)
        {
            if (!distance_sort) // <--- alpha "sort" rigged faces by maintaining original draw order
            {
                //sort faces by things that break batches, including avatar and mesh id
                std::sort(faces, faces + face_count, CompareBatchBreakerRigged());
            }
        }
        else if (!distance_sort)
        {
            //sort faces by things that break batches, not including avatar and mesh id
            std::sort(faces, faces + face_count, CompareBatchBreaker());
        }
		else
		{
			//sort faces by distance
			std::sort(faces, faces+face_count, LLFace::CompareDistanceGreater());
		}
	}
				
	bool hud_group = group->isHUDGroup() ;
	LLFace** face_iter = faces;
	LLFace** end_faces = faces+face_count;
	
	LLSpatialGroup::buffer_map_t buffer_map;

	LLViewerTexture* last_tex = NULL;
	S32 buffer_index = 0;

	S32 texture_index_channels = 1;
	
	if (gGLManager.mGLSLVersionMajor > 1 || gGLManager.mGLSLVersionMinor >= 30)
	{
		texture_index_channels = LLGLSLShader::sIndexedTextureChannels-1; //always reserve one for shiny for now just for simplicity;
	}

	if (LLPipeline::sRenderDeferred && distance_sort)
	{
		texture_index_channels = gDeferredAlphaProgram.mFeatures.mIndexedTextureChannels;
	}
    
    if (distance_sort)
    {
        buffer_index = -1;
    }

	texture_index_channels = LLGLSLShader::sIndexedTextureChannels;

	bool flexi = false;

	while (face_iter != end_faces)
	{
		//pull off next face
		LLFace* facep = *face_iter;
		LLViewerTexture* tex = facep->getTexture();
        const LLTextureEntry* te = facep->getTextureEntry();
		LLMaterialPtr mat = te->getMaterialParams();
        LLMaterialID matId = te->getMaterialID();

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

		bool bake_sunlight = LLPipeline::sBakeSunlight && facep->getDrawable()->isStatic(); 

		U32 index_count = facep->getIndicesCount();
		U32 geom_count = facep->getGeomCount();

		flexi = flexi || facep->getViewerObject()->getVolume()->isUnique();

		//sum up vertices needed for this render batch
		LLFace** i = face_iter;
		++i;
		
		const U32 MAX_TEXTURE_COUNT = 32;
		LLViewerTexture* texture_list[MAX_TEXTURE_COUNT];
		
		U32 texture_count = 0;

		{
            LL_PROFILE_ZONE_NAMED("genDrawInfo - face size");
			if (batch_textures)
			{
				U8 cur_tex = 0;
				facep->setTextureIndex(cur_tex);
				if (texture_count < MAX_TEXTURE_COUNT)
				{
					texture_list[texture_count++] = tex;
				}

				if (can_batch_texture(facep))
				{ //populate texture_list with any textures that can be batched
				  //move i to the next unbatchable face
					while (i != end_faces)
					{
						facep = *i;
						
						if (!can_batch_texture(facep))
						{ //face is bump mapped or has an animated texture matrix -- can't 
							//batch more than 1 texture at a time
							facep->setTextureIndex(0);
							break;
						}

						if (facep->getTexture() != tex)
						{
							if (distance_sort)
							{ //textures might be out of order, see if texture exists in current batch
								bool found = false;
								for (U32 tex_idx = 0; tex_idx < texture_count; ++tex_idx)
								{
									if (facep->getTexture() == texture_list[tex_idx])
									{
										cur_tex = tex_idx;
										found = true;
										break;
									}
								}

								if (!found)
								{
									cur_tex = texture_count;
								}
							}
							else
							{
								cur_tex++;
							}

							if (cur_tex >= texture_index_channels)
							{ //cut batches when index channels are depleted
								break;
							}

							tex = facep->getTexture();

							if (texture_count < MAX_TEXTURE_COUNT)
							{
								texture_list[texture_count++] = tex;
							}
						}

						if (geom_count + facep->getGeomCount() > max_vertices)
						{ //cut batches on geom count too big
							break;
						}

						++i;

						flexi = flexi || facep->getViewerObject()->getVolume()->isUnique();

						index_count += facep->getIndicesCount();
						geom_count += facep->getGeomCount();

						facep->setTextureIndex(cur_tex);
					}
				}
				else
				{
					facep->setTextureIndex(0);
				}

				tex = texture_list[0];
			}
			else
			{
				while (i != end_faces && 
					(LLPipeline::sTextureBindTest || 
						(distance_sort || 
							((*i)->getTexture() == tex))))
				{
					facep = *i;
                    const LLTextureEntry* nextTe = facep->getTextureEntry();
                    if (nextTe->getMaterialID() != matId)
                    {
                        break;
                    }

					//face has no texture index
					facep->mDrawInfo = NULL;
					facep->setTextureIndex(FACE_DO_NOT_BATCH_TEXTURES);

					if (geom_count + facep->getGeomCount() > max_vertices)
					{ //cut batches on geom count too big
						break;
					}

					++i;
					index_count += facep->getIndicesCount();
					geom_count += facep->getGeomCount();

					flexi = flexi || facep->getViewerObject()->getVolume()->isUnique();
				}
			}
		}

		//create vertex buffer
		LLPointer<LLVertexBuffer> buffer;

		{
            LL_PROFILE_ZONE_NAMED("genDrawInfo - allocate");
			buffer = new LLVertexBuffer(mask);
			if(!buffer->allocateBuffer(geom_count, index_count))
			{
				LL_WARNS() << "Failed to allocate group Vertex Buffer to "
					<< geom_count << " vertices and "
					<< index_count << " indices" << LL_ENDL;
				buffer = NULL;
			}
		}

		if (buffer)
		{
			geometryBytes += buffer->getSize() + buffer->getIndicesSize();
			buffer_map[mask][*face_iter].push_back(buffer);
		}

		//add face geometry

		U32 indices_index = 0;
		U16 index_offset = 0;

		while (face_iter < i)
		{
			//update face indices for new buffer
			facep = *face_iter;
			if (buffer.isNull())
			{
				// Bulk allocation failed
				facep->setVertexBuffer(buffer);
				facep->setSize(0, 0); // mark as no geometry
				++face_iter;
				continue;
			}
			facep->setIndicesIndex(indices_index);
			facep->setGeomIndex(index_offset);
			facep->setVertexBuffer(buffer);	
			
			if (batch_textures && facep->getTextureIndex() == FACE_DO_NOT_BATCH_TEXTURES)
			{
				LL_ERRS() << "Invalid texture index." << LL_ENDL;
			}
			
			{
				//for debugging, set last time face was updated vs moved
				facep->updateRebuildFlags();

				if (!LLPipeline::sDelayVBUpdate)
				{ //copy face geometry into vertex buffer
					LLDrawable* drawablep = facep->getDrawable();
					LLVOVolume* vobj = drawablep->getVOVolume();
					LLVolume* volume = vobj->getVolume();

					if (drawablep->isState(LLDrawable::ANIMATED_CHILD))
					{
						vobj->updateRelativeXform(true);
					}

					U32 te_idx = facep->getTEOffset();

					if (!facep->getGeometryVolume(*volume, te_idx, 
						vobj->getRelativeXform(), vobj->getRelativeXformInvTrans(), index_offset,true))
					{
						LL_WARNS() << "Failed to get geometry for face!" << LL_ENDL;
					}

					if (drawablep->isState(LLDrawable::ANIMATED_CHILD))
					{
						vobj->updateRelativeXform(false);
					}
				}
			}

			index_offset += facep->getGeomCount();
			indices_index += facep->getIndicesCount();

			//append face to appropriate render batch

			BOOL force_simple = facep->getPixelArea() < FORCE_SIMPLE_RENDER_AREA;
			BOOL fullbright = facep->isState(LLFace::FULLBRIGHT);
			if ((mask & LLVertexBuffer::MAP_NORMAL) == 0)
			{ //paranoia check to make sure GL doesn't try to read non-existant normals
				fullbright = TRUE;
			}

            const LLTextureEntry* te = facep->getTextureEntry();
            LLGLTFMaterial* gltf_mat = te->getGLTFRenderMaterial();

			if (hud_group && gltf_mat == nullptr)
			{ //all hud attachments are fullbright
				fullbright = TRUE;
			}
			
			tex = facep->getTexture();

			BOOL is_alpha = (facep->getPoolType() == LLDrawPool::POOL_ALPHA) ? TRUE : FALSE;

            LLMaterial* mat = nullptr;
            bool can_be_shiny = false;

            // ignore traditional material if GLTF material is present
            if (gltf_mat == nullptr)
            {
                mat = te->getMaterialParams().get();

                can_be_shiny = true;
                if (mat)
                {
                    U8 mode = mat->getDiffuseAlphaMode();
                    can_be_shiny = mode == LLMaterial::DIFFUSE_ALPHA_MODE_NONE ||
                        mode == LLMaterial::DIFFUSE_ALPHA_MODE_EMISSIVE;
                }
            }

            F32 te_alpha = te->getColor().mV[3]; 
			bool use_legacy_bump = te->getBumpmap() && (te->getBumpmap() < 18) && (!mat || mat->getNormalID().isNull());
			bool opaque = te_alpha >= 0.999f;
            bool transparent = te_alpha < 0.999f;

            is_alpha = (is_alpha || transparent) ? TRUE : FALSE;

			if (gltf_mat || (mat && !hud_group))
			{
				bool material_pass = false;

                if (gltf_mat)
                { // all other parameters ignored if gltf material is present
                    if (gltf_mat->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_BLEND)
                    {
                        registerFace(group, facep, LLRenderPass::PASS_ALPHA);
                    }
                    else if (gltf_mat->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_MASK)
                    {
                        registerFace(group, facep, LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK);
                    }
                    else
                    {
                        registerFace(group, facep, LLRenderPass::PASS_GLTF_PBR);
                    }
                }
                else
				// do NOT use 'fullbright' for this logic or you risk sending
				// things without normals down the materials pipeline and will
				// render poorly if not crash NORSPEC-240,314
				//
				if (te->getFullbright())
				{
					if (mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_MASK)
					{
						if (opaque)
						{
							registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK);
						}
						else
						{
							registerFace(group, facep, LLRenderPass::PASS_ALPHA);
						}
					}
					else if (is_alpha)
					{
						registerFace(group, facep, LLRenderPass::PASS_ALPHA);
					}
					else
					{
						if (mat->getEnvironmentIntensity() > 0 || te->getShiny() > 0)
						{
							material_pass = true;
						}
						else
						{
                            if (opaque)
						    {
							    registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT);
                            }
                            else
                            {
                                registerFace(group, facep, LLRenderPass::PASS_ALPHA);
                            }
						}
					}
				}
				else if (transparent)
				{
					registerFace(group, facep, LLRenderPass::PASS_ALPHA);
				}
				else if (use_legacy_bump)
				{
					// we have a material AND legacy bump settings, but no normal map
					registerFace(group, facep, LLRenderPass::PASS_BUMP);
				}
				else
				{
					material_pass = true;
				}

				if (material_pass)
				{
					static const U32 pass[] = 
					{
						LLRenderPass::PASS_MATERIAL,
						LLRenderPass::PASS_ALPHA, //LLRenderPass::PASS_MATERIAL_ALPHA,
						LLRenderPass::PASS_MATERIAL_ALPHA_MASK,
						LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE,
						LLRenderPass::PASS_SPECMAP,
						LLRenderPass::PASS_ALPHA, //LLRenderPass::PASS_SPECMAP_BLEND,
						LLRenderPass::PASS_SPECMAP_MASK,
						LLRenderPass::PASS_SPECMAP_EMISSIVE,
						LLRenderPass::PASS_NORMMAP,
						LLRenderPass::PASS_ALPHA, //LLRenderPass::PASS_NORMMAP_BLEND,
						LLRenderPass::PASS_NORMMAP_MASK,
						LLRenderPass::PASS_NORMMAP_EMISSIVE,
						LLRenderPass::PASS_NORMSPEC,
						LLRenderPass::PASS_ALPHA, //LLRenderPass::PASS_NORMSPEC_BLEND,
						LLRenderPass::PASS_NORMSPEC_MASK,
						LLRenderPass::PASS_NORMSPEC_EMISSIVE,
					};

					U32 mask = mat->getShaderMask();

					llassert(mask < sizeof(pass)/sizeof(U32));

					mask = llmin(mask, (U32)(sizeof(pass)/sizeof(U32)-1));

					registerFace(group, facep, pass[mask]);
				}
			}
			else if (mat)
			{
				U8 mode = mat->getDiffuseAlphaMode();

                is_alpha = (is_alpha || (mode == LLMaterial::DIFFUSE_ALPHA_MODE_BLEND));

				if (is_alpha)
				{
					mode = LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
				}

				if (mode == LLMaterial::DIFFUSE_ALPHA_MODE_MASK)
				{
					registerFace(group, facep, fullbright ? LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK : LLRenderPass::PASS_ALPHA_MASK);
				}
				else if (is_alpha )
				{
					registerFace(group, facep, LLRenderPass::PASS_ALPHA);
				}
				else if (gPipeline.shadersLoaded()
					&& LLPipeline::sRenderBump 
					&& te->getShiny() 
					&& can_be_shiny)
				{
					registerFace(group, facep, fullbright ? LLRenderPass::PASS_FULLBRIGHT_SHINY : LLRenderPass::PASS_SHINY);
				}
				else
				{
					registerFace(group, facep, fullbright ? LLRenderPass::PASS_FULLBRIGHT : LLRenderPass::PASS_SIMPLE);
				}
			}
			else if (is_alpha)
			{
				// can we safely treat this as an alpha mask?
				if (facep->getFaceColor().mV[3] <= 0.f)
				{ //100% transparent, don't render unless we're highlighting transparent
					registerFace(group, facep, LLRenderPass::PASS_ALPHA_INVISIBLE);
				}
				else if (facep->canRenderAsMask() && !hud_group)
				{
					if (te->getFullbright() || LLPipeline::sNoAlpha)
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
			}
			else if (gPipeline.shadersLoaded()
				&& LLPipeline::sRenderBump 
				&& te->getShiny() 
				&& can_be_shiny)
			{ //shiny
				if (tex->getPrimaryFormat() == GL_ALPHA)
				{ //invisiprim+shiny
					registerFace(group, facep, LLRenderPass::PASS_INVISI_SHINY);
					registerFace(group, facep, LLRenderPass::PASS_INVISIBLE);
				}
				else if (!hud_group)
				{ //deferred rendering
					if (te->getFullbright())
					{ //register in post deferred fullbright shiny pass
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_SHINY);
						if (te->getBumpmap())
						{ //register in post deferred bump pass
							registerFace(group, facep, LLRenderPass::PASS_POST_BUMP);
						}
					}
					else if (use_legacy_bump)
					{ //register in deferred bump pass
						registerFace(group, facep, LLRenderPass::PASS_BUMP);
					}
					else
					{ //register in deferred simple pass (deferred simple includes shiny)
						llassert(mask & LLVertexBuffer::MAP_NORMAL);
						registerFace(group, facep, LLRenderPass::PASS_SIMPLE);
					}
				}
				else if (fullbright)
				{	//not deferred, register in standard fullbright shiny pass					
					registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_SHINY);
				}
				else
				{ //not deferred or fullbright, register in standard shiny pass
					registerFace(group, facep, LLRenderPass::PASS_SHINY);
				}
			}
			else
			{ //not alpha and not shiny
				if (!is_alpha && tex->getPrimaryFormat() == GL_ALPHA)
				{ //invisiprim
					registerFace(group, facep, LLRenderPass::PASS_INVISIBLE);
				}
				else if (fullbright || bake_sunlight)
				{ //fullbright
					if (mat && mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_MASK)
					{
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK);
					}
					else
					{
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT);
					}
					if (!hud_group && LLPipeline::sRenderBump && use_legacy_bump)
					{ //if this is the deferred render and a bump map is present, register in post deferred bump
						registerFace(group, facep, LLRenderPass::PASS_POST_BUMP);
					}
				}
				else
				{
					if (LLPipeline::sRenderDeferred && LLPipeline::sRenderBump && use_legacy_bump)
					{ //non-shiny or fullbright deferred bump
						registerFace(group, facep, LLRenderPass::PASS_BUMP);
					}
					else
					{ //all around simple
						llassert(mask & LLVertexBuffer::MAP_NORMAL);
						if (mat && mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_MASK)
						{ //material alpha mask can be respected in non-deferred
							registerFace(group, facep, LLRenderPass::PASS_ALPHA_MASK);
						}
						else
						{
							registerFace(group, facep, LLRenderPass::PASS_SIMPLE);
						}
				    }
				}
				
				
				if (!gPipeline.shadersLoaded() && 
					!is_alpha && 
					te->getShiny() && 
					LLPipeline::sRenderBump)
				{ //shiny as an extra pass when shaders are disabled
					registerFace(group, facep, LLRenderPass::PASS_SHINY);
				}
			}
			
			//not sure why this is here, and looks like it might cause bump mapped objects to get rendered redundantly -- davep 5/11/2010
			if (!is_alpha && hud_group)
			{
				llassert((mask & LLVertexBuffer::MAP_NORMAL) || fullbright);
				facep->setPoolType((fullbright) ? LLDrawPool::POOL_FULLBRIGHT : LLDrawPool::POOL_SIMPLE);
				
				if (!force_simple && LLPipeline::sRenderBump && use_legacy_bump)
				{
					registerFace(group, facep, LLRenderPass::PASS_BUMP);
				}
			}

			if (!is_alpha && LLPipeline::sRenderGlow && te->getGlow() > 0.f)
			{
                if (gltf_mat)
                {
                    registerFace(group, facep, LLRenderPass::PASS_GLTF_GLOW);
                }
                else
                {
                    registerFace(group, facep, LLRenderPass::PASS_GLOW);
                }
			}
						
			++face_iter;
		}

		if (buffer)
		{
			buffer->unmapBuffer();
		}
	}

	group->mBufferMap[mask].clear();
	for (LLSpatialGroup::buffer_texture_map_t::iterator i = buffer_map[mask].begin(); i != buffer_map[mask].end(); ++i)
	{
		group->mBufferMap[mask][i->first] = i->second;
	}

	return geometryBytes;
}

void LLVolumeGeometryManager::addGeometryCount(LLSpatialGroup* group, U32& vertex_count, U32& index_count)
{
    //for each drawable
    for (LLSpatialGroup::element_iter drawable_iter = group->getDataBegin(); drawable_iter != group->getDataEnd(); ++drawable_iter)
    {
        LLDrawable* drawablep = (LLDrawable*)(*drawable_iter)->getDrawable();

        if (!drawablep || drawablep->isDead())
        {
            continue;
        }
    }
}

void LLGeometryManager::addGeometryCount(LLSpatialGroup* group, U32 &vertex_count, U32 &index_count)
{	
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOLUME;

    //clear off any old faces
    mFaceList.clear();

	//for each drawable
	for (LLSpatialGroup::element_iter drawable_iter = group->getDataBegin(); drawable_iter != group->getDataEnd(); ++drawable_iter)
	{
		LLDrawable* drawablep = (LLDrawable*)(*drawable_iter)->getDrawable();
		
		if (!drawablep || drawablep->isDead())
		{
			continue;
		}
	
		//for each face
		for (S32 i = 0; i < drawablep->getNumFaces(); i++)
		{
			//sum up face verts and indices
			drawablep->updateFaceSize(i);
			LLFace* facep = drawablep->getFace(i);
			if (facep)
			{
				if (facep->hasGeometry() && facep->getPixelArea() > FORCE_CULL_AREA && 
					facep->getGeomCount() + vertex_count <= 65536)
				{
					vertex_count += facep->getGeomCount();
					index_count += facep->getIndicesCount();
				
					//remember face (for sorting)
					mFaceList.push_back(facep);
				}
				else
				{
					facep->clearVertexBuffer();
				}
			}
		}
	}
}

LLHUDPartition::LLHUDPartition(LLViewerRegion* regionp) : LLBridgePartition(regionp)
{
	mPartitionType = LLViewerRegion::PARTITION_HUD;
	mDrawableType = LLPipeline::RENDER_TYPE_HUD;
	mSlopRatio = 0.f;
	mLODPeriod = 1;
}

void LLHUDPartition::shift(const LLVector4a &offset)
{
	//HUD objects don't shift with region crossing.  That would be silly.
}
