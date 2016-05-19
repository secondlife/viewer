/**
 * @file llcommandhandler.h
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
#ifndef LLCOMMANDHANDLER_H
#define LLCOMMANDHANDLER_H

#include "llsd.h"

/* Example:  secondlife:///app/foo/<uuid>
   Command "foo" that takes one parameter, a UUID.

class LLFooHandler : public LLCommandHandler
{
public:
    // Inform the system you handle commands starting
	// with "foo" and they are only allowed from
	// "trusted" (pointed at Linden content) browsers
	LLFooHandler() : LLCommandHandler("foo", UNTRUSTED_BLOCK) { }

    // Your code here
	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		if (tokens.size() < 1) return false;
		LLUUID id( tokens[0] );
		return do_foo(id);
	}
};

// *NOTE: Creating the object registers with the dispatcher.
LLFooHandler gFooHandler;

*/

class LLMediaCtrl;

class LLCommandHandler
{
public:
	enum EUntrustedAccess
	{
		UNTRUSTED_ALLOW,       // allow commands from untrusted browsers
		UNTRUSTED_BLOCK,       // ignore commands from untrusted browsers
		UNTRUSTED_THROTTLE     // allow untrusted, but only a few per min.
	};

	LLCommandHandler(const char* command, EUntrustedAccess untrusted_access);
		// Automatically registers object to get called when 
		// command is executed.  All commands can be processed
		// in links from LLMediaCtrl, but some (like teleport)
		// should not be allowed from outside the app.
		
	virtual ~LLCommandHandler();

	virtual bool handle(const LLSD& params,
						const LLSD& query_map,
						LLMediaCtrl* web) = 0;
		// For URL secondlife:///app/foo/bar/baz?cat=1&dog=2
		// @params - array of "bar", "baz", possibly empty
		// @query_map - map of "cat" -> 1, "dog" -> 2, possibly empty
		// @web - pointer to web browser control, possibly NULL
		// Return true if you did something, false if the parameters
		// are invalid or on error.
};


class LLCommandDispatcher
{
public:
	static bool dispatch(const std::string& cmd,
						 const LLSD& params,
						 const LLSD& query_map,
						 LLMediaCtrl* web,
						 const std::string& nav_type,
						 bool trusted_browser);
		// Execute a command registered via the above mechanism,
		// passing string parameters.
		// Returns true if command was found and executed correctly.
	/// Return an LLSD::Map of registered LLCommandHandlers and associated
	/// info (e.g. EUntrustedAccess).
	static LLSD enumerate();
};

#endif
