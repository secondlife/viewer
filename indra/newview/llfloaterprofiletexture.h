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

class LLFloaterProfileTexture : public LLFloater
{
public:
    LLFloaterProfileTexture(LLView* owner);
    ~LLFloaterProfileTexture();

    void draw() override;
    void onOpen(const LLSD& key) override;

    void resetAsset();
    void loadAsset(const LLUUID &image_id);


    static void onTextureLoaded(
        BOOL success,
        LLViewerFetchedTexture *src_vi,
        LLImageRaw* src,
        LLImageRaw* aux_src,
        S32 discard_level,
        BOOL final,
        void* userdata);

    void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE) override;
protected:
    BOOL postBuild() override;

private:
    void updateDimensions();

    LLUUID mImageID;
    LLPointer<LLViewerFetchedTexture> mImage;
    S32 mImageOldBoostLevel;
    S32 mAssetStatus;
    F32 mContextConeOpacity;
    S32 mLastHeight;
    S32 mLastWidth;
    BOOL mUpdateDimensions;

    LLHandle<LLView> mOwnerHandle;
    LLIconCtrl* mProfileIcon;
    LLButton* mCloseButton;

    LLLoadedCallbackEntry::source_callback_list_t mCallbackTextureList;
};
#endif  // LL_LLFLOATERPROFILETEXTURE_H
