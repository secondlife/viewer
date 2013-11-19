/**
 * @file   llviewerwindowlistener.h
 * @author Nat Goodspeed
 * @date   2009-06-30
 * @brief  Event API for subset of LLViewerWindow methods
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
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
