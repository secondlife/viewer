/**
 * @file   llareslistener.h
 * @author Nat Goodspeed
 * @date   2009-03-18
 * @brief  LLEventPump API for LLAres. This header doesn't actually define the
 *         API; the API is defined by the pump name on which this class
 *         listens, and by the expected content of LLSD it receives.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLARESLISTENER_H)
#define LL_LLARESLISTENER_H

#include <string>
#include <map>
#include <boost/function.hpp>
#include "llevents.h"

class LLAres;
class LLSD;

/// Listen on an LLEventPump with specified name for LLAres request events.
class LLAresListener
{
public:
    /// Specify the pump name on which to listen, and bind the LLAres instance
    /// to use (e.g. gAres)
    LLAresListener(const std::string& pumpname, LLAres* llares);

    /// Handle request events on the event pump specified at construction time
    bool process(const LLSD& command);

private:
    /// command["op"] == "rewriteURI" 
    void rewriteURI(const LLSD& data);

    typedef boost::function<void(const LLSD&)> Callable;
    typedef std::map<std::string, Callable> DispatchMap;
    DispatchMap mDispatch;
    LLTempBoundListener mBoundListener;
    LLAres* mAres;
};

#endif /* ! defined(LL_LLARESLISTENER_H) */
