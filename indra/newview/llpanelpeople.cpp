/** 
 * @file llpanelpeople.cpp
 * @brief Side tray "People" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

// libs
#include "llfloaterreg.h"
#include "llmenugl.h"
#include "llsearcheditor.h"
#include "lltabcontainer.h"
#include "lluictrlfactory.h"

#include "llpanelpeople.h"

// newview
#include "llagent.h"
#include "llavatarlist.h"
#include "llcallingcard.h"			// for LLAvatarTracker
#include "llfloateravatarpicker.h"
#include "llfloaterminiinspector.h"
#include "llavataractions.h"
#include "llgroupactions.h"
#include "llgrouplist.h"
#include "llrecentpeople.h"
#include "llviewercontrol.h"		// for gSavedSettings
#include "llviewermenu.h"			// for gMenuHolder
#include "llvoiceclient.h"
#include "llworld.h"

using namespace LLOldEvents;

#define FRIEND_LIST_UPDATE_TIMEOUT	0.5
#define NEARBY_LIST_UPDATE_INTERVAL 1
#define RECENT_LIST_UPDATE_DELAY	1

static const std::string NEARBY_TAB_NAME	= "nearby_panel";
static const std::string FRIENDS_TAB_NAME	= "friends_panel";
static const std::string GROUP_TAB_NAME		= "groups_panel";
static const std::string RECENT_TAB_NAME	= "recent_panel";

static LLRegisterPanelClassWrapper<LLPanelPeople> t_people("panel_people");

//=============================================================================

/**
 * Updates given list either on regular basis or on external events (up to implementation). 
 */
class LLPanelPeople::Updater
{
public:
	typedef boost::function<bool(U32)> callback_t;
	Updater(callback_t cb)
	: mCallback(cb)
	{
	}

	virtual ~Updater()
	{
	}

	/**
	 * Force the list updates.
	 * 
	 * This may start repeated updates until all names are complete.
	 */
	virtual void forceUpdate() {}

	/**
	 * Activate/deactivate updater.
	 *
	 * This may start/stop regular updates.
	 */
	virtual void setActive(bool) {}

protected:
	bool updateList(U32 mask = 0)
	{
		return mCallback(mask);
	}

	callback_t		mCallback;
};

class LLAvatarListUpdater : public LLPanelPeople::Updater, public LLEventTimer
{
public:
	LLAvatarListUpdater(callback_t cb, F32 period)
	:	LLEventTimer(period),
		LLPanelPeople::Updater(cb)
	{
		mEventTimer.stop();
	}
};

/**
 * Updates the friends list.
 * 
 * Updates the list on external events which trigger the changed() method. 
 */
class LLFriendListUpdater : public LLAvatarListUpdater, public LLFriendObserver
{
	LOG_CLASS(LLFriendListUpdater);

public: 
	LLFriendListUpdater(callback_t cb)
	:	LLAvatarListUpdater(cb, FRIEND_LIST_UPDATE_TIMEOUT)
	{
		LLAvatarTracker::instance().addObserver(this);
		// For notification when SIP online status changes.
		LLVoiceClient::getInstance()->addObserver(this);
	}

	~LLFriendListUpdater()
	{
		LLVoiceClient::getInstance()->removeObserver(this);
		LLAvatarTracker::instance().removeObserver(this);
	}

	/*virtual*/ void forceUpdate()
	{
		// Perform updates until all names are loaded.
		if (!updateList(LLFriendObserver::ADD))
			changed(LLFriendObserver::ADD);
	}

	/*virtual*/ void changed(U32 mask)
	{
		// events can arrive quickly in bulk - we need not process EVERY one of them -
		// so we wait a short while to let others pile-in, and process them in aggregate.
		mEventTimer.start();

		// save-up all the mask-bits which have come-in
		mMask |= mask;
	}

	/*virtual*/ BOOL tick()
	{
		if (updateList(mMask))
		{
			// Got all names, stop updates.
			mEventTimer.stop();
			mMask = 0;
		}

		return FALSE;
	}

private:
	U32 mMask;
};

/**
 * Periodically updates the nearby people list while the Nearby tab is active.
 * 
 * The period is defined by NEARBY_LIST_UPDATE_INTERVAL constant.
 */
class LLNearbyListUpdater : public LLAvatarListUpdater
{
	LOG_CLASS(LLNearbyListUpdater);

public:
	LLNearbyListUpdater(callback_t cb)
	:	LLAvatarListUpdater(cb, NEARBY_LIST_UPDATE_INTERVAL)
	{
		setActive(false);
	}

	/*virtual*/ void setActive(bool val)
	{
		if (val)
		{
			// update immediately and start regular updates
			updateList();
			mEventTimer.start(); 
		}
		else
		{
			// stop regular updates
			mEventTimer.stop();
		}
	}

	/*virtual*/ void forceUpdate()
	{
		updateList();
	}

	/*virtual*/ BOOL tick()
	{
		updateList();
		return FALSE;
	}
private:
};

/**
 * Updates the recent people list (those the agent has recently interacted with).
 */
class LLRecentListUpdater : public LLAvatarListUpdater
{
	LOG_CLASS(LLRecentListUpdater);

public:
	LLRecentListUpdater(callback_t cb)
	:	LLAvatarListUpdater(cb, RECENT_LIST_UPDATE_DELAY)
	{
		LLRecentPeople::instance().setChangedCallback(boost::bind(&LLRecentListUpdater::onRecentPeopleChanged, this));
	}

private:
	/*virtual*/ void forceUpdate()
	{
		onRecentPeopleChanged();
	}
	
	/*virtual*/ BOOL tick()
	{
		// Update the list until we get all the names. 
		if (updateList())
		{
			// Got all names, stop updates.
			mEventTimer.stop();
		}

		return FALSE;
	}

	void onRecentPeopleChanged()
	{
		if (!updateList())
		{
			// Some names are incomplete, schedule another update.
			mEventTimer.start();
		}
	}
};

/**
 * Updates the group list on events from LLAgent.
 */
class LLGroupListUpdater : public LLPanelPeople::Updater, public LLSimpleListener
{
	LOG_CLASS(LLGroupListUpdater);

public:
	LLGroupListUpdater(callback_t cb)
	:	LLPanelPeople::Updater(cb)
	{
		gAgent.addListener(this, "new group");
	}

	~LLGroupListUpdater()
	{
		gAgent.removeListener(this);
	}

	/*virtual*/ void forceUpdate()
	{
		updateList();
	}

	/*virtual*/ bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// Why is "new group" sufficient?
		if (event->desc() == "new group")
		{
			updateList();
			return true;
		}

		return false;
	}
};

//=============================================================================

LLPanelPeople::LLPanelPeople()
	:	LLPanel(),
		mFilterSubString(LLStringUtil::null),
		mSearchEditor(NULL),
		mTabContainer(NULL),
		mFriendList(NULL),
		mNearbyList(NULL),
		mRecentList(NULL)
{
	mFriendListUpdater = new LLFriendListUpdater(boost::bind(&LLPanelPeople::onFriendListUpdate,this, _1));
	mNearbyListUpdater = new LLNearbyListUpdater(boost::bind(&LLPanelPeople::updateNearbyList,	this));
	mRecentListUpdater = new LLRecentListUpdater(boost::bind(&LLPanelPeople::updateRecentList,	this));
	mGroupListUpdater  = new LLGroupListUpdater	(boost::bind(&LLPanelPeople::updateGroupList,	this));
}

LLPanelPeople::~LLPanelPeople()
{
	delete mNearbyListUpdater;
	delete mFriendListUpdater;
	delete mRecentListUpdater;
	delete mGroupListUpdater;

	LLView::deleteViewByHandle(mGroupPlusMenuHandle);
}

BOOL LLPanelPeople::postBuild()
{
	mSearchEditor = getChild<LLSearchEditor>("filter_input");
	mSearchEditor->setSearchCallback(boost::bind(&LLPanelPeople::onSearchEdit, this, _1));

	mTabContainer = getChild<LLTabContainer>("tabs");
	mTabContainer->setCommitCallback(boost::bind(&LLPanelPeople::onTabSelected, this, _2));
	mTabContainer->selectTabByName(FRIENDS_TAB_NAME); // must go after setting commit callback

	mFriendList = getChild<LLPanel>(FRIENDS_TAB_NAME)->getChild<LLAvatarList>("avatar_list");
	mNearbyList = getChild<LLPanel>(NEARBY_TAB_NAME)->getChild<LLAvatarList>("avatar_list");
	mRecentList = getChild<LLPanel>(RECENT_TAB_NAME)->getChild<LLAvatarList>("avatar_list");
	mGroupList = getChild<LLGroupList>("group_list");

	LLPanel* groups_panel = getChild<LLPanel>(GROUP_TAB_NAME);
	groups_panel->childSetAction("activate_btn", boost::bind(&LLPanelPeople::onActivateButtonClicked,	this));
	groups_panel->childSetAction("plus_btn",	boost::bind(&LLPanelPeople::onGroupPlusButtonClicked,	this));
	groups_panel->childSetAction("minus_btn",	boost::bind(&LLPanelPeople::onGroupMinusButtonClicked,	this));

	LLPanel* friends_panel = getChild<LLPanel>(FRIENDS_TAB_NAME);
	friends_panel->childSetAction("add_btn",	boost::bind(&LLPanelPeople::onAddFriendWizButtonClicked,	this));
	friends_panel->childSetAction("del_btn",	boost::bind(&LLPanelPeople::onDeleteFriendButtonClicked,	this));

	mFriendList->setDoubleClickCallback(boost::bind(&LLPanelPeople::onAvatarListDoubleClicked, this, mFriendList));
	mNearbyList->setDoubleClickCallback(boost::bind(&LLPanelPeople::onAvatarListDoubleClicked, this, mNearbyList));
	mRecentList->setDoubleClickCallback(boost::bind(&LLPanelPeople::onAvatarListDoubleClicked, this, mRecentList));
	mFriendList->setCommitCallback(boost::bind(&LLPanelPeople::onAvatarListCommitted, this, mFriendList));
	mNearbyList->setCommitCallback(boost::bind(&LLPanelPeople::onAvatarListCommitted, this, mNearbyList));
	mRecentList->setCommitCallback(boost::bind(&LLPanelPeople::onAvatarListCommitted, this, mRecentList));

	mGroupList->setDoubleClickCallback(boost::bind(&LLPanelPeople::onGroupInfoButtonClicked, this));
	mGroupList->setCommitCallback(boost::bind(&LLPanelPeople::updateButtons, this));

	buttonSetAction("view_profile_btn",	boost::bind(&LLPanelPeople::onViewProfileButtonClicked,	this));
	buttonSetAction("add_friend_btn",	boost::bind(&LLPanelPeople::onAddFriendButtonClicked,	this));
	buttonSetAction("group_info_btn",	boost::bind(&LLPanelPeople::onGroupInfoButtonClicked,	this));
	buttonSetAction("chat_btn",			boost::bind(&LLPanelPeople::onChatButtonClicked,		this));
	buttonSetAction("im_btn",			boost::bind(&LLPanelPeople::onImButtonClicked,			this));
	buttonSetAction("call_btn",			boost::bind(&LLPanelPeople::onCallButtonClicked,		this));
	buttonSetAction("teleport_btn",		boost::bind(&LLPanelPeople::onTeleportButtonClicked,	this));
	buttonSetAction("share_btn",		boost::bind(&LLPanelPeople::onShareButtonClicked,		this));
	buttonSetAction("more_btn",			boost::bind(&LLPanelPeople::onMoreButtonClicked,		this));

	// Create menus.
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("People.Group.Plus.Action",  boost::bind(&LLPanelPeople::onGroupPlusMenuItemClicked,  this, _2));
	LLMenuGL* plus_menu  = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_group_plus.xml",  gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mGroupPlusMenuHandle  = plus_menu->getHandle();

	// Perform initial update.
	mFriendListUpdater->forceUpdate();
	updateGroupList();
	updateRecentList();

	return TRUE;
}

bool LLPanelPeople::updateFriendList(U32 changed_mask)
{
	// Refresh names.
	if (changed_mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::ONLINE))
	{
		// get all buddies we know about
		LLAvatarTracker::buddy_map_t all_buddies;
		LLAvatarTracker::instance().copyBuddyList(all_buddies);

		// *TODO: it's suboptimal to rebuild the whole list on online status change.

		// convert the buddy map to vector
		mFriendVec.clear();
		LLAvatarTracker::buddy_map_t::const_iterator buddy_it = all_buddies.begin();
		for (; buddy_it != all_buddies.end(); ++buddy_it)
			mFriendVec.push_back(buddy_it->first);

		return filterFriendList();
	}

	return true;
}

bool LLPanelPeople::updateNearbyList()
{
	LLWorld::getInstance()->getAvatars(&mNearbyVec, NULL, gAgent.getPositionGlobal(), gSavedSettings.getF32("NearMeRange"));
	filterNearbyList();

	return true;
}

bool LLPanelPeople::updateRecentList()
{
	LLRecentPeople::instance().get(mRecentVec);
	filterRecentList();

	return true;
}

bool LLPanelPeople::updateGroupList()
{
	bool have_names = mGroupList->update(mFilterSubString);

	if (mGroupList->isEmpty())
		mGroupList->setCommentText(getString("no_groups"));

	return have_names;
}

bool LLPanelPeople::filterFriendList()
{
	if (mFriendVec.size() > 0)
		return mFriendList->update(mFriendVec, mFilterSubString);

	mFriendList->setCommentText(getString("no_friends"));
	return true;
}

bool LLPanelPeople::filterNearbyList()
{
	bool have_names = mNearbyList->update(mNearbyVec, mFilterSubString);

	if (mNearbyVec.size() == 0)
		mNearbyList->setCommentText(getString("no_one_near"));

	return have_names;
}

bool LLPanelPeople::filterRecentList()
{
	if (mRecentVec.size() > 0)
		return mRecentList->update(mRecentVec, mFilterSubString);

	mRecentList->setCommentText(getString("no_people"));

	return true;
}

void LLPanelPeople::buttonSetVisible(std::string btn_name, BOOL visible)
{
	// Currently all bottom buttons are wrapped with layout panels.
	// Hiding a button has no effect: the panel still occupies its space.
	// So we have to hide the whole panel (along with its button)
	// to free some space up.
	LLButton* btn = getChild<LLView>("button_bar")->getChild<LLButton>(btn_name);
	LLPanel* btn_parent = dynamic_cast<LLPanel*>(btn->getParent());
	if (btn_parent)
		btn_parent->setVisible(visible);
}

void LLPanelPeople::buttonSetEnabled(const std::string& btn_name, bool enabled)
{
	// To make sure we're referencing the right widget (a child of the button bar).
	LLButton* button = getChild<LLView>("button_bar")->getChild<LLButton>(btn_name);
	button->setEnabled(enabled);
}

void LLPanelPeople::buttonSetAction(const std::string& btn_name, const commit_signal_t::slot_type& cb)
{
	// To make sure we're referencing the right widget (a child of the button bar).
	LLButton* button = getChild<LLView>("button_bar")->getChild<LLButton>(btn_name);
	button->setClickedCallback(cb);
}

void LLPanelPeople::updateButtons()
{
	std::string cur_tab		= mTabContainer->getCurrentPanel()->getName();
	bool nearby_tab_active	= (cur_tab == NEARBY_TAB_NAME);
	bool friends_tab_active = (cur_tab == FRIENDS_TAB_NAME);
	bool group_tab_active	= (cur_tab == GROUP_TAB_NAME);
	bool recent_tab_active	= (cur_tab == RECENT_TAB_NAME);
	LLUUID selected_id;

	buttonSetVisible("group_info_btn",		group_tab_active);
	buttonSetVisible("chat_btn",			group_tab_active);
	buttonSetVisible("add_friend_btn",		nearby_tab_active || recent_tab_active);
	buttonSetVisible("view_profile_btn",	!group_tab_active);
	buttonSetVisible("im_btn",				!group_tab_active);
	buttonSetVisible("teleport_btn",		friends_tab_active || group_tab_active);
	buttonSetVisible("share_btn",			!recent_tab_active && false); // not implemented yet

	if (group_tab_active)
	{
		bool item_selected = mGroupList->getFirstSelected() != NULL;
		bool cur_group_active = true;

		if (item_selected)
		{
			selected_id = mGroupList->getCurrentID();
			cur_group_active = (gAgent.getGroupID() == selected_id);
		}
	
		LLPanel* groups_panel = mTabContainer->getCurrentPanel();
		groups_panel->childSetEnabled("activate_btn",	item_selected && !cur_group_active); // "none" or a non-active group selected
		groups_panel->childSetEnabled("plus_btn",		item_selected);
		groups_panel->childSetEnabled("minus_btn",		item_selected && selected_id.notNull());
	}
	else
	{
		bool is_friend = true;
		LLAvatarList* list;

		// Check whether selected avatar is our friend.
		if ((list = getActiveAvatarList()) && (selected_id = list->getCurrentID()).notNull())
		{
			is_friend = LLAvatarTracker::instance().getBuddyInfo(selected_id) != NULL;
		}

		childSetEnabled("add_friend_btn",	!is_friend);
	}

	bool item_selected = selected_id.notNull();
	buttonSetEnabled("teleport_btn",		friends_tab_active && item_selected);
	buttonSetEnabled("view_profile_btn",	item_selected);
	buttonSetEnabled("im_btn",				item_selected);
	buttonSetEnabled("call_btn",			item_selected && false); // not implemented yet
	buttonSetEnabled("share_btn",			item_selected && false); // not implemented yet
	buttonSetEnabled("group_info_btn",		item_selected);
	buttonSetEnabled("chat_btn",			item_selected);
}

LLAvatarList* LLPanelPeople::getActiveAvatarList() const
{
	std::string cur_tab = mTabContainer->getCurrentPanel()->getName();

	if (cur_tab == FRIENDS_TAB_NAME)
		return mFriendList;
	if (cur_tab == NEARBY_TAB_NAME)
		return mNearbyList;
	if (cur_tab == RECENT_TAB_NAME)
		return mRecentList;

	return NULL;
}

LLUUID LLPanelPeople::getCurrentItemID() const
{
	LLAvatarList* alist = getActiveAvatarList();
	if (alist)
		return alist->getCurrentID();
	return mGroupList->getCurrentID();
}

void LLPanelPeople::showGroupMenu(LLMenuGL* menu)
{
	// Shows the menu at the top of the button bar.

	// Calculate its coordinates.
	// (assumes that groups panel is the current tab)
	LLPanel* bottom_panel = mTabContainer->getCurrentPanel()->getChild<LLPanel>("bottom_panel"); 
	LLPanel* parent_panel = mTabContainer->getCurrentPanel();
	menu->arrangeAndClear();
	S32 menu_height = menu->getRect().getHeight();
	S32 menu_x = -2; // *HACK: compensates HPAD in showPopup()
	S32 menu_y = bottom_panel->getRect().mTop + menu_height;

	// Actually show the menu.
	menu->buildDrawLabels();
	menu->updateParent(LLMenuGL::sMenuContainer);
	LLMenuGL::showPopup(parent_panel, menu, menu_x, menu_y);
}

void LLPanelPeople::onVisibilityChange(BOOL new_visibility)
{
	if (new_visibility == FALSE)
	{
		// Don't update anything while we're invisible.
		mNearbyListUpdater->setActive(FALSE);
	}
	else
	{
		reSelectedCurrentTab();
	}
}

// Make the tab-container re-select current tab
// for onTabSelected() callback to get called.
// (currently this is needed to reactivate nearby list updates
// when we get visible)
void LLPanelPeople::reSelectedCurrentTab()
{
	mTabContainer->selectTab(mTabContainer->getCurrentPanelIndex());
}

void LLPanelPeople::onSearchEdit(const std::string& search_string)
{
	if (mFilterSubString == search_string)
		return;

	mFilterSubString = search_string;

	LLStringUtil::toUpper(mFilterSubString);
	LLStringUtil::trimHead(mFilterSubString);
	mSearchEditor->setText(mFilterSubString);

	// Apply new filter to all tabs.
	filterNearbyList();
	filterFriendList();
	filterRecentList();
	updateGroupList();

	updateButtons();
}

void LLPanelPeople::onTabSelected(const LLSD& param)
{
	std::string tab_name = getChild<LLPanel>(param.asString())->getName();
	mNearbyListUpdater->setActive(tab_name == NEARBY_TAB_NAME);
	updateButtons();
}

void LLPanelPeople::onAvatarListDoubleClicked(LLAvatarList* list)
{
	LLUUID clicked_id = list->getCurrentID();

	if (clicked_id.isNull())
		return;

	// Open mini-inspector for the avatar being clicked
	LLFloaterReg::showInstance("mini_inspector", clicked_id);
	// inspector will delete itself on close
}

void LLPanelPeople::onAvatarListCommitted(LLAvatarList* list)
{
	(void) list;
	updateButtons();
}

void LLPanelPeople::onViewProfileButtonClicked()
{
	LLUUID id = getCurrentItemID();
	LLAvatarActions::showProfile(id);
}

void LLPanelPeople::onAddFriendButtonClicked()
{
	LLUUID id = getCurrentItemID();
	if (id.notNull())
	{
		std::string name;
		gCacheName->getFullName(id, name);
		LLAvatarActions::requestFriendshipDialog(id, name);
	}
}

void LLPanelPeople::onAddFriendWizButtonClicked()
{
	// Show add friend wizard.
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(onAvatarPicked, NULL, FALSE, TRUE);
	LLFloater* root_floater = gFloaterView->getParentFloater(this);
	if (root_floater)
	{
		root_floater->addDependentFloater(picker);
	}
}

void LLPanelPeople::onDeleteFriendButtonClicked()
{
	LLAvatarActions::removeFriendDialog(getCurrentItemID());
}

void LLPanelPeople::onGroupInfoButtonClicked()
{
	LLUUID group_id = getCurrentItemID();
	if (group_id.notNull())
		LLGroupActions::info(group_id);
}

void LLPanelPeople::onChatButtonClicked()
{
	LLUUID group_id = getCurrentItemID();
	if (group_id.notNull())
		LLGroupActions::startChat(group_id);
}

void LLPanelPeople::onImButtonClicked()
{
	LLUUID id = getCurrentItemID();
	if (id.notNull())
	{
		LLAvatarActions::startIM(id);
	}
}

void LLPanelPeople::onActivateButtonClicked()
{
	LLGroupActions::activate(mGroupList->getCurrentID());
}

// static
void LLPanelPeople::onAvatarPicked(
		const std::vector<std::string>& names,
		const std::vector<LLUUID>& ids,
		void*)
{
	if (!names.empty() && !ids.empty())
		LLAvatarActions::requestFriendshipDialog(ids[0], names[0]);
}

bool LLPanelPeople::onFriendListUpdate(U32 changed_mask)
{
	bool have_names = updateFriendList(changed_mask);

	// Update online status in the Recent tab.
	// *TODO: isn't it too much to update the whole list?
	updateRecentList();

	return have_names;
}

void LLPanelPeople::onGroupPlusButtonClicked()
{
	LLMenuGL* plus_menu = (LLMenuGL*)mGroupPlusMenuHandle.get();
	if (!plus_menu)
		return;

	showGroupMenu(plus_menu);
}

void LLPanelPeople::onGroupMinusButtonClicked()
{
	LLUUID group_id = getCurrentItemID();
	if (group_id.notNull())
		LLGroupActions::leave(group_id);
}

void LLPanelPeople::onGroupPlusMenuItemClicked(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();

	if (chosen_item == "join_group")
		LLGroupActions::search();
	else if (chosen_item == "new_group")
		LLGroupActions::create();
}

void LLPanelPeople::onCallButtonClicked()
{
	// *TODO: not implemented yet
}

void LLPanelPeople::onTeleportButtonClicked()
{
	LLAvatarActions::offerTeleport(getCurrentItemID());
}

void LLPanelPeople::onShareButtonClicked()
{
	// *TODO: not implemented yet
}

void LLPanelPeople::onMoreButtonClicked()
{
	// *TODO: not implemented yet
}

void	LLPanelPeople::onOpen(const LLSD& key)
{
	std::string tab_name = key.asString();

	if (!tab_name.empty())
		mTabContainer->selectTabByName(tab_name);
	else
		reSelectedCurrentTab();
}
