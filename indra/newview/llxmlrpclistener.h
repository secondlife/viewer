/**
 * @file   llxmlrpclistener.h
 * @author Nat Goodspeed
 * @date   2009-03-18
 * @brief  LLEventPump API for LLXMLRPCTransaction. This header doesn't
 *         actually define the API; the API is defined by the pump name on
 *         which this class listens, and by the expected content of LLSD it
 *         receives.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#if ! defined(LL_LLXMLRPCLISTENER_H)
#define LL_LLXMLRPCLISTENER_H

#include "llevents.h"

/// Listen on an LLEventPump with specified name for LLXMLRPCTransaction
/// request events.
class LLXMLRPCListener
{
public:
    /// Specify the pump name on which to listen
    LLXMLRPCListener(const std::string& pumpname);

    /// Handle request events on the event pump specified at construction time
    bool process(const LLSD& command);

private:
    LLTempBoundListener mBoundListener;
};

#endif /* ! defined(LL_LLXMLRPCLISTENER_H) */
