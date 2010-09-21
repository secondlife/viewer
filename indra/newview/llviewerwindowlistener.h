/**
 * @file   llviewerwindowlistener.h
 * @author Nat Goodspeed
 * @date   2009-06-30
 * @brief  Event API for subset of LLViewerWindow methods
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLVIEWERWINDOWLISTENER_H)
#define LL_LLVIEWERWINDOWLISTENER_H

#include "lleventapi.h"

class LLViewerWindow;
class LLSD;

/// Listen on an LLEventPump with specified name for LLViewerWindow request events.
class LLViewerWindowListener: public LLEventAPI
{
public:
    /// Bind the LLViewerWindow instance to use (e.g. gViewerWindow).
    LLViewerWindowListener(LLViewerWindow* llviewerwindow);

private:
    void saveSnapshot(const LLSD& event) const;
    void requestReshape(LLSD const & event_data) const;

    LLViewerWindow* mViewerWindow;
};

#endif /* ! defined(LL_LLVIEWERWINDOWLISTENER_H) */
