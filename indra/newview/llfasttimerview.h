/** 
 * @file llfasttimerview.h
 * @brief LLFastTimerView class definition
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLFASTTIMERVIEW_H
#define LL_LLFASTTIMERVIEW_H

#include "llfloater.h"
#include "llframetimer.h"

class LLFastTimerView : public LLFloater
{
public:
	LLFastTimerView(const LLRect& rect);
	virtual ~LLFastTimerView();

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual void draw();

	S32 getLegendIndex(S32 y);
	F64 getTime(LLFastTimer::EFastTimerType tidx);
	
private:	
	S32* mBarStart;
	S32* mBarEnd;
	S32 mDisplayMode;
	S32 mDisplayCenter;
	S32 mDisplayCalls;
	S32 mDisplayHz;
	U64 mAvgCountTotal;
	U64 mMaxCountTotal;
	LLRect mBarRect;
	S32	mScrollIndex;
	S32 mHoverIndex;
	S32 mHoverBarIndex;
	LLFrameTimer mHighlightTimer;
	S32 mSubtractHidden;
	S32 mPrintStats;
};

#endif
