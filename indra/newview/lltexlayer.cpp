/** 
 * @file lltexlayer.cpp
 * @brief A texture layer. Used for avatars.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "lltexlayer.h"

#include "llagent.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llnotificationsutil.h"
#include "llvfile.h"
#include "llvfs.h"
#include "llviewerstats.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "pipeline.h"
#include "llassetuploadresponders.h"
#include "lltexlayerparams.h"
#include "llui.h"
#include "llagentwearables.h"
#include "llwearable.h"
#include "llviewercontrol.h"
#include "llviewershadermgr.h"
#include "llviewervisualparam.h"

//#include "../tools/imdebug/imdebug.h"

using namespace LLVOAvatarDefines;

static const S32 BAKE_UPLOAD_ATTEMPTS = 7;
static const F32 BAKE_UPLOAD_RETRY_DELAY = 2.f; // actual delay grows by power of 2 each attempt

// runway consolidate
extern std::string self_av_string();

class LLTexLayerInfo
{
	friend class LLTexLayer;
	friend class LLTexLayerTemplate;
	friend class LLTexLayerInterface;
public:
	LLTexLayerInfo();
	~LLTexLayerInfo();

	BOOL parseXml(LLXmlTreeNode* node);
	BOOL createVisualParams(LLVOAvatar *avatar);
	BOOL isUserSettable() { return mLocalTexture != -1;	}
	S32  getLocalTexture() const { return mLocalTexture; }
	BOOL getOnlyAlpha() const { return mUseLocalTextureAlphaOnly; }
	std::string getName() const { return mName;	}

private:
	std::string				mName;
	
	BOOL					mWriteAllChannels; // Don't use masking.  Just write RGBA into buffer,
	LLTexLayerInterface::ERenderPass mRenderPass;

	std::string				mGlobalColor;
	LLColor4				mFixedColor;

	S32						mLocalTexture;
	std::string				mStaticImageFileName;
	BOOL					mStaticImageIsMask;
	BOOL					mUseLocalTextureAlphaOnly; // Ignore RGB channels from the input texture.  Use alpha as a mask
	BOOL					mIsVisibilityMask;

	typedef std::vector< std::pair< std::string,BOOL > > morph_name_list_t;
	morph_name_list_t		    mMorphNameList;
	param_color_info_list_t		mParamColorInfoList;
	param_alpha_info_list_t		mParamAlphaInfoList;
};

//-----------------------------------------------------------------------------
// LLBakedUploadData()
//-----------------------------------------------------------------------------
LLBakedUploadData::LLBakedUploadData(const LLVOAvatarSelf* avatar,
									 LLTexLayerSet* layerset,
									 const LLUUID& id,
									 bool highest_res) :
	mAvatar(avatar),
	mTexLayerSet(layerset),
	mID(id),
	mStartTime(LLFrameTimer::getTotalTime()),		// Record starting time
	mIsHighestRes(highest_res)
{ 
}

//-----------------------------------------------------------------------------
// LLTexLayerSetBuffer
// The composite image that a LLTexLayerSet writes to.  Each LLTexLayerSet has one.
//-----------------------------------------------------------------------------

// static
S32 LLTexLayerSetBuffer::sGLByteCount = 0;

LLTexLayerSetBuffer::LLTexLayerSetBuffer(LLTexLayerSet* const owner, 
										 S32 width, S32 height) :
	// ORDER_LAST => must render these after the hints are created.
	LLViewerDynamicTexture( width, height, 4, LLViewerDynamicTexture::ORDER_LAST, TRUE ), 
	mUploadPending(FALSE), // Not used for any logic here, just to sync sending of updates
	mNeedsUpload(FALSE),
	mNumLowresUploads(0),
	mUploadFailCount(0),
	mNeedsUpdate(TRUE),
	mNumLowresUpdates(0),
	mTexLayerSet(owner)
{
	LLTexLayerSetBuffer::sGLByteCount += getSize();
	mNeedsUploadTimer.start();
	mNeedsUpdateTimer.start();
}

LLTexLayerSetBuffer::~LLTexLayerSetBuffer()
{
	LLTexLayerSetBuffer::sGLByteCount -= getSize();
	destroyGLTexture();
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		LLViewerDynamicTexture::sInstances[order].erase(this);  // will fail in all but one case.
	}
}

//virtual 
S8 LLTexLayerSetBuffer::getType() const 
{
	return LLViewerDynamicTexture::LL_TEX_LAYER_SET_BUFFER ;
}

//virtual 
void LLTexLayerSetBuffer::restoreGLTexture() 
{	
	LLViewerDynamicTexture::restoreGLTexture() ;
}

//virtual 
void LLTexLayerSetBuffer::destroyGLTexture() 
{
	LLViewerDynamicTexture::destroyGLTexture() ;
}

// static
void LLTexLayerSetBuffer::dumpTotalByteCount()
{
	llinfos << "Composite System GL Buffers: " << (LLTexLayerSetBuffer::sGLByteCount/1024) << "KB" << llendl;
}

void LLTexLayerSetBuffer::requestUpdate()
{
	restartUpdateTimer();
	mNeedsUpdate = TRUE;
	mNumLowresUpdates = 0;
	// If we're in the middle of uploading a baked texture, we don't care about it any more.
	// When it's downloaded, ignore it.
	mUploadID.setNull();
}

void LLTexLayerSetBuffer::requestUpload()
{
	conditionalRestartUploadTimer();
	mNeedsUpload = TRUE;
	mNumLowresUploads = 0;
	mUploadPending = TRUE;
}

void LLTexLayerSetBuffer::conditionalRestartUploadTimer()
{
	// If we requested a new upload but haven't even uploaded
	// a low res version of our last upload request, then
	// keep the timer ticking instead of resetting it.
	if (mNeedsUpload && (mNumLowresUploads == 0))
	{
		mNeedsUploadTimer.unpause();
	}
	else
	{
		mNeedsUploadTimer.reset();
		mNeedsUploadTimer.start();
	}
}

void LLTexLayerSetBuffer::restartUpdateTimer()
{
	mNeedsUpdateTimer.reset();
	mNeedsUpdateTimer.start();
}

void LLTexLayerSetBuffer::cancelUpload()
{
	mNeedsUpload = FALSE;
	mUploadPending = FALSE;
	mNeedsUploadTimer.pause();
	mUploadRetryTimer.reset();
}

void LLTexLayerSetBuffer::pushProjection() const
{
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadIdentity();
	gGL.ortho(0.0f, mFullWidth, 0.0f, mFullHeight, -1.0f, 1.0f);

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	gGL.loadIdentity();
}

void LLTexLayerSetBuffer::popProjection() const
{
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();
}

BOOL LLTexLayerSetBuffer::needsRender()
{
	llassert(mTexLayerSet->getAvatar() == gAgentAvatarp);
	if (!isAgentAvatarValid()) return FALSE;

	const BOOL upload_now = mNeedsUpload && isReadyToUpload();
	const BOOL update_now = mNeedsUpdate && isReadyToUpdate();

	// Don't render if we don't want to (or aren't ready to) upload or update.
	if (!(update_now || upload_now))
	{
		return FALSE;
	}

	// Don't render if we're animating our appearance.
	if (gAgentAvatarp->getIsAppearanceAnimating())
	{
		return FALSE;
	}

	// Don't render if we are trying to create a shirt texture but aren't wearing a skirt.
	if (gAgentAvatarp->getBakedTE(mTexLayerSet) == LLVOAvatarDefines::TEX_SKIRT_BAKED && 
		!gAgentAvatarp->isWearingWearableType(LLWearableType::WT_SKIRT))
	{
		cancelUpload();
		return FALSE;
	}

	// Render if we have at least minimal level of detail for each local texture.
	return mTexLayerSet->isLocalTextureDataAvailable();
}

void LLTexLayerSetBuffer::preRender(BOOL clear_depth)
{
	// Set up an ortho projection
	pushProjection();
	
	// keep depth buffer, we don't need to clear it
	LLViewerDynamicTexture::preRender(FALSE);
}

void LLTexLayerSetBuffer::postRender(BOOL success)
{
	popProjection();

	LLViewerDynamicTexture::postRender(success);
}

BOOL LLTexLayerSetBuffer::render()
{
	// Default color mask for tex layer render
	gGL.setColorMask(true, true);

	// do we need to upload, and do we have sufficient data to create an uploadable composite?
	// TODO: When do we upload the texture if gAgent.mNumPendingQueries is non-zero?
	const BOOL upload_now = mNeedsUpload && isReadyToUpload();
	const BOOL update_now = mNeedsUpdate && isReadyToUpdate();
	
	BOOL success = TRUE;
	
	bool use_shaders = LLGLSLShader::sNoFixedFunction;

	if (use_shaders)
	{
		gAlphaMaskProgram.bind();
		gAlphaMaskProgram.setMinimumAlpha(0.004f);
	}

	LLVertexBuffer::unbind();

	// Composite the color data
	LLGLSUIDefault gls_ui;
	success &= mTexLayerSet->render( mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight );
	gGL.flush();

	if(upload_now)
	{
		if (!success)
		{
			llinfos << "Failed attempt to bake " << mTexLayerSet->getBodyRegionName() << llendl;
			mUploadPending = FALSE;
		}
		else
		{
			if (mTexLayerSet->isVisible())
			{
				mTexLayerSet->getAvatar()->debugBakedTextureUpload(mTexLayerSet->getBakedTexIndex(), FALSE); // FALSE for start of upload, TRUE for finish.
				doUpload();
			}
			else
			{
				mUploadPending = FALSE;
				mNeedsUpload = FALSE;
				mNeedsUploadTimer.pause();
				mTexLayerSet->getAvatar()->setNewBakedTexture(mTexLayerSet->getBakedTexIndex(),IMG_INVISIBLE);
			}
		}
	}
	
	if (update_now)
	{
		doUpdate();
	}

	if (use_shaders)
	{
		gAlphaMaskProgram.unbind();
	}

	LLVertexBuffer::unbind();
	
	// reset GL state
	gGL.setColorMask(true, true);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	// we have valid texture data now
	mGLTexturep->setGLTextureCreated(true);

	return success;
}

BOOL LLTexLayerSetBuffer::isInitialized(void) const
{
	return mGLTexturep.notNull() && mGLTexturep->isGLTextureCreated();
}

BOOL LLTexLayerSetBuffer::uploadPending() const
{
	return mUploadPending;
}

BOOL LLTexLayerSetBuffer::uploadNeeded() const
{
	return mNeedsUpload;
}

BOOL LLTexLayerSetBuffer::uploadInProgress() const
{
	return !mUploadID.isNull();
}

BOOL LLTexLayerSetBuffer::isReadyToUpload() const
{
	if (!gAgentQueryManager.hasNoPendingQueries()) return FALSE; // Can't upload if there are pending queries.
	if (isAgentAvatarValid() && !gAgentAvatarp->isUsingBakedTextures()) return FALSE; // Don't upload if avatar is using composites.

	BOOL ready = FALSE;
	if (mTexLayerSet->isLocalTextureDataFinal())
	{
		// If we requested an upload and have the final LOD ready, upload (or wait a while if this is a retry)
		if (mUploadFailCount == 0)
		{
			ready = TRUE;
		}
		else
		{
			ready = mUploadRetryTimer.getElapsedTimeF32() >= BAKE_UPLOAD_RETRY_DELAY * (1 << (mUploadFailCount - 1));
		}
	}
	else
	{
		// Upload if we've hit a timeout.  Upload is a pretty expensive process so we need to make sure
		// we aren't doing uploads too frequently.
		const U32 texture_timeout = gSavedSettings.getU32("AvatarBakedTextureUploadTimeout");
		if (texture_timeout != 0)
		{
			// The timeout period increases exponentially between every lowres upload in order to prevent
			// spamming the server with frequent uploads.
			const U32 texture_timeout_threshold = texture_timeout*(1 << mNumLowresUploads);

			// If we hit our timeout and have textures available at even lower resolution, then upload.
			const BOOL is_upload_textures_timeout = mNeedsUploadTimer.getElapsedTimeF32() >= texture_timeout_threshold;
			const BOOL has_lower_lod = mTexLayerSet->isLocalTextureDataAvailable();
			ready = has_lower_lod && is_upload_textures_timeout;
		}
	}

	return ready;
}

BOOL LLTexLayerSetBuffer::isReadyToUpdate() const
{
	// If we requested an update and have the final LOD ready, then update.
	if (mTexLayerSet->isLocalTextureDataFinal()) return TRUE;

	// If we haven't done an update yet, then just do one now regardless of state of textures.
	if (mNumLowresUpdates == 0) return TRUE;

	// Update if we've hit a timeout.  Unlike for uploads, we can make this timeout fairly small
	// since render unnecessarily doesn't cost much.
	const U32 texture_timeout = gSavedSettings.getU32("AvatarBakedLocalTextureUpdateTimeout");
	if (texture_timeout != 0)
	{
		// If we hit our timeout and have textures available at even lower resolution, then update.
		const BOOL is_update_textures_timeout = mNeedsUpdateTimer.getElapsedTimeF32() >= texture_timeout;
		const BOOL has_lower_lod = mTexLayerSet->isLocalTextureDataAvailable();
		if (has_lower_lod && is_update_textures_timeout) return TRUE; 
	}

	return FALSE;
}

BOOL LLTexLayerSetBuffer::requestUpdateImmediate()
{
	mNeedsUpdate = TRUE;
	BOOL result = FALSE;

	if (needsRender())
	{
		preRender(FALSE);
		result = render();
		postRender(result);
	}

	return result;
}

// Create the baked texture, send it out to the server, then wait for it to come
// back so we can switch to using it.
void LLTexLayerSetBuffer::doUpload()
{
	llinfos << "Uploading baked " << mTexLayerSet->getBodyRegionName() << llendl;
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_TEX_BAKES);

	// Don't need caches since we're baked now.  (note: we won't *really* be baked 
	// until this image is sent to the server and the Avatar Appearance message is received.)
	mTexLayerSet->deleteCaches();

	// Get the COLOR information from our texture
	U8* baked_color_data = new U8[ mFullWidth * mFullHeight * 4 ];
	glReadPixels(mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight, GL_RGBA, GL_UNSIGNED_BYTE, baked_color_data );
	stop_glerror();

	// Get the MASK information from our texture
	LLGLSUIDefault gls_ui;
	LLPointer<LLImageRaw> baked_mask_image = new LLImageRaw(mFullWidth, mFullHeight, 1 );
	U8* baked_mask_data = baked_mask_image->getData(); 
	mTexLayerSet->gatherMorphMaskAlpha(baked_mask_data, mFullWidth, mFullHeight);


	// Create the baked image from our color and mask information
	const S32 baked_image_components = 5; // red green blue [bump] clothing
	LLPointer<LLImageRaw> baked_image = new LLImageRaw( mFullWidth, mFullHeight, baked_image_components );
	U8* baked_image_data = baked_image->getData();
	S32 i = 0;
	for (S32 u=0; u < mFullWidth; u++)
	{
		for (S32 v=0; v < mFullHeight; v++)
		{
			baked_image_data[5*i + 0] = baked_color_data[4*i + 0];
			baked_image_data[5*i + 1] = baked_color_data[4*i + 1];
			baked_image_data[5*i + 2] = baked_color_data[4*i + 2];
			baked_image_data[5*i + 3] = baked_color_data[4*i + 3]; // alpha should be correct for eyelashes.
			baked_image_data[5*i + 4] = baked_mask_data[i];
			i++;
		}
	}
	
	LLPointer<LLImageJ2C> compressedImage = new LLImageJ2C;
	const char* comment_text = LINDEN_J2C_COMMENT_PREFIX "RGBHM"; // writes into baked_color_data. 5 channels (rgb, heightfield/alpha, mask)
	if (compressedImage->encode(baked_image, comment_text))
	{
		LLTransactionID tid;
		tid.generate();
		const LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
		if (LLVFile::writeFile(compressedImage->getData(), compressedImage->getDataSize(),
							   gVFS, asset_id, LLAssetType::AT_TEXTURE))
		{
			// Read back the file and validate.
			BOOL valid = FALSE;
			LLPointer<LLImageJ2C> integrity_test = new LLImageJ2C;
			S32 file_size = 0;
			U8* data = LLVFile::readFile(gVFS, asset_id, LLAssetType::AT_TEXTURE, &file_size);
			if (data)
			{
				valid = integrity_test->validate(data, file_size); // integrity_test will delete 'data'
			}
			else
			{
				integrity_test->setLastError("Unable to read entire file");
			}
			
			if (valid)
			{
				const bool highest_lod = mTexLayerSet->isLocalTextureDataFinal();
				// Baked_upload_data is owned by the responder and deleted after the request completes.
				LLBakedUploadData* baked_upload_data = new LLBakedUploadData(gAgentAvatarp, 
																			 this->mTexLayerSet, 
																			 asset_id,
																			 highest_lod);
				// upload ID is used to avoid overlaps, e.g. when the user rapidly makes two changes outside of Face Edit.
				mUploadID = asset_id;

				// Upload the image
				const std::string url = gAgent.getRegion()->getCapability("UploadBakedTexture");
				if(!url.empty()
					&& !LLPipeline::sForceOldBakedUpload // toggle debug setting UploadBakedTexOld to change between the new caps method and old method
					&& (mUploadFailCount < (BAKE_UPLOAD_ATTEMPTS - 1))) // Try last ditch attempt via asset store if cap upload is failing.
				{
					LLSD body = LLSD::emptyMap();
					// The responder will call LLTexLayerSetBuffer::onTextureUploadComplete()
					LLHTTPClient::post(url, body, new LLSendTexLayerResponder(body, mUploadID, LLAssetType::AT_TEXTURE, baked_upload_data));
					llinfos << "Baked texture upload via capability of " << mUploadID << " to " << url << llendl;
				} 
				else
				{
					gAssetStorage->storeAssetData(tid,
												  LLAssetType::AT_TEXTURE,
												  LLTexLayerSetBuffer::onTextureUploadComplete,
												  baked_upload_data,
												  TRUE,		// temp_file
												  TRUE,		// is_priority
												  TRUE);	// store_local
					llinfos << "Baked texture upload via Asset Store." <<  llendl;
				}

				if (highest_lod)
				{
					// Sending the final LOD for the baked texture.  All done, pause 
					// the upload timer so we know how long it took.
					mNeedsUpload = FALSE;
					mNeedsUploadTimer.pause();
				}
				else
				{
					// Sending a lower level LOD for the baked texture.  Restart the upload timer.
					mNumLowresUploads++;
					mNeedsUploadTimer.unpause();
					mNeedsUploadTimer.reset();
				}

				// Print out notification that we uploaded this texture.
				if (gSavedSettings.getBOOL("DebugAvatarRezTime"))
				{
					const std::string lod_str = highest_lod ? "HighRes" : "LowRes";
					LLSD args;
					args["EXISTENCE"] = llformat("%d",(U32)mTexLayerSet->getAvatar()->debugGetExistenceTimeElapsedF32());
					args["TIME"] = llformat("%d",(U32)mNeedsUploadTimer.getElapsedTimeF32());
					args["BODYREGION"] = mTexLayerSet->getBodyRegionName();
					args["RESOLUTION"] = lod_str;
					LLNotificationsUtil::add("AvatarRezSelfBakedTextureUploadNotification",args);
					LL_DEBUGS("Avatar") << self_av_string() << "Uploading [ name: " << mTexLayerSet->getBodyRegionName() << " res:" << lod_str << " time:" << (U32)mNeedsUploadTimer.getElapsedTimeF32() << " ]" << LL_ENDL;
				}
			}
			else
			{
				// The read back and validate operation failed.  Remove the uploaded file.
				mUploadPending = FALSE;
				LLVFile file(gVFS, asset_id, LLAssetType::AT_TEXTURE, LLVFile::WRITE);
				file.remove();
				llinfos << "Unable to create baked upload file (reason: corrupted)." << llendl;
			}
		}
	}
	else
	{
		// The VFS write file operation failed.
		mUploadPending = FALSE;
		llinfos << "Unable to create baked upload file (reason: failed to write file)" << llendl;
	}

	delete [] baked_color_data;
}

// Mostly bookkeeping; don't need to actually "do" anything since
// render() will actually do the update.
void LLTexLayerSetBuffer::doUpdate()
{
	const BOOL highest_lod = mTexLayerSet->isLocalTextureDataFinal();
	if (highest_lod)
	{
		mNeedsUpdate = FALSE;
	}
	else
	{
		mNumLowresUpdates++;
	}

	restartUpdateTimer();

	// need to swtich to using this layerset if this is the first update
	// after getting the lowest LOD
	mTexLayerSet->getAvatar()->updateMeshTextures();
	
	// Print out notification that we uploaded this texture.
	if (gSavedSettings.getBOOL("DebugAvatarRezTime"))
	{
		const BOOL highest_lod = mTexLayerSet->isLocalTextureDataFinal();
		const std::string lod_str = highest_lod ? "HighRes" : "LowRes";
		LLSD args;
		args["EXISTENCE"] = llformat("%d",(U32)mTexLayerSet->getAvatar()->debugGetExistenceTimeElapsedF32());
		args["TIME"] = llformat("%d",(U32)mNeedsUpdateTimer.getElapsedTimeF32());
		args["BODYREGION"] = mTexLayerSet->getBodyRegionName();
		args["RESOLUTION"] = lod_str;
		LLNotificationsUtil::add("AvatarRezSelfBakedTextureUpdateNotification",args);
		LL_DEBUGS("Avatar") << self_av_string() << "Locally updating [ name: " << mTexLayerSet->getBodyRegionName() << " res:" << lod_str << " time:" << (U32)mNeedsUpdateTimer.getElapsedTimeF32() << " ]" << LL_ENDL;
	}
}

// static
void LLTexLayerSetBuffer::onTextureUploadComplete(const LLUUID& uuid,
												  void* userdata,
												  S32 result,
												  LLExtStat ext_status) // StoreAssetData callback (not fixed)
{
	LLBakedUploadData* baked_upload_data = (LLBakedUploadData*)userdata;

	if (isAgentAvatarValid() &&
		!gAgentAvatarp->isDead() &&
		(baked_upload_data->mAvatar == gAgentAvatarp) && // Sanity check: only the user's avatar should be uploading textures.
		(baked_upload_data->mTexLayerSet->hasComposite()))
	{
		LLTexLayerSetBuffer* layerset_buffer = baked_upload_data->mTexLayerSet->getComposite();
		S32 failures = layerset_buffer->mUploadFailCount;
		layerset_buffer->mUploadFailCount = 0;

		if (layerset_buffer->mUploadID.isNull())
		{
			// The upload got canceled, we should be in the
			// process of baking a new texture so request an
			// upload with the new data

			// BAP: does this really belong in this callback, as
			// opposed to where the cancellation takes place?
			// suspect this does nothing.
			layerset_buffer->requestUpload();
		}
		else if (baked_upload_data->mID == layerset_buffer->mUploadID)
		{
			// This is the upload we're currently waiting for.
			layerset_buffer->mUploadID.setNull();
			const std::string name(baked_upload_data->mTexLayerSet->getBodyRegionName());
			const std::string resolution = baked_upload_data->mIsHighestRes ? " full res " : " low res ";
			if (result >= 0)
			{
				layerset_buffer->mUploadPending = FALSE; // Allows sending of AgentSetAppearance later
				LLVOAvatarDefines::ETextureIndex baked_te = gAgentAvatarp->getBakedTE(layerset_buffer->mTexLayerSet);
				// Update baked texture info with the new UUID
				U64 now = LLFrameTimer::getTotalTime();		// Record starting time
				llinfos << "Baked" << resolution << "texture upload for " << name << " took " << (S32)((now - baked_upload_data->mStartTime) / 1000) << " ms" << llendl;
				gAgentAvatarp->setNewBakedTexture(baked_te, uuid);
			}
			else
			{	
				++failures;
				S32 max_attempts = baked_upload_data->mIsHighestRes ? BAKE_UPLOAD_ATTEMPTS : 1; // only retry final bakes
				llwarns << "Baked" << resolution << "texture upload for " << name << " failed (attempt " << failures << "/" << max_attempts << ")" << llendl;
				if (failures < max_attempts)
				{
					layerset_buffer->mUploadFailCount = failures;
					layerset_buffer->mUploadRetryTimer.start();
					layerset_buffer->requestUpload();
				}
			}
		}
		else
		{
			llinfos << "Received baked texture out of date, ignored." << llendl;
		}

		gAgentAvatarp->dirtyMesh();
	}
	else
	{
		// Baked texture failed to upload (in which case since we
		// didn't set the new baked texture, it means that they'll try
		// and rebake it at some point in the future (after login?)),
		// or this response to upload is out of date, in which case a
		// current response should be on the way or already processed.
		llwarns << "Baked upload failed" << llendl;
	}

	delete baked_upload_data;
}

//-----------------------------------------------------------------------------
// LLTexLayerSet
// An ordered set of texture layers that get composited into a single texture.
//-----------------------------------------------------------------------------

LLTexLayerSetInfo::LLTexLayerSetInfo() :
	mBodyRegion( "" ),
	mWidth( 512 ),
	mHeight( 512 ),
	mClearAlpha( TRUE )
{
}

LLTexLayerSetInfo::~LLTexLayerSetInfo( )
{
	std::for_each(mLayerInfoList.begin(), mLayerInfoList.end(), DeletePointer());
}

BOOL LLTexLayerSetInfo::parseXml(LLXmlTreeNode* node)
{
	llassert( node->hasName( "layer_set" ) );
	if( !node->hasName( "layer_set" ) )
	{
		return FALSE;
	}

	// body_region
	static LLStdStringHandle body_region_string = LLXmlTree::addAttributeString("body_region");
	if( !node->getFastAttributeString( body_region_string, mBodyRegion ) )
	{
		llwarns << "<layer_set> is missing body_region attribute" << llendl;
		return FALSE;
	}

	// width, height
	static LLStdStringHandle width_string = LLXmlTree::addAttributeString("width");
	if( !node->getFastAttributeS32( width_string, mWidth ) )
	{
		return FALSE;
	}

	static LLStdStringHandle height_string = LLXmlTree::addAttributeString("height");
	if( !node->getFastAttributeS32( height_string, mHeight ) )
	{
		return FALSE;
	}

	// Optional alpha component to apply after all compositing is complete.
	static LLStdStringHandle alpha_tga_file_string = LLXmlTree::addAttributeString("alpha_tga_file");
	node->getFastAttributeString( alpha_tga_file_string, mStaticAlphaFileName );

	static LLStdStringHandle clear_alpha_string = LLXmlTree::addAttributeString("clear_alpha");
	node->getFastAttributeBOOL( clear_alpha_string, mClearAlpha );

	// <layer>
	for (LLXmlTreeNode* child = node->getChildByName( "layer" );
		 child;
		 child = node->getNextNamedChild())
	{
		LLTexLayerInfo* info = new LLTexLayerInfo();
		if( !info->parseXml( child ))
		{
			delete info;
			return FALSE;
		}
		mLayerInfoList.push_back( info );		
	}
	return TRUE;
}

// creates visual params without generating layersets or layers
void LLTexLayerSetInfo::createVisualParams(LLVOAvatar *avatar)
{
	//layer_info_list_t		mLayerInfoList;
	for (layer_info_list_t::iterator layer_iter = mLayerInfoList.begin();
		 layer_iter != mLayerInfoList.end();
		 layer_iter++)
	{
		LLTexLayerInfo *layer_info = *layer_iter;
		layer_info->createVisualParams(avatar);
	}
}

//-----------------------------------------------------------------------------
// LLTexLayerSet
// An ordered set of texture layers that get composited into a single texture.
//-----------------------------------------------------------------------------

BOOL LLTexLayerSet::sHasCaches = FALSE;

LLTexLayerSet::LLTexLayerSet(LLVOAvatarSelf* const avatar) :
	mComposite( NULL ),
	mAvatar( avatar ),
	mUpdatesEnabled( FALSE ),
	mIsVisible( TRUE ),
	mBakedTexIndex(LLVOAvatarDefines::BAKED_HEAD),
	mInfo( NULL )
{
}

LLTexLayerSet::~LLTexLayerSet()
{
	deleteCaches();
	std::for_each(mLayerList.begin(), mLayerList.end(), DeletePointer());
	std::for_each(mMaskLayerList.begin(), mMaskLayerList.end(), DeletePointer());
}

//-----------------------------------------------------------------------------
// setInfo
//-----------------------------------------------------------------------------

BOOL LLTexLayerSet::setInfo(const LLTexLayerSetInfo *info)
{
	llassert(mInfo == NULL);
	mInfo = info;
	//mID = info->mID; // No ID

	mLayerList.reserve(info->mLayerInfoList.size());
	for (LLTexLayerSetInfo::layer_info_list_t::const_iterator iter = info->mLayerInfoList.begin(); 
		 iter != info->mLayerInfoList.end(); 
		 iter++)
	{
		LLTexLayerInterface *layer = NULL;
		if ( (*iter)->isUserSettable() )
		{
			layer = new LLTexLayerTemplate( this );
		}
		else
		{
			layer = new LLTexLayer(this);
		}
		// this is the first time this layer (of either type) is being created - make sure you add the parameters to the avatar
		if (!layer->setInfo(*iter, NULL))
		{
			mInfo = NULL;
			return FALSE;
		}
		if (!layer->isVisibilityMask())
		{
			mLayerList.push_back( layer );
		}
		else
		{
			mMaskLayerList.push_back(layer);
		}
	}

	requestUpdate();

	stop_glerror();

	return TRUE;
}

#if 0 // obsolete
//-----------------------------------------------------------------------------
// parseData
//-----------------------------------------------------------------------------

BOOL LLTexLayerSet::parseData(LLXmlTreeNode* node)
{
	LLTexLayerSetInfo *info = new LLTexLayerSetInfo;

	if (!info->parseXml(node))
	{
		delete info;
		return FALSE;
	}
	if (!setInfo(info))
	{
		delete info;
		return FALSE;
	}
	return TRUE;
}
#endif

void LLTexLayerSet::deleteCaches()
{
	for( layer_list_t::iterator iter = mLayerList.begin(); iter != mLayerList.end(); iter++ )
	{
		LLTexLayerInterface* layer = *iter;
		layer->deleteCaches();
	}
	for (layer_list_t::iterator iter = mMaskLayerList.begin(); iter != mMaskLayerList.end(); iter++)
	{
		LLTexLayerInterface* layer = *iter;
		layer->deleteCaches();
	}
}

// Returns TRUE if at least one packet of data has been received for each of the textures that this layerset depends on.
BOOL LLTexLayerSet::isLocalTextureDataAvailable() const
{
	if (!mAvatar->isSelf()) return FALSE;
	return ((LLVOAvatarSelf *)mAvatar)->isLocalTextureDataAvailable(this);
}


// Returns TRUE if all of the data for the textures that this layerset depends on have arrived.
BOOL LLTexLayerSet::isLocalTextureDataFinal() const
{
	if (!mAvatar->isSelf()) return FALSE;
	return ((LLVOAvatarSelf *)mAvatar)->isLocalTextureDataFinal(this);
}


BOOL LLTexLayerSet::render( S32 x, S32 y, S32 width, S32 height )
{
	BOOL success = TRUE;
	mIsVisible = TRUE;

	if (mMaskLayerList.size() > 0)
	{
		for (layer_list_t::iterator iter = mMaskLayerList.begin(); iter != mMaskLayerList.end(); iter++)
		{
			LLTexLayerInterface* layer = *iter;
			if (layer->isInvisibleAlphaMask())
			{
				mIsVisible = FALSE;
			}
		}
	}

	bool use_shaders = LLGLSLShader::sNoFixedFunction;

	LLGLSUIDefault gls_ui;
	LLGLDepthTest gls_depth(GL_FALSE, GL_FALSE);
	gGL.setColorMask(true, true);

	// clear buffer area to ensure we don't pick up UI elements
	{
		gGL.flush();
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		if (use_shaders)
		{
			gAlphaMaskProgram.setMinimumAlpha(0.0f);
		}
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f( 0.f, 0.f, 0.f, 1.f );

		gl_rect_2d_simple( width, height );

		gGL.flush();
		if (use_shaders)
		{
			gAlphaMaskProgram.setMinimumAlpha(0.004f);
		}
	}

	if (mIsVisible)
	{
		// composite color layers
		for( layer_list_t::iterator iter = mLayerList.begin(); iter != mLayerList.end(); iter++ )
		{
			LLTexLayerInterface* layer = *iter;
			if (layer->getRenderPass() == LLTexLayer::RP_COLOR)
			{
				gGL.flush();
				success &= layer->render(x, y, width, height);
				gGL.flush();
			}
		}
		
		renderAlphaMaskTextures(x, y, width, height, false);
	
		stop_glerror();
	}
	else
	{
		gGL.flush();

		gGL.setSceneBlendType(LLRender::BT_REPLACE);
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		if (use_shaders)
		{
			gAlphaMaskProgram.setMinimumAlpha(0.f);
		}

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f( 0.f, 0.f, 0.f, 0.f );

		gl_rect_2d_simple( width, height );
		gGL.setSceneBlendType(LLRender::BT_ALPHA);

		gGL.flush();
		if (use_shaders)
		{
			gAlphaMaskProgram.setMinimumAlpha(0.004f);
		}
	}

	return success;
}


BOOL LLTexLayerSet::isBodyRegion(const std::string& region) const 
{ 
	return mInfo->mBodyRegion == region; 
}

const std::string LLTexLayerSet::getBodyRegionName() const 
{ 
	return mInfo->mBodyRegion; 
}

void LLTexLayerSet::requestUpdate()
{
	if( mUpdatesEnabled )
	{
		createComposite();
		mComposite->requestUpdate(); 
	}
}

void LLTexLayerSet::requestUpload()
{
	createComposite();
	mComposite->requestUpload();
}

void LLTexLayerSet::cancelUpload()
{
	if(mComposite)
	{
		mComposite->cancelUpload();
	}
}

void LLTexLayerSet::createComposite()
{
	if(!mComposite)
	{
		S32 width = mInfo->mWidth;
		S32 height = mInfo->mHeight;
		// Composite other avatars at reduced resolution
		if( !mAvatar->isSelf() )
		{
			llerrs << "composites should not be created for non-self avatars!" << llendl;
		}
		mComposite = new LLTexLayerSetBuffer( this, width, height );
	}
}

void LLTexLayerSet::destroyComposite()
{
	if( mComposite )
	{
		mComposite = NULL;
	}
}

void LLTexLayerSet::setUpdatesEnabled( BOOL b )
{
	mUpdatesEnabled = b; 
}


void LLTexLayerSet::updateComposite()
{
	createComposite();
	mComposite->requestUpdateImmediate();
}

LLTexLayerSetBuffer* LLTexLayerSet::getComposite()
{
	if (!mComposite)
	{
		createComposite();
	}
	return mComposite;
}

const LLTexLayerSetBuffer* LLTexLayerSet::getComposite() const
{
	return mComposite;
}

void LLTexLayerSet::gatherMorphMaskAlpha(U8 *data, S32 width, S32 height)
{
	memset(data, 255, width * height);

	for( layer_list_t::iterator iter = mLayerList.begin(); iter != mLayerList.end(); iter++ )
	{
		LLTexLayerInterface* layer = *iter;
		layer->gatherAlphaMasks(data, mComposite->getOriginX(),mComposite->getOriginY(), width, height);
	}
	
	// Set alpha back to that of our alpha masks.
	renderAlphaMaskTextures(mComposite->getOriginX(), mComposite->getOriginY(), width, height, true);
}

void LLTexLayerSet::renderAlphaMaskTextures(S32 x, S32 y, S32 width, S32 height, bool forceClear)
{
	const LLTexLayerSetInfo *info = getInfo();
	
	bool use_shaders = LLGLSLShader::sNoFixedFunction;

	gGL.setColorMask(false, true);
	gGL.setSceneBlendType(LLRender::BT_REPLACE);
	
	// (Optionally) replace alpha with a single component image from a tga file.
	if (!info->mStaticAlphaFileName.empty())
	{
		gGL.flush();
		{
			LLViewerTexture* tex = LLTexLayerStaticImageList::getInstance()->getTexture(info->mStaticAlphaFileName, TRUE);
			if( tex )
			{
				LLGLSUIDefault gls_ui;
				gGL.getTexUnit(0)->bind(tex);
				gGL.getTexUnit(0)->setTextureBlendType( LLTexUnit::TB_REPLACE );
				gl_rect_2d_simple_tex( width, height );
			}
		}
		gGL.flush();
	}
	else if (forceClear || info->mClearAlpha || (mMaskLayerList.size() > 0))
	{
		// Set the alpha channel to one (clean up after previous blending)
		gGL.flush();
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		if (use_shaders)
		{
			gAlphaMaskProgram.setMinimumAlpha(0.f);
		}
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f( 0.f, 0.f, 0.f, 1.f );
		
		gl_rect_2d_simple( width, height );
		
		gGL.flush();
		if (use_shaders)
		{
			gAlphaMaskProgram.setMinimumAlpha(0.004f);
		}
	}
	
	// (Optional) Mask out part of the baked texture with alpha masks
	// will still have an effect even if mClearAlpha is set or the alpha component was replaced
	if (mMaskLayerList.size() > 0)
	{
		gGL.setSceneBlendType(LLRender::BT_MULT_ALPHA);
		gGL.getTexUnit(0)->setTextureBlendType( LLTexUnit::TB_REPLACE );
		for (layer_list_t::iterator iter = mMaskLayerList.begin(); iter != mMaskLayerList.end(); iter++)
		{
			LLTexLayerInterface* layer = *iter;
			gGL.flush();
			layer->blendAlphaTexture(x,y,width, height);
			gGL.flush();
		}
		
	}
	
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	
	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	gGL.setColorMask(true, true);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

void LLTexLayerSet::applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components)
{
	mAvatar->applyMorphMask(tex_data, width, height, num_components, mBakedTexIndex);
}

BOOL LLTexLayerSet::isMorphValid() const
{
	for(layer_list_t::const_iterator iter = mLayerList.begin(); iter != mLayerList.end(); iter++ )
	{
		const LLTexLayerInterface* layer = *iter;
		if (layer && !layer->isMorphValid())
		{
			return FALSE;
		}
	}
	return TRUE;
}

void LLTexLayerSet::invalidateMorphMasks()
{
	for( layer_list_t::iterator iter = mLayerList.begin(); iter != mLayerList.end(); iter++ )
	{
		LLTexLayerInterface* layer = *iter;
		if (layer)
		{
			layer->invalidateMorphMasks();
		}
	}
}


//-----------------------------------------------------------------------------
// LLTexLayerInfo
//-----------------------------------------------------------------------------
LLTexLayerInfo::LLTexLayerInfo() :
	mWriteAllChannels( FALSE ),
	mRenderPass(LLTexLayer::RP_COLOR),
	mFixedColor( 0.f, 0.f, 0.f, 0.f ),
	mLocalTexture( -1 ),
	mStaticImageIsMask( FALSE ),
	mUseLocalTextureAlphaOnly(FALSE),
	mIsVisibilityMask(FALSE)
{
}

LLTexLayerInfo::~LLTexLayerInfo( )
{
	std::for_each(mParamColorInfoList.begin(), mParamColorInfoList.end(), DeletePointer());
	std::for_each(mParamAlphaInfoList.begin(), mParamAlphaInfoList.end(), DeletePointer());
}

BOOL LLTexLayerInfo::parseXml(LLXmlTreeNode* node)
{
	llassert( node->hasName( "layer" ) );

	// name attribute
	static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
	if( !node->getFastAttributeString( name_string, mName ) )
	{
		return FALSE;
	}
	
	static LLStdStringHandle write_all_channels_string = LLXmlTree::addAttributeString("write_all_channels");
	node->getFastAttributeBOOL( write_all_channels_string, mWriteAllChannels );

	std::string render_pass_name;
	static LLStdStringHandle render_pass_string = LLXmlTree::addAttributeString("render_pass");
	if( node->getFastAttributeString( render_pass_string, render_pass_name ) )
	{
		if( render_pass_name == "bump" )
		{
			mRenderPass = LLTexLayer::RP_BUMP;
		}
	}

	// Note: layers can have either a "global_color" attrib, a "fixed_color" attrib, or a <param_color> child.
	// global color attribute (optional)
	static LLStdStringHandle global_color_string = LLXmlTree::addAttributeString("global_color");
	node->getFastAttributeString( global_color_string, mGlobalColor );

	// Visibility mask (optional)
	BOOL is_visibility;
	static LLStdStringHandle visibility_mask_string = LLXmlTree::addAttributeString("visibility_mask");
	if (node->getFastAttributeBOOL(visibility_mask_string, is_visibility))
	{
		mIsVisibilityMask = is_visibility;
	}

	// color attribute (optional)
	LLColor4U color4u;
	static LLStdStringHandle fixed_color_string = LLXmlTree::addAttributeString("fixed_color");
	if( node->getFastAttributeColor4U( fixed_color_string, color4u ) )
	{
		mFixedColor.setVec( color4u );
	}

		// <texture> optional sub-element
	for (LLXmlTreeNode* texture_node = node->getChildByName( "texture" );
		 texture_node;
		 texture_node = node->getNextNamedChild())
	{
		std::string local_texture_name;
		static LLStdStringHandle tga_file_string = LLXmlTree::addAttributeString("tga_file");
		static LLStdStringHandle local_texture_string = LLXmlTree::addAttributeString("local_texture");
		static LLStdStringHandle file_is_mask_string = LLXmlTree::addAttributeString("file_is_mask");
		static LLStdStringHandle local_texture_alpha_only_string = LLXmlTree::addAttributeString("local_texture_alpha_only");
		if( texture_node->getFastAttributeString( tga_file_string, mStaticImageFileName ) )
		{
			texture_node->getFastAttributeBOOL( file_is_mask_string, mStaticImageIsMask );
		}
		else if (texture_node->getFastAttributeString(local_texture_string, local_texture_name))
		{
			texture_node->getFastAttributeBOOL( local_texture_alpha_only_string, mUseLocalTextureAlphaOnly );

			/* if ("upper_shirt" == local_texture_name)
				mLocalTexture = TEX_UPPER_SHIRT; */
			mLocalTexture = TEX_NUM_INDICES;
			for (LLVOAvatarDictionary::Textures::const_iterator iter = LLVOAvatarDictionary::getInstance()->getTextures().begin();
				 iter != LLVOAvatarDictionary::getInstance()->getTextures().end();
				 iter++)
			{
				const LLVOAvatarDictionary::TextureEntry *texture_dict = iter->second;
				if (local_texture_name == texture_dict->mName)
			{
					mLocalTexture = iter->first;
					break;
			}
			}
			if (mLocalTexture == TEX_NUM_INDICES)
			{
				llwarns << "<texture> element has invalid local_texture attribute: " << mName << " " << local_texture_name << llendl;
				return FALSE;
			}
		}
		else	
		{
			llwarns << "<texture> element is missing a required attribute. " << mName << llendl;
			return FALSE;
		}
	}

	for (LLXmlTreeNode* maskNode = node->getChildByName( "morph_mask" );
		 maskNode;
		 maskNode = node->getNextNamedChild())
	{
		std::string morph_name;
		static LLStdStringHandle morph_name_string = LLXmlTree::addAttributeString("morph_name");
		if (maskNode->getFastAttributeString(morph_name_string, morph_name))
		{
			BOOL invert = FALSE;
			static LLStdStringHandle invert_string = LLXmlTree::addAttributeString("invert");
			maskNode->getFastAttributeBOOL(invert_string, invert);			
			mMorphNameList.push_back(std::pair<std::string,BOOL>(morph_name,invert));
		}
	}

	// <param> optional sub-element (color or alpha params)
	for (LLXmlTreeNode* child = node->getChildByName( "param" );
		 child;
		 child = node->getNextNamedChild())
	{
		if( child->getChildByName( "param_color" ) )
		{
			// <param><param_color/></param>
			LLTexLayerParamColorInfo* info = new LLTexLayerParamColorInfo();
			if (!info->parseXml(child))
			{
				delete info;
				return FALSE;
			}
			mParamColorInfoList.push_back(info);
		}
		else if( child->getChildByName( "param_alpha" ) )
		{
			// <param><param_alpha/></param>
			LLTexLayerParamAlphaInfo* info = new LLTexLayerParamAlphaInfo( );
			if (!info->parseXml(child))
			{
				delete info;
				return FALSE;
			}
 			mParamAlphaInfoList.push_back(info);
		}
	}
	
	return TRUE;
}

BOOL LLTexLayerInfo::createVisualParams(LLVOAvatar *avatar)
{
	BOOL success = TRUE;
	for (param_color_info_list_t::iterator color_info_iter = mParamColorInfoList.begin();
		 color_info_iter != mParamColorInfoList.end();
		 color_info_iter++)
	{
		LLTexLayerParamColorInfo * color_info = *color_info_iter;
		LLTexLayerParamColor* param_color = new LLTexLayerParamColor(avatar);
		if (!param_color->setInfo(color_info, TRUE))
		{
			llwarns << "NULL TexLayer Color Param could not be added to visual param list. Deleting." << llendl;
			delete param_color;
			success = FALSE;
		}
	}

	for (param_alpha_info_list_t::iterator alpha_info_iter = mParamAlphaInfoList.begin();
		 alpha_info_iter != mParamAlphaInfoList.end();
		 alpha_info_iter++)
	{
		LLTexLayerParamAlphaInfo * alpha_info = *alpha_info_iter;
		LLTexLayerParamAlpha* param_alpha = new LLTexLayerParamAlpha(avatar);
		if (!param_alpha->setInfo(alpha_info, TRUE))
		{
			llwarns << "NULL TexLayer Alpha Param could not be added to visual param list. Deleting." << llendl;
			delete param_alpha;
			success = FALSE;
		}
	}

	return success;
}

LLTexLayerInterface::LLTexLayerInterface(LLTexLayerSet* const layer_set):
	mTexLayerSet( layer_set ),
	mMorphMasksValid( FALSE ),
	mInfo(NULL),
	mHasMorph(FALSE)
{
}

LLTexLayerInterface::LLTexLayerInterface(const LLTexLayerInterface &layer, LLWearable *wearable):
	mTexLayerSet( layer.mTexLayerSet ),
	mInfo(NULL)
{
	// don't add visual params for cloned layers
	setInfo(layer.getInfo(), wearable);

	mHasMorph = layer.mHasMorph;
}

BOOL LLTexLayerInterface::setInfo(const LLTexLayerInfo *info, LLWearable* wearable  ) // This sets mInfo and calls initialization functions
{
	// setInfo should only be called once. Code is not robust enough to handle redefinition of a texlayer.
	// Not a critical warning, but could be useful for debugging later issues. -Nyx
	if (mInfo != NULL) 
	{
			llwarns << "mInfo != NULL" << llendl;
	}
	mInfo = info;
	//mID = info->mID; // No ID

	mParamColorList.reserve(mInfo->mParamColorInfoList.size());
	for (param_color_info_list_t::const_iterator iter = mInfo->mParamColorInfoList.begin(); 
		 iter != mInfo->mParamColorInfoList.end(); 
		 iter++)
	{
		LLTexLayerParamColor* param_color;
		if (!wearable)
			{
				param_color = new LLTexLayerParamColor(this);
				if (!param_color->setInfo(*iter, TRUE))
				{
					mInfo = NULL;
					return FALSE;
				}
			}
			else
			{
				param_color = (LLTexLayerParamColor*)wearable->getVisualParam((*iter)->getID());
				if (!param_color)
				{
					mInfo = NULL;
					return FALSE;
				}
			}
			mParamColorList.push_back( param_color );
		}

	mParamAlphaList.reserve(mInfo->mParamAlphaInfoList.size());
	for (param_alpha_info_list_t::const_iterator iter = mInfo->mParamAlphaInfoList.begin(); 
		 iter != mInfo->mParamAlphaInfoList.end(); 
		 iter++)
		{
			LLTexLayerParamAlpha* param_alpha;
			if (!wearable)
			{
				param_alpha = new LLTexLayerParamAlpha( this );
				if (!param_alpha->setInfo(*iter, TRUE))
				{
					mInfo = NULL;
					return FALSE;
				}
			}
			else
			{
				param_alpha = (LLTexLayerParamAlpha*) wearable->getVisualParam((*iter)->getID());
				if (!param_alpha)
				{
					mInfo = NULL;
					return FALSE;
				}
			}
			mParamAlphaList.push_back( param_alpha );
		}

	return TRUE;
}

/*virtual*/ void LLTexLayerInterface::requestUpdate()
{
	mTexLayerSet->requestUpdate();
}

const std::string& LLTexLayerInterface::getName() const
{
	return mInfo->mName; 
}

LLTexLayerInterface::ERenderPass LLTexLayerInterface::getRenderPass() const
{
	return mInfo->mRenderPass; 
}

const std::string& LLTexLayerInterface::getGlobalColor() const
{
	return mInfo->mGlobalColor; 
}

BOOL LLTexLayerInterface::isVisibilityMask() const
{
	return mInfo->mIsVisibilityMask;
}

void LLTexLayerInterface::invalidateMorphMasks()
{
	mMorphMasksValid = FALSE;
}

LLViewerVisualParam* LLTexLayerInterface::getVisualParamPtr(S32 index) const
{
	LLViewerVisualParam *result = NULL;
	for (param_color_list_t::const_iterator color_iter = mParamColorList.begin(); color_iter != mParamColorList.end() && !result; ++color_iter)
	{
		if ((*color_iter)->getID() == index)
		{
			result = *color_iter;
		}
	}
	for (param_alpha_list_t::const_iterator alpha_iter = mParamAlphaList.begin(); alpha_iter != mParamAlphaList.end() && !result; ++alpha_iter)
	{
		if ((*alpha_iter)->getID() == index)
		{
			result = *alpha_iter;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// LLTexLayer
// A single texture layer, consisting of:
//		* color, consisting of either
//			* one or more color parameters (weighted colors)
//			* a reference to a global color
//			* a fixed color with non-zero alpha
//			* opaque white (the default)
//		* (optional) a texture defined by either
//			* a GUID
//			* a texture entry index (TE)
//		* (optional) one or more alpha parameters (weighted alpha textures)
//-----------------------------------------------------------------------------
LLTexLayer::LLTexLayer(LLTexLayerSet* const layer_set) :
	LLTexLayerInterface( layer_set ),
	mLocalTextureObject(NULL)
{
}

LLTexLayer::LLTexLayer(const LLTexLayer &layer, LLWearable *wearable) :
	LLTexLayerInterface( layer, wearable ),
	mLocalTextureObject(NULL)
{
}

LLTexLayer::LLTexLayer(const LLTexLayerTemplate &layer_template, LLLocalTextureObject *lto, LLWearable *wearable) :
	LLTexLayerInterface( layer_template, wearable ),
	mLocalTextureObject(lto)
{
}

LLTexLayer::~LLTexLayer()
{
	// mParamAlphaList and mParamColorList are LLViewerVisualParam's and get
	// deleted with ~LLCharacter()
	//std::for_each(mParamAlphaList.begin(), mParamAlphaList.end(), DeletePointer());
	//std::for_each(mParamColorList.begin(), mParamColorList.end(), DeletePointer());
	
	for( alpha_cache_t::iterator iter = mAlphaCache.begin();
		 iter != mAlphaCache.end(); iter++ )
	{
		U8* alpha_data = iter->second;
		delete [] alpha_data;
	}

}

//-----------------------------------------------------------------------------
// setInfo
//-----------------------------------------------------------------------------

BOOL LLTexLayer::setInfo(const LLTexLayerInfo* info, LLWearable* wearable  )
{
	return LLTexLayerInterface::setInfo(info, wearable);
}

//static 
void LLTexLayer::calculateTexLayerColor(const param_color_list_t &param_list, LLColor4 &net_color)
{
	for (param_color_list_t::const_iterator iter = param_list.begin();
		 iter != param_list.end(); iter++)
	{
		const LLTexLayerParamColor* param = *iter;
		LLColor4 param_net = param->getNetColor();
		const LLTexLayerParamColorInfo *info = (LLTexLayerParamColorInfo *)param->getInfo();
		switch(info->getOperation())
		{
			case LLTexLayerParamColor::OP_ADD:
				net_color += param_net;
				break;
			case LLTexLayerParamColor::OP_MULTIPLY:
				net_color = net_color * param_net;
				break;
			case LLTexLayerParamColor::OP_BLEND:
				net_color = lerp(net_color, param_net, param->getWeight());
				break;
			default:
				llassert(0);
				break;
		}
	}
	net_color.clamp();
}

/*virtual*/ void LLTexLayer::deleteCaches()
{
	// Only need to delete caches for alpha params. Color params don't hold extra memory
	for (param_alpha_list_t::iterator iter = mParamAlphaList.begin();
		 iter != mParamAlphaList.end(); iter++ )
	{
		LLTexLayerParamAlpha* param = *iter;
		param->deleteCaches();
	}
}

BOOL LLTexLayer::render(S32 x, S32 y, S32 width, S32 height)
{
	LLGLEnable color_mat(GL_COLOR_MATERIAL);
	gPipeline.disableLights();

	bool use_shaders = LLGLSLShader::sNoFixedFunction;

	LLColor4 net_color;
	BOOL color_specified = findNetColor(&net_color);
	
	if (mTexLayerSet->getAvatar()->mIsDummy)
	{
		color_specified = true;
		net_color = LLVOAvatar::getDummyColor();
	}

	BOOL success = TRUE;
	
	// If you can't see the layer, don't render it.
	if( is_approx_zero( net_color.mV[VW] ) )
	{
		return success;
	}

	BOOL alpha_mask_specified = FALSE;
	param_alpha_list_t::const_iterator iter = mParamAlphaList.begin();
	if( iter != mParamAlphaList.end() )
	{
		// If we have alpha masks, but we're skipping all of them, skip the whole layer.
		// However, we can't do this optimization if we have morph masks that need updating.
/*		if (!mHasMorph)
		{
			BOOL skip_layer = TRUE;

			while( iter != mParamAlphaList.end() )
			{
				const LLTexLayerParamAlpha* param = *iter;
		
				if( !param->getSkip() )
				{
					skip_layer = FALSE;
					break;
				}

				iter++;
			} 

			if( skip_layer )
			{
				return success;
			}
		}//*/

		renderMorphMasks(x, y, width, height, net_color);
		alpha_mask_specified = TRUE;
		gGL.flush();
		gGL.blendFunc(LLRender::BF_DEST_ALPHA, LLRender::BF_ONE_MINUS_DEST_ALPHA);
	}

	gGL.color4fv( net_color.mV);

	if( getInfo()->mWriteAllChannels )
	{
		gGL.flush();
		gGL.setSceneBlendType(LLRender::BT_REPLACE);
	}

	if( (getInfo()->mLocalTexture != -1) && !getInfo()->mUseLocalTextureAlphaOnly )
	{
		{
			LLViewerTexture* tex = NULL;
			if (mLocalTextureObject && mLocalTextureObject->getImage())
			{
				tex = mLocalTextureObject->getImage();
				if (mLocalTextureObject->getID() == IMG_DEFAULT_AVATAR)
				{
					tex = NULL;
				}
			}
			else
			{
				llinfos << "lto not defined or image not defined: " << getInfo()->getLocalTexture() << " lto: " << mLocalTextureObject << llendl;
			}
//			if( mTexLayerSet->getAvatar()->getLocalTextureGL((ETextureIndex)getInfo()->mLocalTexture, &image_gl ) )
			{
				if( tex )
				{
					bool no_alpha_test = getInfo()->mWriteAllChannels;
					LLGLDisable alpha_test(no_alpha_test ? GL_ALPHA_TEST : 0);
					if (use_shaders && no_alpha_test)
					{
						gAlphaMaskProgram.setMinimumAlpha(0.f);
					}
					
					LLTexUnit::eTextureAddressMode old_mode = tex->getAddressMode();
					
					gGL.getTexUnit(0)->bind(tex, TRUE);
					gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

					gl_rect_2d_simple_tex( width, height );

					gGL.getTexUnit(0)->setTextureAddressMode(old_mode);
					gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
					if (use_shaders && no_alpha_test)
					{
						gAlphaMaskProgram.setMinimumAlpha(0.004f);
					}
					
				}
			}
//			else
//			{
//				success = FALSE;
//			}
		}
	}

	if( !getInfo()->mStaticImageFileName.empty() )
	{
		{
			LLViewerTexture* tex = LLTexLayerStaticImageList::getInstance()->getTexture(getInfo()->mStaticImageFileName, getInfo()->mStaticImageIsMask);
			if( tex )
			{
				gGL.getTexUnit(0)->bind(tex, TRUE);
				gl_rect_2d_simple_tex( width, height );
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			}
			else
			{
				success = FALSE;
			}
		}
	}

	if(((-1 == getInfo()->mLocalTexture) ||
		 getInfo()->mUseLocalTextureAlphaOnly) &&
		getInfo()->mStaticImageFileName.empty() &&
		color_specified )
	{
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		if (use_shaders)
		{
			gAlphaMaskProgram.setMinimumAlpha(0.f);
		}
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4fv( net_color.mV );
		gl_rect_2d_simple( width, height );
		if (use_shaders)
		{
			gAlphaMaskProgram.setMinimumAlpha(0.004f);
		}
	}

	if( alpha_mask_specified || getInfo()->mWriteAllChannels )
	{
		// Restore standard blend func value
		gGL.flush();
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
		stop_glerror();
	}

	if( !success )
	{
		llinfos << "LLTexLayer::render() partial: " << getInfo()->mName << llendl;
	}
	return success;
}

const U8*	LLTexLayer::getAlphaData() const
{
	LLCRC alpha_mask_crc;
	const LLUUID& uuid = getUUID();
	alpha_mask_crc.update((U8*)(&uuid.mData), UUID_BYTES);

	for (param_alpha_list_t::const_iterator iter = mParamAlphaList.begin(); iter != mParamAlphaList.end(); iter++)
	{
		const LLTexLayerParamAlpha* param = *iter;
		// MULTI-WEARABLE: verify visual parameters used here
		F32 param_weight = param->getWeight();
		alpha_mask_crc.update((U8*)&param_weight, sizeof(F32));
	}

	U32 cache_index = alpha_mask_crc.getCRC();

	alpha_cache_t::const_iterator iter2 = mAlphaCache.find(cache_index);
	return (iter2 == mAlphaCache.end()) ? 0 : iter2->second;
}

BOOL LLTexLayer::findNetColor(LLColor4* net_color) const
{
	// Color is either:
	//	* one or more color parameters (weighted colors)  (which may make use of a global color or fixed color)
	//	* a reference to a global color
	//	* a fixed color with non-zero alpha
	//	* opaque white (the default)

	if( !mParamColorList.empty() )
	{
		if( !getGlobalColor().empty() )
		{
			net_color->setVec( mTexLayerSet->getAvatar()->getGlobalColor( getInfo()->mGlobalColor ) );
		}
		else if (getInfo()->mFixedColor.mV[VW])
		{
			net_color->setVec( getInfo()->mFixedColor );
		}
		else
		{
			net_color->setVec( 0.f, 0.f, 0.f, 0.f );
		}
		
		calculateTexLayerColor(mParamColorList, *net_color);
		return TRUE;
	}

	if( !getGlobalColor().empty() )
	{
		net_color->setVec( mTexLayerSet->getAvatar()->getGlobalColor( getGlobalColor() ) );
		return TRUE;
	}

	if( getInfo()->mFixedColor.mV[VW] )
	{
		net_color->setVec( getInfo()->mFixedColor );
		return TRUE;
	}

	net_color->setToWhite();

	return FALSE; // No need to draw a separate colored polygon
}

BOOL LLTexLayer::blendAlphaTexture(S32 x, S32 y, S32 width, S32 height)
{
	BOOL success = TRUE;

	gGL.flush();
	
	bool use_shaders = LLGLSLShader::sNoFixedFunction;

	if( !getInfo()->mStaticImageFileName.empty() )
	{
		LLViewerTexture* tex = LLTexLayerStaticImageList::getInstance()->getTexture( getInfo()->mStaticImageFileName, getInfo()->mStaticImageIsMask );
		if( tex )
		{
			LLGLSNoAlphaTest gls_no_alpha_test;
			if (use_shaders)
			{
				gAlphaMaskProgram.setMinimumAlpha(0.f);
			}
			gGL.getTexUnit(0)->bind(tex, TRUE);
			gl_rect_2d_simple_tex( width, height );
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			if (use_shaders)
			{
				gAlphaMaskProgram.setMinimumAlpha(0.004f);
			}
		}
		else
		{
			success = FALSE;
		}
	}
	else
	{
		if (getInfo()->mLocalTexture >=0 && getInfo()->mLocalTexture < TEX_NUM_INDICES)
		{
			LLViewerTexture* tex = mLocalTextureObject->getImage();
			if (tex)
			{
				LLGLSNoAlphaTest gls_no_alpha_test;
				if (use_shaders)
				{
					gAlphaMaskProgram.setMinimumAlpha(0.f);
				}
				gGL.getTexUnit(0)->bind(tex);
				gl_rect_2d_simple_tex( width, height );
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				success = TRUE;
				if (use_shaders)
				{
					gAlphaMaskProgram.setMinimumAlpha(0.004f);
				}
			}
		}
	}
	
	return success;
}

/*virtual*/ void LLTexLayer::gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height)
{
	addAlphaMask(data, originX, originY, width, height);
}

BOOL LLTexLayer::renderMorphMasks(S32 x, S32 y, S32 width, S32 height, const LLColor4 &layer_color)
{
	BOOL success = TRUE;

	llassert( !mParamAlphaList.empty() );

	bool use_shaders = LLGLSLShader::sNoFixedFunction;

	if (use_shaders)
	{
		gAlphaMaskProgram.setMinimumAlpha(0.f);
	}

	gGL.setColorMask(false, true);

	LLTexLayerParamAlpha* first_param = *mParamAlphaList.begin();
	// Note: if the first param is a mulitply, multiply against the current buffer's alpha
	if( !first_param || !first_param->getMultiplyBlend() )
	{
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	
		// Clear the alpha
		gGL.flush();
		gGL.setSceneBlendType(LLRender::BT_REPLACE);

		gGL.color4f( 0.f, 0.f, 0.f, 0.f );
		gl_rect_2d_simple( width, height );
	}

	// Accumulate alphas
	LLGLSNoAlphaTest gls_no_alpha_test;
	gGL.color4f( 1.f, 1.f, 1.f, 1.f );
	for (param_alpha_list_t::iterator iter = mParamAlphaList.begin(); iter != mParamAlphaList.end(); iter++)
	{
		LLTexLayerParamAlpha* param = *iter;
		success &= param->render( x, y, width, height );
	}

	// Approximates a min() function
	gGL.flush();
	gGL.setSceneBlendType(LLRender::BT_MULT_ALPHA);

	// Accumulate the alpha component of the texture
	if( getInfo()->mLocalTexture != -1 )
	{
		LLViewerTexture* tex = mLocalTextureObject->getImage();
		if( tex && (tex->getComponents() == 4) )
		{
			LLGLSNoAlphaTest gls_no_alpha_test;
			LLTexUnit::eTextureAddressMode old_mode = tex->getAddressMode();
			
			gGL.getTexUnit(0)->bind(tex, TRUE);
			gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

			gl_rect_2d_simple_tex( width, height );

			gGL.getTexUnit(0)->setTextureAddressMode(old_mode);
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		}
	}

	if( !getInfo()->mStaticImageFileName.empty() )
	{
		LLViewerTexture* tex = LLTexLayerStaticImageList::getInstance()->getTexture(getInfo()->mStaticImageFileName, getInfo()->mStaticImageIsMask);
		if( tex )
		{
			if(	(tex->getComponents() == 4) ||
				( (tex->getComponents() == 1) && getInfo()->mStaticImageIsMask ) )
			{
				LLGLSNoAlphaTest gls_no_alpha_test;
				gGL.getTexUnit(0)->bind(tex, TRUE);
				gl_rect_2d_simple_tex( width, height );
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			}
		}
	}

	// Draw a rectangle with the layer color to multiply the alpha by that color's alpha.
	// Note: we're still using gGL.blendFunc( GL_DST_ALPHA, GL_ZERO );
	if (layer_color.mV[VW] != 1.f)
	{
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4fv(layer_color.mV);
		gl_rect_2d_simple( width, height );
	}

	if (use_shaders)
	{
		gAlphaMaskProgram.setMinimumAlpha(0.004f);
	}

	LLGLSUIDefault gls_ui;

	gGL.setColorMask(true, true);
	
	if (hasMorph() && success)
	{
		LLCRC alpha_mask_crc;
		const LLUUID& uuid = getUUID();
		alpha_mask_crc.update((U8*)(&uuid.mData), UUID_BYTES);
		
		for (param_alpha_list_t::const_iterator iter = mParamAlphaList.begin(); iter != mParamAlphaList.end(); iter++)
		{
			const LLTexLayerParamAlpha* param = *iter;
			F32 param_weight = param->getWeight();
			alpha_mask_crc.update((U8*)&param_weight, sizeof(F32));
		}

		U32 cache_index = alpha_mask_crc.getCRC();
		U8* alpha_data = get_if_there(mAlphaCache,cache_index,(U8*)NULL);
		if (!alpha_data)
		{
			// clear out a slot if we have filled our cache
			S32 max_cache_entries = getTexLayerSet()->getAvatar()->isSelf() ? 4 : 1;
			while ((S32)mAlphaCache.size() >= max_cache_entries)
			{
				alpha_cache_t::iterator iter2 = mAlphaCache.begin(); // arbitrarily grab the first entry
				alpha_data = iter2->second;
				delete [] alpha_data;
				mAlphaCache.erase(iter2);
			}
			alpha_data = new U8[width * height];
			mAlphaCache[cache_index] = alpha_data;
			glReadPixels(x, y, width, height, GL_ALPHA, GL_UNSIGNED_BYTE, alpha_data);
		}
		
		getTexLayerSet()->getAvatar()->dirtyMesh();

		mMorphMasksValid = TRUE;
		getTexLayerSet()->applyMorphMask(alpha_data, width, height, 1);
	}

	return success;
}

void LLTexLayer::addAlphaMask(U8 *data, S32 originX, S32 originY, S32 width, S32 height)
{
	S32 size = width * height;
	const U8* alphaData = getAlphaData();
	if (!alphaData && hasAlphaParams())
	{
		LLColor4 net_color;
		findNetColor( &net_color );
		// TODO: eliminate need for layer morph mask valid flag
		invalidateMorphMasks();
		renderMorphMasks(originX, originY, width, height, net_color);
		alphaData = getAlphaData();
	}
	if (alphaData)
	{
		for( S32 i = 0; i < size; i++ )
		{
			U8 curAlpha = data[i];
			U16 resultAlpha = curAlpha;
			resultAlpha *= (alphaData[i] + 1);
			resultAlpha = resultAlpha >> 8;
			data[i] = (U8)resultAlpha;
		}
	}
}

/*virtual*/ BOOL LLTexLayer::isInvisibleAlphaMask() const
{
	if (mLocalTextureObject)
	{
		if (mLocalTextureObject->getID() == IMG_INVISIBLE)
		{
			return TRUE;
		}
	}

	return FALSE;
}

LLUUID LLTexLayer::getUUID() const
{
	LLUUID uuid;
	if( getInfo()->mLocalTexture != -1 )
	{
			LLViewerTexture* tex = mLocalTextureObject->getImage();
			if (tex)
			{
				uuid = mLocalTextureObject->getID();
			}
	}
	if( !getInfo()->mStaticImageFileName.empty() )
	{
			LLViewerTexture* tex = LLTexLayerStaticImageList::getInstance()->getTexture(getInfo()->mStaticImageFileName, getInfo()->mStaticImageIsMask);
			if( tex )
			{
				uuid = tex->getID();
			}
	}
	return uuid;
}


//-----------------------------------------------------------------------------
// LLTexLayerTemplate
// A single texture layer, consisting of:
//		* color, consisting of either
//			* one or more color parameters (weighted colors)
//			* a reference to a global color
//			* a fixed color with non-zero alpha
//			* opaque white (the default)
//		* (optional) a texture defined by either
//			* a GUID
//			* a texture entry index (TE)
//		* (optional) one or more alpha parameters (weighted alpha textures)
//-----------------------------------------------------------------------------
LLTexLayerTemplate::LLTexLayerTemplate(LLTexLayerSet* layer_set) :
	LLTexLayerInterface(layer_set)
{
}

LLTexLayerTemplate::LLTexLayerTemplate(const LLTexLayerTemplate &layer) :
	LLTexLayerInterface(layer)
{
}

LLTexLayerTemplate::~LLTexLayerTemplate()
{
}

//-----------------------------------------------------------------------------
// setInfo
//-----------------------------------------------------------------------------

/*virtual*/ BOOL LLTexLayerTemplate::setInfo(const LLTexLayerInfo* info, LLWearable* wearable  )
{
	return LLTexLayerInterface::setInfo(info, wearable);
}

U32 LLTexLayerTemplate::updateWearableCache() const
{
	mWearableCache.clear();

	S32 te = mInfo->mLocalTexture;
	if (te == -1)
	{
		//this isn't a cloneable layer 
		return 0;
	}
	LLWearableType::EType wearable_type = LLVOAvatarDictionary::getTEWearableType((ETextureIndex)te);
	U32 num_wearables = gAgentWearables.getWearableCount(wearable_type);
	U32 added = 0;
	for (U32 i = 0; i < num_wearables; i++)
	{
		LLWearable*  wearable = gAgentWearables.getWearable(wearable_type, i);
		if (!wearable)
		{
			continue;
		}
		mWearableCache.push_back(wearable);
		added++;
	}
	return added;
}
LLTexLayer* LLTexLayerTemplate::getLayer(U32 i) const
{
	if (mWearableCache.size() <= i)
	{
		return NULL;
	}
	LLWearable *wearable = mWearableCache[i];
	LLLocalTextureObject *lto = NULL;
	LLTexLayer *layer = NULL;
	if (wearable)
	{
		 lto = wearable->getLocalTextureObject(mInfo->mLocalTexture);
	}
	if (lto)
	{
		layer = lto->getTexLayer(getName());
	}
	return layer;
}

/*virtual*/ BOOL LLTexLayerTemplate::render(S32 x, S32 y, S32 width, S32 height)
{
	if(!mInfo)
	{
		return FALSE ;
	}

	BOOL success = TRUE;
	updateWearableCache();
	for (wearable_cache_t::const_iterator iter = mWearableCache.begin(); iter!= mWearableCache.end(); iter++)
	{
		LLWearable* wearable = NULL;
		LLLocalTextureObject *lto = NULL;
		LLTexLayer *layer = NULL;
		wearable = *iter;
		if (wearable)
		{
			lto = wearable->getLocalTextureObject(mInfo->mLocalTexture);
		}
		if (lto)
		{
			layer = lto->getTexLayer(getName());
		}
		if (layer)
		{
			wearable->writeToAvatar();
			layer->setLTO(lto);
			success &= layer->render(x,y,width,height);
		}
	}

	return success;
}

/*virtual*/ BOOL LLTexLayerTemplate::blendAlphaTexture( S32 x, S32 y, S32 width, S32 height) // Multiplies a single alpha texture against the frame buffer
{
	BOOL success = TRUE;
	U32 num_wearables = updateWearableCache();
	for (U32 i = 0; i < num_wearables; i++)
	{
		LLTexLayer *layer = getLayer(i);
		if (layer)
		{
			success &= layer->blendAlphaTexture(x,y,width,height);
		}
	}
	return success;
}

/*virtual*/ void LLTexLayerTemplate::gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height)
{
	U32 num_wearables = updateWearableCache();
	for (U32 i = 0; i < num_wearables; i++)
	{
		LLTexLayer *layer = getLayer(i);
		if (layer)
		{
			layer->addAlphaMask(data, originX, originY, width, height);
		}
	}
}

/*virtual*/ void LLTexLayerTemplate::setHasMorph(BOOL newval)
{ 
	mHasMorph = newval;
	U32 num_wearables = updateWearableCache();
	for (U32 i = 0; i < num_wearables; i++)
	{
		LLTexLayer *layer = getLayer(i);
		if (layer)
		{	
			layer->setHasMorph(newval);
		}
	}
}

/*virtual*/ void LLTexLayerTemplate::deleteCaches()
{
	U32 num_wearables = updateWearableCache();
	for (U32 i = 0; i < num_wearables; i++)
	{
		LLTexLayer *layer = getLayer(i);
		if (layer)
		{
			layer->deleteCaches();
		}
	}
}

/*virtual*/ BOOL LLTexLayerTemplate::isInvisibleAlphaMask() const
{
	U32 num_wearables = updateWearableCache();
	for (U32 i = 0; i < num_wearables; i++)
	{
		LLTexLayer *layer = getLayer(i);
		if (layer)
		{
			 if (layer->isInvisibleAlphaMask())
			 {
				 return TRUE;
			 }
		}
	}

	return FALSE;
}


//-----------------------------------------------------------------------------
// finds a specific layer based on a passed in name
//-----------------------------------------------------------------------------
LLTexLayerInterface*  LLTexLayerSet::findLayerByName(const std::string& name)
{
	for (layer_list_t::iterator iter = mLayerList.begin(); iter != mLayerList.end(); iter++ )
	{
		LLTexLayerInterface* layer = *iter;
		if (layer->getName() == name)
		{
			return layer;
		}
	}
	for (layer_list_t::iterator iter = mMaskLayerList.begin(); iter != mMaskLayerList.end(); iter++ )
	{
		LLTexLayerInterface* layer = *iter;
		if (layer->getName() == name)
		{
			return layer;
		}
	}
	return NULL;
}

void LLTexLayerSet::cloneTemplates(LLLocalTextureObject *lto, LLVOAvatarDefines::ETextureIndex tex_index, LLWearable *wearable)
{
	// initialize all texlayers with this texture type for this LTO
	for( LLTexLayerSet::layer_list_t::iterator iter = mLayerList.begin(); iter != mLayerList.end(); iter++ )
	{
		LLTexLayerTemplate* layer = (LLTexLayerTemplate*)*iter;
		if (layer->getInfo()->getLocalTexture() == (S32) tex_index)
		{
			lto->addTexLayer(layer, wearable);
		}
	}
	for( LLTexLayerSet::layer_list_t::iterator iter = mMaskLayerList.begin(); iter != mMaskLayerList.end(); iter++ )
	{
		LLTexLayerTemplate* layer = (LLTexLayerTemplate*)*iter;
		if (layer->getInfo()->getLocalTexture() == (S32) tex_index)
		{
			lto->addTexLayer(layer, wearable);
		}
	}
}
//-----------------------------------------------------------------------------
// LLTexLayerStaticImageList
//-----------------------------------------------------------------------------

LLTexLayerStaticImageList::LLTexLayerStaticImageList() :
	mGLBytes(0),
	mTGABytes(0),
	mImageNames(16384)
{
}

LLTexLayerStaticImageList::~LLTexLayerStaticImageList()
{
	deleteCachedImages();
}

void LLTexLayerStaticImageList::dumpByteCount() const
{
	llinfos << "Avatar Static Textures " <<
		"KB GL:" << (mGLBytes / 1024) <<
		"KB TGA:" << (mTGABytes / 1024) << "KB" << llendl;
}

void LLTexLayerStaticImageList::deleteCachedImages()
{
	if( mGLBytes || mTGABytes )
	{
		llinfos << "Clearing Static Textures " <<
			"KB GL:" << (mGLBytes / 1024) <<
			"KB TGA:" << (mTGABytes / 1024) << "KB" << llendl;

		//mStaticImageLists uses LLPointers, clear() will cause deletion
		
		mStaticImageListTGA.clear();
		mStaticImageList.clear();
		
		mGLBytes = 0;
		mTGABytes = 0;
	}
}

// Note: in general, for a given image image we'll call either getImageTga() or getTexture().
// We call getImageTga() if the image is used as an alpha gradient.
// Otherwise, we call getTexture()

// Returns an LLImageTGA that contains the encoded data from a tga file named file_name.
// Caches the result to speed identical subsequent requests.
LLImageTGA* LLTexLayerStaticImageList::getImageTGA(const std::string& file_name)
{
	const char *namekey = mImageNames.addString(file_name);
	image_tga_map_t::const_iterator iter = mStaticImageListTGA.find(namekey);
	if( iter != mStaticImageListTGA.end() )
	{
		return iter->second;
	}
	else
	{
		std::string path;
		path = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,file_name);
		LLPointer<LLImageTGA> image_tga = new LLImageTGA( path );
		if( image_tga->getDataSize() > 0 )
		{
			mStaticImageListTGA[ namekey ] = image_tga;
			mTGABytes += image_tga->getDataSize();
			return image_tga;
		}
		else
		{
			return NULL;
		}
	}
}

// Returns a GL Image (without a backing ImageRaw) that contains the decoded data from a tga file named file_name.
// Caches the result to speed identical subsequent requests.
LLViewerTexture* LLTexLayerStaticImageList::getTexture(const std::string& file_name, BOOL is_mask)
{
	LLPointer<LLViewerTexture> tex;
	const char *namekey = mImageNames.addString(file_name);

	texture_map_t::const_iterator iter = mStaticImageList.find(namekey);
	if( iter != mStaticImageList.end() )
	{
		tex = iter->second;
	}
	else
	{
		tex = LLViewerTextureManager::getLocalTexture( FALSE );
		LLPointer<LLImageRaw> image_raw = new LLImageRaw;
		if( loadImageRaw( file_name, image_raw ) )
		{
			if( (image_raw->getComponents() == 1) && is_mask )
			{
				// Note: these are static, unchanging images so it's ok to assume
				// that once an image is a mask it's always a mask.
				tex->setExplicitFormat( GL_ALPHA8, GL_ALPHA );
			}
			tex->createGLTexture(0, image_raw, 0, TRUE, LLViewerTexture::LOCAL);

			gGL.getTexUnit(0)->bind(tex);
			tex->setAddressMode(LLTexUnit::TAM_CLAMP);

			mStaticImageList [ namekey ] = tex;
			mGLBytes += (S32)tex->getWidth() * tex->getHeight() * tex->getComponents();
		}
		else
		{
			tex = NULL;
		}
	}

	return tex;
}

// Reads a .tga file, decodes it, and puts the decoded data in image_raw.
// Returns TRUE if successful.
BOOL LLTexLayerStaticImageList::loadImageRaw(const std::string& file_name, LLImageRaw* image_raw)
{
	BOOL success = FALSE;
	std::string path;
	path = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,file_name);
	LLPointer<LLImageTGA> image_tga = new LLImageTGA( path );
	if( image_tga->getDataSize() > 0 )
	{
		// Copy data from tga to raw.
		success = image_tga->decode( image_raw );
	}

	return success;
}

const std::string LLTexLayerSetBuffer::dumpTextureInfo() const
{
	if (!isAgentAvatarValid()) return "";

	const BOOL is_high_res = !mNeedsUpload;
	const U32 num_low_res = mNumLowresUploads;
	const U32 upload_time = (U32)mNeedsUploadTimer.getElapsedTimeF32();
	const std::string local_texture_info = gAgentAvatarp->debugDumpLocalTextureDataInfo(mTexLayerSet);

	std::string status 				= "CREATING ";
	if (!uploadNeeded()) status 	= "DONE     ";
	if (uploadInProgress()) status 	= "UPLOADING";

	std::string text = llformat("[%s] [HiRes:%d LoRes:%d] [Elapsed:%d] %s",
								status.c_str(),
								is_high_res, num_low_res,
								upload_time, 
								local_texture_info.c_str());
	return text;
}
