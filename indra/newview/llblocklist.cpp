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

#include "llviewerprecompiledheaders.h"

#include "llblocklist.h"

#include "llavataractions.h"
#include "llblockedlistitem.h"
#include "llfloatersidepanelcontainer.h"
#include "llviewermenu.h"

static LLDefaultChildRegistry::Register<LLBlockList> r("block_list");

static const LLBlockListNameComparator 		NAME_COMPARATOR;
static const LLBlockListNameTypeComparator	NAME_TYPE_COMPARATOR;

LLBlockList::LLBlockList(const Params& p)
:	LLFlatListViewEx(p),
 	mDirty(true),
	mShouldAddAll(true),
	mActionType(NONE),
	mMuteListSize(0)
{

	LLMuteList::getInstance()->addObserver(this);
	mMuteListSize = LLMuteList::getInstance()->getMutes().size();

	// Set up context menu.
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

	registrar.add		("Block.Action",	boost::bind(&LLBlockList::onCustomAction,	this, _2));
	enable_registrar.add("Block.Enable",	boost::bind(&LLBlockList::isActionEnabled,	this, _2));
	enable_registrar.add("Block.Check",     boost::bind(&LLBlockList::isMenuItemChecked, this, _2));
	enable_registrar.add("Block.Visible",   boost::bind(&LLBlockList::isMenuItemVisible, this, _2));
	
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

void LLBlockList::createList()
{
	std::vector<LLMute> mutes = LLMuteList::instance().getMutes();
	std::vector<LLMute>::const_iterator mute_it = mutes.begin();

	for (; mute_it != mutes.end(); ++mute_it)
	{
		addNewItem(&*mute_it);
	}
}

BlockListActionType LLBlockList::getCurrentMuteListActionType()
{
	BlockListActionType type = NONE;
	U32 curSize = LLMuteList::getInstance()->getMutes().size();
	if( curSize > mMuteListSize)
		type = ADD;
	else if(curSize < mMuteListSize)
		type = REMOVE;

	return type;
}

void LLBlockList::onChangeDetailed(const LLMute &mute)
{
	mActionType = getCurrentMuteListActionType();

	mCurItemId = mute.mID;
	mCurItemName = mute.mName;
	mCurItemType = mute.mType;
	mCurItemFlags = mute.mFlags;

	refresh();
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

void LLBlockList::removeListItem(const LLMute* mute)
{
	if (mute->mID.notNull())
	{
		removeItemByUUID(mute->mID);
	}
	else
	{
		removeItemByValue(mute->mName);
	}
}

void LLBlockList::hideListItem(LLBlockedListItem* item, bool show)
{
	item->setVisible(show);
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
	if (item->getUUID().notNull())
	{
		addItem(item, item->getUUID(), ADD_BOTTOM);
	}
	else
	{
		addItem(item, item->getName(), ADD_BOTTOM);
	}
}

void LLBlockList::refresh()
{
	bool have_filter = !mNameFilter.empty();

	// save selection to restore it after list rebuilt
	LLSD selected = getSelectedValue();
	LLSD next_selected;

	if(mShouldAddAll)	// creating list of blockers
	{
		clear();
		createList();
		mShouldAddAll = false;
	}
	else
	{
		// handle remove/add functionality
		LLMute mute(mCurItemId, mCurItemName, mCurItemType, mCurItemFlags);
		if(mActionType == ADD)
		{
			addNewItem(&mute);
		}
		else if(mActionType == REMOVE)
		{
			if ((mute.mID.notNull() && selected.isUUID() && selected.asUUID() == mute.mID)
				|| (mute.mID.isNull() && selected.isString() && selected.asString() == mute.mName))
			{
				// we are going to remove currently selected item, so select next item and save the selection to restore it
				if (!selectNextItemPair(false, true))
				{
					selectNextItemPair(true, true);
				}
				next_selected = getSelectedValue();
			}
			removeListItem(&mute);
		}
		mActionType = NONE;
	}

	// handle filter functionality
	if(have_filter || (!have_filter && !mPrevNameFilter.empty()))
	{
		// we should update visibility of our items if previous filter was not empty
		std::vector < LLPanel* > allItems;
		getItems(allItems);
		std::vector < LLPanel* >::iterator it = allItems.begin();

		for(; it != allItems.end() ; ++it)
		{
			LLBlockedListItem * curItem = dynamic_cast<LLBlockedListItem *> (*it);
			if(curItem)
	{
				hideListItem(curItem, findInsensitive(curItem->getName(), mNameFilter));
			}
		}
	}
	mPrevNameFilter = mNameFilter;

	if (selected.isDefined())
	{
		if (getItemPair(selected))
		{
			// restore previously selected item
			selectItemPair(getItemPair(selected), true);
		}
		else if (next_selected.isDefined() && getItemPair(next_selected))
		{
			// previously selected item was removed, so select next item
			selectItemPair(getItemPair(next_selected), true);
		}
	}
	mMuteListSize = LLMuteList::getInstance()->getMutes().size();

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

	if ("profile_item" == command_name 
		|| "block_voice" == command_name
		|| "block_text" == command_name
		|| "block_particles" == command_name
		|| "block_obj_sounds" == command_name)
	{
		LLBlockedListItem* item = getBlockedItem();
		action_enabled = item && (LLMute::AGENT == item->getType());
	}

	if ("unblock_item" == command_name)
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

		default:
			break;
		}
	}
	else if ("block_voice" == command_name)
	{
		toggleMute(LLMute::flagVoiceChat);
	}
	else if ("block_text" == command_name)
	{
		toggleMute(LLMute::flagTextChat);
	}
	else if ("block_particles" == command_name)
	{
		toggleMute(LLMute::flagParticles);
	}
	else if ("block_obj_sounds" == command_name)
	{
		toggleMute(LLMute::flagObjectSounds);
	}
}

bool LLBlockList::isMenuItemChecked(const LLSD& userdata)
{
	LLBlockedListItem* item = getBlockedItem();
	if (!item)
	{
		return false;
	}

	const std::string command_name = userdata.asString();

	if ("block_voice" == command_name)
	{
		return LLMuteList::getInstance()->isMuted(item->getUUID(), LLMute::flagVoiceChat);
	}
	else if ("block_text" == command_name)
	{
		return LLMuteList::getInstance()->isMuted(item->getUUID(), LLMute::flagTextChat);
	}
	else if ("block_particles" == command_name)
	{
		return LLMuteList::getInstance()->isMuted(item->getUUID(), LLMute::flagParticles);
	}
	else if ("block_obj_sounds" == command_name)
	{
		return LLMuteList::getInstance()->isMuted(item->getUUID(), LLMute::flagObjectSounds);
	}

	return false;
}

bool LLBlockList::isMenuItemVisible(const LLSD& userdata)
{
	LLBlockedListItem* item = getBlockedItem();
	const std::string command_name = userdata.asString();

	if ("block_voice" == command_name
		|| "block_text" == command_name
		|| "block_particles" == command_name
		|| "block_obj_sounds" == command_name)
	{
		return item && (LLMute::AGENT == item->getType());
	}

	return false;
}

void LLBlockList::toggleMute(U32 flags)
{
	LLBlockedListItem* item = getBlockedItem();
	LLMute mute(item->getUUID(), item->getName(), item->getType());

	if (!LLMuteList::getInstance()->isMuted(item->getUUID(), flags))
	{
		LLMuteList::getInstance()->add(mute, flags);
	}
	else
	{
		LLMuteList::getInstance()->remove(mute, flags);
	}
}

bool LLBlockListItemComparator::compare(const LLPanel* item1, const LLPanel* item2) const
{
	const LLBlockedListItem* blocked_item1 = dynamic_cast<const LLBlockedListItem*>(item1);
	const LLBlockedListItem* blocked_item2 = dynamic_cast<const LLBlockedListItem*>(item2);

	if (!blocked_item1 || !blocked_item2)
	{
		LL_ERRS() << "blocked_item1 and blocked_item2 cannot be null" << LL_ENDL;
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

	// if mute type is LLMute::BY_NAME or LLMute::OBJECT it means that this mute is an object
	bool both_mutes_are_objects = (LLMute::OBJECT == type1 || LLMute::BY_NAME == type1) && (LLMute::OBJECT == type2 || LLMute::BY_NAME == type2);

	// mute types may be different, but since both LLMute::BY_NAME and LLMute::OBJECT types represent objects
	// it's needed to perform additional checking of both_mutes_are_objects variable
	if (type1 != type2 && !both_mutes_are_objects)
	{
		// objects in block list go first, so return true if mute type is not an avatar
		return LLMute::AGENT != type1;
	}

	return NAME_COMPARATOR.compare(blocked_item1, blocked_item2);
}
