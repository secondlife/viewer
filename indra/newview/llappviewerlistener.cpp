/**
 * @file   llappviewerlistener.cpp
 * @author Nat Goodspeed
 * @date   2009-06-23
 * @brief  Implementation for llappviewerlistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "llappviewerlistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llappviewer.h"
#include "llviewercontrol.h"

LLAppViewerListener::LLAppViewerListener(const std::string& pumpname, LLAppViewer* llappviewer):
    LLDispatchListener(pumpname, "op"),
    mAppViewer(llappviewer)
{
    // add() every method we want to be able to invoke via this event API.
    add("requestQuit", &LLAppViewerListener::requestQuit);
    add("setSetting", &LLAppViewerListener::setSetting);
}

void LLAppViewerListener::requestQuit(const LLSD& event) const
{
    mAppViewer->requestQuit();
}

void LLAppViewerListener::setSetting(const LLSD & event) const
{
	std::string control_name = event["name"].asString();
	if (gSavedSettings.controlExists(control_name))
	{
		LLControlVariable* control = gSavedSettings.getControl(control_name);

		control->set(event["value"]);
	}
}
