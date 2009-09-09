/** 
 * @file lldockablefloater.h
 * @brief Creates a panel of a specific kind for a toast.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LL_DOCKABLEFLOATER_H
#define LL_DOCKABLEFLOATER_H

#include "llerror.h"
#include "llfloater.h"
#include "lldockcontrol.h"

/**
 * Represents floater that can dock.
 * In case impossibility deriving from LLDockableFloater use LLDockControl.
 */
class LLDockableFloater : public LLFloater
{
	static const U32 UNDOCK_LEAP_HEIGHT = 12;
public:
	LOG_CLASS(LLDockableFloater);
	LLDockableFloater(LLDockControl* dockControl, const LLSD& key, const Params& params = getDefaultParams());
	virtual ~LLDockableFloater();

	/* virtula */BOOL postBuild();
	/* virtual */void setDocked(bool docked, bool pop_on_undock = true);
	/* virtual */void draw();

protected:
	void setDockControl(LLDockControl* dockControl);
	LLDockControl* getDockControl();
	const LLUIImagePtr& getDockTongue();

private:
	std::auto_ptr<LLDockControl> mDockControl;
	LLUIImagePtr mDockTongue;
};

#endif /* LL_DOCKABLEFLOATER_H */
