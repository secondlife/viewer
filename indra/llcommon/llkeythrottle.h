/** 
 * @file llkeythrottle.h
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2008, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLKEY_THROTTLE_H
#define LL_LLKEY_THROTTLE_H

// LLKeyThrottle keeps track of the number of action occurences with a key value
// for a type over a given time period.  If the rate set in the constructor is
// exceeed, the key is considered blocked.  The transition from unblocked to
// blocked is noted so the responsible agent can be informed.  This transition
// takes twice the look back window to clear.

#include "linden_common.h"

#include "llframetimer.h"
#include <map>


// forward declaration so LLKeyThrottleImpl can befriend it
template <class T> class LLKeyThrottle;


// Implementation utility class - use LLKeyThrottle, not this
template <class T>
class LLKeyThrottleImpl
{
	friend class LLKeyThrottle<T>;
protected:
	struct Entry {
		U32		count;
		BOOL	blocked;

		Entry() : count(0), blocked(FALSE) { }
	};

	typedef std::map<T, Entry> EntryMap;

	EntryMap * prevMap;
	EntryMap * currMap;
	
	U32 countLimit;
		// maximum number of keys allowed per interval
		
	U64 interval_usec;
		// each map covers this time period
	U64 start_usec;
		// currMap started counting at this time
		// prevMap covers the previous interval
	
	LLKeyThrottleImpl() : prevMap(0), currMap(0),
			      countLimit(0), interval_usec(0),
			      start_usec(0) { };

	static U64 getTime()
	{
		return LLFrameTimer::getTotalTime();
	}
};


template< class T >
class LLKeyThrottle
{
public:
	LLKeyThrottle(U32 limit, F32 interval)
		: m(* new LLKeyThrottleImpl<T>)
	{
		// limit is the maximum number of keys
		// allowed per interval (in seconds)
		m.countLimit = limit;
		m.interval_usec = (U64)(interval * USEC_PER_SEC);
		m.start_usec = LLKeyThrottleImpl<T>::getTime();

		m.prevMap = new typename LLKeyThrottleImpl<T>::EntryMap;
		m.currMap = new typename LLKeyThrottleImpl<T>::EntryMap;
	}
		
	~LLKeyThrottle()
	{
		delete m.prevMap;
		delete m.currMap;
		delete &m;
	}

	enum State {
		THROTTLE_OK,			// rate not exceeded, let pass
		THROTTLE_NEWLY_BLOCKED,	// rate exceed for the first time
		THROTTLE_BLOCKED,		// rate exceed, block key
	};

	// call each time the key wants use
	State noteAction(const T& id, S32 weight = 1)
	{
		U64 now = LLKeyThrottleImpl<T>::getTime();

		if (now >= (m.start_usec + m.interval_usec))
		{
			if (now < (m.start_usec + 2 * m.interval_usec))
			{
				// prune old data
				delete m.prevMap;
				m.prevMap = m.currMap;
				m.currMap = new typename LLKeyThrottleImpl<T>::EntryMap;

				m.start_usec += m.interval_usec;
			}
			else
			{
				// lots of time has passed, all data is stale
				delete m.prevMap;
				delete m.currMap;
				m.prevMap = new typename LLKeyThrottleImpl<T>::EntryMap;
				m.currMap = new typename LLKeyThrottleImpl<T>::EntryMap;

				m.start_usec = now;
			}
		}

		U32 prevCount = 0;
		BOOL prevBlocked = FALSE;

		typename LLKeyThrottleImpl<T>::EntryMap::const_iterator prev = m.prevMap->find(id);
		if (prev != m.prevMap->end())
		{
			prevCount = prev->second.count;
			prevBlocked = prev->second.blocked;
		}

		typename LLKeyThrottleImpl<T>::Entry& curr = (*m.currMap)[id];

		bool wereBlocked = curr.blocked || prevBlocked;

		curr.count += weight;

		// curr.count is the number of keys in
		// this current 'time slice' from the beginning of it until now
		// prevCount is the number of keys in the previous
		// time slice scaled to be one full time slice back from the current 
		// (now) time.

		// compute current, windowed rate
		F64 timeInCurrent = ((F64)(now - m.start_usec) / m.interval_usec);
		F64 averageCount = curr.count + prevCount * (1.0 - timeInCurrent);
		
		curr.blocked |= averageCount > m.countLimit;

		bool nowBlocked = curr.blocked || prevBlocked;

		if (!nowBlocked)
		{
			return THROTTLE_OK;
		}
		else if (!wereBlocked)
		{
			return THROTTLE_NEWLY_BLOCKED;
		}
		else
		{
			return THROTTLE_BLOCKED;
		}
	}

	// call to force throttle conditions for id
	void throttleAction(const T& id)
	{
		noteAction(id);
		typename LLKeyThrottleImpl<T>::Entry& curr = (*m.currMap)[id];
		if (curr.count < m.countLimit)
		{
			curr.count = m.countLimit;
		}
		curr.blocked = TRUE;
	}

	// returns TRUE if key is blocked
	BOOL isThrottled(const T& id) const
	{
		if (m.currMap->empty()
			&& m.prevMap->empty())
		{
			// most of the time we'll fall in here
			return FALSE;
		}

		// NOTE, we ignore the case where id is in the map but the map is stale.  
		// You might think that we'd stop throttling things in such a case, 
		// however it may be that a god has disabled scripts in the region or 
		// estate --> we probably want to report the state of the id when the 
		// scripting engine was paused.
		typename LLKeyThrottleImpl<T>::EntryMap::const_iterator entry = m.currMap->find(id);
		if (entry != m.currMap->end())
		{
			return entry->second.blocked;
		}
		entry = m.prevMap->find(id);
		if (entry != m.prevMap->end())
		{
			return entry->second.blocked;
		}
		return FALSE;
	}

protected:
	LLKeyThrottleImpl<T>& m;
};

#endif
