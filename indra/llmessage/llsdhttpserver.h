/** 
 * @file llsdhttpserver.h
 * @brief Standard LLSD services
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSDHTTPSERVER_H
#define LL_LLSDHTTPSERVER_H

/** 
 * This module implements and defines common services that should be included
 * in all server URL trees.  These services facilitate debugging and
 * introsepction.
 */

class LLHTTPStandardServices
{
public:
	static void useServices();
		/**< 
			Having a call to this function causes the following services to be
			registered:
				/web/echo			-- echo input
				/web/hello			-- return "hello"
				/web/server/api		-- return a list of url paths on the server
				/web/server/api/<..path..>
									-- return description of the path
		*/
};

#endif // LL_LLSDHTTPSERVER_H
