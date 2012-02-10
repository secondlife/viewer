/** 
 * @file lldebugview.h
 * @brief A view containing debug UI elements
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLDEBUGVIEW_H
#define LL_LLDEBUGVIEW_H

// requires:
// stdtypes.h

#include "llview.h"

// declarations
class LLButton;
class LLStatusPanel;
class LLFastTimerView;
class LLMemoryView;
class LLConsole;
class LLTextureView;
class LLFloaterStats;

class LLDebugView : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Params()
		{
			changeDefault(mouse_opaque, false);
		}
	};
	
	LLDebugView(const Params&);
	~LLDebugView();

	void init();
	void draw();
	
	void setStatsVisible(BOOL visible);
	
	LLFastTimerView* mFastTimerView;
	LLMemoryView*	 mMemoryView;
	LLConsole*		 mDebugConsolep;
	LLView*			 mFloaterSnapRegion;
};

extern LLDebugView* gDebugView;

#endif
