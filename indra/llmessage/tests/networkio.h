/**
 * @file   networkio.h
 * @author Nat Goodspeed
 * @date   2009-01-09
 * @brief  
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
