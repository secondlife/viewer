/** 
 * @file llfloaterscriptdebug.cpp
 * @brief Chat window for showing script errors and warnings
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#include "llvieweruictrlfactory.h"
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

//
// Member Functions
//
LLFloaterScriptDebug::LLFloaterScriptDebug() : 
	LLMultiFloater()
{
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
		sInstance->open();		/* Flawfinder: ignore */
		sInstance->showFloater(floaterp);
	}
}

BOOL LLFloaterScriptDebug::postBuild()
{
	LLMultiFloater::postBuild();

	if (mTabContainer)
	{
		// *FIX: apparantly fails for tab containers?
// 		mTabContainer->requires("all_scripts", WIDGET_TYPE_FLOATER);
// 		mTabContainer->checkRequirements();
		return TRUE;
	}

	return FALSE;
}

void* getOutputWindow(void* data)
{
	return new LLFloaterScriptDebugOutput();
}

LLFloater* LLFloaterScriptDebug::addOutputWindow(const LLUUID &object_id)
{
	if (!sInstance)
	{
		sInstance = new LLFloaterScriptDebug();
		LLCallbackMap::map_t factory_map;
		factory_map["all_scripts"] = LLCallbackMap(getOutputWindow, NULL);
		gUICtrlFactory->buildFloater(sInstance, "floater_script_debug.xml", &factory_map);
		sInstance->setVisible(FALSE);
	}

	LLFloater* floaterp = NULL;
	LLFloater::setFloaterHost(sInstance);
	{
		floaterp = LLFloaterScriptDebugOutput::show(object_id);
	}
	LLFloater::setFloaterHost(NULL);

	// Tabs sometimes overlap resize handle
	sInstance->moveResizeHandlesToFront();

	return floaterp;
}

void LLFloaterScriptDebug::addScriptLine(const std::string &utf8mesg, const std::string &user_name, const LLColor4& color, const LLUUID& source_id)
{
	LLViewerObject* objectp = gObjectList.findObject(source_id);
	LLString floater_label;

	if (objectp)
	{
		objectp->setIcon(gImageList.getImage(LLUUID(gViewerArt.getString("script_error.tga"))));
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

LLFloaterScriptDebugOutput::LLFloaterScriptDebugOutput()
: mObjectID(LLUUID::null)
{
	sInstanceMap[mObjectID] = this;
}

LLFloaterScriptDebugOutput::LLFloaterScriptDebugOutput(const LLUUID& object_id)
: LLFloater("script instance floater", LLRect(0, 200, 200, 0), "Script", TRUE), mObjectID(object_id)
{
	S32 y = getRect().getHeight() - LLFLOATER_HEADER_SIZE - LLFLOATER_VPAD;
	S32 x = LLFLOATER_HPAD;
	// History editor
	// Give it a border on the top
	LLRect history_editor_rect(
		x,
		y,
		getRect().getWidth() - LLFLOATER_HPAD,
				LLFLOATER_VPAD );
	mHistoryEditor = new LLViewerTextEditor( "Chat History Editor", 
										history_editor_rect, S32_MAX, "", LLFontGL::sSansSerif);
	mHistoryEditor->setWordWrap( TRUE );
	mHistoryEditor->setFollowsAll();
	mHistoryEditor->setEnabled( FALSE );
	mHistoryEditor->setTabStop( TRUE );  // We want to be able to cut or copy from the history.
	addChild(mHistoryEditor);
}

void LLFloaterScriptDebugOutput::init(const LLString& title, BOOL resizable, 
						S32 min_width, S32 min_height, BOOL drag_on_left,
						BOOL minimizable, BOOL close_btn)
{
	LLFloater::init(title, resizable, min_width, min_height, drag_on_left, minimizable, close_btn);
	S32 y = getRect().getHeight() - LLFLOATER_HEADER_SIZE - LLFLOATER_VPAD;
	S32 x = LLFLOATER_HPAD;
	// History editor
	// Give it a border on the top
	LLRect history_editor_rect(
		x,
		y,
		getRect().getWidth() - LLFLOATER_HPAD,
				LLFLOATER_VPAD );
	mHistoryEditor = new LLViewerTextEditor( "Chat History Editor", 
										history_editor_rect, S32_MAX, "", LLFontGL::sSansSerif);
	mHistoryEditor->setWordWrap( TRUE );
	mHistoryEditor->setFollowsAll();
	mHistoryEditor->setEnabled( FALSE );
	mHistoryEditor->setTabStop( TRUE );  // We want to be able to cut or copy from the history.
	addChild(mHistoryEditor);
}

LLFloaterScriptDebugOutput::~LLFloaterScriptDebugOutput()
{
	sInstanceMap.erase(mObjectID);
}

void LLFloaterScriptDebugOutput::addLine(const std::string &utf8mesg, const std::string &user_name, const LLColor4& color)
{
	if (mObjectID.isNull())
	{
		//setTitle("[All scripts]");
		setCanTearOff(FALSE);
		setCanClose(FALSE);
	}
	else
	{
		setTitle(user_name);
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
		sInstanceMap[object_id] = floaterp;
		floaterp->open();		/* Flawfinder: ignore*/
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
