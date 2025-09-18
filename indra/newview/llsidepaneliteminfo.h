/**
 * @file llsidepaneliteminfo.h
 * @brief A panel which shows an inventory item's properties.
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

#ifndef LL_LLSIDEPANELITEMINFO_H
#define LL_LLSIDEPANELITEMINFO_H

#include "llinventoryobserver.h"
#include "llpanel.h"
#include "llstyle.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLSidepanelItemInfo
// Object properties for inventory side panel.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLAvatarName;
class LLButton;
class LLFloater;
class LLIconCtrl;
class LLViewerInventoryItem;
class LLItemPropertiesObserver;
class LLObjectInventoryObserver;
class LLViewerObject;
class LLPermissions;
class LLTextBox;
class LLTextEditor;

class LLSidepanelItemInfo : public LLPanel, public LLInventoryObserver
{
public:
    LLSidepanelItemInfo(const LLPanel::Params& p = getDefaultParams());
    virtual ~LLSidepanelItemInfo();

    /*virtual*/ bool postBuild() override;
    /*virtual*/ void reset();

    void setObjectID(const LLUUID& object_id);
    void setItemID(const LLUUID& item_id);
    void setParentFloater(LLFloater* parent); // For simplicity

    const LLUUID& getObjectID() const;
    const LLUUID& getItemID() const;

    // if received update and item id (from callback) matches internal ones, update UI
    void onUpdateCallback(const LLUUID& item_id, S32 received_update_id);

    void changed(U32 mask) override;
    void dirty();

    static void onIdle( void* user_data );
    void updateOwnerName(const LLUUID& owner_id, const LLAvatarName& owner_name, const LLStyle::Params& style_params);
    void updateCreatorName(const LLUUID& creator_id, const LLAvatarName& creator_name, const LLStyle::Params& style_params);

protected:
    void refresh() override;
    void save();

    LLViewerInventoryItem* findItem() const;
    LLViewerObject*  findObject() const;

    void refreshFromItem(LLViewerInventoryItem* item);

private:
    static void setAssociatedExperience( LLHandle<LLSidepanelItemInfo> hInfo, const LLSD& experience );

    void startObjectInventoryObserver();
    void stopObjectInventoryObserver();
    void setPropertiesFieldsEnabled(bool enabled);

    boost::signals2::connection mOwnerCacheConnection;
    boost::signals2::connection mCreatorCacheConnection;

    LLUUID mItemID;     // inventory UUID for the inventory item.
    LLUUID mObjectID;   // in-world task UUID, or null if in agent inventory.
    LLObjectInventoryObserver* mObjectInventoryObserver; // for syncing changes to items inside an object

    // We can send multiple properties updates simultaneously, make sure only last response counts and there won't be a race condition.
    S32 mUpdatePendingId;
    bool mIsDirty;         // item properties need to be updated
    LLFloater* mParentFloater;

    LLUICtrl* mChangeThumbnailBtn;
    LLIconCtrl* mItemTypeIcon;
    LLTextBox* mLabelOwnerName;
    LLTextBox* mLabelCreatorName;
    LLTextEditor* mLabelItemDesc;

    //
    // UI Elements
    //
protected:
    void                        onClickCreator();
    void                        onClickOwner();
    void                        onCommitName();
    void                        onCommitDescription();
    void                        onCommitPermissions(LLUICtrl* ctrl);
    void                        updatePermissions();
    void                        onEditThumbnail();
    void                        onCommitSaleInfo(LLUICtrl* ctrl);
    void                        updateSaleInfo();
    void                        onCommitChanges(LLPointer<LLViewerInventoryItem> item);
};

#endif // LL_LLSIDEPANELITEMINFO_H
