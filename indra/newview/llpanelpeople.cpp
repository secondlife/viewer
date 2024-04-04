/** 
 * @file llpanelpeople.cpp
 * @brief Side tray "People" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

// libs
#include "llavatarname.h"
#include "llconversationview.h"
#include "llfloaterimcontainer.h"
#include "llfloaterreg.h"
#include "llfloatersidepanelcontainer.h"
#include "llmenubutton.h"
#include "llmenugl.h"
#include "llnotificationsutil.h"
#include "lleventtimer.h"
#include "llfiltereditor.h"
#include "lltabcontainer.h"
#include "lltoggleablemenu.h"
#include "lluictrlfactory.h"

#include "llpanelpeople.h"

// newview
#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llagent.h"
#include "llagentbenefits.h"
#include "llavataractions.h"
#include "llavatarlist.h"
#include "llavatarlistitem.h"
#include "llavatarnamecache.h"
#include "llcallingcard.h"			// for LLAvatarTracker
#include "llcallbacklist.h"
#include "llerror.h"
#include "llfloateravatarpicker.h"
#include "llfriendcard.h"
#include "llgroupactions.h"
#include "llgrouplist.h"
#include "llinventoryobserver.h"
#include "llnetmap.h"
#include "llpanelpeoplemenus.h"
#include "llparticipantlist.h"
#include "llsidetraypanelcontainer.h"
#include "llrecentpeople.h"
#include "llviewercontrol.h"		// for gSavedSettings
#include "llviewermenu.h"			// for gMenuHolder
#include "llviewerregion.h"
#include "llvoiceclient.h"
#include "llworld.h"
#include "llspeakers.h"
#include "llfloaterwebcontent.h"

#include "llagentui.h"
#include "llslurl.h"

#define FRIEND_LIST_UPDATE_TIMEOUT	0.5
#define NEARBY_LIST_UPDATE_INTERVAL 1

static const std::string NEARBY_TAB_NAME	= "nearby_panel";
static const std::string FRIENDS_TAB_NAME	= "friends_panel";
static const std::string GROUP_TAB_NAME		= "groups_panel";
static const std::string RECENT_TAB_NAME	= "recent_panel";
static const std::string BLOCKED_TAB_NAME	= "blocked_panel"; // blocked avatars
static const std::string COLLAPSED_BY_USER  = "collapsed_by_user";

/** Comparator for comparing avatar items by last interaction date */
class LLAvatarItemRecentComparator : public LLAvatarItemComparator
{
public:
	LLAvatarItemRecentComparator() {};
	virtual ~LLAvatarItemRecentComparator() {};

protected:
	virtual bool doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const
	{
		LLRecentPeople& people = LLRecentPeople::instance();
		const LLDate& date1 = people.getDate(avatar_item1->getAvatarId());
		const LLDate& date2 = people.getDate(avatar_item2->getAvatarId());

		//older comes first
		return date1 > date2;
	}
};

/** Compares avatar items by online status, then by name */
class LLAvatarItemStatusComparator : public LLAvatarItemComparator
{
public:
	LLAvatarItemStatusComparator() {};

protected:
	/**
	 * @return true if item1 < item2, false otherwise
	 */
	virtual bool doCompare(const LLAvatarListItem* item1, const LLAvatarListItem* item2) const
	{
		LLAvatarTracker& at = LLAvatarTracker::instance();
		bool online1 = at.isBuddyOnline(item1->getAvatarId());
		bool online2 = at.isBuddyOnline(item2->getAvatarId());

		if (online1 == online2)
		{
			std::string name1 = item1->getAvatarName();
			std::string name2 = item2->getAvatarName();

			LLStringUtil::toUpper(name1);
			LLStringUtil::toUpper(name2);

			return name1 < name2;
		}
		
		return online1 > online2; 
	}
};

/** Compares avatar items by distance between you and them */
class LLAvatarItemDistanceComparator : public LLAvatarItemComparator
{
public:
	typedef std::map < LLUUID, LLVector3d > id_to_pos_map_t;
	LLAvatarItemDistanceComparator() {};

	void updateAvatarsPositions(std::vector<LLVector3d>& positions, uuid_vec_t& uuids)
	{
		std::vector<LLVector3d>::const_iterator
			pos_it = positions.begin(),
			pos_end = positions.end();

		uuid_vec_t::const_iterator
			id_it = uuids.begin(),
			id_end = uuids.end();

		LLAvatarItemDistanceComparator::id_to_pos_map_t pos_map;

		mAvatarsPositions.clear();

		for (;pos_it != pos_end && id_it != id_end; ++pos_it, ++id_it )
		{
			mAvatarsPositions[*id_it] = *pos_it;
		}
	};

protected:
	virtual bool doCompare(const LLAvatarListItem* item1, const LLAvatarListItem* item2) const
	{
		const LLVector3d& me_pos = gAgent.getPositionGlobal();
		const LLVector3d& item1_pos = mAvatarsPositions.find(item1->getAvatarId())->second;
		const LLVector3d& item2_pos = mAvatarsPositions.find(item2->getAvatarId())->second;
		
		return dist_vec_squared(item1_pos, me_pos) < dist_vec_squared(item2_pos, me_pos);
	}
private:
	id_to_pos_map_t mAvatarsPositions;
};

/** Comparator for comparing nearby avatar items by last spoken time */
class LLAvatarItemRecentSpeakerComparator : public  LLAvatarItemNameComparator
{
public:
	LLAvatarItemRecentSpeakerComparator() {};
	virtual ~LLAvatarItemRecentSpeakerComparator() {};

protected:
	virtual bool doCompare(const LLAvatarListItem* item1, const LLAvatarListItem* item2) const
	{
		LLPointer<LLSpeaker> lhs = LLActiveSpeakerMgr::instance().findSpeaker(item1->getAvatarId());
		LLPointer<LLSpeaker> rhs = LLActiveSpeakerMgr::instance().findSpeaker(item2->getAvatarId());
		if ( lhs.notNull() && rhs.notNull() )
		{
			// Compare by last speaking time
			if( lhs->mLastSpokeTime != rhs->mLastSpokeTime )
				return ( lhs->mLastSpokeTime > rhs->mLastSpokeTime );
		}
		else if ( lhs.notNull() )
		{
			// True if only item1 speaker info available
			return true;
		}
		else if ( rhs.notNull() )
		{
			// False if only item2 speaker info available
			return false;
		}
		// By default compare by name.
		return LLAvatarItemNameComparator::doCompare(item1, item2);
	}
};

class LLAvatarItemRecentArrivalComparator : public  LLAvatarItemNameComparator
{
public:
	LLAvatarItemRecentArrivalComparator() {};
	virtual ~LLAvatarItemRecentArrivalComparator() {};

protected:
	virtual bool doCompare(const LLAvatarListItem* item1, const LLAvatarListItem* item2) const
	{

		F32 arr_time1 = LLRecentPeople::instance().getArrivalTimeByID(item1->getAvatarId());
		F32 arr_time2 = LLRecentPeople::instance().getArrivalTimeByID(item2->getAvatarId());

		if (arr_time1 == arr_time2)
		{
			std::string name1 = item1->getAvatarName();
			std::string name2 = item2->getAvatarName();

			LLStringUtil::toUpper(name1);
			LLStringUtil::toUpper(name2);

			return name1 < name2;
		}

		return arr_time1 > arr_time2;
	}
};

static const LLAvatarItemRecentComparator RECENT_COMPARATOR;
static const LLAvatarItemStatusComparator STATUS_COMPARATOR;
static LLAvatarItemDistanceComparator DISTANCE_COMPARATOR;
static const LLAvatarItemRecentSpeakerComparator RECENT_SPEAKER_COMPARATOR;
static LLAvatarItemRecentArrivalComparator RECENT_ARRIVAL_COMPARATOR;

static LLPanelInjector<LLPanelPeople> t_people("panel_people");

//=============================================================================

/**
 * Updates given list either on regular basis or on external events (up to implementation). 
 */
class LLPanelPeople::Updater
{
public:
	typedef boost::function<void()> callback_t;
	Updater(callback_t cb)
	: mCallback(cb)
	{
	}

	virtual ~Updater()
	{
	}

	/**
	 * Activate/deactivate updater.
	 *
	 * This may start/stop regular updates.
	 */
	virtual void setActive(bool) {}

protected:
	void update()
	{
		mCallback();
	}

	callback_t		mCallback;
};

/**
 * Update buttons on changes in our friend relations (STORM-557).
 */
class LLButtonsUpdater : public LLPanelPeople::Updater, public LLFriendObserver
{
public:
	LLButtonsUpdater(callback_t cb)
	:	LLPanelPeople::Updater(cb)
	{
		LLAvatarTracker::instance().addObserver(this);
	}

	~LLButtonsUpdater()
	{
		LLAvatarTracker::instance().removeObserver(this);
	}

	/*virtual*/ void changed(U32 mask)
	{
		(void) mask;
		update();
	}
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

	virtual bool tick() // from LLEventTimer
	{
		return false;
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
	,	mIsActive(false)
	{
		LLAvatarTracker::instance().addObserver(this);

		// For notification when SIP online status changes.
		LLVoiceClient::getInstance()->addObserver(this);
		mInvObserver = new LLInventoryFriendCardObserver(this);
	}

	~LLFriendListUpdater()
	{
		// will be deleted by ~LLInventoryModel
		//delete mInvObserver;
        if (LLVoiceClient::instanceExists())
        {
            LLVoiceClient::getInstance()->removeObserver(this);
        }
		LLAvatarTracker::instance().removeObserver(this);
	}

	/*virtual*/ void changed(U32 mask)
	{
		if (mIsActive)
		{
			// events can arrive quickly in bulk - we need not process EVERY one of them -
			// so we wait a short while to let others pile-in, and process them in aggregate.
			mEventTimer.start();
		}

		// save-up all the mask-bits which have come-in
		mMask |= mask;
	}


	/*virtual*/ bool tick()
	{
		if (!mIsActive) return false;

		if (mMask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::ONLINE))
		{
			update();
		}

		// Stop updates.
		mEventTimer.stop();
		mMask = 0;

		return false;
	}

	// virtual
	void setActive(bool active)
	{
		mIsActive = active;
		if (active)
		{
			tick();
		}
	}

private:
	U32 mMask;
	LLInventoryFriendCardObserver* mInvObserver;
	bool mIsActive;

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
			LL_DEBUGS() << "Inventory changed: " << mask << LL_ENDL;

			static bool synchronize_friends_folders = true;
			if (synchronize_friends_folders)
			{
				// Checks whether "Friends" and "Friends/All" folders exist in "Calling Cards" folder,
				// fetches their contents if needed and synchronizes it with buddies list.
				// If the folders are not found they are created.
				LLFriendCardsManager::instance().syncFriendCardsFolders();
				synchronize_friends_folders = false;
			}

			// *NOTE: deleting of InventoryItem is performed via moving to Trash. 
			// That means LLInventoryObserver::STRUCTURE is present in MASK instead of LLInventoryObserver::REMOVE
			if ((CALLINGCARD_ADDED & mask) == CALLINGCARD_ADDED)
			{
				LL_DEBUGS() << "Calling card added: count: " << gInventory.getChangedIDs().size() 
					<< ", first Inventory ID: "<< (*gInventory.getChangedIDs().begin())
					<< LL_ENDL;

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
					LL_DEBUGS() << "friend found, panel should be updated" << LL_ENDL;
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
			update();
			mEventTimer.start(); 
		}
		else
		{
			// stop regular updates
			mEventTimer.stop();
		}
	}

	/*virtual*/ bool tick()
	{
		update();
		return false;
	}
private:
};

/**
 * Updates the recent people list (those the agent has recently interacted with).
 */
class LLRecentListUpdater : public LLAvatarListUpdater, public boost::signals2::trackable
{
	LOG_CLASS(LLRecentListUpdater);

public:
	LLRecentListUpdater(callback_t cb)
	:	LLAvatarListUpdater(cb, 0)
	{
		LLRecentPeople::instance().setChangedCallback(boost::bind(&LLRecentListUpdater::update, this));
	}
};

//=============================================================================

LLPanelPeople::LLPanelPeople()
	:	LLPanel(),
		mTabContainer(NULL),
		mOnlineFriendList(NULL),
		mAllFriendList(NULL),
		mNearbyList(NULL),
		mRecentList(NULL),
		mGroupList(NULL),
		mMiniMap(NULL)
{
	mFriendListUpdater = new LLFriendListUpdater(boost::bind(&LLPanelPeople::updateFriendList,	this));
	mNearbyListUpdater = new LLNearbyListUpdater(boost::bind(&LLPanelPeople::updateNearbyList,	this));
	mRecentListUpdater = new LLRecentListUpdater(boost::bind(&LLPanelPeople::updateRecentList,	this));
	mButtonsUpdater = new LLButtonsUpdater(boost::bind(&LLPanelPeople::updateButtons, this));

	mCommitCallbackRegistrar.add("People.AddFriend", boost::bind(&LLPanelPeople::onAddFriendButtonClicked, this));
	mCommitCallbackRegistrar.add("People.AddFriendWizard",	boost::bind(&LLPanelPeople::onAddFriendWizButtonClicked,	this));
	mCommitCallbackRegistrar.add("People.DelFriend",		boost::bind(&LLPanelPeople::onDeleteFriendButtonClicked,	this));
	mCommitCallbackRegistrar.add("People.Group.Minus",		boost::bind(&LLPanelPeople::onGroupMinusButtonClicked,  this));
	mCommitCallbackRegistrar.add("People.Chat",			boost::bind(&LLPanelPeople::onChatButtonClicked,		this));
	mCommitCallbackRegistrar.add("People.Gear",			boost::bind(&LLPanelPeople::onGearButtonClicked,		this, _1));

	mCommitCallbackRegistrar.add("People.Group.Plus.Action",  boost::bind(&LLPanelPeople::onGroupPlusMenuItemClicked,  this, _2));
	mCommitCallbackRegistrar.add("People.Friends.ViewSort.Action",  boost::bind(&LLPanelPeople::onFriendsViewSortMenuItemClicked,  this, _2));
	mCommitCallbackRegistrar.add("People.Nearby.ViewSort.Action",  boost::bind(&LLPanelPeople::onNearbyViewSortMenuItemClicked,  this, _2));
	mCommitCallbackRegistrar.add("People.Groups.ViewSort.Action",  boost::bind(&LLPanelPeople::onGroupsViewSortMenuItemClicked,  this, _2));
	mCommitCallbackRegistrar.add("People.Recent.ViewSort.Action",  boost::bind(&LLPanelPeople::onRecentViewSortMenuItemClicked,  this, _2));

	mEnableCallbackRegistrar.add("People.Friends.ViewSort.CheckItem",	boost::bind(&LLPanelPeople::onFriendsViewSortMenuItemCheck,	this, _2));
	mEnableCallbackRegistrar.add("People.Recent.ViewSort.CheckItem",	boost::bind(&LLPanelPeople::onRecentViewSortMenuItemCheck,	this, _2));
	mEnableCallbackRegistrar.add("People.Nearby.ViewSort.CheckItem",	boost::bind(&LLPanelPeople::onNearbyViewSortMenuItemCheck,	this, _2));

	mEnableCallbackRegistrar.add("People.Group.Plus.Validate",	boost::bind(&LLPanelPeople::onGroupPlusButtonValidate,	this));

	doPeriodically(boost::bind(&LLPanelPeople::updateNearbyArrivalTime, this), 2.0);
}

LLPanelPeople::~LLPanelPeople()
{
	delete mButtonsUpdater;
	delete mNearbyListUpdater;
	delete mFriendListUpdater;
	delete mRecentListUpdater;

    mNearbyFilterCommitConnection.disconnect();
    mFriedsFilterCommitConnection.disconnect();
    mGroupsFilterCommitConnection.disconnect();
    mRecentFilterCommitConnection.disconnect();

	if(LLVoiceClient::instanceExists())
	{
		LLVoiceClient::getInstance()->removeObserver(this);
	}
}

void LLPanelPeople::onFriendsAccordionExpandedCollapsed(LLUICtrl* ctrl, const LLSD& param, LLAvatarList* avatar_list)
{
	if(!avatar_list)
	{
		LL_ERRS() << "Bad parameter" << LL_ENDL;
		return;
	}

	bool expanded = param.asBoolean();

	setAccordionCollapsedByUser(ctrl, !expanded);
	if(!expanded)
	{
		avatar_list->resetSelection();
	}
}


void LLPanelPeople::removePicker()
{
    if(mPicker.get())
    {
        mPicker.get()->closeFloater();
    }
}

bool LLPanelPeople::postBuild()
{
	S32 max_premium = LLAgentBenefitsMgr::get("Premium").getGroupMembershipLimit();

    mNearbyFilterCommitConnection = getChild<LLFilterEditor>("nearby_filter_input")->setCommitCallback(boost::bind(&LLPanelPeople::onFilterEdit, this, _2));
    mFriedsFilterCommitConnection = getChild<LLFilterEditor>("friends_filter_input")->setCommitCallback(boost::bind(&LLPanelPeople::onFilterEdit, this, _2));
    mGroupsFilterCommitConnection = getChild<LLFilterEditor>("groups_filter_input")->setCommitCallback(boost::bind(&LLPanelPeople::onFilterEdit, this, _2));
    mRecentFilterCommitConnection = getChild<LLFilterEditor>("recent_filter_input")->setCommitCallback(boost::bind(&LLPanelPeople::onFilterEdit, this, _2));

	if(LLAgentBenefitsMgr::current().getGroupMembershipLimit() < max_premium)
	{
		getChild<LLTextBox>("groupcount")->setText(getString("GroupCountWithInfo"));
		getChild<LLTextBox>("groupcount")->setURLClickedCallback(boost::bind(&LLPanelPeople::onGroupLimitInfo, this));
	}

	mTabContainer = getChild<LLTabContainer>("tabs");
	mTabContainer->setCommitCallback(boost::bind(&LLPanelPeople::onTabSelected, this, _2));
	mSavedFilters.resize(mTabContainer->getTabCount());
	mSavedOriginalFilters.resize(mTabContainer->getTabCount());

	LLPanel* friends_tab = getChild<LLPanel>(FRIENDS_TAB_NAME);
	// updater is active only if panel is visible to user.
	friends_tab->setVisibleCallback(boost::bind(&Updater::setActive, mFriendListUpdater, _2));
    friends_tab->setVisibleCallback(boost::bind(&LLPanelPeople::removePicker, this));

	mOnlineFriendList = friends_tab->getChild<LLAvatarList>("avatars_online");
	mAllFriendList = friends_tab->getChild<LLAvatarList>("avatars_all");
	mOnlineFriendList->setNoItemsCommentText(getString("no_friends_online"));
	mOnlineFriendList->setShowIcons("FriendsListShowIcons");
	mOnlineFriendList->showPermissions("FriendsListShowPermissions");
	mOnlineFriendList->setShowCompleteName(!gSavedSettings.getBOOL("FriendsListHideUsernames"));
	mAllFriendList->setNoItemsCommentText(getString("no_friends"));
	mAllFriendList->setShowIcons("FriendsListShowIcons");
	mAllFriendList->showPermissions("FriendsListShowPermissions");
	mAllFriendList->setShowCompleteName(!gSavedSettings.getBOOL("FriendsListHideUsernames"));

	LLPanel* nearby_tab = getChild<LLPanel>(NEARBY_TAB_NAME);
	nearby_tab->setVisibleCallback(boost::bind(&Updater::setActive, mNearbyListUpdater, _2));
	mNearbyList = nearby_tab->getChild<LLAvatarList>("avatar_list");
	mNearbyList->setNoItemsCommentText(getString("no_one_near"));
	mNearbyList->setNoItemsMsg(getString("no_one_near"));
	mNearbyList->setNoFilteredItemsMsg(getString("no_one_filtered_near"));
	mNearbyList->setShowIcons("NearbyListShowIcons");
	mNearbyList->setShowCompleteName(!gSavedSettings.getBOOL("NearbyListHideUsernames"));
	mMiniMap = (LLNetMap*)getChildView("Net Map",true);
	mMiniMap->setToolTipMsg(gSavedSettings.getBOOL("DoubleClickTeleport") ? 
		getString("AltMiniMapToolTipMsg") :	getString("MiniMapToolTipMsg"));

	mRecentList = getChild<LLPanel>(RECENT_TAB_NAME)->getChild<LLAvatarList>("avatar_list");
	mRecentList->setNoItemsCommentText(getString("no_recent_people"));
	mRecentList->setNoItemsMsg(getString("no_recent_people"));
	mRecentList->setNoFilteredItemsMsg(getString("no_filtered_recent_people"));
	mRecentList->setShowIcons("RecentListShowIcons");

	mGroupList = getChild<LLGroupList>("group_list");
	mGroupList->setNoItemsCommentText(getString("no_groups_msg"));
	mGroupList->setNoItemsMsg(getString("no_groups_msg"));
	mGroupList->setNoFilteredItemsMsg(getString("no_filtered_groups_msg"));

	mNearbyList->setContextMenu(&LLPanelPeopleMenus::gNearbyPeopleContextMenu);
	mRecentList->setContextMenu(&LLPanelPeopleMenus::gPeopleContextMenu);
	mAllFriendList->setContextMenu(&LLPanelPeopleMenus::gPeopleContextMenu);
	mOnlineFriendList->setContextMenu(&LLPanelPeopleMenus::gPeopleContextMenu);

	setSortOrder(mRecentList,		(ESortOrder)gSavedSettings.getU32("RecentPeopleSortOrder"),	false);
	setSortOrder(mAllFriendList,	(ESortOrder)gSavedSettings.getU32("FriendsSortOrder"),		false);
	setSortOrder(mNearbyList,		(ESortOrder)gSavedSettings.getU32("NearbyPeopleSortOrder"),	false);

	mOnlineFriendList->setItemDoubleClickCallback(boost::bind(&LLPanelPeople::onAvatarListDoubleClicked, this, _1));
	mAllFriendList->setItemDoubleClickCallback(boost::bind(&LLPanelPeople::onAvatarListDoubleClicked, this, _1));
	mNearbyList->setItemDoubleClickCallback(boost::bind(&LLPanelPeople::onAvatarListDoubleClicked, this, _1));
	mRecentList->setItemDoubleClickCallback(boost::bind(&LLPanelPeople::onAvatarListDoubleClicked, this, _1));

	mOnlineFriendList->setCommitCallback(boost::bind(&LLPanelPeople::onAvatarListCommitted, this, mOnlineFriendList));
	mAllFriendList->setCommitCallback(boost::bind(&LLPanelPeople::onAvatarListCommitted, this, mAllFriendList));
	mNearbyList->setCommitCallback(boost::bind(&LLPanelPeople::onAvatarListCommitted, this, mNearbyList));
	mRecentList->setCommitCallback(boost::bind(&LLPanelPeople::onAvatarListCommitted, this, mRecentList));

	// Set openning IM as default on return action for avatar lists
	mOnlineFriendList->setReturnCallback(boost::bind(&LLPanelPeople::onImButtonClicked, this));
	mAllFriendList->setReturnCallback(boost::bind(&LLPanelPeople::onImButtonClicked, this));
	mNearbyList->setReturnCallback(boost::bind(&LLPanelPeople::onImButtonClicked, this));
	mRecentList->setReturnCallback(boost::bind(&LLPanelPeople::onImButtonClicked, this));

	mGroupList->setDoubleClickCallback(boost::bind(&LLPanelPeople::onChatButtonClicked, this));
	mGroupList->setCommitCallback(boost::bind(&LLPanelPeople::updateButtons, this));
	mGroupList->setReturnCallback(boost::bind(&LLPanelPeople::onChatButtonClicked, this));

	LLMenuButton* groups_gear_btn = getChild<LLMenuButton>("groups_gear_btn");

	// Use the context menu of the Groups list for the Groups tab gear menu.
	LLToggleableMenu* groups_gear_menu = mGroupList->getContextMenu();
	if (groups_gear_menu)
	{
		groups_gear_btn->setMenu(groups_gear_menu, LLMenuButton::MP_BOTTOM_LEFT);
	}
	else
	{
		LL_WARNS() << "People->Groups list menu not found" << LL_ENDL;
	}

	LLAccordionCtrlTab* accordion_tab = getChild<LLAccordionCtrlTab>("tab_all");
	accordion_tab->setDropDownStateChangedCallback(
		boost::bind(&LLPanelPeople::onFriendsAccordionExpandedCollapsed, this, _1, _2, mAllFriendList));

	accordion_tab = getChild<LLAccordionCtrlTab>("tab_online");
	accordion_tab->setDropDownStateChangedCallback(
		boost::bind(&LLPanelPeople::onFriendsAccordionExpandedCollapsed, this, _1, _2, mOnlineFriendList));

	// Must go after setting commit callback and initializing all pointers to children.
	mTabContainer->selectTabByName(NEARBY_TAB_NAME);

	LLVoiceClient::getInstance()->addObserver(this);

	// call this method in case some list is empty and buttons can be in inconsistent state
	updateButtons();

	mOnlineFriendList->setRefreshCompleteCallback(boost::bind(&LLPanelPeople::onFriendListRefreshComplete, this, _1, _2));
	mAllFriendList->setRefreshCompleteCallback(boost::bind(&LLPanelPeople::onFriendListRefreshComplete, this, _1, _2));

	return true;
}

// virtual
void LLPanelPeople::onChange(EStatusType status, const std::string &channelURI, bool proximal)
{
	if(status == STATUS_JOINING || status == STATUS_LEFT_CHANNEL)
	{
		return;
	}
	
	updateButtons();
}

void LLPanelPeople::updateFriendListHelpText()
{
	// show special help text for just created account to help finding friends. EXT-4836
	static LLTextBox* no_friends_text = getChild<LLTextBox>("no_friends_help_text");

	// Seems sometimes all_friends can be empty because of issue with Inventory loading (clear cache, slow connection...)
	// So, lets check all lists to avoid overlapping the text with online list. See EXT-6448.
	bool any_friend_exists = mAllFriendList->filterHasMatches() || mOnlineFriendList->filterHasMatches();
	no_friends_text->setVisible(!any_friend_exists);
	if (no_friends_text->getVisible())
	{
		//update help text for empty lists
		const std::string& filter = mSavedOriginalFilters[mTabContainer->getCurrentPanelIndex()];

		std::string message_name = filter.empty() ? "no_friends_msg" : "no_filtered_friends_msg";
		LLStringUtil::format_map_t args;
		args["[SEARCH_TERM]"] = LLURI::escape(filter);
		no_friends_text->setText(getString(message_name, args));
	}
}

void LLPanelPeople::updateFriendList()
{
	if (!mOnlineFriendList || !mAllFriendList)
		return;

	// get all buddies we know about
	const LLAvatarTracker& av_tracker = LLAvatarTracker::instance();
	LLAvatarTracker::buddy_map_t all_buddies;
	av_tracker.copyBuddyList(all_buddies);

	// save them to the online and all friends vectors
	uuid_vec_t& online_friendsp = mOnlineFriendList->getIDs();
	uuid_vec_t& all_friendsp = mAllFriendList->getIDs();

	all_friendsp.clear();
	online_friendsp.clear();

	uuid_vec_t buddies_uuids;
	LLAvatarTracker::buddy_map_t::const_iterator buddies_iter;

	// Fill the avatar list with friends UUIDs
	for (buddies_iter = all_buddies.begin(); buddies_iter != all_buddies.end(); ++buddies_iter)
	{
		buddies_uuids.push_back(buddies_iter->first);
	}

	if (buddies_uuids.size() > 0)
	{
		LL_DEBUGS() << "Friends added to the list: " << buddies_uuids.size() << LL_ENDL;
		all_friendsp = buddies_uuids;
	}
	else
	{
		LL_DEBUGS() << "No friends found" << LL_ENDL;
	}

	LLAvatarTracker::buddy_map_t::const_iterator buddy_it = all_buddies.begin();
	for (; buddy_it != all_buddies.end(); ++buddy_it)
	{
		LLUUID buddy_id = buddy_it->first;
		if (av_tracker.isBuddyOnline(buddy_id))
			online_friendsp.push_back(buddy_id);
	}

	/*
	 * Avatarlists  will be hidden by showFriendsAccordionsIfNeeded(), if they do not have items.
	 * But avatarlist can be updated only if it is visible @see LLAvatarList::draw();   
	 * So we need to do force update of lists to avoid inconsistency of data and view of avatarlist. 
	 */
	mOnlineFriendList->setDirty(true, !mOnlineFriendList->filterHasMatches());// do force update if list do NOT have items
	mAllFriendList->setDirty(true, !mAllFriendList->filterHasMatches());
	//update trash and other buttons according to a selected item
	updateButtons();
	showFriendsAccordionsIfNeeded();
}

void LLPanelPeople::updateNearbyList()
{
	if (!mNearbyList)
		return;

	std::vector<LLVector3d> positions;

	LLWorld::getInstance()->getAvatars(&mNearbyList->getIDs(), &positions, gAgent.getPositionGlobal(), gSavedSettings.getF32("NearMeRange"));
	mNearbyList->setDirty();

	DISTANCE_COMPARATOR.updateAvatarsPositions(positions, mNearbyList->getIDs());
	LLActiveSpeakerMgr::instance().update(true);
}

void LLPanelPeople::updateRecentList()
{
	if (!mRecentList)
		return;

	LLRecentPeople::instance().get(mRecentList->getIDs());
	mRecentList->setDirty();
}

void LLPanelPeople::updateButtons()
{
	std::string cur_tab		= getActiveTabName();
	bool friends_tab_active = (cur_tab == FRIENDS_TAB_NAME);
	bool group_tab_active	= (cur_tab == GROUP_TAB_NAME);
	//bool recent_tab_active	= (cur_tab == RECENT_TAB_NAME);
	LLUUID selected_id;

	uuid_vec_t selected_uuids;
	getCurrentItemIDs(selected_uuids);
	bool item_selected = (selected_uuids.size() == 1);
	bool multiple_selected = (selected_uuids.size() >= 1);

	if (group_tab_active)
	{
		if (item_selected)
		{
			selected_id = mGroupList->getSelectedUUID();
		}

		LLPanel* groups_panel = mTabContainer->getCurrentPanel();
		groups_panel->getChildView("minus_btn")->setEnabled(item_selected && selected_id.notNull()); // a real group selected

		U32 groups_count = gAgent.mGroups.size();
		S32 max_groups = LLAgentBenefitsMgr::current().getGroupMembershipLimit();
		U32 groups_remaining = max_groups > groups_count ? max_groups - groups_count : 0;
		groups_panel->getChild<LLUICtrl>("groupcount")->setTextArg("[COUNT]", llformat("%d", groups_count));
		groups_panel->getChild<LLUICtrl>("groupcount")->setTextArg("[REMAINING]", llformat("%d", groups_remaining));
	}
	else
	{
		bool is_friend = true;
		bool is_self = false;
		// Check whether selected avatar is our friend.
		if (item_selected)
		{
			selected_id = selected_uuids.front();
			is_friend = LLAvatarTracker::instance().getBuddyInfo(selected_id) != NULL;
			is_self = gAgent.getID() == selected_id;
		}

		LLPanel* cur_panel = mTabContainer->getCurrentPanel();
		if (cur_panel)
		{
			if (cur_panel->hasChild("add_friend_btn", true))
				cur_panel->getChildView("add_friend_btn")->setEnabled(item_selected && !is_friend && !is_self);

			if (friends_tab_active)
			{
				cur_panel->getChildView("friends_del_btn")->setEnabled(multiple_selected);
			}

			if (!group_tab_active)
			{
				cur_panel->getChildView("gear_btn")->setEnabled(multiple_selected);
			}
		}
	}
}

std::string LLPanelPeople::getActiveTabName() const
{
	return mTabContainer->getCurrentPanel()->getName();
}

LLUUID LLPanelPeople::getCurrentItemID() const
{
	std::string cur_tab = getActiveTabName();

	if (cur_tab == FRIENDS_TAB_NAME) // this tab has two lists
	{
		LLUUID cur_online_friend;

		if ((cur_online_friend = mOnlineFriendList->getSelectedUUID()).notNull())
			return cur_online_friend;

		return mAllFriendList->getSelectedUUID();
	}

	if (cur_tab == NEARBY_TAB_NAME)
		return mNearbyList->getSelectedUUID();

	if (cur_tab == RECENT_TAB_NAME)
		return mRecentList->getSelectedUUID();

	if (cur_tab == GROUP_TAB_NAME)
		return mGroupList->getSelectedUUID();

	if (cur_tab == BLOCKED_TAB_NAME)
		return LLUUID::null; // FIXME?

	llassert(0 && "unknown tab selected");
	return LLUUID::null;
}

void LLPanelPeople::getCurrentItemIDs(uuid_vec_t& selected_uuids) const
{
	std::string cur_tab = getActiveTabName();

	if (cur_tab == FRIENDS_TAB_NAME)
	{
		// friends tab has two lists
		mOnlineFriendList->getSelectedUUIDs(selected_uuids);
		mAllFriendList->getSelectedUUIDs(selected_uuids);
	}
	else if (cur_tab == NEARBY_TAB_NAME)
		mNearbyList->getSelectedUUIDs(selected_uuids);
	else if (cur_tab == RECENT_TAB_NAME)
		mRecentList->getSelectedUUIDs(selected_uuids);
	else if (cur_tab == GROUP_TAB_NAME)
		mGroupList->getSelectedUUIDs(selected_uuids);
	else if (cur_tab == BLOCKED_TAB_NAME)
		selected_uuids.clear(); // FIXME?
	else
		llassert(0 && "unknown tab selected");

}

void LLPanelPeople::setSortOrder(LLAvatarList* list, ESortOrder order, bool save)
{
	switch (order)
	{
	case E_SORT_BY_NAME:
		list->sortByName();
		break;
	case E_SORT_BY_STATUS:
		list->setComparator(&STATUS_COMPARATOR);
		list->sort();
		break;
	case E_SORT_BY_MOST_RECENT:
		list->setComparator(&RECENT_COMPARATOR);
		list->sort();
		break;
	case E_SORT_BY_RECENT_SPEAKERS:
		list->setComparator(&RECENT_SPEAKER_COMPARATOR);
		list->sort();
		break;
	case E_SORT_BY_DISTANCE:
		list->setComparator(&DISTANCE_COMPARATOR);
		list->sort();
		break;
	case E_SORT_BY_RECENT_ARRIVAL:
		list->setComparator(&RECENT_ARRIVAL_COMPARATOR);
		list->sort();
		break;
	default:
		LL_WARNS() << "Unrecognized people sort order for " << list->getName() << LL_ENDL;
		return;
	}

	if (save)
	{
		std::string setting;

		if (list == mAllFriendList || list == mOnlineFriendList)
			setting = "FriendsSortOrder";
		else if (list == mRecentList)
			setting = "RecentPeopleSortOrder";
		else if (list == mNearbyList)
			setting = "NearbyPeopleSortOrder";

		if (!setting.empty())
			gSavedSettings.setU32(setting, order);
	}
}

void LLPanelPeople::onFilterEdit(const std::string& search_string)
{
	const S32 cur_tab_idx = mTabContainer->getCurrentPanelIndex();
	std::string& filter = mSavedOriginalFilters[cur_tab_idx];
	std::string& saved_filter = mSavedFilters[cur_tab_idx];

	filter = search_string;
	LLStringUtil::trimHead(filter);

	// Searches are case-insensitive
	std::string search_upper = filter;
	LLStringUtil::toUpper(search_upper);

	if (saved_filter == search_upper)
		return;

	saved_filter = search_upper;

	// Apply new filter to the current tab.
	const std::string cur_tab = getActiveTabName();
	if (cur_tab == NEARBY_TAB_NAME)
	{
		mNearbyList->setNameFilter(filter);
	}
	else if (cur_tab == FRIENDS_TAB_NAME)
	{
		// store accordion tabs opened/closed state before any manipulation with accordion tabs
		if (!saved_filter.empty())
        {
            notifyChildren(LLSD().with("action","store_state"));
        }

		mOnlineFriendList->setNameFilter(filter);
		mAllFriendList->setNameFilter(filter);

        setAccordionCollapsedByUser("tab_online", false);
        setAccordionCollapsedByUser("tab_all", false);
        showFriendsAccordionsIfNeeded();

		// restore accordion tabs state _after_ all manipulations
		if(saved_filter.empty())
        {
            notifyChildren(LLSD().with("action","restore_state"));
        }
    }
	else if (cur_tab == GROUP_TAB_NAME)
	{
		mGroupList->setNameFilter(filter);
	}
	else if (cur_tab == RECENT_TAB_NAME)
	{
		mRecentList->setNameFilter(filter);
	}
}

void LLPanelPeople::onGroupLimitInfo()
{
	LLSD args;

	S32 max_basic = LLAgentBenefitsMgr::get("Base").getGroupMembershipLimit();
	S32 max_premium = LLAgentBenefitsMgr::get("Premium").getGroupMembershipLimit();
	
	args["MAX_BASIC"] = max_basic;
	args["MAX_PREMIUM"] = max_premium;

	if (LLAgentBenefitsMgr::has("Premium_Plus"))
	{
		S32 max_premium_plus = LLAgentBenefitsMgr::get("Premium_Plus").getGroupMembershipLimit();
		args["MAX_PREMIUM_PLUS"] = max_premium_plus;
		LLNotificationsUtil::add("GroupLimitInfoPlus", args);
	}
	else
	{
		LLNotificationsUtil::add("GroupLimitInfo", args);
	}	
}

void LLPanelPeople::onTabSelected(const LLSD& param)
{
	std::string tab_name = getChild<LLPanel>(param.asString())->getName();
	updateButtons();

	showFriendsAccordionsIfNeeded();
}

void LLPanelPeople::onAvatarListDoubleClicked(LLUICtrl* ctrl)
{
	LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*>(ctrl);
	if(!item)
	{
		return;
	}

	LLUUID clicked_id = item->getAvatarId();
	if(gAgent.getID() == clicked_id)
	{
		return;
	}
	
#if 0 // SJB: Useful for testing, but not currently functional or to spec
	LLAvatarActions::showProfile(clicked_id);
#else // spec says open IM window
	LLAvatarActions::startIM(clicked_id);
#endif
}

void LLPanelPeople::onAvatarListCommitted(LLAvatarList* list)
{
	if (getActiveTabName() == NEARBY_TAB_NAME)
	{
		uuid_vec_t selected_uuids;
		getCurrentItemIDs(selected_uuids);
		mMiniMap->setSelected(selected_uuids);
	} else
	// Make sure only one of the friends lists (online/all) has selection.
	if (getActiveTabName() == FRIENDS_TAB_NAME)
	{
		if (list == mOnlineFriendList)
			mAllFriendList->resetSelection(true);
		else if (list == mAllFriendList)
			mOnlineFriendList->resetSelection(true);
		else
			llassert(0 && "commit on unknown friends list");
	}

	updateButtons();
}

void LLPanelPeople::onAddFriendButtonClicked()
{
	LLUUID id = getCurrentItemID();
	if (id.notNull())
	{
		LLAvatarActions::requestFriendshipDialog(id);
	}
}

bool LLPanelPeople::isItemsFreeOfFriends(const uuid_vec_t& uuids)
{
	const LLAvatarTracker& av_tracker = LLAvatarTracker::instance();
	for ( uuid_vec_t::const_iterator
			  id = uuids.begin(),
			  id_end = uuids.end();
		  id != id_end; ++id )
	{
		if (av_tracker.isBuddy (*id))
		{
			return false;
		}
	}
	return true;
}

void LLPanelPeople::onAddFriendWizButtonClicked()
{
    LLPanel* cur_panel = mTabContainer->getCurrentPanel();
    LLView * button = cur_panel->findChild<LLButton>("friends_add_btn", true);

	// Show add friend wizard.
    LLFloater* root_floater = gFloaterView->getParentFloater(this);
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(boost::bind(&LLPanelPeople::onAvatarPicked, _1, _2), false, true, false, root_floater->getName(), button);
	if (!picker)
	{
		return;
	}

	// Need to disable 'ok' button when friend occurs in selection
	picker->setOkBtnEnableCb(boost::bind(&LLPanelPeople::isItemsFreeOfFriends, this, _1));
	
	if (root_floater)
	{
		root_floater->addDependentFloater(picker);
	}

    mPicker = picker->getHandle();
}

void LLPanelPeople::onDeleteFriendButtonClicked()
{
	uuid_vec_t selected_uuids;
	getCurrentItemIDs(selected_uuids);

	if (selected_uuids.size() == 1)
	{
		LLAvatarActions::removeFriendDialog( selected_uuids.front() );
	}
	else if (selected_uuids.size() > 1)
	{
		LLAvatarActions::removeFriendsDialog( selected_uuids );
	}
}

void LLPanelPeople::onChatButtonClicked()
{
	LLUUID group_id = getCurrentItemID();
	if (group_id.notNull())
		LLGroupActions::startIM(group_id);
}

void LLPanelPeople::onGearButtonClicked(LLUICtrl* btn)
{
	uuid_vec_t selected_uuids;
	getCurrentItemIDs(selected_uuids);
	// Spawn at bottom left corner of the button.
	if (getActiveTabName() == NEARBY_TAB_NAME)
		LLPanelPeopleMenus::gNearbyPeopleContextMenu.show(btn, selected_uuids, 0, 0);
	else
		LLPanelPeopleMenus::gPeopleContextMenu.show(btn, selected_uuids, 0, 0);
}

void LLPanelPeople::onImButtonClicked()
{
	uuid_vec_t selected_uuids;
	getCurrentItemIDs(selected_uuids);
	if ( selected_uuids.size() == 1 )
	{
		// if selected only one person then start up IM
		LLAvatarActions::startIM(selected_uuids.at(0));
	}
	else if ( selected_uuids.size() > 1 )
	{
		// for multiple selection start up friends conference
		LLAvatarActions::startConference(selected_uuids);
	}
}

// static
void LLPanelPeople::onAvatarPicked(const uuid_vec_t& ids, const std::vector<LLAvatarName> names)
{
	if (!names.empty() && !ids.empty())
		LLAvatarActions::requestFriendshipDialog(ids[0], names[0].getCompleteName());
}

bool LLPanelPeople::onGroupPlusButtonValidate()
{
	if (!gAgent.canJoinGroups())
	{
		LLNotificationsUtil::add("JoinedTooManyGroups");
		return false;
	}

	return true;
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
		setSortOrder(mAllFriendList, E_SORT_BY_NAME);
	}
	else if (chosen_item == "sort_status")
	{
		setSortOrder(mAllFriendList, E_SORT_BY_STATUS);
	}
	else if (chosen_item == "view_icons")
	{
		mAllFriendList->toggleIcons();
		mOnlineFriendList->toggleIcons();
	}
	else if (chosen_item == "view_permissions")
	{
		bool show_permissions = !gSavedSettings.getBOOL("FriendsListShowPermissions");
		gSavedSettings.setBOOL("FriendsListShowPermissions", show_permissions);

		mAllFriendList->showPermissions(show_permissions);
		mOnlineFriendList->showPermissions(show_permissions);
	}
	else if (chosen_item == "view_usernames")
	{
		bool hide_usernames = !gSavedSettings.getBOOL("FriendsListHideUsernames");
		gSavedSettings.setBOOL("FriendsListHideUsernames", hide_usernames);

		mAllFriendList->setShowCompleteName(!hide_usernames);
		mAllFriendList->handleDisplayNamesOptionChanged();
		mOnlineFriendList->setShowCompleteName(!hide_usernames);
		mOnlineFriendList->handleDisplayNamesOptionChanged();
	}
	}

void LLPanelPeople::onGroupsViewSortMenuItemClicked(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();

	if (chosen_item == "show_icons")
	{
		mGroupList->toggleIcons();
	}
}

void LLPanelPeople::onNearbyViewSortMenuItemClicked(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();

	if (chosen_item == "sort_by_recent_speakers")
	{
		setSortOrder(mNearbyList, E_SORT_BY_RECENT_SPEAKERS);
	}
	else if (chosen_item == "sort_name")
	{
		setSortOrder(mNearbyList, E_SORT_BY_NAME);
	}
	else if (chosen_item == "view_icons")
	{
		mNearbyList->toggleIcons();
	}
	else if (chosen_item == "sort_distance")
	{
		setSortOrder(mNearbyList, E_SORT_BY_DISTANCE);
	}
	else if (chosen_item == "sort_arrival")
	{
		setSortOrder(mNearbyList, E_SORT_BY_RECENT_ARRIVAL);
	}
	else if (chosen_item == "view_usernames")
	{
	    bool hide_usernames = !gSavedSettings.getBOOL("NearbyListHideUsernames");
	    gSavedSettings.setBOOL("NearbyListHideUsernames", hide_usernames);

	    mNearbyList->setShowCompleteName(!hide_usernames);
	    mNearbyList->handleDisplayNamesOptionChanged();
	}
}

bool LLPanelPeople::onNearbyViewSortMenuItemCheck(const LLSD& userdata)
{
	std::string item = userdata.asString();
	U32 sort_order = gSavedSettings.getU32("NearbyPeopleSortOrder");

	if (item == "sort_by_recent_speakers")
		return sort_order == E_SORT_BY_RECENT_SPEAKERS;
	if (item == "sort_name")
		return sort_order == E_SORT_BY_NAME;
	if (item == "sort_distance")
		return sort_order == E_SORT_BY_DISTANCE;
	if (item == "sort_arrival")
		return sort_order == E_SORT_BY_RECENT_ARRIVAL;

	return false;
}

void LLPanelPeople::onRecentViewSortMenuItemClicked(const LLSD& userdata)
{
	std::string chosen_item = userdata.asString();

	if (chosen_item == "sort_recent")
	{
		setSortOrder(mRecentList, E_SORT_BY_MOST_RECENT);
	} 
	else if (chosen_item == "sort_name")
	{
		setSortOrder(mRecentList, E_SORT_BY_NAME);
	}
	else if (chosen_item == "view_icons")
	{
		mRecentList->toggleIcons();
	}
}

bool LLPanelPeople::onFriendsViewSortMenuItemCheck(const LLSD& userdata) 
{
	std::string item = userdata.asString();
	U32 sort_order = gSavedSettings.getU32("FriendsSortOrder");

	if (item == "sort_name") 
		return sort_order == E_SORT_BY_NAME;
	if (item == "sort_status")
		return sort_order == E_SORT_BY_STATUS;

	return false;
}

bool LLPanelPeople::onRecentViewSortMenuItemCheck(const LLSD& userdata) 
{
	std::string item = userdata.asString();
	U32 sort_order = gSavedSettings.getU32("RecentPeopleSortOrder");

	if (item == "sort_recent")
		return sort_order == E_SORT_BY_MOST_RECENT;
	if (item == "sort_name") 
		return sort_order == E_SORT_BY_NAME;

	return false;
}

void LLPanelPeople::onMoreButtonClicked()
{
	// *TODO: not implemented yet
}

void	LLPanelPeople::onOpen(const LLSD& key)
{
	std::string tab_name = key["people_panel_tab_name"];
	if (!tab_name.empty())
	{
		mTabContainer->selectTabByName(tab_name);
		if(tab_name == BLOCKED_TAB_NAME)
		{
			LLPanel* blocked_tab = mTabContainer->getCurrentPanel()->findChild<LLPanel>("panel_block_list_sidetray");
			if(blocked_tab)
			{
				blocked_tab->onOpen(key);
			}
		}
	}
}

bool LLPanelPeople::notifyChildren(const LLSD& info)
{
	if (info.has("task-panel-action") && info["task-panel-action"].asString() == "handle-tri-state")
	{
		LLSideTrayPanelContainer* container = dynamic_cast<LLSideTrayPanelContainer*>(getParent());
		if (!container)
		{
			LL_WARNS() << "Cannot find People panel container" << LL_ENDL;
			return true;
		}

		if (container->getCurrentPanelIndex() > 0) 
		{
			// if not on the default panel, switch to it
			container->onOpen(LLSD().with(LLSideTrayPanelContainer::PARAM_SUB_PANEL_NAME, getName()));
		}
		else
			LLFloaterReg::hideInstance("people");

		return true; // this notification is only supposed to be handled by task panels
	}

	return LLPanel::notifyChildren(info);
}

void LLPanelPeople::showAccordion(const std::string name, bool show)
{
	if(name.empty())
	{
		LL_WARNS() << "No name provided" << LL_ENDL;
		return;
	}

	LLAccordionCtrlTab* tab = getChild<LLAccordionCtrlTab>(name);
	tab->setVisible(show);
	if(show)
	{
		// don't expand accordion if it was collapsed by user
		if(!isAccordionCollapsedByUser(tab))
		{
			// expand accordion
			tab->changeOpenClose(false);
		}
	}
}

void LLPanelPeople::showFriendsAccordionsIfNeeded()
{
	if(FRIENDS_TAB_NAME == getActiveTabName())
	{
		// Expand and show accordions if needed, else - hide them
		showAccordion("tab_online", mOnlineFriendList->filterHasMatches());
		showAccordion("tab_all", mAllFriendList->filterHasMatches());

		// Rearrange accordions
		LLAccordionCtrl* accordion = getChild<LLAccordionCtrl>("friends_accordion");
		accordion->arrange();

		// *TODO: new no_matched_tabs_text attribute was implemented in accordion (EXT-7368).
		// this code should be refactored to use it
		// keep help text in a synchronization with accordions visibility.
		updateFriendListHelpText();
	}
}

void LLPanelPeople::onFriendListRefreshComplete(LLUICtrl*ctrl, const LLSD& param)
{
	if(ctrl == mOnlineFriendList)
	{
		showAccordion("tab_online", param.asInteger());
	}
	else if(ctrl == mAllFriendList)
	{
		showAccordion("tab_all", param.asInteger());
	}
}

void LLPanelPeople::setAccordionCollapsedByUser(LLUICtrl* acc_tab, bool collapsed)
{
	if(!acc_tab)
	{
		LL_WARNS() << "Invalid parameter" << LL_ENDL;
		return;
	}

	LLSD param = acc_tab->getValue();
	param[COLLAPSED_BY_USER] = collapsed;
	acc_tab->setValue(param);
}

void LLPanelPeople::setAccordionCollapsedByUser(const std::string& name, bool collapsed)
{
	setAccordionCollapsedByUser(getChild<LLUICtrl>(name), collapsed);
}

bool LLPanelPeople::isAccordionCollapsedByUser(LLUICtrl* acc_tab)
{
	if(!acc_tab)
	{
		LL_WARNS() << "Invalid parameter" << LL_ENDL;
		return false;
	}

	LLSD param = acc_tab->getValue();
	if(!param.has(COLLAPSED_BY_USER))
	{
		return false;
	}
	return param[COLLAPSED_BY_USER].asBoolean();
}

bool LLPanelPeople::isAccordionCollapsedByUser(const std::string& name)
{
	return isAccordionCollapsedByUser(getChild<LLUICtrl>(name));
}

bool LLPanelPeople::updateNearbyArrivalTime()
{
	std::vector<LLVector3d> positions;
	std::vector<LLUUID> uuids;
	static LLCachedControl<F32> range(gSavedSettings, "NearMeRange");
	LLWorld::getInstance()->getAvatars(&uuids, &positions, gAgent.getPositionGlobal(), range);
	LLRecentPeople::instance().updateAvatarsArrivalTime(uuids);
	return LLApp::isExiting();
}


// EOF
