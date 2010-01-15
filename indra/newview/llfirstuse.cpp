/** 
 * @file llfirstuse.cpp
 * @brief Methods that spawn "first-use" dialogs
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

#include "llviewerprecompiledheaders.h"

#include "llfirstuse.h"

// library includes
#include "indra_constants.h"
#include "llnotificationsutil.h"

// viewer includes
#include "llagent.h"	// for gAgent.inPrelude()
#include "llviewercontrol.h"
#include "llui.h"
#include "llappviewer.h"
#include "lltracker.h"

/*
// static
std::set<std::string> LLFirstUse::sConfigVariables;

// static
void LLFirstUse::addConfigVariable(const std::string& var)
{
	sConfigVariables.insert(var);
}

// static
void LLFirstUse::disableFirstUse()
{
	// Set all first-use warnings to disabled
	for (std::set<std::string>::iterator iter = sConfigVariables.begin();
		 iter != sConfigVariables.end(); ++iter)
	{
		gWarningSettings.setBOOL(*iter, FALSE);
	}
}

// static
void LLFirstUse::resetFirstUse()
{
	// Set all first-use warnings to disabled
	for (std::set<std::string>::iterator iter = sConfigVariables.begin();
		 iter != sConfigVariables.end(); ++iter)
	{
		gWarningSettings.setBOOL(*iter, TRUE);
	}
}
*/
/*

// Called whenever the viewer detects that your balance went up
void LLFirstUse::useBalanceIncrease(S32 delta)
{
	if (gWarningSettings.getBOOL("FirstBalanceIncrease"))
	{
		gWarningSettings.setBOOL("FirstBalanceIncrease", FALSE);

		LLSD args;
		args["AMOUNT"] = llformat("%d",delta);
		LLNotificationsUtil::add("FirstBalanceIncrease", args);
	}
}


// Called whenever the viewer detects your balance went down
void LLFirstUse::useBalanceDecrease(S32 delta)
{
	if (gWarningSettings.getBOOL("FirstBalanceDecrease"))
	{
		gWarningSettings.setBOOL("FirstBalanceDecrease", FALSE);

		LLSD args;
		args["AMOUNT"] = llformat("%d",-delta);
		LLNotificationsUtil::add("FirstBalanceDecrease", args);
	}
}


// static
void LLFirstUse::useSit()
{
	// Our orientation island uses sitting to teach vehicle driving
	// so just never show this message. JC
	//if (gWarningSettings.getBOOL("FirstSit"))
	//{
	//	gWarningSettings.setBOOL("FirstSit", FALSE);
        //
	//	LLNotificationsUtil::add("FirstSit");
	//}
}

// static
void LLFirstUse::useMap()
{
	if (gWarningSettings.getBOOL("FirstMap"))
	{
		gWarningSettings.setBOOL("FirstMap", FALSE);

		LLNotificationsUtil::add("FirstMap");
	}
}

// static
void LLFirstUse::useGoTo()
{
	// nothing for now JC
}

// static
void LLFirstUse::useBuild()
{
	if (gWarningSettings.getBOOL("FirstBuild"))
	{
		gWarningSettings.setBOOL("FirstBuild", FALSE);

		LLNotificationsUtil::add("FirstBuild");
	}
}
 
 */
/*
// static
void LLFirstUse::useLeftClickNoHit()
{ 
	if (gWarningSettings.getBOOL("FirstLeftClickNoHit"))
	{
		gWarningSettings.setBOOL("FirstLeftClickNoHit", FALSE);

		LLNotificationsUtil::add("FirstLeftClickNoHit");
	}
}
*/
/*
// static
void LLFirstUse::useTeleport()
{
	if (gWarningSettings.getBOOL("FirstTeleport"))
	{
		LLVector3d teleportDestination = LLTracker::getTrackedPositionGlobal();
		if(teleportDestination != LLVector3d::zero)
		{
			gWarningSettings.setBOOL("FirstTeleport", FALSE);

		        LLNotificationsUtil::add("FirstTeleport");
		}
	}
}
*/
// static
void LLFirstUse::useOverrideKeys()
{
	// Our orientation island uses key overrides to teach vehicle driving
	// so don't show this message until you get off OI. JC
	if (!gAgent.inPrelude())
	{
		if (gWarningSettings.getBOOL("FirstOverrideKeys"))
		{
			gWarningSettings.setBOOL("FirstOverrideKeys", FALSE);

			LLNotificationsUtil::add("FirstOverrideKeys");
		}
	}
}
/*
// static
void LLFirstUse::useAttach()
{
	// nothing for now
}

// static
void LLFirstUse::useAppearance()
{
	if (gWarningSettings.getBOOL("FirstAppearance"))
	{
		gWarningSettings.setBOOL("FirstAppearance", FALSE);

		LLNotificationsUtil::add("FirstAppearance");
	}
}

// static
void LLFirstUse::useInventory()
{
	if (gWarningSettings.getBOOL("FirstInventory"))
	{
		gWarningSettings.setBOOL("FirstInventory", FALSE);

		LLNotificationsUtil::add("FirstInventory");
	}
}

*/

// static
void LLFirstUse::useSandbox()
{
	if (gWarningSettings.getBOOL("FirstSandbox"))
	{
		gWarningSettings.setBOOL("FirstSandbox", FALSE);

		LLSD args;
		args["HOURS"] = llformat("%d",SANDBOX_CLEAN_FREQ);
		args["TIME"] = llformat("%d",SANDBOX_FIRST_CLEAN_HOUR);
		LLNotificationsUtil::add("FirstSandbox", args);
	}
}
/*
// static
void LLFirstUse::useFlexible()
{
	if (gWarningSettings.getBOOL("FirstFlexible"))
	{
		gWarningSettings.setBOOL("FirstFlexible", FALSE);

		LLNotificationsUtil::add("FirstFlexible");
	}
}

// static
void LLFirstUse::useDebugMenus()
{
	if (gWarningSettings.getBOOL("FirstDebugMenus"))
	{
		gWarningSettings.setBOOL("FirstDebugMenus", FALSE);

		LLNotificationsUtil::add("FirstDebugMenus");
	}
}

// static
void LLFirstUse::useSculptedPrim()
{
	if (gWarningSettings.getBOOL("FirstSculptedPrim"))
	{
		gWarningSettings.setBOOL("FirstSculptedPrim", FALSE);

		LLNotificationsUtil::add("FirstSculptedPrim");
		
	}
}

// static 
void LLFirstUse::useMedia()
{
	if (gWarningSettings.getBOOL("FirstMedia"))
	{
		gWarningSettings.setBOOL("FirstMedia", FALSE);

		// Popup removed as a short-term fix for EXT-1643.
		// Ultimately, the plan is to kill all First Use dialogs
		//LLNotificationsUtil::add("FirstMedia");
	}
}
*/
