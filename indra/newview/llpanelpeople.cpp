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
#include "llfiltereditor.h"
#include "lltabcontainer.h"
#include "lluictrlfactory.h"

#include "llpanelpeople.h"

// newview
#include "llagent.h"
#include "llavataractions.h"
#include "llavatarlist.h"
#include "llcallingcard.h"			// for LLAvatarTracker
#include "llfloateravatarpicker.h"
//#include "llfloaterminiinspector.h"
#include "llfriendcard.h"
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
	virtual void forceUpdate()
	{
		updateList();
	}

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
	class LLInventoryFriendCardObserver;

public: 
	friend class LLInventoryFriendCardObserver;
	LLFriendListUpdater(callback_t cb)
	:	LLAvatarListUpdater(cb, FRIEND_LIST_UPDATE_TIMEOUT)
	{
		LLAvatarTracker::instance().addObserver(this);

		// For notification when SIP online status changes.
		LLVoiceClient::getInstance()->addObserver(this);
		mInvObserver = new LLInventoryFriendCardObserver(this);
	}

	~LLFriendListUpdater()
	{
		delete mInvObserver;
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
	LLInventoryFriendCardObserver* mInvObserver;

	/**
	 *	This class is intended for updating Friend List when Inventory Friend Card is added/removed.
	 * 
	 *	The main usage is when Inventory Friends/All content is added while synchronizing with 
	 *		friends list on startup is performed. In this case Friend Panel should be updated when 
	 *		missing Inventory Friend Card is created.
	 *	*NOTE: updating is fired when Inventory item is added into CallingCards/Friends subfolder.
	 *		Otherwise LLFriendObserver functionality is enough to keep Friends Panel synchronized.
	 */
	class LLInventoryFriendCardObserver : public LLInventoryObserver
	{
		LOG_CLASS(LLFriendListUpdater::LLInventoryFriendCardObserver);

		friend class LLFriendListUpdater;

	private:
		LLInventoryFriendCardObserver(LLFriendListUpdater* updater) : mUpdater(updater)
		{
			gInventory.addObserver(this);
		}
		~LLInventoryFriendCardObserver()
		{
			gInventory.removeObserver(this);
		}
		/*virtual*/ void changed(U32 mask)
		{
			lldebugs << "Inventory changed: " << mask << llendl;

			// *NOTE: deleting of InventoryItem is performed via moving to Trash. 
			// That means LLInventoryObserver::STRUCTURE is present in MASK instead of LLInventoryObserver::REMOVE
			if ((CALLINGCARD_ADDED & mask) == CALLINGCARD_ADDED)
			{
				lldebugs << "Calling card added: count: " << gInventory.getChangedIDs().size() 
					<< ", first Inventory ID: "<< (*gInventory.getChangedIDs().begin())
					<< llendl;

				bool friendFound = false;
				std::set<LLUUID> changedIDs = gInventory.getChangedIDs();
				for (std::set<LLUUID>::const_iterator it = changedIDs.begin(); it != changedIDs.end(); ++it)
				{
					if (isDescendentOfInventoryFriends(*it))
					{
						friendFound = true;
						break;
					}
				}

				if (friendFound)
				{
					lldebugs << "friend found, panel should be updated" << llendl;
					mUpdater->changed(LLFriendObserver::ADD);
				}
			}
		}

		bool isDescendentOfInventoryFriends(const LLUUID& invItemID)
		{
			LLViewerInventoryItem * item = gInventory.getItem(invItemID);
			if (NULL == item)
				return false;

			return LLFriendCardsManager::instance().isItemInAnyFriendsList(item);
		}
		LLFriendListUpdater* mUpdater;

		static const U32 CALLINGCARD_ADDED = LLInventoryObserver::ADD | LLInventoryObserver::CALLING_CARD;
	};
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
		mFilterEditor(NULL),
		mTabContainer(NULL),
		mOnlineFriendList(NULL),
		mAllFriendList(NULL),
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
	LLView::deleteViewByHandle(mNearbyViewSortMenuHandle);
	LLView::deleteViewByHandle(mFriendsViewSortMenuHandle);
	LLView::deleteViewByHandle(mRecentViewSortMenuHandle);

}
void onAvatarListTmpDoubleClicked(LLAvatarListTmp* list)
{
	LLUUID clicked_id = list->getCurrentID();

	if (clicked_id.isNull())
		return;

#if 0 // SJB: Useful for testing, but not currently functional or to spec
	LLAvatarActions::showProfile(clicked_id);
#else // spec says open IM window
	LLAvatarActions::startIM(clicked_id);
#endif
}

BOOL LLPanelPeople::postBuild()
{
	mVisibleSignal.connect(boost::bind(&LLPanelPeople::onVisibilityChange, this, _2));
	
	mFilterEditor = getChild<LLFilterEditor>("filter_input");
	mFilterEditor->setCommitCallback(boost::bind(&LLPanelPeople::onFilterEdit, this, _2));

	mTabContainer = getChild<LLTabContainer>("tabs");
	mTabContainer->setCommitCallback(boost::bind(&LLPanelPeople::onTabSelected, this, _2));

	mOnlineFriendList = getChild<LLPanel>(FRIENDS_TAB_NAME)->getChild<LLAvatarList>("avatars_online");
	mAllFriendList = getChild<LLPanel>(FRIENDS_TAB_NAME)->getChild<LLAvatarList>("avatars_all");

	mNearbyList = getChild<LLPanel>(NEARBY_TAB_NAME)->getChild<LLAvatarList>("avatar_list");

	mRecentList = getChild<LLPanel>(RECENT_TAB_NAME)->getChild<LLAvatarListTmp>("avatar_list");
	mGroupList = getChild<LLGroupList>("group_list");

	LLPanel* groups_panel = getChild<LLPanel>(GROUP_TAB_NAME);
	groups_panel->childSetAction("activate_btn", boost::bind(&LLPanelPeople::onActivateButtonClicked,	this));
	groups_panel->childSetAction("plus_btn",	boost::bind(&LLPanelPeople::onGroupPlusButtonClicked,	this));
	groups_panel->childSetAction("minus_btn",	boost::bind(&LLPanelPeople::onGroupMinusButtonClicked,	this));

	LLPanel* friends_panel = getChild<LLPanel>(FRIENDS_TAB_NAME);
	friends_panel->childSetAction("add_btn",	boost::bind(&LLPanelPeople::onAddFriendWizButtonClicked,	this));
	friends_panel->childSetAction("del_btn",	boost::bind(&LLPanelPeople::onDeleteFriendButtonClicked,	this));

	mOnlineFriendList->setDoubleClickCallback(boost::bind(&LLPanelPeople::onAvatarListDoubleClicked, this, mOnlineFriendList));
	mAllFriendList->setDoubleClickCallback(boost::bind(&LLPanelPeople::onAvatarListDoubleClicked, this, mAllFriendList));
	mNearbyList->setDoubleClickCallback(boost::bind(&LLPanelPeople::onAvatarListDoubleClicked, this, mNearbyList));
	mRecentList->setDoubleClickCallback(boost::bind(onAvatarListTmpDoubleClicked, mRecentList));
	mOnlineFriendList->setCommitCallback(boost::bind(&LLPanelPeople::onAvatarListCommitted, this, mOnlineFriendList));
	mAllFriendList->setCommitCallback(boost::bind(&LLPanelPeople::onAvatarListCommitted, this, mAllFriendList));
	mNearbyList->setCommitCallback(boost::bind(&LLPanelPeople::onAvatarListCommitted, this, mNearbyList));
	mRecentList->setCommitCallback(boost::bind(&LLPanelPeople::updateButtons, this));

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

	getChild<LLPanel>(NEARBY_TAB_NAME)->childSetAction("nearby_view_sort_btn",boost::bind(&LLPanelPeople::onNearbyViewSortButtonClicked,		this));
	getChild<LLPanel>(RECENT_TAB_NAME)->childSetAction("recent_viewsort_btn",boost::bind(&LLPanelPeople::onRecentViewSortButtonClicked,		this));
	getChild<LLPanel>(FRIENDS_TAB_NAME)->childSetAction("friends_viewsort_btn",boost::bind(&LLPanelPeople::onFriendsViewSortButtonClicked,		this));

	// Must go after setting commit callback and initializing all pointers to children.
	mTabContainer->selectTabByName(FRIENDS_TAB_NAME);

	// Create menus.
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	
	registrar.add("People.Group.Plus.Action",  boost::bind(&LLPanelPeople::onGroupPlusMenuItemClicked,  this, _2));
	registrar.add("People.Friends.ViewSort.Action",  boost::bind(&LLPanelPeople::onFriendsViewSortMenuItemClicked,  this, _2));
	registrar.add("People.Nearby.ViewSort.Action",  boost::bind(&LLPanelPeople::onNearbyViewSortMenuItemClicked,  this, _2));
	registrar.add("People.Recent.ViewSort.Action",  boost::bind(&LLPanelPeople::onRecentViewSortMenuItemClicked,  this, _2));
	
	LLMenuGL* plus_menu  = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_group_plus.xml",  gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mGroupPlusMenuHandle  = plus_menu->getHandle();

	LLMenuGL* nearby_view_sort  = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_people_nearby_view_sort.xml",  gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if(nearby_view_sort)
		mNearbyViewSortMenuHandle  = nearby_view_sort->getHandle();

	LLMenuGL* friend_view_sort  = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_people_friends_view_sort.xml",  gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if(friend_view_sort)
		mFriendsViewSortMenuHandle  = friend_view_sort->getHandle();

	LLMenuGL* recent_view_sort  = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_people_recent_view_sort.xml",  gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if(recent_view_sort)
		mRecentViewSortMenuHandle  = recent_view_sort->getHandle();



	// Perform initial update.
	mFriendListUpdater->forceUpdate();
	mRecentListUpdater->forceUpdate();
	mGroupListUpdater->forceUpdate();
	mRecentListUpdater->forceUpdate();

	return TRUE;
}

bool LLPanelPeople::updateFriendList(U32 changed_mask)
{
	// Refresh names.
	if (changed_mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::ONLINE))
	{
		// get all buddies we know about
		const LLAvatarTracker& av_tracker = LLAvatarTracker::instance();
		LLAvatarTracker::buddy_map_t all_buddies;
		av_tracker.copyBuddyList(all_buddies);

		// *TODO: it's suboptimal to rebuild the whole lists on online status change.

		// save them to the online and all friends vectors
		mOnlineFriendVec.clear();
		mAllFriendVec.clear();

		LLFriendCardsManager::folderid_buddies_map_t listMap;

		// *NOTE: For now collectFriendsLists returns data only for Friends/All folder. EXT-694.
		LLFriendCardsManager::instance().collectFriendsLists(listMap);
		if (listMap.size() > 0)
		{
			lldebugs << "Friends Cards were found, count: " << listMap.begin()->second.size() << llendl;
			mAllFriendVec = listMap.begin()->second;
		}
		else
		{
			lldebugs << "Friends Cards were not found" << llendl;
		}

		LLAvatarTracker::buddy_map_t::const_iterator buddy_it = all_buddies.begin();
		for (; buddy_it != all_buddies.end(); ++buddy_it)
		{
			LLUUID buddy_id = buddy_it->first;
			if (av_tracker.isBuddyOnline(buddy_id))
				mOnlineFriendVec.push_back(buddy_id);
		}

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
	if (!mGroupList)
		return true; // there's no point in further updates

	bool have_names = mGroupList->update(mFilterSubString);

	if (mGroupList->isEmpty())
		mGroupList->setCommentText(getString("no_groups"));

	return have_names;
}

bool LLPanelPeople::filterFriendList()
{
	if (!mOnlineFriendList || !mAllFriendList)
		return true; // there's no point in further updates

	// We must always update Friends list to clear the latest removed friend.
	bool have_names =
			mOnlineFriendList->update(mOnlineFriendVec, mFilterSubString) &
			mAllFriendList->update(mAllFriendVec, mFilterSubString);

	if (mOnlineFriendVec.size() == 0)
		mOnlineFriendList->setCommentText(getString("no_friends_online"));

	if (mAllFriendVec.size() == 0)
		mAllFriendList->setCommentText(getString("no_friends"));

	return have_names;
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
	if (!mRecentList)
		return true;

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
	std::string cur_tab		= getActiveTabName();
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
	buttonSetVisible("teleport_btn",		friends_tab_active);
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

		// Check whether selected avatar is our friend.
		if ((selected_id = getCurrentItemID()).notNull())
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

const std::string& LLPanelPeople::getActiveTabName() const
{
	return mTabContainer->getCurrentPanel()->getName();
}

LLUUID LLPanelPeople::getCurrentItemID() const
{
	std::string cur_tab = getActiveTabName();

	if (cur_tab == FRIENDS_TAB_NAME) // this tab has two lists
	{
		LLUUID cur_online_friend;

		if ((cur_online_friend = mOnlineFriendList->getCurrentID()).notNull())
			return cur_online_friend;

		return mAllFriendList->getCurrentID();
	}

	if (cur_tab == NEARBY_TAB_NAME)
		return mNearbyList->getCurrentID();

	if (cur_tab == RECENT_TAB_NAME)
		return mRecentList->getCurrentID();

	if (cur_tab == GROUP_TAB_NAME)
		return mGroupList->getCurrentID();

	llassert(0 && "unknown tab selected");
	return LLUUID::null;
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

void LLPanelPeople::onVisibilityChange(const LLSD& new_visibility)
{
	if (new_visibility.asBoolean() == FALSE)
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

void LLPanelPeople::onFilterEdit(const std::string& search_string)
{
	if (mFilterSubString == search_string)
		return;

	mFilterSubString = search_string;

	// Searches are case-insensitive
	LLStringUtil::toUpper(mFilterSubString);
	LLStringUtil::trimHead(mFilterSubString);

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

	if (GROUP_TAB_NAME == tab_name)
		mFilterEditor->setLabel(getString("groups_filter_label"));
	else
		mFilterEditor->setLabel(getString("people_filter_label"));
}

void LLPanelPeople::onAvatarListDoubleClicked(LLAvatarList* list)
{
	LLUUID clicked_id = list->getCurrentID();

	if (clicked_id.isNull())
		return;
	
#if 0 // SJB: Useful for testing, but not currently functional or to spec
	LLAvatarActions::showProfile(clicked_id);
#else // spec says open IM window
	LLAvatarActions::startIM(clicked_id);
#endif
}

void LLPanelPeople::onAvatarListCommitted(LLAvatarList* list)
{
	// Make sure only one of the friends lists (online/all) has selection.
	if (getActiveTabName() == FRIENDS_TAB_NAME)
	{
		if (list == mOnlineFriendList)
			mAllFriendList->deselectAllItems(TRUE);
		else if (list == mAllFriendList)
			mOnlineFriendList->deselectAllItems(TRUE);
		else
			llassert(0 && "commit on unknown friends list");
	}

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
		LLAvatarActions::requestFriendshipDialog(id);
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
	LLGroupActions::show(getCurrentItemID());
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
		LLGroupActions::createGroup();
}

void LLPanelPeople::onFriendsViewSortMenuItemClicked(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();

	if (chosen_item == "sort_name")
	{
	}
	else if (chosen_item == "sort_status")
	{
	}
	else if (chosen_item == "view_icons")
	{
	}
	else if (chosen_item == "organize_offline")
	{
	}
}
void LLPanelPeople::onNearbyViewSortMenuItemClicked(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();

	if (chosen_item == "sort_recent")
	{
	}
	else if (chosen_item == "sort_name")
	{
	}
	else if (chosen_item == "view_icons")
	{
	}
	else if (chosen_item == "sort_distance")
	{
	}
}
void LLPanelPeople::onRecentViewSortMenuItemClicked(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();

	if (chosen_item == "sort_most")
	{
	}
	else if (chosen_item == "sort_name")
	{
	}
	else if (chosen_item == "view_icons")
	{
	}
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
void LLPanelPeople::onFriendsViewSortButtonClicked()
{
	LLMenuGL* menu = (LLMenuGL*)mFriendsViewSortMenuHandle.get();
	if (!menu)
		return;
	showGroupMenu(menu);
}
void LLPanelPeople::onRecentViewSortButtonClicked()
{
	LLMenuGL* menu = (LLMenuGL*)mRecentViewSortMenuHandle.get();
	if (!menu)
		return;
	showGroupMenu(menu);
}
void LLPanelPeople::onNearbyViewSortButtonClicked()
{
	LLMenuGL* menu = (LLMenuGL*)mNearbyViewSortMenuHandle.get();
	if (!menu)
		return;
	showGroupMenu(menu);
}

void	LLPanelPeople::onOpen(const LLSD& key)
{
	std::string tab_name = key["people_panel_tab_name"];
	mFilterEditor -> clear();
	onFilterEdit("");
	
	if (!tab_name.empty())
		mTabContainer->selectTabByName(tab_name);
	else
		reSelectedCurrentTab();
}

