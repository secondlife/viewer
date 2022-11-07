/**
 * @file llwldaycycle.cpp
 * @brief Implementation for the LLWLDayCycle class.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llwldaycycle.h"
#include "llsdserialize.h"
#include "llwlparammanager.h"
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
#if 0
    LL_DEBUGS() << "Loading day cycle (day_data.size() = " << day_data.size() << ", scope = " << scope << ")" << LL_ENDL;
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
            // *HACK: If loading region day cycle, try local sky presets as well.
            // Local presets may be referenced by a region day cycle after
            // it has been edited but the changes have not been uploaded.
            if (scope == LLEnvKey::SCOPE_REGION)
            {
                frame.scope = LLEnvKey::SCOPE_LOCAL;
                success = LLWLParamManager::getInstance()->getParamSet(frame, pset);
            }

            if (!success)
            {
                // alert the user
                LLSD args;
                args["SKY"] = day_data[i][1].asString();
                LLNotifications::instance().add("WLMissingSky", args, LLSD());
                continue;
            }
        }
        
        // then add the keyframe
        addKeyframe((F32)day_data[i][0].asReal(), frame);
    }
#endif
}

void LLWLDayCycle::loadDayCycleFromFile(const std::string & fileName)
{
    loadDayCycle(loadCycleDataFromFile(fileName), LLWLParamKey::SCOPE_LOCAL);
}

/*static*/ LLSD LLWLDayCycle::loadCycleDataFromFile(const std::string & fileName)
{
    // *FIX: Cannot load user day cycles.
    std::string pathName(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, 
        "windlight/days", fileName));

    return loadDayCycleFromPath(pathName);
}

// static
LLSD LLWLDayCycle::loadDayCycleFromPath(const std::string& file_path)
{
    LL_DEBUGS("Windlight") << "Loading DayCycle settings from " << file_path << LL_ENDL;
    
    llifstream day_cycle_xml(file_path.c_str());
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
    std::string pathName(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight/days", fileName));
    //LL_INFOS() << "Saving WindLight settings to " << pathName << LL_ENDL;

    save(pathName);
}

void LLWLDayCycle::save(const std::string& file_path)
{
    LLSD day_data = asLLSD();

    llofstream day_cycle_xml(file_path.c_str());
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

    LL_DEBUGS() << "Dumping day cycle (" << mTimeMap.size() << ") to LLSD: " << day_data << LL_ENDL;
    return day_data;
}

// bool LLWLDayCycle::getSkyRefs(std::map<LLWLParamKey, LLWLParamSet>& refs) const
// {
//  bool result = true;
//  LLWLParamManager& wl_mgr = LLWLParamManager::instance();
// 
//  refs.clear();
//  for (std::map<F32, LLWLParamKey>::const_iterator iter = mTimeMap.begin(); iter != mTimeMap.end(); ++iter)
//  {
//      const LLWLParamKey& key = iter->second;
//      if (!wl_mgr.getParamSet(key, refs[key]))
//      {
//          LL_WARNS() << "Cannot find sky [" << key.name << "] referenced by a day cycle" << LL_ENDL;
//          result = false;
//      }
//  }
// 
//  return result;
// }

bool LLWLDayCycle::getSkyMap(LLSD& sky_map) const
{
//  std::map<LLWLParamKey, LLWLParamSet> refs;
// 
//  if (!getSkyRefs(refs))
//  {
//      return false;
//  }
// 
//  sky_map = LLWLParamManager::createSkyMap(refs);
    return true;
}

void LLWLDayCycle::clearKeyframes()
{
    LL_DEBUGS() << "Clearing key frames" << LL_ENDL;
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
        LL_DEBUGS() << "Adding key frame (" << newTime << ", " << frame.toLLSD() << ")" << LL_ENDL;
        return true;
    }

    // otherwise, don't add, and return error
    LL_WARNS() << "Error adding key frame (" << newTime << ", " << frame.toLLSD() << ")" << LL_ENDL;
    return false;
}

bool LLWLDayCycle::changeKeyframeTime(F32 oldTime, F32 newTime)
{
    LL_DEBUGS() << "Changing key frame time (" << oldTime << " => " << newTime << ")" << LL_ENDL;

    // just remove and add back
    LLWLParamKey frame = mTimeMap[oldTime];

    bool stat = removeKeyframe(oldTime);
    if(stat == false) 
    {
        LL_DEBUGS() << "Failed to change key frame time (" << oldTime << " => " << newTime << ")" << LL_ENDL;
        return stat;
    }

    return addKeyframe(newTime, frame);
}

// bool LLWLDayCycle::changeKeyframeParam(F32 time, LLWLParamKey key)
// {
//  LL_DEBUGS() << "Changing key frame param (" << time << ", " << key.toLLSD() << ")" << LL_ENDL;
// 
//  // just remove and add back
//  // make sure param exists
//  LLWLParamSet tmp;
//  bool stat = LLWLParamManager::getInstance()->getParamSet(key, tmp);
//  if(stat == false) 
//  {
//      LL_DEBUGS() << "Failed to change key frame param (" << time << ", " << key.toLLSD() << ")" << LL_ENDL;
//      return stat;
//  }
// 
//  mTimeMap[time] = key;
//  return true;
// }


bool LLWLDayCycle::removeKeyframe(F32 time)
{
    LL_DEBUGS() << "Removing key frame (" << time << ")" << LL_ENDL;

    // look for the time.  If there, erase it
    std::map<F32, LLWLParamKey>::iterator mIt = mTimeMap.find(time);
    if(mIt != mTimeMap.end()) 
    {
        mTimeMap.erase(mIt);
        return true;
    }

    return false;
}

bool LLWLDayCycle::getKeytime(LLWLParamKey frame, F32& key_time) const
{
    // scroll through till we find the correct value in the map
    std::map<F32, LLWLParamKey>::const_iterator mIt = mTimeMap.begin();
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

// bool LLWLDayCycle::getKeyedParam(F32 time, LLWLParamSet& param)
// {
//  // just scroll on through till you find it
//  std::map<F32, LLWLParamKey>::iterator mIt = mTimeMap.find(time);
//  if(mIt != mTimeMap.end())
//  {
//      return LLWLParamManager::getInstance()->getParamSet(mIt->second, param);
//  }
// 
//  // return error if not found
//  LL_DEBUGS() << "Key " << time << " not found" << LL_ENDL;
//  return false;
// }

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
    LL_DEBUGS() << "Key " << time << " not found" << LL_ENDL;
    return false;
}

bool LLWLDayCycle::hasReferencesTo(const LLWLParamKey& keyframe) const
{
    F32 dummy;
    return getKeytime(keyframe, dummy);
}

void LLWLDayCycle::removeReferencesTo(const LLWLParamKey& keyframe)
{
    LL_DEBUGS() << "Removing references to key frame " << keyframe.toLLSD() << LL_ENDL;
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
