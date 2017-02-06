/** 
 * @file llfloatergridstatus.cpp
 * @brief Grid status floater - uses an embedded web browser to show Grid status info
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

#include "llfloatergridstatus.h"

#include "llhttpconstants.h"
#include "llmediactrl.h"
#include "llviewercontrol.h"


LLFloaterGridStatus::LLFloaterGridStatus(const Params& key) :
	LLFloaterWebContent(key)
{
}

BOOL LLFloaterGridStatus::postBuild()
{
	LLFloaterWebContent::postBuild();
	mWebBrowser->addObserver(this);

	return TRUE;
}

void LLFloaterGridStatus::onOpen(const LLSD& key)
{
	Params p(key);
	p.trusted_content = true;
	p.allow_address_entry = false;

	LLFloaterWebContent::onOpen(p);
	applyPreferredRect();
	if (mWebBrowser)
	{
		std::string url = "http://secondlife-status.statuspage.io/";
		mWebBrowser->navigateTo(url, HTTP_CONTENT_TEXT_HTML);
	}
}
// virtual
void LLFloaterGridStatus::handleReshape(const LLRect& new_rect, bool by_user)
{
	if (by_user && !isMinimized())
	{
		gSavedSettings.setRect("GridStatusFloaterRect", new_rect);
	}

	LLFloaterWebContent::handleReshape(new_rect, by_user);
}

void LLFloaterGridStatus::applyPreferredRect()
{
	const LLRect preferred_rect = gSavedSettings.getRect("GridStatusFloaterRect");

	// Don't override position that may have been set by floater stacking code.
	LLRect new_rect = getRect();
	new_rect.setLeftTopAndSize(
		new_rect.mLeft, new_rect.mTop,
		preferred_rect.getWidth(), preferred_rect.getHeight());
	setShape(new_rect);
}
