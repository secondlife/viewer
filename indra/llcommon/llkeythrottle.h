/** 
 * @file llkeythrottle.h
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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
		bool	blocked;

		Entry() : count(0), blocked(false) { }
	};

	typedef std::map<T, Entry> EntryMap;

	EntryMap* prevMap;
	EntryMap* currMap;
	
	U32 countLimit;
		// maximum number of keys allowed per interval
		
	U64 intervalLength;		// each map covers this time period (usec or frame number)
	U64 startTime;			// start of the time period (usec or frame number)
		// currMap started counting at this time
		// prevMap covers the previous interval
	
	LLKeyThrottleImpl() :
		prevMap(NULL),
		currMap(NULL),
		countLimit(0),
		intervalLength(1),
		startTime(0)
	{}

	static U64 getTime()
	{
		return LLFrameTimer::getTotalTime();
	}
	static U64 getFrame()		// Return the current frame number
	{
		return (U64) LLFrameTimer::getFrameCount();
	}
};


template< class T >
class LLKeyThrottle
{
public:
	// @param realtime = FALSE for frame-based throttle, TRUE for usec
	// real-time throttle
	LLKeyThrottle(U32 limit, F32 interval, BOOL realtime = TRUE)	
		: m(* new LLKeyThrottleImpl<T>)
	{
		setParameters( limit, interval, realtime );
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
		U64 now = 0;
		if ( mIsRealtime )
		{
			now = LLKeyThrottleImpl<T>::getTime();
		}
		else
		{
			now = LLKeyThrottleImpl<T>::getFrame();
		}

		if (now >= (m.startTime + m.intervalLength))
		{
			if (now < (m.startTime + 2 * m.intervalLength))
			{
				// prune old data
				delete m.prevMap;
				m.prevMap = m.currMap;
				m.currMap = new typename LLKeyThrottleImpl<T>::EntryMap;

				m.startTime += m.intervalLength;
			}
			else
			{
				// lots of time has passed, all data is stale
				delete m.prevMap;
				delete m.currMap;
				m.prevMap = new typename LLKeyThrottleImpl<T>::EntryMap;
				m.currMap = new typename LLKeyThrottleImpl<T>::EntryMap;

				m.startTime = now;
			}
		}

		U32 prevCount = 0;
		bool prevBlocked = false;

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
		F64 timeInCurrent = ((F64)(now - m.startTime) / m.intervalLength);
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
		curr.count = llmax(m.countLimit, curr.count);
		curr.blocked = true;
	}

	// returns true if key is blocked
	bool isThrottled(const T& id) const
	{
		if (m.currMap->empty()
			&& m.prevMap->empty())
		{
			// most of the time we'll fall in here
			return false;
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
		return false;
	}

	// Get the throttling parameters
	void getParameters( U32 & out_limit, F32 & out_interval, BOOL & out_realtime )
	{
		out_limit = m.countLimit;
		out_interval = m.intervalLength;
		out_realtime = mIsRealtime;
	}

	// Set the throttling behavior
	void setParameters( U32 limit, F32 interval, BOOL realtime = TRUE )
	{
		// limit is the maximum number of keys
		// allowed per interval (in seconds or frames)
		mIsRealtime = realtime;
		m.countLimit = limit;
		if ( mIsRealtime )
		{
			m.intervalLength = (U64)(interval * USEC_PER_SEC);
			m.startTime = LLKeyThrottleImpl<T>::getTime();
		}
		else
		{
			m.intervalLength = (U64)interval;
			m.startTime = LLKeyThrottleImpl<T>::getFrame();
		}

		if ( m.intervalLength == 0 )
		{	// Don't allow zero intervals
			m.intervalLength = 1;
		}

		delete m.prevMap;
		m.prevMap = new typename LLKeyThrottleImpl<T>::EntryMap;
		delete m.currMap;
		m.currMap = new typename LLKeyThrottleImpl<T>::EntryMap;
	}

protected:
	LLKeyThrottleImpl<T>& m;
	BOOL	mIsRealtime;	// TRUE to be time based (default), FALSE for frame based
};

#endif
