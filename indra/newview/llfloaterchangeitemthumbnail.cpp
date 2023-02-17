/** 
 * @file llfloaterchangeitemthumbnail.cpp
 * @brief LLFloaterChangeItemThumbnail class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterchangeitemthumbnail.h"

#include "llbutton.h"
#include "llclipboard.h"
#include "lliconctrl.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "lllineeditor.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "llthumbnailctrl.h"
#include "llviewerobjectlist.h"
#include "llwindow.h"


LLFloaterChangeItemThumbnail::LLFloaterChangeItemThumbnail(const LLSD& key)
    : LLFloater(key)
    , mObserverInitialized(false)
    , mTooltipState(TOOLTIP_NONE)
{
}

LLFloaterChangeItemThumbnail::~LLFloaterChangeItemThumbnail()
{
    gInventory.removeObserver(this);
    removeVOInventoryListener();
}

BOOL LLFloaterChangeItemThumbnail::postBuild()
{
    mItemNameText = getChild<LLUICtrl>("item_name");
    mItemTypeIcon = getChild<LLIconCtrl>("item_type_icon");
    mThumbnailCtrl = getChild<LLThumbnailCtrl>("item_thumbnail");
    mToolTipTextBox = getChild<LLTextBox>("tooltip_text");

    LLSD tooltip_text;
    mToolTipTextBox->setValue(tooltip_text);

    LLButton *upload_local = getChild<LLButton>("upload_local");
    upload_local->setClickedCallback(onUploadLocal, (void*)this);
    upload_local->setMouseEnterCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseEnter, this, _1, _2, TOOLTIP_UPLOAD_LOCAL));
    upload_local->setMouseLeaveCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseLeave, this, _1, _2, TOOLTIP_UPLOAD_LOCAL));

    LLButton *upload_snapshot = getChild<LLButton>("upload_snapshot");
    upload_snapshot->setClickedCallback(onUploadSnapshot, (void*)this);
    upload_snapshot->setMouseEnterCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseEnter, this, _1, _2, TOOLTIP_UPLOAD_SNAPSHOT));
    upload_snapshot->setMouseLeaveCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseLeave, this, _1, _2, TOOLTIP_UPLOAD_SNAPSHOT));

    LLButton *use_texture = getChild<LLButton>("use_texture");
    use_texture->setClickedCallback(onUseTexture, (void*)this);
    use_texture->setMouseEnterCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseEnter, this, _1, _2, TOOLTIP_USE_TEXTURE));
    use_texture->setMouseLeaveCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseLeave, this, _1, _2, TOOLTIP_USE_TEXTURE));

    mCopyToClipboardBtn = getChild<LLButton>("copy_to_clipboard");
    mCopyToClipboardBtn->setClickedCallback(onCopyToClipboard, (void*)this);
    mCopyToClipboardBtn->setMouseEnterCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseEnter, this, _1, _2, TOOLTIP_COPY_TO_CLIPBOARD));
    mCopyToClipboardBtn->setMouseLeaveCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseLeave, this, _1, _2, TOOLTIP_COPY_TO_CLIPBOARD));

    mPasteFromClipboardBtn = getChild<LLButton>("paste_from_clipboard");
    mPasteFromClipboardBtn->setClickedCallback(onPasteFromClipboard, (void*)this);
    mPasteFromClipboardBtn->setMouseEnterCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseEnter, this, _1, _2, TOOLTIP_COPY_FROM_CLIPBOARD));
    mPasteFromClipboardBtn->setMouseLeaveCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseLeave, this, _1, _2, TOOLTIP_COPY_FROM_CLIPBOARD));

    mRemoveImageBtn = getChild<LLButton>("remove_image");
    mRemoveImageBtn->setClickedCallback(onRemove, (void*)this);
    mRemoveImageBtn->setMouseEnterCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseEnter, this, _1, _2, TOOLTIP_REMOVE));
    mRemoveImageBtn->setMouseLeaveCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseLeave, this, _1, _2, TOOLTIP_REMOVE));

    return LLFloater::postBuild();
}

void LLFloaterChangeItemThumbnail::onOpen(const LLSD& key)
{
    if (!key.has("item_id") && !key.isUUID())
    {
        closeFloater();
    }

    if (key.isUUID())
    {
        mItemId = key.asUUID();
    }
    else
    {
        mItemId = key["item_id"].asUUID();
        mTaskId = key["task_id"].asUUID();
    }

    refreshFromInventory();
}

void LLFloaterChangeItemThumbnail::changed(U32 mask)
{
    //LLInventoryObserver

    if (mTaskId.notNull() || mItemId.isNull())
    {
        // Task inventory or not set up yet
        return;
    }

    const std::set<LLUUID>& mChangedItemIDs = gInventory.getChangedIDs();
    std::set<LLUUID>::const_iterator it;

    for (it = mChangedItemIDs.begin(); it != mChangedItemIDs.end(); it++)
    {
        // set dirty for 'item profile panel' only if changed item is the item for which 'item profile panel' is shown (STORM-288)
        if (*it == mItemId)
        {
            // if there's a change we're interested in.
            if ((mask & (LLInventoryObserver::LABEL | LLInventoryObserver::INTERNAL | LLInventoryObserver::REMOVE)) != 0)
            {
                refreshFromInventory();
            }
        }
    }
}

void LLFloaterChangeItemThumbnail::inventoryChanged(LLViewerObject* object,
    LLInventoryObject::object_list_t* inventory,
    S32 serial_num,
    void* user_data)
{
    //LLVOInventoryListener
    refreshFromInventory();
}

LLViewerInventoryItem* LLFloaterChangeItemThumbnail::getItem()
{
    LLViewerInventoryItem* item = NULL;
    if (mTaskId.isNull())
    {
        // it is in agent inventory
        if (!mObserverInitialized)
        {
            gInventory.addObserver(this);
            mObserverInitialized = true;
        }

        item = gInventory.getItem(mItemId);
    }
    else
    {
        LLViewerObject* object = gObjectList.findObject(mTaskId);
        if (object)
        {
            if (!mObserverInitialized)
            {
                registerVOInventoryListener(object, NULL);
                mObserverInitialized = false;
            }

            item = static_cast<LLViewerInventoryItem*>(object->getInventoryObject(mItemId));
        }
    }
    return item;
}

void LLFloaterChangeItemThumbnail::refreshFromInventory()
{
    LLViewerInventoryItem* item = getItem();
    if (!item)
    {
        closeFloater();
    }

    if (item)
    {
        // This floater probably shouldn't be be possible to open
        // for imcomplete items
        llassert(item->isFinished());

        const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
        bool in_trash = (item->getUUID() == trash_id) || gInventory.isObjectDescendentOf(item->getUUID(), trash_id);
        if (in_trash)
        {
            // Close properties when moving to trash
            // Aren't supposed to view properties from trash
            closeFloater();
        }
        else
        {
            refreshFromItem(item);
        }
    }
    else
    {
        closeFloater();
    }
}

void LLFloaterChangeItemThumbnail::refreshFromItem(LLViewerInventoryItem* item)
{
    LLUIImagePtr icon_img = LLInventoryIcon::getIcon(item->getType(), item->getInventoryType(), item->getFlags(), FALSE);
    mItemTypeIcon->setImage(icon_img);
    mItemNameText->setValue(item->getName());

    LLUUID thumbnail_id = item->getThumbnailUUID();
    mThumbnailCtrl->setValue(thumbnail_id);

    mCopyToClipboardBtn->setEnabled(thumbnail_id.notNull());
    mRemoveImageBtn->setEnabled(thumbnail_id.notNull() && (item->getActualType() != LLAssetType::AT_TEXTURE) || (item->getAssetUUID() != thumbnail_id));

    // todo: some elements might not support setting thumbnails
    // since they already have them
}

void LLFloaterChangeItemThumbnail::onUploadLocal(void *userdata)
{

}

void LLFloaterChangeItemThumbnail::onUploadSnapshot(void *userdata)
{

}

void LLFloaterChangeItemThumbnail::onUseTexture(void *userdata)
{
    LLFloaterChangeItemThumbnail *self = (LLFloaterChangeItemThumbnail*)userdata;
    LLViewerInventoryItem* item = self->getItem();
    if (item)
    {
        self->showTexturePicker(item->getThumbnailUUID());
    }

}

void LLFloaterChangeItemThumbnail::onCopyToClipboard(void *userdata)
{
    LLFloaterChangeItemThumbnail *self = (LLFloaterChangeItemThumbnail*)userdata;
    LLViewerInventoryItem* item = self->getItem();
    if (item)
    {
        LLClipboard::instance().addToClipboard(item->getThumbnailUUID());
    }
}

void LLFloaterChangeItemThumbnail::onPasteFromClipboard(void *userdata)
{
    LLFloaterChangeItemThumbnail *self = (LLFloaterChangeItemThumbnail*)userdata;
    std::vector<LLUUID> objects;
    LLClipboard::instance().pasteFromClipboard(objects);
    if (objects.size() > 0)
    {
        LLViewerInventoryItem* item = self->getItem();
        if (item)
        {
            item->setThumbnailUUID(objects[0]);
        }
    }
}

void LLFloaterChangeItemThumbnail::onRemove(void *userdata)
{
    LLFloaterChangeItemThumbnail *self = (LLFloaterChangeItemThumbnail*)userdata;
    LLViewerInventoryItem* item = self->getItem();
    if (item)
    {
        item->setThumbnailUUID(LLUUID::null);
    }
}

void LLFloaterChangeItemThumbnail::showTexturePicker(const LLUUID &thumbnail_id)
{
    // show hourglass cursor when loading inventory window
    getWindow()->setCursor(UI_CURSOR_WAIT);

    LLFloater* floaterp = mPickerHandle.get();
    // Show the dialog
    if (floaterp)
    {
        floaterp->openFloater();
    }
    else
    {
        floaterp = new LLFloaterTexturePicker(
            this,
            thumbnail_id,
            thumbnail_id,
            thumbnail_id,
            FALSE,
            TRUE,
            "SELECT PHOTO",
            PERM_NONE,
            PERM_NONE,
            PERM_NONE,
            FALSE,
            NULL);

        mPickerHandle = floaterp->getHandle();

        LLFloaterTexturePicker* texture_floaterp = dynamic_cast<LLFloaterTexturePicker*>(floaterp);
        if (texture_floaterp)
        {
            //texture_floaterp->setTextureSelectedCallback();
            //texture_floaterp->setOnUpdateImageStatsCallback();
            texture_floaterp->setOnFloaterCommitCallback([this](LLTextureCtrl::ETexturePickOp op, LLUUID id)
            {
                if (op == LLTextureCtrl::TEXTURE_SELECT)
                {
                    onTexturePickerCommit(id);
                }
            }
            );

            texture_floaterp->setLocalTextureEnabled(FALSE);
            texture_floaterp->setBakeTextureEnabled(FALSE);
            texture_floaterp->setCanApplyImmediately(false);
            texture_floaterp->setCanApply(false, true);
        }

        floaterp->openFloater();
    }
    floaterp->setFocus(TRUE);
}

void LLFloaterChangeItemThumbnail::onTexturePickerCommit(LLUUID id)
{
    LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mPickerHandle.get();
    LLViewerInventoryItem* item = getItem();

    if (item && floaterp)
    {
        item->setThumbnailUUID(floaterp->getAssetID());
    }
}

void LLFloaterChangeItemThumbnail::onButtonMouseEnter(LLUICtrl* button, const LLSD& param, EToolTipState state)
{
    mTooltipState = state;

    std::string tooltip_text;
    std::string tooltip_name = "tooltip_" + button->getName();
    if (hasString(tooltip_name))
    {
        tooltip_text = getString(tooltip_name);
    }

    mToolTipTextBox->setValue(tooltip_text);
}

void LLFloaterChangeItemThumbnail::onButtonMouseLeave(LLUICtrl* button, const LLSD& param, EToolTipState state)
{
    if (mTooltipState == state)
    {
        mTooltipState = TOOLTIP_NONE;
        LLSD tooltip_text;
        mToolTipTextBox->setValue(tooltip_text);
    }
}

