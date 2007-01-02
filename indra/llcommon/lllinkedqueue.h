/** 
 * @file lllinkedqueue.h
 * @brief Declaration of linked queue classes.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLLINKEDQUEUE_H
#define LL_LLLINKEDQUEUE_H

#include "llerror.h"

// node that actually contains the data
template <class DATA_TYPE> class LLLinkedQueueNode
{
public:
	DATA_TYPE			mData;
	LLLinkedQueueNode	*mNextp;
	LLLinkedQueueNode	*mPrevp;


public:
	LLLinkedQueueNode();
	LLLinkedQueueNode(const DATA_TYPE data);

	// destructor does not, by default, destroy associated data
	// however, the mDatap must be NULL to ensure that we aren't causing memory leaks
	~LLLinkedQueueNode();
};



template <class DATA_TYPE> class LLLinkedQueue
{

public:
	LLLinkedQueue();

	// destructor destroys list and nodes, but not data in nodes
	~LLLinkedQueue();

	// Puts at end of FIFO
	void push(const DATA_TYPE data);

	// Takes off front of FIFO
	BOOL pop(DATA_TYPE &data);
	BOOL peek(DATA_TYPE &data);

	void reset();

	S32 getLength() const;

	BOOL isEmpty() const;

	BOOL remove(const DATA_TYPE data);

	BOOL checkData(const DATA_TYPE data) const;

private:
	// add node to end of list
	// set mCurrentp to mQueuep
	void addNodeAtEnd(LLLinkedQueueNode<DATA_TYPE> *nodep);

private:
	LLLinkedQueueNode<DATA_TYPE> mHead;		// head node
	LLLinkedQueueNode<DATA_TYPE> mTail;		// tail node
	S32 mLength;
};


//
// Nodes
//

template <class DATA_TYPE>
LLLinkedQueueNode<DATA_TYPE>::LLLinkedQueueNode() : 
	mData(), mNextp(NULL), mPrevp(NULL)
{ }

template <class DATA_TYPE>
LLLinkedQueueNode<DATA_TYPE>::LLLinkedQueueNode(const DATA_TYPE data) : 
	mData(data), mNextp(NULL), mPrevp(NULL)
{ }

template <class DATA_TYPE>
LLLinkedQueueNode<DATA_TYPE>::~LLLinkedQueueNode()
{ }


//
// Queue itself
//

template <class DATA_TYPE>
LLLinkedQueue<DATA_TYPE>::LLLinkedQueue()
:	mHead(), 
	mTail(), 
	mLength(0)
{ }


// destructor destroys list and nodes, but not data in nodes
template <class DATA_TYPE>
LLLinkedQueue<DATA_TYPE>::~LLLinkedQueue()
{
	reset();
}


// put data into a node and stick it at the end of the list
template <class DATA_TYPE>
void LLLinkedQueue<DATA_TYPE>::push(const DATA_TYPE data)
{
	// make the new node
	LLLinkedQueueNode<DATA_TYPE> *nodep = new LLLinkedQueueNode<DATA_TYPE>(data);

	addNodeAtEnd(nodep);
}


// search the list starting at mHead.mNextp and remove the link with mDatap == data
// set mCurrentp to mQueuep, or NULL if mQueuep points to node with mDatap == data
// return TRUE if found, FALSE if not found
template <class DATA_TYPE>
BOOL LLLinkedQueue<DATA_TYPE>::remove(const DATA_TYPE data)
{
	BOOL b_found = FALSE;

	LLLinkedQueueNode<DATA_TYPE> *currentp = mHead.mNextp;

	while (currentp)
	{
		if (currentp->mData == data)
		{
			b_found = TRUE;

			// if there is a next one, fix it
			if (currentp->mNextp)
			{
				currentp->mNextp->mPrevp = currentp->mPrevp;
			}
			else // we are at end of list
			{
				mTail.mPrevp = currentp->mPrevp;
			}

			// if there is a previous one, fix it
			if (currentp->mPrevp)
			{
				currentp->mPrevp->mNextp = currentp->mNextp;
			}
			else // we are at beginning of list
			{
				mHead.mNextp = currentp->mNextp;
			}

			// remove the node
			delete currentp;
			mLength--;
			break;
		}
		currentp = currentp->mNextp; 
	}

	return b_found;
}


// remove all nodes from the list but do not delete associated data
template <class DATA_TYPE>
void LLLinkedQueue<DATA_TYPE>::reset()
{
	LLLinkedQueueNode<DATA_TYPE> *currentp;
	LLLinkedQueueNode<DATA_TYPE> *nextp;
	currentp = mHead.mNextp;

	while (currentp)
	{
		nextp = currentp->mNextp;
		delete currentp;
		currentp = nextp;
	}

	// reset mHead and mCurrentp
	mHead.mNextp = NULL;
	mTail.mPrevp = NULL;
	mLength = 0;
}

template <class DATA_TYPE>
S32 LLLinkedQueue<DATA_TYPE>::getLength() const
{
	return mLength;
}

template <class DATA_TYPE>
BOOL LLLinkedQueue<DATA_TYPE>::isEmpty() const
{
	return mLength <= 0;
}

// check to see if data is in list
// set mCurrentp and mQueuep to the target of search if found, otherwise set mCurrentp to mQueuep
// return TRUE if found, FALSE if not found
template <class DATA_TYPE>
BOOL LLLinkedQueue<DATA_TYPE>::checkData(const DATA_TYPE data) const
{
	LLLinkedQueueNode<DATA_TYPE> *currentp = mHead.mNextp;

	while (currentp)
	{
		if (currentp->mData == data)
		{
			return TRUE;
		}
		currentp = currentp->mNextp;
	}
	return FALSE;
}

template <class DATA_TYPE>
BOOL LLLinkedQueue<DATA_TYPE>::pop(DATA_TYPE &data)
{
	LLLinkedQueueNode<DATA_TYPE> *currentp;

	currentp = mHead.mNextp;
	if (!currentp)
	{
		return FALSE;
	}

	mHead.mNextp = currentp->mNextp;
	if (currentp->mNextp)
	{
		currentp->mNextp->mPrevp = currentp->mPrevp;
	}
	else
	{
		mTail.mPrevp = currentp->mPrevp;
	}

	data = currentp->mData;
	delete currentp;
	mLength--;
	return TRUE;
}

template <class DATA_TYPE>
BOOL LLLinkedQueue<DATA_TYPE>::peek(DATA_TYPE &data)
{
	LLLinkedQueueNode<DATA_TYPE> *currentp;

	currentp = mHead.mNextp;
	if (!currentp)
	{
		return FALSE;
	}
	data = currentp->mData;
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////////
// private members
//////////////////////////////////////////////////////////////////////////////////////////


// add node to end of list
// set mCurrentp to mQueuep
template <class DATA_TYPE>
void LLLinkedQueue<DATA_TYPE>::addNodeAtEnd(LLLinkedQueueNode<DATA_TYPE> *nodep)
{
	// add the node to the end of the list
	nodep->mNextp = NULL;
	nodep->mPrevp = mTail.mPrevp;
	mTail.mPrevp = nodep;

	// if there's something in the list, fix its back pointer
	if (nodep->mPrevp)
	{
		nodep->mPrevp->mNextp = nodep;
	}
	else	// otherwise fix the head node
	{
		mHead.mNextp = nodep;
	}
	mLength++;
}

#endif
