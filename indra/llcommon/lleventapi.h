/**
 * @file   lleventapi.h
 * @author Nat Goodspeed
 * @date   2009-10-28
 * @brief  LLEventAPI is the base class for every class that wraps a C++ API
 *         in an event API
 * (see https://wiki.lindenlab.com/wiki/Incremental_Viewer_Automation/Event_API).
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
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
class LLEventAPI: public LLDispatchListener,
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

private:
    std::string mDesc;
};

#endif /* ! defined(LL_LLEVENTAPI_H) */
