/**
 * @file llautoreplace.cpp
 * @brief Auto Replace Manager
 *
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
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llautoreplace.h"
#include "llsdserialize.h"
#include "llboost.h"
#include "llcontrol.h"
#include "llviewercontrol.h"
#include "llnotificationsutil.h"

LLAutoReplace* LLAutoReplace::sInstance;

const char* LLAutoReplace::SETTINGS_FILE_NAME = "settings_autoreplace.xml";

LLAutoReplace::LLAutoReplace()
{
}

LLAutoReplace::~LLAutoReplace()
{
	sInstance = NULL;
}

void LLAutoReplace::autoreplaceCallback(LLUIString& inputText, S32& cursorPos)
{
	static LLCachedControl<bool> perform_autoreplace(gSavedSettings, "AutoReplace");
	if(perform_autoreplace)
	{
		S32 wordStart = 0;
		S32 wordEnd = cursorPos-1;

		if(wordEnd < 1)
		{
			return;
		}

		LLWString text = inputText.getWString();
		if(text.size()<1)
		{
			return;
		}

		if(LLWStringUtil::isPartOfWord(text[wordEnd]))
		{
			return;//we only check on word breaks
		}

		wordEnd--;
		if ( LLWStringUtil::isPartOfWord(text[wordEnd]) )
		{
			while ((wordEnd > 0) && (' ' != text[wordEnd-1]))
			{
				wordEnd--;
			}

			wordStart=wordEnd;

			while ((wordEnd < (S32)text.length()) && (' ' != text[wordEnd]))
			{
				wordEnd++;
			}

			std::string strLastWord = std::string(text.begin(), text.end());
			std::string lastTypedWord = strLastWord.substr(wordStart, wordEnd-wordStart);
			std::string replacedWord( mSettings.replaceWord(lastTypedWord) );

			if(replacedWord != lastTypedWord)
			{
				LLWString strNew = utf8str_to_wstring(replacedWord);
				LLWString strOld = utf8str_to_wstring(lastTypedWord);
				int nDiff = strNew.size() - strOld.size();

				text.replace(wordStart,lastTypedWord.length(),strNew);
				inputText = wstring_to_utf8str(text);
				cursorPos+=nDiff;
			}
		}
	}
}

LLAutoReplace* LLAutoReplace::getInstance()
{
	if(!sInstance)
	{
		sInstance = new LLAutoReplace();
		sInstance->loadFromSettings();
	}
	return sInstance;
}

std::string LLAutoReplace::getUserSettingsFileName()
{
	std::string path=gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "");

	if (!path.empty())
	{
		path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, SETTINGS_FILE_NAME);
	}
	return path;
}

std::string LLAutoReplace::getAppSettingsFileName()
{
	std::string path=gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "");

	if (!path.empty())
	{
		path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, SETTINGS_FILE_NAME);
	}
	else
	{
		LL_ERRS("AutoReplace") << "Failed to get app settings directory name" << LL_ENDL;
	}
	return path;
}

LLAutoReplaceSettings LLAutoReplace::getSettings()
{
	return mSettings;
}

void LLAutoReplace::setSettings(const LLAutoReplaceSettings& newSettings)
{
	mSettings.set(newSettings);
}

void LLAutoReplace::loadFromSettings()
{
	std::string filename=getUserSettingsFileName();
	if (filename.empty())
	{
		LL_INFOS("AutoReplace") << "no valid user settings directory." << LL_ENDL;
	}
	if(gDirUtilp->fileExists(filename))
	{
		LLSD userSettings;
		llifstream file;
		file.open(filename.c_str());
		if (file.is_open())
		{
			LLSDSerialize::fromXML(userSettings, file);
		}
		file.close();
		if ( mSettings.setFromLLSD(userSettings) )
		{
			LL_INFOS("AutoReplace") << "settings loaded from '" << filename << "'" << LL_ENDL;
		}
		else
		{
			LL_WARNS("AutoReplace") << "invalid settings found in '" << filename << "'" << LL_ENDL;
		}
	}
	else // no user settings found, try application settings
	{
		std::string defaultName = getAppSettingsFileName();
		LL_INFOS("AutoReplace") << " user settings file '" << filename << "' not found;\n"
								<< " trying '" << defaultName.c_str() << "'"
								<< LL_ENDL;

		if(gDirUtilp->fileExists(defaultName))
		{
			LLSD appDefault;
			llifstream file;
			file.open(defaultName.c_str());
			if (file.is_open())
			{
				LLSDSerialize::fromXMLDocument(appDefault, file);
			}
			file.close();

			if ( mSettings.setFromLLSD(appDefault) )
			{
				LL_INFOS("AutoReplace") << "settings loaded from '" << filename << "'" << LL_ENDL;
				saveToUserSettings(appDefault);
			}
			else
			{
				LL_WARNS("AutoReplace") << "invalid settings found in '" << filename << "'" << LL_ENDL;
			}
		}
		else
		{
			if (mSettings.setFromLLSD(mSettings.getExampleLLSD()))
			{
				LL_WARNS("AutoReplace") << "no settings found; loaded example." << LL_ENDL;
			}
			else
			{
				LL_WARNS("AutoReplace") << "no settings found and example invalid!" << LL_ENDL;
			}
		}
	}
}

void LLAutoReplace::saveToUserSettings(const LLSD& newSettings)
{
	std::string filename=getUserSettingsFileName();
	llofstream file;
	file.open(filename.c_str());
	LLSDSerialize::toPrettyXML(newSettings, file);
	file.close();
	LL_INFOS("AutoReplace") << "settings saved to '" << filename << "'" << LL_ENDL;
}

// ================================================================
// LLAutoReplaceSettings
// ================================================================

const std::string LLAutoReplaceSettings::AUTOREPLACE_LIST_NAME         = "name";    	 ///< key for looking up list names
const std::string LLAutoReplaceSettings::AUTOREPLACE_LIST_REPLACEMENTS = "replacements"; ///< key for looking up replacement map

LLAutoReplaceSettings::LLAutoReplaceSettings()
{
}

LLAutoReplaceSettings::LLAutoReplaceSettings(const LLAutoReplaceSettings& settings)
{
	// copy all values through fundamental type intermediates for thread safety
	mLists = LLSD::emptyArray();

	for ( LLSD::array_const_iterator list = settings.mLists.beginArray(), listEnd = settings.mLists.endArray();
		  list != listEnd;
		  list++
		 )
	{
		LLSD listMap = LLSD::emptyMap();
		std::string listName = (*list)[AUTOREPLACE_LIST_NAME];
		listMap[AUTOREPLACE_LIST_NAME] = listName;
		listMap[AUTOREPLACE_LIST_REPLACEMENTS] = LLSD::emptyMap();

		for ( LLSD::map_const_iterator
				  entry = (*list)[AUTOREPLACE_LIST_REPLACEMENTS].beginMap(),
				  entriesEnd = (*list)[AUTOREPLACE_LIST_REPLACEMENTS].endMap();
			  entry != entriesEnd;
			  entry++
			 )
		{
			std::string keyword = entry->first;
			std::string replacement = entry->second.asString();
			listMap[AUTOREPLACE_LIST_REPLACEMENTS].insert(keyword, LLSD(replacement));
		}

		mLists.append(listMap);
	}
}

void LLAutoReplaceSettings::set(const LLAutoReplaceSettings& newSettings)
{
	mLists = newSettings.mLists;
}

bool LLAutoReplaceSettings::setFromLLSD(const LLSD& settingsFromLLSD)
{
	bool settingsValid = true;

	if ( settingsFromLLSD.isArray() )
	{
		for ( LLSD::array_const_iterator
				  list = settingsFromLLSD.beginArray(),
				  listEnd = settingsFromLLSD.endArray();
			  settingsValid && list != listEnd;
			  list++
			 )
		{
			settingsValid = listIsValid(*list);
		}
	}
	else
	{
		settingsValid = false;
		LL_WARNS("AutoReplace") << "settings are not an array" << LL_ENDL;
	}

	if ( settingsValid )
	{
		mLists = settingsFromLLSD;
	}
	else
	{
		LL_WARNS("AutoReplace") << "invalid settings discarded; using hard coded example" << LL_ENDL;
	}

	return settingsValid;
}

bool LLAutoReplaceSettings::listNameMatches( const LLSD& list, const std::string name )
{
	return list.isMap()
		&& list.has(AUTOREPLACE_LIST_NAME)
		&& list[AUTOREPLACE_LIST_NAME].asString() == name;
}

const LLSD* LLAutoReplaceSettings::getListEntries(std::string listName)
{
	const LLSD* returnedEntries = NULL;
	for( LLSD::array_const_iterator list = mLists.beginArray(), endList = mLists.endArray();
		 returnedEntries == NULL && list != endList;
		 list++
		)
	{
		const LLSD& thisList = *list;
		if ( listNameMatches(thisList, listName) )
		{
			returnedEntries = &thisList[AUTOREPLACE_LIST_REPLACEMENTS];
		}
	}
	return returnedEntries;
}

std::string LLAutoReplaceSettings::replacementFor(std::string keyword, std::string listName)
{
	std::string replacement;
	bool foundList = false;
	for( LLSD::array_const_iterator list = mLists.beginArray(), endList = mLists.endArray();
		 ! foundList && list != endList;
		 list++
		)
	{
		const LLSD& thisList = *list;
		if ( listNameMatches(thisList, listName) )
		{
			foundList = true; // whether there is a replacement or not, we're done
			if (   thisList.isMap()
				&& thisList.has(AUTOREPLACE_LIST_REPLACEMENTS)
				&& thisList[AUTOREPLACE_LIST_REPLACEMENTS].has(keyword)
				)
			{
				replacement = thisList[AUTOREPLACE_LIST_REPLACEMENTS][keyword].asString();
				LL_DEBUGS("AutoReplace")<<"'"<<keyword<<"' -> '"<<replacement<<"'"<<LL_ENDL;
			}
		}
		if (!foundList)
		{
			LL_WARNS("AutoReplace")<<"failed to find list '"<<listName<<"'"<<LL_ENDL;
		}
	}
	if (replacement.empty())
	{
		LL_WARNS("AutoReplace")<<"failed to find '"<<keyword<<"'"<<LL_ENDL;
	}
	return replacement;
}

LLSD LLAutoReplaceSettings::getListNames()
{
	LLSD toReturn = LLSD::emptyArray();
	for( LLSD::array_const_iterator list = mLists.beginArray(), endList = mLists.endArray();
		 list != endList && (*list).isDefined(); // deleting entries leaves undefined items in the iterator 
		 list++
		)
	{
		const LLSD& thisList = *list;
		if ( thisList.isMap() )
		{
			if ( thisList.has(AUTOREPLACE_LIST_NAME) )
			{
				std::string name = thisList[AUTOREPLACE_LIST_NAME].asString();
				toReturn.append(LLSD(name));
			}
			else
			{
				LL_ERRS("AutoReplace") << "  ! MISSING "<<AUTOREPLACE_LIST_NAME<< LL_ENDL;
			}
		}
		else
		{
			LL_ERRS("AutoReplace") << "  ! not a map: "<<LLSD::typeString(thisList.type())<< LL_ENDL;
		}
	}
	return toReturn;
}

bool LLAutoReplaceSettings::listIsValid(const LLSD& list)
{
	bool listValid = true;
	if ( ! list.isMap() )
	{
		listValid = false;
		LL_WARNS("AutoReplace") << "list is not a map" << LL_ENDL;
	}
	else if (   ! list.has(AUTOREPLACE_LIST_NAME)
			 || ! list[AUTOREPLACE_LIST_NAME].isString()
			 || list[AUTOREPLACE_LIST_NAME].asString().empty()
			 )
	{
		listValid = false;
		LL_WARNS("AutoReplace")
			<< "list found without " << AUTOREPLACE_LIST_NAME
			<< " (or it is empty)"
			<< LL_ENDL;
	}
	else if ( ! list.has(AUTOREPLACE_LIST_REPLACEMENTS) || ! list[AUTOREPLACE_LIST_REPLACEMENTS].isMap() )
	{
		listValid = false;
		LL_WARNS("AutoReplace") << "list '" << list[AUTOREPLACE_LIST_NAME].asString() << "' without " << AUTOREPLACE_LIST_REPLACEMENTS << LL_ENDL;
	}
	else
	{
		for ( LLSD::map_const_iterator
				  entry = list[AUTOREPLACE_LIST_REPLACEMENTS].beginMap(),
				  entriesEnd = list[AUTOREPLACE_LIST_REPLACEMENTS].endMap();
			  listValid && entry != entriesEnd;
			  entry++
			 )
		{
			if ( ! entry->second.isString() )
			{
				listValid = false;
				LL_WARNS("AutoReplace")
					<< "non-string replacement value found in list '"
					<< list[AUTOREPLACE_LIST_NAME].asString() << "'"
					<< LL_ENDL;
			}
		}
	}

	return listValid;
}

const LLSD* LLAutoReplaceSettings::exportList(std::string listName)
{
	const LLSD* exportedList = NULL;
	for ( LLSD::array_const_iterator list = mLists.beginArray(), listEnd = mLists.endArray();
		  exportedList == NULL && list != listEnd;
		  list++
		 )
	{
		if ( listNameMatches(*list, listName) )
		{
			const LLSD& namedList = (*list);
			exportedList = &namedList;
		}
	}
	return exportedList;
}

bool LLAutoReplaceSettings::listNameIsUnique(const LLSD& newList)
{
	bool nameIsUnique = true;
	// this must always be called with a valid list, so it is safe to assume it has a name
	std::string newListName = newList[AUTOREPLACE_LIST_NAME].asString();
	for ( LLSD::array_const_iterator list = mLists.beginArray(), listEnd = mLists.endArray();
		  nameIsUnique && list != listEnd;
		  list++
		 )
	{
		if ( listNameMatches(*list, newListName) )
		{
			LL_WARNS("AutoReplace")<<"duplicate list name '"<<newListName<<"'"<<LL_ENDL;
			nameIsUnique = false;
		}
	}
	return nameIsUnique;
}

/* static */
void LLAutoReplaceSettings::createEmptyList(LLSD& emptyList)
{
	emptyList = LLSD::emptyMap();
	emptyList[AUTOREPLACE_LIST_NAME] = "Empty";
	emptyList[AUTOREPLACE_LIST_REPLACEMENTS] = LLSD::emptyMap();
}

/* static */
void LLAutoReplaceSettings::setListName(LLSD& list, const std::string& newName)
{
	list[AUTOREPLACE_LIST_NAME] = newName;
}

/* static */
std::string LLAutoReplaceSettings::getListName(LLSD& list)
{
	std::string name;
	if ( list.isMap() && list.has(AUTOREPLACE_LIST_NAME) && list[AUTOREPLACE_LIST_NAME].isString() )
	{
		name = list[AUTOREPLACE_LIST_NAME].asString();
	}
	return name;
}

LLAutoReplaceSettings::AddListResult LLAutoReplaceSettings::addList(const LLSD& newList)
{
	AddListResult result;
	if ( listIsValid( newList ) )
	{
		if ( listNameIsUnique( newList ) )
		{
			mLists.append(newList);
			result = AddListOk;
		}
		else
		{
			LL_WARNS("AutoReplace") << "attempt to add duplicate name" << LL_ENDL;
			result = AddListDuplicateName;
		}
	}
	else
	{
		LL_WARNS("AutoReplace") << "attempt to add invalid list" << LL_ENDL;
		result = AddListInvalidList;
	}
	return result;
}

bool LLAutoReplaceSettings::removeReplacementList(std::string listName)
{
	bool found = false;
	for( S32 index = 0; !found && mLists[index].isDefined(); index++ )
	{
		if( listNameMatches(mLists.get(index), listName) )
		{
			LL_DEBUGS("AutoReplace")<<"list '"<<listName<<"'"<<LL_ENDL;
			mLists.erase(index);
			found = true;
		}
	}
	return found;
}

/// Move the named list up in the priority order
bool LLAutoReplaceSettings::increaseListPriority(std::string listName)
{
	LL_DEBUGS("AutoReplace")<<listName<<LL_ENDL;
	bool found = false;
	S32 search_index;
	LLSD targetList;
	for ( search_index = 0, targetList = mLists[0];
		  !found && targetList.isDefined();
		  search_index += 1, targetList = mLists[search_index]
		 )
	{
		if ( listNameMatches( targetList, listName) )
		{
			LL_DEBUGS("AutoReplace")<<"found at index "<<search_index<<LL_ENDL;
			found = true;
			if (search_index > 0)
			{
				// copy the target to before the element preceding it
				mLists.insert(search_index-1,targetList);
				// delete the original, now duplicate, copy
				mLists.erase(search_index+1);
			}
			else
			{
				LL_WARNS("AutoReplace") << "attempted to move top list up" << LL_ENDL;
			}
		}
	}
	return found;
}

/// Move the named list down in the priority order
bool LLAutoReplaceSettings::decreaseListPriority(std::string listName)
{
	LL_DEBUGS("AutoReplace")<<listName<<LL_ENDL;
	S32 found_index = -1;
	S32 search_index;
	for ( search_index = 0;
		  found_index == -1 && mLists[search_index].isDefined();
		  search_index++
		 )
	{
		if ( listNameMatches( mLists[search_index], listName) )
		{
			LL_DEBUGS("AutoReplace")<<"found at index "<<search_index<<LL_ENDL;
			found_index = search_index;
		}
	}
	if (found_index != -1)
	{
		// move this list after the found_index from after it to before it
		if (mLists[found_index+1].isDefined())
		{
			// copy item to after the element that follows it
			mLists.insert(found_index+2, mLists[found_index]);
			// erase the original, now duplicate, copy
			mLists.erase(found_index);
		}
		else
		{
			LL_WARNS("AutoReplace") << "attempted to move bottom list down" << LL_ENDL;
		}
	}
	return (found_index != -1);
}


std::string LLAutoReplaceSettings::replaceWord(const std::string currentWord)
{
	std::string returnedWord = currentWord; // in case no replacement is found
	static LLCachedControl<bool> autoreplace_enabled(gSavedSettings, "AutoReplace");
	if ( autoreplace_enabled )
	{
		//loop through lists in order
		bool found = false;
		for( LLSD::array_const_iterator list = mLists.beginArray(), endLists = mLists.endArray();
			 ! found && list != endLists;
			 list++
			)
		{
			const LLSD& checkList = *list;
			const LLSD& replacements = checkList[AUTOREPLACE_LIST_REPLACEMENTS];

			if ( replacements.has(currentWord) )
			{
				found = true;
				LL_DEBUGS("AutoReplace")
					<< "found in list '" << checkList[AUTOREPLACE_LIST_NAME].asString() << "' : '"
					<< currentWord << "' => '" << replacements[currentWord].asString() << "'"
					<< LL_ENDL;
				returnedWord = replacements[currentWord].asString();
			}
		}
	}
	return returnedWord;
}

bool LLAutoReplaceSettings::addEntryToList(std::string keyword, std::string replacement, std::string listName)
{
	bool added = false;

	if ( ! keyword.empty() && ! replacement.empty() )
	{
		bool isOneWord = true;
		for (S32 character = 0; isOneWord && character < keyword.size(); character++ )
		{
			if ( ! LLWStringUtil::isPartOfWord(keyword[character]) )
			{
				LL_WARNS("AutoReplace") << "keyword '" << keyword << "' not a single word" << LL_ENDL;
				isOneWord = false;
			}
		}

		if ( isOneWord )
		{
			bool listFound = false;
			for( LLSD::array_iterator list = mLists.beginArray(), endLists = mLists.endArray();
				 ! listFound && list != endLists;
				 list++
				)
			{
				if ( listNameMatches(*list, listName) )
				{
					listFound = true;
					(*list)[AUTOREPLACE_LIST_REPLACEMENTS][keyword]=replacement;
				}
			}
			if (listFound)
			{
				added = true;
			}
			else
			{
				LL_WARNS("AutoReplace") << "list '" << listName << "' not found" << LL_ENDL;
			}
		}
	}

	return added;
}

bool LLAutoReplaceSettings::removeEntryFromList(std::string keyword, std::string listName)
{
	bool found = false;
	for( LLSD::array_iterator list = mLists.beginArray(), endLists = mLists.endArray();
		 ! found && list != endLists;
		 list++
		)
	{
		if ( listNameMatches(*list, listName) )
		{
			found = true;
			(*list)[AUTOREPLACE_LIST_REPLACEMENTS].erase(keyword);
		}
	}
	if (!found)
	{
		LL_WARNS("AutoReplace") << "list '" << listName << "' not found" << LL_ENDL;
	}
	return found;
}

LLSD LLAutoReplaceSettings::getExampleLLSD()
{
	LL_DEBUGS("AutoReplace")<<LL_ENDL;
	LLSD example = LLSD::emptyArray();

	example[0] = LLSD::emptyMap();
	example[0][AUTOREPLACE_LIST_NAME]    = "Example List 1";
	example[0][AUTOREPLACE_LIST_REPLACEMENTS]    = LLSD::emptyMap();
	example[0][AUTOREPLACE_LIST_REPLACEMENTS]["keyword1"] = "replacement string 1";
	example[0][AUTOREPLACE_LIST_REPLACEMENTS]["keyword2"] = "replacement string 2";

	example[1] = LLSD::emptyMap();
	example[1][AUTOREPLACE_LIST_NAME]    = "Example List 2";
	example[1][AUTOREPLACE_LIST_REPLACEMENTS]    = LLSD::emptyMap();
	example[1][AUTOREPLACE_LIST_REPLACEMENTS]["mistake1"] = "correction 1";
	example[1][AUTOREPLACE_LIST_REPLACEMENTS]["mistake2"] = "correction 2";

	return example;
}

LLAutoReplaceSettings::~LLAutoReplaceSettings()
{
}
