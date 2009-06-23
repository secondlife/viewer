/**
 * @file   llcapabilitylistener.h
 * @author Nat Goodspeed
 * @date   2009-01-07
 * @brief  Provide an event-based API for capability requests
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLCAPABILITYLISTENER_H)
#define LL_LLCAPABILITYLISTENER_H

#include "llevents.h"               // LLEventPump
#include "llsdmessage.h"            // LLSDMessage::ArgError
#include "llerror.h"                // LOG_CLASS()

class LLCapabilityProvider;
class LLMessageSystem;
class LLSD;

class LLCapabilityListener
{
    LOG_CLASS(LLCapabilityListener);
public:
    LLCapabilityListener(const std::string& name, LLMessageSystem* messageSystem,
                         const LLCapabilityProvider& provider,
                         const LLUUID& agentID, const LLUUID& sessionID);

    /// Capability-request exception
    typedef LLSDMessage::ArgError ArgError;
    /// Get LLEventPump on which we listen for capability requests
    /// (https://wiki.lindenlab.com/wiki/Viewer:Messaging/Messaging_Notes#Capabilities)
    LLEventPump& getCapAPI() { return mEventPump; }

    /**
     * Base class for mapping an as-yet-undeployed capability name to a (pair
     * of) LLMessageSystem message(s). To map a capability name to such
     * messages, derive a subclass of CapabilityMapper and declare a static
     * instance in a translation unit known to be loaded. The mapping is not
     * region-specific. If an LLViewerRegion's capListener() receives a
     * request for a supported capability, it will use the capability's URL.
     * If not, it will look for an applicable CapabilityMapper subclass
     * instance.
     */
    class CapabilityMapper
    {
    public:
        /**
         * Base-class constructor. Typically your subclass constructor will
         * pass these parameters as literals.
         * @param cap the capability name handled by this (subclass) instance
         * @param reply the name of the response LLMessageSystem message. Omit
         * if the LLMessageSystem message you intend to send doesn't prompt a
         * reply message, or if you already handle that message in some other
         * way.
         */
        CapabilityMapper(const std::string& cap, const std::string& reply = "");
        virtual ~CapabilityMapper();
        /// query the capability name
        std::string getCapName() const { return mCapName; }
        /// query the reply message name
        std::string getReplyName() const { return mReplyName; }
        /**
         * Override this method to build the LLMessageSystem message we should
         * send instead of the requested capability message. DO NOT send that
         * message: that will be handled by the caller.
         */
        virtual void buildMessage(LLMessageSystem* messageSystem,
                                  const LLUUID& agentID,
                                  const LLUUID& sessionID,
                                  const std::string& capabilityName,
                                  const LLSD& payload) const = 0;
        /**
         * Override this method if you pass a non-empty @a reply
         * LLMessageSystem message name to the constructor: that is, if you
         * expect to receive an LLMessageSystem message in response to the
         * message you constructed in buildMessage(). If you don't pass a @a
         * reply message name, you need not override this method as it won't
         * be called.
         *
         * Using LLMessageSystem message-reading operations, your
         * readResponse() override should construct and return an LLSD object
         * of the form you expect to receive from the real implementation of
         * the capability you intend to invoke, when it finally goes live.
         */
        virtual LLSD readResponse(LLMessageSystem* messageSystem) const;

    private:
        const std::string mCapName;
        const std::string mReplyName;
    };

private:
    /// Bind the LLCapabilityProvider passed to our ctor
    const LLCapabilityProvider& mProvider;

    /// Post an event to this LLEventPump to invoke a capability message on
    /// the bound LLCapabilityProvider's server
    /// (https://wiki.lindenlab.com/wiki/Viewer:Messaging/Messaging_Notes#Capabilities)
    LLEventStream mEventPump;

    LLMessageSystem* mMessageSystem;
    LLUUID mAgentID, mSessionID;

    /// listener to process capability requests
    bool capListener(const LLSD&);

    /// helper class for capListener()
    class CapabilityMappers;
};

#endif /* ! defined(LL_LLCAPABILITYLISTENER_H) */
