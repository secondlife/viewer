/** 
 * @file llstringtable.h
 * @brief The LLStringTable class provides a _fast_ method for finding
 * unique copies of strings.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_STRING_TABLE_H
#define LL_STRING_TABLE_H

#include "lldefs.h"
#include "llformat.h"
#include "llstl.h"
#include <list>
#include <set>

#if LL_WINDOWS
# if (_MSC_VER >= 1300 && _MSC_VER < 1400)
#  define STRING_TABLE_HASH_MAP 1
# endif
#else
//# define STRING_TABLE_HASH_MAP 1
#endif

#if STRING_TABLE_HASH_MAP
#include <hash_map>
#endif

const U32 MAX_STRINGS_LENGTH = 256;

class LLStringTableEntry
{
public:
	LLStringTableEntry(const char *str)
		: mString(NULL), mCount(1)
	{
		// Copy string
		U32 length = (U32)strlen(str) + 1;	 /*Flawfinder: ignore*/
		length = llmin(length, MAX_STRINGS_LENGTH);
		mString = new char[length];
		strncpy(mString, str, length);	 /*Flawfinder: ignore*/
		mString[length - 1] = 0;
	}
	~LLStringTableEntry()
	{
		delete [] mString;
		mCount = 0;
	}
	void incCount()		{ mCount++; }
	BOOL decCount()		{ return --mCount; }

	char *mString;
	S32  mCount;
};

class LLStringTable
{
public:
	LLStringTable(int tablesize);
	~LLStringTable();

	char *checkString(const char *str);
	char *checkString(const std::string& str);
	LLStringTableEntry *checkStringEntry(const char *str);
	LLStringTableEntry *checkStringEntry(const std::string& str);

	char *addString(const char *str);
	char *addString(const std::string& str);
	LLStringTableEntry *addStringEntry(const char *str);
	LLStringTableEntry *addStringEntry(const std::string& str);
	void  removeString(const char *str);

	S32 mMaxEntries;
	S32 mUniqueEntries;
	
#if STRING_TABLE_HASH_MAP
	typedef std::hash_multimap<U32, LLStringTableEntry *> string_hash_t;
	string_hash_t mStringHash;
#else
	typedef std::list<LLStringTableEntry *> string_list_t;
	typedef string_list_t * string_list_ptr_t;
	string_list_ptr_t	*mStringList;
#endif	
};

extern LLStringTable gStringTable;

//============================================================================

// This class is designed to be used locally,
// e.g. as a member of an LLXmlTree
// Strings can be inserted only, then quickly looked up

typedef const std::string* LLStdStringHandle;

class LLStdStringTable
{
public:
	LLStdStringTable(S32 tablesize = 0)
	{
		if (tablesize == 0)
		{
			tablesize = 256; // default
		}
		// Make sure tablesize is power of 2
		for (S32 i = 31; i>0; i--)
		{
			if (tablesize & (1<<i))
			{
				if (tablesize >= (3<<(i-1)))
					tablesize = (1<<(i+1));
				else
					tablesize = (1<<i);
				break;
			}
		}
		mTableSize = tablesize;
		mStringList = new string_set_t[tablesize];
	}
	~LLStdStringTable()
	{
		cleanup();
		delete[] mStringList;
	}
	void cleanup()
	{
		// remove strings
		for (S32 i = 0; i<mTableSize; i++)
		{
			string_set_t& stringset = mStringList[i];
			for (string_set_t::iterator iter = stringset.begin(); iter != stringset.end(); iter++)
			{
				delete *iter;
			}
			stringset.clear();
		}
	}

	LLStdStringHandle lookup(const std::string& s)
	{
		U32 hashval = makehash(s);
		return lookup(hashval, s);
	}
	
	LLStdStringHandle checkString(const std::string& s)
	{
		U32 hashval = makehash(s);
		return lookup(hashval, s);
	}

	LLStdStringHandle insert(const std::string& s)
	{
		U32 hashval = makehash(s);
		LLStdStringHandle result = lookup(hashval, s);
		if (result == NULL)
		{
			result = new std::string(s);
			mStringList[hashval].insert(result);
		}
		return result;
	}
	LLStdStringHandle addString(const std::string& s)
	{
		return insert(s);
	}
	
private:
	U32 makehash(const std::string& s)
	{
		S32 len = (S32)s.size();
		const char* c = s.c_str();
		U32 hashval = 0;
		for (S32 i=0; i<len; i++)
		{
			hashval = ((hashval<<5) + hashval) + *c++;
		}
		return hashval & (mTableSize-1);
	}
	LLStdStringHandle lookup(U32 hashval, const std::string& s)
	{
		string_set_t& stringset = mStringList[hashval];
		LLStdStringHandle handle = &s;
		string_set_t::iterator iter = stringset.find(handle); // compares actual strings
		if (iter != stringset.end())
		{
			return *iter;
		}
		else
		{
			return NULL;
		}
	}
	
private:
	S32 mTableSize;
	typedef std::set<LLStdStringHandle, compare_pointer_contents<std::string> > string_set_t;
	string_set_t* mStringList; // [mTableSize]
};


#endif
