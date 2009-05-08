/**
 * @file   llcapabilityprovider.h
 * @author Nat Goodspeed
 * @date   2009-01-07
 * @brief  Interface by which to reference (e.g.) LLViewerRegion to obtain a
 *         capability.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
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
    virtual LLHost getHost() const = 0;
    /**
     * Describe this LLCapabilityProvider for logging etc.
     */
    virtual std::string getDescription() const = 0;
};

#endif /* ! defined(LL_LLCAPABILITYPROVIDER_H) */
