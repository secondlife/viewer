/**
 * @file   commtest.h
 * @author Nat Goodspeed
 * @date   2009-01-09
 * @brief  
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
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
