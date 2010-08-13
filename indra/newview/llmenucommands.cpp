/** 
 * @file llmenucommands.cpp
 * @brief Implementations of menu commands.
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

#include "llmenucommands.h"

#include "imageids.h"
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llstring.h"
#include "message.h"

#include "llagentcamera.h"
#include "llcallingcard.h"
#include "llviewercontrol.h"
//#include "llfirstuse.h"
#include "llfloaterworldmap.h"
#include "lllineeditor.h"
#include "llstatusbar.h"
#include "llimview.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewergesture.h"			// for triggering gestures
#include "llviewermessage.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "llworldmap.h"
#include "llfocusmgr.h"
#include "llnearbychatbar.h"

void handle_mouselook(void*)
{
	gAgentCamera.changeCameraToMouselook();
}


void handle_chat(void*)
{
	// give focus to chatbar if it's open but not focused
	if (gSavedSettings.getBOOL("ChatVisible") && 
		gFocusMgr.childHasKeyboardFocus(LLNearbyChatBar::getInstance()->getChatBox()))
	{
		LLNearbyChatBar::stopChat();
	}
	else
	{
		LLNearbyChatBar::startChat(NULL);
	}
}

void handle_slash_key(void*)
{
	// LLBottomTray::startChat("/");
	//
	// Don't do this, it results in a double-slash in the input field.
	// Another "/" will be automatically typed for us, because the WM_KEYDOWN event
	// that generated the menu accelerator call (and hence puts focus in
	// the chat edtior) will be followed by a "/" WM_CHAR character message,
	// which will type the slash.  Yes, it's weird.  It only matters for
	// menu accelerators that put input focus into a field.   And Mac works
	// the same way.  JC

	LLNearbyChatBar::startChat(NULL);
}
