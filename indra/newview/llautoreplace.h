/** 
 * @file llautoreplace.h
 * @brief Auto Replace Manager
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef LLAUTOREPLACE_H
#define LLAUTOREPLACE_H

#include "lllineeditor.h"

class LLAutoReplace;

/** The configuration data for the LLAutoReplace object
 *
 * This is a separate class so that the LLFloaterAutoReplaceSettings
 * can have a copy of the configuration to manipulate before committing
 * the changes back to the LLAutoReplace singleton that provides the 
 * autoreplace callback.
 */
class LLAutoReplaceSettings
{
  public:
	LLAutoReplaceSettings();
	~LLAutoReplaceSettings();

	/// Constructor for creating a tempory copy of the current settings
	LLAutoReplaceSettings(const LLAutoReplaceSettings& settings);

	/// Replace the current settings with new ones and save them to the user settings file
	void set(const LLAutoReplaceSettings& newSettings);
	
	/// Load the current settings read from an LLSD file
	bool setFromLLSD(const LLSD& settingsFromLLSD);
	///< @returns whether or not the settingsFromLLSD were valid
	
	// ================================================================
	///@{ @name List Operations
	// ================================================================

	/// @returns the configured list names as an LLSD Array of strings
	LLSD getListNames();

	/// Status values returned from the addList method
	typedef enum
	{
		AddListOk,
		AddListDuplicateName,
		AddListInvalidList,
	} AddListResult;
	
	/// Inserts a new list at the end of the priority order
	AddListResult addList(const LLSD& newList);

	/// Removes the named list, @returns false if not found
	bool removeReplacementList(std::string listName);

	/// Move the named list up in the priority order 
	bool increaseListPriority(std::string listName);
	///< @returns false if the list is not found

	/// Move the named list down in the priority order 
	bool decreaseListPriority(std::string listName);
	///< @returns false if the list is not found

	/// Get a copy of just one list (for saving to an export file)
	const LLSD* exportList(std::string listName);
	/// @returns an LLSD map 

	/// Checks for required elements, and that each has the correct type.
	bool listIsValid(const LLSD& listSettings);
	
	/// Checks for required elements, and that each has the correct type.
	bool listNameIs(const LLSD& listSettings);
	
	/// Checks to see if a new lists name conflicts with one in the settings
	bool listNameIsUnique(const LLSD& newList);
	/// @note must be called with LLSD that has passed listIsValid
	
	/// Initializes emptyList to an empty list named 'Empty'
	static void createEmptyList(LLSD& emptyList);

	/// Resets the name of a list to a new value
	static void setListName(LLSD& list, const std::string& newName);

	/// Gets the name of a list
	static std::string getListName(LLSD& list);

    ///@}
	// ================================================================
	///@{ @name Replacement Entry Operations
	// ================================================================
	
	/// Get the replacements specified by a given list
	const LLSD* getListEntries(std::string listName);
	///< @returns an LLSD Map of keyword -> replacement test pairs

	/// Get the replacement for the keyword from the specified list
	std::string replacementFor(std::string keyword, std::string listName);
	
	/// Adds a keywword/replacement pair to the named list
	bool addEntryToList(LLWString keyword, LLWString replacement, std::string listName);

	/// Removes the keywword and its replacement from the named list
	bool removeEntryFromList(std::string keyword, std::string listName);

	/** 
	 * Look for currentWord in the lists in order, returning any substitution found
	 * If no configured substitution is found, returns currentWord
	 */
	std::string replaceWord(const std::string currentWord /**< word to search for */ );

	/// Provides a hard-coded example of settings 
	LLSD getExampleLLSD();

	/// Get the actual settings as LLSD
	const LLSD& getAsLLSD();
	///< @note for use only in AutoReplace::saveToUserSettings
	
  private:
	/// Efficiently and safely compare list names 
	bool listNameMatches( const LLSD& list, const std::string name );

	/// The actual llsd data structure
	LLSD mLists;

	static const std::string AUTOREPLACE_LIST_NAME;    	 ///< key for looking up list names
	static const std::string AUTOREPLACE_LIST_REPLACEMENTS; ///< key for looking up replacement map
	
    /**<
	 * LLSD structure of the lists
	 * - The configuration is an array (mLists),
	 * - Each entry in the array is a replacement list
	 * - Each replacement list is a map with three keys:
	 * @verbatim  
	 *     "name"    	  String    the name of the list
	 *     "replacements" Map       keyword -> replacement pairs
	 * 
	 * <llsd>
	 *   <array> 
	 *     <map>
	 *       <key>name</key>    <string>List 1</string>
	 *       <key>data</key>
	 *         <map>
	 *           <key>keyword1</key>  <string>replacement1</string>
	 *           <key>keyword2</key>  <string>replacement2</string>
	 *    	   </map>
	 *     </map> 
	 *     <map>
	 *       <key>name</key>    <string>List 2</string>
	 *       <key>data</key>
	 *         <map>
	 *           <key>keyword1</key>  <string>replacement1</string>
	 *           <key>keyword2</key>  <string>replacement2</string>
	 *    	   </map>
	 *     </map>
	 *   </array> 
	 * </llsd>
	 * @endverbatim
	 */
};

/** Provides a facility to auto-replace text dynamically as it is entered.
 *
 * When the end of a word is detected (defined as any punctuation character,
 * or any whitespace except newline or return), the preceding word is used
 * as a lookup key in an ordered list of maps.  If a match is found in any
 * map, the keyword is replaced by the associated value from the map.
 *
 * See the autoreplaceCallback method for how to add autoreplace functionality
 * to a text entry tool.
 */
class LLAutoReplace : public LLSingleton<LLAutoReplace>
{
  public:
	LLAutoReplace();
	~LLAutoReplace();

	/// @return a pointer to the active instance
	static LLAutoReplace* getInstance();

	/// Callback that provides the hook for use in text entry methods
	void autoreplaceCallback(LLUIString& inputText, S32& cursorPos);

	/// Get a copy of the current settings
	LLAutoReplaceSettings getSettings();

	/// Commit new settings after making changes
	void setSettings(const LLAutoReplaceSettings& settings);

  private:
	friend class LLSingleton<LLAutoReplace>;
	static LLAutoReplace* sInstance; ///< the active settings instance

	LLAutoReplaceSettings mSettings; ///< configuration information
	
	/// Read settings from persistent storage
	void loadFromSettings();

	/// Make the newSettings active and write them to user storage
	void saveToUserSettings();

	/// Compute the user settings file name
	std::string getUserSettingsFileName();

	/// Compute the (read-ony) application settings file name
	std::string getAppSettingsFileName();

	/// basename for the settings files
	static const char* SETTINGS_FILE_NAME;
};

#endif /* LLAUTOREPLACE_H */
