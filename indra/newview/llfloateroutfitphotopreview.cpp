/** 
 * @file llfloateroutfitphotopreview.cpp
 * @brief LLFloaterOutfitPhotoPreview class implementation
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

#include "llfloateroutfitphotopreview.h"

#include "llagent.h"
#include "llappearancemgr.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfilepicker.h"
#include "llfloaterreg.h"
#include "llimagetga.h"
#include "llimagepng.h"
#include "llinventory.h"
#include "llinventorymodel.h"
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

const S32 MAX_OUTFIT_PHOTO_WIDTH = 256;
const S32 MAX_OUTFIT_PHOTO_HEIGHT = 256;

const S32 CLIENT_RECT_VPAD = 4;

LLFloaterOutfitPhotoPreview::LLFloaterOutfitPhotoPreview(const LLSD& key)
    : LLPreview(key),
      mUpdateDimensions(TRUE),
      mImage(NULL),
      mOutfitID(LLUUID()),
      mImageOldBoostLevel(LLGLTexture::BOOST_NONE),
      mExceedLimits(FALSE)
{
    updateImageID();
}

LLFloaterOutfitPhotoPreview::~LLFloaterOutfitPhotoPreview()
{
    LLLoadedCallbackEntry::cleanUpCallbackList(&mCallbackTextureList) ;

    if (mImage.notNull())
    {
        mImage->setBoostLevel(mImageOldBoostLevel);
        mImage = NULL;
    }
}

// virtual
BOOL LLFloaterOutfitPhotoPreview::postBuild()
{
    getChild<LLButton>("ok_btn")->setClickedCallback(boost::bind(&LLFloaterOutfitPhotoPreview::onOkBtn, this));
    getChild<LLButton>("cancel_btn")->setClickedCallback(boost::bind(&LLFloaterOutfitPhotoPreview::onCancelBtn, this));

    return LLPreview::postBuild();
}

void LLFloaterOutfitPhotoPreview::draw()
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
            // Draw the texture
            gGL.diffuseColor3f( 1.f, 1.f, 1.f );
            gl_draw_scaled_image(interior.mLeft,
                                interior.mBottom,
                                interior.getWidth(),
                                interior.getHeight(),
                                mImage);

            // Pump the texture priority
            F32 pixel_area = (F32)(interior.getWidth() * interior.getHeight() );
            mImage->addTextureStats( pixel_area );

            S32 int_width = interior.getWidth();
            S32 int_height = interior.getHeight();
            mImage->setKnownDrawSize(int_width, int_height);
        }
    } 

}

// virtual
void LLFloaterOutfitPhotoPreview::reshape(S32 width, S32 height, BOOL called_from_parent)
{
    LLPreview::reshape(width, height, called_from_parent);

    LLRect dim_rect(getChildView("dimensions")->getRect());

    S32 horiz_pad = 2 * (LLPANEL_BORDER_WIDTH + PREVIEW_PAD) + PREVIEW_RESIZE_HANDLE_SIZE;

    S32 info_height = dim_rect.mTop + CLIENT_RECT_VPAD;

    LLRect client_rect(horiz_pad, getRect().getHeight(), getRect().getWidth() - horiz_pad, 0);
    client_rect.mTop -= (PREVIEW_HEADER_SIZE + CLIENT_RECT_VPAD);
    client_rect.mBottom += PREVIEW_BORDER + CLIENT_RECT_VPAD + info_height ;

    S32 client_width = client_rect.getWidth();
    S32 client_height = client_width;

    if(client_height > client_rect.getHeight())
    {
        client_height = client_rect.getHeight();
        client_width = client_height;
    }
    mClientRect.setLeftTopAndSize(client_rect.getCenterX() - (client_width / 2), client_rect.getCenterY() +  (client_height / 2), client_width, client_height);

}


void LLFloaterOutfitPhotoPreview::updateDimensions()
{
    if (!mImage)
    {
        return;
    }
    if ((mImage->getFullWidth() * mImage->getFullHeight()) == 0)
    {
        return;
    }

    if (mAssetStatus != PREVIEW_ASSET_LOADED)
    {
        mAssetStatus = PREVIEW_ASSET_LOADED;
        mUpdateDimensions = TRUE;
    }
    
    getChild<LLUICtrl>("dimensions")->setTextArg("[WIDTH]",  llformat("%d", mImage->getFullWidth()));
    getChild<LLUICtrl>("dimensions")->setTextArg("[HEIGHT]", llformat("%d", mImage->getFullHeight()));

    if ((mImage->getFullWidth() <= MAX_OUTFIT_PHOTO_WIDTH) && (mImage->getFullHeight() <= MAX_OUTFIT_PHOTO_HEIGHT))
    {
        getChild<LLButton>("ok_btn")->setEnabled(TRUE);
        mExceedLimits = FALSE;
    }
    else
    {
        mExceedLimits = TRUE;
        LLStringUtil::format_map_t args;
        args["MAX_WIDTH"] = llformat("%d", MAX_OUTFIT_PHOTO_WIDTH);
        args["MAX_HEIGHT"] = llformat("%d", MAX_OUTFIT_PHOTO_HEIGHT);
        std::string label = getString("exceed_limits", args);
        getChild<LLUICtrl>("notification")->setValue(label);
        getChild<LLUICtrl>("notification")->setColor(LLColor4::yellow);
        getChild<LLButton>("ok_btn")->setEnabled(FALSE);
    }

    if (mUpdateDimensions)
    {
        mUpdateDimensions = FALSE;

        reshape(getRect().getWidth(), getRect().getHeight());
        gFloaterView->adjustToFitScreen(this, FALSE);
    }
}

void LLFloaterOutfitPhotoPreview::loadAsset()
{
    if (mImage.notNull())
    {
        mImage->setBoostLevel(mImageOldBoostLevel);
    }
    mImage = LLViewerTextureManager::getFetchedTexture(mImageID, FTT_DEFAULT, MIPMAP_TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
    mImageOldBoostLevel = mImage->getBoostLevel();
    mImage->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
    mImage->forceToSaveRawImage(0) ;
    mAssetStatus = PREVIEW_ASSET_LOADING;
    mUpdateDimensions = TRUE;
    updateDimensions();
}

LLPreview::EAssetStatus LLFloaterOutfitPhotoPreview::getAssetStatus()
{
    if (mImage.notNull() && (mImage->getFullWidth() * mImage->getFullHeight() > 0))
    {
        mAssetStatus = PREVIEW_ASSET_LOADED;
    }
    return mAssetStatus;
}

void LLFloaterOutfitPhotoPreview::updateImageID()
{
    const LLViewerInventoryItem *item = static_cast<const LLViewerInventoryItem*>(getItem());
    if(item)
    {
        mImageID = item->getAssetUUID();
        LLPermissions perm(item->getPermissions());
    }
    else
    {
        mImageID = mItemUUID;
    }

}

/* virtual */
void LLFloaterOutfitPhotoPreview::setObjectID(const LLUUID& object_id)
{
    mObjectUUID = object_id;

    const LLUUID old_image_id = mImageID;

    updateImageID();
    if (mImageID != old_image_id)
    {
        mAssetStatus = PREVIEW_ASSET_UNLOADED;
        loadAsset();
    }
    refreshFromItem();
}

void LLFloaterOutfitPhotoPreview::setOutfitID(const LLUUID& outfit_id)
{
    mOutfitID = outfit_id;
    LLViewerInventoryCategory* outfit_folder = gInventory.getCategory(mOutfitID);
    if(outfit_folder && !mExceedLimits)
    {
        getChild<LLUICtrl>("notification")->setValue( getString("photo_confirmation"));
        getChild<LLUICtrl>("notification")->setTextArg("[OUTFIT]", outfit_folder->getName());
        getChild<LLUICtrl>("notification")->setColor(LLColor4::white);
    }

}

void LLFloaterOutfitPhotoPreview::onOkBtn()
{
    if(mOutfitID.notNull() && getItem())
    {
        LLAppearanceMgr::instance().removeOutfitPhoto(mOutfitID);
        LLPointer<LLInventoryCallback> cb = NULL;
        link_inventory_object(mOutfitID, LLConstPointer<LLInventoryObject>(getItem()), cb);
    }
    closeFloater();
}

void LLFloaterOutfitPhotoPreview::onCancelBtn()
{
    closeFloater();
}
