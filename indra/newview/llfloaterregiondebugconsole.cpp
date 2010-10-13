/** 
 * @file llfloaterregiondebugconsole.h
 * @author Brad Kittenbrink <brad@lindenlab.com>
 * @brief Quick and dirty console for region debug settings
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llfloaterregiondebugconsole.h"

#include "llagent.h"
#include "llhttpclient.h"
#include "lllineeditor.h"
#include "lltexteditor.h"
#include "llviewerregion.h"

class Responder : public LLHTTPClient::Responder {
public:
    Responder(LLTextEditor *output) : mOutput(output)
    {
    }

    /*virtual*/
    void error(U32 status, const std::string& reason)
    {
    }

    /*virtual*/
    void result(const LLSD& content)
    {
		std::string text = content.asString() + "\n\n> ";
		mOutput->appendText(text, false);
    };

    LLTextEditor * mOutput;
};

LLFloaterRegionDebugConsole::LLFloaterRegionDebugConsole(LLSD const & key)
: LLFloater(key), mOutput(NULL)
{
}

BOOL LLFloaterRegionDebugConsole::postBuild()
{
	LLLineEditor* input = getChild<LLLineEditor>("region_debug_console_input");
	input->setEnableLineHistory(true);
	input->setCommitCallback(boost::bind(&LLFloaterRegionDebugConsole::onInput, this, _1, _2));
	input->setFocus(true);
	input->setCommitOnFocusLost(false);

	mOutput = getChild<LLTextEditor>("region_debug_console_output");

	std::string url = gAgent.getRegion()->getCapability("SimConsole");
	if ( url.size() == 0 )
	{
		mOutput->appendText("This region does not support the simulator console.\n\n> ", false);
	}
	else
	{
		mOutput->appendText("> ", false);
	}
	

	return TRUE;
}

void LLFloaterRegionDebugConsole::onInput(LLUICtrl* ctrl, const LLSD& param)
{
	LLLineEditor* input = static_cast<LLLineEditor*>(ctrl);
	std::string text = input->getText() + "\n";


    std::string url = gAgent.getRegion()->getCapability("SimConsole");

	if ( url.size() > 0 )
	{
		LLHTTPClient::post(url, LLSD(input->getText()), new ::Responder(mOutput));
	}
	else
	{
		text += "\nError: No console available for this region/simulator.\n\n> ";
	}

	mOutput->appendText(text, false);

	input->clear();
}

