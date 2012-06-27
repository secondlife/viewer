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
#include <map>
#include <string>
#include <boost/function.hpp>
#include <boost/ptr_container/ptr_map.hpp>

/// Listener class implementing LLLeap query/control operations.
/// See https://jira.lindenlab.com/jira/browse/DEV-31978.
class LLLeapListener: public LLEventAPI
{
public:
    /**
     * Decouple LLLeap by dependency injection. Certain LLLeapListener
     * operations must be able to cause LLLeap to listen on a specified
     * LLEventPump with the LLLeap listener that wraps incoming events in an
     * outer (pump=, data=) map and forwards them to the plugin. Very well,
     * define the signature for a function that will perform that, and make
     * our constructor accept such a function.
     */
    typedef boost::function<LLBoundListener(LLEventPump&, const std::string& listener)>
            ConnectFunc;
    LLLeapListener(const ConnectFunc& connect);
    ~LLLeapListener();

    static LLSD getFeatures();

private:
    void newpump(const LLSD&);
    void killpump(const LLSD&);
    void listen(const LLSD&);
    void stoplistening(const LLSD&);
    void ping(const LLSD&) const;
    void getAPIs(const LLSD&) const;
    void getAPI(const LLSD&) const;
    void getFeatures(const LLSD&) const;
    void getFeature(const LLSD&) const;

    void saveListener(const std::string& pump_name, const std::string& listener_name,
                      const LLBoundListener& listener);

    ConnectFunc mConnect;

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
    // Similar lifespan reasoning applies to LLEventPumps instantiated by
    // newpump() operations.
    typedef boost::ptr_map<std::string, LLEventPump> EventPumpsMap;
    EventPumpsMap mEventPumps;
};

#endif /* ! defined(LL_LLLEAPLISTENER_H) */
