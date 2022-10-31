/** 
 * @file llfloateroutfitphotopreview.h
 * @brief LLFloaterOutfitPhotoPreview class definition
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

#ifndef LL_LLFLOATEROUTFITPHOTOPREVIEW_H
#define LL_LLFLOATEROUTFITPHOTOPREVIEW_H

#include "llpreview.h"
#include "llbutton.h"
#include "llframetimer.h"
#include "llviewertexture.h"

class LLComboBox;
class LLImageRaw;

class LLFloaterOutfitPhotoPreview : public LLPreview
{
public:
    LLFloaterOutfitPhotoPreview(const LLSD& key);
    ~LLFloaterOutfitPhotoPreview();

    virtual void        draw();

    virtual void        loadAsset();
    virtual EAssetStatus    getAssetStatus();
    
    virtual void        reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

    /*virtual*/ void setObjectID(const LLUUID& object_id);

    void setOutfitID(const LLUUID& outfit_id);
    void onOkBtn();
    void onCancelBtn();

protected:
    void                init();
    /* virtual */ BOOL  postBuild();
    
private:
    void                updateImageID(); // set what image is being uploaded.
    void                updateDimensions();
    LLUUID              mImageID;
    LLUUID              mOutfitID;
    LLPointer<LLViewerFetchedTexture>       mImage;
    S32                 mImageOldBoostLevel;

    // This is stored off in a member variable, because the save-as
    // button and drag and drop functionality need to know.
    BOOL mUpdateDimensions;

    BOOL mExceedLimits;

    LLLoadedCallbackEntry::source_callback_list_t mCallbackTextureList ;
};
#endif  // LL_LLFLOATEROUTFITPHOTOPREVIEW_H
