/** 
 * @file llpreviewtexture.cpp
 * @brief LLPreviewTexture class implementation
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
	  mImage(NULL)
{
	updateImageID();
	if (key.has("save_as"))
	{
		mPreviewToSave = TRUE;
	}
	//Called from floater reg: LLUICtrlFactory::getInstance()->buildFloater(this, "floater_preview_texture.xml", FALSE);
}

LLPreviewTexture::~LLPreviewTexture()
{
	if( mLoadingFullImage )
	{
		getWindow()->decBusyCount();
	}

	mImage = NULL;
}

// virtual
BOOL LLPreviewTexture::postBuild()
{
	if (mCopyToInv) 
	{
		getChild<LLButton>("Keep")->setLabel(getString("Copy"));
		childSetAction("Keep",LLPreview::onBtnCopyToInv,this);
		childSetVisible("Discard", false);
	}
	else if (mShowKeepDiscard)
	{
		childSetAction("Keep",onKeepBtn,this);
		childSetAction("Discard",onDiscardBtn,this);
	}
	else
	{
		childSetVisible("Keep", false);
		childSetVisible("Discard", false);
	}
	
	childSetAction("save_tex_btn", LLPreviewTexture::onSaveAsBtn, this);
	childSetVisible("save_tex_btn", canSaveAs());
	
	if (!mCopyToInv) 
	{
		const LLInventoryItem* item = getItem();
		
		if (item)
		{
			childSetCommitCallback("desc", LLPreview::onText, this);
			childSetText("desc", item->getDescription());
			childSetPrevalidate("desc", &LLTextValidate::validateASCIIPrintableNoPipe);
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
	if (mUpdateDimensions)
	{
		updateDimensions();
	}
	
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
			glColor3f( 1.f, 1.f, 1.f );
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
	mImage->setLoadedCallback( LLPreviewTexture::onFileLoadedForSave, 
								0, TRUE, FALSE, new LLUUID( mItemUUID ) );
}

// virtual
void LLPreviewTexture::reshape(S32 width, S32 height, BOOL called_from_parent)
{
//	mLastHeight = 0;
//	mLastWidth = 0;
	mUpdateDimensions = TRUE;
	LLPreview::reshape(width, height, called_from_parent);
}

// virtual
void LLPreviewTexture::onFocusReceived()
{
	mLastHeight = 0;
	mLastWidth = 0;
	mUpdateDimensions = TRUE;
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
		return;
	
	mUpdateDimensions = FALSE;
	
	// set text on dimensions display (should be moved out of here and into a callback of some sort)
	childSetTextArg("dimensions", "[WIDTH]", llformat("%d", mImage->getFullWidth()));
	childSetTextArg("dimensions", "[HEIGHT]", llformat("%d", mImage->getFullHeight()));

	LLRect dim_rect;
	childGetRect("dimensions", dim_rect);

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
		else
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

	// Hide the aspect ratio label if the window is too narrow
	// Assumes the label should be to the right of the dimensions
	LLRect aspect_label_rect;
	childGetRect("aspect_ratio", aspect_label_rect);
	childSetVisible("aspect_ratio", dim_rect.mRight < aspect_label_rect.mLeft);
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
	mImage = LLViewerTextureManager::getFetchedTexture(mImageID, MIPMAP_TRUE, LLViewerTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	mImage->setBoostLevel(LLViewerTexture::BOOST_PREVIEW);
	mImage->forceToSaveRawImage(0) ;
	mAssetStatus = PREVIEW_ASSET_LOADING;
	updateDimensions();
	childSetVisible("save_tex_btn", canSaveAs());
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
		mShowKeepDiscard = item->getPermissions().getCreator() != gAgent.getID();
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
