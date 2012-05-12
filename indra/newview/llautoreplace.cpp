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

LLAutoReplace::LLAutoReplace()
{
	sInstance = this;
	sInstance->loadFromDisk();
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

		if(LLWStringUtil::isPartOfWord(text[wordEnd]))
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
			std::string replacedWord(replaceWord(lastTypedWord));

			if(replacedWord != lastTypedWord)
			{
				LLWString strNew = utf8str_to_wstring(replacedWord);
				LLWString strOld = utf8str_to_wstring(lastTypedWord);
				int nDiff = strNew.size() - strOld.size();

				//int wordStart = regText.find(lastTypedWord);
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
	}
	return sInstance;
}

void LLAutoReplace::save()
{
	saveToDisk(mAutoReplaces);
}

std::string LLAutoReplace::getFileName()
{
	std::string path=gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "");

	if (!path.empty())
	{
		path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "settings_autoreplace.xml");
	}
	return path;  
}

std::string LLAutoReplace::getDefaultFileName()
{
	std::string path=gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "");

	if (!path.empty())
	{
		path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "settings_autoreplace.xml");
	}
	return path;  
}

LLSD LLAutoReplace::exportList(std::string listName)
{
	LLSD toReturn;
	if(mAutoReplaces.has(listName))
	{
		toReturn["listName"]=listName;
		toReturn["data"]=mAutoReplaces[listName]["data"];
		toReturn["priority"]=mAutoReplaces[listName]["priority"];
	}
	return toReturn;
}

BOOL LLAutoReplace::addReplacementList(LLSD newList)
{
	if(newList.has("listName"))
	{
		std::string name = newList["listName"];
		LLSD newPart;
		newPart["data"]=newList["data"];
		newPart["enabled"]=TRUE;
		newPart["priority"]=newList["priority"].asInteger();
		llinfos << "adding new list with settings priority "<<newPart["priority"].asInteger() <<llendl;
		mAutoReplaces[name]=newPart;

		return TRUE;
	}
	return FALSE;
}

BOOL LLAutoReplace::removeReplacementList(std::string listName)
{
	if(mAutoReplaces.has(listName))
	{
		mAutoReplaces.erase(listName);
		return TRUE;
	}
	return FALSE;
}

BOOL LLAutoReplace::setListEnabled(std::string listName, BOOL enabled)
{
	if(mAutoReplaces.has(listName))
	{
		mAutoReplaces[listName]["enabled"]=enabled;
		return TRUE;
	}
	
	return FALSE;
}

BOOL LLAutoReplace::setListPriority(std::string listName, int priority)
{
	if(mAutoReplaces.has(listName))
	{
		mAutoReplaces[listName]["priority"]=priority;
		return TRUE;
	}
	return FALSE;
}

LLSD LLAutoReplace::getAutoReplaces()
{
	return mAutoReplaces;
}

void LLAutoReplace::loadFromDisk()
{
	std::string filename=getFileName();
	if (filename.empty())
	{
		llinfos << "no valid user directory." << llendl; 
	}
	if(!gDirUtilp->fileExists(filename))
	{
		std::string defaultName = getDefaultFileName();
		llinfos << " user settings file doesnt exist, going to try and read default one from "<<defaultName.c_str()<< llendl;

		if(gDirUtilp->fileExists(defaultName))
		{
			LLSD blankllsd;
			llifstream file;
			file.open(defaultName.c_str());
			if (file.is_open())
			{
				LLSDSerialize::fromXMLDocument(blankllsd, file);
			}
			file.close();
			saveToDisk(blankllsd);
		}
		else
		{
			saveToDisk(getExampleLLSD());
		}
	}
	else
	{
		llifstream file;
		file.open(filename.c_str());
		if (file.is_open())
		{
			LLSDSerialize::fromXML(mAutoReplaces, file);
		}
		file.close();
	}	
}

void LLAutoReplace::saveToDisk(LLSD newSettings)
{
	mAutoReplaces=newSettings;
	std::string filename=getFileName();
	llofstream file;
	file.open(filename.c_str());
	LLSDSerialize::toPrettyXML(mAutoReplaces, file);
	file.close();
}

void LLAutoReplace::runTest()
{
	std::string startS("He just abandonned all his abilties");
	std::string endS = replaceWords(startS);
	llinfos << "!!! Test of autoreplace; start with "<<startS.c_str() << " end with " << endS.c_str()<<llendl;


}

BOOL LLAutoReplace::saveListToDisk(std::string listName, std::string fileName)
{
	if(mAutoReplaces.has(listName))
	{
		llofstream file;
		file.open(fileName.c_str());
		LLSDSerialize::toPrettyXML(exportList(listName), file);
		file.close();
		return TRUE;
	}
	return FALSE;
}

LLSD LLAutoReplace::getAutoReplaceEntries(std::string listName)
{
	LLSD toReturn;
	if(mAutoReplaces.has(listName))
	{
		toReturn=mAutoReplaces[listName];
	}
	return toReturn;
}

std::string LLAutoReplace::replaceWord(std::string currentWord)
{
	static LLCachedControl<bool> perform_autoreplace(gSavedSettings, "AutoReplace");
	if(!(perform_autoreplace))return currentWord;
	//loop through priorities
	for(int currentPriority = 10;currentPriority>=0;currentPriority--)
	{
		LLSD::map_const_iterator loc_it = mAutoReplaces.beginMap();
		LLSD::map_const_iterator loc_end = mAutoReplaces.endMap();
		for (; loc_it != loc_end; ++loc_it)
		{
			const std::string& location = (*loc_it).first;
			//llinfos << "location is "<<location.c_str() << " word is "<<currentWord.c_str()<<llendl;
			const LLSD& loc_map = (*loc_it).second;
			if(loc_map["priority"].asInteger()==currentPriority)
			{
				if((loc_map["data"].has(currentWord))&&(loc_map["enabled"].asBoolean()))
				{
					std::string replacement = loc_map["data"][currentWord];
					lldebugs << "found a word in list " << location.c_str() << " and it will replace  " << currentWord.c_str() << " => " << replacement.c_str() << llendl;
					return replacement;
				}
			}
		}
	}
	return currentWord;
}

std::string LLAutoReplace::replaceWords(std::string words)
{
	static LLCachedControl<bool> perform_autoreplace(gSavedSettings, "AutoReplace");
	if(!(perform_autoreplace))
	{
		return words;
	}
	
	boost_tokenizer tokens(words, boost::char_separator<char>(" "));
	for (boost_tokenizer::iterator token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		std::string currentWord(*token_iter);
		LLSD::map_const_iterator loc_it = mAutoReplaces.beginMap();
		LLSD::map_const_iterator loc_end = mAutoReplaces.endMap();
		for (; loc_it != loc_end; ++loc_it)
		{
			const std::string& location = (*loc_it).first;
			const LLSD& loc_map = (*loc_it).second;
			if((loc_map["data"].has(currentWord))&&(loc_map["enabled"].asBoolean()))
			{
				std::string replacement = loc_map["data"][currentWord];
				lldebugs << "found a word in list " << location.c_str() << " and it will replace  " << currentWord.c_str() << " => " << replacement.c_str() << llendl;
				int wordStart = words.find(currentWord);
				words.replace(wordStart,currentWord.length(),replacement);
				return replaceWords(words);//lol recursion!
			}
		}		
	}
	return words;
}

BOOL LLAutoReplace::addEntryToList(std::string wrong, std::string right, std::string listName)
{
	// *HACK: Make sure the "Custom" list exists, because the design of this
	// system prevents us from updating it by changing the original file...
	if(mAutoReplaces.has(listName))
	{
		mAutoReplaces[listName]["data"][wrong]=right;
		return TRUE;
	}
	else if(listName == "Custom")
	{
		mAutoReplaces[listName]["data"][wrong] = right;
		mAutoReplaces[listName]["enabled"] = 1;
		mAutoReplaces[listName]["priority"] = 10;
		return TRUE;
	}
		
	return FALSE;
}

BOOL LLAutoReplace::removeEntryFromList(std::string wrong, std::string listName)
{
	if(mAutoReplaces.has(listName))
	{
		if(mAutoReplaces[listName]["data"].has(wrong))
		{
			mAutoReplaces[listName]["data"].erase(wrong);
			return TRUE;
		}
	}
	return FALSE;
}

LLSD LLAutoReplace::getExampleLLSD()
{
	LLSD toReturn;

	LLSD listone;
	LLSD listtwo;

	LLSD itemOne;
	itemOne["wrong"]="wrong1";
	itemOne["right"]="right1";
	listone[0]=itemOne;

	LLSD itemTwo;
	itemTwo["wrong"]="wrong2";
	itemTwo["right"]="right2";
	listone[1]=itemTwo;

	toReturn["listOne"]=listone;


	itemOne["wrong"]="secondwrong1";
	itemOne["right"]="secondright1";
	listone[0]=itemOne;

	itemTwo["wrong"]="secondwrong2";
	itemTwo["right"]="secondright2";
	listone[1]=itemTwo;

	toReturn["listTwo"]=listone;	

	return toReturn;
}

