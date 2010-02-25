/** 
* @file llpanelhome.cpp
* @author Martin Reddy
* @brief The Home side tray panel
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
* 
* Copyright (c) 2009, Linden Research, Inc.
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
		mBrowser->setTrusted(true);
		mBrowser->setHomePageUrl(url);
	}

    return TRUE;
}

void LLPanelHome::handleMediaEvent(LLPluginClassMedia *self, EMediaEvent event)
{
}
