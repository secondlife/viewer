/** 
 * @file llpanelpostresult.cpp
 * @brief Result of publishing a snapshot (success/failure).
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloaterreg.h"
#include "llpanel.h"
#include "llsidetraypanelcontainer.h"

/**
 * Displays snapshot publishing result.
 */
class LLPanelPostResult
:	public LLPanel
{
	LOG_CLASS(LLPanelPostResult);

public:
	LLPanelPostResult();

	/*virtual*/ void onOpen(const LLSD& key);
private:
	void onBack();
	void onClose();
};

static LLRegisterPanelClassWrapper<LLPanelPostResult> panel_class("llpanelpostresult");

LLPanelPostResult::LLPanelPostResult()
{
	mCommitCallbackRegistrar.add("Snapshot.Result.Back",	boost::bind(&LLPanelPostResult::onBack,		this));
	mCommitCallbackRegistrar.add("Snapshot.Result.Close",	boost::bind(&LLPanelPostResult::onClose,	this));
}


// virtual
void LLPanelPostResult::onOpen(const LLSD& key)
{
	if (key.isMap() && key.has("post-result") && key.has("post-type"))
	{
		bool ok = key["post-result"].asBoolean();
		std::string type = key["post-type"].asString();
		std::string result_text = getString(type + "_" + (ok ? "succeeded_str" : "failed_str"));
		getChild<LLTextBox>("result_lbl")->setText(result_text);
	}
	else
	{
		llwarns << "Invalid key" << llendl;
	}
}

void LLPanelPostResult::onBack()
{
	LLSideTrayPanelContainer* parent = dynamic_cast<LLSideTrayPanelContainer*>(getParent());
	if (!parent)
	{
		llwarns << "Cannot find panel container" << llendl;
		return;
	}

	parent->openPreviousPanel();
}

void LLPanelPostResult::onClose()
{
	LLFloaterReg::hideInstance("snapshot");
}
