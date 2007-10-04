/** 
 * @file llmap.h
 * @brief LLMap class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLMAP_H
#define LL_LLMAP_H

// llmap uses the fast stl library code in a manner consistant with LLSkipMap, et. al. 

template<class INDEX_TYPE, class MAPPED_TYPE> class LLMap
{
private:
	typedef typename std::map<INDEX_TYPE, MAPPED_TYPE> stl_map_t;
	typedef typename stl_map_t::iterator stl_iter_t;
	typedef typename stl_map_t::value_type stl_value_t;
	
	stl_map_t mStlMap;
	stl_iter_t mCurIter;	// *iterator = pair<const INDEX_TYPE, MAPPED_TYPE>
	MAPPED_TYPE dummy_data;
	INDEX_TYPE dummy_index;
	
public:
	LLMap() : mStlMap()
	{
		memset((void*)(&dummy_data),  0x0, sizeof(MAPPED_TYPE));
		memset((void*)(&dummy_index), 0x0, sizeof(INDEX_TYPE));
		mCurIter = mStlMap.begin();
	}
	~LLMap()
	{
		mStlMap.clear();
	}

	// use these functions to itterate through a list
	void resetMap()
	{
		mCurIter = mStlMap.begin();
	}

	// get the current data and bump mCurrentp
	// This is kind of screwy since it returns a reference;
	//   We have to have a dummy value for when we reach the end
	//   or in case we have an empty list. Presumably, this value
	//   will initialize to some NULL value that will end the iterator.
	// We really shouldn't be using getNextData() or getNextKey() anyway...
	MAPPED_TYPE &getNextData()
	{
		if (mCurIter == mStlMap.end())
		{
			return dummy_data;
		}
		else
		{
			return (*mCurIter++).second;
		}
	}

	const INDEX_TYPE &getNextKey()
	{
		if (mCurIter == mStlMap.end())
		{
			return dummy_index;
		}
		else
		{
			return (*mCurIter++).first;
		}
	}

	MAPPED_TYPE &getFirstData()
	{
		resetMap();
		return getNextData();
	}

	const INDEX_TYPE &getFirstKey()
	{
		resetMap();
		return getNextKey();
	}

	S32 getLength()
	{
		return mStlMap.size();
	}

	void addData(const INDEX_TYPE &index, MAPPED_TYPE pointed_to)
	{
		mStlMap.insert(stl_value_t(index, pointed_to));
	}

	void addData(const INDEX_TYPE &index)
	{
		mStlMap.insert(stl_value_t(index, dummy_data));
	}

	// if index doesn't exist, then insert a new node and return it
	MAPPED_TYPE &getData(const INDEX_TYPE &index)
	{
		std::pair<stl_iter_t, bool> res;
		res = mStlMap.insert(stl_value_t(index, dummy_data));
		return res.first->second;
	}

	// if index doesn't exist, then insert a new node, return it, and set b_new_entry to true
	MAPPED_TYPE &getData(const INDEX_TYPE &index, BOOL &b_new_entry)
	{
		std::pair<stl_iter_t, bool> res;
		res = mStlMap.insert(stl_value_t(index, dummy_data));
		b_new_entry = res.second;
		return res.first->second;
	}

	// If there, returns the data.
	// If not, returns NULL.
	// Never adds entries to the map.
	MAPPED_TYPE getIfThere(const INDEX_TYPE &index)
	{
		stl_iter_t iter;
		iter = mStlMap.find(index);
		if (iter == mStlMap.end())
		{
			return (MAPPED_TYPE)0;
		}
		else
		{
			return (*iter).second;
		}
	}


	// if index doesn't exist, then make a new node and return it
	MAPPED_TYPE &operator[](const INDEX_TYPE &index)
	{
		return getData(index);
	}

	// do a reverse look-up, return NULL if failed
	INDEX_TYPE reverseLookup(const MAPPED_TYPE data)
	{
		stl_iter_t iter;
		stl_iter_t end_iter;
		iter = mStlMap.begin();
		end_iter = mStlMap.end();
		while (iter != end_iter)
		{
			if ((*iter).second == data)
				return (*iter).first;
			iter++;
		}
		return (INDEX_TYPE)0;
	}

	BOOL removeData(const INDEX_TYPE &index)
	{
		mCurIter = mStlMap.find(index);
		if (mCurIter == mStlMap.end())
		{
			return FALSE;
		}
		else
		{
			stl_iter_t iter = mCurIter++; // incrament mCurIter to the next element
			mStlMap.erase(iter);
			return TRUE;
		}
	}

	// does this index exist?
	BOOL checkData(const INDEX_TYPE &index)
	{
		stl_iter_t iter;
		iter = mStlMap.find(index);
		if (iter == mStlMap.end())
		{
			return FALSE;
		}
		else
		{
			mCurIter = iter;
			return TRUE;
		}
	}

	BOOL deleteData(const INDEX_TYPE &index)
	{
		mCurIter = mStlMap.find(index);
		if (mCurIter == mStlMap.end())
		{
			return FALSE;
		}
		else
		{
			stl_iter_t iter = mCurIter++; // incrament mCurIter to the next element
			delete (*iter).second;
			mStlMap.erase(iter);
			return TRUE;
		}
	}

	void deleteAllData()
	{
		stl_iter_t iter;
		stl_iter_t end_iter;
		iter = mStlMap.begin();
		end_iter = mStlMap.end();
		while (iter != end_iter)
		{
			delete (*iter).second;
			iter++;
		}
		mStlMap.clear();
		mCurIter = mStlMap.end();
	}
	
	void removeAllData()
	{
		mStlMap.clear();
	}
};


#endif
