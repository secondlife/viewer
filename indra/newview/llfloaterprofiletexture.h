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
#include "llviewertexture.h"

class LLButton;
class LLImageRaw;
class LLIconCtrl;

class LLProfileImageMonitor: public LLHandleProvider<LLProfileImageMonitor>
{
public:
    LLProfileImageMonitor();
    virtual ~LLProfileImageMonitor();

    void setImageAssetId(const LLUUID& asset_id);
    LLUUID getImageAssetId() { return mImageID; }
    void refreshImageStats();
    LLPointer<LLViewerFetchedTexture> getImage() {return mImage;}

    virtual void onImageLoaded(BOOL success, LLViewerFetchedTexture* imagep) = 0;
private:
    static void onImageLoaded(BOOL success,
                              LLViewerFetchedTexture* src_vi,
                              LLImageRaw* src,
                              LLImageRaw* aux_src,
                              S32 discard_level,
                              BOOL final,
                              void* userdata);
    void releaseTexture();
protected:
    LLPointer<LLViewerFetchedTexture> mImage;
    S32 mAssetStatus;
private:
    LLUUID mImageID;
    S32 mImageOldBoostLevel;
    bool mWasNoDelete;
    LLLoadedCallbackEntry::source_callback_list_t mCallbackTextureList;

};

class LLFloaterProfileTexture : public LLFloater, public LLProfileImageMonitor
{
public:
    LLFloaterProfileTexture(LLView* owner);
    ~LLFloaterProfileTexture();

    void draw() override;
    void onOpen(const LLSD& key) override;

    void resetAsset();
    void loadAsset(const LLUUID &image_id);

    void onImageLoaded(BOOL success, LLViewerFetchedTexture* imagep) override;

    void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE) override;

    LLHandle<LLFloater> getHandle() const { return LLFloater::getHandle(); }
protected:
    BOOL postBuild() override;

private:
    void updateDimensions();

    F32 mContextConeOpacity;
    S32 mLastHeight;
    S32 mLastWidth;
    BOOL mUpdateDimensions;

    LLHandle<LLView> mOwnerHandle;
    LLIconCtrl* mProfileIcon;
    LLButton* mCloseButton;
};
#endif  // LL_LLFLOATERPROFILETEXTURE_H
