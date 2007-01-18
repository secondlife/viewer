/** 
 * @file llskiplist.h
 * @brief skip list implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#ifndef LL_LLSKIPLIST_H
#define LL_LLSKIPLIST_H

#include "llerror.h"
//#include "vmath.h"

// NOTA BENE: Insert first needs to be < NOT <=

template <class DATA_TYPE, S32 BINARY_DEPTH = 10>
class LLSkipList
{
public:
	typedef BOOL (*compare)(const DATA_TYPE& first, const DATA_TYPE& second);
	typedef compare insert_func;
	typedef compare equals_func;

	void init();

	// basic constructor
	LLSkipList();

	// basic constructor including sorter
	LLSkipList(insert_func insert_first, equals_func equals);
	~LLSkipList();

	inline void setInsertFirst(insert_func insert_first);
	inline void setEquals(equals_func equals);

	inline BOOL addData(const DATA_TYPE& data);
	inline BOOL checkData(const DATA_TYPE& data);

	// returns number of items in the list
	inline S32 getLength() const; // NOT a constant time operation, traverses entire list!

	inline BOOL moveData(const DATA_TYPE& data, LLSkipList *newlist);

	inline BOOL removeData(const DATA_TYPE& data);

	// remove all nodes from the list but do not delete data
	inline void removeAllNodes();

	// place mCurrentp on first node
	inline void resetList();

	// return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
	inline DATA_TYPE getCurrentData();

	// same as getCurrentData() but a more intuitive name for the operation
	inline DATA_TYPE getNextData();

	// remove the Node at mCurentOperatingp
	// leave mCurrentp and mCurentOperatingp on the next entry
	inline void removeCurrentData();

	// reset the list and return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
	inline DATA_TYPE getFirstData();

	class LLSkipNode
	{
	public:
		LLSkipNode()
		:	mData(0)
		{
			S32 i;
			for (i = 0; i < BINARY_DEPTH; i++)
			{
				mForward[i] = NULL;
			}
		}

		LLSkipNode(DATA_TYPE data)
			: mData(data)
		{
			S32 i;
			for (i = 0; i < BINARY_DEPTH; i++)
			{
				mForward[i] = NULL;
			}
		}

		~LLSkipNode()
		{
		}

		DATA_TYPE					mData;
		LLSkipNode					*mForward[BINARY_DEPTH];

	private:
		// Disallow copying of LLSkipNodes by not implementing these methods.
		LLSkipNode(const LLSkipNode &);
		LLSkipNode &operator=(const LLSkipNode &);
	};

	static BOOL defaultEquals(const DATA_TYPE& first, const DATA_TYPE& second)
	{
		return first == second;
	}

private:
	LLSkipNode					mHead;
	LLSkipNode					*mUpdate[BINARY_DEPTH];
	LLSkipNode					*mCurrentp;
	LLSkipNode					*mCurrentOperatingp;
	S32							mLevel;
	insert_func mInsertFirst;
	equals_func mEquals;

private:
	// Disallow copying of LLSkipNodes by not implementing these methods.
	LLSkipList(const LLSkipList &);
	LLSkipList &operator=(const LLSkipList &);
};


///////////////////////
//
// Implementation
//

template <class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipList<DATA_TYPE, BINARY_DEPTH>::init()
{
	if (BINARY_DEPTH < 2)
	{
		llerrs << "Trying to create skip list with too little depth, "
			"must be 2 or greater" << llendl;
	}
	S32 i;
	for (i = 0; i < BINARY_DEPTH; i++)
	{
		mHead.mForward[i] = NULL;
		mUpdate[i] = NULL;
	}
	mLevel = 1;
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}


// basic constructor
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline LLSkipList<DATA_TYPE, BINARY_DEPTH>::LLSkipList()
:	mInsertFirst(NULL), 
	mEquals(defaultEquals)
{
	init();
}

// basic constructor including sorter
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline LLSkipList<DATA_TYPE, BINARY_DEPTH>::LLSkipList(insert_func insert,
													   equals_func equals)
:	mInsertFirst(insert), 
	mEquals(equals)
{
	init();
}

template <class DATA_TYPE, S32 BINARY_DEPTH>
inline LLSkipList<DATA_TYPE, BINARY_DEPTH>::~LLSkipList()
{
	removeAllNodes();
}

template <class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipList<DATA_TYPE, BINARY_DEPTH>::setInsertFirst(insert_func insert_first)
{
	mInsertFirst = insert_first;
}

template <class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipList<DATA_TYPE, BINARY_DEPTH>::setEquals(equals_func equals)
{
	mEquals = equals;
}

template <class DATA_TYPE, S32 BINARY_DEPTH>
inline BOOL LLSkipList<DATA_TYPE, BINARY_DEPTH>::addData(const DATA_TYPE& data)
{
	S32			level;
	LLSkipNode	*current = &mHead;
	LLSkipNode	*temp;
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
		if (ll_frand() < 0.5f)
			break;
	}

	LLSkipNode *snode = new LLSkipNode(data);

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
inline BOOL LLSkipList<DATA_TYPE, BINARY_DEPTH>::checkData(const DATA_TYPE& data)
{
	S32			level;
	LLSkipNode	*current = &mHead;
	LLSkipNode	*temp;
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

	return FALSE;
}

// returns number of items in the list
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline S32 LLSkipList<DATA_TYPE, BINARY_DEPTH>::getLength() const
{
	U32	length = 0;
	for (LLSkipNode* temp = *(mHead.mForward); temp != NULL; temp = temp->mForward[0])
	{
		length++;
	}
	return length;
}


template <class DATA_TYPE, S32 BINARY_DEPTH>
inline BOOL LLSkipList<DATA_TYPE, BINARY_DEPTH>::moveData(const DATA_TYPE& data, LLSkipList *newlist)
{
	BOOL removed = removeData(data);
	BOOL added = newlist->addData(data);
	return removed && added;
}


template <class DATA_TYPE, S32 BINARY_DEPTH>
inline BOOL LLSkipList<DATA_TYPE, BINARY_DEPTH>::removeData(const DATA_TYPE& data)
{
	S32			level;
	LLSkipNode	*current = &mHead;
	LLSkipNode	*temp;
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

// remove all nodes from the list but do not delete data
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipList<DATA_TYPE, BINARY_DEPTH>::removeAllNodes()
{
	LLSkipNode *temp;
	// reset mCurrentp
	mCurrentp = *(mHead.mForward);

	while (mCurrentp)
	{
		temp = mCurrentp->mForward[0];
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
inline void LLSkipList<DATA_TYPE, BINARY_DEPTH>::resetList()
{
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}

// return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE LLSkipList<DATA_TYPE, BINARY_DEPTH>::getCurrentData()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mData;
	}
	else
	{
		//return NULL;		// causes compile warning
		return (DATA_TYPE)0; 			// equivalent, but no warning
	}
}

// same as getCurrentData() but a more intuitive name for the operation
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE LLSkipList<DATA_TYPE, BINARY_DEPTH>::getNextData()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mData;
	}
	else
	{
		//return NULL;		// causes compile warning
		return (DATA_TYPE)0; 			// equivalent, but no warning
	}
}

// remove the Node at mCurentOperatingp
// leave mCurrentp and mCurentOperatingp on the next entry
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline void LLSkipList<DATA_TYPE, BINARY_DEPTH>::removeCurrentData()
{
	if (mCurrentOperatingp)
	{
		removeData(mCurrentOperatingp->mData);
	}
}

// reset the list and return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
template <class DATA_TYPE, S32 BINARY_DEPTH>
inline DATA_TYPE LLSkipList<DATA_TYPE, BINARY_DEPTH>::getFirstData()
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
		return (DATA_TYPE)0; 			// equivalent, but no warning
	}
}


#endif
