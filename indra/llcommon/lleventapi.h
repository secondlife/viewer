/**
 * @file   lleventapi.h
 * @author Nat Goodspeed
 * @date   2009-10-28
 * @brief  LLEventAPI is the base class for every class that wraps a C++ API
 *         in an event API
 * (see https://wiki.lindenlab.com/wiki/Incremental_Viewer_Automation/Event_API).
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

#if ! defined(LL_LLEVENTAPI_H)
#define LL_LLEVENTAPI_H

#include "lleventdispatcher.h"
#include "llinstancetracker.h"
#include <string>

/**
 * LLEventAPI not only provides operation dispatch functionality, inherited
 * from LLDispatchListener -- it also gives us event API introspection.
 * Deriving from LLInstanceTracker lets us enumerate instances.
 */
class LL_COMMON_API LLEventAPI: public LLDispatchListener,
                  public LLInstanceTracker<LLEventAPI, std::string>
{
    typedef LLDispatchListener lbase;
    typedef LLInstanceTracker<LLEventAPI, std::string> ibase;

public:
    /**
     * @param name LLEventPump name on which this LLEventAPI will listen. This
     * also serves as the LLInstanceTracker instance key.
     * @param desc Documentation string shown to a client trying to discover
     * available event APIs.
     * @param field LLSD::Map key used by LLDispatchListener to look up the
     * subclass method to invoke [default "op"].
     */
    LLEventAPI(const std::string& name, const std::string& desc, const std::string& field="op");
    virtual ~LLEventAPI();

    /// Get the string name of this LLEventAPI
    std::string getName() const { return ibase::getKey(); }
    /// Get the documentation string
    std::string getDesc() const { return mDesc; }

    /**
     * Publish only selected add() methods from LLEventDispatcher.
     * Every LLEventAPI add() @em must have a description string.
     */
    template <typename CALLABLE>
    void add(const std::string& name,
             const std::string& desc,
             CALLABLE callable,
             const LLSD& required=LLSD())
    {
        LLEventDispatcher::add(name, desc, callable, required);
    }

private:
    std::string mDesc;
};

#endif /* ! defined(LL_LLEVENTAPI_H) */
