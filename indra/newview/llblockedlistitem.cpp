/**
 * @file llviewerobjectlistitem.cpp
 * @brief viewer object list item implementation
 *
 * Class LLPanelInventoryListItemBase displays inventory item as an element
 * of LLInventoryItemsList.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "llblockedlistitem.h"

// llui
#include "lliconctrl.h"
#include "lltextbox.h"
#include "lltextutil.h"

// newview
#include "llavatariconctrl.h"
#include "llinventoryicon.h"
#include "llviewerobject.h"

LLBlockedListItem::LLBlockedListItem(const LLMute* item)
:	LLPanel(),
	mItemID(item->mID),
	mItemName(item->mName),
	mMuteType(item->mType)
{
	buildFromFile("panel_blocked_list_item.xml");
}

BOOL LLBlockedListItem::postBuild()
{
	mTitleCtrl = getChild<LLTextBox>("item_name");
	mTitleCtrl->setValue(mItemName);

	switch (mMuteType)
	{
	case LLMute::AGENT:
		{
			LLAvatarIconCtrl* avatar_icon = getChild<LLAvatarIconCtrl>("avatar_icon");
			avatar_icon->setVisible(TRUE);
			avatar_icon->setValue(mItemID);
		}
		break;

	case LLMute::OBJECT:
		getChild<LLUICtrl>("object_icon")->setVisible(TRUE);
		break;

	default:
		break;
	}

	return TRUE;
}

void LLBlockedListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	getChildView("hovered_icon")->setVisible(true);
	LLPanel::onMouseEnter(x, y, mask);
}

void LLBlockedListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	getChildView("hovered_icon")->setVisible(false);
	LLPanel::onMouseLeave(x, y, mask);
}

void LLBlockedListItem::setValue(const LLSD& value)
{
	if (!value.isMap() || !value.has("selected"))
	{
		return;
	}

	getChildView("selected_icon")->setVisible(value["selected"]);
}

void LLBlockedListItem::highlightName(const std::string& highlited_text)
{
	LLStyle::Params params;
	LLTextUtil::textboxSetHighlightedVal(mTitleCtrl, params, mItemName, highlited_text);
}
