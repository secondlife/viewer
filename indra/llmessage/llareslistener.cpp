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

#if LL_WINDOWS
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

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

LLAresListener::LLAresListener(const std::string& pumpname, LLAres* llares):
    mAres(llares),
    mBoundListener(LLEventPumps::instance().
                   obtain(pumpname).
                   listen("LLAresListener", boost::bind(&LLAresListener::process, this, _1)))
{
    mDispatch["rewriteURI"] = boost::bind(&LLAresListener::rewriteURI, this, _1);
}

bool LLAresListener::process(const LLSD& command)
{
    const std::string op(command["op"]);
    // Look up the requested operation.
    DispatchMap::const_iterator found = mDispatch.find(op);
    if (found == mDispatch.end())
    {
        // There's no feedback other than our own reply. If somebody asks
        // for an operation that's not supported (perhaps because of a
        // typo?), unless we holler loudly, the request will be silently
        // ignored. Throwing a tantrum on such errors will hopefully make
        // this product more robust.
        LL_ERRS("LLAresListener") << "Unsupported request " << op << LL_ENDL;
        return false;
    }
    // Having found the operation, call it.
    found->second(command);
    // Conventional LLEventPump listener return
    return false;
}

/// This UriRewriteResponder subclass packages returned URIs as an LLSD
/// array to send back to the requester.
class UriRewriteResponder: public LLAres::UriRewriteResponder
{
public:
    /// Specify the event pump name on which to send the reply
    UriRewriteResponder(const std::string& pumpname):
        mPumpName(pumpname)
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
        LLEventPumps::instance().obtain(mPumpName).post(result);
    }

private:
    const std::string mPumpName;
};

void LLAresListener::rewriteURI(const LLSD& data)
{
    const std::string uri(data["uri"]);
    const std::string reply(data["reply"]);
    // Validate that the request is well-formed
    if (uri.empty() || reply.empty())
    {
        LL_ERRS("LLAresListener") << "rewriteURI request missing";
        std::string separator;
        if (uri.empty())
        {
            LL_CONT << " 'uri'";
            separator = " and";
        }
        if (reply.empty())
        {
            LL_CONT << separator << " 'reply'";
        }
        LL_CONT << LL_ENDL;
    }
    // Looks as though we have what we need; issue the request
    mAres->rewriteURI(uri, new UriRewriteResponder(reply));
}
