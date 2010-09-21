/** 
 * @file linked_lists.h
 * @brief LLLinkedList class header amd implementation file.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LINKED_LISTS_H
#define LL_LINKED_LISTS_H

/** 
 * Provides a standard doubly linked list for fun and profit
 * Utilizes a neat trick off of Flipcode where the back pointer is a
 * pointer to a pointer, allowing easier transfer of nodes between lists, &c
 * And a template class, of course
 */

#include "llerror.h"


template <class DATA_TYPE> class LLLinkedList
{
public:
	friend class LLLinkNode;
	// External interface

	// basic constructor
	LLLinkedList() : mHead(NULL), mCurrentp(NULL), mInsertBefore(NULL)
	{
		mCurrentp = mHead.mNextp;
		mCurrentOperatingp = mHead.mNextp;
		mCount = 0;
	}

	// basic constructor
	LLLinkedList(BOOL	(*insert_before)(DATA_TYPE *data_new, DATA_TYPE *data_tested)) : mHead(NULL), mCurrentp(NULL), mInsertBefore(insert_before)
	{
		mCurrentp = mHead.mNextp;
		mCurrentOperatingp = mHead.mNextp;
		mCount = 0;
	}

	// destructor destroys list and nodes, but not data in nodes
	~LLLinkedList()
	{
		removeAllNodes();
	}

	// set mInsertBefore
	void setInsertBefore(BOOL (*insert_before)(DATA_TYPE *data_new, DATA_TYPE *data_tested))
	{
		mInsertBefore = insert_before;
	}

	//
	// WARNING!!!!!!!
	// addData and addDataSorted are NOT O(1) operations, but O(n) because they check
	// for existence of the data in the linked list first.  Why, I don't know - djs
	// If you don't care about dupes, use addDataNoCheck
	//

	// put data into a node and stick it at the front of the list
	inline BOOL addData(DATA_TYPE *data);

	// put data into a node and sort into list by mInsertBefore()
	// calls normal add if mInsertBefore isn't set
	inline BOOL addDataSorted(DATA_TYPE *data);

	inline BOOL addDataNoCheck(DATA_TYPE *data);

	// bubbleSortList
	// does an improved bubble sort of the list . . . works best with almost sorted data
	// does nothing if mInsertBefore isn't set
	// Nota Bene: Swaps are accomplished by swapping data pointers
	inline void bubbleSortList();

	// put data into a node and stick it at the end of the list
	inline BOOL addDataAtEnd(DATA_TYPE *data);

	// returns number of items in the list
	inline S32 getLength() const;

	inline BOOL isEmpty();

	// search the list starting at mHead.mNextp and remove the link with mDatap == data
	// leave mCurrentp and mCurrentOperatingp on the next entry
	// return TRUE if found, FALSE if not found
	inline BOOL removeData(DATA_TYPE *data);

		// search the list starting at mHead.mNextp and delete the link with mDatap == data
	// leave mCurrentp and mCurrentOperatingp on the next entry
	// return TRUE if found, FALSE if not found
	inline BOOL deleteData(DATA_TYPE *data);

	// remove all nodes from the list and delete the associated data
	inline void deleteAllData();

	// remove all nodes from the list but do not delete data
	inline void removeAllNodes();

	// check to see if data is in list
	// if TRUE then mCurrentp and mCurrentOperatingp point to data
	inline BOOL	checkData(DATA_TYPE *data);

	// place mCurrentp on first node
	inline void resetList();

	// return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
	inline DATA_TYPE	*getCurrentData();

	// same as getCurrentData() but a more intuitive name for the operation
	inline DATA_TYPE	*getNextData();

	// reset the list and return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
	inline DATA_TYPE	*getFirstData();

	// reset the list and return the data at position n, set mCurentOperatingp to that node and bump mCurrentp
	// Note: n is zero-based
	inline DATA_TYPE	*getNthData( U32 n);

	// reset the list and return the last data in it, set mCurentOperatingp to that node and bump mCurrentp
	inline DATA_TYPE	*getLastData();

	// remove the Node at mCurentOperatingp
	// leave mCurrentp and mCurentOperatingp on the next entry
	inline void removeCurrentData();

	// remove the Node at mCurentOperatingp and add it to newlist
	// leave mCurrentp and mCurentOperatingp on the next entry
	void moveCurrentData(LLLinkedList *newlist, BOOL b_sort);

	BOOL moveData(DATA_TYPE *data, LLLinkedList *newlist, BOOL b_sort);

	// delete the Node at mCurentOperatingp
	// leave mCurrentp anf mCurentOperatingp on the next entry
	void deleteCurrentData();

private:
	// node that actually contains the data
	class LLLinkNode
	{
	public:
		// assign the mDatap pointer
		LLLinkNode(DATA_TYPE *data) : mDatap(data), mNextp(NULL), mPrevpp(NULL)
		{
		}

		// destructor does not, by default, destroy associated data
		// however, the mDatap must be NULL to ensure that we aren't causing memory leaks
		~LLLinkNode()
		{
			if (mDatap)
			{
				llerror("Attempting to call LLLinkNode destructor with a non-null mDatap!", 1);
			}
		}

		// delete associated data and NULL out pointer
		void deleteData()
		{
			delete mDatap;
			mDatap = NULL;
		}

		// NULL out pointer
		void removeData()
		{
			mDatap = NULL;
		}

		DATA_TYPE	*mDatap;
		LLLinkNode	*mNextp;
		LLLinkNode	**mPrevpp;
	};

	// add a node at the front of the list
	void addData(LLLinkNode *node)
	{
		// don't allow NULL to be passed to addData
		if (!node)
		{
			llerror("NULL pointer passed to LLLinkedList::addData", 0);
		}

		// add the node to the front of the list
		node->mPrevpp = &mHead.mNextp;
		node->mNextp = mHead.mNextp;

		// if there's something in the list, fix its back pointer
		if (node->mNextp)
		{
			node->mNextp->mPrevpp = &node->mNextp;
		}

		mHead.mNextp = node;
	}

	LLLinkNode			mHead;															// fake head node. . . makes pointer operations faster and easier
	LLLinkNode			*mCurrentp;														// mCurrentp is the Node that getCurrentData returns
	LLLinkNode			*mCurrentOperatingp;											// this is the node that the various mumbleCurrentData functions act on
	BOOL				(*mInsertBefore)(DATA_TYPE *data_new, DATA_TYPE *data_tested);	// user function set to allow sorted lists
	U32					mCount;
};

template <class DATA_TYPE>
BOOL LLLinkedList<DATA_TYPE>::addData(DATA_TYPE *data)
{
	// don't allow NULL to be passed to addData
	if (!data)
	{
		llerror("NULL pointer passed to LLLinkedList::addData", 0);
	}

	LLLinkNode *tcurr = mCurrentp;
	LLLinkNode *tcurrop = mCurrentOperatingp;

	if ( checkData(data))
	{
		mCurrentp = tcurr;
		mCurrentOperatingp = tcurrop;
		return FALSE;
	}

	// make the new node
	LLLinkNode *temp = new LLLinkNode(data);

	// add the node to the front of the list
	temp->mPrevpp = &mHead.mNextp;
	temp->mNextp = mHead.mNextp;

	// if there's something in the list, fix its back pointer
	if (temp->mNextp)
	{
		temp->mNextp->mPrevpp = &temp->mNextp;
	}

	mHead.mNextp = temp;
	mCurrentp = tcurr;
	mCurrentOperatingp = tcurrop;
	mCount++;
	return TRUE;
}


template <class DATA_TYPE>
BOOL LLLinkedList<DATA_TYPE>::addDataNoCheck(DATA_TYPE *data)
{
	// don't allow NULL to be passed to addData
	if (!data)
	{
		llerror("NULL pointer passed to LLLinkedList::addData", 0);
	}

	LLLinkNode *tcurr = mCurrentp;
	LLLinkNode *tcurrop = mCurrentOperatingp;

	// make the new node
	LLLinkNode *temp = new LLLinkNode(data);

	// add the node to the front of the list
	temp->mPrevpp = &mHead.mNextp;
	temp->mNextp = mHead.mNextp;

	// if there's something in the list, fix its back pointer
	if (temp->mNextp)
	{
		temp->mNextp->mPrevpp = &temp->mNextp;
	}

	mHead.mNextp = temp;
	mCurrentp = tcurr;
	mCurrentOperatingp = tcurrop;
	mCount++;
	return TRUE;
}


template <class DATA_TYPE>
BOOL LLLinkedList<DATA_TYPE>::addDataSorted(DATA_TYPE *data)
{
	LLLinkNode *tcurr = mCurrentp;
	LLLinkNode *tcurrop = mCurrentOperatingp;
	// don't allow NULL to be passed to addData
	if (!data)
	{
		llerror("NULL pointer passed to LLLinkedList::addDataSorted", 0);
	}

	if (checkData(data))
	{
		// restore
		mCurrentp = tcurr;
		mCurrentOperatingp = tcurrop;
		return FALSE;
	}

	// mInsertBefore not set?
	if (!mInsertBefore)
	{
		addData(data);
		// restore
		mCurrentp = tcurr;
		mCurrentOperatingp = tcurrop;
		return FALSE;
	}

	// empty list?
	if (!mHead.mNextp)
	{
		addData(data);
		// restore
		mCurrentp = tcurr;
		mCurrentOperatingp = tcurrop;
		return TRUE;
	}

	// make the new node
	LLLinkNode *temp = new LLLinkNode(data);

	// walk the list until mInsertBefore returns true 
	mCurrentp = mHead.mNextp;
	while (mCurrentp->mNextp)
	{
		if (mInsertBefore(data, mCurrentp->mDatap))
		{
			// insert before the current one
			temp->mPrevpp = mCurrentp->mPrevpp;
			temp->mNextp = mCurrentp;
			*(temp->mPrevpp) = temp;
			mCurrentp->mPrevpp = &temp->mNextp;
			// restore
			mCurrentp = tcurr;
			mCurrentOperatingp = tcurrop;
			mCount++;
			return TRUE;
		}
		else
		{
			mCurrentp = mCurrentp->mNextp;
		}
	}

	// on the last element, add before?
	if (mInsertBefore(data, mCurrentp->mDatap))
	{
		// insert before the current one
		temp->mPrevpp = mCurrentp->mPrevpp;
		temp->mNextp = mCurrentp;
		*(temp->mPrevpp) = temp;
		mCurrentp->mPrevpp = &temp->mNextp;
		// restore
		mCurrentp = tcurr;
		mCurrentOperatingp = tcurrop;
	}
	else // insert after
	{
		temp->mPrevpp = &mCurrentp->mNextp;
		temp->mNextp = NULL;
		mCurrentp->mNextp = temp;

		// restore
		mCurrentp = tcurr;
		mCurrentOperatingp = tcurrop;
	}
	mCount++;
	return TRUE;
}

template <class DATA_TYPE>
void LLLinkedList<DATA_TYPE>::bubbleSortList()
{
	// mInsertBefore not set
	if (!mInsertBefore)
	{
		return;
	}

	LLLinkNode *tcurr = mCurrentp;
	LLLinkNode *tcurrop = mCurrentOperatingp;

	BOOL		b_swapped = FALSE;
	DATA_TYPE	*temp;

	// Nota Bene: This will break if more than 0x7FFFFFFF members in list!
	S32			length = 0x7FFFFFFF;
	S32			count = 0;
	do
	{
		b_swapped = FALSE;
		mCurrentp = mHead.mNextp;
		count = 0;
		while (  (count + 1 < length)
			   &&(mCurrentp))
		{
			if (mCurrentp->mNextp)
			{
				if (!mInsertBefore(mCurrentp->mDatap, mCurrentp->mNextp->mDatap))
				{
					// swap data pointers!
					temp = mCurrentp->mDatap;
					mCurrentp->mDatap = mCurrentp->mNextp->mDatap;
					mCurrentp->mNextp->mDatap = temp;
					b_swapped = TRUE;
				}
			}
			else
			{
				break;
			}
			count++;
			mCurrentp = mCurrentp->mNextp;
		}
		length = count;
	} while (b_swapped);

	// restore
	mCurrentp = tcurr;
	mCurrentOperatingp = tcurrop;
}


template <class DATA_TYPE>
BOOL LLLinkedList<DATA_TYPE>::addDataAtEnd(DATA_TYPE *data)
{
	LLLinkNode *tcurr = mCurrentp;
	LLLinkNode *tcurrop = mCurrentOperatingp;

	// don't allow NULL to be passed to addData
	if (!data)
	{
		llerror("NULL pointer passed to LLLinkedList::addData", 0);
	}

	if (checkData(data))
	{
		mCurrentp = tcurr;
		mCurrentOperatingp = tcurrop;
		return FALSE;
	}

	// make the new node
	LLLinkNode *temp = new LLLinkNode(data);

	// add the node to the end of the list

	// if empty, add to the front and be done with it
	if (!mHead.mNextp)
	{
		temp->mPrevpp = &mHead.mNextp;
		temp->mNextp = NULL;
		mHead.mNextp = temp;
	}
	else
	{
		// otherwise, walk to the end of the list
		mCurrentp = mHead.mNextp;
		while (mCurrentp->mNextp)
		{
			mCurrentp = mCurrentp->mNextp;
		}
		temp->mPrevpp = &mCurrentp->mNextp;
		temp->mNextp = NULL;
		mCurrentp->mNextp = temp;
	}

	// restore
	mCurrentp = tcurr;
	mCurrentOperatingp = tcurrop;
	mCount++;
	return TRUE;
}


// returns number of items in the list
template <class DATA_TYPE>
S32 LLLinkedList<DATA_TYPE>::getLength() const
{
//	S32	length = 0;
//	for (LLLinkNode* temp = mHead.mNextp; temp != NULL; temp = temp->mNextp)
//	{
//		length++;
//	}
	return mCount;
}


template <class DATA_TYPE>
BOOL LLLinkedList<DATA_TYPE>::isEmpty()
{
	return (mCount == 0);
}


// search the list starting at mHead.mNextp and remove the link with mDatap == data
// leave mCurrentp and mCurrentOperatingp on the next entry
// return TRUE if found, FALSE if not found
template <class DATA_TYPE>
BOOL LLLinkedList<DATA_TYPE>::removeData(DATA_TYPE *data)
{
	BOOL b_found = FALSE;
	// don't allow NULL to be passed to addData
	if (!data)
	{
		llerror("NULL pointer passed to LLLinkedList::removeData", 0);
	}

	LLLinkNode *tcurr = mCurrentp;
	LLLinkNode *tcurrop = mCurrentOperatingp;

	mCurrentp = mHead.mNextp;
	mCurrentOperatingp = mHead.mNextp;

	while (mCurrentOperatingp)
	{
		if (mCurrentOperatingp->mDatap == data)
		{
			b_found = TRUE;

			// remove the node

			// if there is a next one, fix it
			if (mCurrentOperatingp->mNextp)
			{
				mCurrentOperatingp->mNextp->mPrevpp = mCurrentOperatingp->mPrevpp;
			}
			*(mCurrentOperatingp->mPrevpp) = mCurrentOperatingp->mNextp;

			// remove the LLLinkNode

			// if we were on the one we want to delete, bump the cached copies
			if (mCurrentOperatingp == tcurrop)
			{
				tcurrop = tcurr = mCurrentOperatingp->mNextp;
			}
			else if (mCurrentOperatingp == tcurr)
			{
				tcurrop = tcurr = mCurrentOperatingp->mNextp;
			}

			mCurrentp = mCurrentOperatingp->mNextp;

			mCurrentOperatingp->removeData();
			delete mCurrentOperatingp;
			mCurrentOperatingp = mCurrentp;
			mCount--;
			break;
		}
		mCurrentOperatingp = mCurrentOperatingp->mNextp;
	}
	// restore
	mCurrentp = tcurr;
	mCurrentOperatingp = tcurrop;
	return b_found;
}

// search the list starting at mHead.mNextp and delete the link with mDatap == data
// leave mCurrentp and mCurrentOperatingp on the next entry
// return TRUE if found, FALSE if not found
template <class DATA_TYPE>
BOOL LLLinkedList<DATA_TYPE>::deleteData(DATA_TYPE *data)
{
	BOOL b_found = FALSE;
	// don't allow NULL to be passed to addData
	if (!data)
	{
		llerror("NULL pointer passed to LLLinkedList::removeData", 0);
	}

	LLLinkNode *tcurr = mCurrentp;
	LLLinkNode *tcurrop = mCurrentOperatingp;

	mCurrentp = mHead.mNextp;
	mCurrentOperatingp = mHead.mNextp;

	while (mCurrentOperatingp)
	{
		if (mCurrentOperatingp->mDatap == data)
		{
			b_found = TRUE;

			// remove the node
			// if there is a next one, fix it
			if (mCurrentOperatingp->mNextp)
			{
				mCurrentOperatingp->mNextp->mPrevpp = mCurrentOperatingp->mPrevpp;
			}
			*(mCurrentOperatingp->mPrevpp) = mCurrentOperatingp->mNextp;

			// delete the LLLinkNode
			// if we were on the one we want to delete, bump the cached copies
			if (mCurrentOperatingp == tcurrop)
			{
				tcurrop = tcurr = mCurrentOperatingp->mNextp;
			}

			// and delete the associated data
			llassert(mCurrentOperatingp);
			mCurrentp = mCurrentOperatingp->mNextp;
			mCurrentOperatingp->deleteData();
			delete mCurrentOperatingp;
			mCurrentOperatingp = mCurrentp;
			mCount--;
			break;
		}
		mCurrentOperatingp = mCurrentOperatingp->mNextp;
	}
	// restore
	mCurrentp = tcurr;
	mCurrentOperatingp = tcurrop;
	return b_found;
}

	// remove all nodes from the list and delete the associated data
template <class DATA_TYPE>
void LLLinkedList<DATA_TYPE>::deleteAllData()
{
	LLLinkNode *temp;
	// reset mCurrentp
	mCurrentp = mHead.mNextp;

	while (mCurrentp)
	{
		temp = mCurrentp->mNextp;
		mCurrentp->deleteData();
		delete mCurrentp;
		mCurrentp = temp;
	}

	// reset mHead and mCurrentp
	mHead.mNextp = NULL;
	mCurrentp = mHead.mNextp;
	mCurrentOperatingp = mHead.mNextp;
	mCount = 0;
}

// remove all nodes from the list but do not delete data
template <class DATA_TYPE>
void LLLinkedList<DATA_TYPE>::removeAllNodes()
{
	LLLinkNode *temp;
	// reset mCurrentp
	mCurrentp = mHead.mNextp;

	while (mCurrentp)
	{
		temp = mCurrentp->mNextp;
		mCurrentp->removeData();
		delete mCurrentp;
		mCurrentp = temp;
	}

	// reset mHead and mCurrentp
	mHead.mNextp = NULL;
	mCurrentp = mHead.mNextp;
	mCurrentOperatingp = mHead.mNextp;
	mCount = 0;
}

// check to see if data is in list
// if TRUE then mCurrentp and mCurrentOperatingp point to data
template <class DATA_TYPE>
BOOL LLLinkedList<DATA_TYPE>::checkData(DATA_TYPE *data)
{
	// reset mCurrentp
	mCurrentp = mHead.mNextp;

	while (mCurrentp)
	{
		if (mCurrentp->mDatap == data)
		{
			mCurrentOperatingp = mCurrentp;
			return TRUE;
		}
		mCurrentp = mCurrentp->mNextp;
	}
	mCurrentOperatingp = mCurrentp;
	return FALSE;
}

// place mCurrentp on first node
template <class DATA_TYPE>
void LLLinkedList<DATA_TYPE>::resetList()
{
	mCurrentp = mHead.mNextp;
	mCurrentOperatingp = mHead.mNextp;
}

// return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
template <class DATA_TYPE>
DATA_TYPE *LLLinkedList<DATA_TYPE>::getCurrentData()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mNextp;
		return mCurrentOperatingp->mDatap;
	}
	else
	{
		return NULL;
	}
}

// same as getCurrentData() but a more intuitive name for the operation
template <class DATA_TYPE>
DATA_TYPE *LLLinkedList<DATA_TYPE>::getNextData()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mNextp;
		return mCurrentOperatingp->mDatap;
	}
	else
	{
		return NULL;
	}
}

// reset the list and return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
template <class DATA_TYPE>
DATA_TYPE *LLLinkedList<DATA_TYPE>::getFirstData()
{
	mCurrentp = mHead.mNextp;
	mCurrentOperatingp = mHead.mNextp;
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mNextp;
		return mCurrentOperatingp->mDatap;
	}
	else
	{
		return NULL;
	}
}

// Note: n is zero-based
template <class DATA_TYPE>
DATA_TYPE	*LLLinkedList<DATA_TYPE>::getNthData( U32 n )
{
	mCurrentOperatingp = mHead.mNextp;

	// if empty, return NULL
	if (!mCurrentOperatingp)
	{
		return NULL;
	}

	for( U32 i = 0; i < n; i++ )
	{
		mCurrentOperatingp = mCurrentOperatingp->mNextp;
		if( !mCurrentOperatingp )
		{
			return NULL;
		}
	}

	mCurrentp = mCurrentOperatingp->mNextp;
	return mCurrentOperatingp->mDatap;
}


// reset the list and return the last data in it, set mCurentOperatingp to that node and bump mCurrentp
template <class DATA_TYPE>
DATA_TYPE *LLLinkedList<DATA_TYPE>::getLastData()
{
	mCurrentOperatingp = mHead.mNextp;

	// if empty, return NULL
	if (!mCurrentOperatingp)
		return NULL;

	// walk until we're pointing at the last entry
	while (mCurrentOperatingp->mNextp)
	{
		mCurrentOperatingp = mCurrentOperatingp->mNextp;
	}
	mCurrentp = mCurrentOperatingp->mNextp;
	return mCurrentOperatingp->mDatap;
}

// remove the Node at mCurentOperatingp
// leave mCurrentp and mCurentOperatingp on the next entry
// return TRUE if found, FALSE if not found
template <class DATA_TYPE>
void LLLinkedList<DATA_TYPE>::removeCurrentData()
{
	if (mCurrentOperatingp)
	{
		// remove the node
		// if there is a next one, fix it
		if (mCurrentOperatingp->mNextp)
		{
			mCurrentOperatingp->mNextp->mPrevpp = mCurrentOperatingp->mPrevpp;
		}
		*(mCurrentOperatingp->mPrevpp) = mCurrentOperatingp->mNextp;

		// remove the LLLinkNode
		mCurrentp = mCurrentOperatingp->mNextp;

		mCurrentOperatingp->removeData();
		delete mCurrentOperatingp;
		mCount--;
		mCurrentOperatingp = mCurrentp;
	}
}

// remove the Node at mCurentOperatingp and add it to newlist
// leave mCurrentp and mCurentOperatingp on the next entry
// return TRUE if found, FALSE if not found
template <class DATA_TYPE>
void LLLinkedList<DATA_TYPE>::moveCurrentData(LLLinkedList *newlist, BOOL b_sort)
{
	if (mCurrentOperatingp)
	{
		// remove the node
		// if there is a next one, fix it
		if (mCurrentOperatingp->mNextp)
		{
			mCurrentOperatingp->mNextp->mPrevpp = mCurrentOperatingp->mPrevpp;
		}
		*(mCurrentOperatingp->mPrevpp) = mCurrentOperatingp->mNextp;

		// remove the LLLinkNode
		mCurrentp = mCurrentOperatingp->mNextp;
		// move the node to the new list
		newlist->addData(mCurrentOperatingp);
		if (b_sort)
			bubbleSortList();
		mCurrentOperatingp = mCurrentp;
	}
}

template <class DATA_TYPE>
BOOL LLLinkedList<DATA_TYPE>::moveData(DATA_TYPE *data, LLLinkedList *newlist, BOOL b_sort)
{
	BOOL b_found = FALSE;
	// don't allow NULL to be passed to addData
	if (!data)
	{
		llerror("NULL pointer passed to LLLinkedList::removeData", 0);
	}

	LLLinkNode *tcurr = mCurrentp;
	LLLinkNode *tcurrop = mCurrentOperatingp;

	mCurrentp = mHead.mNextp;
	mCurrentOperatingp = mHead.mNextp;

	while (mCurrentOperatingp)
	{
		if (mCurrentOperatingp->mDatap == data)
		{
			b_found = TRUE;

			// remove the node

			// if there is a next one, fix it
			if (mCurrentOperatingp->mNextp)
			{
				mCurrentOperatingp->mNextp->mPrevpp = mCurrentOperatingp->mPrevpp;
			}
			*(mCurrentOperatingp->mPrevpp) = mCurrentOperatingp->mNextp;

			// if we were on the one we want to delete, bump the cached copies
			if (  (mCurrentOperatingp == tcurrop)
				||(mCurrentOperatingp == tcurr))
			{
				tcurrop = tcurr = mCurrentOperatingp->mNextp;
			}

			// remove the LLLinkNode
			mCurrentp = mCurrentOperatingp->mNextp;
			// move the node to the new list
			newlist->addData(mCurrentOperatingp);
			if (b_sort)
				newlist->bubbleSortList();
			mCurrentOperatingp = mCurrentp;
			break;
		}
		mCurrentOperatingp = mCurrentOperatingp->mNextp;
	}
	// restore
	mCurrentp = tcurr;
	mCurrentOperatingp = tcurrop;
	return b_found;
}

// delete the Node at mCurentOperatingp
// leave mCurrentp anf mCurentOperatingp on the next entry
// return TRUE if found, FALSE if not found
template <class DATA_TYPE>
void LLLinkedList<DATA_TYPE>::deleteCurrentData()
{
	if (mCurrentOperatingp)
	{
		// remove the node
		// if there is a next one, fix it
		if (mCurrentOperatingp->mNextp)
		{
			mCurrentOperatingp->mNextp->mPrevpp = mCurrentOperatingp->mPrevpp;
		}
		*(mCurrentOperatingp->mPrevpp) = mCurrentOperatingp->mNextp;

		// remove the LLLinkNode
		mCurrentp = mCurrentOperatingp->mNextp;

		mCurrentOperatingp->deleteData();
		if (mCurrentOperatingp->mDatap)
			llerror("This is impossible!", 0);
		delete mCurrentOperatingp;
		mCurrentOperatingp = mCurrentp;
		mCount--;
	}
}

#endif
