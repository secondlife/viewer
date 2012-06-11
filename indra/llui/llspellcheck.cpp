/** 
 * @file llspellcheck.cpp
 * @brief Spell checking functionality
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

#include "lldir.h"
#include "llsdserialize.h"

#include "llspellcheck.h"
#if LL_WINDOWS
	#include <hunspell/hunspelldll.h>
	#pragma comment(lib, "libhunspell.lib")
#else
	#include <hunspell/hunspell.hxx>
#endif

static const std::string DICT_DIR = "dictionaries";
static const std::string DICT_CUSTOM_SUFFIX = "_custom";
static const std::string DICT_IGNORE_SUFFIX = "_ignore";

LLSD LLSpellChecker::sDictMap;
LLSpellChecker::settings_change_signal_t LLSpellChecker::sSettingsChangeSignal;

LLSpellChecker::LLSpellChecker()
	: mHunspell(NULL)
{
	// Load initial dictionary information
	refreshDictionaryMap();
}

LLSpellChecker::~LLSpellChecker()
{
	delete mHunspell;
}

bool LLSpellChecker::checkSpelling(const std::string& word) const
{
	if ( (!mHunspell) || (word.length() < 3) || (0 != mHunspell->spell(word.c_str())) )
	{
		return true;
	}
	if (mIgnoreList.size() > 0)
	{
		std::string word_lower(word);
		LLStringUtil::toLower(word_lower);
		return (mIgnoreList.end() != std::find(mIgnoreList.begin(), mIgnoreList.end(), word_lower));
	}
	return false;
}

S32 LLSpellChecker::getSuggestions(const std::string& word, std::vector<std::string>& suggestions) const
{
	suggestions.clear();
	if ( (!mHunspell) || (word.length() < 3) )
	{
		return 0;
	}

	char** suggestion_list; int suggestion_cnt = 0;
	if ( (suggestion_cnt = mHunspell->suggest(&suggestion_list, word.c_str())) != 0 )
	{
		for (int suggestion_index = 0; suggestion_index < suggestion_cnt; suggestion_index++)
		{
			suggestions.push_back(suggestion_list[suggestion_index]);
		}
		mHunspell->free_list(&suggestion_list, suggestion_cnt);	
	}
	return suggestions.size();
}

// static
const LLSD LLSpellChecker::getDictionaryData(const std::string& dict_language)
{
	for (LLSD::array_const_iterator it = sDictMap.beginArray(); it != sDictMap.endArray(); ++it)
	{
		const LLSD& dict_entry = *it;
		if (dict_language == dict_entry["language"].asString())
		{
			return dict_entry;
		}
	}
	return LLSD();
}

// static
bool LLSpellChecker::hasDictionary(const std::string& dict_language, bool check_installed)
{
	const LLSD dict_info = getDictionaryData(dict_language);
	return dict_info.has("language") && ( (!check_installed) || (dict_info["installed"].asBoolean()) );
}

// static
void LLSpellChecker::setDictionaryData(const LLSD& dict_info)
{
	const std::string dict_language = dict_info["language"].asString();
	if (dict_language.empty())
	{
		return;
	}

	for (LLSD::array_iterator it = sDictMap.beginArray(); it != sDictMap.endArray(); ++it)
	{
		LLSD& dict_entry = *it;
		if (dict_language == dict_entry["language"].asString())
		{
			dict_entry = dict_info;
			return;
		}
	}
	sDictMap.append(dict_info);
	return;
}

// static
void LLSpellChecker::refreshDictionaryMap()
{
	const std::string app_path = getDictionaryAppPath();
	const std::string user_path = getDictionaryUserPath();

	// Load dictionary information (file name, friendly name, ...)
	llifstream user_file(user_path + "dictionaries.xml", std::ios::binary);
	if ( (!user_file.is_open()) || (0 == LLSDSerialize::fromXMLDocument(sDictMap, user_file)) || (0 == sDictMap.size()) )
	{
		llifstream app_file(app_path + "dictionaries.xml", std::ios::binary);
		if ( (!app_file.is_open()) || (0 == LLSDSerialize::fromXMLDocument(sDictMap, app_file)) || (0 == sDictMap.size()) )
		{
			return;
		}
	}

	// Load user installed dictionary information
	llifstream custom_file(user_path + "user_dictionaries.xml", std::ios::binary);
	if (custom_file.is_open())
	{
		LLSD custom_dict_map;
		LLSDSerialize::fromXMLDocument(custom_dict_map, custom_file);
		for (LLSD::array_iterator it = custom_dict_map.beginArray(); it != custom_dict_map.endArray(); ++it)
		{
			LLSD& dict_info = *it;
			dict_info["user_installed"] = true;
			setDictionaryData(dict_info);
		}
		custom_file.close();
	}

	// Look for installed dictionaries
	std::string tmp_app_path, tmp_user_path;
	for (LLSD::array_iterator it = sDictMap.beginArray(); it != sDictMap.endArray(); ++it)
	{
		LLSD& sdDict = *it;
		tmp_app_path = (sdDict.has("name")) ? app_path + sdDict["name"].asString() : LLStringUtil::null;
		tmp_user_path = (sdDict.has("name")) ? user_path + sdDict["name"].asString() : LLStringUtil::null;
		sdDict["installed"] = 
			(!tmp_app_path.empty()) && ((gDirUtilp->fileExists(tmp_user_path + ".dic")) || (gDirUtilp->fileExists(tmp_app_path + ".dic")));
		sdDict["has_custom"] = (!tmp_user_path.empty()) && (gDirUtilp->fileExists(tmp_user_path + DICT_CUSTOM_SUFFIX + ".dic"));
		sdDict["has_ignore"] = (!tmp_user_path.empty()) && (gDirUtilp->fileExists(tmp_user_path + DICT_IGNORE_SUFFIX + ".dic"));
	}

	sSettingsChangeSignal();
}

void LLSpellChecker::addToCustomDictionary(const std::string& word)
{
	if (mHunspell)
	{
		mHunspell->add(word.c_str());
	}
	addToDictFile(getDictionaryUserPath() + mDictFile + DICT_CUSTOM_SUFFIX + ".dic", word);
	sSettingsChangeSignal();
}

void LLSpellChecker::addToIgnoreList(const std::string& word)
{
	std::string word_lower(word);
	LLStringUtil::toLower(word_lower);
	if (mIgnoreList.end() == std::find(mIgnoreList.begin(), mIgnoreList.end(), word_lower))
	{
		mIgnoreList.push_back(word_lower);
		addToDictFile(getDictionaryUserPath() + mDictFile + DICT_IGNORE_SUFFIX + ".dic", word_lower);
		sSettingsChangeSignal();
	}
}

void LLSpellChecker::addToDictFile(const std::string& dict_path, const std::string& word)
{
	std::vector<std::string> word_list;

	if (gDirUtilp->fileExists(dict_path))
	{
		llifstream file_in(dict_path, std::ios::in);
		if (file_in.is_open())
		{
			std::string word; int line_num = 0;
			while (getline(file_in, word))
			{
				// Skip over the first line since that's just a line count
				if (0 != line_num)
				{
					word_list.push_back(word);
				}
				line_num++;
			}
		}
		else
		{
			// TODO: show error message?
			return;
		}
	}

	word_list.push_back(word);

	llofstream file_out(dict_path, std::ios::out | std::ios::trunc);	
	if (file_out.is_open())
	{
		file_out << word_list.size() << std::endl;
		for (std::vector<std::string>::const_iterator itWord = word_list.begin(); itWord != word_list.end(); ++itWord)
		{
			file_out << *itWord << std::endl;
		}
		file_out.close();
	}
}

void LLSpellChecker::setSecondaryDictionaries(dict_list_t dict_list)
{
	if (!getUseSpellCheck())
	{
		return;
	}

	// Check if we're only adding secondary dictionaries, or removing them
	dict_list_t dict_add(llmax(dict_list.size(), mDictSecondary.size())), dict_rem(llmax(dict_list.size(), mDictSecondary.size()));
	dict_list.sort();
	mDictSecondary.sort();
	dict_list_t::iterator end_added = std::set_difference(dict_list.begin(), dict_list.end(), mDictSecondary.begin(), mDictSecondary.end(), dict_add.begin());
	dict_list_t::iterator end_removed = std::set_difference(mDictSecondary.begin(), mDictSecondary.end(), dict_list.begin(), dict_list.end(), dict_rem.begin());

	if (end_removed != dict_rem.begin())		// We can't remove secondary dictionaries so we need to recreate the Hunspell instance
	{
		mDictSecondary = dict_list;

		std::string dict_name = mDictName;
		initHunspell(dict_name);
	}
	else if (end_added != dict_add.begin())		// Add the new secondary dictionaries one by one
	{
		const std::string app_path = getDictionaryAppPath();
		const std::string user_path = getDictionaryUserPath();
		for (dict_list_t::const_iterator it_added = dict_add.begin(); it_added != end_added; ++it_added)
		{
			const LLSD dict_entry = getDictionaryData(*it_added);
			if ( (!dict_entry.isDefined()) || (!dict_entry["installed"].asBoolean()) )
			{
				continue;
			}

			const std::string strFileDic = dict_entry["name"].asString() + ".dic";
			if (gDirUtilp->fileExists(user_path + strFileDic))
			{
				mHunspell->add_dic((user_path + strFileDic).c_str());
			}
			else if (gDirUtilp->fileExists(app_path + strFileDic))
			{
				mHunspell->add_dic((app_path + strFileDic).c_str());
			}
		}
		mDictSecondary = dict_list;
		sSettingsChangeSignal();
	}
}

void LLSpellChecker::initHunspell(const std::string& dict_name)
{
	if (mHunspell)
	{
		delete mHunspell;
		mHunspell = NULL;
		mDictName.clear();
		mDictFile.clear();
		mIgnoreList.clear();
	}

	const LLSD dict_entry = (!dict_name.empty()) ? getDictionaryData(dict_name) : LLSD();
	if ( (!dict_entry.isDefined()) || (!dict_entry["installed"].asBoolean()) || (!dict_entry["is_primary"].asBoolean()))
	{
		sSettingsChangeSignal();
		return;
	}

	const std::string app_path = getDictionaryAppPath();
	const std::string user_path = getDictionaryUserPath();
	if (dict_entry.has("name"))
	{
		const std::string filename_aff = dict_entry["name"].asString() + ".aff";
		const std::string filename_dic = dict_entry["name"].asString() + ".dic";
		if ( (gDirUtilp->fileExists(user_path + filename_aff)) && (gDirUtilp->fileExists(user_path + filename_dic)) )
		{
			mHunspell = new Hunspell((user_path + filename_aff).c_str(), (user_path + filename_dic).c_str());
		}
		else if ( (gDirUtilp->fileExists(app_path + filename_aff)) && (gDirUtilp->fileExists(app_path + filename_dic)) )
		{
			mHunspell = new Hunspell((app_path + filename_aff).c_str(), (app_path + filename_dic).c_str());
		}
		if (!mHunspell)
		{
			return;
		}

		mDictName = dict_name;
		mDictFile = dict_entry["name"].asString();

		if (dict_entry["has_custom"].asBoolean())
		{
			const std::string filename_dic = user_path + mDictFile + DICT_CUSTOM_SUFFIX + ".dic";
			mHunspell->add_dic(filename_dic.c_str());
		}

		if (dict_entry["has_ignore"].asBoolean())
		{
			llifstream file_in(user_path + mDictFile + DICT_IGNORE_SUFFIX + ".dic", std::ios::in);
			if (file_in.is_open())
			{
				std::string word; int idxLine = 0;
				while (getline(file_in, word))
				{
					// Skip over the first line since that's just a line count
					if (0 != idxLine)
					{
						LLStringUtil::toLower(word);
						mIgnoreList.push_back(word);
					}
					idxLine++;
				}
			}
		}

		for (dict_list_t::const_iterator it = mDictSecondary.begin(); it != mDictSecondary.end(); ++it)
		{
			const LLSD dict_entry = getDictionaryData(*it);
			if ( (!dict_entry.isDefined()) || (!dict_entry["installed"].asBoolean()) )
			{
				continue;
			}

			const std::string filename_dic = dict_entry["name"].asString() + ".dic";
			if (gDirUtilp->fileExists(user_path + filename_dic))
			{
				mHunspell->add_dic((user_path + filename_dic).c_str());
			}
			else if (gDirUtilp->fileExists(app_path + filename_dic))
			{
				mHunspell->add_dic((app_path + filename_dic).c_str());
			}
		}
	}

	sSettingsChangeSignal();
}

// static
const std::string LLSpellChecker::getDictionaryAppPath()
{
	std::string dict_path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, DICT_DIR, "");
	return dict_path;
}

// static
const std::string LLSpellChecker::getDictionaryUserPath()
{
	std::string dict_path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, DICT_DIR, "");
	if (!gDirUtilp->fileExists(dict_path))
	{
		LLFile::mkdir(dict_path);
	}
	return dict_path;
}

// static
bool LLSpellChecker::getUseSpellCheck()
{
	return (LLSpellChecker::instanceExists()) && (LLSpellChecker::instance().mHunspell);
}

// static
boost::signals2::connection LLSpellChecker::setSettingsChangeCallback(const settings_change_signal_t::slot_type& cb)
{
	return sSettingsChangeSignal.connect(cb);
}

// static
void LLSpellChecker::setUseSpellCheck(const std::string& dict_name)
{
	if ( (((dict_name.empty()) && (getUseSpellCheck())) || (!dict_name.empty())) && 
		 (LLSpellChecker::instance().mDictName != dict_name) )
	{
		LLSpellChecker::instance().initHunspell(dict_name);
	}
}

// static
void LLSpellChecker::initClass()
{
	if (sDictMap.isUndefined())
	{
		refreshDictionaryMap();
	}
}
