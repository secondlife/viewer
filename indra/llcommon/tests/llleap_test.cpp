/**
 * @file   llleap_test.cpp
 * @author Nat Goodspeed
 * @date   2012-02-21
 * @brief  Test for llleap.
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llleap.h"
// STL headers
// std headers
// external library headers
#include <boost/assign/list_of.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/foreach.hpp>
// other Linden headers
#include "../test/lltut.h"
#include "../test/namedtempfile.h"
#include "../test/manageapr.h"
#include "../test/catch_and_store_what_in.h"
#include "wrapllerrs.h"
#include "llevents.h"
#include "llprocess.h"
#include "stringize.h"
#include "StringVec.h"

using boost::assign::list_of;

static ManageAPR manager;

StringVec sv(const StringVec& listof) { return listof; }

#if defined(LL_WINDOWS)
#define sleep(secs) _sleep((secs) * 1000)
#endif

void waitfor(const std::vector<LLLeap*>& instances)
{
    int i, timeout = 60;
    for (i = 0; i < timeout; ++i)
    {
        // Every iteration, test whether any of the passed LLLeap instances
        // still exist (are still running).
        std::vector<LLLeap*>::const_iterator vli(instances.begin()), vlend(instances.end());
        for ( ; vli != vlend; ++vli)
        {
            // getInstance() returns NULL if it's terminated/gone, non-NULL if
            // it's still running
            if (LLLeap::getInstance(*vli))
                break;
        }
        // If we made it through all of 'instances' without finding one that's
        // still running, we're done.
        if (vli == vlend)
            return;
        // Found an instance that's still running. Wait and pump LLProcess.
        sleep(1);
        LLEventPumps::instance().obtain("mainloop").post(LLSD());
    }
    tut::ensure("timed out without terminating", i < timeout);
}

void waitfor(LLLeap* instance)
{
    std::vector<LLLeap*> instances;
    instances.push_back(instance);
    waitfor(instances);
}

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llleap_data
    {
        llleap_data():
            reader(".py",
                   // This logic is adapted from vita.viewerclient.receiveEvent()
                   "import re\n"
                   "import sys\n"
                   "LEFTOVER = ''\n"
                   "class ProtocolError(Exception):\n"
                   "    pass\n"
                   "def get():\n"
                   "    global LEFTOVER\n"
                   "    hdr = LEFTOVER\n"
                   "    if ':' not in hdr:\n"
                   "        hdr += sys.stdin.read(20)\n"
                   "        if not hdr:\n"
                   "            sys.exit(0)\n"
                   "    parts = hdr.split(':', 1)\n"
                   "    if len(parts) != 2:\n"
                   "        raise ProtocolError('Expected len:data, got %r' % hdr)\n"
                   "    try:\n"
                   "        length = int(parts[0])\n"
                   "    except ValueError:\n"
                   "        raise ProtocolError('Non-numeric len %r' % parts[0])\n"
                   "    del parts[0]\n"
                   "    received = len(parts[0])\n"
                   "    while received < length:\n"
                   "        parts.append(sys.stdin.read(length - received))\n"
                   "        received += len(parts[-1])\n"
                   "    if received > length:\n"
                   "        excess = length - received\n"
                   "        LEFTOVER = parts[-1][excess:]\n"
                   "        parts[-1] = parts[-1][:excess]\n"
                   "    data = ''.join(parts)\n"
                   "    assert len(data) == length\n"
                   "    return data\n"
                   "\n"
                   "def put(req):\n"
                   "    sys.stdout.write(':'.join((str(len(req)), req)))\n"
                   "    sys.stdout.flush()\n"
                   "\n"
                   "# deal with initial stdin message\n"
                   // this will throw if the initial write to stdin
                   // doesn't follow len:data protocol
                   "_initial = get()\n"
                   "_match = re.search(r\"'pump':'(.*?)'\", _initial)\n"
                   // this will throw if we couldn't find
                   // 'pump':'etc.' in the initial write
                   "_reply = _match.group(1)\n"
                   "\n"
                   "def replypump():\n"
                   "    return _reply\n"
                   "\n"
                   "def escape(str):\n"
                   "    return ''.join(('\\\\'+c if c in r\"\\'\" else c) for c in str)\n"
                   "\n"
                   "def quote(str):\n"
                   "    return \"'%s'\" % escape(str)\n"),
            // Get the actual pathname of the NamedExtTempFile and trim off
            // the ".py" extension. (We could cache reader.getName() in a
            // separate member variable, but I happen to know getName() just
            // returns a NamedExtTempFile member rather than performing any
            // computation, so I don't mind calling it twice.) Then take the
            // basename.
            reader_module(LLProcess::basename(
                              reader.getName().substr(0, reader.getName().length()-3))),
            pPYTHON(getenv("PYTHON")),
            PYTHON(pPYTHON? pPYTHON : "")
        {
            ensure("Set PYTHON to interpreter pathname", pPYTHON);
        }
        NamedExtTempFile reader;
        const std::string reader_module;
        const char* pPYTHON;
        const std::string PYTHON;
    };
    typedef test_group<llleap_data> llleap_group;
    typedef llleap_group::object object;
    llleap_group llleapgrp("llleap");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("multiple LLLeap instances");
        NamedTempFile script("py",
                             "import time\n"
                             "time.sleep(1)\n");
        std::vector<LLLeap*> instances;
        instances.push_back(LLLeap::create(get_test_name(),
                                           sv(list_of(PYTHON)(script.getName()))));
        instances.push_back(LLLeap::create(get_test_name(),
                                           sv(list_of(PYTHON)(script.getName()))));
        // In this case we're simply establishing that two LLLeap instances
        // can coexist without throwing exceptions or bombing in any other
        // way. Wait for them to terminate.
        waitfor(instances);
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("stderr to log");
        NamedTempFile script("py",
                             "import sys\n"
                             "sys.stderr.write('''Hello from Python!\n"
                             "note partial line''')\n");
        CaptureLog log(LLError::LEVEL_INFO);
        waitfor(LLLeap::create(get_test_name(),
                               sv(list_of(PYTHON)(script.getName()))));
        log.messageWith("Hello from Python!");
        log.messageWith("note partial line");
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("empty plugin vector");
        std::string threw;
        try
        {
            LLLeap::create("empty", StringVec());
        }
        CATCH_AND_STORE_WHAT_IN(threw, LLLeap::Error)
        ensure_contains("LLLeap::Error", threw, "no plugin");
        // try the suppress-exception variant
        ensure("bad launch returned non-NULL", ! LLLeap::create("empty", StringVec(), false));
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("bad launch");
        // Synthesize bogus executable name
        std::string BADPYTHON(PYTHON.substr(0, PYTHON.length()-1) + "x");
        CaptureLog log;
        std::string threw;
        try
        {
            LLLeap::create("bad exe", BADPYTHON);
        }
        CATCH_AND_STORE_WHAT_IN(threw, LLLeap::Error)
        ensure_contains("LLLeap::create() didn't throw", threw, "failed");
        log.messageWith("failed");
        log.messageWith(BADPYTHON);
        // try the suppress-exception variant
        ensure("bad launch returned non-NULL", ! LLLeap::create("bad exe", BADPYTHON, false));
    }

    // Generic self-contained listener: derive from this and override its
    // call() method, then tell somebody to post on the pump named getName().
    // Control will reach your call() override.
    struct ListenerBase
    {
        // Pass the pump name you want; will tweak for uniqueness.
        ListenerBase(const std::string& name):
            mPump(name, true)
        {
            mPump.listen(name, boost::bind(&ListenerBase::call, this, _1));
        }

        virtual bool call(const LLSD& request)
        {
            return false;
        }

        LLEventPump& getPump() { return mPump; }
        const LLEventPump& getPump() const { return mPump; }

        std::string getName() const { return mPump.getName(); }
        void post(const LLSD& data) { mPump.post(data); }

        LLEventStream mPump;
    };

    // Mimic a dummy little LLEventAPI that merely sends a reply back to its
    // requester on the "reply" pump.
    struct API: public ListenerBase
    {
        API(): ListenerBase("API") {}

        virtual bool call(const LLSD& request)
        {
            LLEventPumps::instance().obtain(request["reply"]).post("ack");
            return false;
        }
    };

    // Give LLLeap script a way to post success/failure.
    struct Result: public ListenerBase
    {
        Result(): ListenerBase("Result") {}

        virtual bool call(const LLSD& request)
        {
            mData = request;
            return false;
        }

        void ensure() const
        {
            tut::ensure(std::string("never posted to ") + getName(), mData.isDefined());
            // Post an empty string for success, non-empty string is failure message.
            tut::ensure(mData, mData.asString().empty());
        }

        LLSD mData;
    };

    template<> template<>
    void object::test<5>()
    {
        set_test_name("round trip");
        API api;
        Result result;
        NamedTempFile script("py",
                             boost::lambda::_1 <<
                             "import sys\n"
                             "from " << reader_module << " import *\n"
                             // make a request on our little API
                             "put(\"{'pump':'" << api.getName() << "','data':{'reply':'%s'}}\" %\n"
                             "    replypump())\n"
                             // wait for its response
                             "resp = get()\n"
                             // We expect "{'data':'ack','pump':'%s'}", but
                             // don't depend on the order of the keys.
                             "result = 'bad: ' + resp\n"
                             "if resp.startswith('{') and resp.endswith('}'):\n"
                             "    expect = set((\"'data':'ack'\", \"'pump':'%s'\" % replypump()))\n"
                             "    actual = set(resp[1:-1].split(','))\n"
                             "    if actual == expect:\n"
                             "        result = ''\n"
                             "put(\"{'pump':'" << result.getName() << "','data':%s}\" %\n"
                             "    quote(result))\n");
        waitfor(LLLeap::create(get_test_name(), sv(list_of(PYTHON)(script.getName()))));
        result.ensure();
    }
} // namespace tut
