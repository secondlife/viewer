/**
 * @file   networkio.h
 * @author Nat Goodspeed
 * @date   2009-01-09
 * @brief  
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

#if ! defined(LL_NETWORKIO_H)
#define LL_NETWORKIO_H

#include "llmemory.h"               // LLSingleton
#include "llapr.h"
#include "llares.h"
#include "llpumpio.h"
#include "llhttpclient.h"

/*****************************************************************************
*   NetworkIO
*****************************************************************************/
// Doing this initialization in a class constructor makes sense. But we don't
// want to redo it for each different test. Nor do we want to do it at static-
// init time. Use the lazy, on-demand initialization we get from LLSingleton.
class NetworkIO: public LLSingleton<NetworkIO>
{
public:
    NetworkIO():
        mServicePump(NULL),
        mDone(false)
    {
        ll_init_apr();
        if (! gAPRPoolp)
        {
            throw std::runtime_error("Can't initialize APR");
        }

        // Create IO Pump to use for HTTP Requests.
        mServicePump = new LLPumpIO(gAPRPoolp);
        LLHTTPClient::setPump(*mServicePump);
        if (ll_init_ares() == NULL || !gAres->isInitialized())
        {
            throw std::runtime_error("Can't start DNS resolver");
        }

        // You can interrupt pump() without waiting the full timeout duration
        // by posting an event to the LLEventPump named "done".
        LLEventPumps::instance().obtain("done").listen("self",
                                                       boost::bind(&NetworkIO::done, this, _1));
    }

    bool pump(F32 timeout=10)
    {
        // Reset the done flag so we don't pop out prematurely
        mDone = false;
        // Evidently the IO structures underlying LLHTTPClient need to be
        // "pumped". Do some stuff normally performed in the viewer's main
        // loop.
        LLTimer timer;
        while (timer.getElapsedTimeF32() < timeout)
        {
            if (mDone)
            {
//              std::cout << "NetworkIO::pump(" << timeout << "): breaking loop after "
//                        << timer.getElapsedTimeF32() << " seconds\n";
                return true;
            }
            pumpOnce();
        }
        return false;
    }

    void pumpOnce()
    {
        gAres->process();
        mServicePump->pump();
        mServicePump->callback();
    }

    bool done(const LLSD&)
    {
        mDone = true;
        return false;
    }

private:
    LLPumpIO* mServicePump;
    bool mDone;
};

#endif /* ! defined(LL_NETWORKIO_H) */
