/** 
 * @file llnametable.h
 * @brief LLNameTable class is a table to associate pointers with string names
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLNAMETABLE_H
#define LL_LLNAMETABLE_H

#include <map>

#include "string_table.h"

template <class DATA>
class LLNameTable
{
public:
	LLNameTable()
	:	mNameMap()
	{
	}

	~LLNameTable() 
	{ 
	}

	void addEntry(const std::string& name, DATA data)
	{
		addEntry(name.c_str(), data);
	}

	void addEntry(const char *name, DATA data)
	{
		char *tablename = gStringTable.addString(name);
		mNameMap[tablename] = data;
	}

	BOOL checkName(const std::string& name) const
	{
		return checkName(name.c_str());
	}

	// "logically const" even though it modifies the global nametable
	BOOL checkName(const char *name) const
	{
		char *tablename = gStringTable.addString(name);
		return mNameMap.count(tablename) ? TRUE : FALSE;
	}

	DATA resolveName(const std::string& name) const
	{
		return resolveName(name.c_str());
	}

	// "logically const" even though it modifies the global nametable
	DATA resolveName(const char *name) const
	{
		char *tablename = gStringTable.addString(name);
		const_iter_t iter = mNameMap.find(tablename);
		if (iter != mNameMap.end())
			return iter->second;
		else
			return 0;
	}

	// O(N)! (currently only used in one place... (newsim/llstate.cpp))
	const char *resolveData(const DATA &data) const
	{
		const_iter_t iter = mNameMap.begin();
		const_iter_t end = mNameMap.end();
		for (; iter != end; ++iter)
		{
			if (iter->second == data)
				return iter->first;
		}
   		return NULL;
	}		

	typedef std::map<const char *, DATA> name_map_t;
	typedef typename std::map<const char *,DATA>::iterator iter_t;
	typedef typename std::map<const char *,DATA>::const_iterator const_iter_t;
	name_map_t		mNameMap;
};

#endif
