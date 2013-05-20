/**
 * @file   llappviewerlistener.h
 * @author Nat Goodspeed
 * @date   2009-06-18
 * @brief  Wrap subset of LLAppViewer API in event API
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

#if ! defined(LL_LLAPPVIEWERLISTENER_H)
#define LL_LLAPPVIEWERLISTENER_H

#include "lleventapi.h"
#include <boost/function.hpp>

class LLAppViewer;
class LLSD;

/// Listen on an LLEventPump with specified name for LLAppViewer request events.
class LLAppViewerListener: public LLEventAPI
{
public:
    typedef boost::function<LLAppViewer*(void)> LLAppViewerGetter;
    /// Bind the LLAppViewer instance to use (e.g. LLAppViewer::instance()).
    LLAppViewerListener(const LLAppViewerGetter& getter);

private:
    void requestQuit(const LLSD& event);
    void forceQuit(const LLSD& event);

    LLAppViewerGetter mAppViewerGetter;
};

#endif /* ! defined(LL_LLAPPVIEWERLISTENER_H) */
