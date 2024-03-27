/**
 * @file llcommandhandler.cpp
 * @brief Central registry for text-driven "commands", most of
 * which manipulate user interface.  For example, the command
 * "agent (uuid) about" will open the UI for an avatar's profile.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llcommandhandler.h"
#include "llnotificationsutil.h"
#include "llcommanddispatcherlistener.h"
#include "llstartup.h"
#include "stringize.h"

// system includes
#include <boost/tokenizer.hpp>

#define THROTTLE_PERIOD    5    // required seconds between throttled commands

static LLCommandDispatcherListener sCommandDispatcherListener;
const std::string LLCommandHandler::NAV_TYPE_CLICKED = "clicked";
const std::string LLCommandHandler::NAV_TYPE_EXTERNAL = "external";
const std::string LLCommandHandler::NAV_TYPE_NAVIGATED = "navigated";

//---------------------------------------------------------------------------
// Underlying registry for command handlers, not directly accessible.
//---------------------------------------------------------------------------
struct LLCommandHandlerInfo
{
	LLCommandHandler::EUntrustedAccess mUntrustedBrowserAccess;
	LLCommandHandler* mHandler;	// safe, all of these are static objects
};

class LLCommandHandlerRegistry
{
public:
	static LLCommandHandlerRegistry& instance();
	void add(const char* cmd,
			 LLCommandHandler::EUntrustedAccess untrusted_access,
			 LLCommandHandler* handler);
	bool dispatch(const std::string& cmd,
				  const LLSD& params,
				  const LLSD& query_map,
                  const std::string& grid,
				  LLMediaCtrl* web,
				  const std::string& nav_type,
				  bool trusted_browser);

private:
    void notifySlurlBlocked();
    void notifySlurlThrottled();

	friend LLSD LLCommandDispatcher::enumerate();
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

void LLCommandHandlerRegistry::add(const char* cmd,
								   LLCommandHandler::EUntrustedAccess untrusted_access,
								   LLCommandHandler* handler)
{
	LLCommandHandlerInfo info;
	info.mUntrustedBrowserAccess = untrusted_access;
	info.mHandler = handler;

	mMap[cmd] = info;
}

bool LLCommandHandlerRegistry::dispatch(const std::string& cmd,
										const LLSD& params,
										const LLSD& query_map,
										const std::string& grid,
										LLMediaCtrl* web,
										const std::string& nav_type,
										bool trusted_browser)
{
	static F64 last_throttle_time = 0.0;
	F64 cur_time = 0.0;
	std::map<std::string, LLCommandHandlerInfo>::iterator it = mMap.find(cmd);
	if (it == mMap.end()) return false;
	const LLCommandHandlerInfo& info = it->second;
	if (!trusted_browser)
	{
		switch (info.mUntrustedBrowserAccess)
		{
		case LLCommandHandler::UNTRUSTED_ALLOW:
			// fall through and let the command be handled
			break;

		case LLCommandHandler::UNTRUSTED_BLOCK:
			// block request from external browser, but report as
			// "handled" because it was well formatted.
			LL_WARNS_ONCE("SLURL") << "Blocked SLURL command from untrusted browser" << LL_ENDL;
            notifySlurlBlocked();
			return true;

        case LLCommandHandler::UNTRUSTED_CLICK_ONLY:
            if (nav_type == LLCommandHandler::NAV_TYPE_CLICKED
                && info.mHandler->canHandleUntrusted(params, query_map, web, nav_type))
            {
                break;
            }
            LL_WARNS_ONCE("SLURL") << "Blocked SLURL click-only command " << cmd << " from untrusted browser" << LL_ENDL;
            notifySlurlBlocked();
            return true;

		case LLCommandHandler::UNTRUSTED_THROTTLE:
			//skip initial request from external browser before STATE_BROWSER_INIT
			if (LLStartUp::getStartupState() == STATE_FIRST)
			{
				return true;
			}
            if (!info.mHandler->canHandleUntrusted(params, query_map, web, nav_type))
            {
                LL_WARNS_ONCE("SLURL") << "Blocked SLURL command from untrusted browser" << LL_ENDL;
                notifySlurlBlocked();
                return true;
            }
			// if users actually click on a link, we don't need to throttle it
			// (throttling mechanism is used to prevent an avalanche of clicks via
			// javascript
			if (nav_type == LLCommandHandler::NAV_TYPE_CLICKED)
			{
				break;
			}
			cur_time = LLTimer::getElapsedSeconds();
			if (cur_time < last_throttle_time + THROTTLE_PERIOD)
			{
				// block request from external browser if it happened
				// within THROTTLE_PERIOD seconds of the last command
				LL_WARNS_ONCE("SLURL") << "Throttled SLURL command from untrusted browser" << LL_ENDL;
                notifySlurlThrottled();
				return true;
			}
			last_throttle_time = cur_time;
			break;
		}
	}
	if (!info.mHandler) return false;
	return info.mHandler->handle(params, query_map, grid, web);
}

void LLCommandHandlerRegistry::notifySlurlBlocked()
{
    static bool slurl_blocked = false;
    if (!slurl_blocked)
    {
        if (LLStartUp::getStartupState() >= STATE_BROWSER_INIT)
        {
            // Note: commands can arrive before we initialize everything we need for Notification.
            LLNotificationsUtil::add("BlockedSLURL");
        }
        slurl_blocked = true;
    }
}

void LLCommandHandlerRegistry::notifySlurlThrottled()
{
    static bool slurl_throttled = false;
    if (!slurl_throttled)
    {
        if (LLStartUp::getStartupState() >= STATE_BROWSER_INIT)
        {
            // Note: commands can arrive before we initialize everything we need for Notification.
            LLNotificationsUtil::add("ThrottledSLURL");
        }
        slurl_throttled = true;
    }
}

//---------------------------------------------------------------------------
// Automatic registration of commands, runs before main()
//---------------------------------------------------------------------------

LLCommandHandler::LLCommandHandler(const char* cmd,
								   EUntrustedAccess untrusted_access)
{
	LLCommandHandlerRegistry::instance().add(cmd, untrusted_access, this);
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
								   const std::string& grid,
								   LLMediaCtrl* web,
								   const std::string& nav_type,
								   bool trusted_browser)
{
	return LLCommandHandlerRegistry::instance().dispatch(
		cmd, params, query_map, grid, web, nav_type, trusted_browser);
}

static std::string lookup(LLCommandHandler::EUntrustedAccess value);

LLSD LLCommandDispatcher::enumerate()
{
	LLSD response;
	LLCommandHandlerRegistry& registry(LLCommandHandlerRegistry::instance());
	for (std::map<std::string, LLCommandHandlerInfo>::const_iterator chi(registry.mMap.begin()),
																	 chend(registry.mMap.end());
		 chi != chend; ++chi)
	{
		LLSD info;
		info["untrusted"] = chi->second.mUntrustedBrowserAccess;
		info["untrusted_str"] = lookup(chi->second.mUntrustedBrowserAccess);
		response[chi->first] = info;
	}
	return response;
}

/*------------------------------ lookup stuff ------------------------------*/
struct symbol_info
{
	const char* name;
	LLCommandHandler::EUntrustedAccess value;
};

#define ent(SYMBOL)										\
	{													\
		&#SYMBOL[28], /* skip "LLCommandHandler::UNTRUSTED_" prefix */	\
		SYMBOL											\
	}

symbol_info symbols[] =
{
	ent(LLCommandHandler::UNTRUSTED_ALLOW),		  // allow commands from untrusted browsers
	ent(LLCommandHandler::UNTRUSTED_BLOCK),		  // ignore commands from untrusted browsers
    ent(LLCommandHandler::UNTRUSTED_CLICK_ONLY),  // allow untrusted, but only if clicked
	ent(LLCommandHandler::UNTRUSTED_THROTTLE)	  // allow untrusted, but only a few per min.
};

#undef ent

static std::string lookup(LLCommandHandler::EUntrustedAccess value)
{
	for (symbol_info *sii(symbols), *siend(symbols + (sizeof(symbols)/sizeof(symbols[0])));
		 sii != siend; ++sii)
	{
		if (sii->value == value)
		{
			return sii->name;
		}
	}
	return STRINGIZE("UNTRUSTED_" << value);
}
