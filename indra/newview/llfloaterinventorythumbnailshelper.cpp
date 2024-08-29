/**
 * @file llfloaterinventorythumbnailshelper.cpp
 * @author Callum Prentice
 * @brief LLFloaterInventoryThumbnailsHelper class implementation
 *
 * Usage instructions and some brief notes can be found in Confluence here:
 * https://lindenlab.atlassian.net/wiki/spaces/~174746736/pages/2928672843/Inventory+Thumbnail+Helper+Tool
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llaisapi.h"
#include "llclipboard.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "lltexteditor.h"
#include "lluictrlfactory.h"
#include "lluuid.h"

#include "llfloaterinventorythumbnailshelper.h"

LLFloaterInventoryThumbnailsHelper::LLFloaterInventoryThumbnailsHelper(const LLSD& key)
    :   LLFloater("floater_inventory_thumbnails_helper")
{
}

LLFloaterInventoryThumbnailsHelper::~LLFloaterInventoryThumbnailsHelper()
{
}

bool LLFloaterInventoryThumbnailsHelper::postBuild()
{
    mInventoryThumbnailsList = getChild<LLScrollListCtrl>("inventory_thumbnails_list");
    mInventoryThumbnailsList->setAllowMultipleSelection(true);

    mOutputLog = getChild<LLTextEditor>("output_log");
    mOutputLog->setMaxTextLength(0xffff * 0x10);

    mPasteItemsBtn = getChild<LLUICtrl>("paste_items_btn");
    mPasteItemsBtn->setCommitCallback(boost::bind(&LLFloaterInventoryThumbnailsHelper::onPasteItems, this));
    mPasteItemsBtn->setEnabled(true);

    mPasteTexturesBtn = getChild<LLUICtrl>("paste_textures_btn");
    mPasteTexturesBtn->setCommitCallback(boost::bind(&LLFloaterInventoryThumbnailsHelper::onPasteTextures, this));
    mPasteTexturesBtn->setEnabled(true);

    mWriteThumbnailsBtn = getChild<LLUICtrl>("write_thumbnails_btn");
    mWriteThumbnailsBtn->setCommitCallback(boost::bind(&LLFloaterInventoryThumbnailsHelper::onWriteThumbnails, this));
    mWriteThumbnailsBtn->setEnabled(false);

    mLogMissingThumbnailsBtn = getChild<LLUICtrl>("log_missing_thumbnails_btn");
    mLogMissingThumbnailsBtn->setCommitCallback(boost::bind(&LLFloaterInventoryThumbnailsHelper::onLogMissingThumbnails, this));
    mLogMissingThumbnailsBtn->setEnabled(false);

    mClearThumbnailsBtn = getChild<LLUICtrl>("clear_thumbnails_btn");
    mClearThumbnailsBtn->setCommitCallback(boost::bind(&LLFloaterInventoryThumbnailsHelper::onClearThumbnails, this));
    mClearThumbnailsBtn->setEnabled(false);

    return true;
}

// Records an entry in the pasted items - saves it to a map and writes it to the log
// window for later confirmation/validation - since it uses a map, duplicates (based on
// the name) are discarded
void LLFloaterInventoryThumbnailsHelper::recordInventoryItemEntry(LLViewerInventoryItem* item)
{
    const std::string name = item->getName();

    std::map<std::string, LLViewerInventoryItem*>::iterator iter = mItemNamesItems.find(name);
    if (iter == mItemNamesItems.end())
    {
        mItemNamesItems.insert({name, item});

        writeToLog(
            STRINGIZE(
                "ITEM " << mItemNamesItems.size() << "> " <<
                name <<
                std::endl
            ), false);
    }
    else
    {
        // dupe - do not save
    }
}

// Called when the user has copied items from their inventory and selects the Paste Items button
// in the UI - iterates over items and folders and saves details of each one.
// The first use of this tool is for updating NUX items and as such, only looks for OBJECTS,
// CLOTHING and BODYPARTS - later versions of this tool should make that selection editable.
void LLFloaterInventoryThumbnailsHelper::onPasteItems()
{
    if (!LLClipboard::instance().hasContents())
    {
        return;
    }

    writeToLog(
        STRINGIZE(
            "\n==== Pasting items from inventory ====" <<
            std::endl
        ), false);

    std::vector<LLUUID> objects;
    LLClipboard::instance().pasteFromClipboard(objects);
    size_t count = objects.size();

    for (size_t i = 0; i < count; i++)
    {
        const LLUUID& entry = objects.at(i);

        // Check for a folder
        const LLInventoryCategory* cat = gInventory.getCategory(entry);
        if (cat)
        {
            LLInventoryModel::cat_array_t cat_array;
            LLInventoryModel::item_array_t item_array;

            LLIsType is_object(LLAssetType::AT_OBJECT);
            gInventory.collectDescendentsIf(cat->getUUID(),
                                            cat_array,
                                            item_array,
                                            LLInventoryModel::EXCLUDE_TRASH,
                                            is_object);

            LLIsType is_bodypart(LLAssetType::AT_BODYPART);
            gInventory.collectDescendentsIf(cat->getUUID(),
                                            cat_array,
                                            item_array,
                                            LLInventoryModel::EXCLUDE_TRASH,
                                            is_bodypart);

            LLIsType is_clothing(LLAssetType::AT_CLOTHING);
            gInventory.collectDescendentsIf(cat->getUUID(),
                                            cat_array,
                                            item_array,
                                            LLInventoryModel::EXCLUDE_TRASH,
                                            is_clothing);

            for (size_t i = 0; i < item_array.size(); i++)
            {
                LLViewerInventoryItem* item = item_array.at(i);
                recordInventoryItemEntry(item);
            }
        }

        // Check for an item
        LLViewerInventoryItem* item = gInventory.getItem(entry);
        if (item)
        {
            const LLAssetType::EType item_type = item->getType();
            if (item_type == LLAssetType::AT_OBJECT || item_type == LLAssetType::AT_BODYPART || item_type == LLAssetType::AT_CLOTHING)
            {
                recordInventoryItemEntry(item);
            }
        }
    }

    // update the main list view based on what we found
    updateDisplayList();

    // update the buttons enabled state based on what we found/saved
    updateButtonStates();
}

// Records a entry in the pasted textures - saves it to a map and writes it to the log
// window for later confirmation/validation - since it uses a map, duplicates (based on
// the name) are discarded
void LLFloaterInventoryThumbnailsHelper::recordTextureItemEntry(LLViewerInventoryItem* item)
{
    const std::string name = item->getName();

    std::map<std::string, LLUUID>::iterator iter = mTextureNamesIDs.find(name);
    if (iter == mTextureNamesIDs.end())
    {
        LLUUID id = item->getAssetUUID();
        mTextureNamesIDs.insert({name, id});

        writeToLog(
            STRINGIZE(
                "TEXTURE " << mTextureNamesIDs.size() << "> " <<
                name <<
                //" | " <<
                //id.asString() <<
                std::endl
            ), false);
    }
    else
    {
        // dupe - do not save
    }
}

// Called when the user has copied textures from their inventory and selects the Paste Textures
// button in the UI - iterates over textures and folders and saves details of each one.
void LLFloaterInventoryThumbnailsHelper::onPasteTextures()
{
    if (!LLClipboard::instance().hasContents())
    {
        return;
    }

    writeToLog(
        STRINGIZE(
            "\n==== Pasting textures from inventory ====" <<
            std::endl
        ), false);

    std::vector<LLUUID> objects;
    LLClipboard::instance().pasteFromClipboard(objects);
    size_t count = objects.size();

    for (size_t i = 0; i < count; i++)
    {
        const LLUUID& entry = objects.at(i);

        const LLInventoryCategory* cat = gInventory.getCategory(entry);
        if (cat)
        {
            LLInventoryModel::cat_array_t cat_array;
            LLInventoryModel::item_array_t item_array;

            LLIsType is_object(LLAssetType::AT_TEXTURE);
            gInventory.collectDescendentsIf(cat->getUUID(),
                                            cat_array,
                                            item_array,
                                            LLInventoryModel::EXCLUDE_TRASH,
                                            is_object);

            for (size_t i = 0; i < item_array.size(); i++)
            {
                LLViewerInventoryItem* item = item_array.at(i);
                recordTextureItemEntry(item);
            }
        }

        LLViewerInventoryItem* item = gInventory.getItem(entry);
        if (item)
        {
            const LLAssetType::EType item_type = item->getType();
            if (item_type == LLAssetType::AT_TEXTURE)
            {
                recordTextureItemEntry(item);
            }
        }
    }

    // update the main list view based on what we found
    updateDisplayList();

    // update the buttons enabled state based on what we found/saved
    updateButtonStates();
}

// Updates the main list of entries in the UI based on what is in the maps/storage
void LLFloaterInventoryThumbnailsHelper::updateDisplayList()
{
    mInventoryThumbnailsList->deleteAllItems();

    std::map<std::string, LLViewerInventoryItem*>::iterator item_iter = mItemNamesItems.begin();
    while (item_iter != mItemNamesItems.end())
    {
        std::string item_name = (*item_iter).first;

        std::string existing_texture_name = std::string();
        LLUUID existing_thumbnail_id = (*item_iter).second->getThumbnailUUID();
        if (existing_thumbnail_id != LLUUID::null)
        {
            existing_texture_name = existing_thumbnail_id.asString();
        }
        else
        {
            existing_texture_name = "none";
        }

        std::string new_texture_name = std::string();
        std::map<std::string, LLUUID>::iterator texture_iter = mTextureNamesIDs.find(item_name);
        if (texture_iter != mTextureNamesIDs.end())
        {
            new_texture_name = (*texture_iter).first;
        }
        else
        {
            new_texture_name = "missing";
        }

        LLSD row;
        row["columns"][EListColumnNum::NAME]["column"] = "item_name";
        row["columns"][EListColumnNum::NAME]["type"] = "text";
        row["columns"][EListColumnNum::NAME]["value"] = item_name;
        row["columns"][EListColumnNum::NAME]["font"]["name"] = "Monospace";

        row["columns"][EListColumnNum::EXISTING_TEXTURE]["column"] = "existing_texture";
        row["columns"][EListColumnNum::EXISTING_TEXTURE]["type"] = "text";
        row["columns"][EListColumnNum::EXISTING_TEXTURE]["font"]["name"] = "Monospace";
        row["columns"][EListColumnNum::EXISTING_TEXTURE]["value"] = existing_texture_name;

        row["columns"][EListColumnNum::NEW_TEXTURE]["column"] = "new_texture";
        row["columns"][EListColumnNum::NEW_TEXTURE]["type"] = "text";
        row["columns"][EListColumnNum::NEW_TEXTURE]["font"]["name"] = "Monospace";
        row["columns"][EListColumnNum::NEW_TEXTURE]["value"] = new_texture_name;

        mInventoryThumbnailsList->addElement(row);

        ++item_iter;
    }
}

#if 1
// *TODO$: LLInventoryCallback should be deprecated to conform to the new boost::bind/coroutine model.
// temp code in transition
void inventoryThumbnailsHelperCb(LLPointer<LLInventoryCallback> cb, LLUUID id)
{
    if (cb.notNull())
    {
        cb->fire(id);
    }
}
#endif

// Makes calls to the AIS v3 API to record the local changes made to the thumbnails.
// If this is not called, the operations (e.g. set thumbnail or clear thumbnail)
// appear to work but do not push the changes back to the inventory (local cache view only)
bool writeInventoryThumbnailID(LLUUID item_id, LLUUID thumbnail_asset_id)
{
    if (AISAPI::isAvailable())
    {

        LLSD updates;
        updates["thumbnail"] = LLSD().with("asset_id", thumbnail_asset_id.asString());

        LLPointer<LLInventoryCallback> cb;

        AISAPI::completion_t cr = boost::bind(&inventoryThumbnailsHelperCb, cb, _1);
        AISAPI::UpdateItem(item_id, updates, cr);

        return true;
    }
    else
    {
        LL_WARNS() << "Unable to write inventory thumbnail because the AIS API is not available" << LL_ENDL;
        return false;
    }
}

// Called when the Write Thumbanils button is pushed. Iterates over the name/item and
// name/.texture maps and where it finds a common name, extracts what is needed and
// writes the thumbnail accordingly.
void LLFloaterInventoryThumbnailsHelper::onWriteThumbnails()
{
    // create and show confirmation (Yes/No) textbox since this is a destructive operation
    LLNotificationsUtil::add("WriteInventoryThumbnailsWarning", LLSD(), LLSD(),
                             [&](const LLSD & notif, const LLSD & resp)
    {
        S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
        if (opt == 0)
        {
            std::map<std::string, LLViewerInventoryItem*>::iterator item_iter = mItemNamesItems.begin();
            while (item_iter != mItemNamesItems.end())
            {
                std::string item_name = (*item_iter).first;

                std::map<std::string, LLUUID>::iterator texture_iter = mTextureNamesIDs.find(item_name);
                if (texture_iter != mTextureNamesIDs.end())
                {
                    LLUUID item_id = (*item_iter).second->getUUID();

                    LLUUID thumbnail_asset_id = (*texture_iter).second;

                    writeToLog(
                        STRINGIZE(
                            "WRITING THUMB " <<
                            (*item_iter).first <<
                            "\n" <<
                            "item ID: " <<
                            item_id <<
                            "\n" <<
                            "thumbnail texture ID: " <<
                            thumbnail_asset_id <<
                            "\n"
                        ), true);


                    (*item_iter).second->setThumbnailUUID(thumbnail_asset_id);

                    // This additional step (notifying AIS API) is required
                    // to make the changes persist outside of the local cache
                    writeInventoryThumbnailID(item_id, thumbnail_asset_id);
                }

                ++item_iter;
            }

            updateDisplayList();
        }
        else
        {
            LL_INFOS() << "Writing new thumbnails was canceled" << LL_ENDL;
        }
    });
}

// Called when the Log Items with Missing Thumbnails is selected. This merely writes
// a list of all the items for which the thumbnail ID is Null. Typical use case is to
// copy from the log window, pasted to Slack to illustrate which items are missing
// a thumbnail
void LLFloaterInventoryThumbnailsHelper::onLogMissingThumbnails()
{
    std::map<std::string, LLViewerInventoryItem*>::iterator item_iter = mItemNamesItems.begin();
    while (item_iter != mItemNamesItems.end())
    {
        LLUUID thumbnail_id = (*item_iter).second->getThumbnailUUID();

        if (thumbnail_id == LLUUID::null)
        {
            writeToLog(
                STRINGIZE(
                    "Missing thumbnail: " <<
                    (*item_iter).first <<
                    std::endl
                ), true);
        }

        ++item_iter;
    }
}

// Called when the Clear Thumbnail button is selected.  Code to perform the clear (really
// just writing a NULL UUID into the thumbnail field) is behind an "Are you Sure?" dialog
// since it cannot be undone and potentinally, you could remove the thumbnails from your
// whole inventory this way.
void LLFloaterInventoryThumbnailsHelper::onClearThumbnails()
{
    // create and show confirmation (Yes/No) textbox since this is a destructive operation
    LLNotificationsUtil::add("ClearInventoryThumbnailsWarning", LLSD(), LLSD(),
                             [&](const LLSD & notif, const LLSD & resp)
    {
        S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
        if (opt == 0)
        {
            std::map<std::string, LLViewerInventoryItem*>::iterator item_iter = mItemNamesItems.begin();
            while (item_iter != mItemNamesItems.end())
            {
                (*item_iter).second->setThumbnailUUID(LLUUID::null);

                // This additional step (notifying AIS API) is required
                // to make the changes persist outside of the local cache
                const LLUUID item_id = (*item_iter).second->getUUID();
                writeInventoryThumbnailID(item_id, LLUUID::null);

                ++item_iter;
            }

            updateDisplayList();
        }
        else
        {
            LL_INFOS() << "Clearing on thumbnails was canceled" << LL_ENDL;
        }
    });
}

// Update the endabled state of some of the UI buttons based on what has
// been recorded so far.  For example, if there are no valid item/texture pairs,
// then the Write Thumbnails button is not enabled.
void LLFloaterInventoryThumbnailsHelper::updateButtonStates()
{
    size_t found_count = 0;

    std::map<std::string, LLViewerInventoryItem*>::iterator item_iter = mItemNamesItems.begin();
    while (item_iter != mItemNamesItems.end())
    {
        std::string item_name = (*item_iter).first;

        std::map<std::string, LLUUID>::iterator texture_iter = mTextureNamesIDs.find(item_name);
        if (texture_iter != mTextureNamesIDs.end())
        {
            found_count++;
        }

        ++item_iter;
    }

    // the "Write Thumbnails" button is only enabled when there is at least one
    // item with a matching texture ready to be written to the thumbnail field
    if (found_count > 0)
    {
        mWriteThumbnailsBtn->setEnabled(true);
    }
    else
    {
        mWriteThumbnailsBtn->setEnabled(false);
    }

    // The "Log Missing Items" and "Clear Thumbnails" buttons are only enabled
    // when there is at least 1 item that was pasted from inventory (doesn't need
    // to have a matching texture for these operations)
    if (mItemNamesItems.size() > 0)
    {
        mLogMissingThumbnailsBtn->setEnabled(true);
        mClearThumbnailsBtn->setEnabled(true);
    }
    else
    {
        mLogMissingThumbnailsBtn->setEnabled(false);
        mClearThumbnailsBtn->setEnabled(false);
    }
}

// Helper function for writing a line to the log window. Currently the only additional
// feature is that it scrolls to the bottom each time a line is written but it
// is envisaged that other common actions will be added here eventually - E.G. write eavh
// line to the Second Life log too for example.
void LLFloaterInventoryThumbnailsHelper::writeToLog(std::string logline, bool prepend_newline)
{
    mOutputLog->appendText(logline, prepend_newline);

    mOutputLog->setCursorAndScrollToEnd();
}
