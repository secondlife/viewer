/** 
 * @file llfloatersearch.cpp
 * @author Martin Reddy
 * @brief Search floater - uses an embedded web browser control
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
#include "llfloatersearch.h"
#include "llmediactrl.h"


LLFloaterSearch::LLFloaterSearch(const LLSD& key) :
	LLFloater(key),
	mBrowser(NULL)
{
}

BOOL LLFloaterSearch::postBuild()
{
	mBrowser = getChild<LLMediaCtrl>("browser");
	if (mBrowser)
	{
		mBrowser->addObserver(this);
		mBrowser->setTrusted(true);
		mBrowser->setHomePageUrl(getString("search_url"));
	}

	return TRUE;
}

void LLFloaterSearch::onOpen(const LLSD& key)
{
	search(key);
}

void LLFloaterSearch::handleMediaEvent(LLPluginClassMedia *self, EMediaEvent event)
{
	switch (event) 
	{
	case MEDIA_EVENT_NAVIGATE_BEGIN:
		childSetText("status_text", getString("loading_text"));
		break;
		
	case MEDIA_EVENT_NAVIGATE_COMPLETE:
		childSetText("status_text", getString("done_text"));
		break;
		
	default:
		break;
	}
}

void LLFloaterSearch::search(const LLSD &key)
{
	if (mBrowser)
	{
		std::string query = getString("search_url");
		if (key.has("id"))
		{
			query += std::string("?q=") + key["id"].asString();
		}
		mBrowser->navigateTo(query);
	}
}
