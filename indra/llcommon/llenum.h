/** 
 * @file llenum.h
 * @author Tom Yedwab
 * @brief Utility class for storing enum value <-> string lookup.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
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
