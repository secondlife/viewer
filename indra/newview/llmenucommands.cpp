/** 
 * @file llmenucommands.cpp
 * @brief Implementations of menu commands.
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

#include "llmenucommands.h"

#include "imageids.h"
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llstring.h"
#include "message.h"

#include "llagent.h"
#include "llcallingcard.h"
#include "llviewercontrol.h"
//#include "llfirstuse.h"
#include "llfloaterchat.h"
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
	gAgent.changeCameraToMouselook();
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
