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
#include "lluuid.h"

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

    mInventoryItems = getChild<LLTextEditor>("inventory_items");
    mInventoryItems->setMaxTextLength(0xffff * 0x10);

    mProcessBulkyThumbsBtn = getChild<LLUICtrl>("process_bulky_thumbs");
    mProcessBulkyThumbsBtn->setCommitCallback(boost::bind(&LLFloaterBulkyThumbs::onProcessBulkyThumbs, this));
    mProcessBulkyThumbsBtn->setEnabled(false);

    mWriteBulkyThumbsBtn = getChild<LLUICtrl>("write_bulky_thumbs");
    mWriteBulkyThumbsBtn->setCommitCallback(boost::bind(&LLFloaterBulkyThumbs::onWriteBulkyThumbs, this));
    mWriteBulkyThumbsBtn->setEnabled(false);

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

        std::string output_line = "ITEM: ";
        output_line += name;
        output_line += "|";
        output_line += id.asString();
        output_line +=  "\n";
        mInventoryItems->appendText(output_line, false);
    }
    else
    {
        // dupe - do not save
    }
}

void LLFloaterBulkyThumbs::recordTextureItemEntry(LLViewerInventoryItem* item)
{
    const std::string name = item->getName();

    std::map<std::string, LLUUID>::iterator iter = mTextureNamesIDs.find(name);
    if (iter == mTextureNamesIDs.end())
    {
        LLUUID id = item->getUUID();
        mTextureNamesIDs.insert({name, id});

        std::string output_line = "TEXR: ";
        output_line += name;
        output_line += "|";
        output_line += id.asString();
        output_line +=  "\n";
        mInventoryItems->appendText(output_line, false);
    }
    else
    {
        // dupe - do not save
    }
}

void LLFloaterBulkyThumbs::onPasteItems()
{
    if (!LLClipboard::instance().hasContents())
    {
        return;
    }

    std::vector<LLUUID> objects;
    LLClipboard::instance().pasteFromClipboard(objects);
    size_t count = objects.size();

    if (count > 0)
    {
        for (size_t i = 0; i < count; i++)
        {
            const LLUUID& item_id = objects.at(i);

            const LLInventoryCategory* cat = gInventory.getCategory(item_id);
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


                for (size_t i = 0; i < item_array.size(); i++)
                {
                    LLViewerInventoryItem* item = item_array.at(i);
                    recordInventoryItemEntry(item);
                }
            }

            LLViewerInventoryItem* item = gInventory.getItem(item_id);
            if (item)
            {
                if (item->getType() == LLAssetType::AT_OBJECT)
                {
                    recordInventoryItemEntry(item);
                }
            }
        }

        mInventoryItems->setCursorAndScrollToEnd();

        mProcessBulkyThumbsBtn->setEnabled(true);
    }
}

void LLFloaterBulkyThumbs::onPasteTextures()
{
    if (!LLClipboard::instance().hasContents())
    {
        return;
    }

    std::vector<LLUUID> objects;
    LLClipboard::instance().pasteFromClipboard(objects);
    size_t count = objects.size();

    if (count > 0)
    {
        for (size_t i = 0; i < count; i++)
        {
            const LLUUID& item_id = objects.at(i);

            const LLInventoryCategory* cat = gInventory.getCategory(item_id);
            if (cat)
            {
                LLInventoryModel::cat_array_t cat_array;
                LLInventoryModel::item_array_t item_array;
                LLIsType is_texture(LLAssetType::AT_TEXTURE);
                gInventory.collectDescendentsIf(cat->getUUID(),
                                                cat_array,
                                                item_array,
                                                LLInventoryModel::EXCLUDE_TRASH,
                                                is_texture);

                std::cout << "CAT: found " << cat_array.size() << " additional categories" << std::endl;
                std::cout << "ITEM: found " << item_array.size() << " items" << std::endl;
                for (size_t i = 0; i < item_array.size(); i++)
                {
                    LLViewerInventoryItem* item = item_array.at(i);
                    recordTextureItemEntry(item);
                }
            }

            LLViewerInventoryItem* item = gInventory.getItem(item_id);
            if (item)
            {
                if (item->getType() == LLAssetType::AT_TEXTURE)
                {
                    recordTextureItemEntry(item);
                }
            }
        }

        mInventoryItems->setCursorAndScrollToEnd();

        mProcessBulkyThumbsBtn->setEnabled(true);
    }
}

void LLFloaterBulkyThumbs::onProcessBulkyThumbs()
{
    std::map<std::string, LLUUID>::iterator item_iter = mItemNamesIDs.begin();
    while (item_iter != mItemNamesIDs.end())
    {
        std::string output_line = "PROCESSING: ";
        std::string item_name = (*item_iter).first;
        output_line += item_name;
        output_line +=  "\n";
        mInventoryItems->appendText(output_line, false);

        bool found = false;
        std::map<std::string, LLUUID>::iterator texture_iter = mTextureNamesIDs.begin();
        while (texture_iter != mTextureNamesIDs.end())
        {
            std::string output_line = "    COMPARING WITH: ";
            std::string texture_name = (*texture_iter).first;
            output_line += texture_name;

            if (item_name == texture_name)
            {
                output_line += " MATCH";
                found = true;
                output_line +=  "\n";
                mInventoryItems->appendText(output_line, false);
                break;
            }
            else
            {
                output_line += " NO MATCH";
            }
            output_line +=  "\n";
            mInventoryItems->appendText(output_line, false);
            mInventoryItems->setCursorAndScrollToEnd();


            ++texture_iter;
        }

        if (found == true)
        {
            mNameItemIDTextureId.insert({item_name, {(*item_iter).second, (*texture_iter).second}});
        }
        else
        {
            std::string output_line = "WARNING: ";
            output_line += "No texture found for ";
            output_line += item_name;
            output_line +=  "\n";
            mInventoryItems->appendText(output_line, false);
            mInventoryItems->setCursorAndScrollToEnd();

        }

        ++item_iter;
    }

    mInventoryItems->appendText("Finished - final list is", true);
    std::map<std::string, std::pair< LLUUID, LLUUID>>::iterator iter = mNameItemIDTextureId.begin();
    while (iter != mNameItemIDTextureId.end())
    {
        std::string output_line = (*iter).first;
        output_line += "\n";
        output_line += "item ID: ";
        output_line += ((*iter).second).first.asString();
        output_line += "\n";
        output_line += "thumbnail texture ID: ";
        output_line += ((*iter).second).second.asString();
        output_line +=  "\n";
        mInventoryItems->appendText(output_line, true);

        ++iter;
    }
    mInventoryItems->setCursorAndScrollToEnd();

    mWriteBulkyThumbsBtn->setEnabled(true);
}

void LLFloaterBulkyThumbs::onWriteBulkyThumbs()
{
	// look at  LLFloaterChangeItemThumbnail::onOpen(const LLSD& key)

    mInventoryItems->appendText("Writing thumbnails", true);
    std::map<std::string, std::pair< LLUUID, LLUUID>>::iterator iter = mNameItemIDTextureId.begin();
    while (iter != mNameItemIDTextureId.end())
    {
        std::string output_line = (*iter).first;
        output_line += "\n";
        output_line += "item ID: ";
        output_line += ((*iter).second).first.asString();
        output_line += "\n";
        output_line += "thumbnail texture ID: ";
        output_line += ((*iter).second).second.asString();
        output_line +=  "\n";
        mInventoryItems->appendText(output_line, true);

        ++iter;
    }
    mInventoryItems->setCursorAndScrollToEnd();
}
