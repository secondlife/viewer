/**
 * @file   llxmlrpclistener.h
 * @author Nat Goodspeed
 * @date   2009-03-18
 * @brief  LLEventPump API for LLXMLRPCTransaction. This header doesn't
 *         actually define the API; the API is defined by the pump name on
 *         which this class listens, and by the expected content of LLSD it
 *         receives.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLXMLRPCLISTENER_H)
#define LL_LLXMLRPCLISTENER_H

#include "llevents.h"

/// Listen on an LLEventPump with specified name for LLXMLRPCTransaction
/// request events.
class LLXMLRPCListener
{
public:
    /// Specify the pump name on which to listen
    LLXMLRPCListener(const std::string& pumpname);

    /// Handle request events on the event pump specified at construction time
    bool process(const LLSD& command);

private:
    LLTempBoundListener mBoundListener;
};

#endif /* ! defined(LL_LLXMLRPCLISTENER_H) */
