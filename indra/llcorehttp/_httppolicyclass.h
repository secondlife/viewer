/**
 * @file _httppolicyclass.h
 * @brief Declarations for internal class defining policy class options.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2013, Linden Research, Inc.
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

#ifndef _LLCORE_HTTP_POLICY_CLASS_H_
#define _LLCORE_HTTP_POLICY_CLASS_H_


#include "httprequest.h"


namespace LLCore
{

/// Options struct for per-class policy options.
///
/// Combines both raw blob data access with semantics-enforcing
/// set/get interfaces.  For internal operations by the worker
/// thread, just grab the setting directly from instance and test/use
/// as needed.  When attached to external APIs (the public API
/// options interfaces) the set/get methods are available to
/// enforce correct ranges, data types, contexts, etc. and suitable
/// status values are returned.
///
/// Threading:  Single-threaded.  In practice, init thread before
/// worker starts, worker thread after.
class HttpPolicyClass
{
public:
    HttpPolicyClass();
    ~HttpPolicyClass();

    HttpPolicyClass & operator=(const HttpPolicyClass &);
    HttpPolicyClass(const HttpPolicyClass &);           // Not defined

public:
    HttpStatus set(HttpRequest::EPolicyOption opt, long value);
    HttpStatus get(HttpRequest::EPolicyOption opt, long * value) const;
    
public:
    long                        mConnectionLimit;
    long                        mPerHostConnectionLimit;
    long                        mPipelining;
    long                        mThrottleRate;
};  // end class HttpPolicyClass

}  // end namespace LLCore

#endif // _LLCORE_HTTP_POLICY_CLASS_H_
