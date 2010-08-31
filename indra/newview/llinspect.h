/** 
 * @file llinspect.h
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LLINSPECT_H
#define LLINSPECT_H

#include "llfloater.h"
#include "llframetimer.h"

/// Base class for all inspectors (super-tooltips showing a miniature
/// properties view).
class LLInspect : public LLFloater
{
public:
	LLInspect(const LLSD& key);
	virtual ~LLInspect();
	
	/// Inspectors have a custom fade-in/fade-out animation
	/*virtual*/ void draw();
	
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleToolTip(S32 x, S32 y, MASK mask);
	/*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);
	
	/// Start open animation
	/*virtual*/ void onOpen(const LLSD& avatar_id);
	
	/// Inspectors close themselves when they lose focus
	/*virtual*/ void onFocusLost();
	
protected:

	virtual bool childHasVisiblePopupMenu();

	LLFrameTimer		mCloseTimer;
	LLFrameTimer		mOpenTimer;
};

#endif

