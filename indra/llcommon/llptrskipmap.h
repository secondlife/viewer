/** 
 * @file llptrskipmap.h
 * @brief Just like a LLSkipMap, but since it's pointers, you can call
 * deleteAllData
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#ifndef LL_LLPTRSKIPMAP_H
#define LL_LLPTRSKIPMAP_H

#include "llerror.h"
#include "llrand.h"

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH = 8> 
class LLPtrSkipMapNode
{
public:
	LLPtrSkipMapNode()
	{
		S32 i;
		for (i = 0; i < BINARY_DEPTH; i++)
		{
			mForward[i] = NULL;
		}

		U8  *zero = (U8 *)&mIndex;

		for (i = 0; i < (S32)sizeof(INDEX_T); i++)
		{
			*(zero + i) = 0;
		}

		zero = (U8 *)&mData;

		for (i = 0; i < (S32)sizeof(DATA_T); i++)
		{
			*(zero + i) = 0;
		}
	}

	LLPtrSkipMapNode(const INDEX_T &index)
	:	mIndex(index)
	{

		S32 i;
		for (i = 0; i < BINARY_DEPTH; i++)
		{
			mForward[i] = NULL;
		}

		U8 *zero = (U8 *)&mData;

		for (i = 0; i < (S32)sizeof(DATA_T); i++)
		{
			*(zero + i) = 0;
		}
	}

	LLPtrSkipMapNode(const INDEX_T &index, DATA_T datap)
	:	mIndex(index)
	{

		S32 i;
		for (i = 0; i < BINARY_DEPTH; i++)
		{
			mForward[i] = NULL;
		}

		mData = datap;
	}

	~LLPtrSkipMapNode()
	{
	}

	// delete associated data and NULLs out pointer
	void deleteData()
	{
		delete mData;
		mData = 0;
	}

	// NULLs out pointer
	void removeData()
	{
		mData = 0;
	}

	INDEX_T					mIndex;
	DATA_T					mData;
	LLPtrSkipMapNode				*mForward[BINARY_DEPTH];

private:
	// Disallow copying of LLPtrSkipMapNodes by not implementing these methods.
	LLPtrSkipMapNode(const LLPtrSkipMapNode &);
	LLPtrSkipMapNode &operator=(const LLPtrSkipMapNode &rhs);
};

//---------------------------------------------------------------------------

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH = 8> 
class LLPtrSkipMap
{
public:
	typedef BOOL (*compare)(const DATA_T& first, const DATA_T& second);
	typedef compare insert_func;
	typedef compare equals_func;

	void init();

	// basic constructor
	LLPtrSkipMap();

	// basic constructor including sorter
	LLPtrSkipMap(insert_func insert_first, equals_func equals);

	~LLPtrSkipMap();

	void setInsertFirst(insert_func insert_first);
	void setEquals(equals_func equals);

	DATA_T &addData(const INDEX_T &index, DATA_T datap);
	DATA_T &addData(const INDEX_T &index);
	DATA_T &getData(const INDEX_T &index);
	DATA_T &operator[](const INDEX_T &index);

	// If index present, returns data.
	// If index not present, adds <index,NULL> and returns NULL.
	DATA_T &getData(const INDEX_T &index, BOOL &b_new_entry);

	// returns data entry before and after index
	BOOL getInterval(const INDEX_T &index, INDEX_T &index_before, INDEX_T &index_after,
		DATA_T &data_before, DATA_T &data_after	);

	// Returns TRUE if data present in map.
	BOOL checkData(const INDEX_T &index);

	// Returns TRUE if key is present in map. This is useful if you
	// are potentially storing NULL pointers in the map
	BOOL checkKey(const INDEX_T &index);

	// If there, returns the data.
	// If not, returns NULL.
	// Never adds entries to the map.
	DATA_T getIfThere(const INDEX_T &index);

	INDEX_T reverseLookup(const DATA_T datap);

	// returns number of items in the list
	S32 getLength(); // WARNING!  getLength is O(n), not O(1)!

	BOOL removeData(const INDEX_T &index);
	BOOL deleteData(const INDEX_T &index);

	// remove all nodes from the list but do not delete data
	void removeAllData();
	void deleteAllData();

	// place mCurrentp on first node
	void resetList();

	// return the data currently pointed to
	DATA_T	getCurrentDataWithoutIncrement();

	// return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
	DATA_T	getCurrentData();

	// same as getCurrentData() but a more intuitive name for the operation
	DATA_T	getNextData();

	INDEX_T	getNextKey();

	// return the key currently pointed to
	INDEX_T	getCurrentKeyWithoutIncrement();

	// remove the Node at mCurentOperatingp
	// leave mCurrentp and mCurentOperatingp on the next entry
	void removeCurrentData();

	void deleteCurrentData();

	// reset the list and return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
	DATA_T	getFirstData();

	INDEX_T	getFirstKey();

	static BOOL	defaultEquals(const INDEX_T &first, const INDEX_T &second)
	{
		return first == second;
	}

private:
	// don't generate implicit copy constructor or copy assignment
	LLPtrSkipMap(const LLPtrSkipMap &);
	LLPtrSkipMap &operator=(const LLPtrSkipMap &);

private:
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	mHead;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*mUpdate[BINARY_DEPTH];
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*mCurrentp;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*mCurrentOperatingp;
	S32							mLevel;
	BOOL						(*mInsertFirst)(const INDEX_T &first, const INDEX_T &second);
	BOOL						(*mEquals)(const INDEX_T &first, const INDEX_T &second);
	S32							mNumberOfSteps;
};

//////////////////////////////////////////////////
//
// LLPtrSkipMap implementation
//
//

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::LLPtrSkipMap()
	:	mInsertFirst(NULL),
		mEquals(defaultEquals)
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

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::LLPtrSkipMap(insert_func insert_first, 
																 equals_func equals) 
:	mInsertFirst(insert_first),
	mEquals(equals)
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

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::~LLPtrSkipMap()
{
	removeAllData();
}

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline void LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::setInsertFirst(insert_func insert_first)
{
	mInsertFirst = insert_first;
}

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline void LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::setEquals(equals_func equals)
{
	mEquals = equals;
}

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline DATA_T &LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::addData(const INDEX_T &index, DATA_T datap)
{
	S32			level;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*current = &mHead;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*temp;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
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
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
			}
			*(mUpdate + level) = current;
		}
	}
	
	// we're now just in front of where we want to be . . . take one step forward
	current = *current->mForward;

	// replace the existing data if a node is already there
	if (  (current)
		&&(mEquals(current->mIndex, index)))
	{
		current->mData = datap;
		return current->mData;
	}

	// now add the new node
	S32 newlevel;
	for (newlevel = 1; newlevel <= mLevel && newlevel < BINARY_DEPTH; newlevel++)
	{
		if (frand(1.f) < 0.5f)
		{
			break;
		}
	}

	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH> *snode 
		= new LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>(index, datap);

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
	return snode->mData;
}

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline DATA_T &LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::addData(const INDEX_T &index)
{
	S32			level;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*current = &mHead;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*temp;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
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
				   &&(temp->mIndex < index))
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

	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH> *snode 
		= new LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>(index);

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
	return snode->mData;
}

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline DATA_T &LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::getData(const INDEX_T &index)
{
	S32			level;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*current = &mHead;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*temp;

	mNumberOfSteps = 0;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
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
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
			}
			*(mUpdate + level) = current;
		}
	}
	
	// we're now just in front of where we want to be . . . take one step forward
	current = *current->mForward;
	mNumberOfSteps++;

	if (  (current)
		&&(mEquals(current->mIndex, index)))
	{
		
		return current->mData;
	}
	
	// now add the new node
	S32 newlevel;
	for (newlevel = 1; newlevel <= mLevel && newlevel < BINARY_DEPTH; newlevel++)
	{
		if (frand(1.f) < 0.5f)
			break;
	}

	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH> *snode 
		= new LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>(index);

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
	
	return snode->mData;
}

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline BOOL LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::getInterval(const INDEX_T &index, 
																	 INDEX_T &index_before, 
																	 INDEX_T &index_after, 
																	 DATA_T &data_before, 
																	 DATA_T &data_after)
{
	S32			level;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*current = &mHead;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*temp;

	mNumberOfSteps = 0;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
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
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
			}
			*(mUpdate + level) = current;
		}
	}
	
	BOOL both_found = TRUE;

	if (current != &mHead)
	{
		index_before = current->mIndex;
		data_before = current->mData;
	}
	else
	{
		data_before = 0;
		index_before = 0;
		both_found = FALSE;
	}

	// we're now just in front of where we want to be . . . take one step forward
	mNumberOfSteps++;
	current = *current->mForward;

	if (current)
	{
		data_after = current->mData;
		index_after = current->mIndex;
	}
	else
	{
		data_after = 0;
		index_after = 0;
		both_found = FALSE;
	}

	return both_found;
}


template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline DATA_T &LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::operator[](const INDEX_T &index)
{
	
	return getData(index);
}

// If index present, returns data.
// If index not present, adds <index,NULL> and returns NULL.
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline DATA_T &LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::getData(const INDEX_T &index, BOOL &b_new_entry)
{
	S32			level;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*current = &mHead;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*temp;

	mNumberOfSteps = 0;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
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
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
			}
			*(mUpdate + level) = current;
		}
	}
	
	// we're now just in front of where we want to be . . . take one step forward
	mNumberOfSteps++;
	current = *current->mForward;

	if (  (current)
		&&(mEquals(current->mIndex, index)))
	{
		
		return current->mData;
	}
	b_new_entry = TRUE;
	addData(index);
	
	return current->mData;
}

// Returns TRUE if data present in map.
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline BOOL LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::checkData(const INDEX_T &index)
{
	S32			level;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*current = &mHead;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*temp;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
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
				   &&(temp->mIndex < index))
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
		// Gets rid of some compiler ambiguity for the LLPointer<> templated class.
		if (current->mData)
		{
			return mEquals(current->mIndex, index);
		}
	}

	return FALSE;
}

// Returns TRUE if key is present in map. This is useful if you
// are potentially storing NULL pointers in the map
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline BOOL LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::checkKey(const INDEX_T &index)
{
	S32			level;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*current = &mHead;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*temp;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
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
				   &&(temp->mIndex < index))
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
		return mEquals(current->mIndex, index);
	}

	return FALSE;
}

// If there, returns the data.
// If not, returns NULL.
// Never adds entries to the map.
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline DATA_T LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::getIfThere(const INDEX_T &index)
{
	S32			level;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*current = &mHead;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*temp;

	mNumberOfSteps = 0;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
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
				   &&(temp->mIndex < index))
			{
				current = temp;
				temp = *(current->mForward + level);
				mNumberOfSteps++;
			}
			*(mUpdate + level) = current;
		}
	}
	
	// we're now just in front of where we want to be . . . take one step forward
	mNumberOfSteps++;
	current = *current->mForward;

	if (current)
	{
		if (mEquals(current->mIndex, index))
		{
			return current->mData;
		}
	}

	// Avoid Linux compiler warning on returning NULL.
	return (DATA_T)0;
}

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline INDEX_T LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::reverseLookup(const DATA_T datap)
{
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*current = &mHead;

	while (current)
	{
		if (datap == current->mData)
		{
			
			return current->mIndex;
		}
		current = *current->mForward;
	}

	// not found! return NULL
	return INDEX_T();
}

// returns number of items in the list
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline S32 LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::getLength()
{
	U32	length = 0;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>* temp;
	for (temp = *(mHead.mForward); temp != NULL; temp = temp->mForward[0])
	{
		length++;
	}
	
	return length;
}

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline BOOL LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::removeData(const INDEX_T &index)
{
	S32			level;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*current = &mHead;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*temp;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
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
				   &&(temp->mIndex < index))
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
	if (!mEquals(current->mIndex, index))
	{
		// nope!
		
		return FALSE;
	}
	else
	{
		// do we need to fix current or currentop?
		if (current == mCurrentp)
		{
			mCurrentp = *current->mForward;
		}

		if (current == mCurrentOperatingp)
		{
			mCurrentOperatingp = *current->mForward;
		}
		// yes it is!  change pointers as required
		for (level = 0; level < mLevel; level++)
		{
			if (*((*(mUpdate + level))->mForward + level) != current)
			{
				// cool, we've fixed all the pointers!
				break;
			}
			*((*(mUpdate + level))->mForward + level) = *(current->mForward + level);
		}

		// clean up cuurent
		current->removeData();
		delete current;

		// clean up mHead
		while (  (mLevel > 1)
			   &&(!*(mHead.mForward + mLevel - 1)))
		{
			mLevel--;
		}
	}
	
	return TRUE;
}

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline BOOL LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::deleteData(const INDEX_T &index)
{
	S32			level;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*current = &mHead;
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH>	*temp;

	// find the pointer one in front of the one we want
	if (mInsertFirst)
	{
		for (level = mLevel - 1; level >= 0; level--)
		{
			temp = *(current->mForward + level);
			while (  (temp)
				   &&(mInsertFirst(temp->mIndex, index)))
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
				   &&(temp->mIndex < index))
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
	if (!mEquals(current->mIndex, index))
	{
		// nope!
		
		return FALSE;
	}
	else
	{
		// do we need to fix current or currentop?
		if (current == mCurrentp)
		{
			mCurrentp = *current->mForward;
		}

		if (current == mCurrentOperatingp)
		{
			mCurrentOperatingp = *current->mForward;
		}
		// yes it is!  change pointers as required
		for (level = 0; level < mLevel; level++)
		{
			if (*((*(mUpdate + level))->mForward + level) != current)
			{
				// cool, we've fixed all the pointers!
				break;
			}
			*((*(mUpdate + level))->mForward + level) = *(current->mForward + level);
		}

		// clean up cuurent
		current->deleteData();
		delete current;

		// clean up mHead
		while (  (mLevel > 1)
			   &&(!*(mHead.mForward + mLevel - 1)))
		{
			mLevel--;
		}
	}
	
	return TRUE;
}

// remove all nodes from the list but do not delete data
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
void LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::removeAllData()
{
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH> *temp;
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

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline void LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::deleteAllData()
{
	LLPtrSkipMapNode<INDEX_T, DATA_T, BINARY_DEPTH> *temp;
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
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline void LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::resetList()
{
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
}


// return the data currently pointed to
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline DATA_T LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::getCurrentDataWithoutIncrement()
{
	if (mCurrentOperatingp)
	{
		return mCurrentOperatingp->mData;
	}
	else
	{
		//return NULL; 		// causes warning
		return (DATA_T)0;			// equivalent, but no warning
	}
}

// return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline DATA_T LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::getCurrentData()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mData;
	}
	else
	{
		//return NULL; 		// causes warning
		return (DATA_T)0;			// equivalent, but no warning
	}
}

// same as getCurrentData() but a more intuitive name for the operation
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline DATA_T LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::getNextData()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mData;
	}
	else
	{
		//return NULL;	// causes compile warning
		return (DATA_T)0;		// equivalent, but removes warning
	}
}

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline INDEX_T LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::getNextKey()
{
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mIndex;
	}
	else
	{
		return mHead.mIndex;
	}
}

// return the key currently pointed to
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline INDEX_T LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::getCurrentKeyWithoutIncrement()
{
	if (mCurrentOperatingp)
	{
		return mCurrentOperatingp->mIndex;
	}
	else
	{
		//return NULL;	// causes compile warning
		return (INDEX_T)0;		// equivalent, but removes warning
	}
}


// remove the Node at mCurentOperatingp
// leave mCurrentp and mCurentOperatingp on the next entry
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline void LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::removeCurrentData()
{
	if (mCurrentOperatingp)
	{
		removeData(mCurrentOperatingp->mIndex);
	}
}

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline void LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::deleteCurrentData()
{
	if (mCurrentOperatingp)
	{
		deleteData(mCurrentOperatingp->mIndex);
	}
}

// reset the list and return the data currently pointed to, set mCurentOperatingp to that node and bump mCurrentp
template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline DATA_T LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::getFirstData()
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
		//return NULL;	// causes compile warning
		return (DATA_T)0;		// equivalent, but removes warning
	}
}

template <class INDEX_T, class DATA_T, S32 BINARY_DEPTH>
inline INDEX_T LLPtrSkipMap<INDEX_T, DATA_T, BINARY_DEPTH>::getFirstKey()
{
	mCurrentp = *(mHead.mForward);
	mCurrentOperatingp = *(mHead.mForward);
	if (mCurrentp)
	{
		mCurrentOperatingp = mCurrentp;
		mCurrentp = mCurrentp->mForward[0];
		return mCurrentOperatingp->mIndex;
	}
	else
	{
		return mHead.mIndex;
	}
}

#endif
