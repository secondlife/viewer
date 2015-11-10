/** 
 * @file llrecentpeople.h
 * @brief List of people with which the user has recently interacted.
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

#ifndef LL_LLRECENTPEOPLE_H
#define LL_LLRECENTPEOPLE_H

#include "llevent.h"
#include "llsingleton.h"
#include "lluuid.h"

#include <vector>
#include <set>
#include <boost/signals2.hpp>

class LLDate;

/**
 * List of people the agent recently interacted with.
 * 
 * Includes: anyone with whom the user IM'd or called
 * (1:1 and ad-hoc but not SL Group chat),
 * anyone with whom the user has had a transaction
 * (inventory offer, friend request, etc),
 * and anyone that has chatted within chat range of the user in-world. 
 * 
 *TODO: purge least recently added items? 
 */
class LLRecentPeople: public LLSingleton<LLRecentPeople>, public LLOldEvents::LLSimpleListener
{
	LOG_CLASS(LLRecentPeople);
public:
	typedef boost::signals2::signal<void ()> signal_t;
	
	/**
	 * Add specified avatar to the list if it's not there already.
	 *
	 * @param id avatar to add.
	 *
	 * @param userdata additional information about last interaction party.
	 *				   For example when last interaction party is not an avatar
	 *				   but an avaline caller, additional info (such as phone
	 *				   number, session id and etc.) should be added.
	 *
	 * @return false if the avatar is in the list already, true otherwise
	 */
	bool add(const LLUUID& id, const LLSD& userdata = LLSD().with("date", LLDate::now()));

	/**
	 * @param id avatar to search.
	 * @return true if the avatar is in the list, false otherwise.
	 */
	bool contains(const LLUUID& id) const;

	/**
	 * Get the whole list.
	 * 
	 * @param result where to put the result.
	 */
	void get(uuid_vec_t& result) const;

	/**
	 * Returns last interaction time with specified participant
	 *
	 */
	const LLDate getDate(const LLUUID& id) const;

	/**
	 * Returns data about specified participant
	 *
	 * @param id identifier of specific participant
	 */
	const LLSD& getData(const LLUUID& id) const;

	/**
	 * Checks whether specific participant is an avaline caller
	 *
	 * @param id identifier of specific participant
	 */
	bool isAvalineCaller(const LLUUID& id) const;

	/**
	 * Set callback to be called when the list changed.
	 * 
	 * Multiple callbacks can be set.
	 * 
	 * @return no connection; use boost::bind + boost::signals2::trackable to disconnect slots.
	 */
	void setChangedCallback(const signal_t::slot_type& cb) { mChangedSignal.connect(cb); }

	/**
	 * LLSimpleListener interface.
	 */
	/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);

private:

	const LLUUID& getIDByPhoneNumber(const LLSD& userdata);

	typedef std::map<LLUUID, LLSD> recent_people_t;
	recent_people_t		mPeople;
	signal_t			mChangedSignal;
};

#endif // LL_LLRECENTPEOPLE_H
