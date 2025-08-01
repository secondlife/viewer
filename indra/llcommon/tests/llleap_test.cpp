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
#include <functional>
// external library headers
// other Linden headers
#include "../test/lldoctest.h"
#include "../test/namedtempfile.h"
#include "../test/catch_and_store_what_in.h"
#include "wrapllerrs.h"             // CaptureLog
#include "llevents.h"
#include "llprocess.h"
#include "llstring.h"
#include "stringize.h"
#include "StringVec.h"

#if defined(LL_WINDOWS)
#define sleep(secs) _sleep((secs) * 1000)

// WOLF-300: It appears that driving a megabyte of data through an LLLeap pipe
// causes Windows abdominal pain such that it later fails code-signing in some
// mysterious way. Entirely suppressing these LLLeap tests pushes the failure
// rate MUCH lower. Can we re-enable them with a smaller data size on Windows?
const size_t BUFFERED_LENGTH =  100*1024;

#else // not Windows
const size_t BUFFERED_LENGTH = 1023*1024; // try wrangling just under a megabyte of data

#endif

// capture std::weak_ptrs to LLLeap instances so we can tell when they expire
typedef std::vector<std::weak_ptr<LLLeap>> LLLeapVector;

void waitfor(const LLLeapVector& instances, int timeout=60)
{
    int i;
    for (i = 0; i < timeout; ++i)
    {
        // Every iteration, test whether any of the passed LLLeap instances
        // still exist (are still running).
        bool found = false;
        for (auto& ptr : instances)
        {
            if (! ptr.expired())
            {
                found = true;
                break;
            }
        }
        // If we made it through all of 'instances' without finding one that's
        // still running, we're done.
        if (! found)
        {
/*==========================================================================*|
            std::cout << instances.size() << " LLLeap instances terminated in "
                      << i << " seconds, proceeding" << std::endl;
|*==========================================================================*/
            return;
        }
        // Found an instance that's still running. Wait and pump LLProcess.
        sleep(1);
        LLEventPumps::instance().obtain("mainloop").post(LLSD());
    }
    tut::ensure(STRINGIZE("at least 1 of " << instances.size()
                          << " LLLeap instances timed out ("
                          << timeout << " seconds) without terminating"),
                i < timeout);
}

void waitfor(LLLeap* instance, int timeout=60)
{
    LLLeapVector instances;
    instances.push_back(instance->getWeak());
    waitfor(instances, timeout);
}

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

struct llleap_data
{

        llleap_data():
            reader(".py",
                   // This logic is adapted from vita.viewerclient.receiveEvent()
                   [](std::ostream& out){ out <<
                   "import re\n"
                   "import os\n"
                   "import sys\n"
                   "\n"
                   "import llsd\n"
                   "\n"
                   "class ProtocolError(Exception):\n"
                   "    def __init__(self, msg, data):\n"
                   "        Exception.__init__(self, msg)\n"
                   "        self.data = data\n"
                   "\n"
                   "class ParseError(ProtocolError):\n"
                   "    pass\n"
                   "\n"
                   "def get():\n"
                   "    hdr = []\n"
                   "    while b':' not in hdr and len(hdr) < 20:\n"
                   "        hdr.append(sys.stdin.buffer.read(1))\n"
                   "        if not hdr:\n"
                   "            sys.exit(0)\n"
                   "    if not hdr[-1] == b':':\n"
                   "        raise ProtocolError('Expected len:data, got %r' % hdr, hdr)\n"
                   "    try:\n"
                   "        length = int(b''.join(hdr[:-1]))\n"
                   "    except ValueError:\n"
                   "        raise ProtocolError('Non-numeric len %r' % hdr[:-1], hdr[:-1])\n"
                   "    parts = []\n"
                   "    received = 0\n"
                   "    while received < length:\n"
                   "        parts.append(sys.stdin.buffer.read(length - received))\n"
                   "        received += len(parts[-1])\n"
                   "    data = b''.join(parts)\n"
                   "    assert len(data) == length\n"
                   "    try:\n"
                   "        return llsd.parse(data)\n"
                   //   Seems the old indra.base.llsd module didn't properly
                   //   convert IndexError (from running off end of string) to
                   //   LLSDParseError.
                   "    except (IndexError, llsd.LLSDParseError) as e:\n"
                   "        msg = 'Bad received packet (%s)' % e\n"
                   "        print('%s, %s bytes:' % (msg, len(data)), file=sys.stderr)\n"
                   "        showmax = 40\n"
                   //       We've observed failures with very large packets;
                   //       dumping the entire packet wastes time and space.
                   //       But if the error states a particular byte offset,
                   //       truncate to (near) that offset when dumping data.
                   "        location = re.search(r' at (byte|index) ([0-9]+)', str(e))\n"
                   "        if not location:\n"
                   "            # didn't find offset, dump whole thing, no ellipsis\n"
                   "            ellipsis = ''\n"
                   "        else:\n"
                   "            # found offset within error message\n"
                   "            trunc = int(location.group(2)) + showmax\n"
                   "            data = data[:trunc]\n"
                   "            ellipsis = '... (%s more)' % (length - trunc)\n"
                   "        offset = -showmax\n"
                   "        for offset in range(0, len(data)-showmax, showmax):\n"
                   "            print('%04d: %r +' % \\\n"
                   "                  (offset, data[offset:offset+showmax]), file=sys.stderr)\n"
                   "        offset += showmax\n"
                   "        print('%04d: %r%s' % \\\n"
                   "              (offset, data[offset:], ellipsis), file=sys.stderr)\n"
                   "        raise ParseError(msg, data)\n"
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
                   "    sys.stdout.buffer.write(b'%d:%b' % (len(req), req))\n"
                   "    sys.stdout.flush()\n"
                   "\n"
                   "def send(pump, data):\n"
                   "    put(llsd.format_notation(dict(pump=pump, data=data)))\n"
                   "\n"
                   "def request(pump, data):\n"
                   "    # we expect 'data' is a dict\n"
                   "    data['reply'] = _reply\n"
                   "    send(pump, data)\n";
};

TEST_CASE_FIXTURE(llleap_data, "test_1")
{

        set_test_name("multiple LLLeap instances");
        NamedExtTempFile script("py",
                                "import time\n"
                                "time.sleep(1)\n");
        LLLeapVector instances;
        instances.push_back(LLLeap::create(get_test_name(),
                                           StringVec{PYTHON, script.getName()
}

TEST_CASE_FIXTURE(llleap_data, "test_2")
{

        set_test_name("stderr to log");
        NamedExtTempFile script("py",
                                "import sys\n"
                                "sys.stderr.write('''Hello from Python!\n"
                                "note partial line''')\n");
        StringVec vcommand{ PYTHON, script.getName() 
}

TEST_CASE_FIXTURE(llleap_data, "test_3")
{

        set_test_name("bad stdout protocol");
        NamedExtTempFile script("py",
                                "print('Hello from Python!')\n");
        CaptureLog log(LLError::LEVEL_WARN);
        waitfor(LLLeap::create(get_test_name(),
                               StringVec{PYTHON, script.getName()
}

TEST_CASE_FIXTURE(llleap_data, "test_4")
{

        set_test_name("leftover stdout");
        NamedExtTempFile script("py",
                                "import sys\n"
                                // note lack of newline
                                "sys.stdout.write('Hello from Python!')\n");
        CaptureLog log(LLError::LEVEL_WARN);
        waitfor(LLLeap::create(get_test_name(),
                               StringVec{PYTHON, script.getName()
}

TEST_CASE_FIXTURE(llleap_data, "test_5")
{

        set_test_name("bad stdout len prefix");
        NamedExtTempFile script("py",
                                "import sys\n"
                                "sys.stdout.write('5a2:something')\n");
        CaptureLog log(LLError::LEVEL_WARN);
        waitfor(LLLeap::create(get_test_name(),
                               StringVec{PYTHON, script.getName()
}

TEST_CASE_FIXTURE(llleap_data, "test_6")
{

        set_test_name("empty plugin vector");
        std::string threw = catch_what<LLLeap::Error>([](){
                LLLeap::create("empty", StringVec());
            
}

TEST_CASE_FIXTURE(llleap_data, "test_7")
{

        set_test_name("bad launch");
        // Synthesize bogus executable name
        std::string BADPYTHON(PYTHON.substr(0, PYTHON.length()-1) + "x");
        CaptureLog log;
        std::string threw = catch_what<LLLeap::Error>([&BADPYTHON](){
                LLLeap::create("bad exe", BADPYTHON);
            
}

TEST_CASE_FIXTURE(llleap_data, "test_8")
{

        set_test_name("round trip");
        AckAPI api;
        Result result;
        NamedExtTempFile script("py",
                                [&](std::ostream& out){ out <<
                                "from " << reader_module << " import *\n"
                                // make a request on our little API
                                "request(pump='" << api.getName() << "', data={
}

TEST_CASE_FIXTURE(llleap_data, "test_9")
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
        NamedExtTempFile script("py",
                                [&](std::ostream& out){ out <<
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
                                "print('Sending %s requests' % count, file=sys.stderr)\n"
                                "for i in range(count):\n"
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
                                "for i in range(count):\n"
                                "    resp = get()\n"
                                "    if resp['data']['reqid'] != i:\n"
                                "        result = 'expected reqid=%s in %s' % (i, resp)\n"
                                "        break\n"
                                "send(pump='" << result.getName() << "', data=result)\n";
}

TEST_CASE_FIXTURE(llleap_data, "test_10")
{

        set_test_name("very large message");
        test_or_split(PYTHON, reader_module, get_test_name(), BUFFERED_LENGTH);
    
}

} // TEST_SUITE
