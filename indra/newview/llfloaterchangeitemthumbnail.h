/**
 * @file llfloaterchangeitemthumbnail.h
 * @brief LLFloaterChangeItemThumbnail class definition
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#ifndef LL_LLFLOATERCHANGEITEMTHUMBNAIL_H
#define LL_LLFLOATERCHANGEITEMTHUMBNAIL_H

#include "llfloater.h"
#include "llinventoryobserver.h"
#include "llvoinventorylistener.h"

class LLButton;
class LLIconCtrl;
class LLTextBox;
class LLThumbnailCtrl;
class LLUICtrl;
class LLViewerInventoryItem;
class LLViewerFetchedTexture;

class LLFloaterChangeItemThumbnail : public LLFloater, public LLInventoryObserver, public LLVOInventoryListener
{
public:
    LLFloaterChangeItemThumbnail(const LLSD& key);
    ~LLFloaterChangeItemThumbnail();

    bool postBuild() override;
    void onOpen(const LLSD& key) override;
    void onFocusReceived() override;
    void onMouseEnter(S32 x, S32 y, MASK mask) override;

    bool handleDragAndDrop(
        S32 x,
        S32 y,
        MASK mask,
        bool drop,
        EDragAndDropType cargo_type,
        void *cargo_data,
        EAcceptance *accept,
        std::string& tooltip_msg) override;

    void changed(U32 mask) override;
    void inventoryChanged(LLViewerObject* object,
        LLInventoryObject::object_list_t* inventory,
        S32 serial_num,
        void* user_data) override;

    static bool validateAsset(const LLUUID &asset_id);

private:

    LLInventoryObject* getInventoryObject();
    void refreshFromInventory();
    void refreshFromObject(LLInventoryObject* obj);

    static void onUploadLocal(void*);
    static void onUploadSnapshot(void*);
    static void onUseTexture(void*);
    static void onCopyToClipboard(void*);
    static void onPasteFromClipboard(void*);
    static void onRemove(void*);
    static void onRemovalConfirmation(const LLSD& notification, const LLSD& response, LLHandle<LLFloater> handle);

    void assignAndValidateAsset(const LLUUID &asset_id, bool silent = false);
    static void onImageDataLoaded(bool success,
        LLViewerFetchedTexture *src_vi,
        LLImageRaw* src,
        LLImageRaw* aux_src,
        S32 discard_level,
        bool final,
        void* userdata);
    static void onFullImageLoaded(bool success,
                                  LLViewerFetchedTexture* src_vi,
                                  LLImageRaw* src,
                                  LLImageRaw* aux_src,
                                  S32 discard_level,
                                  bool final,
                                  void* userdata);

    void showTexturePicker(const LLUUID &thumbnail_id);
    void onTexturePickerCommit();
    static void onUploadComplete(const LLUUID& asset_id, const LLUUID& task_id, const uuid_set_t& inventory_ids, LLHandle<LLFloater> handle);

    void setThumbnailId(const LLUUID &new_thumbnail_id);
    static void setThumbnailId(const LLUUID& new_thumbnail_id, const LLUUID& task_id, const LLUUID& inv_obj_id);
    static void setThumbnailId(const LLUUID& new_thumbnail_id, const LLUUID& inv_obj_id, LLInventoryObject* obj);

    enum EToolTipState
    {
        TOOLTIP_NONE,
        TOOLTIP_UPLOAD_LOCAL,
        TOOLTIP_UPLOAD_SNAPSHOT,
        TOOLTIP_USE_TEXTURE,
        TOOLTIP_COPY_TO_CLIPBOARD,
        TOOLTIP_COPY_FROM_CLIPBOARD,
        TOOLTIP_REMOVE,
    };

    void onButtonMouseEnter(LLUICtrl* button, const LLSD& param, EToolTipState state);
    void onButtonMouseLeave(LLUICtrl* button, const LLSD& param, EToolTipState state);

    bool mObserverInitialized;
    bool mMultipleThumbnails; // for multiselection
    EToolTipState mTooltipState;
    uuid_set_t mItemList;
    LLUUID mTaskId;
    LLUUID mExpectingAssetId;

    LLIconCtrl *mItemTypeIcon;
    LLUICtrl *mItemNameText;
    LLThumbnailCtrl *mThumbnailCtrl;
    LLTextBox *mToolTipTextBox;
    LLTextBox *mMultipleTextBox;
    LLButton *mCopyToClipboardBtn;
    LLButton *mPasteFromClipboardBtn;
    LLButton *mRemoveImageBtn;

    LLHandle<LLFloater> mPickerHandle;
    LLHandle<LLFloater> mSnapshotHandle;
};
#endif  // LL_LLFLOATERCHANGEITEMTHUMBNAIL_H
