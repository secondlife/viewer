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
#include "llnotifications.h"
#include "llxmlnode.h"

#include <map>

LLWLDayCycle::LLWLDayCycle() : mDayRate(120)
{
}


LLWLDayCycle::~LLWLDayCycle()
{
}

void LLWLDayCycle::loadDayCycle(const std::string & fileName)
{
	// clear the first few things
	mTimeMap.clear();

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

		// add each key
		for(S32 i = 0; i < day_data.size(); ++i)
		{
			// make sure it's a two array
			if(day_data[i].size() != 2)
			{
				continue;
			}
			
			// check each param name exists in param manager
			bool success;
			LLWLParamSet pset;
			success = LLWLParamManager::instance()->getParamSet(day_data[i][1].asString(), pset);
			if(!success)
			{
				// alert the user
				LLSD args;
				args["SKY"] = day_data[i][1].asString();
				LLNotifications::instance().add("WLMissingSky", args);
				continue;
			}
			
			// then add the key
			addKey((F32)day_data[i][0].asReal(), day_data[i][1].asString());
		}

		day_cycle_xml.close();
	}
}

void LLWLDayCycle::saveDayCycle(const std::string & fileName)
{
	LLSD day_data(LLSD::emptyArray());

	std::string pathName(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight/days", fileName));
	//llinfos << "Saving WindLight settings to " << pathName << llendl;

	for(std::map<F32, std::string>::const_iterator mIt = mTimeMap.begin();
		mIt != mTimeMap.end();
		++mIt) 
	{
		LLSD key(LLSD::emptyArray());
		key.append(mIt->first);
		key.append(mIt->second);
		day_data.append(key);
	}

	llofstream day_cycle_xml(pathName);
	LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
	formatter->format(day_data, day_cycle_xml, LLSDFormatter::OPTIONS_PRETTY);
	day_cycle_xml.close();
}


void LLWLDayCycle::clearKeys()
{
	mTimeMap.clear();
}


bool LLWLDayCycle::addKey(F32 newTime, const std::string & paramName)
{
	// no adding negative time
	if(newTime < 0) 
	{
		newTime = 0;
	}

	// if time not being used, add it and return true
	if(mTimeMap.find(newTime) == mTimeMap.end()) 
	{
		mTimeMap.insert(std::pair<F32, std::string>(newTime, paramName));
		return true;
	}

	// otherwise, don't add, and return error
	return false;
}

bool LLWLDayCycle::changeKeyTime(F32 oldTime, F32 newTime)
{
	// just remove and add back
	std::string name = mTimeMap[oldTime];

	bool stat = removeKey(oldTime);
	if(stat == false) 
	{
		return stat;
	}

	return addKey(newTime, name);
}

bool LLWLDayCycle::changeKeyParam(F32 time, const std::string & name)
{
	// just remove and add back
	// make sure param exists
	LLWLParamSet tmp;
	bool stat = LLWLParamManager::instance()->getParamSet(name, tmp);
	if(stat == false) 
	{
		return stat;
	}

	mTimeMap[time] = name;
	return true;
}


bool LLWLDayCycle::removeKey(F32 time)
{
	// look for the time.  If there, erase it
	std::map<F32, std::string>::iterator mIt = mTimeMap.find(time);
	if(mIt != mTimeMap.end()) 
	{
		mTimeMap.erase(mIt);
		return true;
	}

	return false;
}

bool LLWLDayCycle::getKey(const std::string & name, F32& key)
{
	// scroll through till we find the 
	std::map<F32, std::string>::iterator mIt = mTimeMap.begin();
	for(; mIt != mTimeMap.end(); ++mIt) 
	{
		if(name == mIt->second) 
		{
			key = mIt->first;
			return true;
		}
	}

	return false;
}

bool LLWLDayCycle::getKeyedParam(F32 time, LLWLParamSet& param)
{
	// just scroll on through till you find it
	std::map<F32, std::string>::iterator mIt = mTimeMap.find(time);
	if(mIt != mTimeMap.end()) 
	{
		return LLWLParamManager::instance()->getParamSet(mIt->second, param);
	}

	// return error if not found
	return false;
}

bool LLWLDayCycle::getKeyedParamName(F32 time, std::string & name)
{
	// just scroll on through till you find it
	std::map<F32, std::string>::iterator mIt = mTimeMap.find(time);
	if(mIt != mTimeMap.end()) 
	{
		name = mTimeMap[time];
		return true;
	}

	// return error if not found
	return false;
}
