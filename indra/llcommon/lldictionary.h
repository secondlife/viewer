/** 
 * @file lldictionary.h
 * @brief Lldictionary class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLDICTIONARY_H
#define LL_LLDICTIONARY_H

#include <map>
#include <string>

#include "llerror.h"

struct LL_COMMON_API LLDictionaryEntry
{
	LLDictionaryEntry(const std::string &name);
	virtual ~LLDictionaryEntry() {}
	const std::string mName;
	std::string mNameCapitalized;
};

template <class Index, class Entry>
class LLDictionary : public std::map<Index, Entry *>
{
public:
	typedef std::map<Index, Entry *> map_t;
	typedef typename map_t::iterator iterator_t;
	typedef typename map_t::const_iterator const_iterator_t;
	
	LLDictionary() {}
	virtual ~LLDictionary()
	{
		for (iterator_t iter = map_t::begin(); iter != map_t::end(); ++iter)
			delete (iter->second);
	}

	const Entry *lookup(Index index) const
	{
		const_iterator_t dictionary_iter = map_t::find(index);
		if (dictionary_iter == map_t::end()) return NULL;
		return dictionary_iter->second;
	}
	const Index lookup(const std::string &name) const 
	{
		for (const_iterator_t dictionary_iter = map_t::begin();
			 dictionary_iter != map_t::end();
			 dictionary_iter++)
		{
			const Entry *entry = dictionary_iter->second;
			if (entry->mName == name)
			{
				return dictionary_iter->first;
			}
		}
		return notFound();
	}

protected:
	virtual Index notFound() const
	{
		// default is to assert
		// don't assert -- makes it impossible to work on mesh-development and viewer-development simultaneously
		//			-- davep 2010.10.29
		//llassert(false);
		return Index(-1);
	}
	void addEntry(Index index, Entry *entry)
	{
		if (lookup(index))
		{
			llerrs << "Dictionary entry already added (attempted to add duplicate entry)" << llendl;
		}
		(*this)[index] = entry;
	}
};

#endif // LL_LLDICTIONARY_H
