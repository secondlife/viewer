/**
 * @file lltrustedmessageservice.h
 * @brief LLTrustedMessageService class
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

#ifndef LLTRUSTEDMESSAGESERVICE_H
#define LLTRUSTEDMESSAGESERVICE_H

#include "linden_common.h"
#include "llhttpnode.h"

// These are defined in lliosocket.h/lliosocket.cpp  
extern const std::string CONTEXT_REMOTE_HOST;
extern const std::string CONTEXT_REMOTE_PORT;

class LLSD;

class LLTrustedMessageService
{
public:

    bool validate(const std::string& name, LLSD& context) const;
    
    void post(LLHTTPNode::ResponsePtr response,
              const LLSD& context,
              const LLSD& input) const;
};

#endif // LLTRUSTEDMESSAGESERVICE_H
