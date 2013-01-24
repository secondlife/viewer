/** 
 * @file llpreviewtexture.cpp
 * @brief LLPreviewTexture class implementation
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

#include "llwindow.h"

#include "llpreviewtexture.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfilepicker.h"
#include "llfloaterreg.h"
#include "llimagetga.h"
#include "llinventory.h"
#include "llnotificationsutil.h"
#include "llresmgr.h"
#include "lltrans.h"
#include "lltextbox.h"
#include "lltextureview.h"
#include "llui.h"
#include "llviewerinventory.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "lllineeditor.h"

const S32 CLIENT_RECT_VPAD = 4;

const F32 SECONDS_TO_SHOW_FILE_SAVED_MSG = 8.f;

const F32 PREVIEW_TEXTURE_MAX_ASPECT = 200.f;
const F32 PREVIEW_TEXTURE_MIN_ASPECT = 0.005f;


LLPreviewTexture::LLPreviewTexture(const LLSD& key)
	: LLPreview(key),
	  mLoadingFullImage( FALSE ),
	  mShowKeepDiscard(FALSE),
	  mCopyToInv(FALSE),
	  mIsCopyable(FALSE),
	  mUpdateDimensions(TRUE),
	  mLastHeight(0),
	  mLastWidth(0),
	  mAspectRatio(0.f),
	  mPreviewToSave(FALSE),
	  mImage(NULL),
	  mImageOldBoostLevel(LLGLTexture::BOOST_NONE)
{
	updateImageID();
	if (key.has("save_as"))
	{
		mPreviewToSave = TRUE;
	}
}

LLPreviewTexture::~LLPreviewTexture()
{
	LLLoadedCallbackEntry::cleanUpCallbackList(&mCallbackTextureList) ;

	if( mLoadingFullImage )
	{
		getWindow()->decBusyCount();
	}
	mImage->setBoostLevel(mImageOldBoostLevel);
	mImage = NULL;
}

// virtual
BOOL LLPreviewTexture::postBuild()
{
	if (mCopyToInv) 
	{
		getChild<LLButton>("Keep")->setLabel(getString("Copy"));
		childSetAction("Keep",LLPreview::onBtnCopyToInv,this);
		getChildView("Discard")->setVisible( false);
	}
	else if (mShowKeepDiscard)
	{
		childSetAction("Keep",onKeepBtn,this);
		childSetAction("Discard",onDiscardBtn,this);
	}
	else
	{
		getChildView("Keep")->setVisible( false);
		getChildView("Discard")->setVisible( false);
	}
	
	childSetAction("save_tex_btn", LLPreviewTexture::onSaveAsBtn, this);
	getChildView("save_tex_btn")->setVisible( true);
	getChildView("save_tex_btn")->setEnabled(canSaveAs());
	
	if (!mCopyToInv) 
	{
		const LLInventoryItem* item = getItem();
		
		if (item)
		{
			childSetCommitCallback("desc", LLPreview::onText, this);
			getChild<LLUICtrl>("desc")->setValue(item->getDescription());
			getChild<LLLineEditor>("desc")->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);
		}
	}
	
	childSetCommitCallback("combo_aspect_ratio", onAspectRatioCommit, this);
	LLComboBox* combo = getChild<LLComboBox>("combo_aspect_ratio");
	combo->setCurrentByIndex(0);
	
	return LLPreview::postBuild();
}

// static
void LLPreviewTexture::onSaveAsBtn(void* data)
{
	LLPreviewTexture* self = (LLPreviewTexture*)data;
	self->saveAs();
}

void LLPreviewTexture::draw()
{
	updateDimensions();
	
	LLPreview::draw();

	if (!isMinimized())
	{
		LLGLSUIDefault gls_ui;
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		
		const LLRect& border = mClientRect;
		LLRect interior = mClientRect;
		interior.stretch( -PREVIEW_BORDER_WIDTH );

		// ...border
		gl_rect_2d( border, LLColor4(0.f, 0.f, 0.f, 1.f));
		gl_rect_2d_checkerboard( interior );

		if ( mImage.notNull() )
		{
			// Automatically bring up SaveAs dialog if we opened this to save the texture.
			if (mPreviewToSave)
			{
				mPreviewToSave = FALSE;
				saveAs();
			}
			// Draw the texture
			gGL.diffuseColor3f( 1.f, 1.f, 1.f );
			gl_draw_scaled_image(interior.mLeft,
								interior.mBottom,
								interior.getWidth(),
								interior.getHeight(),
								mImage);

			// Pump the texture priority
			F32 pixel_area = mLoadingFullImage ? (F32)MAX_IMAGE_AREA  : (F32)(interior.getWidth() * interior.getHeight() );
			mImage->addTextureStats( pixel_area );

			// Don't bother decoding more than we can display, unless
			// we're loading the full image.
			if (!mLoadingFullImage)
			{
				S32 int_width = interior.getWidth();
				S32 int_height = interior.getHeight();
				mImage->setKnownDrawSize(int_width, int_height);
			}
			else
			{
				// Don't use this feature
				mImage->setKnownDrawSize(0, 0);
			}

			if( mLoadingFullImage )
			{
				LLFontGL::getFontSansSerif()->renderUTF8(LLTrans::getString("Receiving"), 0,
					interior.mLeft + 4, 
					interior.mBottom + 4,
					LLColor4::white, LLFontGL::LEFT, LLFontGL::BOTTOM,
					LLFontGL::NORMAL,
					LLFontGL::DROP_SHADOW);
				
				F32 data_progress = mImage->getDownloadProgress() ;
				
				// Draw the progress bar.
				const S32 BAR_HEIGHT = 12;
				const S32 BAR_LEFT_PAD = 80;
				S32 left = interior.mLeft + 4 + BAR_LEFT_PAD;
				S32 bar_width = getRect().getWidth() - left - RESIZE_HANDLE_WIDTH - 2;
				S32 top = interior.mBottom + 4 + BAR_HEIGHT;
				S32 right = left + bar_width;
				S32 bottom = top - BAR_HEIGHT;

				LLColor4 background_color(0.f, 0.f, 0.f, 0.75f);
				LLColor4 decoded_color(0.f, 1.f, 0.f, 1.0f);
				LLColor4 downloaded_color(0.f, 0.5f, 0.f, 1.0f);

				gl_rect_2d(left, top, right, bottom, background_color);

				if (data_progress > 0.0f)
				{
					// Downloaded bytes
					right = left + llfloor(data_progress * (F32)bar_width);
					if (right > left)
					{
						gl_rect_2d(left, top, right, bottom, downloaded_color);
					}
				}
			}
			else
			if( !mSavedFileTimer.hasExpired() )
			{
				LLFontGL::getFontSansSerif()->renderUTF8(LLTrans::getString("FileSaved"), 0,
					interior.mLeft + 4,
					interior.mBottom + 4,
					LLColor4::white, LLFontGL::LEFT, LLFontGL::BOTTOM,
					LLFontGL::NORMAL,
					LLFontGL::DROP_SHADOW);
			}
		}
	} 

}


// virtual
BOOL LLPreviewTexture::canSaveAs() const
{
	return mIsCopyable && !mLoadingFullImage && mImage.notNull() && !mImage->isMissingAsset();
}


// virtual
void LLPreviewTexture::saveAs()
{
	if( mLoadingFullImage )
		return;

	LLFilePicker& file_picker = LLFilePicker::instance();
	const LLInventoryItem* item = getItem() ;
	if( !file_picker.getSaveFile( LLFilePicker::FFSAVE_TGA, item ? LLDir::getScrubbedFileName(item->getName()) : LLStringUtil::null) )
	{
		// User canceled or we failed to acquire save file.
		return;
	}
	// remember the user-approved/edited file name.
	mSaveFileName = file_picker.getFirstFile();
	mLoadingFullImage = TRUE;
	getWindow()->incBusyCount();

	mImage->forceToSaveRawImage(0) ;//re-fetch the raw image if the old one is removed.
	mImage->setLoadedCallback( LLPreviewTexture::onFileLoadedForSave, 
								0, TRUE, FALSE, new LLUUID( mItemUUID ), &mCallbackTextureList );
}

// virtual
void LLPreviewTexture::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPreview::reshape(width, height, called_from_parent);

	LLRect dim_rect(getChildView("dimensions")->getRect());

	S32 horiz_pad = 2 * (LLPANEL_BORDER_WIDTH + PREVIEW_PAD) + PREVIEW_RESIZE_HANDLE_SIZE;

	// add space for dimensions and aspect ratio
	S32 info_height = dim_rect.mTop + CLIENT_RECT_VPAD;

	LLRect client_rect(horiz_pad, getRect().getHeight(), getRect().getWidth() - horiz_pad, 0);
	client_rect.mTop -= (PREVIEW_HEADER_SIZE + CLIENT_RECT_VPAD);
	client_rect.mBottom += PREVIEW_BORDER + CLIENT_RECT_VPAD + info_height ;

	S32 client_width = client_rect.getWidth();
	S32 client_height = client_rect.getHeight();

	if (mAspectRatio > 0.f)
	{
		if(mAspectRatio > 1.f)
		{
			client_height = llceil((F32)client_width / mAspectRatio);
			if(client_height > client_rect.getHeight())
			{
				client_height = client_rect.getHeight();
				client_width = llceil((F32)client_height * mAspectRatio);
			}
		}
		else//mAspectRatio < 1.f
		{
			client_width = llceil((F32)client_height * mAspectRatio);
			if(client_width > client_rect.getWidth())
			{
				client_width = client_rect.getWidth();
				client_height = llceil((F32)client_width / mAspectRatio);
			}
		}
	}

	mClientRect.setLeftTopAndSize(client_rect.getCenterX() - (client_width / 2), client_rect.getCenterY() +  (client_height / 2), client_width, client_height);

}

// virtual
void LLPreviewTexture::onFocusReceived()
{
	LLPreview::onFocusReceived();
}

void LLPreviewTexture::openToSave()
{
	mPreviewToSave = TRUE;
}

// static
void LLPreviewTexture::onFileLoadedForSave(BOOL success, 
					   LLViewerFetchedTexture *src_vi,
					   LLImageRaw* src, 
					   LLImageRaw* aux_src, 
					   S32 discard_level,
					   BOOL final,
					   void* userdata)
{
	LLUUID* item_uuid = (LLUUID*) userdata;

	LLPreviewTexture* self = LLFloaterReg::findTypedInstance<LLPreviewTexture>("preview_texture", *item_uuid);

	if( final || !success )
	{
		delete item_uuid;

		if( self )
		{
			self->getWindow()->decBusyCount();
			self->mLoadingFullImage = FALSE;
		}
	}

	if( self && final && success )
	{
		LLPointer<LLImageTGA> image_tga = new LLImageTGA;
		if( !image_tga->encode( src ) )
		{
			LLSD args;
			args["FILE"] = self->mSaveFileName;
			LLNotificationsUtil::add("CannotEncodeFile", args);
		}
		else if( !image_tga->save( self->mSaveFileName ) )
		{
			LLSD args;
			args["FILE"] = self->mSaveFileName;
			LLNotificationsUtil::add("CannotWriteFile", args);
		}
		else
		{
			self->mSavedFileTimer.reset();
			self->mSavedFileTimer.setTimerExpirySec( SECONDS_TO_SHOW_FILE_SAVED_MSG );
		}

		self->mSaveFileName.clear();
	}

	if( self && !success )
	{
		LLNotificationsUtil::add("CannotDownloadFile");
	}

}


// It takes a while until we get height and width information.
// When we receive it, reshape the window accordingly.
void LLPreviewTexture::updateDimensions()
{
	if (!mImage)
	{
		return;
	}
	if ((mImage->getFullWidth() * mImage->getFullHeight()) == 0)
	{
		return;
	}
	
	// Update the width/height display every time
	getChild<LLUICtrl>("dimensions")->setTextArg("[WIDTH]",  llformat("%d", mImage->getFullWidth()));
	getChild<LLUICtrl>("dimensions")->setTextArg("[HEIGHT]", llformat("%d", mImage->getFullHeight()));

	// Reshape the floater only when required
	if (mUpdateDimensions)
	{
		mUpdateDimensions = FALSE;
		
		//reshape floater
		reshape(getRect().getWidth(), getRect().getHeight());

		gFloaterView->adjustToFitScreen(this, FALSE);

		LLRect dim_rect(getChildView("dimensions")->getRect());
		LLRect aspect_label_rect(getChildView("aspect_ratio")->getRect());
		getChildView("aspect_ratio")->setVisible( dim_rect.mRight < aspect_label_rect.mLeft);
	}
}


// Return true if everything went fine, false if we somewhat modified the ratio as we bumped on border values
bool LLPreviewTexture::setAspectRatio(const F32 width, const F32 height)
{
	mUpdateDimensions = TRUE;

	// We don't allow negative width or height. Also, if height is positive but too small, we reset to default
	// A default 0.f value for mAspectRatio means "unconstrained" in the rest of the code
	if ((width <= 0.f) || (height <= F_APPROXIMATELY_ZERO))
	{
		mAspectRatio = 0.f;
		return false;
	}
	
	// Compute and store the ratio
	F32 ratio = width / height;
	mAspectRatio = llclamp(ratio, PREVIEW_TEXTURE_MIN_ASPECT, PREVIEW_TEXTURE_MAX_ASPECT);
	
	// Return false if we clamped the value, true otherwise
	return (ratio == mAspectRatio);
}


void LLPreviewTexture::onAspectRatioCommit(LLUICtrl* ctrl, void* userdata)
{	
	LLPreviewTexture* self = (LLPreviewTexture*) userdata;
	
	std::string ratio(ctrl->getValue().asString());
	std::string::size_type separator(ratio.find_first_of(":/\\"));
	
	if (std::string::npos == separator) {
		// If there's no separator assume we want an unconstrained ratio
		self->setAspectRatio( 0.f, 0.f );
		return;
	}
	
	F32 width, height;
	std::istringstream numerator(ratio.substr(0, separator));
	std::istringstream denominator(ratio.substr(separator + 1));
	numerator >> width;
	denominator >> height;
	
	self->setAspectRatio( width, height );	
}

void LLPreviewTexture::loadAsset()
{
	mImage = LLViewerTextureManager::getFetchedTexture(mImageID, MIPMAP_TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	mImageOldBoostLevel = mImage->getBoostLevel();
	mImage->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
	mImage->forceToSaveRawImage(0) ;
	mAssetStatus = PREVIEW_ASSET_LOADING;
	mUpdateDimensions = TRUE;
	updateDimensions();
	getChildView("save_tex_btn")->setEnabled(canSaveAs());
}

LLPreview::EAssetStatus LLPreviewTexture::getAssetStatus()
{
	if (mImage.notNull() && (mImage->getFullWidth() * mImage->getFullHeight() > 0))
	{
		mAssetStatus = PREVIEW_ASSET_LOADED;
	}
	return mAssetStatus;
}

void LLPreviewTexture::updateImageID()
{
	const LLViewerInventoryItem *item = static_cast<const LLViewerInventoryItem*>(getItem());
	if(item)
	{
		mImageID = item->getAssetUUID();

		// here's the old logic...
		//mShowKeepDiscard = item->getPermissions().getCreator() != gAgent.getID();
		// here's the new logic... 'cos we hate disappearing buttons.
		mShowKeepDiscard = TRUE;

		mCopyToInv = FALSE;
		mIsCopyable = item->checkPermissionsSet(PERM_ITEM_UNRESTRICTED);
	}
	else // not an item, assume it's an asset id
	{
		mImageID = mItemUUID;
		mShowKeepDiscard = FALSE;
		mCopyToInv = TRUE;
		mIsCopyable = TRUE;
	}

}

/* virtual */
void LLPreviewTexture::setObjectID(const LLUUID& object_id)
{
	mObjectUUID = object_id;

	const LLUUID old_image_id = mImageID;

	// Update what image we're pointing to, such as if we just specified the mObjectID
	// that this mItemID is part of.
	updateImageID();

	// If the imageID has changed, start over and reload the new image.
	if (mImageID != old_image_id)
	{
		mAssetStatus = PREVIEW_ASSET_UNLOADED;
		loadAsset();
	}
}
