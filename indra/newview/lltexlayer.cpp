/** 
 * @file lltexlayer.cpp
 * @brief A texture layer. Used for avatars.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llagent.h"
#include "lltexlayer.h"
#include "llviewerstats.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "pipeline.h"
#include "llassetuploadresponders.h"
#include "lltexlayerparams.h"
#include "llui.h"

//#include "../tools/imdebug/imdebug.h"

using namespace LLVOAvatarDefines;

//-----------------------------------------------------------------------------
// LLBakedUploadData()
//-----------------------------------------------------------------------------
LLBakedUploadData::LLBakedUploadData(const LLVOAvatarSelf* avatar,
									 LLTexLayerSet* layerset,
									 const LLUUID& id) : 
	mAvatar(avatar),
	mTexLayerSet(layerset),
	mID(id),
	mStartTime(LLFrameTimer::getTotalTime())		// Record starting time
{ 
}

//-----------------------------------------------------------------------------
// LLTexLayerSetBuffer
// The composite image that a LLTexLayerSet writes to.  Each LLTexLayerSet has one.
//-----------------------------------------------------------------------------

// static
S32 LLTexLayerSetBuffer::sGLByteCount = 0;
S32 LLTexLayerSetBuffer::sGLBumpByteCount = 0;

LLTexLayerSetBuffer::LLTexLayerSetBuffer(LLTexLayerSet* const owner, 
										 S32 width, S32 height, 
										 BOOL has_bump) :
	// ORDER_LAST => must render these after the hints are created.
	LLViewerDynamicTexture( width, height, 4, LLViewerDynamicTexture::ORDER_LAST, TRUE ), 
	mNeedsUpdate( TRUE ),
	mNeedsUpload( FALSE ),
	mUploadPending( FALSE ), // Not used for any logic here, just to sync sending of updates
	mTexLayerSet(owner),
	mHasBump(has_bump),
	mBumpTex(NULL)
{
	LLTexLayerSetBuffer::sGLByteCount += getSize();
	createBumpTexture() ;
}

LLTexLayerSetBuffer::~LLTexLayerSetBuffer()
{
	LLTexLayerSetBuffer::sGLByteCount -= getSize();

	if( mBumpTex.notNull())
	{
		mBumpTex = NULL ;
		LLImageGL::sGlobalTextureMemoryInBytes -= mFullWidth * mFullHeight * 4;
		LLTexLayerSetBuffer::sGLBumpByteCount -= mFullWidth * mFullHeight * 4;
	}
}

//virtual 
void LLTexLayerSetBuffer::restoreGLTexture() 
{	
	createBumpTexture() ;
	LLViewerDynamicTexture::restoreGLTexture() ;
}

//virtual 
void LLTexLayerSetBuffer::destroyGLTexture() 
{
	if( mBumpTex.notNull() )
	{
		mBumpTex = NULL ;
		LLImageGL::sGlobalTextureMemoryInBytes -= mFullWidth * mFullHeight * 4;
		LLTexLayerSetBuffer::sGLBumpByteCount -= mFullWidth * mFullHeight * 4;
	}

	LLViewerDynamicTexture::destroyGLTexture() ;
}

void LLTexLayerSetBuffer::createBumpTexture()
{
	if( mHasBump )
	{
		LLGLSUIDefault gls_ui;
		mBumpTex = LLViewerTextureManager::getLocalTexture(FALSE) ;
		if(!mBumpTex->createGLTexture())
		{
			mBumpTex = NULL ;
			return ;
		}

		gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, mBumpTex->getTexName());
		stop_glerror();

		gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);

		LLImageGL::setManualImage(GL_TEXTURE_2D, 0, GL_RGBA8, mFullWidth, mFullHeight, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		stop_glerror();

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		LLImageGL::sGlobalTextureMemoryInBytes += mFullWidth * mFullHeight * 4;
		LLTexLayerSetBuffer::sGLBumpByteCount += mFullWidth * mFullHeight * 4;
	}
}

// static
void LLTexLayerSetBuffer::dumpTotalByteCount()
{
	llinfos << "Composite System GL Buffers: " << (LLTexLayerSetBuffer::sGLByteCount/1024) << "KB" << llendl;
	llinfos << "Composite System GL Bump Buffers: " << (LLTexLayerSetBuffer::sGLBumpByteCount/1024) << "KB" << llendl;
}

void LLTexLayerSetBuffer::requestUpdate()
{
	mNeedsUpdate = TRUE;

	// If we're in the middle of uploading a baked texture, we don't care about it any more.
	// When it's downloaded, ignore it.
	mUploadID.setNull();
}

void LLTexLayerSetBuffer::requestUpload()
{
	if (!mNeedsUpload)
	{
		mNeedsUpload = TRUE;
		mUploadPending = TRUE;
	}
}

void LLTexLayerSetBuffer::cancelUpload()
{
	if (mNeedsUpload)
	{
		mNeedsUpload = FALSE;
	}
	mUploadPending = FALSE;
}

void LLTexLayerSetBuffer::pushProjection() const
{
	glMatrixMode(GL_PROJECTION);
	gGL.pushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, mFullWidth, 0.0f, mFullHeight, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	gGL.pushMatrix();
	glLoadIdentity();
}

void LLTexLayerSetBuffer::popProjection() const
{
	glMatrixMode(GL_PROJECTION);
	gGL.popMatrix();

	glMatrixMode(GL_MODELVIEW);
	gGL.popMatrix();
}

BOOL LLTexLayerSetBuffer::needsRender()
{
	const LLVOAvatarSelf* avatar = mTexLayerSet->getAvatar();
	BOOL upload_now = mNeedsUpload && mTexLayerSet->isLocalTextureDataFinal();
	BOOL needs_update = gAgentQueryManager.hasNoPendingQueries() && (mNeedsUpdate || upload_now) && !avatar->mAppearanceAnimating;
	if (needs_update)
	{
		BOOL invalid_skirt = avatar->getBakedTE(mTexLayerSet) == LLVOAvatarDefines::TEX_SKIRT_BAKED && !avatar->isWearingWearableType(WT_SKIRT);
		if (invalid_skirt)
		{
			// we were trying to create a skirt texture
			// but we're no longer wearing a skirt...
			needs_update = FALSE;
			cancelUpload();
		}
		else
		{
			needs_update &= (avatar->isSelf() || (avatar->isVisible() && !avatar->isCulled()));
			needs_update &= mTexLayerSet->isLocalTextureDataAvailable();
		}
	}
	return needs_update;
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
	U8* baked_bump_data = NULL;

	// Default color mask for tex layer render
	gGL.setColorMask(true, true);

	// do we need to upload, and do we have sufficient data to create an uploadable composite?
	// When do we upload the texture if gAgent.mNumPendingQueries is non-zero?
	BOOL upload_now = (gAgentQueryManager.hasNoPendingQueries() && mNeedsUpload && mTexLayerSet->isLocalTextureDataFinal());
	BOOL success = TRUE;

	// Composite bump
	if( mBumpTex.notNull() )
	{
		// Composite the bump data
		success &= mTexLayerSet->renderBump( mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight );
		stop_glerror();

		if (success)
		{
			LLGLSUIDefault gls_ui;

			// read back into texture (this is done externally for the color data)
			gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, mBumpTex->getTexName());
			stop_glerror();

			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight);
			stop_glerror();

			// if we need to upload the data, read it back into a buffer
			if( upload_now )
			{
				baked_bump_data = new U8[ mFullWidth * mFullHeight * 4 ];
				glReadPixels(mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight, GL_RGBA, GL_UNSIGNED_BYTE, baked_bump_data );
				stop_glerror();
			}
		}
	}

	// Composite the color data
	LLGLSUIDefault gls_ui;
	success &= mTexLayerSet->render( mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight );
	gGL.flush();

	if( upload_now )
	{
		if (!success)
		{
			llinfos << "Failed attempt to bake " << mTexLayerSet->getBodyRegion() << llendl;
			mUploadPending = FALSE;
		}
		else
		{
			readBackAndUpload(baked_bump_data);
		}
	}

	// reset GL state
	gGL.setColorMask(true, true);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	// we have valid texture data now
	mGLTexturep->setGLTextureCreated(true);
	mNeedsUpdate = FALSE;

	delete [] baked_bump_data;
	return success;
}

bool LLTexLayerSetBuffer::isInitialized(void) const
{
	return mGLTexturep.notNull() && mGLTexturep->isGLTextureCreated();
}

BOOL LLTexLayerSetBuffer::updateImmediate()
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

void LLTexLayerSetBuffer::readBackAndUpload(const U8* baked_bump_data)
{
	// pointers for storing data to upload
	U8* baked_color_data = new U8[ mFullWidth * mFullHeight * 4 ];
	
	glReadPixels(mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight, GL_RGBA, GL_UNSIGNED_BYTE, baked_color_data );
	stop_glerror();

	llinfos << "Baked " << mTexLayerSet->getBodyRegion() << llendl;
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_TEX_BAKES);

	llassert( gAgent.getAvatarObject() == mTexLayerSet->getAvatar() );

	// We won't need our caches since we're baked now.  (Techically, we won't 
	// really be baked until this image is sent to the server and the Avatar
	// Appearance message is received.)
	mTexLayerSet->deleteCaches();

	LLGLSUIDefault gls_ui;

	LLPointer<LLImageRaw> baked_mask_image = new LLImageRaw(mFullWidth, mFullHeight, 1 );
	U8* baked_mask_data = baked_mask_image->getData(); 
	
	mTexLayerSet->gatherMorphMaskAlpha(baked_mask_data, mFullWidth, mFullHeight);

	// writes into baked_color_data
	const char* comment_text = NULL;

	S32 baked_image_components = mBumpTex.notNull() ? 5 : 4; // red green blue [bump] clothing
	LLPointer<LLImageRaw> baked_image = new LLImageRaw( mFullWidth, mFullHeight, baked_image_components );
	U8* baked_image_data = baked_image->getData();
	
	
	if( mBumpTex.notNull() )
	{
		comment_text = LINDEN_J2C_COMMENT_PREFIX "RGBHM"; // 5 channels: rgb, heightfield/alpha, mask

			S32 i = 0;
			for( S32 u = 0; u < mFullWidth; u++ )
			{
				for( S32 v = 0; v < mFullHeight; v++ )
				{
					baked_image_data[5*i + 0] = baked_color_data[4*i + 0];
					baked_image_data[5*i + 1] = baked_color_data[4*i + 1];
					baked_image_data[5*i + 2] = baked_color_data[4*i + 2];
					baked_image_data[5*i + 3] = baked_color_data[4*i + 3]; // alpha should be correct for eyelashes.
					baked_image_data[5*i + 4] = baked_mask_data[i];
					i++;
				}
			}
		}
		else
		{
			S32 i = 0;
			for( S32 u = 0; u < mFullWidth; u++ )
			{
				for( S32 v = 0; v < mFullHeight; v++ )
				{
					baked_image_data[4*i + 0] = baked_color_data[4*i + 0];
					baked_image_data[4*i + 1] = baked_color_data[4*i + 1];
					baked_image_data[4*i + 2] = baked_color_data[4*i + 2];
					baked_image_data[4*i + 3] = baked_color_data[4*i + 3];  // Use alpha, not bump
					i++;
				}
			}
		}
	
	LLPointer<LLImageJ2C> compressedImage = new LLImageJ2C;
	compressedImage->setRate(0.f);
	LLTransactionID tid;
	LLAssetID asset_id;
	tid.generate();
	asset_id = tid.makeAssetID(gAgent.getSecureSessionID());

	BOOL res = false;
	if( compressedImage->encode(baked_image, comment_text))
	{
		res = LLVFile::writeFile(compressedImage->getData(), compressedImage->getDataSize(),
								 gVFS, asset_id, LLAssetType::AT_TEXTURE);
		if (res)
		{
			LLPointer<LLImageJ2C> integrity_test = new LLImageJ2C;
			BOOL valid = FALSE;
			S32 file_size;
			U8* data = LLVFile::readFile(gVFS, asset_id, LLAssetType::AT_TEXTURE, &file_size);
			if (data)
			{
				valid = integrity_test->validate(data, file_size); // integrity_test will delete 'data'
			}
			else
			{
				integrity_test->setLastError("Unable to read entire file");
			}
			
			if( valid )
			{
				// baked_upload_data is owned by the responder and deleted after the request completes
				LLBakedUploadData* baked_upload_data =
					new LLBakedUploadData(gAgent.getAvatarObject(), this->mTexLayerSet, asset_id);
				mUploadID = asset_id;
				
				// upload the image
				std::string url = gAgent.getRegion()->getCapability("UploadBakedTexture");

				if(!url.empty()
					&& !LLPipeline::sForceOldBakedUpload) // Toggle the debug setting UploadBakedTexOld to change between the new caps method and old method
				{
					llinfos << "Baked texture upload via capability of " << mUploadID << " to " << url << llendl;

					LLSD body = LLSD::emptyMap();
					LLHTTPClient::post(url, body, new LLSendTexLayerResponder(body, mUploadID, LLAssetType::AT_TEXTURE, baked_upload_data));
					// Responder will call LLTexLayerSetBuffer::onTextureUploadComplete()
				} 
				else
				{
					llinfos << "Baked texture upload via Asset Store." <<  llendl;
					// gAssetStorage->storeAssetData(mTransactionID, LLAssetType::AT_IMAGE_JPEG, &uploadCallback, (void *)this, FALSE);
					gAssetStorage->storeAssetData(tid,
												  LLAssetType::AT_TEXTURE,
												  LLTexLayerSetBuffer::onTextureUploadComplete,
												  baked_upload_data,
												  TRUE,		// temp_file
												  TRUE,		// is_priority
												  TRUE);	// store_local
				}
		
				mNeedsUpload = FALSE;
			}
			else
			{
				mUploadPending = FALSE;
				llinfos << "unable to create baked upload file: corrupted" << llendl;
				LLVFile file(gVFS, asset_id, LLAssetType::AT_TEXTURE, LLVFile::WRITE);
				file.remove();
			}
		}
	}
	if (!res)
	{
		mUploadPending = FALSE;
		llinfos << "unable to create baked upload file" << llendl;
	}

	delete [] baked_color_data;
}


// static
void LLTexLayerSetBuffer::onTextureUploadComplete(const LLUUID& uuid,
												  void* userdata,
												  S32 result,
												  LLExtStat ext_status) // StoreAssetData callback (not fixed)
{
	LLBakedUploadData* baked_upload_data = (LLBakedUploadData*)userdata;

	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();

	if (0 == result &&
		avatar &&
		!avatar->isDead() &&
		baked_upload_data->mAvatar == avatar && // Sanity check: only the user's avatar should be uploading textures.
		baked_upload_data->mTexLayerSet->hasComposite()
		)
	{
		LLTexLayerSetBuffer* layerset_buffer = baked_upload_data->mTexLayerSet->getComposite();
			
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
			layerset_buffer->mUploadPending = FALSE;

			if (result >= 0)
			{
				LLVOAvatarDefines::ETextureIndex baked_te = avatar->getBakedTE(layerset_buffer->mTexLayerSet);
				// Update baked texture info with the new UUID
				U64 now = LLFrameTimer::getTotalTime();		// Record starting time
				llinfos << "Baked texture upload took " << (S32)((now - baked_upload_data->mStartTime) / 1000) << " ms" << llendl;
				avatar->setNewBakedTexture(baked_te, uuid);
			}
			else
			{	
				// Avatar appearance is changing, ignore the upload results
				llinfos << "Baked upload failed. Reason: " << result << llendl;
				// *FIX: retry upload after n seconds, asset server could be busy
			}
		}
		else
		{
			llinfos << "Received baked texture out of date, ignored." << llendl;
		}

		avatar->dirtyMesh();
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

void LLTexLayerSetBuffer::bindBumpTexture( U32 stage )
{
	if( mBumpTex.notNull() ) 
	{
		gGL.getTexUnit(stage)->bindManual(LLTexUnit::TT_TEXTURE, mBumpTex->getTexName());
		gGL.getTexUnit(0)->activate();
	
		mGLTexturep->updateBindStats(mFullWidth * mFullHeight * 4);
	}
	else
	{
		gGL.getTexUnit(stage)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.getTexUnit(0)->activate();
	}
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
	mHasBump( FALSE ),
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
		LLTexLayer* layer = new LLTexLayer( this );
		if (!layer->setInfo(*iter))
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
		LLTexLayer* layer = *iter;
		layer->deleteCaches();
	}
	for (layer_list_t::iterator iter = mMaskLayerList.begin(); iter != mMaskLayerList.end(); iter++)
	{
		LLTexLayer* layer = *iter;
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

	LLGLSUIDefault gls_ui;
	LLGLDepthTest gls_depth(GL_FALSE, GL_FALSE);
	gGL.setColorMask(true, true);

	BOOL render_morph = mAvatar->morphMaskNeedsUpdate(mBakedTexIndex);

	// composite color layers
	for( layer_list_t::iterator iter = mLayerList.begin(); iter != mLayerList.end(); iter++ )
	{
		LLTexLayer* layer = *iter;
		if (layer->getRenderPass() == LLTexLayer::RP_COLOR)
		{
			gGL.flush();
			success &= layer->render(x, y, width, height, render_morph);
			gGL.flush();
			if (layer->isMorphValid())
			{
				mAvatar->setMorphMasksValid(TRUE, mBakedTexIndex);
			}
		}
	}
	
	renderAlphaMaskTextures(width, height, false);

	stop_glerror();

	return success;
}

BOOL LLTexLayerSet::renderBump( S32 x, S32 y, S32 width, S32 height )
{
	BOOL success = TRUE;

	LLGLSUIDefault gls_ui;
	LLGLDepthTest gls_depth(GL_FALSE, GL_FALSE);

	//static S32 bump_layer_count = 1;

	for( layer_list_t::iterator iter = mLayerList.begin(); iter != mLayerList.end(); iter++ )
	{
		LLTexLayer* layer = *iter;
		if (layer->getRenderPass() == LLTexLayer::RP_BUMP)
		{
//			success &= layer->render(x, y, width, height);
		}
	}

	// Set the alpha channel to one (clean up after previous blending)
	LLGLDisable no_alpha(GL_ALPHA_TEST);
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.color4f( 0.f, 0.f, 0.f, 1.f );
	gGL.setColorMask(false, true);

	gl_rect_2d_simple( width, height );
	
	gGL.setColorMask(true, true);
	stop_glerror();

	return success;
}

BOOL LLTexLayerSet::isBodyRegion(const std::string& region) const 
{ 
	return mInfo->mBodyRegion == region; 
}

const std::string LLTexLayerSet::getBodyRegion() const 
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
	if( !mComposite )
	{
		S32 width = mInfo->mWidth;
		S32 height = mInfo->mHeight;
		// Composite other avatars at reduced resolution
		if( !mAvatar->isSelf() )
		{
			// TODO: replace with sanity check to ensure not called for non-self avatars
//			width /= 2;
//			height /= 2;
		}
		mComposite = new LLTexLayerSetBuffer( this, width, height, mHasBump );
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
	mComposite->updateImmediate();
}

LLTexLayerSetBuffer* LLTexLayerSet::getComposite()
{
	createComposite();
	return mComposite;
}

void LLTexLayerSet::gatherMorphMaskAlpha(U8 *data, S32 width, S32 height)
{
	S32 size = width * height;

	memset(data, 255, width * height);

	BOOL render_morph = mAvatar->morphMaskNeedsUpdate(mBakedTexIndex);

	for( layer_list_t::iterator iter = mLayerList.begin(); iter != mLayerList.end(); iter++ )
	{
		LLTexLayer* layer = *iter;
		U8* alphaData = layer->getAlphaData();
		if (!alphaData && layer->hasAlphaParams())
		{
			LLColor4 net_color;
			layer->findNetColor( &net_color );
			// TODO: eliminate need for layer morph mask valid flag
			layer->invalidateMorphMasks();
			mAvatar->invalidateMorphMasks(mBakedTexIndex);
			layer->renderMorphMasks(mComposite->getOriginX(), mComposite->getOriginY(), width, height, net_color, render_morph);
			alphaData = layer->getAlphaData();
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
	
	// Set alpha back to that of our alpha masks.
	renderAlphaMaskTextures(width, height, true);
}

void LLTexLayerSet::renderAlphaMaskTextures(S32 width, S32 height, bool forceClear)
{
	const LLTexLayerSetInfo *info = getInfo();
	
	gGL.setColorMask(false, true);
	gGL.setSceneBlendType(LLRender::BT_REPLACE);
	// (Optionally) replace alpha with a single component image from a tga file.
	if (!info->mStaticAlphaFileName.empty() && mMaskLayerList.empty())
	{
		LLGLSNoAlphaTest gls_no_alpha_test;
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
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f( 0.f, 0.f, 0.f, 1.f );
		
		gl_rect_2d_simple( width, height );
		
		gGL.flush();
	}
	
	// (Optional) Mask out part of the baked texture with alpha masks
	// will still have an effect even if mClearAlpha is set or the alpha component was replaced
	if (mMaskLayerList.size() > 0)
	{
		gGL.setSceneBlendType(LLRender::BT_MULT_ALPHA);
		gGL.getTexUnit(0)->setTextureBlendType( LLTexUnit::TB_REPLACE );
		for (layer_list_t::iterator iter = mMaskLayerList.begin(); iter != mMaskLayerList.end(); iter++)
		{
			LLTexLayer* layer = *iter;
			gGL.flush();
			layer->blendAlphaTexture(width, height);
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

//-----------------------------------------------------------------------------
// finds a specific layer based on a passed in name
//-----------------------------------------------------------------------------
LLTexLayer*  LLTexLayerSet::findLayerByName(std::string name)
{
	for( layer_list_t::iterator iter = mLayerList.begin(); iter != mLayerList.end(); iter++ )
	{
		LLTexLayer* layer = *iter;

		if (layer->getName().compare(name) == 0)
		{
			return layer;
		}
	}
	return NULL;
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
				llwarns << "<texture> element has invalid local_texure attribute: " << mName << " " << local_texture_name << llendl;
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
		if (!param_color->setInfo(color_info))
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
		if (!param_alpha->setInfo(alpha_info))
		{
			llwarns << "NULL TexLayer Alpha Param could not be added to visual param list. Deleting." << llendl;
			delete param_alpha;
			success = FALSE;
		}
	}

	return success;
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
LLTexLayer::LLTexLayer(LLTexLayerSet* layer_set) :
	mTexLayerSet( layer_set ),
	mMorphMasksValid( FALSE ),
	mStaticImageInvalid( FALSE ),
	mInfo(NULL),
	mHasMorph(FALSE)
{
}

LLTexLayer::LLTexLayer(const LLTexLayer &layer) :
	mTexLayerSet( layer.mTexLayerSet )
{
	setInfo(layer.getInfo());

	
	mHasMorph = layer.mHasMorph;

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

BOOL LLTexLayer::setInfo(const LLTexLayerInfo* info)
{
	llassert(mInfo == NULL);
	mInfo = info;
	//mID = info->mID; // No ID

	if (info->mRenderPass == LLTexLayer::RP_BUMP)
		mTexLayerSet->setBump(TRUE);

		mParamColorList.reserve(mInfo->mParamColorInfoList.size());
	for (param_color_info_list_t::const_iterator iter = mInfo->mParamColorInfoList.begin(); 
		 iter != mInfo->mParamColorInfoList.end(); 
		 iter++)
	{
			LLTexLayerParamColor* param_color = new LLTexLayerParamColor(this);
			if (!param_color->setInfo(*iter))
			{
				mInfo = NULL;
				return FALSE;
			}
			mParamColorList.push_back( param_color );
		}

		mParamAlphaList.reserve(mInfo->mParamAlphaInfoList.size());
	for (param_alpha_info_list_t::const_iterator iter = mInfo->mParamAlphaInfoList.begin(); 
		 iter != mInfo->mParamAlphaInfoList.end(); 
		 iter++)
		{
			LLTexLayerParamAlpha* param_alpha = new LLTexLayerParamAlpha( this );
			if (!param_alpha->setInfo(*iter))
			{
				mInfo = NULL;
				return FALSE;
			}
			mParamAlphaList.push_back( param_alpha );
		}
	
	return TRUE;
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

void LLTexLayer::deleteCaches()
{
	for (param_alpha_list_t::iterator iter = mParamAlphaList.begin();
		 iter != mParamAlphaList.end(); iter++ )
	{
		LLTexLayerParamAlpha* param = *iter;
		param->deleteCaches();
	}
}

BOOL LLTexLayer::render(S32 x, S32 y, S32 width, S32 height, BOOL render_morph)
{
	LLGLEnable color_mat(GL_COLOR_MATERIAL);
	gPipeline.disableLights();

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
		if (!mHasMorph)
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
		}

		renderMorphMasks(x, y, width, height, net_color, render_morph);
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
			if( mTexLayerSet->getAvatar()->getLocalTextureGL((ETextureIndex)getInfo()->mLocalTexture, &tex ) )
			{
				if( tex )
				{
					LLGLDisable alpha_test(getInfo()->mWriteAllChannels ? GL_ALPHA_TEST : 0);

					LLTexUnit::eTextureAddressMode old_mode = tex->getAddressMode();
					
					gGL.getTexUnit(0)->bind(tex);
					gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

					gl_rect_2d_simple_tex( width, height );

					gGL.getTexUnit(0)->setTextureAddressMode(old_mode);
					gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				}
			}
			else
			{
				success = FALSE;
			}
		}
	}

	if( !getInfo()->mStaticImageFileName.empty() )
	{
		{
			LLViewerTexture* tex = LLTexLayerStaticImageList::getInstance()->getTexture(getInfo()->mStaticImageFileName, getInfo()->mStaticImageIsMask);
			if( tex )
			{
				gGL.getTexUnit(0)->bind(tex);
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
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4fv( net_color.mV );
		gl_rect_2d_simple( width, height );
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

U8*	LLTexLayer::getAlphaData()
{
	LLCRC alpha_mask_crc;
	const LLUUID& uuid = mTexLayerSet->getAvatar()->getLocalTextureID((ETextureIndex)getInfo()->mLocalTexture);
	alpha_mask_crc.update((U8*)(&uuid.mData), UUID_BYTES);

	for (param_alpha_list_t::const_iterator iter = mParamAlphaList.begin(); iter != mParamAlphaList.end(); iter++)
	{
		const LLTexLayerParamAlpha* param = *iter;
		F32 param_weight = param->getWeight();
		alpha_mask_crc.update((U8*)&param_weight, sizeof(F32));
	}

	U32 cache_index = alpha_mask_crc.getCRC();

	alpha_cache_t::iterator iter2 = mAlphaCache.find(cache_index);
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

BOOL LLTexLayer::blendAlphaTexture(S32 width, S32 height)
{
	BOOL success = TRUE;

	gGL.flush();
	
	if( !getInfo()->mStaticImageFileName.empty() )
	{
		LLViewerTexture* tex = LLTexLayerStaticImageList::getInstance()->getTexture( getInfo()->mStaticImageFileName, getInfo()->mStaticImageIsMask );
		if( tex )
		{
			LLGLSNoAlphaTest gls_no_alpha_test;
			gGL.getTexUnit(0)->bind(tex);
			gl_rect_2d_simple_tex( width, height );
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
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
			LLViewerTexture* tex = NULL;
			if (mTexLayerSet->getAvatar()->getLocalTextureGL((ETextureIndex)getInfo()->mLocalTexture, &tex))
			{
				if (tex)
				{
					LLGLSNoAlphaTest gls_no_alpha_test;
					gGL.getTexUnit(0)->bind(tex);
					gl_rect_2d_simple_tex( width, height );
					gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
					success = TRUE;
				}
			}
		}
	}
	
	return success;
}


BOOL LLTexLayer::renderMorphMasks(S32 x, S32 y, S32 width, S32 height, const LLColor4 &layer_color, BOOL render_morph)
{
	BOOL success = TRUE;

	llassert( !mParamAlphaList.empty() );

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
			LLViewerTexture* tex = NULL;
			if( mTexLayerSet->getAvatar()->getLocalTextureGL((ETextureIndex)getInfo()->mLocalTexture, &tex ) )
			{
				if( tex && (tex->getComponents() == 4) )
				{
					LLGLSNoAlphaTest gls_no_alpha_test;

					LLTexUnit::eTextureAddressMode old_mode = tex->getAddressMode();
					
					gGL.getTexUnit(0)->bind(tex);
					gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

					gl_rect_2d_simple_tex( width, height );

					gGL.getTexUnit(0)->setTextureAddressMode(old_mode);
					gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				}
			}
			else
			{
				success = FALSE;
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
					gGL.getTexUnit(0)->bind(tex);
					gl_rect_2d_simple_tex( width, height );
					gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				}
			}
			else
			{
				success = FALSE;
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


	LLGLSUIDefault gls_ui;

	gGL.setColorMask(true, true);
	
	if (render_morph && mHasMorph)
	{
		LLCRC alpha_mask_crc;
		const LLUUID& uuid = mTexLayerSet->getAvatar()->getLocalTextureID((ETextureIndex)getInfo()->mLocalTexture);
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

// Returns TRUE on success.
BOOL LLTexLayer::renderImageRaw( U8* in_data, S32 in_width, S32 in_height, S32 in_components, S32 width, S32 height, BOOL is_mask )
{
	if (!in_data)
	{
		return FALSE;
	}
	GLenum format_options[4] = { GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
	GLenum format = format_options[in_components-1];
	if( is_mask )
	{
		llassert( 1 == in_components );
		format = GL_ALPHA;
	}

	if( (in_width != SCRATCH_TEX_WIDTH) || (in_height != SCRATCH_TEX_HEIGHT) )
	{
		LLGLSNoAlphaTest gls_no_alpha_test;

		GLenum internal_format_options[4] = { GL_LUMINANCE8, GL_LUMINANCE8_ALPHA8, GL_RGB8, GL_RGBA8 };
		GLenum internal_format = internal_format_options[in_components-1];
		if( is_mask )
		{
			llassert( 1 == in_components );
			internal_format = GL_ALPHA8;
		}
		
		U32 name = 0;
		LLImageGL::generateTextures(1, &name );
		stop_glerror();

		gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, name);
		stop_glerror();

		LLImageGL::setManualImage(
			GL_TEXTURE_2D, 0, internal_format, 
			in_width, in_height,
			format, GL_UNSIGNED_BYTE, in_data );
		stop_glerror();

		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
		
		gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

		gl_rect_2d_simple_tex( width, height );

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		LLImageGL::deleteTextures(1, &name );
		stop_glerror();
	}
	else
	{
		LLGLSNoAlphaTest gls_no_alpha_test;

		if( !mTexLayerSet->getAvatar()->bindScratchTexture(format) )
		{
			return FALSE;
		}

		glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, in_width, in_height, format, GL_UNSIGNED_BYTE, in_data );
		stop_glerror();

		gl_rect_2d_simple_tex( width, height );

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	}

	return TRUE;
}

void LLTexLayer::requestUpdate()
{
	mTexLayerSet->requestUpdate();
}

const std::string& LLTexLayer::getName() const 
{ 
	return mInfo->mName; 
}

LLTexLayer::ERenderPass	LLTexLayer::getRenderPass() const 
{
	return mInfo->mRenderPass; 
}
const std::string& LLTexLayer::getGlobalColor() const 
{
	return mInfo->mGlobalColor; 
}

void LLTexLayer::invalidateMorphMasks()
{
	mMorphMasksValid = FALSE;
	}

BOOL LLTexLayer::isVisibilityMask() const 
	{
	return mInfo->mIsVisibilityMask;
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

void LLTexLayerStaticImageList::dumpByteCount()
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
			tex->createGLTexture(0, image_raw);

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


