/**
 * @file   llareslistener.cpp
 * @author Nat Goodspeed
 * @date   2009-03-18
 * @brief  Implementation for llareslistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llareslistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llares.h"
#include "llerror.h"
#include "llevents.h"
#include "llsdutil.h"

LLAresListener::LLAresListener(const std::string& pumpname, LLAres* llares):
    LLDispatchListener(pumpname, "op"),
    mAres(llares)
{
    // add() every method we want to be able to invoke via this event API.
    // Optional third parameter validates expected LLSD request structure.
    add("rewriteURI", &LLAresListener::rewriteURI,
        LLSD().insert("uri", LLSD()).insert("reply", LLSD()));
}

/// This UriRewriteResponder subclass packages returned URIs as an LLSD
/// array to send back to the requester.
class UriRewriteResponder: public LLAres::UriRewriteResponder
{
public:
    /**
     * Specify the request, containing the event pump name on which to send
     * the reply.
     */
    UriRewriteResponder(const LLSD& request):
        mReqID(request),
        mPumpName(request["reply"])
    {}

    /// Called by base class with results. This is called in both the
    /// success and error cases. On error, the calling logic passes the
    /// original URI.
    virtual void rewriteResult(const std::vector<std::string>& uris)
    {
        LLSD result;
        for (std::vector<std::string>::const_iterator ui(uris.begin()), uend(uris.end());
             ui != uend; ++ui)
        {
            result.append(*ui);
        }
        // This call knows enough to avoid trying to insert a map key into an
        // LLSD array. It's there so that if, for any reason, we ever decide
        // to change the response from array to map, it will Just Start Working.
        mReqID.stamp(result);
        LLEventPumps::instance().obtain(mPumpName).post(result);
    }

private:
    LLReqID mReqID;
    const std::string mPumpName;
};

void LLAresListener::rewriteURI(const LLSD& data)
{
    mAres->rewriteURI(data["uri"], new UriRewriteResponder(data));
}
