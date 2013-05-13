/** 
 * @file llindexedqueue.h
 * @brief An indexed FIFO queue, where only one element with each key
 * can be in the queue.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
