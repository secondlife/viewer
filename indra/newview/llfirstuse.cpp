/** 
 * @file llfirstuse.cpp
 * @brief Methods that spawn "first-use" dialogs
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llfirstuse.h"

// library includes
#include "indra_constants.h"

// viewer includes
#include "llnotify.h"
#include "llviewercontrol.h"
#include "llui.h"
#include "viewer.h"

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

