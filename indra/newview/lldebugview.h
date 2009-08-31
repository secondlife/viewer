/** 
 * @file lldebugview.h
 * @brief A view containing debug UI elements
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
			mouse_opaque = false;
		}
	};
	LLDebugView(const Params&);
	~LLDebugView();

	void setStatsVisible(BOOL visible);
	
	LLFastTimerView* mFastTimerView;
	LLMemoryView*	 mMemoryView;
	LLConsole*		 mDebugConsolep;
};

extern LLDebugView* gDebugView;

#endif
