/** 
 * @file llcallingcard.h
 * @brief Definition of the LLPreviewCallingCard class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLCALLINGCARD_H
#define LL_LLCALLINGCARD_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "lluserrelations.h"
#include "lluuid.h"
#include "v3dmath.h"

//class LLInventoryModel;
//class LLInventoryObserver;
class LLMessageSystem;
class LLTrackingData;

class LLFriendObserver
{
public:
	// This enumeration is a way to refer to what changed in a more
	// human readable format. You can mask the value provided by
	// chaged() to see if the observer is interested in the change.
	enum 
	{
		NONE = 0,
		ADD = 1,
		REMOVE = 2,
		ONLINE = 4,
		POWERS = 8,
		ALL = 0xffffffff
	};
	virtual ~LLFriendObserver() {}
	virtual void changed(U32 mask) = 0;
};

/*
struct LLBuddyInfo
{
	bool mIsOnline;
	bool mIsEmpowered;
	LLBuddyInfo() : mIsOnline(false), mIsEmpowered(false) {}
};
*/

// This is used as a base class for doing operations on all buddies.
class LLRelationshipFunctor
{
public:
	virtual ~LLRelationshipFunctor() {}
	virtual bool operator()(const LLUUID& buddy_id, LLRelationship* buddy) = 0;
};
	

class LLAvatarTracker
{
public:
	static LLAvatarTracker& instance() { return sInstance; }
	
	void track(const LLUUID& avatar_id, const std::string& name);
	void untrack(const LLUUID& avatar_id);
	bool isTrackedAgentValid() { return mTrackedAgentValid; }
	void setTrackedAgentValid(bool valid) { mTrackedAgentValid = valid; }
	void findAgent();

	// coarse update information
	void setTrackedCoarseLocation(const LLVector3d& global_pos);

	// dealing with the tracked agent location
	bool haveTrackingInfo();
	void getDegreesAndDist(F32& rot, F64& horiz_dist, F64& vert_dist);
	LLVector3d getGlobalPos();

	// Get the name passed in, returns null string if uninitialized.
	const std::string& getName();

	// Get the avatar being tracked, returns LLUUID::null if uninitialized
	const LLUUID& getAvatarID();

	// Deal with inventory
	//void observe(LLInventoryModel* model);
	//void inventoryChanged();

	// add or remove agents from buddy list. Each method takes a set
	// of buddies and returns how many were actually added or removed.
	typedef std::map<LLUUID, LLRelationship*> buddy_map_t;

	S32 addBuddyList(const buddy_map_t& buddies);
	//S32 removeBuddyList(const buddy_list_t& exes);
	void copyBuddyList(buddy_map_t& buddies) const;

	// deal with termination of friendhsip
	void terminateBuddy(const LLUUID& id);

	// get full info
	const LLRelationship* getBuddyInfo(const LLUUID& id) const;

	// Is this person a friend/buddy/calling card holder?
	bool isBuddy(const LLUUID& id) const;

	// online status
	void setBuddyOnline(const LLUUID& id, bool is_online);
	bool isBuddyOnline(const LLUUID& id) const;

	// simple empowered status
	void setBuddyEmpowered(const LLUUID& id, bool is_empowered);
	bool isBuddyEmpowered(const LLUUID& id) const;

	// set the empower bit & message the server.
	void empowerList(const buddy_map_t& list, bool grant);
	void empower(const LLUUID& id, bool grant); // wrapper for above

	// register callbacks
	void registerCallbacks(LLMessageSystem* msg);

	// Add/remove an observer. If the observer is destroyed, be sure
	// to remove it. On destruction of the tracker, it will delete any
	// observers left behind.
	void addObserver(LLFriendObserver* observer);
	void removeObserver(LLFriendObserver* observer);
	void notifyObservers();

	// Observers interested in updates of a particular avatar.
	// On destruction these observers are NOT deleted.
	void addParticularFriendObserver(const LLUUID& buddy_id, LLFriendObserver* observer);
	void removeParticularFriendObserver(const LLUUID& buddy_id, LLFriendObserver* observer);
	void notifyParticularFriendObservers(const LLUUID& buddy_id);

	/**
	 * Stores flag for change and id of object change applies to
	 *
	 * This allows outsiders to tell the AvatarTracker if something has
	 * been changed 'under the hood',
	 * and next notification will have exact avatar IDs have been changed.
	 */
	void addChangedMask(U32 mask, const LLUUID& referent);

	const std::set<LLUUID>& getChangedIDs() { return mChangedBuddyIDs; }

	// Apply the functor to every buddy. Do not actually modify the
	// buddy list in the functor or bad things will happen.
	void applyFunctor(LLRelationshipFunctor& f);

	static void formFriendship(const LLUUID& friend_id);

protected:
	void deleteTrackingData();
	void agentFound(const LLUUID& prey,
					const LLVector3d& estimated_global_pos);

	// Message system functionality
	static void processAgentFound(LLMessageSystem* msg, void**);
	static void processOnlineNotification(LLMessageSystem* msg, void**);
	static void processOfflineNotification(LLMessageSystem* msg, void**);
	//static void processGrantedProxies(LLMessageSystem* msg, void**);
	static void processTerminateFriendship(LLMessageSystem* msg, void**);
	static void processChangeUserRights(LLMessageSystem* msg, void**);

	void processNotify(LLMessageSystem* msg, bool online);
	void processChange(LLMessageSystem* msg);

protected:
	static LLAvatarTracker sInstance;
	LLTrackingData* mTrackingData;
	bool mTrackedAgentValid;
	U32 mModifyMask;
	//LLInventoryModel* mInventory;
	//LLInventoryObserver* mInventoryObserver;

	buddy_map_t mBuddyInfo;

	typedef std::set<LLUUID> changed_buddy_t;
	changed_buddy_t mChangedBuddyIDs;

	typedef std::vector<LLFriendObserver*> observer_list_t;
	observer_list_t mObservers;

    typedef std::set<LLFriendObserver*> observer_set_t;
    typedef std::map<LLUUID, observer_set_t> observer_map_t;
    observer_map_t mParticularFriendObserverMap;

private:
	// do not implement
	LLAvatarTracker(const LLAvatarTracker&);
	bool operator==(const LLAvatarTracker&);

public:
	// don't you dare create or delete this object
	LLAvatarTracker();
	~LLAvatarTracker();
};

// collect set of LLUUIDs we're a proxy for
class LLCollectProxyBuddies : public LLRelationshipFunctor
{
public:
	LLCollectProxyBuddies() {}
	virtual ~LLCollectProxyBuddies() {}
	virtual bool operator()(const LLUUID& buddy_id, LLRelationship* buddy);
	typedef std::set<LLUUID> buddy_list_t;
	buddy_list_t mProxy;
};

// collect dictionary sorted map of name -> agent_id for every online buddy
class LLCollectMappableBuddies : public LLRelationshipFunctor
{
public:
	LLCollectMappableBuddies() {}
	virtual ~LLCollectMappableBuddies() {}
	virtual bool operator()(const LLUUID& buddy_id, LLRelationship* buddy);
	typedef std::map<std::string, LLUUID, LLDictionaryLess> buddy_map_t;
	buddy_map_t mMappable;
	std::string mFullName;
};

// collect dictionary sorted map of name -> agent_id for every online buddy
class LLCollectOnlineBuddies : public LLRelationshipFunctor
{
public:
	LLCollectOnlineBuddies() {}
	virtual ~LLCollectOnlineBuddies() {}
	virtual bool operator()(const LLUUID& buddy_id, LLRelationship* buddy);
	typedef std::map<std::string, LLUUID, LLDictionaryLess> buddy_map_t;
	buddy_map_t mOnline;
	std::string mFullName;
};

// collect dictionary sorted map of name -> agent_id for every buddy,
// one map is offline and the other map is online.
class LLCollectAllBuddies : public LLRelationshipFunctor
{
public:
	LLCollectAllBuddies() {}
	virtual ~LLCollectAllBuddies() {}
	virtual bool operator()(const LLUUID& buddy_id, LLRelationship* buddy);
	typedef std::map<std::string, LLUUID, LLDictionaryLess> buddy_map_t;
	buddy_map_t mOnline;
	buddy_map_t mOffline;
	std::string mFullName;
};

#endif // LL_LLCALLINGCARD_H
