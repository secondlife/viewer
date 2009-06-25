/**
 * @file   llappviewerlistener.h
 * @author Nat Goodspeed
 * @date   2009-06-18
 * @brief  Wrap subset of LLAppViewer API in event API
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLAPPVIEWERLISTENER_H)
#define LL_LLAPPVIEWERLISTENER_H

#include "lleventdispatcher.h"

class LLAppViewer;
class LLSD;

/// Listen on an LLEventPump with specified name for LLAppViewer request events.
class LLAppViewerListener: public LLDispatchListener
{
public:
    /// Specify the pump name on which to listen, and bind the LLAppViewer
    /// instance to use (e.g. LLAppViewer::instance()).
    LLAppViewerListener(const std::string& pumpname, LLAppViewer* llappviewer);

private:
    void requestQuit(const LLSD& event) const;

    LLAppViewer* mAppViewer;
};

#endif /* ! defined(LL_LLAPPVIEWERLISTENER_H) */
