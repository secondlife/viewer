/** 
 * @file lscript_http.h
 * @brief LSL HTTP keys
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

// Keys used in LSL HTTP function <key,value> pair lists.

#ifndef LSCRIPT_HTTP_H
#define LSCRIPT_HTTP_H

enum LLScriptHTTPRequestParameterKey
{
  HTTP_METHOD,
  HTTP_MIMETYPE,
  HTTP_BODY_MAXLENGTH,
  HTTP_VERIFY_CERT
};

enum LLScriptHTTPResponseMetadataKey
{
	HTTP_BODY_TRUNCATED
};

#endif
