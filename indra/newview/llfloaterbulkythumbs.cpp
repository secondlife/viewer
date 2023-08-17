/**
 * @file llfloaterbulkythumbs.cpp
 * @author Callum Prentice
 * @brief LLFloaterBulkyThumbs class implementation
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

/**
 * Floater that appears when buying an object, giving a preview
 * of its contents and their permissions.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterbulkythumbs.h"
#include "lluictrlfactory.h"
#include "llclipboard.h"
#include "llinventorymodel.h"
#include "llinventoryfunctions.h"
#include "lltexteditor.h"
#include "llmediactrl.h"
#include "lluuid.h"
#include "llaisapi.h"

LLFloaterBulkyThumbs::LLFloaterBulkyThumbs(const LLSD& key)
    :   LLFloater("floater_bulky_thumbs")
{
}

LLFloaterBulkyThumbs::~LLFloaterBulkyThumbs()
{
}

BOOL LLFloaterBulkyThumbs::postBuild()
{
    mPasteItemsBtn = getChild<LLUICtrl>("paste_items_btn");
    mPasteItemsBtn->setCommitCallback(boost::bind(&LLFloaterBulkyThumbs::onPasteItems, this));

    mPasteTexturesBtn = getChild<LLUICtrl>("paste_textures_btn");
    mPasteTexturesBtn->setCommitCallback(boost::bind(&LLFloaterBulkyThumbs::onPasteTextures, this));

    mOutputLog = getChild<LLTextEditor>("output_log");
    mOutputLog->setMaxTextLength(0xffff * 0x10);

    mMergeItemsTexturesBtn = getChild<LLUICtrl>("merge_items_textures");
    mMergeItemsTexturesBtn->setCommitCallback(boost::bind(&LLFloaterBulkyThumbs::onMergeItemsTextures, this));
    mMergeItemsTexturesBtn->setEnabled(false);

    mWriteThumbnailsBtn = getChild<LLUICtrl>("write_items_thumbnails");
    mWriteThumbnailsBtn->setCommitCallback(boost::bind(&LLFloaterBulkyThumbs::onWriteThumbnails, this));
    mWriteThumbnailsBtn->setEnabled(false);

    mDisplayThumbnaillessItemsBtn = getChild<LLUICtrl>("display_thumbless_items");
    mDisplayThumbnaillessItemsBtn->setCommitCallback(boost::bind(&LLFloaterBulkyThumbs::onDisplayThumbnaillessItems, this));
    mDisplayThumbnaillessItemsBtn->setEnabled(true);

    return true;
}

void LLFloaterBulkyThumbs::recordInventoryItemEntry(LLViewerInventoryItem* item)
{
    const std::string name = item->getName();

    std::map<std::string, LLUUID>::iterator iter = mItemNamesIDs.find(name);
    if (iter == mItemNamesIDs.end())
    {
        LLUUID id = item->getUUID();
        mItemNamesIDs.insert({name, id});

        mOutputLog->appendText(
            STRINGIZE(
                "ITEM " << mItemNamesIDs.size() << "> " <<
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

    if (item->getThumbnailUUID() == LLUUID::null)
    {
        mOutputLog->appendText(
            STRINGIZE(
                "ITEM " << mItemNamesIDs.size() << "> " <<
                name <<
                " has no thumbnail" <<
                //id.asString() <<
                std::endl
            ), false);
    }
}

void LLFloaterBulkyThumbs::onPasteItems()
{
    if (!LLClipboard::instance().hasContents())
    {
        return;
    }

    mOutputLog->appendText(
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

    mOutputLog->setCursorAndScrollToEnd();

    mMergeItemsTexturesBtn->setEnabled(true);
}

void LLFloaterBulkyThumbs::recordTextureItemEntry(LLViewerInventoryItem* item)
{
    const std::string name = item->getName();

    std::map<std::string, LLUUID>::iterator iter = mTextureNamesIDs.find(name);
    if (iter == mTextureNamesIDs.end())
    {
        LLUUID id = item->getAssetUUID();
        mTextureNamesIDs.insert({name, id});

        mOutputLog->appendText(
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

void LLFloaterBulkyThumbs::onPasteTextures()
{
    if (!LLClipboard::instance().hasContents())
    {
        return;
    }

    mOutputLog->appendText(
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

    mOutputLog->setCursorAndScrollToEnd();

    mMergeItemsTexturesBtn->setEnabled(true);
}

void LLFloaterBulkyThumbs::onMergeItemsTextures()
{
    mOutputLog->appendText(
        STRINGIZE(
            "\n==== Matching items and textures for " <<
            mItemNamesIDs.size() <<
            " entries ====" <<
            std::endl
        ), false);

    std::map<std::string, LLUUID>::iterator item_iter = mItemNamesIDs.begin();
    size_t index = 1;

    while (item_iter != mItemNamesIDs.end())
    {
        std::string item_name = (*item_iter).first;

        mOutputLog->appendText(
            STRINGIZE(
                "MATCHING ITEM (" << index++  << "/" << mItemNamesIDs.size() << ") " << item_name << "> "
            ), false);

        std::map<std::string, LLUUID>::iterator texture_iter = mTextureNamesIDs.find(item_name);
        if (texture_iter != mTextureNamesIDs.end())
        {
            mOutputLog->appendText(
                STRINGIZE(
                    "MATCHED" <<
                    std::endl
                ), false);

            mNameItemIDTextureId.insert({item_name, {(*item_iter).second, (*texture_iter).second}});
        }
        else
        {
            mOutputLog->appendText(
                STRINGIZE(
                    "NO MATCH FOUND" <<
                    std::endl
                ), false);
        }

        ++item_iter;
    }

    mOutputLog->appendText(
        STRINGIZE(
            "==== Matched list of items and textures has " <<
            mNameItemIDTextureId.size() <<
            " entries ====" <<
            std::endl
        ), true);

    //std::map<std::string, std::pair< LLUUID, LLUUID>>::iterator iter = mNameItemIDTextureId.begin();
    //while (iter != mNameItemIDTextureId.end())
    //{
    //    std::string output_line = (*iter).first;
    //    output_line += "\n";
    //    output_line += "item ID: ";
    //    output_line += ((*iter).second).first.asString();
    //    output_line += "\n";
    //    output_line += "thumbnail texture ID: ";
    //    output_line += ((*iter).second).second.asString();
    //    output_line +=  "\n";
    //    mOutputLog->appendText(output_line, true);

    //    ++iter;
    //}
    mOutputLog->setCursorAndScrollToEnd();

    mWriteThumbnailsBtn->setEnabled(true);
}

#if 1
// *TODO$: LLInventoryCallback should be deprecated to conform to the new boost::bind/coroutine model.
// temp code in transition
void bulkyInventoryCb(LLPointer<LLInventoryCallback> cb, LLUUID id)
{
    if (cb.notNull())
    {
        cb->fire(id);
    }
}
#endif

bool writeThumbnailID(LLUUID item_id, LLUUID thumbnail_asset_id)
{
    if (AISAPI::isAvailable())
    {

        LLSD updates;
        updates["thumbnail"] = LLSD().with("asset_id", thumbnail_asset_id.asString());

        LLPointer<LLInventoryCallback> cb;

        AISAPI::completion_t cr = boost::bind(&bulkyInventoryCb, cb, _1);
        AISAPI::UpdateItem(item_id, updates, cr);

        return true;
    }
    else
    {
        LL_WARNS() << "Unable to write inventory thumbnail because the AIS API is not available" << LL_ENDL;
        return false;
    }
}

void LLFloaterBulkyThumbs::onWriteThumbnails()
{
    mOutputLog->appendText(
        STRINGIZE(
            "\n==== Writing thumbnails for " <<
            mNameItemIDTextureId.size() <<
            " entries ====" <<
            std::endl
        ), false);

    std::map<std::string, std::pair< LLUUID, LLUUID>>::iterator iter = mNameItemIDTextureId.begin();
    size_t index = 1;

    while (iter != mNameItemIDTextureId.end())
    {
        mOutputLog->appendText(
            STRINGIZE(
                "WRITING THUMB (" << index++  << "/" << mNameItemIDTextureId.size() << ")> " <<
                (*iter).first <<
                "\n" <<
                "item ID: " <<
                ((*iter).second).first.asString() <<
                "\n" <<
                "thumbnail texture ID: " <<
                ((*iter).second).second.asString() <<
                "\n"
            ), true);

        LLUUID item_id = ((*iter).second).first;
        LLUUID thumbnail_asset_id = ((*iter).second).second;

        writeThumbnailID(item_id, thumbnail_asset_id);

        ++iter;
    }
    mOutputLog->setCursorAndScrollToEnd();
}

void LLFloaterBulkyThumbs::onDisplayThumbnaillessItems()
{
    //std::map<std::string, LLUUID>::iterator item_iter = mItemNamesIDs.begin();
    //size_t index = 1;

    //mOutputLog->appendText("======= Items with no thumbnail =======", false);

    //while (item_iter != mItemNamesIDs.end())
    //{
    //    std::string item_name = (*item_iter).first;

    //    //mOutputLog->appendText(
    //    //    STRINGIZE(
    //    //        "MATCHING ITEM (" << index++  << "/" << mItemNamesIDs.size() << ") " << item_name << "> "
    //    //    ), false);

    //    std::map<std::string, LLUUID>::iterator texture_iter = mTextureNamesIDs.find(item_name);
    //    if (texture_iter == mTextureNamesIDs.end())
    //    {
    //        mOutputLog->appendText(
    //            STRINGIZE(
    //                "MISSING:" <<

    //                std::endl
    //            ), false);

    //    }
    //}
}
