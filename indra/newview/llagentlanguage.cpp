/**
 * @file llagentlanguage.cpp
 * @brief Transmit language information to server
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
#include "llagentlanguage.h"
// viewer includes
#include "llagent.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
// library includes
#include "llui.h"					// getLanguage()
#include "httpcommon.h"

// static
void LLAgentLanguage::init()
{
	gSavedSettings.getControl("Language")->getSignal()->connect(boost::bind(&onChange));
	gSavedSettings.getControl("InstallLanguage")->getSignal()->connect(boost::bind(&onChange));
	gSavedSettings.getControl("SystemLanguage")->getSignal()->connect(boost::bind(&onChange));
	gSavedSettings.getControl("LanguageIsPublic")->getSignal()->connect(boost::bind(&onChange));
}

// static
void LLAgentLanguage::onChange()
{
	// Clear inventory cache so that default names of inventory items
	// appear retranslated (EXT-8308).
	gSavedSettings.setBOOL("PurgeCacheOnNextStartup", true);
}

// send language settings to the sim
// static
bool LLAgentLanguage::update()
{
    LLSD body;

	std::string language = LLUI::getLanguage();
		
	body["language"] = language;
	body["language_is_public"] = gSavedSettings.getBOOL("LanguageIsPublic");
		
    if (!gAgent.requestPostCapability("UpdateAgentLanguage", body))
    {
        LL_WARNS("Language") << "Language capability unavailable." << LL_ENDL;
    }

    return true;
}
