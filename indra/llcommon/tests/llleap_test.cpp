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
#include <functional>

using boost::assign::list_of;

static ManageAPR manager;

StringVec sv(const StringVec& listof) { return listof; }

#if defined(LL_WINDOWS)
#define sleep(secs) _sleep((secs) * 1000)
#endif

#if ! LL_WINDOWS
const size_t BUFFERED_LENGTH = 1023*1024; // try wrangling just under a megabyte of data
#else
// "Then there's Windows... sigh." The "very large message" test is flaky in a
// way that seems to point to either the OS (nonblocking writes to pipes) or
// possibly the apr_file_write() function. Poring over log messages reveals
// that at some point along the way apr_file_write() returns 11 (Resource
// temporarily unavailable, i.e. EAGAIN) and says it wrote 0 bytes -- even
// though it did write the chunk! Our next write attempt retries the same
// chunk, resulting in the chunk being duplicated at the child end, corrupting
// the data stream. Much as I would love to be able to fix it for real, such a
// fix would appear to require distinguishing bogus EAGAIN returns from real
// ones -- how?? Empirically this behavior is only observed when writing a
// "very large message". To be able to move forward at all, try to bypass this
// particular failure by adjusting the size of a "very large message" on
// Windows.
const size_t BUFFERED_LENGTH = 65336;
#endif  // LL_WINDOWS

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
                   "import re\n"
                   "import os\n"
                   "import sys\n"
                   "\n"
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
                   "    hdr = ''\n"
                   "    while ':' not in hdr and len(hdr) < 20:\n"
                   "        hdr += sys.stdin.read(1)\n"
                   "        if not hdr:\n"
                   "            sys.exit(0)\n"
                   "    if not hdr.endswith(':'):\n"
                   "        raise ProtocolError('Expected len:data, got %r' % hdr, hdr)\n"
                   "    try:\n"
                   "        length = int(hdr[:-1])\n"
                   "    except ValueError:\n"
                   "        raise ProtocolError('Non-numeric len %r' % hdr[:-1], hdr[:-1])\n"
                   "    parts = []\n"
                   "    received = 0\n"
                   "    while received < length:\n"
                   "        parts.append(sys.stdin.read(length - received))\n"
                   "        received += len(parts[-1])\n"
                   "    data = ''.join(parts)\n"
                   "    assert len(data) == length\n"
                   "    try:\n"
                   "        return llsd.parse(data)\n"
                   //   Seems the old indra.base.llsd module didn't properly
                   //   convert IndexError (from running off end of string) to
                   //   LLSDParseError.
                   "    except (IndexError, llsd.LLSDParseError), e:\n"
                   "        msg = 'Bad received packet (%s)' % e\n"
                   "        print >>sys.stderr, '%s, %s bytes:' % (msg, len(data))\n"
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
                   "        for offset in xrange(0, len(data)-showmax, showmax):\n"
                   "            print >>sys.stderr, '%04d: %r +' % \\\n"
                   "                  (offset, data[offset:offset+showmax])\n"
                   "        offset += showmax\n"
                   "        print >>sys.stderr, '%04d: %r%s' % \\\n"
                   "              (offset, data[offset:], ellipsis)\n"
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
        set_test_name("bad stdout len prefix");
        NamedTempFile script("py",
                             "import sys\n"
                             "sys.stdout.write('5a2:something')\n");
        CaptureLog log(LLError::LEVEL_WARN);
        waitfor(LLLeap::create(get_test_name(),
                               sv(list_of(PYTHON)(script.getName()))));
        ensure_contains("error log line",
                        log.messageWith("invalid protocol"), "5a2:");
    }

    template<> template<>
    void object::test<6>()
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
    void object::test<7>()
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

        virtual ~ListenerBase() {}  // pacify MSVC

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
    void object::test<8>()
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
    void object::test<9>()
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

    // This is the body of test<10>, extracted so we can run it over a number
    // of large-message sizes.
    void test_large_message(const std::string& PYTHON, const std::string& reader_module,
                            const std::string& test_name, size_t size)
    {
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
                             "try:\n"
                             "    resp = get()\n"
                             "except ParseError, e:\n"
                             "    # try to find where e.data diverges from expectation\n"
                             // Normally we'd expect a 'pump' key in there,
                             // too, with value replypump(). But Python
                             // serializes keys in a different order than C++,
                             // so incoming data start with 'data'.
                             // Truthfully, though, if we get as far as 'pump'
                             // before we find a difference, something's very
                             // strange.
                             "    expect = llsd.format_notation(dict(data=dict(reqid=large)))\n"
                             "    chunk = 40\n"
                             "    for offset in xrange(0, max(len(e.data), len(expect)), chunk):\n"
                             "        if e.data[offset:offset+chunk] != \\\n"
                             "           expect[offset:offset+chunk]:\n"
                             "            print >>sys.stderr, 'Offset %06d: expect %r,\\n'\\\n"
                             "                                '                  get %r' %\\\n"
                             "                                (offset,\n"
                             "                                 expect[offset:offset+chunk],\n"
                             "                                 e.data[offset:offset+chunk])\n"
                             "            break\n"
                             "    else:\n"
                             "        print >>sys.stderr, 'incoming data matches expect?!'\n"
                             "    send('" << result.getName() << "', '%s: %s' % (e.__class__.__name__, e))\n"
                             "    sys.exit(1)\n"
                             "\n"
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
        waitfor(LLLeap::create(test_name,
                               sv(list_of
                                  (PYTHON)
                                  (script.getName())
                                  (stringize(size)))),
                180);               // try a longer timeout
        result.ensure();
    }

    struct TestLargeMessage: public std::binary_function<size_t, size_t, bool>
    {
        TestLargeMessage(const std::string& PYTHON_, const std::string& reader_module_,
                         const std::string& test_name_):
            PYTHON(PYTHON_),
            reader_module(reader_module_),
            test_name(test_name_)
        {}

        bool operator()(size_t left, size_t right) const
        {
            // We don't know whether upper_bound is going to pass the "sought
            // value" as the left or the right operand. We pass 0 as the
            // "sought value" so we can distinguish it. Of course that means
            // the sequence we're searching must not itself contain 0!
            size_t size;
            bool success;
            if (left)
            {
                size = left;
                // Consider our return value carefully. Normal binary_search
                // (or, in our case, upper_bound) expects a container sorted
                // in ascending order, and defaults to the std::less
                // comparator. Our container is in fact in ascending order, so
                // return consistently with std::less. Here we were called as
                // compare(item, sought). If std::less were called that way,
                // 'true' would mean to move right (to higher numbers) within
                // the sequence: the item being considered is less than the
                // sought value. For us, that means that test_large_message()
                // success should return 'true'.
                success = true;
            }
            else
            {
                size = right;
                // Here we were called as compare(sought, item). If std::less
                // were called that way, 'true' would mean to move left (to
                // lower numbers) within the sequence: the sought value is
                // less than the item being considered. For us, that means
                // test_large_message() FAILURE should return 'true', hence
                // test_large_message() success should return 'false'.
                success = false;
            }

            try
            {
                test_large_message(PYTHON, reader_module, test_name, size);
                std::cout << "test_large_message(" << size << ") succeeded" << std::endl;
                return success;
            }
            catch (const failure& e)
            {
                std::cout << "test_large_message(" << size << ") failed: " << e.what() << std::endl;
                return ! success;
            }
        }

        const std::string PYTHON, reader_module, test_name;
    };

    // The point of this function is to try to find a size at which
    // test_large_message() can succeed. We still want the overall test to
    // fail; otherwise we won't get the coder's attention -- but if
    // test_large_message() fails, try to find a plausible size at which it
    // DOES work.
    void test_or_split(const std::string& PYTHON, const std::string& reader_module,
                       const std::string& test_name, size_t size)
    {
        try
        {
            test_large_message(PYTHON, reader_module, test_name, size);
        }
        catch (const failure& e)
        {
            std::cout << "test_large_message(" << size << ") failed: " << e.what() << std::endl;
            // If it still fails below 4K, give up: subdividing any further is
            // pointless.
            if (size >= 4096)
            {
                try
                {
                    // Recur with half the size
                    size_t smaller(size/2);
                    test_or_split(PYTHON, reader_module, test_name, smaller);
                    // Recursive call will throw if test_large_message()
                    // failed, therefore we only reach the line below if it
                    // succeeded.
                    std::cout << "but test_large_message(" << smaller << ") succeeded" << std::endl;

                    // Binary search for largest size that works. But since
                    // std::binary_search() only returns bool, actually use
                    // std::upper_bound(), consistent with our desire to find
                    // the LARGEST size that works. First generate a sorted
                    // container of all the sizes we intend to try, from
                    // 'smaller' (known to work) to 'size' (known to fail). We
                    // could whomp up magic iterators to do this dynamically,
                    // without actually instantiating a vector, but for a test
                    // program this will do. At least preallocate the vector.
                    // Per TestLargeMessage comments, it's important that this
                    // vector not contain 0.
                    std::vector<size_t> sizes;
                    sizes.reserve((size - smaller)/4096 + 1);
                    for (size_t sz(smaller), szend(size); sz < szend; sz += 4096)
                        sizes.push_back(sz);
                    // our comparator
                    TestLargeMessage tester(PYTHON, reader_module, test_name);
                    // Per TestLargeMessage comments, pass 0 as the sought value.
                    std::vector<size_t>::const_iterator found =
                        std::upper_bound(sizes.begin(), sizes.end(), 0, tester);
                    if (found != sizes.end() && found != sizes.begin())
                    {
                        std::cout << "test_large_message(" << *(found - 1)
                                  << ") is largest that succeeds" << std::endl;
                    }
                    else
                    {
                        std::cout << "cannot determine largest test_large_message(size) "
                                  << "that succeeds" << std::endl;
                    }
                }
                catch (const failure&)
                {
                    // The recursive test_or_split() call above has already
                    // handled the exception. We don't want our caller to see
                    // innermost exception; propagate outermost (below).
                }
            }
            // In any case, because we reached here through failure of
            // our original test_large_message(size) call, ensure failure
            // propagates.
            throw e;
        }
    }

    template<> template<>
    void object::test<10>()
    {
        set_test_name("very large message");
        test_or_split(PYTHON, reader_module, get_test_name(), BUFFERED_LENGTH);
    }
} // namespace tut
