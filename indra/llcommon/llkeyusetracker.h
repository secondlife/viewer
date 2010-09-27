/** 
 * @file llkeyusetracker.h
 * @brief Declaration of the LLKeyUseTracker class.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LL_KEYUSETRACKER_H
#define LL_KEYUSETRACKER_H

// This is a generic cache for an arbitrary data type indexed against an
// arbitrary key type. The cache length is determined by expiration time
// tince against last use and set size. The age of elements and the size
// of the cache are queryable.
//
// This is implemented as a list, which makes search O(n). If you reuse this
// for something with a large dataset, consider reimplementing with a Boost
// multimap based on both a map(key) and priority queue(last_use).

template <class TKey, class TData>
class KeyUseTrackerNodeImpl
{
public:
	U64 mLastUse;
	U32 mUseCount;
	TKey mKey;
	TData mData;

	KeyUseTrackerNodeImpl( TKey k, TData d ) :
		mLastUse(0),
		mUseCount(0),
		mKey( k ),
		mData( d )
	{
	}
};


template <class TKey, class TData>
class LLKeyUseTracker
{
	typedef KeyUseTrackerNodeImpl<TKey,TData> TKeyUseTrackerNode;
	typedef std::list<TKeyUseTrackerNode *> TKeyList;
	TKeyList mKeyList;
	U64 mMemUsecs;
	U64 mLastExpire;
	U32 mMaxCount;
	U32 mCount;

	static U64 getTime()
	{
		// This function operates on a frame basis, not instantaneous.
		// We can rely on its output for determining first use in a
		// frame.
		return LLFrameTimer::getTotalTime();
	}

	void ageKeys()
	{
		U64 now = getTime();
		if( now == mLastExpire )
		{
			return;
		}
		mLastExpire = now;

		while( !mKeyList.empty() && (now - mKeyList.front()->mLastUse > mMemUsecs ) )
		{
			delete mKeyList.front();
			mKeyList.pop_front();
			mCount--;
		}
	}

	TKeyUseTrackerNode *findNode( TKey key )
	{
		ageKeys();

		typename TKeyList::iterator i;
		for( i = mKeyList.begin(); i != mKeyList.end(); i++ )
		{
			if( (*i)->mKey == key )
				return *i;
		}

		return NULL;
	}

	TKeyUseTrackerNode *removeNode( TKey key )
	{
		TKeyUseTrackerNode *i;
		i = findNode( key );
		if( i )
		{
			mKeyList.remove( i );
			mCount--;
			return i;
		}

		return NULL;
	}

public:
	LLKeyUseTracker( U32 memory_seconds, U32 max_count ) :
		mLastExpire(0),
		mMaxCount( max_count ),
		mCount(0)
	{
		mMemUsecs = ((U64)memory_seconds) * 1000000;
	}

	~LLKeyUseTracker()
	{
		// Flush list
		while( !mKeyList.empty() )
		{
			delete mKeyList.front();
			mKeyList.pop_front();
			mCount--;
		}
	}

	void markUse( TKey key, TData data )
	{
		TKeyUseTrackerNode *node = removeNode( key );
		if( !node )
		{
			node = new TKeyUseTrackerNode(key, data);
		}
		else
		{
			// Update data
			node->mData = data;
		}
		node->mLastUse = getTime();
		node->mUseCount++;
		mKeyList.push_back( node );
		mCount++;

		// Too many items? Drop head
		if( mCount > mMaxCount )
		{
			delete mKeyList.front();
			mKeyList.pop_front();
			mCount--;
		}
	}

	void forgetKey( TKey key )
	{
		TKeyUseTrackerNode *node = removeNode( key );
		if( node )
		{
			delete node;
		}
	}

	U32 getUseCount( TKey key )
	{
		TKeyUseTrackerNode *node = findNode( key );
		if( node )
		{
			return node->mUseCount;
		}
		return 0;
	}

	U64 getTimeSinceUse( TKey key )
	{
		TKeyUseTrackerNode *node = findNode( key );
		if( node )
		{
			U64 now = getTime();
			U64 delta = now - node->mLastUse;
			return (U32)( delta / 1000000 );
		}
		return 0;
	}

	TData *getLastUseData( TKey key )
	{
		TKeyUseTrackerNode *node = findNode( key );
		if( node )
		{
			return &node->mData;
		}
		return NULL;
	}

	U32 getKeyCount()
	{
		return mCount;
	}
};

#endif
