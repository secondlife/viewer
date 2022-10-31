/** 
 * @file llsdhttpserver.h
 * @brief Standard LLSD services
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
                /web/echo           -- echo input
                /web/hello          -- return "hello"
                /web/server/api     -- return a list of url paths on the server
                /web/server/api/<..path..>
                                    -- return description of the path
        */
};

#endif // LL_LLSDHTTPSERVER_H
