/** 
 * @file llviewertexlayer.cpp
 * @brief Viewer texture layer. Used for avatars.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#include "llviewertexlayer.h"

#include "llagent.h"
#include "llimagej2c.h"
#include "llnotificationsutil.h"
#include "llvfile.h"
#include "llvfs.h"
#include "llviewerregion.h"
#include "llglslshader.h"
#include "llvoavatarself.h"
#include "pipeline.h"
#include "llassetuploadresponders.h"
#include "llviewercontrol.h"

static const S32 BAKE_UPLOAD_ATTEMPTS = 7;
static const F32 BAKE_UPLOAD_RETRY_DELAY = 2.f; // actual delay grows by power of 2 each attempt

// runway consolidate
extern std::string self_av_string();


//-----------------------------------------------------------------------------
// LLBakedUploadData()
//-----------------------------------------------------------------------------
LLBakedUploadData::LLBakedUploadData(const LLVOAvatarSelf* avatar,
									 LLViewerTexLayerSet* layerset,
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
// The composite image that a LLViewerTexLayerSet writes to.  Each LLViewerTexLayerSet has one.
//-----------------------------------------------------------------------------

// static
S32 LLTexLayerSetBuffer::sGLByteCount = 0;

LLTexLayerSetBuffer::LLTexLayerSetBuffer(LLViewerTexLayerSet* const owner, 
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

// virtual
BOOL LLTexLayerSetBuffer::needsRender()
{
	llassert(mTexLayerSet->getAvatarAppearance() == gAgentAvatarp);
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

	// need to switch to using this layerset if this is the first update
	// after getting the lowest LOD
	mTexLayerSet->getAvatar()->updateMeshTextures();
	
	// Print out notification that we updated this texture.
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
// LLViewerTexLayerSet
// An ordered set of texture layers that get composited into a single texture.
//-----------------------------------------------------------------------------

LLViewerTexLayerSet::LLViewerTexLayerSet(LLAvatarAppearance* const appearance) :
	LLTexLayerSet(appearance),
	mComposite( NULL ),
	mUpdatesEnabled( FALSE )
{
}

// virtual
LLViewerTexLayerSet::~LLViewerTexLayerSet()
{
}

// Returns TRUE if at least one packet of data has been received for each of the textures that this layerset depends on.
BOOL LLViewerTexLayerSet::isLocalTextureDataAvailable() const
{
	if (!mAvatarAppearance->isSelf()) return FALSE;
	return getAvatar()->isLocalTextureDataAvailable(this);
}


// Returns TRUE if all of the data for the textures that this layerset depends on have arrived.
BOOL LLViewerTexLayerSet::isLocalTextureDataFinal() const
{
	if (!mAvatarAppearance->isSelf()) return FALSE;
	return getAvatar()->isLocalTextureDataFinal(this);
}

// virtual
void LLViewerTexLayerSet::requestUpdate()
{
	if( mUpdatesEnabled )
	{
		createComposite();
		mComposite->requestUpdate(); 
	}
}

void LLViewerTexLayerSet::requestUpload()
{
	createComposite();
	mComposite->requestUpload();
}

void LLViewerTexLayerSet::cancelUpload()
{
	if(mComposite)
	{
		mComposite->cancelUpload();
	}
}

void LLViewerTexLayerSet::createComposite()
{
	if(!mComposite)
	{
		S32 width = mInfo->getWidth();
		S32 height = mInfo->getHeight();
		// Composite other avatars at reduced resolution
		if( !mAvatarAppearance->isSelf() )
		{
			llerrs << "composites should not be created for non-self avatars!" << llendl;
		}
		mComposite = new LLTexLayerSetBuffer( this, width, height );
	}
}

void LLViewerTexLayerSet::destroyComposite()
{
	if( mComposite )
	{
		mComposite = NULL;
	}
}

void LLViewerTexLayerSet::setUpdatesEnabled( BOOL b )
{
	mUpdatesEnabled = b; 
}


void LLViewerTexLayerSet::updateComposite()
{
	createComposite();
	mComposite->requestUpdateImmediate();
}

LLTexLayerSetBuffer* LLViewerTexLayerSet::getComposite()
{
	if (!mComposite)
	{
		createComposite();
	}
	return mComposite;
}

const LLTexLayerSetBuffer* LLViewerTexLayerSet::getComposite() const
{
	return mComposite;
}

void LLViewerTexLayerSet::gatherMorphMaskAlpha(U8 *data, S32 width, S32 height)
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


LLVOAvatarSelf* LLViewerTexLayerSet::getAvatar() const
{
	return dynamic_cast<LLVOAvatarSelf*> (mAvatarAppearance);
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
