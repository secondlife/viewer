/** 
 * @file llautoreplace.h
 * @brief Auto Replace Manager
 * @copyright Copyright (c) 2011 LordGregGreg Back
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
#ifndef AUTO_REPLACE
#define AUTO_REPLACE

#include "lllineeditor.h"

class AutoReplace : public LLSingleton<AutoReplace>
{
	AutoReplace();
	~AutoReplace();
	static AutoReplace* sInstance;
public:
	void autoreplaceCallback(LLUIString& inputText, S32& cursorPos);
	static AutoReplace* getInstance();
	BOOL addReplacementList(LLSD newList);
	BOOL removeReplacementList(std::string listName);
	BOOL setListEnabled(std::string listName, BOOL enabled);
	BOOL setListAnnounceeState(std::string listName, BOOL announce);
	BOOL setListPriority(std::string listName, int priority);
	BOOL setListStyle(std::string listName, BOOL announce);
	std::string replaceWords(std::string words);
	std::string replaceWord(std::string currentWord);
	BOOL addEntryToList(std::string wrong, std::string right, std::string listName);
	BOOL removeEntryFromList(std::string wrong, std::string listName);
	BOOL saveListToDisk(std::string listName, std::string fileName);
	LLSD exportList(std::string listName);
	void runTest();
	LLSD getAutoReplaces();
	LLSD getAutoReplaceEntries(std::string listName);
	void save();

	void loadFromDisk();

private:
	friend class LLSingleton<AutoReplace>;
	void saveToDisk(LLSD newSettings);
	LLSD getExampleLLSD();	
	std::string getFileName();
	std::string getDefaultFileName();

	LLSD mAutoReplaces;

};



#endif 
