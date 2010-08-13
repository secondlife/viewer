/**
 * @file llwlparammanager.h
 * @brief Implementation for the LLWLParamManager class.
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

#ifndef LL_WL_DAY_CYCLE_H
#define LL_WL_DAY_CYCLE_H

class LLWLDayCycle;

#include <vector>
#include <map>
#include <string>
#include "llwlparamset.h"
#include "llwlanimator.h"

class LLWLDayCycle
{
public:

	// lists what param sets are used when during the day
	std::map<F32, std::string> mTimeMap;

	// how long is my day
	F32 mDayRate;

public:

	/// simple constructor
	LLWLDayCycle();

	/// simple destructor
	~LLWLDayCycle();

	/// load a day cycle
	void loadDayCycle(const std::string & fileName);

	/// load a day cycle
	void saveDayCycle(const std::string & fileName);

	/// clear keys
	void clearKeys();

	/// Getters and Setters
	/// add a new key frame to the day cycle
	/// returns true if successful
	/// no negative time
	bool addKey(F32 newTime, const std::string & paramName);

	/// adjust a key's placement in the day cycle
	/// returns true if successful
	bool changeKeyTime(F32 oldTime, F32 newTime);

	/// adjust a key's parameter used
	/// returns true if successful
	bool changeKeyParam(F32 time, const std::string & paramName);

	/// remove a key from the day cycle
	/// returns true if successful
	bool removeKey(F32 time);

	/// get the first key time for a parameter
	/// returns false if not there
	bool getKey(const std::string & name, F32& key);

	/// get the param set at a given time
	/// returns true if found one
	bool getKeyedParam(F32 time, LLWLParamSet& param);

	/// get the name
	/// returns true if it found one
	bool getKeyedParamName(F32 time, std::string & name);

};


#endif
