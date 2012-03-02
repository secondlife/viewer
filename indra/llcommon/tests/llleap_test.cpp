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

const size_t BUFFERED_LENGTH = 1024*1024; // try wrangling a megabyte of data

void waitfor(const std::vector<LLLeap*>& instances, int timeout=60)
{
    int i;
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

void waitfor(LLLeap* instance, int timeout=60)
{
    std::vector<LLLeap*> instances;
    instances.push_back(instance);
    waitfor(instances, timeout);
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
                   boost::lambda::_1 <<
                   "import os\n"
                   "import sys\n"
                   // Don't forget that this Python script is written to some
                   // temp directory somewhere! Its __file__ is useless in
                   // finding indra/lib/python. Use our __FILE__, with
                   // raw-string syntax to deal with Windows pathnames.
                   "mydir = os.path.dirname(r'" << __FILE__ << "')\n"
                   "try:\n"
                   "    from llbase import llsd\n"
                   "except ImportError:\n"
                   // We expect mydir to be .../indra/llcommon/tests.
                   "    sys.path.insert(0,\n"
                   "        os.path.join(mydir, os.pardir, os.pardir, 'lib', 'python'))\n"
                   "    from indra.base import llsd\n"
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
                   "    return llsd.parse(data)\n"
                   "\n"
                   "# deal with initial stdin message\n"
                   // this will throw if the initial write to stdin doesn't
                   // follow len:data protocol, or if we couldn't find 'pump'
                   // in the dict
                   "_reply = get()['pump']\n"
                   "\n"
                   "def replypump():\n"
                   "    return _reply\n"
                   "\n"
                   "def put(req):\n"
                   "    sys.stdout.write(':'.join((str(len(req)), req)))\n"
                   "    sys.stdout.flush()\n"
                   "\n"
                   "def send(pump, data):\n"
                   "    put(llsd.format_notation(dict(pump=pump, data=data)))\n"
                   "\n"
                   "def request(pump, data):\n"
                   "    # we expect 'data' is a dict\n"
                   "    data['reply'] = _reply\n"
                   "    send(pump, data)\n"),
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
        set_test_name("bad stdout protocol");
        NamedTempFile script("py",
                             "print 'Hello from Python!'\n");
        CaptureLog log(LLError::LEVEL_WARN);
        waitfor(LLLeap::create(get_test_name(),
                               sv(list_of(PYTHON)(script.getName()))));
        ensure_contains("error log line",
                        log.messageWith("invalid protocol"), "Hello from Python!");
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("leftover stdout");
        NamedTempFile script("py",
                             "import sys\n"
                             // note lack of newline
                             "sys.stdout.write('Hello from Python!')\n");
        CaptureLog log(LLError::LEVEL_WARN);
        waitfor(LLLeap::create(get_test_name(),
                               sv(list_of(PYTHON)(script.getName()))));
        ensure_contains("error log line",
                        log.messageWith("Discarding"), "Hello from Python!");
    }

    template<> template<>
    void object::test<5>()
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
    void object::test<6>()
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
    struct AckAPI: public ListenerBase
    {
        AckAPI(): ListenerBase("AckAPI") {}

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
    void object::test<7>()
    {
        set_test_name("round trip");
        AckAPI api;
        Result result;
        NamedTempFile script("py",
                             boost::lambda::_1 <<
                             "from " << reader_module << " import *\n"
                             // make a request on our little API
                             "request(pump='" << api.getName() << "', data={})\n"
                             // wait for its response
                             "resp = get()\n"
                             "result = '' if resp == dict(pump=replypump(), data='ack')\\\n"
                             "            else 'bad: ' + str(resp)\n"
                             "send(pump='" << result.getName() << "', data=result)\n");
        waitfor(LLLeap::create(get_test_name(), sv(list_of(PYTHON)(script.getName()))));
        result.ensure();
    }

    struct ReqIDAPI: public ListenerBase
    {
        ReqIDAPI(): ListenerBase("ReqIDAPI") {}

        virtual bool call(const LLSD& request)
        {
            // free function from llevents.h
            sendReply(LLSD(), request);
            return false;
        }
    };

    template<> template<>
    void object::test<8>()
    {
        set_test_name("many small messages");
        // It's not clear to me whether there's value in iterating many times
        // over a send/receive loop -- I don't think that will exercise any
        // interesting corner cases. This test first sends a large number of
        // messages, then receives all the responses. The intent is to ensure
        // that some of that data stream crosses buffer boundaries, loop
        // iterations etc. in OS pipes and the LLLeap/LLProcess implementation.
        ReqIDAPI api;
        Result result;
        NamedTempFile script("py",
                             boost::lambda::_1 <<
                             "import sys\n"
                             "from " << reader_module << " import *\n"
                             // Note that since reader imports llsd, this
                             // 'import *' gets us llsd too.
                             "sample = llsd.format_notation(dict(pump='" <<
                             api.getName() << "', data=dict(reqid=999999, reply=replypump())))\n"
                             // The whole packet has length prefix too: "len:data"
                             "samplen = len(str(len(sample))) + 1 + len(sample)\n"
                             // guess how many messages it will take to
                             // accumulate BUFFERED_LENGTH
                             "count = int(" << BUFFERED_LENGTH << "/samplen)\n"
                             "print >>sys.stderr, 'Sending %s requests' % count\n"
                             "for i in xrange(count):\n"
                             "    request('" << api.getName() << "', dict(reqid=i))\n"
                             // The assumption in this specific test that
                             // replies will arrive in the same order as
                             // requests is ONLY valid because the API we're
                             // invoking sends replies instantly. If the API
                             // had to wait for some external event before
                             // sending its reply, replies could arrive in
                             // arbitrary order, and we'd have to tick them
                             // off from a set.
                             "result = ''\n"
                             "for i in xrange(count):\n"
                             "    resp = get()\n"
                             "    if resp['data']['reqid'] != i:\n"
                             "        result = 'expected reqid=%s in %s' % (i, resp)\n"
                             "        break\n"
                             "send(pump='" << result.getName() << "', data=result)\n");
        waitfor(LLLeap::create(get_test_name(), sv(list_of(PYTHON)(script.getName()))),
                300);               // needs more realtime than most tests
        result.ensure();
    }

    template<> template<>
    void object::test<9>()
    {
        set_test_name("very large message");
        ReqIDAPI api;
        Result result;
        NamedTempFile script("py",
                             boost::lambda::_1 <<
                             "import sys\n"
                             "from " << reader_module << " import *\n"
                             // Generate a very large string value.
                             "desired = int(sys.argv[1])\n"
                             // 7 chars per item: 6 digits, 1 comma
                             "count = int((desired - 50)/7)\n"
                             "large = ''.join('%06d,' % i for i in xrange(count))\n"
                             // Pass 'large' as reqid because we know the API
                             // will echo reqid, and we want to receive it back.
                             "request('" << api.getName() << "', dict(reqid=large))\n"
                             "resp = get()\n"
                             "echoed = resp['data']['reqid']\n"
                             "if echoed == large:\n"
                             "    send('" << result.getName() << "', '')\n"
                             "    sys.exit(0)\n"
                             // Here we know echoed did NOT match; try to find where
                             "for i in xrange(count):\n"
                             "    start = 7*i\n"
                             "    end   = 7*(i+1)\n"
                             "    if end > len(echoed)\\\n"
                             "    or echoed[start:end] != large[start:end]:\n"
                             "        send('" << result.getName() << "',\n"
                             "             'at offset %s, expected %r but got %r' %\n"
                             "             (start, large[start:end], echoed[start:end]))\n"
                             "sys.exit(1)\n");
        waitfor(LLLeap::create(get_test_name(),
                               sv(list_of
                                  (PYTHON)
                                  (script.getName())
                                  (stringize(BUFFERED_LENGTH)))));
        result.ensure();
    }

    // TODO:

} // namespace tut
