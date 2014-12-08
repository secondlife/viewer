/** 
* @file llfloaterhoverheight.cpp
* @brief Controller for self avatar hover height
* @author vir@lindenlab.com
*
* $LicenseInfo:firstyear=2014&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2014, Linden Research, Inc.
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

#include "llfloaterhoverheight.h"
#include "llsliderctrl.h"
#include "llviewercontrol.h"
#include "llsdserialize.h"
#include "llhttpclient.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llvoavatarself.h"

class LLHoverHeightResponder: public LLHTTPClient::Responder
{
public:
	LLHoverHeightResponder(): LLHTTPClient::Responder() {}

private:
	void httpFailure()
	{
		LL_WARNS() << dumpResponse() << LL_ENDL;
	}

	void httpSuccess()
	{
		LL_INFOS() << dumpResponse() << LL_ENDL;
	}
};

LLFloaterHoverHeight::LLFloaterHoverHeight(const LLSD& key) : LLFloater(key)
{
}

BOOL LLFloaterHoverHeight::postBuild()
{

	LLVector3 offset = gSavedSettings.getVector3("AvatarPosFinalOffset");
	F32 value = offset[2];

	LLSliderCtrl* sldrCtrl = getChild<LLSliderCtrl>("HoverHeightSlider");
	sldrCtrl->setValue(value,FALSE);
	sldrCtrl->setSliderMouseUpCallback(boost::bind(&LLFloaterHoverHeight::onFinalCommit,this));
	sldrCtrl->setSliderEditorCommitCallback(boost::bind(&LLFloaterHoverHeight::onFinalCommit,this));
	childSetCommitCallback("HoverHeightSlider", &LLFloaterHoverHeight::onSliderMoved, NULL);

	return TRUE;
}

// static
void LLFloaterHoverHeight::onSliderMoved(LLUICtrl* ctrl, void* userData)
{
}

// Do extra send-to-the-server work when slider drag completes, or new
// value entered as text.
void LLFloaterHoverHeight::onFinalCommit()
{
	LL_INFOS() << "FINAL FINAL!!!" << LL_ENDL;
	sendHoverHeightUpdate();
}

void LLFloaterHoverHeight::sendHoverHeightUpdate()
{
	LLSliderCtrl* sldrCtrl = getChild<LLSliderCtrl>("HoverHeightSlider");
	F32 value = sldrCtrl->getValueF32();

	std::string url = gAgent.getRegion()->getCapability("AgentPreferences");

	if (!url.empty())
	{
		LLSD update = LLSD::emptyMap();
		update["hover_height"] = value;

		LL_INFOS() << "updating hover height to " << value << LL_ENDL;
		LLHTTPClient::post(url, update, new LLHoverHeightResponder);
	}
}
