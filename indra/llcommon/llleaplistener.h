/**
 * @file   llleaplistener.h
 * @author Nat Goodspeed
 * @date   2012-03-16
 * @brief  LLEventAPI supporting LEAP plugins
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLLEAPLISTENER_H)
#define LL_LLLEAPLISTENER_H

#include "lleventapi.h"
#include <functional>
#include <map>
#include <string>

/// Listener class implementing LLLeap query/control operations.
/// See https://jira.lindenlab.com/jira/browse/DEV-31978.
class LLLeapListener: public LLEventAPI
{
public:
    /**
     * Certain LLLeapListener operations listen on a specified LLEventPump.
     * Accept a bool(pump, data) callback from our caller for when any such
     * event is received.
     */
    using Callback = std::function<bool(const std::string& pump, const LLSD& data)>;
    LLLeapListener(std::string_view caller, const Callback& callback);
    ~LLLeapListener();

    LLEventPump& getReplyPump() { return mReplyPump; }

    static LLSD getFeatures();

private:
    void newpump(const LLSD&);
    void listen(const LLSD&);
    void stoplistening(const LLSD&);
    void ping(const LLSD&) const;
    void getAPIs(const LLSD&) const;
    void getAPI(const LLSD&) const;
    void getFeatures(const LLSD&) const;
    void getFeature(const LLSD&) const;

    LLBoundListener connect(LLEventPump& pump, const std::string& listener);
    void saveListener(const std::string& pump_name, const std::string& listener_name,
                      const LLBoundListener& listener);

    // The relative order of these next declarations is important because the
    // constructor will initialize in this order.
    std::string mCaller;
    Callback mCallback;
    LLEventStream mReplyPump;
    LLTempBoundListener mReplyConn;

    // In theory, listen() could simply call the relevant LLEventPump's
    // listen() method, stoplistening() likewise. Lifespan issues make us
    // capture the LLBoundListener objects: when this object goes away, all
    // those listeners should be disconnected. But what if the client listens,
    // stops, listens again on the same LLEventPump with the same listener
    // name? Merely collecting LLBoundListeners wouldn't adequately track
    // that. So capture the latest LLBoundListener for this LLEventPump name
    // and listener name.
    typedef std::map<std::pair<std::string, std::string>, LLBoundListener> ListenersMap;
    ListenersMap mListeners;
};

#endif /* ! defined(LL_LLLEAPLISTENER_H) */
