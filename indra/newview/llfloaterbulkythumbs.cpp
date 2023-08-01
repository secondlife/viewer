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


LLFloaterBulkyThumbs::LLFloaterBulkyThumbs(const LLSD &key)
	:	LLFloater("floater_bulky_thumbs")
{
}

LLFloaterBulkyThumbs::~LLFloaterBulkyThumbs() {}

BOOL LLFloaterBulkyThumbs::postBuild()
{
    mPasteFromInventoryBtn = getChild<LLUICtrl>("paste_from_inventory");
    mPasteFromInventoryBtn->setCommitCallback(boost::bind(&LLFloaterBulkyThumbs::onPasteFromInventory, this));

    mProcessBulkyThumbsBtn = getChild<LLUICtrl>("process_bulky_thumbs");
    mProcessBulkyThumbsBtn->setCommitCallback(boost::bind(&LLFloaterBulkyThumbs::onProcessBulkyThumbs, this));
    mProcessBulkyThumbsBtn->setEnabled(false);

    return true;
}


void LLFloaterBulkyThumbs::onPasteFromInventory()
{ std::cout << "onPasteFromInventory" << std::endl; }

void LLFloaterBulkyThumbs::onProcessBulkyThumbs()
{ std::cout << "onProcessBulkyThumbs" << std::endl; }
