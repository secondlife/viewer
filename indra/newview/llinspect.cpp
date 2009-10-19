/** 
 * @file llinspect.cpp
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
#include "llviewerprecompiledheaders.h"

#include "llinspect.h"

#include "llcontrol.h"	// LLCachedControl
#include "llui.h"		// LLUI::sSettingsGroups

LLInspect::LLInspect(const LLSD& key)
:	LLFloater(key),
	mCloseTimer(),
	mOpenTimer()
{
}

LLInspect::~LLInspect()
{
}

// virtual
void LLInspect::draw()
{
	static LLCachedControl<F32> FADE_TIME(*LLUI::sSettingGroups["config"], "InspectorFadeTime", 1.f);
	if (mOpenTimer.getStarted())
	{
		F32 alpha = clamp_rescale(mOpenTimer.getElapsedTimeF32(), 0.f, FADE_TIME, 0.f, 1.f);
		LLViewDrawContext context(alpha);
		LLFloater::draw();
		if (alpha == 1.f)
		{
			mOpenTimer.stop();
		}
		
	}
	else if (mCloseTimer.getStarted())
	{
		F32 alpha = clamp_rescale(mCloseTimer.getElapsedTimeF32(), 0.f, FADE_TIME, 1.f, 0.f);
		LLViewDrawContext context(alpha);
		LLFloater::draw();
		if (mCloseTimer.getElapsedTimeF32() > FADE_TIME)
		{
			closeFloater(false);
		}
	}
	else
	{
		LLFloater::draw();
	}
}

// virtual
void LLInspect::onOpen(const LLSD& data)
{
	LLFloater::onOpen(data);
	
	mCloseTimer.stop();
	mOpenTimer.start();
}

// virtual
void LLInspect::onFocusLost()
{
	LLFloater::onFocusLost();
	
	// Start closing when we lose focus
	mCloseTimer.start();
	mOpenTimer.stop();
}
