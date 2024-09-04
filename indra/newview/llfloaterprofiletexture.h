/**
 * @file llfloaterprofiletexture.h
 * @brief LLFloaterProfileTexture class definition
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

#ifndef LL_LLFLOATERPROFILETEXTURE_H
#define LL_LLFLOATERPROFILETEXTURE_H

#include "llfloater.h"
#include "lliconctrl.h"
#include "llviewertexture.h"

class LLButton;
class LLImageRaw;

class LLProfileImageCtrl: public LLIconCtrl
{
public:
    struct Params: public LLInitParam::Block<Params, LLIconCtrl::Params>
    {
    };

    LLProfileImageCtrl(const Params& p);
    virtual ~LLProfileImageCtrl();


    virtual void setValue(const LLSD& value) override;
    LLUUID getImageAssetId() { return mImageID; }
    LLPointer<LLViewerFetchedTexture> getImage() {return mImage;}
    void draw() override;

    typedef boost::signals2::signal<void(bool success, LLViewerFetchedTexture* imagep)> image_loaded_signal_t;
    boost::signals2::connection setImageLoadedCallback(const image_loaded_signal_t::slot_type& cb);
private:
    void onImageLoaded(bool success, LLViewerFetchedTexture* src_vi);
    static void onImageLoaded(bool success,
                              LLViewerFetchedTexture* src_vi,
                              LLImageRaw* src,
                              LLImageRaw* aux_src,
                              S32 discard_level,
                              bool final,
                              void* userdata);
    void releaseTexture();

    void setImageAssetId(const LLUUID& asset_id);
private:
    LLPointer<LLViewerFetchedTexture> mImage;
    LLUUID mImageID;
    S32 mImageOldBoostLevel;
    bool mWasNoDelete;
    image_loaded_signal_t* mImageLoadedSignal;
    LLLoadedCallbackEntry::source_callback_list_t mCallbackTextureList;
};

class LLFloaterProfileTexture : public LLFloater
{
public:
    LLFloaterProfileTexture(LLView* owner);
    ~LLFloaterProfileTexture();

    void draw() override;
    void onOpen(const LLSD& key) override;

    void resetAsset();
    void loadAsset(const LLUUID &image_id);

    void onImageLoaded(bool success, LLViewerFetchedTexture* imagep);

    void reshape(S32 width, S32 height, bool called_from_parent = true) override;

    LLHandle<LLFloater> getHandle() const { return LLFloater::getHandle(); }
protected:
    bool postBuild() override;

private:
    void updateDimensions();

    F32 mContextConeOpacity;
    S32 mLastHeight;
    S32 mLastWidth;

    LLHandle<LLView> mOwnerHandle;
    LLProfileImageCtrl* mProfileIcon;
    LLButton* mCloseButton;
};
#endif  // LL_LLFLOATERPROFILETEXTURE_H
