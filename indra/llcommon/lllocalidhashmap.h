/** 
 * @file lllocalidhashmap.h
 * @brief Map specialized for dealing with local ids
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

#ifndef LL_LLLOCALIDHASHMAP_H
#define LL_LLLOCALIDHASHMAP_H

#include "stdtypes.h"
#include "llerror.h"

const S32 MAX_ITERS = 4;
// LocalID hash map

//
// LLLocalIDHashNode
//

template <class DATA, int SIZE>
class LLLocalIDHashNode
{
public:
	LLLocalIDHashNode();

public:
	S32 mCount;
	U32	mKey[SIZE];
	DATA mData[SIZE];
	LLLocalIDHashNode<DATA, SIZE> *mNextNodep;
};


//
// LLLocalIDHashNode implementation
//
template <class DATA, int SIZE>
LLLocalIDHashNode<DATA, SIZE>::LLLocalIDHashNode()
{
	mCount = 0;
	mNextNodep = NULL;
}

//
// LLLocalIDHashMapIter
//
template <class DATA_TYPE, int SIZE>
class LLLocalIDHashMap;

template <class DATA_TYPE, int SIZE>
class LLLocalIDHashMapIter
{
public:
	LLLocalIDHashMapIter(LLLocalIDHashMap<DATA_TYPE, SIZE> *hash_mapp);
	~LLLocalIDHashMapIter();

	void setMap(LLLocalIDHashMap<DATA_TYPE, SIZE> *hash_mapp);
	inline void first();
	inline void next();
	inline DATA_TYPE& current(); // *NOTE: Deprecate? Phoenix 2005-04-15
	inline BOOL done() const;
	inline S32  currentBin() const;
	inline void setBin(S32 bin);

	DATA_TYPE& operator*() const
	{
		return mCurHashNodep->mData[mCurHashNodeKey];
	}
	DATA_TYPE* operator->() const
	{
		return &(operator*());
	}

	LLLocalIDHashMap<DATA_TYPE, SIZE> *mHashMapp;
	LLLocalIDHashNode<DATA_TYPE, SIZE> *mCurHashNodep;

	S32 mCurHashMapNodeNum;
	S32 mCurHashNodeKey;

	DATA_TYPE mNull;

	S32 mIterID;
};



template <class DATA_TYPE, int SIZE>
class LLLocalIDHashMap
{
public:
	friend class LLLocalIDHashMapIter<DATA_TYPE, SIZE>;

	LLLocalIDHashMap(); // DO NOT use this unless you explicitly setNull, or the data type constructs a "null"
						// object by default
	// basic constructor including sorter
	LLLocalIDHashMap(const DATA_TYPE &null_data);
	// Hack, this should really be a const ref, but I'm not doing it that way because the sim
	// usually uses pointers.
	~LLLocalIDHashMap();

	inline DATA_TYPE &get(const U32 local_id);
	inline BOOL check(const U32 local_id) const;
	inline DATA_TYPE &set(const U32 local_id, const DATA_TYPE data);
	inline BOOL remove(const U32 local_id);
	void removeAll();

	void setNull(const DATA_TYPE data) { mNull = data; }

	inline S32 getLength() const; // Warning, NOT O(1!)

	void dumpIter();
	void dumpBin(U32 bin);

protected:
	// Only used by the iterator.
	void addIter(LLLocalIDHashMapIter<DATA_TYPE, SIZE> *iter);
	void removeIter(LLLocalIDHashMapIter<DATA_TYPE, SIZE> *iter);

	// Remove the item and shift all items afterward down the list,
	// fixing up iterators as we go.
	BOOL removeWithShift(const U32 local_id);

protected:
	LLLocalIDHashNode<DATA_TYPE, SIZE> mNodes[256];

	S32 mIterCount;
	LLLocalIDHashMapIter<DATA_TYPE, SIZE> *mIters[MAX_ITERS];

	DATA_TYPE mNull;
};


//
// LLLocalIDHashMap implementation
//

template <class DATA_TYPE, int SIZE>
LLLocalIDHashMap<DATA_TYPE, SIZE>::LLLocalIDHashMap()
:	mIterCount(0),
	mNull()
{
	S32 i;
	for (i = 0; i < MAX_ITERS; i++)
	{
		mIters[i] = NULL;
	}
}

template <class DATA_TYPE, int SIZE>
LLLocalIDHashMap<DATA_TYPE, SIZE>::LLLocalIDHashMap(const DATA_TYPE &null_data)
:	mIterCount(0),
	mNull(null_data)
{
	S32 i;
	for (i = 0; i < MAX_ITERS; i++)
	{
		mIters[i] = NULL;
	}
}

template <class DATA_TYPE, int SIZE>
LLLocalIDHashMap<DATA_TYPE, SIZE>::~LLLocalIDHashMap()
{
	S32 i;
	for (i = 0; i < MAX_ITERS; i++)
	{
		if (mIters[i])
		{
			mIters[i]->mHashMapp = NULL;
			mIterCount--;
		}
	}
	removeAll();
}

template <class DATA_TYPE, int SIZE>
void LLLocalIDHashMap<DATA_TYPE, SIZE>::removeAll()
{
	S32 bin;
	for (bin = 0; bin < 256; bin++)
	{
		LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[bin];

		BOOL first = TRUE;
		do // First node guaranteed to be there
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

			LLLocalIDHashNode<DATA_TYPE, SIZE>* curp = nodep;
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
		} while (nodep);
	}
}

template <class DATA_TYPE, int SIZE>
void LLLocalIDHashMap<DATA_TYPE, SIZE>::dumpIter()
{
	std::cout << "Hash map with " << mIterCount << " iterators" << std::endl;

	std::cout << "Hash Map Iterators:" << std::endl;
	S32 i;
	for (i = 0; i < MAX_ITERS; i++)
	{
		if (mIters[i])
		{
			llinfos << i << " " << mIters[i]->mCurHashNodep << " " << mIters[i]->mCurHashNodeKey << llendl;
		}
		else
		{
			llinfos << i << "null" << llendl;
		}
	}
}

template <class DATA_TYPE, int SIZE>
void LLLocalIDHashMap<DATA_TYPE, SIZE>::dumpBin(U32 bin)
{
	std::cout << "Dump bin " << bin << std::endl;

	LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[bin];
	S32 node = 0;
	do // First node guaranteed to be there.
	{
		std::cout << "Bin " << bin 
			<< " node " << node
			<< " count " << nodep->mCount
			<< " contains " << std::flush;

		S32 i;
		for (i = 0; i < nodep->mCount; i++)
		{
			std::cout << nodep->mData[i] << " " << std::flush;
		}

		std::cout << std::endl;

		nodep = nodep->mNextNodep;
		node++;
	} while (nodep);
}

template <class DATA_TYPE, int SIZE>
inline S32 LLLocalIDHashMap<DATA_TYPE, SIZE>::getLength() const
{
	S32 count = 0;
	S32 bin;
	for (bin = 0; bin < 256; bin++)
	{
		const LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[bin];
		while (nodep)
		{
			count += nodep->mCount;
			nodep = nodep->mNextNodep;
		}
	}
	return count;
}

template <class DATA_TYPE, int SIZE>
inline DATA_TYPE &LLLocalIDHashMap<DATA_TYPE, SIZE>::get(const U32 local_id)
{
	LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[local_id & 0xff];

	do // First node guaranteed to be there
	{
		S32 i;
		const S32 count = nodep->mCount;

		// Iterate through all members of this node
		for (i = 0; i < count; i++)
		{
			if (nodep->mKey[i] == local_id)
			{
				// We found it.
				return nodep->mData[i];
			}
		}

		// Done with all objects in this node, go to the next.
		nodep = nodep->mNextNodep;
	} while (nodep);

	return mNull;
}


template <class DATA_TYPE, int SIZE>
inline BOOL LLLocalIDHashMap<DATA_TYPE, SIZE>::check(const U32 local_id) const
{
	const LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[local_id & 0xff];

	do // First node guaranteed to be there
	{
		S32 i;
		const S32 count = nodep->mCount;

		// Iterate through all members of this node
		for (i = 0; i < count; i++)
		{
			if (nodep->mKey[i] == local_id)
			{
				// We found it.
				return TRUE;
			}
		}

		// Done with all objects in this node, go to the next.
		nodep = nodep->mNextNodep;
	} while (nodep);

	// Didn't find anything
	return FALSE;
}


template <class DATA_TYPE, int SIZE>
inline DATA_TYPE &LLLocalIDHashMap<DATA_TYPE, SIZE>::set(const U32 local_id, const DATA_TYPE data)
{
	// Set is just like a normal find, except that if we find a match
	// we replace it with the input value.
	// If we don't find a match, we append to the end of the list.

	LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[local_id & 0xff];

	while (1)
	{
		const S32 count = nodep->mCount;

		S32 i;
		for (i = 0; i < count; i++)
		{
			if (nodep->mKey[i] == local_id)
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
				nodep->mKey[i] = local_id;
				nodep->mData[i] = data;
				nodep->mCount++;

				return nodep->mData[i];
			}
			else
			{
				// This node is full, append a new node to the end.
				nodep->mNextNodep = new LLLocalIDHashNode<DATA_TYPE, SIZE>;
				nodep->mNextNodep->mKey[0] = local_id;
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
inline BOOL LLLocalIDHashMap<DATA_TYPE, SIZE>::remove(const U32 local_id)
{
	// Remove is the trickiest operation.
	// What we want to do is swap the last element of the last
	// node if we find the one that we want to remove, but we have
	// to deal with deleting the node from the tail if it's empty, but
	// NOT if it's the only node left.

	const S32 node_index = local_id & 0xff;

	LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[node_index];

	// A modification of the standard search algorithm.
	do // First node guaranteed to be there
	{
		const S32 count = nodep->mCount;

		S32 i;
		for (i = 0; i < count; i++)
		{
			if (nodep->mKey[i] == local_id)
			{
				// If we're removing the item currently pointed to by one
				// or more iterators, we can just swap in the last item
				// and back the iterator(s) up by one.
				// Otherwise, we need to do a slow and safe shift of all
				// items back to one position to fill the hole and fix up 
				// all iterators we find.
				BOOL need_shift = FALSE;
				S32 cur_iter;
				if (mIterCount)
				{
					for (cur_iter = 0; cur_iter < MAX_ITERS; cur_iter++)
					{
						if (mIters[cur_iter])
						{
							// We only care if the hash map node is on the one
							// that we're working on.  If it's before, we've already
							// traversed it, if it's after, changing the order doesn't
							// matter.
							if (mIters[cur_iter]->mCurHashMapNodeNum == node_index)
							{
								if ((mIters[cur_iter]->mCurHashNodep == nodep)
									&& (mIters[cur_iter]->mCurHashNodeKey == i))
								{
									// it's on the one we're deleting, we'll
									// fix the iterator quickly below.
								}
								else
								{
									// We're trying to remove an item on this
									// iterator's chain that this
									// iterator doesn't point to!  We need to do
									// the slow remove-and-shift-down case.
									need_shift = TRUE;
								}
							}
						}
					}
				}

				// Removing an item that isn't pointed to by all iterators
				if (need_shift)
				{
					return removeWithShift(local_id);
				}

				// Fix the iterators that point to this node/i pair, the
				// one we're deleting
				for (cur_iter = 0; cur_iter < MAX_ITERS; cur_iter++)
				{
					if (mIters[cur_iter])
					{
						// We only care if the hash map node is on the one
						// that we're working on.  If it's before, we've already
						// traversed it, if it's after, changing the order doesn't
						// matter.
						if (mIters[cur_iter]->mCurHashMapNodeNum == node_index)
						{
							if ((mIters[cur_iter]->mCurHashNodep == nodep)
								&& (mIters[cur_iter]->mCurHashNodeKey == i))
							{
								// We can handle the case where we're deleting 
								// the element we're on trivially (sort of).
								if (nodep->mCount > 1)
								{
									// If we're not going to delete this node,
									// it's OK.
									mIters[cur_iter]->mCurHashNodeKey--;
								}
								else
								{
									// We're going to delete this node, because this
									// is the last element on it.
									
									// Find the next node, and then back up one.
									mIters[cur_iter]->next();
									mIters[cur_iter]->mCurHashNodeKey--;
								}
							}
						}
					}
				}

				// We found the node that we want to remove.
				// Find the last (and next-to-last) node, and the index of the last
				// element.  We could conceviably start from the node we're on,
				// but that makes it more complicated, this is easier.

				LLLocalIDHashNode<DATA_TYPE, SIZE> *prevp = &mNodes[node_index];
				LLLocalIDHashNode<DATA_TYPE, SIZE> *lastp = prevp;

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
					if (lastp != &mNodes[local_id & 0xff])
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
	} while (nodep);

	return FALSE;
}

template <class DATA_TYPE, int SIZE>
BOOL LLLocalIDHashMap<DATA_TYPE, SIZE>::removeWithShift(const U32 local_id)
{
	const S32 node_index = local_id & 0xFF;
	LLLocalIDHashNode<DATA_TYPE, SIZE>* nodep = &mNodes[node_index];
	LLLocalIDHashNode<DATA_TYPE, SIZE>* prevp = NULL;
	BOOL found = FALSE;

	do // First node guaranteed to be there
	{
		const S32 count = nodep->mCount;
		S32 i;
		for (i = 0; i < count; i++)
		{
			if (nodep->mKey[i] == local_id)
			{
				// Found the item.  Start shifting items from later
				// in the list over this item.
				found = TRUE;
			}

			if (found)
			{
				// If there is an iterator on this node, we need to 
				// back it up.
				S32 cur_iter;
				for (cur_iter = 0; cur_iter <MAX_ITERS; cur_iter++)
				{
					LLLocalIDHashMapIter<DATA_TYPE, SIZE>* iter;
					iter = mIters[cur_iter];
					// If an iterator is on this node,i pair, then back it up.
					if (iter
						&& iter->mCurHashMapNodeNum == node_index
						&& iter->mCurHashNodep == nodep
						&& iter->mCurHashNodeKey == i)
					{
						if (i > 0)
						{
							// Don't need to move iterator nodep, since 
							// we're in the same node.
							iter->mCurHashNodeKey--;
						}
						else if (prevp)
						{
							// need to go the previous node, last item
							iter->mCurHashNodep = prevp;
							iter->mCurHashNodeKey = prevp->mCount - 1;
						}
						else
						{
							// we're on the first item in the list, but
							// need to go back anyhow.

							// BUG: If this deletion empties the list, 
							// iter->done() will be wrong until
							// iter->next() is called.
							iter->mCurHashNodeKey = -1;
						}
					}
				}

				// Copy data from the next position into this position.
				if (i < count-1)
				{
					// we're not on the last item in the node,
					// so we can copy within the node
					nodep->mKey[i] = nodep->mKey[i+1];
					nodep->mData[i] = nodep->mData[i+1];
				}
				else if (nodep->mNextNodep)
				{
					// we're on the last item in the node,
					// but there's a next node we can copy from
					nodep->mKey[i] = nodep->mNextNodep->mKey[0];
					nodep->mData[i] = nodep->mNextNodep->mData[0];
				}
				else
				{
					// We're on the last position in the list.
					// No one to copy from.  Replace with nothing.
					nodep->mKey[i] = 0;
					nodep->mData[i] = mNull;
				}
			}
		}

		// Last node in chain, so delete the last node
		if (found
			&& !nodep->mNextNodep)
		{
			// delete the last item off the last node
			nodep->mCount--;

			if (nodep->mCount == 0)
			{
				// We deleted the last element!
				if (nodep != &mNodes[node_index])
				{
					// Always have a prevp if we're not the head.
					llassert(prevp);

					// Only blitz the node if it's not the head
					// Set the previous node to point to NULL, then
					// blitz the empty last node
					prevp->mNextNodep = NULL;
					delete nodep;
					nodep = NULL;
				}
			}

			// Deleted last item in chain, so we're done.
			return found;
		}

		prevp = nodep;
		nodep = nodep->mNextNodep;
	} while (nodep);

	return found;
}

template <class DATA_TYPE, int SIZE>
void LLLocalIDHashMap<DATA_TYPE, SIZE>::removeIter(LLLocalIDHashMapIter<DATA_TYPE, SIZE> *iter)
{
	S32 i;
	for (i = 0; i < MAX_ITERS; i++)
	{
		if (mIters[i] == iter)
		{
			mIters[i] = NULL;
			mIterCount--;
			return;
		}
	}
	llerrs << "Iterator " << iter << " not found for removal in hash map!" << llendl;
}

template <class DATA_TYPE, int SIZE>
void LLLocalIDHashMap<DATA_TYPE, SIZE>::addIter(LLLocalIDHashMapIter<DATA_TYPE, SIZE> *iter)
{
	S32 i;
	for (i = 0; i < MAX_ITERS; i++)
	{
		if (mIters[i] == NULL)
		{
			mIters[i] = iter;
			mIterCount++;
			return;
		}
	}
	llerrs << "More than " << MAX_ITERS << " iterating over a map simultaneously!" << llendl;
}



//
// LLLocalIDHashMapIter Implementation
//
template <class DATA_TYPE, int SIZE>
LLLocalIDHashMapIter<DATA_TYPE, SIZE>::LLLocalIDHashMapIter(LLLocalIDHashMap<DATA_TYPE, SIZE> *hash_mapp)
{
	mHashMapp = NULL;
	setMap(hash_mapp);
}

template <class DATA_TYPE, int SIZE>
LLLocalIDHashMapIter<DATA_TYPE, SIZE>::~LLLocalIDHashMapIter()
{
	if (mHashMapp)
	{
		mHashMapp->removeIter(this);
	}
}

template <class DATA_TYPE, int SIZE>
void LLLocalIDHashMapIter<DATA_TYPE, SIZE>::setMap(LLLocalIDHashMap<DATA_TYPE, SIZE> *hash_mapp)
{
	if (mHashMapp)
	{
		mHashMapp->removeIter(this);
	}
	mHashMapp = hash_mapp;
	if (mHashMapp)
	{
		mHashMapp->addIter(this);
	}

	mCurHashNodep = NULL;
	mCurHashMapNodeNum = -1;
	mCurHashNodeKey = 0;
}

template <class DATA_TYPE, int SIZE>
inline void LLLocalIDHashMapIter<DATA_TYPE, SIZE>::first()
{
	// Iterate through until we find the first non-empty node;
	S32 i;
	for (i = 0; i < 256; i++)
	{
		if (mHashMapp->mNodes[i].mCount)
		{

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
inline BOOL LLLocalIDHashMapIter<DATA_TYPE, SIZE>::done() const
{
	return mCurHashNodep ? FALSE : TRUE;
}

template <class DATA_TYPE, int SIZE>
inline S32 LLLocalIDHashMapIter<DATA_TYPE, SIZE>::currentBin() const
{
	if (  (mCurHashMapNodeNum > 255)
		||(mCurHashMapNodeNum < 0))
	{
		return 0;
	}
	else
	{
		return mCurHashMapNodeNum;
	}
}

template <class DATA_TYPE, int SIZE>
inline void LLLocalIDHashMapIter<DATA_TYPE, SIZE>::setBin(S32 bin)
{
	// Iterate through until we find the first non-empty node;
	S32 i;
	bin = llclamp(bin, 0, 255);
	for (i = bin; i < 256; i++)
	{
		if (mHashMapp->mNodes[i].mCount)
		{

			mCurHashNodep = &mHashMapp->mNodes[i];
			mCurHashMapNodeNum = i;
			mCurHashNodeKey = 0;
			return;
		}
	}
	for (i = 0; i < bin; i++)
	{
		if (mHashMapp->mNodes[i].mCount)
		{

			mCurHashNodep = &mHashMapp->mNodes[i];
			mCurHashMapNodeNum = i;
			mCurHashNodeKey = 0;
			return;
		}
	}
	// Completely empty!
	mCurHashNodep = NULL;
}

template <class DATA_TYPE, int SIZE>
inline DATA_TYPE &LLLocalIDHashMapIter<DATA_TYPE, SIZE>::current()
{
	if (!mCurHashNodep)
	{
		return mNull;
	}
	return mCurHashNodep->mData[mCurHashNodeKey];
}

template <class DATA_TYPE, int SIZE>
inline void LLLocalIDHashMapIter<DATA_TYPE, SIZE>::next()
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
	return;
}

#endif // LL_LLLOCALIDHASHMAP_H
