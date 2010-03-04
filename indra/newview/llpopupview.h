/** 
 * @file llpopupview.h
 * @brief Holds transient popups
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLPOPUPVIEW_H
#define LL_LLPOPUPVIEW_H

#include "llpanel.h"

class LLPopupView : public LLPanel
{
public:
	LLPopupView();

	/*virtual*/ void draw();
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMiddleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMiddleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleRightMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleDoubleClick(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);
	/*virtual*/ BOOL handleToolTip(S32 x, S32 y, MASK mask);

	void addPopup(LLView* popup);
	void removePopup(LLView* popup);
	void clearPopups();

	typedef std::list<LLHandle<LLView> > popup_list_t;
	popup_list_t getCurrentPopups() { return mPopups; }

private:
	BOOL handleMouseEvent(boost::function<BOOL(LLView*, S32, S32)>, boost::function<bool(LLView*)>, S32 x, S32 y);
	popup_list_t mPopups;
};
#endif //LL_LLROOTVIEW_H
