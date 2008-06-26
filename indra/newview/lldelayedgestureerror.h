/** 
 * @file lldelayedgestureerror.h
 * @brief Delayed gesture error message -- try to wait until name has been retrieved
 * @author Dale Glass
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
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


#ifndef LL_DELAYEDGESTUREERROR_H
#define LL_DELAYEDGESTUREERROR_H

#include <list>
#include "lltimer.h"

// TODO: Refactor to be more generic - this may be useful for other delayed notifications in the future

class LLDelayedGestureError
{
public:
	/**
	 * @brief Generates a missing gesture error
	 * @param id UUID of missing gesture
	 * Delays message for up to 5 seconds if UUID can't be immediately converted to a text description
	 */
	static void gestureMissing(const LLUUID &id);

	/**
	 * @brief Generates a gesture failed to load error
	 * @param id UUID of missing gesture
	 * Delays message for up to 5 seconds if UUID can't be immediately converted to a text description
	 */
	static void gestureFailedToLoad(const LLUUID &id);

private:
	

	struct LLErrorEntry
	{
		LLErrorEntry(const std::string& notify, const LLUUID &item) : mTimer(), mNotifyName(notify), mItemID(item) {}

		LLTimer mTimer;
		std::string mNotifyName;
		LLUUID mItemID;
	};


	static bool doDialog(const LLErrorEntry &ent, bool uuid_ok = false);
	static void enqueue(const LLErrorEntry &ent);
	static void onIdle(void *userdata);

	typedef std::list<LLErrorEntry> ErrorQueue;

	static ErrorQueue sQueue;
};


#endif
