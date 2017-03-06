/**
 * @file llconversationloglist.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llavataractions.h"
#include "llagent.h"
#include "llfloaterreg.h"
#include "llfloaterconversationpreview.h"
#include "llgroupactions.h"
#include "llconversationloglist.h"
#include "llconversationloglistitem.h"
#include "llviewermenu.h"
#include "lltrans.h"

static LLDefaultChildRegistry::Register<LLConversationLogList> r("conversation_log_list");

static LLConversationLogListNameComparator NAME_COMPARATOR;
static LLConversationLogListDateComparator DATE_COMPARATOR;

LLConversationLogList::LLConversationLogList(const Params& p)
:	LLFlatListViewEx(p),
	mIsDirty(true)
{
	LLConversationLog::instance().addObserver(this);

	// Set up context menu.
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar check_registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

	registrar.add		("Calllog.Action",	boost::bind(&LLConversationLogList::onCustomAction,	this, _2));
	check_registrar.add ("Calllog.Check",	boost::bind(&LLConversationLogList::isActionChecked,this, _2));
	enable_registrar.add("Calllog.Enable",	boost::bind(&LLConversationLogList::isActionEnabled,this, _2));

	LLToggleableMenu* context_menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
									"menu_conversation_log_gear.xml",
									gMenuHolder,
									LLViewerMenuHolderGL::child_registry_t::instance());
	if(context_menu)
	{
		mContextMenu = context_menu->getHandle();
	}

	mIsFriendsOnTop = gSavedSettings.getBOOL("SortFriendsFirst");
}

LLConversationLogList::~LLConversationLogList()
{
	if (mContextMenu.get())
	{
		mContextMenu.get()->die();
	}

	LLConversationLog::instance().removeObserver(this);
}

void LLConversationLogList::draw()
{
	if (mIsDirty)
	{
		refresh();
	}
	LLFlatListViewEx::draw();
}

BOOL LLConversationLogList::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLUICtrl::handleRightMouseDown(x, y, mask);

	LLToggleableMenu* context_menu = mContextMenu.get();
	{
		context_menu->buildDrawLabels();
	if (context_menu && size())
		context_menu->updateParent(LLMenuGL::sMenuContainer);
		LLMenuGL::showPopup(this, context_menu, x, y);
	}

	return handled;
}

void LLConversationLogList::setNameFilter(const std::string& filter)
{
	std::string filter_upper = filter;
	LLStringUtil::toUpper(filter_upper);
	if (mNameFilter != filter_upper)
	{
		mNameFilter = filter_upper;
		setDirty();
	}
}

bool LLConversationLogList::findInsensitive(std::string haystack, const std::string& needle_upper)
{
    LLStringUtil::toUpper(haystack);
    return haystack.find(needle_upper) != std::string::npos;
}

void LLConversationLogList::sortByName()
{
	setComparator(&NAME_COMPARATOR);
	sort();
}

void LLConversationLogList::sortByDate()
{
	setComparator(&DATE_COMPARATOR);
	sort();
}

void LLConversationLogList::toggleSortFriendsOnTop()
{
	mIsFriendsOnTop = !mIsFriendsOnTop;
	gSavedSettings.setBOOL("SortFriendsFirst", mIsFriendsOnTop);
	sort();
}

void LLConversationLogList::changed()
{
	refresh();
}

void LLConversationLogList::changed(const LLUUID& session_id, U32 mask)
{
	LLConversationLogListItem* item = getConversationLogListItem(session_id);

	if (!item)
	{
		return;
	}

	if (mask & LLConversationLogObserver::CHANGED_TIME)
	{
		item->updateTimestamp();

		// if list is sorted by date and a date of some item has changed,
		// than the whole list should be rebuilt
		if (E_SORT_BY_DATE == getSortOrder())
		{
			mIsDirty = true;
		}
	}
	else if (mask & LLConversationLogObserver::CHANGED_NAME)
	{
		item->updateName();
		// if list is sorted by name and a name of some item has changed,
		// than the whole list should be rebuilt
		if (E_SORT_BY_DATE == getSortOrder())
		{
			mIsDirty = true;
		}
	}
	else if (mask & LLConversationLogObserver::CHANGED_OfflineIMs)
	{
		item->updateOfflineIMs();
	}
}

void LLConversationLogList::addNewItem(const LLConversation* conversation)
{
	LLConversationLogListItem* item = new LLConversationLogListItem(&*conversation);
	if (!mNameFilter.empty())
	{
		item->highlightNameDate(mNameFilter);
	}
	addItem(item, conversation->getSessionID(), ADD_TOP);
}

void LLConversationLogList::refresh()
{
	rebuildList();
	sort();

	mIsDirty = false;
}

void LLConversationLogList::rebuildList()
{
	const LLConversation * selected_conversationp = getSelectedConversation();

	clear();

	bool have_filter = !mNameFilter.empty();
	LLConversationLog &log_instance = LLConversationLog::instance();

	const std::vector<LLConversation>& conversations = log_instance.getConversations();
	std::vector<LLConversation>::const_iterator iter = conversations.begin();

	for (; iter != conversations.end(); ++iter)
	{
		bool not_found = have_filter && !findInsensitive(iter->getConversationName(), mNameFilter) && !findInsensitive(iter->getTimestamp(), mNameFilter);
		if (not_found)
			continue;

		addNewItem(&*iter);
	}

	// try to restore selection of item
	if (NULL != selected_conversationp)
	{
		selectItemByUUID(selected_conversationp->getSessionID());
	}

	bool logging_enabled = log_instance.getIsLoggingEnabled();
	bool log_empty = log_instance.isLogEmpty();
	if (!logging_enabled && log_empty)
	{
		setNoItemsCommentText(LLTrans::getString("logging_calls_disabled_log_empty"));
	}
	else if (!logging_enabled && !log_empty)
	{
		setNoItemsCommentText(LLTrans::getString("logging_calls_disabled_log_not_empty"));
	}
	else if (logging_enabled && log_empty)
	{
		setNoItemsCommentText(LLTrans::getString("logging_calls_enabled_log_empty"));
	}
	else if (logging_enabled && !log_empty)
	{
		setNoItemsCommentText("");
	}
}

void LLConversationLogList::onCustomAction(const LLSD& userdata)
{
	const LLConversation * selected_conversationp = getSelectedConversation();

	if (NULL == selected_conversationp)
	{
		return;
	}

	const std::string command_name = userdata.asString();
	const LLUUID& selected_conversation_participant_id = selected_conversationp->getParticipantID();
	const LLUUID& selected_conversation_session_id = selected_conversationp->getSessionID();
	LLIMModel::LLIMSession::SType stype = getSelectedSessionType();

	if ("im" == command_name)
	{
		switch (stype)
		{
		case LLIMModel::LLIMSession::P2P_SESSION:
			LLAvatarActions::startIM(selected_conversation_participant_id);
			break;

		case LLIMModel::LLIMSession::GROUP_SESSION:
			LLGroupActions::startIM(selected_conversation_session_id);
			break;

		default:
			break;
		}
	}
	else if ("call" == command_name)
	{
		switch (stype)
		{
		case LLIMModel::LLIMSession::P2P_SESSION:
			LLAvatarActions::startCall(selected_conversation_participant_id);
			break;

		case LLIMModel::LLIMSession::GROUP_SESSION:
			LLGroupActions::startCall(selected_conversation_session_id);
			break;

		default:
			break;
		}
	}
	else if ("view_profile" == command_name)
	{
		switch (stype)
		{
		case LLIMModel::LLIMSession::P2P_SESSION:
			LLAvatarActions::showProfile(selected_conversation_participant_id);
			break;

		case LLIMModel::LLIMSession::GROUP_SESSION:
			LLGroupActions::show(selected_conversation_session_id);
			break;

		default:
			break;
		}
	}
	else if ("chat_history" == command_name)
	{
		LLFloaterReg::showInstance("preview_conversation", selected_conversation_session_id, true);
	}
	else if ("offer_teleport" == command_name)
	{
		LLAvatarActions::offerTeleport(selected_conversation_participant_id);
	}
	else if ("request_teleport" == command_name)
	{
		LLAvatarActions::teleportRequest(selected_conversation_participant_id);
	}
	else if("add_friend" == command_name)
	{
		if (!LLAvatarActions::isFriend(selected_conversation_participant_id))
		{
			LLAvatarActions::requestFriendshipDialog(selected_conversation_participant_id);
		}
	}
	else if("remove_friend" == command_name)
	{
		if (LLAvatarActions::isFriend(selected_conversation_participant_id))
		{
			LLAvatarActions::removeFriendDialog(selected_conversation_participant_id);
		}
	}
	else if ("invite_to_group" == command_name)
	{
		LLAvatarActions::inviteToGroup(selected_conversation_participant_id);
	}
	else if ("show_on_map" == command_name)
	{
		LLAvatarActions::showOnMap(selected_conversation_participant_id);
	}
	else if ("share" == command_name)
	{
		LLAvatarActions::share(selected_conversation_participant_id);
	}
	else if ("pay" == command_name)
	{
		LLAvatarActions::pay(selected_conversation_participant_id);
	}
	else if ("block" == command_name)
	{
		LLAvatarActions::toggleBlock(selected_conversation_participant_id);
	}
}

bool LLConversationLogList::isActionEnabled(const LLSD& userdata)
{
	const LLConversation * selected_conversationp = getSelectedConversation();

	if (NULL == selected_conversationp || numSelected() > 1)
	{
		return false;
	}

	const std::string command_name = userdata.asString();

	LLIMModel::LLIMSession::SType stype = getSelectedSessionType();
	const LLUUID& selected_id = selected_conversationp->getParticipantID();

	bool is_p2p   = LLIMModel::LLIMSession::P2P_SESSION == stype;
	bool is_group = LLIMModel::LLIMSession::GROUP_SESSION == stype;
	bool is_group_member = is_group && gAgent.isInGroup(selected_id, TRUE);

	if ("can_im" == command_name)
	{
		return is_p2p || is_group_member;
	}
	else if ("can_view_profile" == command_name)
	{
		return is_p2p || is_group;
	}
	else if ("can_view_chat_history" == command_name)
	{
		return true;
	}
	else if ("can_call"	== command_name)
	{
		return (is_p2p || is_group_member) && LLAvatarActions::canCall();
	}
	else if ("add_rem_friend"		== command_name ||
			 "can_invite_to_group"	== command_name ||
			 "can_share"			== command_name ||
			 "can_block"			== command_name ||
			 "can_pay"				== command_name)
	{
		return is_p2p;
	}
	else if("can_offer_teleport" == command_name)
	{
		return is_p2p && LLAvatarActions::canOfferTeleport(selected_id);
	}
	else if ("can_show_on_map" == command_name)
	{
		return is_p2p && ((LLAvatarTracker::instance().isBuddyOnline(selected_id) && is_agent_mappable(selected_id)) || gAgent.isGodlike());
	}

	return false;
}

bool LLConversationLogList::isActionChecked(const LLSD& userdata)
{
	const LLConversation * selected_conversationp = getSelectedConversation();

	if (NULL == selected_conversationp)
	{
		return false;
	}

	const std::string command_name = userdata.asString();

	const LLUUID& selected_id = selected_conversationp->getParticipantID();
	bool is_p2p = LLIMModel::LLIMSession::P2P_SESSION == getSelectedSessionType();

	if ("is_blocked" == command_name)
	{
		return is_p2p && LLAvatarActions::isBlocked(selected_id);
	}
	else if ("is_friend" == command_name)
	{
		return is_p2p && LLAvatarActions::isFriend(selected_id);
	}
	else if ("is_not_friend" == command_name)
	{
		return is_p2p && !LLAvatarActions::isFriend(selected_id);
	}

	return false;
}

LLIMModel::LLIMSession::SType LLConversationLogList::getSelectedSessionType()
{
	const LLConversationLogListItem* item = getSelectedConversationPanel();

	if (item)
	{
		return item->getConversation()->getConversationType();
	}

	return LLIMModel::LLIMSession::NONE_SESSION;
}

const LLConversationLogListItem* LLConversationLogList::getSelectedConversationPanel()
{
	LLPanel* panel = LLFlatListViewEx::getSelectedItem();
	LLConversationLogListItem* conv_panel = dynamic_cast<LLConversationLogListItem*>(panel);

	return conv_panel;
}

const LLConversation* LLConversationLogList::getSelectedConversation()
{
	const LLConversationLogListItem* panel = getSelectedConversationPanel();

	if (panel)
	{
		return panel->getConversation();
	}

	return NULL;
}

LLConversationLogListItem* LLConversationLogList::getConversationLogListItem(const LLUUID& session_id)
{
	std::vector<LLPanel*> panels;
	LLFlatListViewEx::getItems(panels);
	std::vector<LLPanel*>::iterator iter = panels.begin();

	for (; iter != panels.end(); ++iter)
	{
		LLConversationLogListItem* item = dynamic_cast<LLConversationLogListItem*>(*iter);
		if (item && session_id == item->getConversation()->getSessionID())
		{
			return item;
		}
	}

	return NULL;
}

LLConversationLogList::ESortOrder LLConversationLogList::getSortOrder()
{
	return static_cast<ESortOrder>(gSavedSettings.getU32("CallLogSortOrder"));
}

bool LLConversationLogListItemComparator::compare(const LLPanel* item1, const LLPanel* item2) const
{
	const LLConversationLogListItem* conversation_item1 = dynamic_cast<const LLConversationLogListItem*>(item1);
	const LLConversationLogListItem* conversation_item2 = dynamic_cast<const LLConversationLogListItem*>(item2);

	if (!conversation_item1 || !conversation_item2)
	{
		LL_ERRS() << "conversation_item1 and conversation_item2 cannot be null" << LL_ENDL;
		return true;
	}

	return doCompare(conversation_item1, conversation_item2);
}

bool LLConversationLogListNameComparator::doCompare(const LLConversationLogListItem* conversation1, const LLConversationLogListItem* conversation2) const
{
	std::string name1 = conversation1->getConversation()->getConversationName();
	std::string name2 = conversation2->getConversation()->getConversationName();
	const LLUUID& id1 = conversation1->getConversation()->getParticipantID();
	const LLUUID& id2 = conversation2->getConversation()->getParticipantID();

	LLStringUtil::toUpper(name1);
	LLStringUtil::toUpper(name2);

	bool friends_first = gSavedSettings.getBOOL("SortFriendsFirst");
	if (friends_first && (LLAvatarActions::isFriend(id1) ^ LLAvatarActions::isFriend(id2)))
	{
		return LLAvatarActions::isFriend(id1);
	}

	return name1 < name2;
}

bool LLConversationLogListDateComparator::doCompare(const LLConversationLogListItem* conversation1, const LLConversationLogListItem* conversation2) const
{
	U64Seconds date1 = conversation1->getConversation()->getTime();
	U64Seconds date2 = conversation2->getConversation()->getTime();
	const LLUUID& id1 = conversation1->getConversation()->getParticipantID();
	const LLUUID& id2 = conversation2->getConversation()->getParticipantID();

	bool friends_first = gSavedSettings.getBOOL("SortFriendsFirst");
	if (friends_first && (LLAvatarActions::isFriend(id1) ^ LLAvatarActions::isFriend(id2)))
	{
		return LLAvatarActions::isFriend(id1);
	}

	return date1 > date2;
}
