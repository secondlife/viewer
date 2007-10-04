/** 
 * @file lluuidhashmap.h
 * @brief A uuid based hash map.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#ifndef LL_LLUUIDHASHMAP_H
#define LL_LLUUIDHASHMAP_H

#include "stdtypes.h"
#include "llerror.h"
#include "lluuid.h"

// UUID hash map

	/*
	LLUUIDHashMap<uuid_pair, 32> foo(test_equals);
	LLUUIDHashMapIter<uuid_pair, 32> bar(&foo);

	LLDynamicArray<LLUUID> source_ids;
	const S32 COUNT = 100000;
	S32 q;
	for (q = 0; q < COUNT; q++)
	{
		llinfos << "Creating" << llendl;
		LLUUID id;
		id.generate();
		//llinfos << q << ":" << id << llendl;
		uuid_pair pair;
		pair.mUUID = id;
		pair.mValue = q;
		foo.set(id, pair);
		source_ids.put(id);
		//ms_sleep(1);
	}

	uuid_pair cur;
	llinfos << "Iterating" << llendl;
	for (cur = bar.first(); !bar.done(); cur = bar.next())
	{
		if (source_ids[cur.mValue] != cur.mUUID)
		{
			llerrs << "Incorrect value iterated!" << llendl;
		}
		//llinfos << cur.mValue << ":" << cur.mUUID << llendl;
		//ms_sleep(1);
	}

	llinfos << "Finding" << llendl;
	for (q = 0; q < COUNT; q++)
	{
		cur = foo.get(source_ids[q]);
		if (source_ids[cur.mValue] != cur.mUUID)
		{
			llerrs << "Incorrect value found!" << llendl;
		}
		//llinfos << res.mValue << ":" << res.mUUID << llendl;
		//ms_sleep(1);
	}
	
	llinfos << "Removing" << llendl;
	for (q = 0; q < COUNT/2; q++)
	{
		if (!foo.remove(source_ids[q]))
		{
			llerrs << "Remove failed!" << llendl;
		}
		//ms_sleep(1);
	}

	llinfos << "Iterating" << llendl;
	for (cur = bar.first(); !bar.done(); cur = bar.next())
	{
		if (source_ids[cur.mValue] != cur.mUUID)
		{
			llerrs << "Incorrect value found!" << llendl;
		}
		//llinfos << cur.mValue << ":" << cur.mUUID << llendl;
		//ms_sleep(1);
	}
	llinfos << "Done with UUID map test" << llendl;

	return 0;
	*/


//
// LLUUIDHashNode
//

template <class DATA, int SIZE>
class LLUUIDHashNode
{
public:
	LLUUIDHashNode();

public:
	S32 mCount;
	U8	mKey[SIZE];
	DATA mData[SIZE];
	LLUUIDHashNode<DATA, SIZE> *mNextNodep;
};


//
// LLUUIDHashNode implementation
//
template <class DATA, int SIZE>
LLUUIDHashNode<DATA, SIZE>::LLUUIDHashNode()
{
	mCount = 0;
	mNextNodep = NULL;
}


template <class DATA_TYPE, int SIZE>
class LLUUIDHashMap
{
public:
	// basic constructor including sorter
	LLUUIDHashMap(BOOL (*equals)(const LLUUID &uuid, const DATA_TYPE &data),
				  const DATA_TYPE &null_data);
	~LLUUIDHashMap();

	inline DATA_TYPE &get(const LLUUID &uuid);
	inline BOOL check(const LLUUID &uuid) const;
	inline DATA_TYPE &set(const LLUUID &uuid, const DATA_TYPE &type);
	inline BOOL remove(const LLUUID &uuid);
	void removeAll();

	inline S32 getLength() const; // Warning, NOT O(1!)
public:
	BOOL (*mEquals)(const LLUUID &uuid, const DATA_TYPE &data);
	LLUUIDHashNode<DATA_TYPE, SIZE> mNodes[256];

	S32 mIterCount;
protected:
	DATA_TYPE mNull;
};


//
// LLUUIDHashMap implementation
//

template <class DATA_TYPE, int SIZE>
LLUUIDHashMap<DATA_TYPE, SIZE>::LLUUIDHashMap(BOOL (*equals)(const LLUUID &uuid, const DATA_TYPE &data),
											  const DATA_TYPE &null_data)
:	mEquals(equals),
	mIterCount(0),
	mNull(null_data)
{ }

template <class DATA_TYPE, int SIZE>
LLUUIDHashMap<DATA_TYPE, SIZE>::~LLUUIDHashMap()
{
	removeAll();
}

template <class DATA_TYPE, int SIZE>
void LLUUIDHashMap<DATA_TYPE, SIZE>::removeAll()
{
	S32 bin;
	for (bin = 0; bin < 256; bin++)
	{
		LLUUIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[bin];

		BOOL first = TRUE;
		while (nodep)
		{
			S32 i;
			const S32 count = nodep->mCount;

			// Iterate through all members of this node
			for (i = 0; i < count; i++)
			{
				nodep->mData[i] = mNull;
			}

			nodep->mCount = 0;
			// Done with all objects in this node, go to the next.

			LLUUIDHashNode<DATA_TYPE, SIZE>* curp = nodep;
			nodep = nodep->mNextNodep;

			// Delete the node if it's not the first node
			if (first)
			{
				first = FALSE;
				curp->mNextNodep = NULL;
			}
			else
			{
				delete curp;
			}
		}
	}
}

template <class DATA_TYPE, int SIZE>
inline S32 LLUUIDHashMap<DATA_TYPE, SIZE>::getLength() const
{
	S32 count = 0;
	S32 bin;
	for (bin = 0; bin < 256; bin++)
	{
		LLUUIDHashNode<DATA_TYPE, SIZE>* nodep = (LLUUIDHashNode<DATA_TYPE, SIZE>*) &mNodes[bin];
		while (nodep)
		{
			count += nodep->mCount;
			nodep = nodep->mNextNodep;
		}
	}
	return count;
}

template <class DATA_TYPE, int SIZE>
inline DATA_TYPE &LLUUIDHashMap<DATA_TYPE, SIZE>::get(const LLUUID &uuid)
{
	LLUUIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[uuid.mData[0]];

	// Grab the second byte of the UUID, which is the key for the node data
	const S32 second_byte = uuid.mData[1];
	while (nodep)
	{
		S32 i;
		const S32 count = nodep->mCount;

		// Iterate through all members of this node
		for (i = 0; i < count; i++)
		{
			if ((nodep->mKey[i] == second_byte) && mEquals(uuid, nodep->mData[i]))
			{
				// The second byte matched, and our equality test passed.
				// We found it.
				return nodep->mData[i];
			}
		}

		// Done with all objects in this node, go to the next.
		nodep = nodep->mNextNodep;
	}
	return mNull;
}


template <class DATA_TYPE, int SIZE>
inline BOOL LLUUIDHashMap<DATA_TYPE, SIZE>::check(const LLUUID &uuid) const
{
	const LLUUIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[uuid.mData[0]];

	// Grab the second byte of the UUID, which is the key for the node data
	const S32 second_byte = uuid.mData[1];
	while (nodep)
	{
		S32 i;
		const S32 count = nodep->mCount;

		// Iterate through all members of this node
		for (i = 0; i < count; i++)
		{
			if ((nodep->mKey[i] == second_byte) && mEquals(uuid, nodep->mData[i]))
			{
				// The second byte matched, and our equality test passed.
				// We found it.
				return TRUE;
			}
		}

		// Done with all objects in this node, go to the next.
		nodep = nodep->mNextNodep;
	}

	// Didn't find anything
	return FALSE;
}


template <class DATA_TYPE, int SIZE>
inline DATA_TYPE &LLUUIDHashMap<DATA_TYPE, SIZE>::set(const LLUUID &uuid, const DATA_TYPE &data)
{
	// Set is just like a normal find, except that if we find a match
	// we replace it with the input value.
	// If we don't find a match, we append to the end of the list.

	LLUUIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[uuid.mData[0]];

	const S32 second_byte = uuid.mData[1];
	while (1)
	{
		const S32 count = nodep->mCount;

		S32 i;
		for (i = 0; i < count; i++)
		{
			if ((nodep->mKey[i] == second_byte) && mEquals(uuid, nodep->mData[i]))
			{
				// We found a match for this key, replace the data with
				// the incoming data.
				nodep->mData[i] = data;
				return nodep->mData[i];
			}
		}
		if (!nodep->mNextNodep)
		{
			// We've iterated through all of the keys without finding a match
			if (i < SIZE)
			{
				// There's still some space on this node, append
				// the key and data to it.
				nodep->mKey[i] = second_byte;
				nodep->mData[i] = data;
				nodep->mCount++;

				return nodep->mData[i];
			}
			else
			{
				// This node is full, append a new node to the end.
				nodep->mNextNodep = new LLUUIDHashNode<DATA_TYPE, SIZE>;
				nodep->mNextNodep->mKey[0] = second_byte;
				nodep->mNextNodep->mData[0] = data;
				nodep->mNextNodep->mCount = 1;

				return nodep->mNextNodep->mData[0];
			}
		}

		// No match on this node, go to the next
		nodep = nodep->mNextNodep;
	}
}


template <class DATA_TYPE, int SIZE>
inline BOOL LLUUIDHashMap<DATA_TYPE, SIZE>::remove(const LLUUID &uuid)
{
	if (mIterCount)
	{
		// We don't allow remove when we're iterating, it's bad karma!
		llerrs << "Attempted remove while an outstanding iterator in LLUUIDHashMap!" << llendl;
	}
	// Remove is the trickiest operation.
	// What we want to do is swap the last element of the last
	// node if we find the one that we want to remove, but we have
	// to deal with deleting the node from the tail if it's empty, but
	// NOT if it's the only node left.

	LLUUIDHashNode<DATA_TYPE, SIZE> *nodep = &mNodes[uuid.mData[0]];

	// Not empty, we need to search through the nodes
	const S32 second_byte = uuid.mData[1];

	// A modification of the standard search algorithm.
	while (nodep)
	{
		const S32 count = nodep->mCount;

		S32 i;
		for (i = 0; i < count; i++)
		{
			if ((nodep->mKey[i] == second_byte) && mEquals(uuid, nodep->mData[i]))
			{
				// We found the node that we want to remove.
				// Find the last (and next-to-last) node, and the index of the last
				// element.  We could conceviably start from the node we're on,
				// but that makes it more complicated, this is easier.

				LLUUIDHashNode<DATA_TYPE, SIZE> *prevp = &mNodes[uuid.mData[0]];
				LLUUIDHashNode<DATA_TYPE, SIZE> *lastp = prevp;

				// Find the last and next-to-last
				while (lastp->mNextNodep)
				{
					prevp = lastp;
					lastp = lastp->mNextNodep;
				}

				// First, swap in the last to the current location.
				nodep->mKey[i] = lastp->mKey[lastp->mCount - 1];
				nodep->mData[i] = lastp->mData[lastp->mCount - 1];

				// Now, we delete the entry
				lastp->mCount--;
				lastp->mData[lastp->mCount] = mNull;

				if (!lastp->mCount)
				{
					// We deleted the last element!
					if (lastp != &mNodes[uuid.mData[0]])
					{
						// Only blitz the node if it's not the head
						// Set the previous node to point to NULL, then
						// blitz the empty last node
						prevp->mNextNodep = NULL;
						delete lastp;
					}
				}
				return TRUE;
			}
		}

		// Iterate to the next node, we've scanned all the entries in this one.
		nodep = nodep->mNextNodep;
	}
	return FALSE;
}


//
// LLUUIDHashMapIter
//

template <class DATA_TYPE, int SIZE>
class LLUUIDHashMapIter
{
public:
	LLUUIDHashMapIter(LLUUIDHashMap<DATA_TYPE, SIZE> *hash_mapp);
	~LLUUIDHashMapIter();


	inline void reset();
	inline void first();
	inline void next();
	inline BOOL done() const;

	DATA_TYPE& operator*() const
	{
		return mCurHashNodep->mData[mCurHashNodeKey];
	}
	DATA_TYPE* operator->() const
	{
		return &(operator*());
	}

protected:
	LLUUIDHashMap<DATA_TYPE, SIZE> *mHashMapp;
	LLUUIDHashNode<DATA_TYPE, SIZE> *mCurHashNodep;

	S32 mCurHashMapNodeNum;
	S32 mCurHashNodeKey;

	DATA_TYPE mNull;
};


//
// LLUUIDHashMapIter Implementation
//
template <class DATA_TYPE, int SIZE>
LLUUIDHashMapIter<DATA_TYPE, SIZE>::LLUUIDHashMapIter(LLUUIDHashMap<DATA_TYPE, SIZE> *hash_mapp)
{
	mHashMapp = hash_mapp;
	mCurHashNodep = NULL;
	mCurHashMapNodeNum = 0;
	mCurHashNodeKey = 0;
}

template <class DATA_TYPE, int SIZE>
LLUUIDHashMapIter<DATA_TYPE, SIZE>::~LLUUIDHashMapIter()
{
	reset();
}

template <class DATA_TYPE, int SIZE>
inline void LLUUIDHashMapIter<DATA_TYPE, SIZE>::reset()
{
	if (mCurHashNodep)
	{
		// We're partway through an iteration, we can clean up now
		mHashMapp->mIterCount--;
		mCurHashNodep = NULL;
	}
}

template <class DATA_TYPE, int SIZE>
inline void LLUUIDHashMapIter<DATA_TYPE, SIZE>::first()
{
	// Iterate through until we find the first non-empty node;
	S32 i;
	for (i = 0; i < 256; i++)
	{
		if (mHashMapp->mNodes[i].mCount)
		{
			if (!mCurHashNodep)
			{
				// Increment, since it's no longer safe for us to do a remove
				mHashMapp->mIterCount++;
			}

			mCurHashNodep = &mHashMapp->mNodes[i];
			mCurHashMapNodeNum = i;
			mCurHashNodeKey = 0;
			//return mCurHashNodep->mData[0];
			return;
		}
	}

	// Completely empty!
	mCurHashNodep = NULL;
	//return mNull;
	return;
}

template <class DATA_TYPE, int SIZE>
inline BOOL LLUUIDHashMapIter<DATA_TYPE, SIZE>::done() const
{
	return mCurHashNodep ? FALSE : TRUE;
}

template <class DATA_TYPE, int SIZE>
inline void LLUUIDHashMapIter<DATA_TYPE, SIZE>::next()
{
	// No current entry, this iterator is done
	if (!mCurHashNodep)
	{
		//return mNull;
		return;
	}

	// Go to the next element
	mCurHashNodeKey++;
	if (mCurHashNodeKey < mCurHashNodep->mCount)
	{
		// We're not done with this node, return the current element
		//return mCurHashNodep->mData[mCurHashNodeKey];
		return;
	}

	// Done with this node, move to the next
	mCurHashNodep = mCurHashNodep->mNextNodep;
	if (mCurHashNodep)
	{
		// Return the first element
		mCurHashNodeKey = 0;
		//return mCurHashNodep->mData[0];
		return;
	}

	// Find the next non-empty node (keyed on the first byte)
	mCurHashMapNodeNum++;

	S32 i;
	for (i = mCurHashMapNodeNum; i < 256; i++)
	{
		if (mHashMapp->mNodes[i].mCount)
		{
			// We found one that wasn't empty
			mCurHashNodep = &mHashMapp->mNodes[i];
			mCurHashMapNodeNum = i;
			mCurHashNodeKey = 0;
			//return mCurHashNodep->mData[0];
			return;
		}
	}

	// OK, we're done, nothing else to iterate
	mCurHashNodep = NULL;
	mHashMapp->mIterCount--; // Decrement since we're safe to do removes now
	//return mNull;
}

#endif // LL_LLUUIDHASHMAP_H
