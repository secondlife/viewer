/**
 * @file   commtest.h
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

#if ! defined(LL_COMMTEST_H)
#define LL_COMMTEST_H

#include "networkio.h"
#include "llevents.h"
#include "llsd.h"
#include "llhost.h"
#include "stringize.h"
#include <map>
#include <string>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

struct CommtestError: public std::runtime_error
{
    CommtestError(const std::string& what): std::runtime_error(what) {}
};

static bool query_verbose()
{
    const char* cbose = getenv("INTEGRATION_TEST_VERBOSE");
    if (! cbose)
    {
        cbose = "1";
    }
    std::string strbose(cbose);
    return (! (strbose == "0" || strbose == "off" ||
               strbose == "false" || strbose == "quiet"));
}

bool verbose()
{
    // This should only be initialized once.
    static bool vflag = query_verbose();
    return vflag;
}

static int query_port(const std::string& var)
{
    const char* cport = getenv(var.c_str());
    if (! cport)
    {
        throw CommtestError(STRINGIZE("missing environment variable" << var));
    }
    // This will throw, too, if the value of PORT isn't numeric.
    int port(boost::lexical_cast<int>(cport));
    if (verbose())
    {
        std::cout << "getport('" << var << "') = " << port << std::endl;
    }
    return port;
}

static int getport(const std::string& var)
{
    typedef std::map<std::string, int> portsmap;
    static portsmap ports;
    // We can do this with a single map lookup with map::insert(). Either it
    // returns an existing entry and 'false' (not newly inserted), or it
    // inserts the specified value and 'true'.
    std::pair<portsmap::iterator, bool> inserted(ports.insert(portsmap::value_type(var, 0)));
    if (inserted.second)
    {
        // We haven't yet seen this var. Remember its value.
        inserted.first->second = query_port(var);
    }
    // Return the (existing or new) iterator's value.
    return inserted.first->second;
}

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
        host("127.0.0.1", getport("PORT")),
        server(STRINGIZE("http://" << host.getString() << "/"))
    {
        replyPump.listen("self", boost::bind(&commtest_data::outcome, this, _1, true));
        errorPump.listen("self", boost::bind(&commtest_data::outcome, this, _1, false));
    }

    static int getport(const std::string& var)
    {
        // We have a couple consumers of commtest_data::getport(). But we've
        // since moved it out to the global namespace. So this is just a
        // facade.
        return ::getport(var);
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
