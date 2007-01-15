/** 
 * @file llpreviewtexture.cpp
 * @brief LLPreviewTexture class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llpreviewtexture.h"

#include "llviewerimage.h"
#include "llviewerimagelist.h"
#include "llresmgr.h"
#include "llagent.h"
#include "llbutton.h"
#include "llui.h"
#include "llinventoryview.h"
#include "llinventory.h"
#include "llviewerwindow.h"
#include "lltextbox.h"
#include "llimagetga.h"
#include "llfilepicker.h"
#include "llvieweruictrlfactory.h"

const S32 PREVIEW_TEXTURE_MIN_WIDTH = 300;
const S32 PREVIEW_TEXTURE_MIN_HEIGHT = 120;

const S32 CLIENT_RECT_VPAD = 4;

const F32 SECONDS_TO_SHOW_FILE_SAVED_MSG = 8.f;

LLPreviewTexture::LLPreviewTexture(const std::string& name,
								   const LLRect& rect,
								   const std::string& title,
								   const LLUUID& item_uuid,
								   const LLUUID& object_id,
								   BOOL show_keep_discard)
:	LLPreview(name, rect, title, item_uuid, object_id, TRUE, PREVIEW_TEXTURE_MIN_WIDTH, PREVIEW_TEXTURE_MIN_HEIGHT ),
	mLoadingFullImage( FALSE ),
	mShowKeepDiscard(show_keep_discard),
	mCopyToInv(FALSE),
	mIsCopyable(FALSE),
	mLastHeight(0),
	mLastWidth(0)
{
	LLInventoryItem *item = getItem();
	if(item)
	{
		mImageID = item->getAssetUUID();
		const LLPermissions& perm = item->getPermissions();
		U32 mask = PERM_NONE;
		if(perm.getOwner() == gAgent.getID())
		{
			mask = perm.getMaskBase();
		}
		else if(gAgent.isInGroup(perm.getGroup()))
		{
			mask = perm.getMaskGroup();
		}
		else
		{
			mask = perm.getMaskEveryone();
		}
		if((mask & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED)
		{
			mIsCopyable = TRUE;
		}
	}

	init();

	setTitle(title);

	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}
}


// Note: uses asset_id as a dummy item id.
LLPreviewTexture::LLPreviewTexture(
	const std::string& name,
	const LLRect& rect,
	const std::string& title,
	const LLUUID& asset_id,
	BOOL copy_to_inv)
	:
	LLPreview(
		name,
		rect,
		title,
		asset_id,
		LLUUID::null,
		TRUE,
		PREVIEW_TEXTURE_MIN_WIDTH,
		PREVIEW_TEXTURE_MIN_HEIGHT ),
	mImageID(asset_id),
	mLoadingFullImage( FALSE ),
	mShowKeepDiscard(FALSE),
	mCopyToInv(copy_to_inv),
	mIsCopyable(TRUE),
	mLastHeight(0),
	mLastWidth(0)
{

	init();

	setTitle(title);

	LLRect curRect = getRect();
	translate(curRect.mLeft - rect.mLeft, curRect.mTop - rect.mTop);
	
}


LLPreviewTexture::~LLPreviewTexture()
{
	if( mLoadingFullImage )
	{
		getWindow()->decBusyCount();
	}

	mImage = NULL;
}


void LLPreviewTexture::init()
{
	
	
	if (mCopyToInv) 
	{
		gUICtrlFactory->buildFloater(this,"floater_preview_embedded_texture.xml");

		childSetAction("Copy To Inventory",LLPreview::onBtnCopyToInv,this);
	}

	else if (mShowKeepDiscard)
	{
		gUICtrlFactory->buildFloater(this,"floater_preview_texture_keep_discard.xml");

		childSetAction("Keep",onKeepBtn,this);
		childSetAction("Discard",onDiscardBtn,this);
	}

	else 
	{
		gUICtrlFactory->buildFloater(this,"floater_preview_texture.xml");
	}


	if (!mCopyToInv) 
	{
		LLInventoryItem* item = getItem();
		
		if (item)
		{
			childSetCommitCallback("desc", LLPreview::onText, this);
			childSetText("desc", item->getDescription());
			childSetPrevalidate("desc", &LLLineEditor::prevalidatePrintableNotPipe);
		}
	}
}


// virtual
BOOL LLPreviewTexture::canSaveAs()
{
	return mIsCopyable && !mLoadingFullImage && mImage.notNull() && !mImage->isMissingAsset();
}


// virtual
void LLPreviewTexture::saveAs()
{
	if( !mLoadingFullImage )
	{
		LLFilePicker& file_picker = LLFilePicker::instance();
		if( !file_picker.getSaveFile( LLFilePicker::FFSAVE_TGA ) )
		{
			// User canceled save.
			return;
		}
		mSaveFileName = file_picker.getFirstFile();
		mLoadingFullImage = TRUE;
		getWindow()->incBusyCount();
		mImage->setLoadedCallback( LLPreviewTexture::onFileLoadedForSave, 
									0, 
									TRUE, 
									new LLUUID( mItemUUID ) );
	}
}


// static
void LLPreviewTexture::onFileLoadedForSave(BOOL success, 
											LLViewerImage *src_vi,
											LLImageRaw* src, 
											LLImageRaw* aux_src, 
											S32 discard_level,
											BOOL final,
											void* userdata)
{
	LLUUID* item_uuid = (LLUUID*) userdata;
	LLPreviewTexture* self = NULL;
	preview_map_t::iterator found_it = LLPreview::sInstances.find(*item_uuid);
	if(found_it != LLPreview::sInstances.end())
	{
		self = (LLPreviewTexture*) found_it->second;
	}

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
			LLStringBase<char>::format_map_t args;
			args["[FILE]"] = self->mSaveFileName;
			gViewerWindow->alertXml("CannotEncodeFile", args);
		}
		else if( !image_tga->save( self->mSaveFileName ) )
		{
			LLStringBase<char>::format_map_t args;
			args["[FILE]"] = self->mSaveFileName;
			gViewerWindow->alertXml("CannotWriteFile", args);
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
		gViewerWindow->alertXml("CannotDownloadFile");
	}
}


// It takes a while until we get height and width information.
// When we receive it, reshape the window accordingly.
void LLPreviewTexture::updateAspectRatio()
{
	if (!mImage) return;

	S32 image_height = llmax(1, mImage->getHeight(0));
	S32 image_width = llmax(1, mImage->getWidth(0));
	// Attempt to make the image 1:1 on screen.
	// If that fails, cut width by half.
	S32 client_width = image_width;
	S32 horiz_pad = 2 * (LLPANEL_BORDER_WIDTH + PREVIEW_PAD) + PREVIEW_RESIZE_HANDLE_SIZE;
	S32 screen_width = gViewerWindow->getWindowWidth();
	S32 max_client_width = screen_width - horiz_pad;

	while (client_width > max_client_width)
	{
		client_width /= 2;
	}

	// Demand width at least 128
	if (client_width < 128)
	{
		client_width = 128;
	}

	S32 view_width = client_width + horiz_pad;

	// Adjust the height based on the width computed above.
	F32 inv_aspect_ratio = (F32) image_height / (F32) image_width;
	S32 client_height = llround(client_width * inv_aspect_ratio);
	S32 view_height = 
		PREVIEW_HEADER_SIZE +				// header (includes top border)
		client_height + 2 * CLIENT_RECT_VPAD +  // texture plus uniform spacing (which leaves room for resize handle)
		LLPANEL_BORDER_WIDTH;				// bottom border
	
	// set text on dimensions display (should be moved out of here and into a callback of some sort)
	childSetTextArg("dimensions", "[WIDTH]", llformat("%d", mImage->mFullWidth));
	childSetTextArg("dimensions", "[HEIGHT]", llformat("%d", mImage->mFullHeight));
	
	// add space for dimensions
	S32 info_height = 0;
	LLRect dim_rect;
	childGetRect("dimensions", dim_rect);
	S32 dim_height = dim_rect.getHeight();
	info_height += dim_height + CLIENT_RECT_VPAD;
	view_height += info_height;
	
	S32 button_height = 0;
	if (mShowKeepDiscard || mCopyToInv) {  //mCopyToInvBtn

		// add space for buttons
		view_height += 	BTN_HEIGHT + CLIENT_RECT_VPAD;
		button_height = BTN_HEIGHT + PREVIEW_PAD;
	}
	
	LLRect window_rect(0, mRect.getHeight(), mRect.getWidth(), 0);
	window_rect.mTop -= (PREVIEW_HEADER_SIZE + CLIENT_RECT_VPAD);
	window_rect.mBottom += PREVIEW_BORDER + button_height + CLIENT_RECT_VPAD + info_height + CLIENT_RECT_VPAD;
	LLMultiFloater* hostp = getHost();
	if (hostp)
	{
		// try to keep aspect ratio when hosted, as hosting view can resize without user input
		mClientRect.setLeftTopAndSize(window_rect.getCenterX() - (client_width / 2), window_rect.mTop, client_width, client_height);
	}
	else
	{
		mClientRect.setLeftTopAndSize(LLPANEL_BORDER_WIDTH + PREVIEW_PAD + 6,
						mRect.getHeight() - (PREVIEW_HEADER_SIZE + CLIENT_RECT_VPAD),
						mRect.getWidth() - horiz_pad,
						mRect.getHeight() - (view_height - client_height) - 8);
	}

	if (mImage->mFullHeight > mLastHeight && mImage->mFullWidth > mLastWidth)
	{
		mLastWidth = image_width;
		mLastHeight = image_height;

		S32 old_top = mRect.mTop;
		S32 old_left = mRect.mLeft;
		if (hostp)
		{
			hostp->growToFit(this, view_width, view_height);
		}
		else
		{
			reshape( view_width, view_height );
			S32 new_bottom = old_top - mRect.getHeight();
			setOrigin( old_left, new_bottom );
		}

		// Try to keep whole view onscreen, don't allow partial offscreen.
		gFloaterView->adjustToFitScreen(this, FALSE);
	}
}

void LLPreviewTexture::loadAsset()
{
	mImage = gImageList.getImage(mImageID, MIPMAP_FALSE, FALSE);
	mImage->setBoostLevel(LLViewerImage::BOOST_PREVIEW);
	mAssetStatus = PREVIEW_ASSET_LOADING;
}

LLPreview::EAssetStatus LLPreviewTexture::getAssetStatus()
{
	if (mImage.notNull() && (mImage->mFullWidth * mImage->mFullHeight > 0))
	{
		mAssetStatus = PREVIEW_ASSET_LOADED;
	}
	return mAssetStatus;
}
