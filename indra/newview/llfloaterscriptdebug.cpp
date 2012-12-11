/** 
 * @file llfloaterscriptdebug.cpp
 * @brief Chat window for showing script errors and warnings
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llfloaterscriptdebug.h"

#include "llfloaterreg.h"
#include "lluictrlfactory.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llstring.h"
#include "message.h"

// project include
#include "llviewertexteditor.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "llviewertexturelist.h"

//
// Statics
//

//
// Member Functions
//
LLFloaterScriptDebug::LLFloaterScriptDebug(const LLSD& key)
  : LLMultiFloater(key)
{
	// avoid resizing of the window to match 
	// the initial size of the tabbed-childs, whenever a tab is opened or closed
	mAutoResize = FALSE;
	// enabled autocous blocks controling focus via  LLFloaterReg::showInstance
	setAutoFocus(FALSE);
}

LLFloaterScriptDebug::~LLFloaterScriptDebug()
{
}

void LLFloaterScriptDebug::show(const LLUUID& object_id)
{
	addOutputWindow(object_id);
}

BOOL LLFloaterScriptDebug::postBuild()
{
	LLMultiFloater::postBuild();

	if (mTabContainer)
	{
		return TRUE;
	}

	return FALSE;
}

LLFloater* LLFloaterScriptDebug::addOutputWindow(const LLUUID &object_id)
{
	LLMultiFloater* host = LLFloaterReg::showTypedInstance<LLMultiFloater>("script_debug", LLSD());
	if (!host)
		return NULL;

	LLFloater::setFloaterHost(host);
	// prevent stealing focus, see EXT-8040
	LLFloater* floaterp = LLFloaterReg::showInstance("script_debug_output", object_id, FALSE);
	LLFloater::setFloaterHost(NULL);

	return floaterp;
}

void LLFloaterScriptDebug::addScriptLine(const std::string &utf8mesg, const std::string &user_name, const LLColor4& color, const LLUUID& source_id)
{
	LLViewerObject* objectp = gObjectList.findObject(source_id);
	std::string floater_label;

	// Handle /me messages.
	std::string prefix = utf8mesg.substr(0, 4);
	std::string message = (prefix == "/me " || prefix == "/me'") ? user_name + utf8mesg.substr(3) : utf8mesg;

	if (objectp)
	{
		objectp->setIcon(LLViewerTextureManager::getFetchedTextureFromFile("script_error.j2c", TRUE, LLGLTexture::BOOST_UI));
		floater_label = llformat("%s(%.0f, %.0f, %.0f)",
						user_name.c_str(),
						objectp->getPositionRegion().mV[VX],
						objectp->getPositionRegion().mV[VY],
						objectp->getPositionRegion().mV[VZ]);
	}
	else
	{
		floater_label = user_name;
	}

	addOutputWindow(LLUUID::null);
	addOutputWindow(source_id);

	// add to "All" floater
	LLFloaterScriptDebugOutput* floaterp = 	LLFloaterReg::getTypedInstance<LLFloaterScriptDebugOutput>("script_debug_output", LLUUID::null);
	if (floaterp)
	{
		floaterp->addLine(message, user_name, color);
	}
	
	// add to specific script instance floater
	floaterp = LLFloaterReg::getTypedInstance<LLFloaterScriptDebugOutput>("script_debug_output", source_id);
	if (floaterp)
	{
		floaterp->addLine(message, floater_label, color);
	}
}

//
// LLFloaterScriptDebugOutput
//

LLFloaterScriptDebugOutput::LLFloaterScriptDebugOutput(const LLSD& object_id)
  : LLFloater(LLSD(object_id)),
	mObjectID(object_id.asUUID())
{
	// enabled autocous blocks controling focus via  LLFloaterReg::showInstance
	setAutoFocus(FALSE);
}

BOOL LLFloaterScriptDebugOutput::postBuild()
{
	LLFloater::postBuild();
	mHistoryEditor = getChild<LLViewerTextEditor>("Chat History Editor");
	return TRUE;
}

LLFloaterScriptDebugOutput::~LLFloaterScriptDebugOutput()
{
}

void LLFloaterScriptDebugOutput::addLine(const std::string &utf8mesg, const std::string &user_name, const LLColor4& color)
{
	if (mObjectID.isNull())
	{
		setCanTearOff(FALSE);
		setCanClose(FALSE);
	}
	else
	{
		setTitle(user_name);
		setShortTitle(user_name);
	}

	mHistoryEditor->appendText(utf8mesg, true, LLStyle::Params().color(color));
	mHistoryEditor->blockUndo();
}

