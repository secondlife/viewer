/**
 * @file llcommandhandler.cpp
 * @brief Central registry for text-driven "commands", most of
 * which manipulate user interface.  For example, the command
 * "agent (uuid) about" will open the UI for an avatar's profile.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llcommandhandler.h"

// system includes
#include <boost/tokenizer.hpp>

//---------------------------------------------------------------------------
// Underlying registry for command handlers, not directly accessible.
//---------------------------------------------------------------------------
struct LLCommandHandlerInfo
{
	bool mRequireTrustedBrowser;
	LLCommandHandler* mHandler;	// safe, all of these are static objects
};

class LLCommandHandlerRegistry
{
public:
	static LLCommandHandlerRegistry& instance();
	void add(const char* cmd, bool require_trusted_browser, LLCommandHandler* handler);
	bool dispatch(const std::string& cmd,
				  const LLSD& params,
				  const LLSD& query_map,
				  LLWebBrowserCtrl* web,
				  bool trusted_browser);

private:
	std::map<std::string, LLCommandHandlerInfo> mMap;
};

// static 
LLCommandHandlerRegistry& LLCommandHandlerRegistry::instance()
{
	// Force this to be initialized on first call, because we're going
	// to be adding items to the std::map before main() and we can't
	// rely on a global being initialized in the right order.
	static LLCommandHandlerRegistry instance;
	return instance;
}

void LLCommandHandlerRegistry::add(const char* cmd, bool require_trusted_browser, LLCommandHandler* handler)
{
	LLCommandHandlerInfo info;
	info.mRequireTrustedBrowser = require_trusted_browser;
	info.mHandler = handler;

	mMap[cmd] = info;
}

bool LLCommandHandlerRegistry::dispatch(const std::string& cmd,
										const LLSD& params,
										const LLSD& query_map,
										LLWebBrowserCtrl* web,
										bool trusted_browser)
{
	std::map<std::string, LLCommandHandlerInfo>::iterator it = mMap.find(cmd);
	if (it == mMap.end()) return false;
	const LLCommandHandlerInfo& info = it->second;
	if (!trusted_browser && info.mRequireTrustedBrowser)
	{
		// block request from external browser, but report as
		// "handled" because it was well formatted.
		LL_WARNS_ONCE("SLURL") << "Blocked SLURL command from untrusted browser" << LL_ENDL;
		return true;
	}
	if (!info.mHandler) return false;
	return info.mHandler->handle(params, query_map, web);
}

//---------------------------------------------------------------------------
// Automatic registration of commands, runs before main()
//---------------------------------------------------------------------------

LLCommandHandler::LLCommandHandler(const char* cmd,
								   bool require_trusted_browser)
{
	LLCommandHandlerRegistry::instance().add(
			cmd, require_trusted_browser, this);
}

LLCommandHandler::~LLCommandHandler()
{
	// Don't care about unregistering these, all the handlers
	// should be static objects.
}

//---------------------------------------------------------------------------
// Public interface
//---------------------------------------------------------------------------

// static
bool LLCommandDispatcher::dispatch(const std::string& cmd,
								   const LLSD& params,
								   const LLSD& query_map,
								   LLWebBrowserCtrl* web,
								   bool trusted_browser)
{
	return LLCommandHandlerRegistry::instance().dispatch(
		cmd, params, query_map, web, trusted_browser);
}
