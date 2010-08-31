/** 
 * @file llrecentpeople.h
 * @brief List of people with which the user has recently interacted.
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
	 * @return false if the avatar is in the list already, true otherwise
	 */
	bool add(const LLUUID& id);

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

	const LLDate& getDate(const LLUUID& id) const;

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
	typedef std::map<LLUUID, LLDate> recent_people_t;
	recent_people_t		mPeople;
	signal_t			mChangedSignal;
};

#endif // LL_LLRECENTPEOPLE_H
