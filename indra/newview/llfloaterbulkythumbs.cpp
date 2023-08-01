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
#include "lltexteditor.h"

LLFloaterBulkyThumbs::LLFloaterBulkyThumbs(const LLSD& key)
    :   LLFloater("floater_bulky_thumbs")
{
}

LLFloaterBulkyThumbs::~LLFloaterBulkyThumbs() {}

BOOL LLFloaterBulkyThumbs::postBuild()
{
    mPasteFromInventoryBtn = getChild<LLUICtrl>("paste_from_inventory");
    mPasteFromInventoryBtn->setCommitCallback(boost::bind(&LLFloaterBulkyThumbs::onPasteFromInventory, this));

    mInventoryItems = getChild<LLTextEditor>("inventory_items");
    mInventoryItems->setMaxTextLength(0x8000);
    mInventoryItems->setCommitCallback(boost::bind(&LLFloaterBulkyThumbs::onCommitInventoryItems, this));

    mProcessBulkyThumbsBtn = getChild<LLUICtrl>("process_bulky_thumbs");
    mProcessBulkyThumbsBtn->setCommitCallback(boost::bind(&LLFloaterBulkyThumbs::onProcessBulkyThumbs, this));
    mProcessBulkyThumbsBtn->setEnabled(false);

    return true;
}


void LLFloaterBulkyThumbs::onPasteFromInventory()
{
    std::cout << "onPasteFromInventory" << std::endl;

    if (!LLClipboard::instance().hasContents())
    {
        return;
    }

    std::vector<LLUUID> objects;
    LLClipboard::instance().pasteFromClipboard(objects);
    size_t count = objects.size();

    if (count > 0)
    {
        mInventoryItems->clear();

        for (size_t i = 0; i < count; i++)
        {
            const LLUUID& item_id = objects.at(i);

            const LLInventoryItem* item = gInventory.getItem(item_id);
            if (item)
            {
                if (item->getType() == LLAssetType::AT_OBJECT)
                {
                    mInventoryItems->appendText(item->getName(), "\n");
                }
            }
        }

        mProcessBulkyThumbsBtn->setEnabled(true);
    }
}

void LLFloaterBulkyThumbs::onProcessBulkyThumbs()
{
    std::cout << "onProcessBulkyThumbs" << std::endl;
}

void LLFloaterBulkyThumbs::onCommitInventoryItems()
{
    std::cout << "onCommitInventoryItems" << std::endl;
}
