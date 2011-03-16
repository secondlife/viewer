/** 
 * @file llfirstuse.cpp
 * @brief Methods that spawn "first-use" dialogs
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llfirstuse.h"

// library includes
#include "indra_constants.h"
#include "llnotifications.h"

// viewer includes
#include "llagent.h"	// for gAgent.inPrelude()
#include "llviewercontrol.h"
#include "llui.h"
#include "llappviewer.h"
#include "lltracker.h"


// static
//std::set<std::string> LLFirstUse::sConfigVariables;
std::set<std::string> LLFirstUse::sConfigVariablesEnabled;

// static
//void LLFirstUse::addConfigVariable(const std::string& var)
//{
//	sConfigVariables.insert(var);
//}

// static
//void LLFirstUse::disableFirstUse()
//{
//	// Set all first-use warnings to disabled
//	for (std::set<std::string>::iterator iter = sConfigVariables.begin();
//		 iter != sConfigVariables.end(); ++iter)
//	{
//		gWarningSettings.setBOOL(*iter, FALSE);
//	}
//}

// static
//void LLFirstUse::resetFirstUse()
//{
//	// Set all first-use warnings to disabled
//	for (std::set<std::string>::iterator iter = sConfigVariables.begin();
//		 iter != sConfigVariables.end(); ++iter)
//	{
//		gWarningSettings.setBOOL(*iter, TRUE);
//	}
//}

// static
void LLFirstUse::otherAvatarChatFirst(bool enable)
{
	firstUseNotification("FirstOtherChatBeforeUser", enable, "HintChat", LLSD(), LLSD().with("target", "chat_bar").with("direction", "top_right").with("distance", 24));
}

// static
void LLFirstUse::speak(bool enable)
{
	firstUseNotification("FirstSpeak", enable, "HintSpeak", LLSD(), LLSD().with("target", "speak_btn").with("direction", "top"));
}

// static
void LLFirstUse::sit(bool enable)
{
	firstUseNotification("FirstSit", enable, "HintSit", LLSD(), LLSD().with("target", "stand_btn").with("direction", "top"));
}

// static
void LLFirstUse::newInventory(bool enable)
{
	// turning this off until bug EXP-62 can be fixed (inventory hint appears for new users when their initial inventory is acquired)
	// firstUseNotification("FirstInventoryOffer", enable, "HintInventory", LLSD(), LLSD().with("target", "inventory_btn").with("direction", "left"));
}

// static
void LLFirstUse::useSandbox()
{
	firstUseNotification("FirstSandbox", true, "FirstSandbox", LLSD().with("HOURS", SANDBOX_CLEAN_FREQ).with("TIME", SANDBOX_FIRST_CLEAN_HOUR));
}

// static
void LLFirstUse::notUsingDestinationGuide(bool enable)
{
	// not doing this yet
	firstUseNotification("FirstNotUseDestinationGuide", enable, "HintDestinationGuide", LLSD(), LLSD().with("target", "dest_guide_btn").with("direction", "top"));
}

void LLFirstUse::notUsingAvatarPicker(bool enable)
{
	// not doing this yet
	firstUseNotification("FirstNotUseAvatarPicker", enable, "HintAvatarPicker", LLSD(), LLSD().with("target", "avatar_picker_btn").with("direction", "top"));
}


// static
void LLFirstUse::notUsingSidePanel(bool enable)
{
	// not doing this yet
	//firstUseNotification("FirstNotUseSidePanel", enable, "HintSidePanel", LLSD(), LLSD().with("target", "side_panel_btn").with("direction", "left"));
}

// static
void LLFirstUse::notMoving(bool enable)
{
	// fire off 2 notifications and rely on filtering to select the relevant one
	firstUseNotification("FirstNotMoving", enable, "HintMove", LLSD(), LLSD().with("target", "move_btn").with("direction", "top"));
	firstUseNotification("FirstNotMoving", enable, "HintMoveArrows", LLSD(), LLSD().with("target", "bottom_tray").with("direction", "top").with("hint_image", "arrow_keys.png").with("down_arrow", ""));
}

// static
void LLFirstUse::viewPopup(bool enable)
{
	firstUseNotification("FirstViewPopup", enable, "HintView", LLSD(), LLSD().with("target", "view_popup").with("direction", "right"));
}

// static
void LLFirstUse::setDisplayName(bool enable)
{
	firstUseNotification("FirstDisplayName", enable, "HintDisplayName", LLSD(), LLSD().with("target", "set_display_name").with("direction", "left"));
}

// static
void LLFirstUse::receiveLindens(bool enable)
{
	firstUseNotification("FirstReceiveLindens", enable, "HintLindenDollar", LLSD(), LLSD().with("target", "linden_balance").with("direction", "bottom"));
}


//static 
void LLFirstUse::firstUseNotification(const std::string& control_var, bool enable, const std::string& notification_name, LLSD args, LLSD payload)
{
	init();

	if (enable)
	{
		if(sConfigVariablesEnabled.find(control_var) != sConfigVariablesEnabled.end())
		{
			return ; //already added
		}

		if (gSavedSettings.getBOOL("EnableUIHints"))
		{
			LL_DEBUGS("LLFirstUse") << "Trigger first use notification " << notification_name << LL_ENDL;

			// if notification doesn't already exist and this notification hasn't been disabled...
			if (gWarningSettings.getBOOL(control_var))
			{ 
				sConfigVariablesEnabled.insert(control_var) ;

				// create new notification
				LLNotifications::instance().add(LLNotification::Params().name(notification_name).substitutions(args).payload(payload.with("control_var", control_var)));
			}
		}
	}	
	else
	{
		LL_DEBUGS("LLFirstUse") << "Disabling first use notification " << notification_name << LL_ENDL;
		LLNotifications::instance().cancelByName(notification_name);
		// redundantly clear settings var here, in case there are no notifications to cancel
		gWarningSettings.setBOOL(control_var, FALSE);
	}
}

// static
void LLFirstUse::init()
{
	static bool initialized = false;
	if (!initialized)
	{
		LLNotifications::instance().getChannel("Hints")->connectChanged(&processNotification);
	}
	initialized = true;
}

//static 
bool LLFirstUse::processNotification(const LLSD& notify)
{
	if (notify["sigtype"].asString() == "delete")
	{
		LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID());
		if (notification)
		{
			// disable any future notifications
			gWarningSettings.setBOOL(notification->getPayload()["control_var"], FALSE);
		}
	}
	return false;
}
