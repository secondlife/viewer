/** 
 * @file llindexedqueue.h
 * @brief An indexed FIFO queue, where only one element with each key
 * can be in the queue.
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

#ifndef LL_LLINDEXEDQUEUE_H
#define LL_LLINDEXEDQUEUE_H

// An indexed FIFO queue, where only one element with each key can be in the queue.
// This is ONLY used in the interest list, you'll probably want to review this code
// carefully if you want to use it elsewhere - Doug

template <typename Type> 
class LLIndexedQueue
{
protected:
	typedef std::deque<Type> type_deque;
	type_deque mQueue;
	std::set<Type> mKeySet;

public:
	LLIndexedQueue() {}

	// move_if_there is an O(n) operation
	bool push_back(const Type &value, bool move_if_there = false)
	{
		if (mKeySet.find(value) != mKeySet.end())
		{
			// Already on the queue
			if (move_if_there)
			{
				// Remove the existing entry.
				typename type_deque::iterator it;
				for (it = mQueue.begin(); it != mQueue.end(); ++it)
				{
					if (*it == value)
					{
						break;
					}
				}

				// This HAS to succeed, otherwise there's a serious bug in the keyset implementation
				// (although this isn't thread safe, at all)

				mQueue.erase(it);
			}
			else
			{
				// We're not moving it, leave it alone
				return false;
			}
		}
		else
		{
			// Doesn't exist, add it to the key set
			mKeySet.insert(value);
		}

		mQueue.push_back(value);

		// We succeeded in adding the new element.
		return true;
	}

	bool push_front(const Type &value, bool move_if_there = false)
	{
		if (mKeySet.find(value) != mKeySet.end())
		{
			// Already on the queue
			if (move_if_there)
			{
				// Remove the existing entry.
				typename type_deque::iterator it;
				for (it = mQueue.begin(); it != mQueue.end(); ++it)
				{
					if (*it == value)
					{
						break;
					}
				}

				// This HAS to succeed, otherwise there's a serious bug in the keyset implementation
				// (although this isn't thread safe, at all)

				mQueue.erase(it);
			}
			else
			{
				// We're not moving it, leave it alone
				return false;
			}
		}
		else
		{
			// Doesn't exist, add it to the key set
			mKeySet.insert(value);
		}

		mQueue.push_front(value);
		return true;
	}

	void pop()
	{
		Type value = mQueue.front();
		mKeySet.erase(value);
		mQueue.pop_front();
	}

	Type &front()
	{
		return mQueue.front();
	}

	S32 size() const
	{
		return mQueue.size();
	}

	bool empty() const
	{
		return mQueue.empty();
	}

	void clear()
	{
		// Clear out all elements on the queue
		mQueue.clear();
		mKeySet.clear();
	}
};

#endif // LL_LLINDEXEDQUEUE_H
