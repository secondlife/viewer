/** 
 * @file llcriticaldamp.h
 * @brief A lightweight class that calculates critical damping constants once
 * per frame.  
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLCRITICALDAMP_H
#define LL_LLCRITICALDAMP_H

#include <map>

#include "llframetimer.h"

class LLCriticalDamp 
{
public:
	LLCriticalDamp();

	// MANIPULATORS
	static void updateInterpolants();

	// ACCESSORS
	static F32 getInterpolant(const F32 time_constant, BOOL use_cache = TRUE);

protected:	
	static LLFrameTimer sInternalTimer;	// frame timer for calculating deltas

	static std::map<F32, F32> 	sInterpolants;
	static F32					sTimeDelta;
};

#endif  // LL_LLCRITICALDAMP_H
