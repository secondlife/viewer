/** 
 * @file lltoolbar.h
 * @brief Large friendly buttons at bottom of screen.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLTOOLBAR_H
#define LL_LLTOOLBAR_H

#include "llpanel.h"

#include "llframetimer.h"

class LLResizeHandle;

class LLToolBar
:	public LLPanel
{
public:
	LLToolBar();
	~LLToolBar();

	/*virtual*/ BOOL postBuild();

	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg);

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	static void toggle(void*);
	static BOOL visible(void*);

	// Move buttons to appropriate locations based on rect.
	void layoutButtons();

	// Per-frame refresh call
	void refresh();

	// callbacks
	static void onClickCommunicate(LLUICtrl*, const LLSD&);

	static F32 sInventoryAutoOpenTime;

private:
	void updateCommunicateList();


private:
	BOOL		mInventoryAutoOpen;
	LLFrameTimer mInventoryAutoOpenTimer;
	S32			mNumUnreadIMs;
#if LL_DARWIN
	LLResizeHandle *mResizeHandle;
#endif // LL_DARWIN
};

extern LLToolBar *gToolBar;

#endif
