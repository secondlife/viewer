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
#include "llviewertexture.h"
#include "llviewertexturelist.h"


 //////////////////////////////////////////////////////////////////////////
 // LLProfileImageCtrl
 //////////////////////////////////////////////////////////////////////////

static LLDefaultChildRegistry::Register<LLProfileImageCtrl> r("profile_image");

LLProfileImageCtrl::LLProfileImageCtrl(const LLProfileImageCtrl::Params& p)
    : LLIconCtrl(p)
    , mImage(NULL)
    , mImageOldBoostLevel(LLGLTexture::BOOST_NONE)
    , mWasNoDelete(false)
    , mImageLoadedSignal(NULL)
{
}

LLProfileImageCtrl::~LLProfileImageCtrl()
{
    LLLoadedCallbackEntry::cleanUpCallbackList(&mCallbackTextureList);
    releaseTexture();

    delete mImageLoadedSignal;
}

void LLProfileImageCtrl::releaseTexture()
{
    if (mImage.notNull())
    {
        mImage->setBoostLevel(mImageOldBoostLevel);
        if (!mWasNoDelete)
        {
            // In most cases setBoostLevel marks images as NO_DELETE
            mImage->forceActive();
        }
        mImage = NULL;
    }
}

void LLProfileImageCtrl::setValue(const LLSD& value)
{
    LLUUID id = value.asUUID();
    setImageAssetId(id);
    if (id.isNull())
    {
        LLIconCtrl::setValue("Generic_Person_Large", LLGLTexture::BOOST_UI);
    }
    else
    {
        // called second to not change priority before it gets saved to mImageOldBoostLevel
        LLIconCtrl::setValue(value, LLGLTexture::BOOST_PREVIEW);
    }
}

void LLProfileImageCtrl::draw()
{
    if (mImage.notNull())
    {
        // Pump the texture priority
        mImage->addTextureStats(MAX_IMAGE_AREA);
        mImage->setKnownDrawSize(LLViewerTexture::MAX_IMAGE_SIZE_DEFAULT, LLViewerTexture::MAX_IMAGE_SIZE_DEFAULT);
    }
    LLIconCtrl::draw();
}

boost::signals2::connection LLProfileImageCtrl::setImageLoadedCallback(const image_loaded_signal_t::slot_type& cb)
{
    if (!mImageLoadedSignal) mImageLoadedSignal = new image_loaded_signal_t();

    return mImageLoadedSignal->connect(cb);
}

void LLProfileImageCtrl::setImageAssetId(const LLUUID& asset_id)
{
    if (mImageID == asset_id)
    {
        return;
    }

    releaseTexture();

    mImageID = asset_id;
    if (mImageID.notNull())
    {
        mImage = LLViewerTextureManager::getFetchedTexture(mImageID, FTT_DEFAULT, MIPMAP_YES, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
        mWasNoDelete = mImage->getTextureState() == LLGLTexture::NO_DELETE;
        mImageOldBoostLevel = mImage->getBoostLevel();
        mImage->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
        mImage->setKnownDrawSize(LLViewerTexture::MAX_IMAGE_SIZE_DEFAULT, LLViewerTexture::MAX_IMAGE_SIZE_DEFAULT);
        mImage->forceToSaveRawImage(0);

        if ((mImage->getFullWidth() * mImage->getFullHeight()) == 0)
        {
            mImage->setLoadedCallback(LLProfileImageCtrl::onImageLoaded,
                                      0, true, false, new LLHandle<LLUICtrl>(getHandle()), &mCallbackTextureList);
        }
        else
        {
            onImageLoaded(true, mImage);
        }
    }
}

void LLProfileImageCtrl::onImageLoaded(bool success, LLViewerFetchedTexture* img)
{
    if (mImageLoadedSignal)
    {
        (*mImageLoadedSignal)(success, img);
    }
}

// static
void LLProfileImageCtrl::onImageLoaded(bool success,
                                          LLViewerFetchedTexture* src_vi,
                                          LLImageRaw* src,
                                          LLImageRaw* aux_src,
                                          S32 discard_level,
                                          bool final,
                                          void* userdata)
{
    if (!userdata) return;

    LLHandle<LLUICtrl>* handle = (LLHandle<LLUICtrl>*)userdata;

    if (!handle->isDead())
    {
        LLProfileImageCtrl* caller = static_cast<LLProfileImageCtrl*>(handle->get());
        if (caller && caller->mImageLoadedSignal)
        {
            (*caller->mImageLoadedSignal)(success, src_vi);
        }
    }

    if (final || !success)
    {
        delete handle;
    }
}


//////////////////////////////////////////////////////////////////////////
// LLFloaterProfileTexture
 //////////////////////////////////////////////////////////////////////////

LLFloaterProfileTexture::LLFloaterProfileTexture(LLView* owner)
    : LLFloater(LLSD())
    , mLastHeight(0)
    , mLastWidth(0)
    , mOwnerHandle(owner->getHandle())
    , mContextConeOpacity(0.f)
    , mCloseButton(NULL)
    , mProfileIcon(NULL)
{
    buildFromFile("floater_profile_texture.xml");
}

LLFloaterProfileTexture::~LLFloaterProfileTexture()
{
}

// virtual
bool LLFloaterProfileTexture::postBuild()
{
    mProfileIcon = getChild<LLProfileImageCtrl>("profile_pic");
    mProfileIcon->setImageLoadedCallback([this](bool success, LLViewerFetchedTexture* imagep) {onImageLoaded(success, imagep); });

    mCloseButton = getChild<LLButton>("close_btn");
    mCloseButton->setCommitCallback([this](LLUICtrl*, void*) { closeFloater(); }, nullptr);

    return true;
}

// virtual
void LLFloaterProfileTexture::reshape(S32 width, S32 height, bool called_from_parent)
{
    LLFloater::reshape(width, height, called_from_parent);
}

// It takes a while until we get height and width information.
// When we receive it, reshape the window accordingly.
void LLFloaterProfileTexture::updateDimensions()
{
    LLPointer<LLViewerFetchedTexture> image = mProfileIcon->getImage();
    if (image.isNull())
    {
        return;
    }
    if ((image->getFullWidth() * image->getFullHeight()) == 0)
    {
        return;
    }

    S32 img_width = image->getFullWidth();
    S32 img_height = image->getFullHeight();

    mLastHeight = img_height;
    mLastWidth = img_width;

    LLRect old_floater_rect = getRect();
    LLRect old_image_rect = mProfileIcon->getRect();
    S32 width = old_floater_rect.getWidth() - old_image_rect.getWidth() + mLastWidth;
    S32 height = old_floater_rect.getHeight() - old_image_rect.getHeight() + mLastHeight;

    const F32 MAX_DIMENTIONS = 512; // most profiles are supposed to be 256x256

    S32 biggest_dim = llmax(width, height);
    if (biggest_dim > MAX_DIMENTIONS)
    {
        F32 scale_down = MAX_DIMENTIONS / (F32)biggest_dim;
        width = (S32)(width * scale_down);
        height = (S32)(height * scale_down);
    }

    //reshape floater
    reshape(width, height);

    gFloaterView->adjustToFitScreen(this, false);
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
    mProfileIcon->setValue(LLUUID::null);
}
void LLFloaterProfileTexture::loadAsset(const LLUUID &image_id)
{
    mProfileIcon->setValue(image_id);
    updateDimensions();
}

void LLFloaterProfileTexture::onImageLoaded(bool success, LLViewerFetchedTexture* imagep)
{
    if (success)
    {
        updateDimensions();
    }
}
