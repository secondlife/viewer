/** 
 * @file llfloaterregiondebugconsole.h
 * @author Brad Kittenbrink <brad@lindenlab.com>
 * @brief Quick and dirty console for region debug settings
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010-2010, Linden Research, Inc.
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

#include "llfloaterregiondebugconsole.h"

#include "llagent.h"
#include "llhttpclient.h"
#include "llhttpnode.h"
#include "lllineeditor.h"
#include "lltexteditor.h"
#include "llviewerregion.h"

// Two versions of the sim console API are supported.
//
// SimConsole capability (deprecated):
// This is the initial implementation that is supported by some versions of the
// simulator. It is simple and straight forward, just POST a command and the
// body of the response has the result. This API is deprecated because it
// doesn't allow the sim to use any asynchronous API.
//
// SimConsoleAsync capability:
// This capability replaces the original SimConsole capability. It is similar
// in that the command is POSTed to the SimConsoleAsync cap, but the response
// comes in through the event poll, which gives the simulator more flexibility
// and allows it to perform complex operations without blocking any frames.
//
// We will assume the SimConsoleAsync capability is available, and fall back to
// the SimConsole cap if it is not. The simulator will only support one or the
// other.

namespace
{
	// Signal used to notify the floater of responses from the asynchronous
	// API.
	typedef boost::signals2::signal<
		void (const std::string& output)> console_reply_signal_t;
	console_reply_signal_t sConsoleReplySignal;

	const std::string PROMPT("\n\n> ");
	const std::string UNABLE_TO_SEND_COMMAND(
		"ERROR: The last command was not received by the server.");
	const std::string CONSOLE_UNAVAILABLE(
		"ERROR: No console available for this region/simulator.");
	const std::string CONSOLE_NOT_SUPPORTED(
		"This region does not support the simulator console.");

	// This responder handles the initial response. Unless error() is called
	// we assume that the simulator has received our request. Error will be
	// called if this request times out.
	class AsyncConsoleResponder : public LLHTTPClient::Responder
	{
	public:
		/* virtual */
		void error(U32 status, const std::string& reason)
		{
			sConsoleReplySignal(UNABLE_TO_SEND_COMMAND);
		}
	};

	class ConsoleResponder : public LLHTTPClient::Responder
	{
	public:
		ConsoleResponder(LLTextEditor *output) : mOutput(output)
		{
		}

		/*virtual*/
		void error(U32 status, const std::string& reason)
		{
			if (mOutput)
			{
				mOutput->appendText(
					UNABLE_TO_SEND_COMMAND + PROMPT,
					false);
			}
		}

		/*virtual*/
		void result(const LLSD& content)
		{
			if (mOutput)
			{
				mOutput->appendText(
					content.asString() + PROMPT, false);
			}
		}

		LLTextEditor * mOutput;
	};

	// This handles responses for console commands sent via the asynchronous
	// API.
	class ConsoleResponseNode : public LLHTTPNode
	{
	public:
		/* virtual */
		void post(
			LLHTTPNode::ResponsePtr reponse,
			const LLSD& context,
			const LLSD& input) const
		{
			llinfos << "Received response from the debug console: "
				<< input << llendl;
			sConsoleReplySignal(input["body"].asString());
		}
	};
}

LLFloaterRegionDebugConsole::LLFloaterRegionDebugConsole(LLSD const & key)
: LLFloater(key), mOutput(NULL)
{
	mReplySignalConnection = sConsoleReplySignal.connect(
		boost::bind(
			&LLFloaterRegionDebugConsole::onReplyReceived,
			this,
			_1));
}

LLFloaterRegionDebugConsole::~LLFloaterRegionDebugConsole()
{
	mReplySignalConnection.disconnect();
}

BOOL LLFloaterRegionDebugConsole::postBuild()
{
	LLLineEditor* input = getChild<LLLineEditor>("region_debug_console_input");
	input->setEnableLineHistory(true);
	input->setCommitCallback(boost::bind(&LLFloaterRegionDebugConsole::onInput, this, _1, _2));
	input->setFocus(true);
	input->setCommitOnFocusLost(false);

	mOutput = getChild<LLTextEditor>("region_debug_console_output");

	std::string url = gAgent.getRegion()->getCapability("SimConsoleAsync");
	if (url.empty())
	{
		// Fall back to see if the old API is supported.
		url = gAgent.getRegion()->getCapability("SimConsole");
		if (url.empty())
		{
			mOutput->appendText(
				CONSOLE_NOT_SUPPORTED + PROMPT,
				false);
			return TRUE;
		}
	}

	mOutput->appendText("> ", false);
	return TRUE;
}

void LLFloaterRegionDebugConsole::onInput(LLUICtrl* ctrl, const LLSD& param)
{
	LLLineEditor* input = static_cast<LLLineEditor*>(ctrl);
	std::string text = input->getText() + "\n";

	std::string url = gAgent.getRegion()->getCapability("SimConsoleAsync");
	if (url.empty())
	{
		// Fall back to the old API
		url = gAgent.getRegion()->getCapability("SimConsole");
		if (url.empty())
		{
			text += CONSOLE_UNAVAILABLE + PROMPT;
		}
		else
		{
			// Using SimConsole (deprecated)
			LLHTTPClient::post(
				url,
				LLSD(input->getText()),
				new ConsoleResponder(mOutput));
		}
	}
	else
	{
		// Using SimConsoleAsync
		LLHTTPClient::post(
			url,
			LLSD(input->getText()),
			new AsyncConsoleResponder);
	}

	mOutput->appendText(text, false);
	input->clear();
}

void LLFloaterRegionDebugConsole::onReplyReceived(const std::string& output)
{
	mOutput->appendText(output + PROMPT, false);
}

LLHTTPRegistration<ConsoleResponseNode>
	gHTTPRegistrationMessageDebugConsoleResponse(
		"/message/SimConsoleResponse");
