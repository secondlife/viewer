/**
 * @file llpreviewtexture.h
 * @brief LLPreviewTexture class definition
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

#ifndef LL_LLPREVIEWTEXTURE_H
#define LL_LLPREVIEWTEXTURE_H

#include "llpreview.h"
#include "llbutton.h"
#include "llframetimer.h"
#include "llviewertexture.h"

class LLComboBox;
class LLImageRaw;
class LLLayoutPanel;

class LLPreviewTexture : public LLPreview
{
public:
    LLPreviewTexture(const LLSD& key);
    ~LLPreviewTexture();

    virtual void        draw();

    virtual bool        canSaveAs() const;
    virtual void        saveAs();

    virtual void        loadAsset();
    virtual EAssetStatus    getAssetStatus();

    virtual void        reshape(S32 width, S32 height, bool called_from_parent = true);
    virtual void        onFocusReceived();

    static void         onFileLoadedForSave(
                            bool success,
                            LLViewerFetchedTexture *src_vi,
                            LLImageRaw* src,
                            LLImageRaw* aux_src,
                            S32 discard_level,
                            bool final,
                            void* userdata );
    void                openToSave();

    void                saveTextureToFile(const std::vector<std::string>& filenames);
    void                saveMultipleToFile(const std::string& file_name = "");

    static void         onSaveAsBtn(void* data);

    void                hideCtrlButtons();

    /*virtual*/ void setObjectID(const LLUUID& object_id);
protected:
    void                init();
    void                populateRatioList();
    /* virtual */ bool  postBuild();
    bool                setAspectRatio(const F32 width, const F32 height);
    static void         onAspectRatioCommit(LLUICtrl*,void* userdata);
    void                adjustAspectRatio();

private:
    void                updateImageID(); // set what image is being uploaded.
    void                updateDimensions();
    LLUUID              mImageID;
    LLPointer<LLViewerFetchedTexture>       mImage;
    S32                 mImageOldBoostLevel;
    std::string         mSaveFileName;
    LLFrameTimer        mSavedFileTimer;
    bool                mSavingMultiple;
    bool                mLoadingFullImage;
    bool                mShowKeepDiscard;
    bool                mCopyToInv;

    // Save the image once it's loaded.
    bool                mPreviewToSave;

    // This is stored off in a member variable, because the save-as
    // button and drag and drop functionality need to know.
    bool mIsCopyable;
    bool mIsFullPerm;
    bool mUpdateDimensions;
    S32 mLastHeight;
    S32 mLastWidth;
    F32 mAspectRatio;

    LLLoadedCallbackEntry::source_callback_list_t mCallbackTextureList ;
    std::vector<std::string>        mRatiosList;

    LLLayoutPanel* mButtonsPanel = nullptr;
    LLUICtrl* mDimensionsText = nullptr;
    LLUICtrl* mAspectRatioText = nullptr;
};
#endif  // LL_LLPREVIEWTEXTURE_H
