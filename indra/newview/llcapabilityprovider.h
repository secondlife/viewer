/**
 * @file   llcapabilityprovider.h
 * @author Nat Goodspeed
 * @date   2009-01-07
 * @brief  Interface by which to reference (e.g.) LLViewerRegion to obtain a
 *         capability.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
