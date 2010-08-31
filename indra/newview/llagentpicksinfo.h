/** 
 * @file llagentpicksinfo.h
 * @brief LLAgentPicksInfo class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#ifndef LL_LLAGENTPICKS_H
#define LL_LLAGENTPICKS_H

#include "llsingleton.h"

struct LLAvatarPicks;

/**
 * Class that provides information about Agent Picks
 */
class LLAgentPicksInfo : public LLSingleton<LLAgentPicksInfo>
{
	class LLAgentPicksObserver;

public:

	LLAgentPicksInfo();
	
	virtual ~LLAgentPicksInfo();

	/**
	 * Requests number of picks from server. 
	 * 
	 * Number of Picks is requested from server, thus it is not available immediately.
	 */
	void requestNumberOfPicks();

	/**
	 * Returns number of Picks.
	 */
	S32 getNumberOfPicks() { return mNumberOfPicks; }

	/**
	 * Returns maximum number of Picks.
	 */
	S32 getMaxNumberOfPicks() { return mMaxNumberOfPicks; }

	/**
	 * Returns TRUE if Agent has maximum allowed number of Picks.
	 */
	bool isPickLimitReached();

	/**
	 * After creating or deleting a Pick we can assume operation on server will be 
	 * completed successfully. Incrementing/decrementing number of picks makes new number
	 * of picks available immediately. Actual number of picks will be updated when we receive 
	 * response from server.
	 */
	void incrementNumberOfPicks() { ++mNumberOfPicks; }

	void decrementNumberOfPicks() { --mNumberOfPicks; }

private:

	void onServerRespond(LLAvatarPicks* picks);

	/**
	* Sets number of Picks.
	*/
	void setNumberOfPicks(S32 number) { mNumberOfPicks = number; }

	/**
	* Sets maximum number of Picks.
	*/
	void setMaxNumberOfPicks(S32 max_picks) { mMaxNumberOfPicks = max_picks; }

private:

	LLAgentPicksObserver* mAgentPicksObserver;
	S32 mMaxNumberOfPicks;
	S32 mNumberOfPicks;
};

#endif //LL_LLAGENTPICKS_H
