/**
 * @file llblocklist.cpp
 * @brief List of the blocked avatars and objects.
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

#include "llavataractions.h"
#include "llblocklist.h"
#include "llblockedlistitem.h"
#include "llfloatersidepanelcontainer.h"
#include "llviewermenu.h"

static LLDefaultChildRegistry::Register<LLBlockList> r("block_list");

static const LLBlockListNameComparator 		NAME_COMPARATOR;
static const LLBlockListNameTypeComparator	NAME_TYPE_COMPARATOR;

LLBlockList::LLBlockList(const Params& p)
:	LLFlatListViewEx(p),
 	mSelectedItem(NULL),
 	mDirty(true)
{

	LLMuteList::getInstance()->addObserver(this);

	// Set up context menu.
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

	registrar.add		("Block.Action",	boost::bind(&LLBlockList::onCustomAction,	this, _2));
	enable_registrar.add("Block.Enable",	boost::bind(&LLBlockList::isActionEnabled,	this, _2));

	LLToggleableMenu* context_menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
									"menu_people_blocked_gear.xml",
									gMenuHolder,
									LLViewerMenuHolderGL::child_registry_t::instance());
	if(context_menu)
	{
		mContextMenu = context_menu->getHandle();
	}
}

LLBlockList::~LLBlockList()
{
	if (mContextMenu.get())
	{
		mContextMenu.get()->die();
	}

	LLMuteList::getInstance()->removeObserver(this);
}

BOOL LLBlockList::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLUICtrl::handleRightMouseDown(x, y, mask);

	LLToggleableMenu* context_menu = mContextMenu.get();
	if (context_menu && size())
	{
		context_menu->buildDrawLabels();
		context_menu->updateParent(LLMenuGL::sMenuContainer);
		LLMenuGL::showPopup(this, context_menu, x, y);
	}

	return handled;
}

void LLBlockList::setNameFilter(const std::string& filter)
{
	std::string filter_upper = filter;
	LLStringUtil::toUpper(filter_upper);
	if (mNameFilter != filter_upper)
	{
		mNameFilter = filter_upper;
		setDirty();
	}
}

void LLBlockList::sortByName()
{
	setComparator(&NAME_COMPARATOR);
	sort();
}

void LLBlockList::sortByType()
{
	setComparator(&NAME_TYPE_COMPARATOR);
	sort();
}

void LLBlockList::draw()
{
	if (mDirty)
	{
		refresh();
	}

	LLFlatListView::draw();
}

void LLBlockList::addNewItem(const LLMute* mute)
{
	LLBlockedListItem* item = new LLBlockedListItem(mute);
	if (!mNameFilter.empty())
	{
		item->highlightName(mNameFilter);
	}
	addItem(item, item->getUUID(), ADD_BOTTOM);
}

void LLBlockList::refresh()
{
	bool have_filter = !mNameFilter.empty();

	// save selection to restore it after list rebuilt
	LLUUID selected = getSelectedUUID();

	// calling refresh may be initiated by removing currently selected item
	// so select next item and save the selection to restore it after list rebuilt
	if (!selectNextItemPair(false, true))
	{
		selectNextItemPair(true, true);
	}
	LLUUID next_selected = getSelectedUUID();

	clear();

	std::vector<LLMute> mutes = LLMuteList::instance().getMutes();
	std::vector<LLMute>::const_iterator mute_it = mutes.begin();

	for (; mute_it != mutes.end(); ++mute_it)
	{
		if (have_filter && !findInsensitive(mute_it->mName, mNameFilter))
			continue;

		addNewItem(&*mute_it);
	}

	if (getItemPair(selected))
	{
		// restore previously selected item
		selectItemPair(getItemPair(selected), true);
	}
	else if (getItemPair(next_selected))
	{
		// previously selected item was removed, so select next item
		selectItemPair(getItemPair(next_selected), true);
	}

	// Sort the list.
	sort();

	setDirty(false);
}

bool LLBlockList::findInsensitive(std::string haystack, const std::string& needle_upper)
{
    LLStringUtil::toUpper(haystack);
    return haystack.find(needle_upper) != std::string::npos;
}

LLBlockedListItem* LLBlockList::getBlockedItem() const
{
	LLPanel* panel = LLFlatListView::getSelectedItem();
	LLBlockedListItem* item = dynamic_cast<LLBlockedListItem*>(panel);
	return item;
}

bool LLBlockList::isActionEnabled(const LLSD& userdata)
{
	bool action_enabled = true;

	const std::string command_name = userdata.asString();

	if ("unblock_item" == command_name || "profile_item" == command_name)
	{
		action_enabled = getSelectedItem() != NULL;
	}

	return action_enabled;
}

void LLBlockList::onCustomAction(const LLSD& userdata)
{
	if (!isActionEnabled(userdata))
	{
		return;
	}

	LLBlockedListItem* item = getBlockedItem();
	const std::string command_name = userdata.asString();

	if ("unblock_item" == command_name)
	{
		LLMute mute(item->getUUID(), item->getName());
		LLMuteList::getInstance()->remove(mute);
	}
	else if ("profile_item" == command_name)
	{
		switch(item->getType())
		{

		case LLMute::AGENT:
			LLAvatarActions::showProfile(item->getUUID());
			break;

		case LLMute::OBJECT:
			LLFloaterSidePanelContainer::showPanel("inventory", LLSD().with("id", item->getUUID()));
			break;

		default:
			break;
		}
	}
}

bool LLBlockListItemComparator::compare(const LLPanel* item1, const LLPanel* item2) const
{
	const LLBlockedListItem* blocked_item1 = dynamic_cast<const LLBlockedListItem*>(item1);
	const LLBlockedListItem* blocked_item2 = dynamic_cast<const LLBlockedListItem*>(item2);

	if (!blocked_item1 || !blocked_item2)
	{
		llerror("blocked_item1 and blocked_item2 cannot be null", 0);
		return true;
	}

	return doCompare(blocked_item1, blocked_item2);
}

bool LLBlockListNameComparator::doCompare(const LLBlockedListItem* blocked_item1, const LLBlockedListItem* blocked_item2) const
{
	std::string name1 = blocked_item1->getName();
	std::string name2 = blocked_item2->getName();

	LLStringUtil::toUpper(name1);
	LLStringUtil::toUpper(name2);

	return name1 < name2;
}

bool LLBlockListNameTypeComparator::doCompare(const LLBlockedListItem* blocked_item1, const LLBlockedListItem* blocked_item2) const
{
	LLMute::EType type1 = blocked_item1->getType();
	LLMute::EType type2 = blocked_item2->getType();

	if (type1 != type2)
	{
		return type1 > type2;
	}

	return NAME_COMPARATOR.compare(blocked_item1, blocked_item2);
}
