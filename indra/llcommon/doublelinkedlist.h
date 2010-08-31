/** 
 * @file doublelinkedlist.h
 * @brief Provides a standard doubly linked list for fun and profit.
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

#ifndef LL_DOUBLELINKEDLIST_H
#define LL_DOUBLELINKEDLIST_H

#include "llerror.h"
#include "llrand.h"

// node that actually contains the data
template <class DATA_TYPE> class LLDoubleLinkedNode
{
public:
	DATA_TYPE			*mDatap;
	LLDoubleLinkedNode	*mNextp;
	LLDoubleLinkedNode	*mPrevp;


public:
	// assign the mDatap pointer
	LLDoubleLinkedNode(DATA_TYPE *data);

	// destructor does not, by default, destroy associated data
	// however, the mDatap must be NULL to ensure that we aren't causing memory leaks
	~LLDoubleLinkedNode();

	// delete associated data and NULL out pointer
	void deleteData();

	// remove associated data and NULL out pointer
	void removeData();
};


const U32 LLDOUBLE_LINKED_LIST_STATE_STACK_DEPTH = 4;

template <class DATA_TYPE> class LLDoubleLinkedList
{
private:
	LLDoubleLinkedNode<DATA_TYPE> mHead;		// head node
	LLDoubleLinkedNode<DATA_TYPE> mTail;		// tail node
	LLDoubleLinkedNode<DATA_TYPE> *mQueuep;		// The node in the batter's box
	LLDoubleLinkedNode<DATA_TYPE> *mCurrentp;	// The node we're talking about

	// The state stack allows nested exploration of the LLDoubleLinkedList
	// but should be used with great care
	LLDoubleLinkedNode<DATA_TYPE> *mQueuepStack[LLDOUBLE_LINKED_LIST_STATE_STACK_DEPTH];
	LLDoubleLinkedNode<DATA_TYPE> *mCurrentpStack[LLDOUBLE_LINKED_LIST_STATE_STACK_DEPTH];
	U32 mStateStackDepth;
	U32	mCount;

	// mInsertBefore is a pointer to a user-set function that returns 
	// TRUE if "first" should be located before "second"
	// NOTE: mInsertBefore() should never return TRUE when ("first" == "second")
	// or never-ending loops can occur
	BOOL				(*mInsertBefore)(DATA_TYPE *first, DATA_TYPE *second);	
																				
public:
	LLDoubleLinkedList();

	// destructor destroys list and nodes, but not data in nodes
	~LLDoubleLinkedList();

	// put data into a node and stick it at the front of the list
	// set mCurrentp to mQueuep
	void addData(DATA_TYPE *data);

	// put data into a node and stick it at the end of the list
	// set mCurrentp to mQueuep
	void addDataAtEnd(DATA_TYPE *data);

	S32 getLength() const;
	// search the list starting at mHead.mNextp and remove the link with mDatap == data
	// set mCurrentp to mQueuep
	// return TRUE if found, FALSE if not found
	BOOL removeData(const DATA_TYPE *data);

	// search the list starting at mHead.mNextp and delete the link with mDatap == data
	// set mCurrentp to mQueuep
	// return TRUE if found, FALSE if not found
	BOOL deleteData(DATA_TYPE *data);

	// remove all nodes from the list and delete the associated data
	void deleteAllData();

	// remove all nodes from the list but do not delete data
	void removeAllNodes();

	BOOL isEmpty();

	// check to see if data is in list
	// set mCurrentp and mQueuep to the target of search if found, otherwise set mCurrentp to mQueuep
	// return TRUE if found, FALSE if not found
	BOOL	checkData(const DATA_TYPE *data);

	// NOTE: This next two funtions are only included here 
	// for those too familiar with the LLLinkedList template class.
	// They are depreciated.  resetList() is unecessary while 
	// getCurrentData() is identical to getNextData() and has
	// a misleading name.
	//
	// The recommended way to loop through a list is as follows:
	//
	// datap = list.getFirstData();
	// while (datap)
	// {
	//     /* do stuff */
	//     datap = list.getNextData();
	// }

		// place mQueuep on mHead node
		void resetList();
	
		// return the data currently pointed to, 
		// set mCurrentp to that node and bump mQueuep down the list
		// NOTE: this function is identical to getNextData()
		DATA_TYPE *getCurrentData();


	// reset the list and return the data currently pointed to, 
	// set mCurrentp to that node and bump mQueuep down the list
	DATA_TYPE *getFirstData();


	// reset the list and return the data at position n, set mCurentp 
	// to that node and bump mQueuep down the list
	// Note: n=0 will behave like getFirstData()
	DATA_TYPE *getNthData(U32 n);

	// reset the list and return the last data in it, 
	// set mCurrentp to that node and bump mQueuep up the list
	DATA_TYPE *getLastData();

	// return data in mQueuep,
	// set mCurrentp mQueuep and bump mQueuep down the list
	DATA_TYPE *getNextData();

	// return the data in mQueuep, 
	// set mCurrentp to mQueuep and bump mQueuep up the list
	DATA_TYPE *getPreviousData();

	// remove the Node at mCurrentp
	// set mCurrentp to mQueuep
	void removeCurrentData();

	// delete the Node at mCurrentp
	// set mCurrentp to mQueuep
	void deleteCurrentData();

	// remove the Node at mCurrentp and insert it into newlist
	// set mCurrentp to mQueuep
	void moveCurrentData(LLDoubleLinkedList<DATA_TYPE> *newlist);

	// insert the node in front of mCurrentp
	// set mCurrentp to mQueuep
	void insertNode(LLDoubleLinkedNode<DATA_TYPE> *node);

	// insert the data in front of mCurrentp
	// set mCurrentp to mQueuep
	void insertData(DATA_TYPE *data);

	// if mCurrentp has a previous node then :
	//   * swaps mCurrentp with its previous
	//   * set mCurrentp to mQueuep
	//     (convenient for forward bubble-sort)
	// otherwise does nothing
	void swapCurrentWithPrevious();

	// if mCurrentp has a next node then :
	//   * swaps mCurrentp with its next
	//   * set mCurrentp to mQueuep
	//     (convenient for backwards bubble-sort)
	// otherwise does nothing
	void swapCurrentWithNext();

	// move mCurrentp to the front of the list
	// set mCurrentp to mQueuep
	void moveCurrentToFront();
	
	// move mCurrentp to the end of the list
	// set mCurrentp to mQueuep
	void moveCurrentToEnd();

	// set mInsertBefore
	void setInsertBefore(BOOL (*insert_before)(DATA_TYPE *first, DATA_TYPE *second));

	// add data in front of first node for which mInsertBefore(datap, node->mDatap) returns TRUE
	// set mCurrentp to mQueuep
	BOOL addDataSorted(DATA_TYPE *datap);

	// sort the list using bubble-sort
	// Yes, this is a different name than the same function in LLLinkedList.
	// When it comes time for a name consolidation hopefully this one will win.
	BOOL bubbleSort();

	// does a single bubble sort pass on the list
	BOOL lazyBubbleSort();

	// returns TRUE if state successfully pushed (state stack not full)
	BOOL pushState();

	// returns TRUE if state successfully popped (state stack not empty)
	BOOL popState();

	// empties the state stack
	void clearStateStack();

	// randomly move the the links in the list for debug or (Discordian) purposes
	// sets mCurrentp and mQueuep to top of list
	void scramble();

private:
	// add node to beginning of list
	// set mCurrentp to mQueuep
	void addNode(LLDoubleLinkedNode<DATA_TYPE> *node);

	// add node to end of list
	// set mCurrentp to mQueuep
	void addNodeAtEnd(LLDoubleLinkedNode<DATA_TYPE> *node);
};

//#endif

////////////////////////////////////////////////////////////////////////////////////////////

// doublelinkedlist.cpp
// LLDoubleLinkedList template class implementation file.
// Provides a standard doubly linked list for fun and profit.
// 
// Copyright 2001, Linden Research, Inc.

//#include "llerror.h"
//#include "doublelinkedlist.h"

//////////////////////////////////////////////////////////////////////////////////////////
// LLDoubleLinkedNode
//////////////////////////////////////////////////////////////////////////////////////////


// assign the mDatap pointer
template <class DATA_TYPE>
LLDoubleLinkedNode<DATA_TYPE>::LLDoubleLinkedNode(DATA_TYPE *data) : 
	mDatap(data), mNextp(NULL), mPrevp(NULL)
{
}


// destructor does not, by default, destroy associated data
// however, the mDatap must be NULL to ensure that we aren't causing memory leaks
template <class DATA_TYPE>
LLDoubleLinkedNode<DATA_TYPE>::~LLDoubleLinkedNode()
{
	if (mDatap)
	{
		llerror("Attempting to call LLDoubleLinkedNode destructor with a non-null mDatap!", 1);
	}
}


// delete associated data and NULL out pointer
template <class DATA_TYPE>
void LLDoubleLinkedNode<DATA_TYPE>::deleteData()
{
	delete mDatap;
	mDatap = NULL;
}


template <class DATA_TYPE>
void LLDoubleLinkedNode<DATA_TYPE>::removeData()
{
	mDatap = NULL;
}


//////////////////////////////////////////////////////////////////////////////////////
// LLDoubleLinkedList
//////////////////////////////////////////////////////////////////////////////////////

//                                   <------- up -------
//
//                                               mCurrentp
//                                   mQueuep         |
//                                      |            |
//                                      |            | 
//                      .------.     .------.     .------.      .------.
//                      |      |---->|      |---->|      |----->|      |-----> NULL
//           NULL <-----|      |<----|      |<----|      |<-----|      |
//                     _'------'     '------'     '------'      '------:_
//             .------. /|               |             |               |\ .------.
//  NULL <-----|mHead |/                 |         mQueuep               \|mTail |-----> NULL
//             |      |               mCurrentp                           |      |
//             '------'                                                   '------'
//                               -------- down --------->

template <class DATA_TYPE>
LLDoubleLinkedList<DATA_TYPE>::LLDoubleLinkedList()
: mHead(NULL), mTail(NULL), mQueuep(NULL)
{
	mCurrentp = mHead.mNextp;
	mQueuep = mHead.mNextp;
	mStateStackDepth = 0;
	mCount = 0;
	mInsertBefore = NULL;
}


// destructor destroys list and nodes, but not data in nodes
template <class DATA_TYPE>
LLDoubleLinkedList<DATA_TYPE>::~LLDoubleLinkedList()
{
	removeAllNodes();
}


// put data into a node and stick it at the front of the list
// doesn't change mCurrentp nor mQueuep
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::addData(DATA_TYPE *data)
{
	// don't allow NULL to be passed to addData
	if (!data)
	{
		llerror("NULL pointer passed to LLDoubleLinkedList::addData()", 0);
	}

	// make the new node
	LLDoubleLinkedNode<DATA_TYPE> *temp = new LLDoubleLinkedNode<DATA_TYPE> (data);

	// add the node to the front of the list
	temp->mPrevp = NULL; 
	temp->mNextp = mHead.mNextp;
	mHead.mNextp = temp;

	// if there's something in the list, fix its back pointer
	if (temp->mNextp)
	{
		temp->mNextp->mPrevp = temp;
	}
	// otherwise, fix the tail of the list
	else 
	{
		mTail.mPrevp = temp;
	}

	mCount++;
}


// put data into a node and stick it at the end of the list
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::addDataAtEnd(DATA_TYPE *data)
{
	// don't allow NULL to be passed to addData
	if (!data)
	{
		llerror("NULL pointer passed to LLDoubleLinkedList::addData()", 0);
	}

	// make the new node
	LLDoubleLinkedNode<DATA_TYPE> *nodep = new LLDoubleLinkedNode<DATA_TYPE>(data);

	addNodeAtEnd(nodep);
	mCount++;
}


// search the list starting at mHead.mNextp and remove the link with mDatap == data
// set mCurrentp to mQueuep, or NULL if mQueuep points to node with mDatap == data
// return TRUE if found, FALSE if not found
template <class DATA_TYPE>
BOOL LLDoubleLinkedList<DATA_TYPE>::removeData(const DATA_TYPE *data)
{
	BOOL b_found = FALSE;
	// don't allow NULL to be passed to addData
	if (!data)
	{
		llerror("NULL pointer passed to LLDoubleLinkedList::removeData()", 0);
	}

	mCurrentp = mHead.mNextp;

	while (mCurrentp)
	{
		if (mCurrentp->mDatap == data)
		{
			b_found = TRUE;

			// if there is a next one, fix it
			if (mCurrentp->mNextp)
			{
				mCurrentp->mNextp->mPrevp = mCurrentp->mPrevp;
			}
			else // we are at end of list
			{
				mTail.mPrevp = mCurrentp->mPrevp;
			}

			// if there is a previous one, fix it
			if (mCurrentp->mPrevp)
			{
				mCurrentp->mPrevp->mNextp = mCurrentp->mNextp;
			}
			else // we are at beginning of list
			{
				mHead.mNextp = mCurrentp->mNextp;
			}

			// remove the node
			mCurrentp->removeData();
			delete mCurrentp;
			mCount--;
			break;
		}
		mCurrentp = mCurrentp->mNextp; 
	}

	// reset the list back to where it was
	if (mCurrentp == mQueuep)
	{
		mCurrentp = mQueuep = NULL;
	}
	else
	{
		mCurrentp = mQueuep;
	}

	return b_found;
}


// search the list starting at mHead.mNextp and delete the link with mDatap == data
// set mCurrentp to mQueuep, or NULL if mQueuep points to node with mDatap == data
// return TRUE if found, FALSE if not found
template <class DATA_TYPE>
BOOL LLDoubleLinkedList<DATA_TYPE>::deleteData(DATA_TYPE *data)
{
	BOOL b_found = FALSE;
	// don't allow NULL to be passed to addData
	if (!data)
	{
		llerror("NULL pointer passed to LLDoubleLinkedList::deleteData()", 0);
	}

	mCurrentp = mHead.mNextp;

	while (mCurrentp)
	{
		if (mCurrentp->mDatap == data)
		{
			b_found = TRUE;

			// if there is a next one, fix it
			if (mCurrentp->mNextp)
			{
				mCurrentp->mNextp->mPrevp = mCurrentp->mPrevp;
			}
			else // we are at end of list
			{
				mTail.mPrevp = mCurrentp->mPrevp;
			}

			// if there is a previous one, fix it
			if (mCurrentp->mPrevp)
			{
				mCurrentp->mPrevp->mNextp = mCurrentp->mNextp;
			}
			else // we are at beginning of list
			{
				mHead.mNextp = mCurrentp->mNextp;
			}

			// remove the node
			mCurrentp->deleteData();
			delete mCurrentp;
			mCount--;
			break;
		}
		mCurrentp = mCurrentp->mNextp;
	}

	// reset the list back to where it was
	if (mCurrentp == mQueuep)
	{
		mCurrentp = mQueuep = NULL;
	}
	else
	{
		mCurrentp = mQueuep;
	}

	return b_found;
}


// remove all nodes from the list and delete the associated data
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::deleteAllData()
{
	mCurrentp = mHead.mNextp;

	while (mCurrentp)
	{
		mQueuep = mCurrentp->mNextp;
		mCurrentp->deleteData();
		delete mCurrentp;
		mCurrentp = mQueuep;
	}

	// reset mHead and mQueuep
	mHead.mNextp = NULL;
	mTail.mPrevp = NULL;
	mCurrentp = mHead.mNextp;
	mQueuep = mHead.mNextp;
	mStateStackDepth = 0;
	mCount = 0;
}


// remove all nodes from the list but do not delete associated data
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::removeAllNodes()
{
	mCurrentp = mHead.mNextp;

	while (mCurrentp)
	{
		mQueuep = mCurrentp->mNextp;
		mCurrentp->removeData();
		delete mCurrentp;
		mCurrentp = mQueuep;
	}

	// reset mHead and mCurrentp
	mHead.mNextp = NULL;
	mTail.mPrevp = NULL;
	mCurrentp = mHead.mNextp;
	mQueuep = mHead.mNextp;
	mStateStackDepth = 0;
	mCount = 0;
}

template <class DATA_TYPE>
S32 LLDoubleLinkedList<DATA_TYPE>::getLength() const
{
//	U32	length = 0;
//	for (LLDoubleLinkedNode<DATA_TYPE>* temp = mHead.mNextp; temp != NULL; temp = temp->mNextp)
//	{
//		length++;
//	}
	return mCount;
}

// check to see if data is in list
// set mCurrentp and mQueuep to the target of search if found, otherwise set mCurrentp to mQueuep
// return TRUE if found, FALSE if not found
template <class DATA_TYPE>
BOOL LLDoubleLinkedList<DATA_TYPE>::checkData(const DATA_TYPE *data)
{
	mCurrentp = mHead.mNextp;

	while (mCurrentp)
	{
		if (mCurrentp->mDatap == data)
		{
			mQueuep = mCurrentp;
			return TRUE;
		}
		mCurrentp = mCurrentp->mNextp;
	}

	mCurrentp = mQueuep;
	return FALSE;
}

// NOTE: This next two funtions are only included here 
// for those too familiar with the LLLinkedList template class.
// They are depreciated.  resetList() is unecessary while 
// getCurrentData() is identical to getNextData() and has
// a misleading name.
//
// The recommended way to loop through a list is as follows:
//
// datap = list.getFirstData();
// while (datap)
// {
//     /* do stuff */
//     datap = list.getNextData();
// }

	// place mCurrentp and mQueuep on first node
	template <class DATA_TYPE>
	void LLDoubleLinkedList<DATA_TYPE>::resetList()
	{
		mCurrentp = mHead.mNextp;
		mQueuep = mHead.mNextp;
		mStateStackDepth = 0;
	}
	
	
	// return the data currently pointed to, 
	// set mCurrentp to that node and bump mQueuep down the list
	template <class DATA_TYPE>
	DATA_TYPE* LLDoubleLinkedList<DATA_TYPE>::getCurrentData()
	{
		if (mQueuep)
		{
			mCurrentp = mQueuep;
			mQueuep = mQueuep->mNextp;
			return mCurrentp->mDatap;
		}
		else
		{
			return NULL;
		}
	}


// reset the list and return the data currently pointed to, 
// set mCurrentp to that node and bump mQueuep down the list
template <class DATA_TYPE>
DATA_TYPE* LLDoubleLinkedList<DATA_TYPE>::getFirstData()
{
	mQueuep = mHead.mNextp;
	mCurrentp = mQueuep;
	if (mQueuep)
	{
		mQueuep = mQueuep->mNextp;
		return mCurrentp->mDatap;
	}
	else
	{
		return NULL;
	}
}


// reset the list and return the data at position n, set mCurentp 
// to that node and bump mQueuep down the list
// Note: n=0 will behave like getFirstData()
template <class DATA_TYPE>
DATA_TYPE* LLDoubleLinkedList<DATA_TYPE>::getNthData(U32 n)
{
	mCurrentp = mHead.mNextp;

	if (mCurrentp)
	{
		for (U32 i=0; i<n; i++)
		{
			mCurrentp = mCurrentp->mNextp;
			if (!mCurrentp)
			{
				break;
			}		
		}
	}

	if (mCurrentp)
	{
		// bump mQueuep down the list
		mQueuep = mCurrentp->mNextp;
		return mCurrentp->mDatap;
	}
	else
	{
		mQueuep = NULL;
		return NULL;
	}
}


// reset the list and return the last data in it, 
// set mCurrentp to that node and bump mQueuep up the list
template <class DATA_TYPE>
DATA_TYPE* LLDoubleLinkedList<DATA_TYPE>::getLastData()
{
	mQueuep = mTail.mPrevp;
	mCurrentp = mQueuep;
	if (mQueuep)
	{
		mQueuep = mQueuep->mPrevp;
		return mCurrentp->mDatap;
	}
	else
	{
		return NULL;
	}
}


// return the data in mQueuep, 
// set mCurrentp to mQueuep and bump mQueuep down the list
template <class DATA_TYPE>
DATA_TYPE* LLDoubleLinkedList<DATA_TYPE>::getNextData()
{
	if (mQueuep)
	{
		mCurrentp = mQueuep;
		mQueuep = mQueuep->mNextp;
		return mCurrentp->mDatap;
	}
	else
	{
		return NULL;
	}
}


// return the data in mQueuep, 
// set mCurrentp to mQueuep and bump mQueuep up the list
template <class DATA_TYPE>
DATA_TYPE* LLDoubleLinkedList<DATA_TYPE>::getPreviousData()
{
	if (mQueuep)
	{
		mCurrentp = mQueuep;
		mQueuep = mQueuep->mPrevp;
		return mCurrentp->mDatap;
	}
	else
	{
		return NULL;
	}
}


// remove the Node at mCurrentp
// set mCurrentp to mQueuep, or NULL if (mCurrentp == mQueuep)
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::removeCurrentData()
{
	if (mCurrentp)
	{
		// if there is a next one, fix it
		if (mCurrentp->mNextp)
		{
			mCurrentp->mNextp->mPrevp = mCurrentp->mPrevp;
		}
		else	// otherwise we are at end of list
		{
			mTail.mPrevp = mCurrentp->mPrevp;
		}

		// if there is a previous one, fix it
		if (mCurrentp->mPrevp)
		{
			mCurrentp->mPrevp->mNextp = mCurrentp->mNextp;
		}
		else	// otherwise we are at beginning of list
		{
			mHead.mNextp = mCurrentp->mNextp;
		}

		// remove the node
		mCurrentp->removeData();
		delete mCurrentp;
		mCount--;

		// check for redundant pointing
		if (mCurrentp == mQueuep)
		{
			mCurrentp = mQueuep = NULL;
		}
		else
		{
			mCurrentp = mQueuep;
		}
	}
}


// delete the Node at mCurrentp
// set mCurrentp to mQueuep, or NULL if (mCurrentp == mQueuep) 
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::deleteCurrentData()
{
	if (mCurrentp)
	{
		// remove the node
		// if there is a next one, fix it
		if (mCurrentp->mNextp)
		{
			mCurrentp->mNextp->mPrevp = mCurrentp->mPrevp;
		}
		else	// otherwise we are at end of list
		{
			mTail.mPrevp = mCurrentp->mPrevp;
		}

		// if there is a previous one, fix it
		if (mCurrentp->mPrevp)
		{
			mCurrentp->mPrevp->mNextp = mCurrentp->mNextp;
		}
		else	// otherwise we are at beginning of list
		{
			mHead.mNextp = mCurrentp->mNextp;
		}

		// remove the LLDoubleLinkedNode
		mCurrentp->deleteData();
		delete mCurrentp;
		mCount--;

		// check for redundant pointing
		if (mCurrentp == mQueuep)
		{
			mCurrentp = mQueuep = NULL;
		}
		else
		{
			mCurrentp = mQueuep;
		}
	}
}


// remove the Node at mCurrentp and insert it into newlist
// set mCurrentp to mQueuep, or NULL if (mCurrentp == mQueuep)
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::moveCurrentData(LLDoubleLinkedList<DATA_TYPE> *newlist)
{
	if (mCurrentp)
	{
		// remove the node
		// if there is a next one, fix it
		if (mCurrentp->mNextp)
		{
			mCurrentp->mNextp->mPrevp = mCurrentp->mPrevp;
		}
		else	// otherwise we are at end of list
		{
			mTail.mPrevp = mCurrentp->mPrevp;
		}

		// if there is a previous one, fix it
		if (mCurrentp->mPrevp)
		{
			mCurrentp->mPrevp->mNextp = mCurrentp->mNextp;
		}
		else	// otherwise we are at beginning of list
		{
			mHead.mNextp = mCurrentp->mNextp;
		}

		// move the node to the new list
		newlist->addNode(mCurrentp);

		// check for redundant pointing
		if (mCurrentp == mQueuep)
		{
			mCurrentp = mQueuep = NULL;
		}
		else
		{
			mCurrentp = mQueuep;
		}
	}
}


// Inserts the node previous to mCurrentp
// set mCurrentp to mQueuep
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::insertNode(LLDoubleLinkedNode<DATA_TYPE> *nodep)
{
	// don't allow pointer to NULL to be passed
	if (!nodep)
	{
		llerror("NULL pointer passed to LLDoubleLinkedList::insertNode()", 0);
	}
	if (!nodep->mDatap)
	{
		llerror("NULL data pointer passed to LLDoubleLinkedList::insertNode()", 0);
	}

	if (mCurrentp)
	{
		if (mCurrentp->mPrevp)
		{
			nodep->mPrevp = mCurrentp->mPrevp;
			nodep->mNextp = mCurrentp;
			mCurrentp->mPrevp->mNextp = nodep;
			mCurrentp->mPrevp = nodep;
		}
		else	// at beginning of list
		{
			nodep->mPrevp = NULL;
			nodep->mNextp = mCurrentp;
			mHead.mNextp = nodep;
			mCurrentp->mPrevp = nodep;
		}
		mCurrentp = mQueuep;
	}
	else	// add to front of list
	{
		addNode(nodep);
	}
}


// insert the data in front of mCurrentp
// set mCurrentp to mQueuep
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::insertData(DATA_TYPE *data)
{
	if (!data)
	{
		llerror("NULL data pointer passed to LLDoubleLinkedList::insertNode()", 0);
	}
	LLDoubleLinkedNode<DATA_TYPE> *node = new LLDoubleLinkedNode<DATA_TYPE>(data);
	insertNode(node);
	mCount++;
}


// if mCurrentp has a previous node then :
//   * swaps mCurrentp with its previous
//   * set mCurrentp to mQueuep
// otherwise does nothing
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::swapCurrentWithPrevious()
{
	if (mCurrentp)
	{
		if (mCurrentp->mPrevp)
		{
			// Pull mCurrentp out of list
			mCurrentp->mPrevp->mNextp = mCurrentp->mNextp;
			if (mCurrentp->mNextp)
			{
				mCurrentp->mNextp->mPrevp = mCurrentp->mPrevp;
			}
			else 	// mCurrentp was at end of list
			{
				mTail.mPrevp = mCurrentp->mPrevp;
			}

			// Fix mCurrentp's pointers
			mCurrentp->mNextp = mCurrentp->mPrevp;
			mCurrentp->mPrevp = mCurrentp->mNextp->mPrevp;
			mCurrentp->mNextp->mPrevp = mCurrentp;

			if (mCurrentp->mPrevp)
			{
				// Fix the backward pointer of mCurrentp's new previous
				mCurrentp->mPrevp->mNextp = mCurrentp;
			}
			else	// mCurrentp is now at beginning of list
			{
				mHead.mNextp = mCurrentp;
			}

			// Set the list back to the way it was
			mCurrentp = mQueuep;
		}
	}
}


// if mCurrentp has a next node then :
//   * swaps mCurrentp with its next
//   * set mCurrentp to mQueuep
// otherwise does nothing
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::swapCurrentWithNext()
{
	if (mCurrentp)
	{
		if (mCurrentp->mNextp)
		{
			// Pull mCurrentp out of list
			mCurrentp->mNextp->mPrevp = mCurrentp->mPrevp;
			if (mCurrentp->mPrevp)
			{
				mCurrentp->mPrevp->mNextp = mCurrentp->mNextp;
			}
			else 	// mCurrentp was at beginning of list
			{
				mHead.mNextp = mCurrentp->mNextp;
			}

			// Fix mCurrentp's pointers
			mCurrentp->mPrevp = mCurrentp->mNextp;
			mCurrentp->mNextp = mCurrentp->mPrevp->mNextp;
			mCurrentp->mPrevp->mNextp = mCurrentp;

			if (mCurrentp->mNextp)
			{
				// Fix the back pointer of mCurrentp's new next
				mCurrentp->mNextp->mPrevp = mCurrentp;
			}
			else 	// mCurrentp is now at end of list
			{
				mTail.mPrevp = mCurrentp;
			}
			
			// Set the list back to the way it was
			mCurrentp = mQueuep;
		}
	}
}

// move mCurrentp to the front of the list
// set mCurrentp to mQueuep
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::moveCurrentToFront()
{
	if (mCurrentp)
	{
		// if there is a previous one, fix it
		if (mCurrentp->mPrevp)
		{
			mCurrentp->mPrevp->mNextp = mCurrentp->mNextp;
		}
		else	// otherwise we are at beginning of list
		{
			// check for redundant pointing
			if (mCurrentp == mQueuep)
			{
				mCurrentp = mQueuep = NULL;
			}
			else
			{
				mCurrentp = mQueuep;
			}
			return;
		}

		// if there is a next one, fix it
		if (mCurrentp->mNextp)
		{
			mCurrentp->mNextp->mPrevp = mCurrentp->mPrevp;
		}
		else	// otherwise we are at end of list
		{
			mTail.mPrevp = mCurrentp->mPrevp;
		}

		// add mCurrentp to beginning of list
		mCurrentp->mNextp = mHead.mNextp;
		mHead.mNextp->mPrevp = mCurrentp;	// mHead.mNextp MUST be valid, 
											// or the list had only one node
											// and we would have returned already
		mCurrentp->mPrevp = NULL;
		mHead.mNextp = mCurrentp;

		// check for redundant pointing
		if (mCurrentp == mQueuep)
		{
			mCurrentp = mQueuep = NULL;
		}
		else
		{
			mCurrentp = mQueuep;
		}
	}

}

// move mCurrentp to the end of the list
// set mCurrentp to mQueuep
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::moveCurrentToEnd()
{
	if (mCurrentp)
	{
		// if there is a next one, fix it
		if (mCurrentp->mNextp)
		{
			mCurrentp->mNextp->mPrevp = mCurrentp->mPrevp;
		}
		else	// otherwise we are at end of list and we're done
		{
			// check for redundant pointing
			if (mCurrentp == mQueuep)
			{
				mCurrentp = mQueuep = NULL;
			}
			else
			{
				mCurrentp = mQueuep;
			}
			return;
		}

		// if there is a previous one, fix it
		if (mCurrentp->mPrevp)
		{
			mCurrentp->mPrevp->mNextp = mCurrentp->mNextp;
		}
		else	// otherwise we are at beginning of list
		{
			mHead.mNextp = mCurrentp->mNextp;
		}

		// add mCurrentp to end of list
		mCurrentp->mPrevp = mTail.mPrevp;
		mTail.mPrevp->mNextp = mCurrentp;	// mTail.mPrevp MUST be valid, 
											// or the list had only one node
											// and we would have returned already
		mCurrentp->mNextp = NULL;
		mTail.mPrevp = mCurrentp;

		// check for redundant pointing
		if (mCurrentp == mQueuep)
		{
			mCurrentp = mQueuep = NULL;
		}
		else
		{
			mCurrentp = mQueuep;
		}
	}
}


template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::setInsertBefore(BOOL (*insert_before)(DATA_TYPE *first, DATA_TYPE *second) )
{
	mInsertBefore = insert_before;
}


// add data in front of the first node for which mInsertBefore(datap, node->mDatap) returns TRUE
// set mCurrentp to mQueuep
template <class DATA_TYPE>
BOOL LLDoubleLinkedList<DATA_TYPE>::addDataSorted(DATA_TYPE *datap)
{
	// don't allow NULL to be passed to addData()
	if (!datap)
	{
		llerror("NULL pointer passed to LLDoubleLinkedList::addDataSorted()", 0);
	}

	// has mInsertBefore not been set?
	if (!mInsertBefore)
	{
		addData(datap);
		return FALSE;
	}

	// is the list empty?
	if (!mHead.mNextp)
	{
		addData(datap);
		return TRUE;
	}

	// Note: this step has been added so that the behavior of LLDoubleLinkedList
	// is as rigorous as the LLLinkedList class about adding duplicate nodes.
	// Duplicate nodes can cause a problem when sorting if mInsertBefore(foo, foo) 
	// returns TRUE.  However, if mInsertBefore(foo, foo) returns FALSE, then there 
	// shouldn't be any reason to exclude duplicate nodes (as we do here).
	if (checkData(datap))
	{
		return FALSE;
	}
	
	mCurrentp = mHead.mNextp;
	while (mCurrentp)
	{
		// check to see if datap is already in the list
		if (datap == mCurrentp->mDatap)
		{
			return FALSE;
		}
		else if (mInsertBefore(datap, mCurrentp->mDatap))
		{
			insertData(datap);
			return TRUE;
		}
		mCurrentp = mCurrentp->mNextp;
	}
	
	addDataAtEnd(datap);
	return TRUE;
}


// bubble-sort until sorted and return TRUE if anything was sorted
// leaves mQueuep pointing at last node that was swapped with its mNextp
//
// NOTE: if you find this function looping for really long times, then you
// probably need to check your implementation of mInsertBefore(a,b) and make 
// sure it does not return TRUE when (a == b)!
template <class DATA_TYPE>
BOOL LLDoubleLinkedList<DATA_TYPE>::bubbleSort()
{
	BOOL b_swapped = FALSE;
	U32 count = 0;
	while (lazyBubbleSort()) 
	{
		b_swapped = TRUE;
		if (count++ > 0x7FFFFFFF)
		{
			llwarning("LLDoubleLinkedList::bubbleSort() : too many passes...", 1);
			llwarning("    make sure the mInsertBefore(a, b) does not return TRUE for a == b", 1);
			break;
		}
	}
	return b_swapped;
}


// do a single bubble-sort pass and return TRUE if anything was sorted
// leaves mQueuep pointing at last node that was swapped with its mNextp
template <class DATA_TYPE>
BOOL LLDoubleLinkedList<DATA_TYPE>::lazyBubbleSort()
{
	// has mInsertBefore been set?
	if (!mInsertBefore)
	{
		return FALSE;
	}

	// is list empty?
	mCurrentp = mHead.mNextp;
	if (!mCurrentp)
	{
		return FALSE;
	}

	BOOL b_swapped = FALSE;

	// the sort will exit after 0x7FFFFFFF nodes or the end of the list, whichever is first
	S32  length = 0x7FFFFFFF;
	S32  count = 0;

	while (mCurrentp  &&  mCurrentp->mNextp  &&  count<length)
	{
		if (mInsertBefore(mCurrentp->mNextp->mDatap, mCurrentp->mDatap))
		{
			b_swapped = TRUE;
			mQueuep = mCurrentp;
			swapCurrentWithNext();	// sets mCurrentp to mQueuep
		}
		count++;
		mCurrentp = mCurrentp->mNextp;
	}
	
	return b_swapped;
}


template <class DATA_TYPE>
BOOL LLDoubleLinkedList<DATA_TYPE>::pushState()
{
	if (mStateStackDepth < LLDOUBLE_LINKED_LIST_STATE_STACK_DEPTH)
	{
		*(mQueuepStack + mStateStackDepth) = mQueuep;
		*(mCurrentpStack + mStateStackDepth) = mCurrentp;
		mStateStackDepth++;
		return TRUE;
	}
	return FALSE;
}


template <class DATA_TYPE>
BOOL LLDoubleLinkedList<DATA_TYPE>::popState()
{
	if (mStateStackDepth > 0)
	{
		mStateStackDepth--;
		mQueuep = *(mQueuepStack + mStateStackDepth);
		mCurrentp = *(mCurrentpStack + mStateStackDepth);
		return TRUE;
	}
	return FALSE;
}


template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::clearStateStack()
{
	mStateStackDepth = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// private members
//////////////////////////////////////////////////////////////////////////////////////////

// add node to beginning of list
// set mCurrentp to mQueuep
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::addNode(LLDoubleLinkedNode<DATA_TYPE> *nodep)
{
	// add the node to the front of the list
	nodep->mPrevp = NULL;
	nodep->mNextp = mHead.mNextp;
	mHead.mNextp = nodep;

	// if there's something in the list, fix its back pointer
	if (nodep->mNextp)
	{
		nodep->mNextp->mPrevp = nodep;
	}
	else	// otherwise fix the tail node
	{
		mTail.mPrevp = nodep;
	}

	mCurrentp = mQueuep;
}


// add node to end of list
// set mCurrentp to mQueuep
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::addNodeAtEnd(LLDoubleLinkedNode<DATA_TYPE> *node)
{
	// add the node to the end of the list
	node->mNextp = NULL;
	node->mPrevp = mTail.mPrevp;
	mTail.mPrevp = node;

	// if there's something in the list, fix its back pointer
	if (node->mPrevp)
	{
		node->mPrevp->mNextp = node;
	}
	else	// otherwise fix the head node
	{
		mHead.mNextp = node;
	}

	mCurrentp = mQueuep;
}


// randomly move nodes in the list for DEBUG (or Discordian) purposes
// sets mCurrentp and mQueuep to top of list
template <class DATA_TYPE>
void LLDoubleLinkedList<DATA_TYPE>::scramble()
{
	S32 random_number;
	DATA_TYPE *datap = getFirstData();
	while(datap)
	{
		random_number = ll_rand(5);

		if (0 == random_number)
		{
			removeCurrentData();
			addData(datap);
		}
		else if (1 == random_number)
		{
			removeCurrentData();
			addDataAtEnd(datap);
		}
		else if (2 == random_number)
		{
			swapCurrentWithPrevious();
		}
		else if (3 == random_number)
		{
			swapCurrentWithNext();
		}
		datap = getNextData();
	}
	mQueuep = mHead.mNextp;
	mCurrentp = mQueuep;
}

template <class DATA_TYPE>
BOOL LLDoubleLinkedList<DATA_TYPE>::isEmpty()
{
	return (mCount == 0);
}


#endif
