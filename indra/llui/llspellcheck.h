/** 
 * @file llspellcheck.h
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

#ifndef LLSPELLCHECK_H
#define LLSPELLCHECK_H

#include "llsingleton.h"
#include "llui.h"
#include "llinitdestroyclass.h"
#include <boost/signals2.hpp>

class Hunspell;

class LLSpellChecker : public LLSingleton<LLSpellChecker>
{
    LLSINGLETON(LLSpellChecker);
    ~LLSpellChecker();

public:
    void addToCustomDictionary(const std::string& word);
    void addToIgnoreList(const std::string& word);
    bool checkSpelling(const std::string& word) const;
    S32  getSuggestions(const std::string& word, std::vector<std::string>& suggestions) const;
protected:
    void addToDictFile(const std::string& dict_path, const std::string& word);
    void initHunspell(const std::string& dict_language);
    void initSingleton();

public:
    typedef std::list<std::string> dict_list_t;

    const std::string&  getPrimaryDictionary() const { return mDictLanguage; }
    const dict_list_t&  getSecondaryDictionaries() const { return mDictSecondary; }
    bool                isActiveDictionary(const std::string& dict_language) const;
    void                setSecondaryDictionaries(dict_list_t dict_list);

    bool                     canRemoveDictionary(const std::string& dict_language);
    static const std::string getDictionaryAppPath();
    static const std::string getDictionaryUserPath();
    const LLSD               getDictionaryData(const std::string& dict_language);
    const LLSD&              getDictionaryMap() { return mDictMap; }
    static bool              getUseSpellCheck();
    bool                     hasDictionary(const std::string& dict_language, bool check_installed = false);
    void                     refreshDictionaryMap();
    void                     removeDictionary(const std::string& dict_language);
    static void              setUseSpellCheck(const std::string& dict_language);
protected:
    static LLSD              loadUserDictionaryMap();
    void                     setDictionaryData(const LLSD& dict_info);
    static void              saveUserDictionaryMap(const LLSD& dict_map);

public:
    typedef boost::signals2::signal<void()> settings_change_signal_t;
    static boost::signals2::connection setSettingsChangeCallback(const settings_change_signal_t::slot_type& cb);

protected:
    Hunspell*   mHunspell;
    std::string mDictLanguage;
    std::string mDictFile;
    dict_list_t mDictSecondary;
    std::vector<std::string> mIgnoreList;
    LLSD mDictMap;

    static settings_change_signal_t sSettingsChangeSignal;
};

#endif // LLSPELLCHECK_H
