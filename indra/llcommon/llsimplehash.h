/** 
 * @file llsimplehash.h
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSIMPLEHASH_H
#define LL_LLSIMPLEHASH_H

#include "llstl.h"

template <typename HASH_KEY_TYPE>
class LLSimpleHashEntry
{
protected:
	HASH_KEY_TYPE mHashKey;
	LLSimpleHashEntry<HASH_KEY_TYPE>* mNextEntry;
public:
	LLSimpleHashEntry(HASH_KEY_TYPE key) :
		mHashKey(key),
		mNextEntry(0)
	{
	}
	virtual ~LLSimpleHashEntry()
	{
	}
	HASH_KEY_TYPE getHashKey() const
	{
		return mHashKey;
	}
	LLSimpleHashEntry<HASH_KEY_TYPE>* getNextEntry() const
	{
		return mNextEntry;
	}
	void setNextEntry(LLSimpleHashEntry<HASH_KEY_TYPE>* next)
	{
		mNextEntry = next;
	}
};

template <typename HASH_KEY_TYPE, int TABLE_SIZE>
class LLSimpleHash
{
public:
	LLSimpleHash()
	{
		llassert(TABLE_SIZE);
		llassert((TABLE_SIZE ^ (TABLE_SIZE-1)) == (TABLE_SIZE | (TABLE_SIZE-1))); // power of 2
		memset(mEntryTable, 0, sizeof(mEntryTable));
	}
	virtual ~LLSimpleHash()
	{
	}

	virtual int getIndex(HASH_KEY_TYPE key)
	{
		return key & (TABLE_SIZE-1);
	}
	
	bool insert(LLSimpleHashEntry<HASH_KEY_TYPE>* entry)
	{
		llassert(entry->getNextEntry() == 0);
		int index = getIndex(entry->getHashKey());
		entry->setNextEntry(mEntryTable[index]);
		mEntryTable[index] = entry;
		return true;
	}
	LLSimpleHashEntry<HASH_KEY_TYPE>* find(HASH_KEY_TYPE key)
	{
		int index = getIndex(key);
		LLSimpleHashEntry<HASH_KEY_TYPE>* res = mEntryTable[index];
		while(res && (res->getHashKey() != key))
		{
			res = res->getNextEntry();
		}
		return res;
	}
	bool erase(LLSimpleHashEntry<HASH_KEY_TYPE>* entry)
	{
		return erase(entry->getHashKey());
	}
	bool erase(HASH_KEY_TYPE key)
	{
		int index = getIndex(key);
		LLSimpleHashEntry<HASH_KEY_TYPE>* prev = 0;
		LLSimpleHashEntry<HASH_KEY_TYPE>* res = mEntryTable[index];
		while(res && (res->getHashKey() != key))
		{
			prev = res;
			res = res->getNextEntry();
		}
		if (res)
		{
			LLSimpleHashEntry<HASH_KEY_TYPE>* next = res->getNextEntry();
			if (prev)
			{
				prev->setNextEntry(next);
			}
			else
			{
				mEntryTable[index] = next;
			}
			return true;
		}
		else
		{
			return false;
		}
	}
	// Removes and returns an arbitrary ("first") element from the table
	// Used for deleting the entire table.
	LLSimpleHashEntry<HASH_KEY_TYPE>* pop_element()
	{
		for (int i=0; i<TABLE_SIZE; i++)
		{
			LLSimpleHashEntry<HASH_KEY_TYPE>* entry = mEntryTable[i];
			if (entry)
			{
				mEntryTable[i] = entry->getNextEntry();
				return entry;
			}
		}
		return 0;
	}
	// debugging
	LLSimpleHashEntry<HASH_KEY_TYPE>* get_element_at_index(S32 index) const
	{
		return mEntryTable[index];
	}
	
	
private:
	LLSimpleHashEntry<HASH_KEY_TYPE>* mEntryTable[TABLE_SIZE];
};

#endif // LL_LLSIMPLEHASH_H
