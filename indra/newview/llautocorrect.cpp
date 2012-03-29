/** 
 * @file llautocorrect.cpp
 * @brief Auto Correct Manager
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
#include "llautocorrect.h"
#include "llsdserialize.h"
#include "llboost.h"
#include "llcontrol.h"
#include "llviewercontrol.h"
#include "llnotificationsutil.h"

AutoCorrect* AutoCorrect::sInstance;

AutoCorrect::AutoCorrect()
{
	sInstance = this;
	sInstance->loadFromDisk();
}

AutoCorrect::~AutoCorrect()
{
	sInstance = NULL;
}

void AutoCorrect::autocorrectCallback(LLUIString& inputText, S32& cursorPos)
{
	static LLCachedControl<bool> perform_autocorrect(gSavedSettings, "AutoCorrect");
	if(perform_autocorrect)
	{
		S32 wordStart = 0;
		S32 wordEnd = cursorPos-1;

		if(wordEnd < 1)
			return;

		LLWString text = inputText.getWString();

		if(text.size()<1)
			return;

		if(LLWStringUtil::isPartOfWord(text[wordEnd]))
			return;//we only check on word breaks

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
			std::string correctedWord(replaceWord(lastTypedWord));

			if(correctedWord != lastTypedWord)
			{
				LLWString strNew = utf8str_to_wstring(correctedWord);
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

AutoCorrect* AutoCorrect::getInstance()
{
	if(sInstance)return sInstance;
	else
	{
		sInstance = new AutoCorrect();
		return sInstance;
	}
}
void AutoCorrect::save()
{
	saveToDisk(mAutoCorrects);
}
std::string AutoCorrect::getFileName()
{
	std::string path=gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "");

	if (!path.empty())
	{
		path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "settings_autocorrect.xml");
	}
	return path;  
}
std::string AutoCorrect::getDefaultFileName()
{
	std::string path=gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "");

	if (!path.empty())
	{
		path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "settings_autocorrect.xml");
	}
	return path;  
}
LLSD AutoCorrect::exportList(std::string listName)
{
	LLSD toReturn;
	if(mAutoCorrects.has(listName))
	{
		toReturn["listName"]=listName;
		toReturn["data"]=mAutoCorrects[listName]["data"];
		toReturn["author"]=mAutoCorrects[listName]["author"];
		toReturn["wordStyle"]=mAutoCorrects[listName]["wordStyle"];
		toReturn["priority"]=mAutoCorrects[listName]["priority"];
	}
	return toReturn;
}
BOOL AutoCorrect::addCorrectionList(LLSD newList)
{
	if(newList.has("listName"))
	{
		std::string name = newList["listName"];
		//if(!mAutoCorrects.has(name)){
		LLSD newPart;
		newPart["data"]=newList["data"];
		newPart["enabled"]=TRUE;
		newPart["announce"]=FALSE;
		newPart["author"]=newList["author"];
		newPart["wordStyle"]=newList["wordStyle"];
		newPart["priority"]=newList["priority"].asInteger();
		llinfos << "adding new list with settings priority "<<newPart["priority"].asInteger() <<llendl;
		mAutoCorrects[name]=newPart;

		return TRUE;

	}
	return FALSE;
}
BOOL AutoCorrect::removeCorrectionList(std::string listName)
{
	if(mAutoCorrects.has(listName))
	{
		mAutoCorrects.erase(listName);
		return TRUE;
	}
	return FALSE;
}
BOOL AutoCorrect::setListEnabled(std::string listName, BOOL enabled)
{
	if(mAutoCorrects.has(listName))
	{
		mAutoCorrects[listName]["enabled"]=enabled;
		return TRUE;
	}
	
	return FALSE;
}
BOOL AutoCorrect::setListAnnounceeState(std::string listName, BOOL announce)
{
	

	if(mAutoCorrects.has(listName))
	{
		mAutoCorrects[listName]["announce"]=announce;
		return TRUE;
	}
	return FALSE;
}
BOOL AutoCorrect::setListStyle(std::string listName, BOOL announce)
{
	if(mAutoCorrects.has(listName))
	{
		mAutoCorrects[listName]["wordStyle"]=announce;
		return TRUE;
	}
	return FALSE;
}
BOOL AutoCorrect::setListPriority(std::string listName, int priority)
{
	if(mAutoCorrects.has(listName))
	{
		mAutoCorrects[listName]["priority"]=priority;
		return TRUE;
	}
	return FALSE;
}
LLSD AutoCorrect::getAutoCorrects()
{
	//loadFromDisk();
	return mAutoCorrects;
}
void AutoCorrect::loadFromDisk()
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
		}else
		saveToDisk(getExampleLLSD());
	}
	else
	{
		llifstream file;
		file.open(filename.c_str());
		if (file.is_open())
		{
			LLSDSerialize::fromXML(mAutoCorrects, file);
		}
		file.close();
	}	
}
void AutoCorrect::saveToDisk(LLSD newSettings)
{
	mAutoCorrects=newSettings;
	std::string filename=getFileName();
	llofstream file;
	file.open(filename.c_str());
	LLSDSerialize::toPrettyXML(mAutoCorrects, file);
	file.close();
}
void AutoCorrect::runTest()
{
	std::string startS("He just abandonned all his abilties");
	std::string endS = replaceWords(startS);
	llinfos << "!!! Test of autoreplace; start with "<<startS.c_str() << " end with " << endS.c_str()<<llendl;


}
BOOL AutoCorrect::saveListToDisk(std::string listName, std::string fileName)
{
	if(mAutoCorrects.has(listName))
	{
		llofstream file;
		file.open(fileName.c_str());
		LLSDSerialize::toPrettyXML(exportList(listName), file);
		file.close();
		return TRUE;
	}
	return FALSE;
}
LLSD AutoCorrect::getAutoCorrectEntries(std::string listName)
{
	LLSD toReturn;
	if(mAutoCorrects.has(listName))
	{
		toReturn=mAutoCorrects[listName];
	}
	return toReturn;
}
std::string AutoCorrect::replaceWord(std::string currentWord)
{
	static LLCachedControl<bool> perform_autocorrect(gSavedSettings, "AutoCorrect");
	if(!(perform_autocorrect))return currentWord;
	//loop through priorities
	for(int currentPriority = 10;currentPriority>=0;currentPriority--)
	{
		LLSD::map_const_iterator loc_it = mAutoCorrects.beginMap();
		LLSD::map_const_iterator loc_end = mAutoCorrects.endMap();
		for (; loc_it != loc_end; ++loc_it)
		{
			const std::string& location = (*loc_it).first;
			//llinfos << "location is "<<location.c_str() << " word is "<<currentWord.c_str()<<llendl;
			const LLSD& loc_map = (*loc_it).second;
			if(loc_map["priority"].asInteger()==currentPriority)
			{
				if(!loc_map["wordStyle"].asBoolean())
				{
					//this means look for partial matches instead of a full word
					if(loc_map["enabled"].asBoolean())
					{
						LLSD::map_const_iterator inner_it = loc_map["data"].beginMap();
						LLSD::map_const_iterator inner_end = loc_map["data"].endMap();
						for (; inner_it != inner_end; ++inner_it)
						{
							const std::string& wrong = (*inner_it).first;
							const std::string& right = (*inner_it).second;
							int location = currentWord.find(wrong);
							if(location != std::string::npos)
							{
								currentWord=currentWord.replace(location,wrong.length(),right);
							}
						}
					}

				}else
				if((loc_map["data"].has(currentWord))&&(loc_map["enabled"].asBoolean()))
				{
					std::string replacement = loc_map["data"][currentWord];
					if(loc_map["announce"].asBoolean())
					{
						LLSD args; 
						//"[Before]" has been auto replaced by "[Replacement]"
						//	based on your [ListName] list.
						args["BEFORE"] = currentWord;
						args["LISTNAME"]=location;
						args["REPLACEMENT"]=replacement;
						LLNotificationsUtil::add("AutoReplace",args);
					}
					lldebugs << "found a word in list " << location.c_str() << " and it will replace  " << currentWord.c_str() << " => " << replacement.c_str() << llendl;
					return replacement;
				}
			}
		}
	}
	return currentWord;
}
std::string AutoCorrect::replaceWords(std::string words)
{
	static LLCachedControl<bool> perform_autocorrect(gSavedSettings, "AutoCorrect");
	if(!(perform_autocorrect))return words;
	//*TODO update this function to use the "wordStyle" thing,
	//but so far this function is never used, so later

	boost_tokenizer tokens(words, boost::char_separator<char>(" "));
	for (boost_tokenizer::iterator token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		std::string currentWord(*token_iter);
		LLSD::map_const_iterator loc_it = mAutoCorrects.beginMap();
		LLSD::map_const_iterator loc_end = mAutoCorrects.endMap();
		for (; loc_it != loc_end; ++loc_it)
		{
			const std::string& location = (*loc_it).first;
			const LLSD& loc_map = (*loc_it).second;
			if((loc_map["data"].has(currentWord))&&(loc_map["enabled"].asBoolean()))
			{
				std::string replacement = loc_map["data"][currentWord];
				if(loc_map["announce"].asBoolean())
				{
					LLSD args; 
					//"[Before]" has been auto replaced by "[Replacement]"
					//	based on your [ListName] list.
					args["BEFORE"] = currentWord;
					args["LISTNAME"]=location;
					args["REPLACEMENT"]=replacement;
					LLNotificationsUtil::add("AutoReplace",args);
				}
				lldebugs << "found a word in list " << location.c_str() << " and it will replace  " << currentWord.c_str() << " => " << replacement.c_str() << llendl;
				int wordStart = words.find(currentWord);
				words.replace(wordStart,currentWord.length(),replacement);
				return replaceWords(words);//lol recursion!
			}
		}		
	}
	return words;
}
BOOL AutoCorrect::addEntryToList(std::string wrong, std::string right, std::string listName)
{
	// *HACK: Make sure the "Custom" list exists, because the design of this
	// system prevents us from updating it by changing the original file...
	if(mAutoCorrects.has(listName))
	{
		mAutoCorrects[listName]["data"][wrong]=right;
		return TRUE;
	}
	else if(listName == "Custom")
	{
		mAutoCorrects[listName]["announce"] = 0;
		mAutoCorrects[listName]["author"] = "You";
		mAutoCorrects[listName]["data"][wrong] = right;
		mAutoCorrects[listName]["enabled"] = 1;
		mAutoCorrects[listName]["priority"] = 10;
		mAutoCorrects[listName]["wordStyle"] = 1;
		return TRUE;
	}
		
	return FALSE;
}
BOOL AutoCorrect::removeEntryFromList(std::string wrong, std::string listName)
{
	if(mAutoCorrects.has(listName))
	{
		if(mAutoCorrects[listName]["data"].has(wrong))
		{
			mAutoCorrects[listName]["data"].erase(wrong);
			return TRUE;
		}
	}
	return FALSE;
}

LLSD AutoCorrect::getExampleLLSD()
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

