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

#include "lluictrlfactory.h"
#include "llfloaterscriptdebug.h"

#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llstring.h"
#include "message.h"

// project include
#include "llviewertexteditor.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "llviewerimagelist.h"

//
// Statics
//
LLFloaterScriptDebug*	LLFloaterScriptDebug::sInstance = NULL;

void* getOutputWindow(void* data);

//
// Member Functions
//
LLFloaterScriptDebug::LLFloaterScriptDebug(const std::string& filename)
  : LLMultiFloater()
{
	mFactoryMap["all_scripts"] = LLCallbackMap(getOutputWindow, NULL);
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_script_debug.xml");

	// avoid resizing of the window to match 
	// the initial size of the tabbed-childs, whenever a tab is opened or closed
	mAutoResize = FALSE;
}

LLFloaterScriptDebug::~LLFloaterScriptDebug()
{
	sInstance = NULL;
}

void LLFloaterScriptDebug::show(const LLUUID& object_id)
{
	LLFloater* floaterp = addOutputWindow(object_id);
	if (sInstance)
	{
		sInstance->openFloater(object_id);
		if (object_id.notNull())
			sInstance->showFloater(floaterp, LLTabContainer::END);
// 		else // Jump to [All scripts], but keep it on the left
// 			sInstance->showFloater(floaterp, LLTabContainer::START);
	}
}

BOOL LLFloaterScriptDebug::postBuild()
{
	LLMultiFloater::postBuild();

	if (mTabContainer)
	{
		// *FIX: apparantly fails for tab containers?
// 		mTabContainer->requires<LLFloater>("all_scripts");
// 		mTabContainer->checkRequirements();
		return TRUE;
	}

	return FALSE;
}

void* getOutputWindow(void* data)
{
	return new LLFloaterScriptDebugOutput(LLUUID::null);
}

LLFloater* LLFloaterScriptDebug::addOutputWindow(const LLUUID &object_id)
{
	if (!sInstance)
	{
		sInstance = new LLFloaterScriptDebug("floater_script_debug.xml");
		sInstance->setVisible(FALSE);
	}

	LLFloater::setFloaterHost(sInstance);
	LLFloater* floaterp = LLFloaterScriptDebugOutput::show(object_id);
	LLFloater::setFloaterHost(NULL);

	// Tabs sometimes overlap resize handle
	sInstance->moveResizeHandlesToFront();

	return floaterp;
}

void LLFloaterScriptDebug::addScriptLine(const std::string &utf8mesg, const std::string &user_name, const LLColor4& color, const LLUUID& source_id)
{
	LLViewerObject* objectp = gObjectList.findObject(source_id);
	std::string floater_label;

	if (objectp)
	{
		objectp->setIcon(gImageList.getImageFromFile("script_error.j2c", TRUE, TRUE));
		floater_label = llformat("%s(%.2f, %.2f)", user_name.c_str(), objectp->getPositionRegion().mV[VX], objectp->getPositionRegion().mV[VY]);
	}
	else
	{
		floater_label = user_name;
	}

	addOutputWindow(LLUUID::null);
	addOutputWindow(source_id);

	// add to "All" floater
	LLFloaterScriptDebugOutput* floaterp = LLFloaterScriptDebugOutput::getFloaterByID(LLUUID::null);
	floaterp->addLine(utf8mesg, user_name, color);

	// add to specific script instance floater
	floaterp = LLFloaterScriptDebugOutput::getFloaterByID(source_id);
	floaterp->addLine(utf8mesg, floater_label, color);
}

//
// LLFloaterScriptDebugOutput
//

std::map<LLUUID, LLFloaterScriptDebugOutput*> LLFloaterScriptDebugOutput::sInstanceMap;

LLFloaterScriptDebugOutput::LLFloaterScriptDebugOutput(const LLUUID& object_id)
  : LLFloater(),
	mObjectID(object_id)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_script_debug_panel.xml");
	sInstanceMap[object_id] = this;
}

BOOL LLFloaterScriptDebugOutput::postBuild()
{
	LLFloater::postBuild();
	mHistoryEditor = getChild<LLViewerTextEditor>("Chat History Editor");
	return TRUE;
}

LLFloaterScriptDebugOutput::~LLFloaterScriptDebugOutput()
{
	sInstanceMap.erase(mObjectID);
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

	mHistoryEditor->appendColoredText(utf8mesg, false, true, color);
}

//static
LLFloaterScriptDebugOutput* LLFloaterScriptDebugOutput::show(const LLUUID& object_id)
{
	LLFloaterScriptDebugOutput* floaterp = NULL;
	instance_map_t::iterator found_it = sInstanceMap.find(object_id);
	if (found_it == sInstanceMap.end())
	{
		floaterp = new LLFloaterScriptDebugOutput(object_id);
		floaterp->openFloater();
	}
	else
	{
		floaterp = found_it->second;
	}

	return floaterp;
}

//static 
LLFloaterScriptDebugOutput* LLFloaterScriptDebugOutput::getFloaterByID(const LLUUID& object_id)
{
	LLFloaterScriptDebugOutput* floaterp = NULL;
	instance_map_t::iterator found_it = sInstanceMap.find(object_id);
	if (found_it != sInstanceMap.end())
	{
		floaterp = found_it->second;
	}

	return floaterp;
}
