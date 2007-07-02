/** 
 * @file llenum.h
 * @author Tom Yedwab
 * @brief Utility class for storing enum value <-> string lookup.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLENUM_H
#define LL_LLENUM_H

class LLEnum
{
public:
	typedef std::pair<const std::string, const U32> enum_t;
	enum
	{
		UNDEFINED = 0xffffffff,
	};

	LLEnum(const enum_t values_array[], const U32 length)
	{
		for (U32 i=0; i<length; ++i)
		{
			mEnumMap.insert(values_array[i]);
			if (values_array[i].second >= mEnumArray.size())
			{
				mEnumArray.resize(values_array[i].second+1);
			}
			mEnumArray[values_array[i].second] = values_array[i].first;
		}
	}

	U32 operator[](std::string str)
	{
		std::map<const std::string, const U32>::iterator itor;
		itor = mEnumMap.find(str);
		if (itor != mEnumMap.end())
		{
			return itor->second;
		}
		return UNDEFINED;
	}

	const std::string operator[](U32 index)
	{
		if (index < mEnumArray.size())
		{
			return mEnumArray[index];
		}
		return "";
	}

private:
	std::map<const std::string, const U32> mEnumMap;
	std::vector<std::string> mEnumArray;
};

#endif // LL_LLENUM_H
