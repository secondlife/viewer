/**
 * @file llwldaycycle.cpp
 * @brief Implementation for the LLWLDayCycle class.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "llwldaycycle.h"
#include "llsdserialize.h"
#include "llwlparammanager.h"
#include "llfloaterdaycycle.h"
#include "llnotifications.h"

#include "llviewerwindow.h"

#include <map>

LLWLDayCycle::LLWLDayCycle() : mDayRate(120)
{
}


LLWLDayCycle::~LLWLDayCycle()
{
}

void LLWLDayCycle::loadDayCycle(const LLSD& day_data, LLWLParamKey::EScope scope)
{
	lldebugs << "Loading day cycle (day_data.size() = " << day_data.size() << ", scope = " << scope << ")" << llendl;
	mTimeMap.clear();

	// add each key frame
	for(S32 i = 0; i < day_data.size(); ++i)
	{
		// make sure it's a two array
		if(day_data[i].size() != 2)
		{
			continue;
		}
		
		// check each param key exists in param manager
		bool success;
		LLWLParamSet pset;
		LLWLParamKey frame = LLWLParamKey(day_data[i][1].asString(), scope);
		success =
			LLWLParamManager::getInstance()->getParamSet(frame, pset);
		if(!success)
		{
			// *HACK try the local-scope ones for "A-something" defaults
			// (because our envManager.lindenDefault() doesn't have the skies yet)
			if (frame.name.find("A-") == 0)
			{
				frame.scope = LLEnvKey::SCOPE_LOCAL;
				success = LLWLParamManager::getInstance()->getParamSet(frame, pset);
			}

			if (!success)
			{
				// alert the user
				LLSD args;
				args["SKY"] = day_data[i][1].asString();
				LLNotifications::instance().add("WLMissingSky", LLSD(), args);
				continue;
			}
		}
		
		// then add the keyframe
		addKeyframe((F32)day_data[i][0].asReal(), frame);
	}
}

void LLWLDayCycle::loadDayCycleFromFile(const std::string & fileName)
{
	loadDayCycle(loadCycleDataFromFile(fileName), LLWLParamKey::SCOPE_LOCAL);
}

/*static*/ LLSD LLWLDayCycle::loadCycleDataFromFile(const std::string & fileName)
{
	// now load the file
	std::string pathName(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, 
		"windlight/days", fileName));
	llinfos << "Loading DayCycle settings from " << pathName << llendl;
	
	llifstream day_cycle_xml(pathName);
	if (day_cycle_xml.is_open())
	{
		// load and parse it
		LLSD day_data(LLSD::emptyArray());
		LLPointer<LLSDParser> parser = new LLSDXMLParser();
		parser->parse(day_cycle_xml, day_data, LLSDSerialize::SIZE_UNLIMITED);
		day_cycle_xml.close();
		return day_data;
	}
	else
	{
		return LLSD();
	}
}

void LLWLDayCycle::saveDayCycle(const std::string & fileName)
{
	LLSD day_data = asLLSD();

	std::string pathName(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight/days", fileName));
	//llinfos << "Saving WindLight settings to " << pathName << llendl;

	llofstream day_cycle_xml(pathName);
	LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
	formatter->format(day_data, day_cycle_xml, LLSDFormatter::OPTIONS_PRETTY);
	day_cycle_xml.close();
}

LLSD LLWLDayCycle::asLLSD()
{
	LLSD day_data(LLSD::emptyArray());
	for(std::map<F32, LLWLParamKey>::const_iterator mIt = mTimeMap.begin(); mIt != mTimeMap.end(); ++mIt) 
	{
		LLSD key(LLSD::emptyArray());
		key.append(mIt->first);
		key.append(mIt->second.name);
		day_data.append(key);
	}

	LL_DEBUGS("Windlight Sync") << "Dumping day cycle (" << mTimeMap.size() << ") to LLSD: " << day_data << LL_ENDL;
	return day_data;
}

void LLWLDayCycle::clearKeyframes()
{
	lldebugs << "Clearing key frames" << llendl;
	mTimeMap.clear();
}


bool LLWLDayCycle::addKeyframe(F32 newTime, LLWLParamKey frame)
{
	// no adding negative time
	if(newTime < 0) 
	{
		newTime = 0;
	}

	// if time not being used, add it and return true
	if(mTimeMap.find(newTime) == mTimeMap.end()) 
	{
		mTimeMap.insert(std::pair<F32, LLWLParamKey>(newTime, frame));
		lldebugs << "Adding key frame (" << newTime << ", " << frame.toLLSD() << ")" << llendl;
		return true;
	}

	// otherwise, don't add, and return error
	llwarns << "Error adding key frame (" << newTime << ", " << frame.toLLSD() << ")" << llendl;
	return false;
}

bool LLWLDayCycle::changeKeyframeTime(F32 oldTime, F32 newTime)
{
	lldebugs << "Changing key frame time (" << oldTime << " => " << newTime << ")" << llendl;

	// just remove and add back
	LLWLParamKey frame = mTimeMap[oldTime];

	bool stat = removeKeyframe(oldTime);
	if(stat == false) 
	{
		lldebugs << "Failed to change key frame time (" << oldTime << " => " << newTime << ")" << llendl;
		return stat;
	}

	return addKeyframe(newTime, frame);
}

bool LLWLDayCycle::changeKeyframeParam(F32 time, LLWLParamKey key)
{
	lldebugs << "Changing key frame param (" << time << ", " << key.toLLSD() << ")" << llendl;

	// just remove and add back
	// make sure param exists
	LLWLParamSet tmp;
	bool stat = LLWLParamManager::getInstance()->getParamSet(key, tmp);
	if(stat == false) 
	{
		lldebugs << "Failed to change key frame param (" << time << ", " << key.toLLSD() << ")" << llendl;
		return stat;
	}

	mTimeMap[time] = key;
	return true;
}


bool LLWLDayCycle::removeKeyframe(F32 time)
{
	lldebugs << "Removing key frame (" << time << ")" << llendl;

	// look for the time.  If there, erase it
	std::map<F32, LLWLParamKey>::iterator mIt = mTimeMap.find(time);
	if(mIt != mTimeMap.end()) 
	{
		mTimeMap.erase(mIt);
		return true;
	}

	return false;
}

bool LLWLDayCycle::getKeytime(LLWLParamKey frame, F32& key_time)
{
	// scroll through till we find the correct value in the map
	std::map<F32, LLWLParamKey>::iterator mIt = mTimeMap.begin();
	for(; mIt != mTimeMap.end(); ++mIt) 
	{
		if(frame == mIt->second) 
		{
			key_time = mIt->first;
			return true;
		}
	}

	return false;
}

bool LLWLDayCycle::getKeyedParam(F32 time, LLWLParamSet& param)
{
	// just scroll on through till you find it
	std::map<F32, LLWLParamKey>::iterator mIt = mTimeMap.find(time);
	if(mIt != mTimeMap.end())
	{
		return LLWLParamManager::getInstance()->getParamSet(mIt->second, param);
	}

	// return error if not found
	lldebugs << "Key " << time << " not found" << llendl;
	return false;
}

bool LLWLDayCycle::getKeyedParamName(F32 time, std::string & name)
{
	// just scroll on through till you find it
	std::map<F32, LLWLParamKey>::iterator mIt = mTimeMap.find(time);
	if(mIt != mTimeMap.end()) 
	{
		name = mTimeMap[time].name;
		return true;
	}

	// return error if not found
	lldebugs << "Key " << time << " not found" << llendl;
	return false;
}

void LLWLDayCycle::removeReferencesTo(const LLWLParamKey& keyframe)
{
	lldebugs << "Removing references to key frame " << keyframe.toLLSD() << llendl;
	F32 keytime;
	bool might_exist;
	do 
	{
		// look for it
		might_exist = getKeytime(keyframe, keytime);
		if(!might_exist)
		{
			return;
		}
		might_exist = removeKeyframe(keytime);

	} while(might_exist); // might be another one
}
