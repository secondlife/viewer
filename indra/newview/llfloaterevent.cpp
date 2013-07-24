/** 
 * @file llfloaterevent.cpp
 * @brief Display for events in the finder
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "llfloaterevent.h"

#include "message.h"
#include "llnotificationsutil.h"
#include "llui.h"

#include "llagent.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llcachename.h"
#include "llcommandhandler.h"	// secondlife:///app/chat/ support
#include "lleventflags.h"
#include "llmediactrl.h"
#include "llexpandabletextbox.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llmediactrl.h"
#include "llfloaterworldmap.h"
#include "llinventorymodel.h"
#include "llsecondlifeurls.h"
#include "llslurl.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lluiconstants.h"
#include "llviewercontrol.h"
#include "llweb.h"
#include "llworldmap.h"
#include "llworldmapmessage.h"
#include "lluictrlfactory.h"
#include "lltrans.h"


LLFloaterEvent::LLFloaterEvent(const LLSD& key)
	: LLFloater(key),
      LLViewerMediaObserver(),
      mBrowser(NULL),
	  mEventID(0)
{
}


LLFloaterEvent::~LLFloaterEvent()
{
}


BOOL LLFloaterEvent::postBuild()
{
	mBrowser = getChild<LLMediaCtrl>("browser");
	if (mBrowser)
	{
		mBrowser->addObserver(this);
	}

	return TRUE;
}

void LLFloaterEvent::handleMediaEvent(LLPluginClassMedia *self, EMediaEvent event)
{
	switch (event) 
	{
		case MEDIA_EVENT_NAVIGATE_BEGIN:
			getChild<LLUICtrl>("status_text")->setValue(getString("loading_text"));
			break;
			
		case MEDIA_EVENT_NAVIGATE_COMPLETE:
			getChild<LLUICtrl>("status_text")->setValue(getString("done_text"));
			break;
			
		default:
			break;
	}
}

void LLFloaterEvent::setEventID(const U32 event_id)
{
	mEventID = event_id;

	if (event_id != 0)
	{
		LLSD subs;
		subs["EVENT_ID"] = (S32)event_id;
        // get the search URL and expand all of the substitutions                                                       
        // (also adds things like [LANGUAGE], [VERSION], [OS], etc.)                                                    
		std::ostringstream url;
		url <<  gSavedSettings.getString("EventURL") << event_id << "/" << std::endl;
		// and load the URL in the web view                                                                             
        mBrowser->navigateTo(url.str());
		
	}
}
