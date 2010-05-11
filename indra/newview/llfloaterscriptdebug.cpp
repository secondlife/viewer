/** 
 * @file llfloaterscriptdebug.cpp
 * @brief Chat window for showing script errors and warnings
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
// 	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_script_debug.xml");
	
	// avoid resizing of the window to match 
	// the initial size of the tabbed-childs, whenever a tab is opened or closed
	mAutoResize = FALSE;
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
	LLFloater* floaterp = LLFloaterReg::showInstance("script_debug_output", object_id);
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
		objectp->setIcon(LLViewerTextureManager::getFetchedTextureFromFile("script_error.j2c", TRUE, LLViewerTexture::BOOST_UI));
		floater_label = llformat("%s(%.2f, %.2f)", user_name.c_str(), objectp->getPositionRegion().mV[VX], objectp->getPositionRegion().mV[VY]);
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
	//LLUICtrlFactory::getInstance()->buildFloater(this, "floater_script_debug_panel.xml");
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

