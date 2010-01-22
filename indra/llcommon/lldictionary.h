/** 
 * @file lldictionary.h
 * @brief Lldictionary class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLDICTIONARY_H
#define LL_LLDICTIONARY_H

#include <map>
#include <string>

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
		llwarns << "Lookup on " << name << " failed" << llendl;
		return Index(-1);
	}

protected:
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
