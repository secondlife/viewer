/** 
 * @file llpanelpostprogress.cpp
 * @brief Displays progress of publishing a snapshot.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the termsllpanelpostprogress of the GNU Lesser General Public
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

#include "llfloaterreg.h"
#include "llpanel.h"
#include "llsidetraypanelcontainer.h"

/**
 * Displays progress of publishing a snapshot.
 */
class LLPanelPostProgress
:	public LLPanel
{
	LOG_CLASS(LLPanelPostProgress);

public:
	/*virtual*/ void onOpen(const LLSD& key);
};

static LLRegisterPanelClassWrapper<LLPanelPostProgress> panel_class("llpanelpostprogress");

// virtual
void LLPanelPostProgress::onOpen(const LLSD& key)
{
	if (key.has("post-type"))
	{
		std::string progress_text = getString(key["post-type"].asString() + "_" + "progress_str");
		getChild<LLTextBox>("progress_lbl")->setText(progress_text);
	}
	else
	{
		llwarns << "Invalid key" << llendl;
	}
}
