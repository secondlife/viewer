/**
 * @file   llcapabilityprovider.h
 * @author Nat Goodspeed
 * @date   2009-01-07
 * @brief  Interface by which to reference (e.g.) LLViewerRegion to obtain a
 *         capability.
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

#if ! defined(LL_LLCAPABILITYPROVIDER_H)
#define LL_LLCAPABILITYPROVIDER_H

#include "llhost.h"
#include <string>

/// Interface for obtaining a capability URL, given a capability name
class LLCapabilityProvider
{
public:
    virtual ~LLCapabilityProvider() {}
    /**
     * Get a capability URL, given a capability name. Returns empty string if
     * no such capability is defined on this provider.
     */
    virtual std::string getCapability(const std::string& name) const = 0;
    /**
     * Get host to which to send that capability request.
     */
    virtual const LLHost& getHost() const = 0;
    /**
     * Describe this LLCapabilityProvider for logging etc.
     */
    virtual std::string getDescription() const = 0;
};

#endif /* ! defined(LL_LLCAPABILITYPROVIDER_H) */
