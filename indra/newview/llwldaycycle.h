/**
 * @file llwlparammanager.h
 * @brief Implementation for the LLWLParamManager class.
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
