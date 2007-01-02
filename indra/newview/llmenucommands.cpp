/** 
 * @file llmenucommands.cpp
 * @brief Implementations of menu commands.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llmenucommands.h"

#include "imageids.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llstring.h"
#include "message.h"

#include "llagent.h"
#include "llcallbacklist.h"
#include "llcallingcard.h"
#include "llchatbar.h"
#include "llviewercontrol.h"
#include "llfirstuse.h"
#include "llfloaterchat.h"
#include "llfloaterclothing.h"
#include "llfloaterdirectory.h"
#include "llfloatermap.h"
#include "llfloaterworldmap.h"
#include "llgivemoney.h"
#include "llinventoryview.h"
#include "llnotify.h"
#include "llstatusbar.h"
#include "llimview.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewergesture.h"			// for triggering gestures
#include "llviewermessage.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llvieweruictrlfactory.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "llworldmap.h"
#include "viewer.h"
#include "llfocusmgr.h"

void handle_track_avatar(const LLUUID& agent_id, const std::string& name)
{	
	LLAvatarTracker::instance().track(agent_id, name);

	LLFloaterDirectory::hide(NULL);
	LLFloaterWorldMap::show(NULL, TRUE);
}

void handle_pay_by_id(const LLUUID& agent_id)
{
	const BOOL is_group = FALSE;
	LLFloaterPay::payDirectly(&give_money, agent_id, is_group);
}

void handle_mouselook(void*)
{
	gAgent.changeCameraToMouselook();
}


void handle_map(void*)
{
	LLFloaterWorldMap::toggle(NULL);
}

void handle_mini_map(void*)
{
	LLFloaterMap::toggle(NULL);
}


void handle_find(void*)
{
	LLFloaterDirectory::toggleFind(NULL);
}


void handle_events(void*)
{
	LLFloaterDirectory::toggleEvents(NULL);
}


void handle_inventory(void*)
{
	// We're using the inventory, possibly for the
	// first time.
	LLFirstUse::useInventory();

	LLInventoryView::toggleVisibility(NULL);
}


void handle_clothing(void*)
{
	LLFloaterClothing::toggleVisibility();
}


void handle_chat(void*)
{
	// give focus to chatbar if it's open but not focused
	if (gSavedSettings.getBOOL("ChatVisible") && gFocusMgr.childHasKeyboardFocus(gChatBar))
	{
		LLChatBar::stopChat();
	}
	else
	{
		LLChatBar::startChat(NULL);
	}
}

void handle_slash_key(void*)
{
	// LLChatBar::startChat("/");
	//
	// Don't do this, it results in a double-slash in the input field.
	// Another "/" will be automatically typed for us, because the WM_KEYDOWN event
	// that generated the menu accelerator call (and hence puts focus in
	// the chat edtior) will be followed by a "/" WM_CHAR character message,
	// which will type the slash.  Yes, it's weird.  It only matters for
	// menu accelerators that put input focus into a field.   And Mac works
	// the same way.  JC

	LLChatBar::startChat(NULL);
}
