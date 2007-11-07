/** 
 * @file llfirstuse.cpp
 * @brief Methods that spawn "first-use" dialogs
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

// viewer includes
#include "llnotify.h"
#include "llfloatervoicewizard.h"
#include "llviewercontrol.h"
#include "llui.h"
#include "llappviewer.h"

// static
std::set<LLString> LLFirstUse::sConfigVariables;

// static
void LLFirstUse::addConfigVariable(const LLString& var)
{
	gSavedSettings.addWarning(var);
	sConfigVariables.insert(var);
}

// static
void LLFirstUse::disableFirstUse()
{
	// Set all first-use warnings to disabled
	for (std::set<LLString>::iterator iter = sConfigVariables.begin();
		 iter != sConfigVariables.end(); ++iter)
	{
		gSavedSettings.setWarning(*iter, FALSE);
	}
}

// static
void LLFirstUse::resetFirstUse()
{
	// Set all first-use warnings to disabled
	for (std::set<LLString>::iterator iter = sConfigVariables.begin();
		 iter != sConfigVariables.end(); ++iter)
	{
		gSavedSettings.setWarning(*iter, TRUE);
	}
}


// Called whenever the viewer detects that your balance went up
void LLFirstUse::useBalanceIncrease(S32 delta)
{
	if (gSavedSettings.getWarning("FirstBalanceIncrease"))
	{
		gSavedSettings.setWarning("FirstBalanceIncrease", FALSE);

		LLString::format_map_t args;
		args["[AMOUNT]"] = llformat("%d",delta);
		LLNotifyBox::showXml("FirstBalanceIncrease", args);
	}
}


// Called whenever the viewer detects your balance went down
void LLFirstUse::useBalanceDecrease(S32 delta)
{
	if (gSavedSettings.getWarning("FirstBalanceDecrease"))
	{
		gSavedSettings.setWarning("FirstBalanceDecrease", FALSE);

		LLString::format_map_t args;
		args["[AMOUNT]"] = llformat("%d",-delta);
		LLNotifyBox::showXml("FirstBalanceDecrease", args);
	}
}


// static
void LLFirstUse::useSit()
{
	if (gSavedSettings.getWarning("FirstSit"))
	{
		gSavedSettings.setWarning("FirstSit", FALSE);

		LLNotifyBox::showXml("FirstSit");
	}
}

// static
void LLFirstUse::useMap()
{
	if (gSavedSettings.getWarning("FirstMap"))
	{
		gSavedSettings.setWarning("FirstMap", FALSE);

		LLNotifyBox::showXml("FirstMap");
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
	if (gSavedSettings.getWarning("FirstBuild"))
	{
		gSavedSettings.setWarning("FirstBuild", FALSE);

		LLNotifyBox::showXml("FirstBuild");
	}
}

// static
void LLFirstUse::useLeftClickNoHit()
{ 
	if (gSavedSettings.getWarning("FirstLeftClickNoHit"))
	{
		gSavedSettings.setWarning("FirstLeftClickNoHit", FALSE);

		LLNotifyBox::showXml("FirstLeftClickNoHit");
	}
}

// static
void LLFirstUse::useTeleport()
{
	if (gSavedSettings.getWarning("FirstTeleport"))
	{
		gSavedSettings.setWarning("FirstTeleport", FALSE);

		LLNotifyBox::showXml("FirstTeleport");
	}
}

// static
void LLFirstUse::useOverrideKeys()
{
	if (gSavedSettings.getWarning("FirstOverrideKeys"))
	{
		gSavedSettings.setWarning("FirstOverrideKeys", FALSE);

		LLNotifyBox::showXml("FirstOverrideKeys");
	}
}

// static
void LLFirstUse::useAttach()
{
	// nothing for now
}

// static
void LLFirstUse::useAppearance()
{
	if (gSavedSettings.getWarning("FirstAppearance"))
	{
		gSavedSettings.setWarning("FirstAppearance", FALSE);

		LLNotifyBox::showXml("FirstAppearance");
	}
}

// static
void LLFirstUse::useInventory()
{
	if (gSavedSettings.getWarning("FirstInventory"))
	{
		gSavedSettings.setWarning("FirstInventory", FALSE);

		LLNotifyBox::showXml("FirstInventory");
	}
}


// static
void LLFirstUse::useSandbox()
{
	if (gSavedSettings.getWarning("FirstSandbox"))
	{
		gSavedSettings.setWarning("FirstSandbox", FALSE);

		LLString::format_map_t args;
		args["[HOURS]"] = llformat("%d",SANDBOX_CLEAN_FREQ);
		args["[TIME]"] = llformat("%d",SANDBOX_FIRST_CLEAN_HOUR);
		LLNotifyBox::showXml("FirstSandbox", args);
	}
}

// static
void LLFirstUse::useFlexible()
{
	if (gSavedSettings.getWarning("FirstFlexible"))
	{
		gSavedSettings.setWarning("FirstFlexible", FALSE);

		LLNotifyBox::showXml("FirstFlexible");
	}
}

// static
void LLFirstUse::useDebugMenus()
{
	if (gSavedSettings.getWarning("FirstDebugMenus"))
	{
		gSavedSettings.setWarning("FirstDebugMenus", FALSE);

		LLNotifyBox::showXml("FirstDebugMenus");
	}
}

// static
void LLFirstUse::useSculptedPrim()
{
	if (gSavedSettings.getWarning("FirstSculptedPrim"))
	{
		gSavedSettings.setWarning("FirstSculptedPrim", FALSE);

		LLNotifyBox::showXml("FirstSculptedPrim");
		
	}
}

// static 
void LLFirstUse::useVoice()
{
	if (gDisableVoice) return;
	if (gSavedSettings.getWarning("FirstVoice"))
	{
		gSavedSettings.setWarning("FirstVoice", FALSE);

		LLFloaterVoiceWizard::showInstance();
	}
}
