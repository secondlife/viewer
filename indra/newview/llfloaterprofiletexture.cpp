/** 
 * @file llfloaterprofiletexture.cpp
 * @brief LLFloaterProfileTexture class implementation
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#include "llfloaterprofiletexture.h"

#include "llbutton.h"
#include "llfloaterreg.h"
#include "llpreview.h" // fors constants
#include "lltrans.h"
#include "llviewercontrol.h"
#include "lltextureview.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"



LLFloaterProfileTexture::LLFloaterProfileTexture(LLView* owner)
    : LLFloater(LLSD())
    , mUpdateDimensions(TRUE)
    , mLastHeight(0)
    , mLastWidth(0)
    , mImage(NULL)
    , mImageOldBoostLevel(LLGLTexture::BOOST_NONE)
    , mOwnerHandle(owner->getHandle())
{
    buildFromFile("floater_profile_texture.xml");
}

LLFloaterProfileTexture::~LLFloaterProfileTexture()
{
    if (mImage.notNull())
    {
        mImage->setBoostLevel(mImageOldBoostLevel);
        mImage = NULL;
    }
}

// virtual
BOOL LLFloaterProfileTexture::postBuild()
{
    mProfileIcon = getChild<LLIconCtrl>("profile_pic");

    mCloseButton = getChild<LLButton>("close_btn");
    mCloseButton->setCommitCallback([this](LLUICtrl*, void*) { closeFloater(); }, nullptr);

	return TRUE;
}

// virtual
void LLFloaterProfileTexture::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLFloater::reshape(width, height, called_from_parent);
}

// It takes a while until we get height and width information.
// When we receive it, reshape the window accordingly.
void LLFloaterProfileTexture::updateDimensions()
{
    if (mImage.isNull())
    {
        return;
    }
    if ((mImage->getFullWidth() * mImage->getFullHeight()) == 0)
    {
        return;
    }

    S32 img_width = mImage->getFullWidth();
    S32 img_height = mImage->getFullHeight();

    if (mAssetStatus != LLPreview::PREVIEW_ASSET_LOADED
        || mLastWidth != img_width
        || mLastHeight != img_height)
    {
        mAssetStatus = LLPreview::PREVIEW_ASSET_LOADED;
        // Asset has been fully loaded
        mUpdateDimensions = TRUE;
    }

    mLastHeight = img_height;
    mLastWidth = img_width;

    // Reshape the floater only when required
    if (mUpdateDimensions)
    {
        mUpdateDimensions = FALSE;

        LLRect old_floater_rect = getRect();
        LLRect old_image_rect = mProfileIcon->getRect();
        S32 width = old_floater_rect.getWidth() - old_image_rect.getWidth() + mLastWidth;
        S32 height = old_floater_rect.getHeight() - old_image_rect.getHeight() + mLastHeight;

        const F32 MAX_DIMENTIONS = 512; // most profiles are supposed to be 256x256

        S32 biggest_dim = llmax(width, height);
        if (biggest_dim > MAX_DIMENTIONS)
        {
            F32 scale_down = MAX_DIMENTIONS / (F32)biggest_dim;
            width *= scale_down;
            height *= scale_down;
        }

        //reshape floater
        reshape(width, height);

        gFloaterView->adjustToFitScreen(this, FALSE);
    }
}

void LLFloaterProfileTexture::draw()
{
    // drawFrustum
    LLView *owner = mOwnerHandle.get();
    static LLCachedControl<F32> max_opacity(gSavedSettings, "PickerContextOpacity", 0.4f);
    drawConeToOwner(mContextConeOpacity, max_opacity, owner);

    LLFloater::draw();
}

void LLFloaterProfileTexture::onOpen(const LLSD& key)
{
    mCloseButton->setFocus(true);
}

void LLFloaterProfileTexture::resetAsset()
{
    mProfileIcon->setValue("Generic_Person_Large");
    mImageID = LLUUID::null;
    if (mImage.notNull())
    {
        mImage->setBoostLevel(mImageOldBoostLevel);
        mImage = NULL;
    }
}
void LLFloaterProfileTexture::loadAsset(const LLUUID &image_id)
{
    if (mImageID != image_id)
    {
        if (mImage.notNull())
        {
            mImage->setBoostLevel(mImageOldBoostLevel);
            mImage = NULL;
        }
    }
    else
    {
        return;
    }

    mProfileIcon->setValue(image_id);
    mImageID = image_id;
    mImage = LLViewerTextureManager::getFetchedTexture(mImageID, FTT_DEFAULT, MIPMAP_TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
    mImageOldBoostLevel = mImage->getBoostLevel();

    if ((mImage->getFullWidth() * mImage->getFullHeight()) == 0)
    {
        mImage->setLoadedCallback(LLFloaterProfileTexture::onTextureLoaded,
            0, TRUE, FALSE, new LLHandle<LLFloater>(getHandle()), &mCallbackTextureList);

        mImage->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
        mAssetStatus = LLPreview::PREVIEW_ASSET_LOADING;
    }
    else
    {
        mAssetStatus = LLPreview::PREVIEW_ASSET_LOADED;
    }

    mUpdateDimensions = TRUE;
    updateDimensions();
}

// static
void LLFloaterProfileTexture::onTextureLoaded(
    BOOL success,
    LLViewerFetchedTexture *src_vi,
    LLImageRaw* src,
    LLImageRaw* aux_src,
    S32 discard_level,
    BOOL final,
    void* userdata)
{
    LLHandle<LLFloater>* handle = (LLHandle<LLFloater>*)userdata;

    if (!handle->isDead())
    {
        LLFloaterProfileTexture* floater = static_cast<LLFloaterProfileTexture*>(handle->get());
        if (floater && success)
        {
            floater->mUpdateDimensions = TRUE;
            floater->updateDimensions();
        }
    }

    if (final || !success)
    {
        delete handle;
    }
}
