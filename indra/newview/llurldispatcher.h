/**
 * @file llurldispatcher.h
 * @brief Central registry for all SL URL handlers
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#ifndef LLURLDISPATCHER_H
#define LLURLDISPATCHER_H

class LLURLDispatcher
{
public:
	static bool isSLURL(const std::string& url);
		// Is this any sort of secondlife:// or sl:// URL?

	static bool isSLURLCommand(const std::string& url);
		// Is this a special secondlife://app/ URL?

	static bool dispatch(const std::string& url);
		// At startup time and on clicks in internal web browsers,
		// teleport, open map, or run requested command.
		// Handles:
		//   secondlife://RegionName/123/45/67/
		//   secondlife://app/agent/3d6181b0-6a4b-97ef-18d8-722652995cf1/show
		//   sl://app/foo/bar
		// Returns true if someone handled the URL.
	static bool dispatchRightClick(const std::string& url);

};

#endif
