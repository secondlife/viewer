/** 
* @file llpanelhome.cpp
* @author Martin Reddy
* @brief The Home side tray panel
*
* $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "llpanelhome.h"

#include "llmediactrl.h"
#include "llviewerhome.h"

static LLRegisterPanelClassWrapper<LLPanelHome> t_home("panel_sidetray_home");

LLPanelHome::LLPanelHome() :
	LLPanel(),
	LLViewerMediaObserver(),
	mBrowser(NULL),
	mFirstView(true)
{
}

void LLPanelHome::onOpen(const LLSD& key)
{
	// display the home page the first time we open the panel
	// *NOTE: this seems to happen during login. Can we avoid that?
	if (mFirstView && mBrowser)
	{
		mBrowser->navigateHome();
	}
	mFirstView = false;
}

BOOL LLPanelHome::postBuild()
{
    mBrowser = getChild<LLMediaCtrl>("browser");
    if (mBrowser)
	{
		// read the URL to display from settings.xml
		std::string url = LLViewerHome::getHomeURL();

		mBrowser->addObserver(this);
		mBrowser->setHomePageUrl(url);
	}

    return TRUE;
}

void LLPanelHome::handleMediaEvent(LLPluginClassMedia *self, EMediaEvent event)
{
}
