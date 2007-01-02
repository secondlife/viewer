/** 
 * @file llptrskiplist.h
 * @brief Skip list implementation.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPTRSKIPLIST_H
#define LL_LLPTRSKIPLIST_H

#include "llerror.h"
//#include "vmath.h"

/////////////////////////////////////////////
//
//	LLPtrSkipList implementation - skip list for pointers to objects
//

template <class DATA_TYPE, S32 BINARY_DEPTH = 8>
class LLPtrSkipList
{
public:
	friend class LLPtrSkipNode;

	// basic constructor
	LLPtrSkipList();
	// basic constructor including sorter
	LLPtrSkipList(BOOL	(*insert_first)(DATA_TYPE *first, DATA_TYPE *second), 
				  BOOL	(*equals)(DATA_TYPE *first, DATA_TYPE *second));
	~LLPtrSkipList();

	inline void setInsertFirst(BOOL (*insert_first)(const DATA_TYPE *first, const DATA_TYPE *second));
	inline void setEquals(BOOL (*equals)(const DATA_TYPE *first, const DATA_TYPE *second));

	inline BOOL addData(DATA_TYPE *data);

	inline BOOL checkData(const DATA_TYPE *data);

	inline S32 getLength();	// returns number of items in the list - NOT constant time!

	inline BOOL removeData(const DATA_TYPE *data);

	// note that b_sort is ignored
	inline BOOL moveData(const DATA_TYPE *data, LLPtrSkipList *newlist, BOOL b_sort);

	inline BOOL moveCurrentData(LLPtrSkipList *newlist, BOOL b_sort);

	// resort -- use when the value we're sorting by changes
	/* IW 12/6/02 - This doesn't work!
	   Instead, remove the data BEFORE you change it
	   Then re-insert it after you change it
	BOOL resortData(DATA_TYPE *data)
	*/

	// remove all nodes from the list but do not delete data
	inline void removeAllNodes();

	inline BOOL deleteData(const DATA_TYPE *data);

	// remove all nodes from the list and delete data
	inline void deleteAllData();

	// place mCurrentp on first node
	inline void resetList();

	// return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
	inline DATA_TYPE	*getCurrentData();

	// same as getCurrentData() but a more intuitive name for the operation
	inline DATA_TYPE	*getNextData();

	// remove the Node at mCurentOperatingp
	// leave mCurrentp and mCurentOperatingp on the next entry
	inline void removeCurrentData();

	// delete the Node at mCurentOperatingp
	// leave mCurrentp and mCurentOperatingp on the next entry
	inline void deleteCurrentData();

	// reset the list and return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
	inline DATA_TYPE	*getFirstData();

	// TRUE if nodes are not in sorted order
	inline BOOL corrupt();

protected:
	class LLPtrSkipNode
	{
	public:
		LLPtrSkipNode()
		:	mData(NULL)
		{
			S32 i;
			for (i = 0; i < BINARY_DEPTH; i++)
			{
				mForward[i] = NULL;
			}
		}

		LLPtrSkipNode(DATA_TYPE *data)
			: mData(data)
		{
			S32 i;
			for (i = 0; i < BINARY_DEPTH; i++)
			{
				mForward[i] = NULL;
			}
		}

		~LLPtrSkipNode()
		{
			if (mData)
			{
				llerror("Attempting to call LLPtrSkipNode destructor with a non-null mDatap!", 1);
			}
		}

		// delete associated data and NULLs out pointer
		void deleteData()
		{
			delete mData;
			mData = NULL;
		}

		// NULLs out pointer
		void removeData()
		{
			mData = NULL;
		}

		DATA_TYPE					*mData;
		LLPtrSkipNode				*mForward[BINARY_DEPTH];
	};

	static BOOL					defaultEquals(const DATA_TYPE *first, const DATA_TYPE *second)
	{
		return first == second;
	}


	LLPtrSkipNode				mHead;
	LLPtrSkipNode				*mUpdate[BINARY_DEPTH];
	LLPtrSkipNode				*mCurrentp;
	LLPtrSkipNode				*mCurrentOperatingp;
	S32							mLevel;
	BOOL						(*mInsertFirst)(const DATA_TYPE *first, const DATA_TYPE *second);
	BOOL						(*mEquals)(const DATA_TYPE *first, const DATA_TYPE *second);
};


// basic constructor
template <class DATA_TYPE, S32 BINARY_DEPTH> 
LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::LLPtrSkipList()
	: mInsertFirst(NULL), mEquals(defaultEquals)
{
	if (BINARY_DEPTH < 2)
	{
		llerrs << "Trying to create skip list with too little depth, "
			"must be 2 or greater" << llendl;
	}
	S32 i;
	for (i = 0; i < BINARY_DEPTH; i++)
	{
		mUpdate[i] = NULL;
	}
	mLevel = 1;
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}

// basic constructor including sorter
template <class DATA_TYPE, S32 BINARY_DEPTH> 
LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::LLPtrSkipList(BOOL	(*insert_first)(DATA_TYPE *first, DATA_TYPE *second), 
													  BOOL	(*equals)(DATA_TYPE *first, DATA_TYPE *second)) 
	:mInsertFirst(insert_first), mEquals(equals)
{
	if (BINARY_DEPTH < 2)
	{
		llerrs << "Trying to create skip list with too little depth, "
			"must be 2 or greater" << llendl;
	}
	mLevel = 1;
	S32 i;
	for (i = 0; i < BINARY_DEPTH; i++)
	{
		mHead.mForward[i] = NULL;
		mUpdate[i] = NULL;
	}
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}

template <class DATA_TYPE, S32 BINARY_DEPTH>
inline LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::~LLPtrSkipList()
{
	removeAllNodes();
}

template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline void LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::setInsertFirst(BOOL (*insert_first)(const DATA_TYPE *first, const DATA_TYPE *second))
{
	mInsertFirst = insert_first;
}

template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline void LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::setEquals(BOOL (*equals)(const DATA_TYPE *first, const DATA_TYPE *second))
{
	mEquals = equals;
}

template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline BOOL LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::addData(DATA_TYPE *data)
{
	S32				level;
	LLPtrSkipNode	*current = &mHead;
	LLPtrSkipNode	*temp;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mData, data)))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	else
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(temp->mData < data))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}

	
	// we're now just in front of where we want to be . . . take one step forward
	current = *current->mForward;

	// now add the new node
	S32 newlevel;
	for (newlevel = 1; newlevel <= mLevel && newlevel < BINARY_DEPTH; newlevel++)
	{
		if (frand(1.f) < 0.5f)
			break;
	}

	LLPtrSkipNode *snode = new LLPtrSkipNode(data);

	if (newlevel > mLevel)
	{
		mHead.mForward[mLevel] = NULL;
		mUpdate[mLevel] = &mHead;
		mLevel = newlevel;
	}

	for (level = 0; level < newlevel; level++)
	{
		snode->mForward[level] = mUpdate[level]->mForward[level];
		mUpdate[level]->mForward[level] = snode;
	}
	return TRUE;
}

template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline BOOL LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::checkData(const DATA_TYPE *data)
{
	S32			level;
	LLPtrSkipNode	*current = &mHead;
	LLPtrSkipNode	*temp;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mData, data)))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	else
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(temp->mData < data))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}

	
	// we're now just in front of where we want to be . . . take one step forward
	current = *current->mForward;
	
	if (current)
	{
		return mEquals(current->mData, data);
	}
	else
	{
		return FALSE;
	}
}

// returns number of items in the list
template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline S32 LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::getLength()
{
	U32	length = 0;
	for (LLPtrSkipNode* temp = *(mHead.mForward); temp != NULL; temp = temp->mForward[0])
	{
		length++;
	}
	return length;
}

template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline BOOL LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::removeData(const DATA_TYPE *data)
{
	S32			level;
	LLPtrSkipNode	*current = &mHead;
	LLPtrSkipNode	*temp;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mData, data)))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	else
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(temp->mData < data))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}

	
	// we're now just in front of where we want to be . . . take one step forward
	current = *current->mForward;

	if (!current)
	{
		// empty list or beyond the end!
		return FALSE;
	}

	// is this the one we want?
	if (!mEquals(current->mData, data))
	{
		// nope!
			return FALSE;
	}
	else
	{
		// yes it is!  change pointers as required
		for (level = 0; level < mLevel; level++)
		{
			if (mUpdate[level]->mForward[level] != current)
			{
				// cool, we've fixed all the pointers!
				break;
			}
			mUpdate[level]->mForward[level] = current->mForward[level];
		}

		// clean up cuurent
		current->removeData();
		delete current;

		// clean up mHead
		while (  (mLevel > 1)
			   &&(!mHead.mForward[mLevel - 1]))
		{
			mLevel--;
		}
	}
	return TRUE;
}

// note that b_sort is ignored
template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline BOOL LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::moveData(const DATA_TYPE *data, LLPtrSkipList *newlist, BOOL b_sort)
{
	BOOL removed = removeData(data);
	BOOL added = newlist->addData(data);
	return removed && added;
}

template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline BOOL LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::moveCurrentData(LLPtrSkipList *newlist, BOOL b_sort)
{
	if (mCurrentOperatingp)
	{
		mCurrentp = mCurrentOperatingp->mForward[0];	
		BOOL removed = removeData(mCurrentOperatingp);
		BOOL added = newlist->addData(mCurrentOperatingp);
		mCurrentOperatingp = mCurrentp;
		return removed && added;
	}
	return FALSE;
}

// resort -- use when the value we're sorting by changes
/* IW 12/6/02 - This doesn't work!
   Instead, remove the data BEFORE you change it
   Then re-insert it after you change it
BOOL resortData(DATA_TYPE *data)
{
	removeData(data);
	addData(data);
}
*/

// remove all nodes from the list but do not delete data
template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline void LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::removeAllNodes()
{
	LLPtrSkipNode *temp;
	// reset mCurrentp
	mCurrentp = *(mHead.mForward);

	while (mCurrentp)
	{
		temp = mCurrentp->mForward[0];
		mCurrentp->removeData();
		delete mCurrentp;
		mCurrentp = temp;
	}

	S32 i;
	for (i = 0; i < BINARY_DEPTH; i++)
	{
		mHead.mForward[i] = NULL;
		mUpdate[i] = NULL;
	}

	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}

template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline BOOL LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::deleteData(const DATA_TYPE *data)
{
	S32			level;
	LLPtrSkipNode	*current = &mHead;
	LLPtrSkipNode	*temp;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mData, data)))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	else
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(temp->mData < data))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}

	
	// we're now just in front of where we want to be . . . take one step forward
	current = *current->mForward;

	if (!current)
	{
		// empty list or beyond the end!
		return FALSE;
	}

	// is this the one we want?
	if (!mEquals(current->mData, data))
	{
		// nope!
		return FALSE;
	}
	else
	{
		// do we need to fix current or currentop?
		if (current == mCurrentp)
		{
			mCurrentp = current->mForward[0];
		}

		if (current == mCurrentOperatingp)
		{
			mCurrentOperatingp = current->mForward[0];
		}

		// yes it is!  change pointers as required
		for (level = 0; level < mLevel; level++)
		{
			if (mUpdate[level]->mForward[level] != current)
			{
				// cool, we've fixed all the pointers!
				break;
			}
			mUpdate[level]->mForward[level] = current->mForward[level];
		}

		// clean up cuurent
		current->deleteData();
		delete current;

		// clean up mHead
		while (  (mLevel > 1)
			   &&(!mHead.mForward[mLevel - 1]))
		{
			mLevel--;
		}
	}
	return TRUE;
}

// remove all nodes from the list and delete data
template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline void LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::deleteAllData()
{
	LLPtrSkipNode *temp;
	// reset mCurrentp
	mCurrentp = *(mHead.mForward);

	while (mCurrentp)
	{
		temp = mCurrentp->mForward[0];
		mCurrentp->deleteData();
		delete mCurrentp;
		mCurrentp = temp;
	}

	S32 i;
	for (i = 0; i < BINARY_DEPTH; i++)
	{
		mHead.mForward[i] = NULL;
		mUpdate[i] = NULL;
	}

	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}

// place mCurrentp on first node
template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline void LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::resetList()
{
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}

// return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline DATA_TYPE *LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::getCurrentData()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = *mCurrentp->mForward;
		return mCurrentOperatingp->mData;
	}
	else
	{
		//return NULL;		// causes compile warning
		return 0; 			// equivalent, but no warning
	}
}

// same as getCurrentData() but a more intuitive name for the operation
template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline DATA_TYPE *LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::getNextData()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = *mCurrentp->mForward;
		return mCurrentOperatingp->mData;
	}
	else
	{
		//return NULL;		// causes compile warning
		return 0; 			// equivalent, but no warning
	}
}

// remove the Node at mCurentOperatingp
// leave mCurrentp and mCurentOperatingp on the next entry
template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline void LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::removeCurrentData()
{
	if (mCurrentOperatingp)
	{
		removeData(mCurrentOperatingp->mData);
	}
}

// delete the Node at mCurentOperatingp
// leave mCurrentp and mCurentOperatingp on the next entry
template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline void LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::deleteCurrentData()
{
	if (mCurrentOperatingp)
	{
		deleteData(mCurrentOperatingp->mData);
	}
}

// reset the list and return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline DATA_TYPE *LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::getFirstData()
{
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mData;
	}
	else
	{
		//return NULL;		// causes compile warning
		return 0; 			// equivalent, but no warning
	}
}

template <class DATA_TYPE, S32 BINARY_DEPTH> 
inline BOOL LLPtrSkipList<DATA_TYPE, BINARY_DEPTH>::corrupt()
{
	LLPtrSkipNode *previous = mHead.mForward[0];

	// Empty lists are not corrupt.
	if (!previous) return FALSE;

	LLPtrSkipNode *current = previous->mForward[0];
	while(current)
	{
		if (!mInsertFirst(previous->mData, current->mData))
		{
			// prev shouldn't be in front of cur!
			return TRUE;
		}
		current = current->mForward[0];
	}
	return FALSE;
}

#endif
