/**
 * @file   commtest.h
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

#if ! defined(LL_COMMTEST_H)
#define LL_COMMTEST_H

#include "networkio.h"
#include "llevents.h"
#include "llsd.h"
#include "llhost.h"
#include "stringize.h"
#include <string>

/**
 * This struct is shared by a couple of standalone comm tests (ADD_COMM_BUILD_TEST).
 */
struct commtest_data
{
    NetworkIO& netio;
    LLEventPumps& pumps;
    LLEventStream replyPump, errorPump;
    LLSD result;
    bool success;
    LLHost host;
    std::string server;

    commtest_data():
        netio(NetworkIO::instance()),
        pumps(LLEventPumps::instance()),
        replyPump("reply"),
        errorPump("error"),
        success(false),
        host("127.0.0.1", 8000),
        server(STRINGIZE("http://" << host.getString() << "/"))
    {
        replyPump.listen("self", boost::bind(&commtest_data::outcome, this, _1, true));
        errorPump.listen("self", boost::bind(&commtest_data::outcome, this, _1, false));
    }

    bool outcome(const LLSD& _result, bool _success)
    {
//      std::cout << "commtest_data::outcome(" << _result << ", " << _success << ")\n";
        result = _result;
        success = _success;
        // Break the wait loop in NetworkIO::pump(), otherwise devs get
        // irritated at making the big monolithic test executable take longer
        pumps.obtain("done").post(success);
        return false;
    }
};

#endif /* ! defined(LL_COMMTEST_H) */
