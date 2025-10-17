// ---------------------------------------------------------------------------
// Auto-generated from llleap_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "llleap.h"
#include <functional>
#include "../test/namedtempfile.h"
#include "../test/catch_and_store_what_in.h"
#include "llevents.h"
#include "llprocess.h"
#include "llstring.h"
#include "stringize.h"
// #include "StringVec.h"  // not available on Windows

TUT_SUITE("llcommon")
{
    TUT_CASE("llleap_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert llleap_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("multiple LLLeap instances");
        //         NamedExtTempFile script("py",
        //                                 "import time\n"
        //                                 "time.sleep(1)\n");
        //         LLLeapVector instances;
        //         instances.push_back(LLLeap::create(get_test_name(),
        //                                            StringVec{PYTHON, script.getName()})->getWeak());
        //         instances.push_back(LLLeap::create(get_test_name(),
        //                                            StringVec{PYTHON, script.getName()})->getWeak());
        //         // In this case we're simply establishing that two LLLeap instances
        //         // can coexist without throwing exceptions or bombing in any other
        //         // way. Wait for them to terminate.
        //         waitfor(instances);
        //     }
    }

    TUT_CASE("llleap_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert llleap_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         set_test_name("stderr to log");
        //         NamedExtTempFile script("py",
        //                                 "import sys\n"
        //                                 "sys.stderr.write('''Hello from Python!\n"
        //                                 "note partial line''')\n");
        //         StringVec vcommand{ PYTHON, script.getName() };
        //         CaptureLog log(LLError::LEVEL_INFO);
        //         waitfor(LLLeap::create(get_test_name(), vcommand));
        //         log.messageWith("Hello from Python!");
        //         log.messageWith("note partial line");
        //     }
    }

    TUT_CASE("llleap_test::object_test_3")
    {
        DOCTEST_FAIL("TODO: convert llleap_test.cpp::object::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<3>()
        //     {
        //         set_test_name("bad stdout protocol");
        //         NamedExtTempFile script("py",
        //                                 "print('Hello from Python!')\n");
        //         CaptureLog log(LLError::LEVEL_WARN);
        //         waitfor(LLLeap::create(get_test_name(),
        //                                StringVec{PYTHON, script.getName()}));
        //         ensure_contains("error log line",
        //                         log.messageWith("invalid protocol"), "Hello from Python!");
        //     }
    }

    TUT_CASE("llleap_test::object_test_4")
    {
        DOCTEST_FAIL("TODO: convert llleap_test.cpp::object::test<4> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<4>()
        //     {
        //         set_test_name("leftover stdout");
        //         NamedExtTempFile script("py",
        //                                 "import sys\n"
        //                                 // note lack of newline
        //                                 "sys.stdout.write('Hello from Python!')\n");
        //         CaptureLog log(LLError::LEVEL_WARN);
        //         waitfor(LLLeap::create(get_test_name(),
        //                                StringVec{PYTHON, script.getName()}));
        //         ensure_contains("error log line",
        //                         log.messageWith("Discarding"), "Hello from Python!");
        //     }
    }

    TUT_CASE("llleap_test::object_test_5")
    {
        DOCTEST_FAIL("TODO: convert llleap_test.cpp::object::test<5> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<5>()
        //     {
        //         set_test_name("bad stdout len prefix");
        //         NamedExtTempFile script("py",
        //                                 "import sys\n"
        //                                 "sys.stdout.write('5a2:something')\n");
        //         CaptureLog log(LLError::LEVEL_WARN);
        //         waitfor(LLLeap::create(get_test_name(),
        //                                StringVec{PYTHON, script.getName()}));
        //         ensure_contains("error log line",
        //                         log.messageWith("invalid protocol"), "5a2:");
        //     }
    }

    TUT_CASE("llleap_test::object_test_6")
    {
        DOCTEST_FAIL("TODO: convert llleap_test.cpp::object::test<6> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<6>()
        //     {
        //         set_test_name("empty plugin vector");
        //         std::string threw = catch_what<LLLeap::Error>([](){
        //                 LLLeap::create("empty", StringVec());
        //             });
        //         ensure_contains("LLLeap::Error", threw, "no plugin");
        //         // try the suppress-exception variant
        //         ensure("bad launch returned non-NULL", ! LLLeap::create("empty", StringVec(), false));
        //     }
    }

    TUT_CASE("llleap_test::object_test_7")
    {
        DOCTEST_FAIL("TODO: convert llleap_test.cpp::object::test<7> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<7>()
        //     {
        //         set_test_name("bad launch");
        //         // Synthesize bogus executable name
        //         std::string BADPYTHON(PYTHON.substr(0, PYTHON.length()-1) + "x");
        //         CaptureLog log;
        //         std::string threw = catch_what<LLLeap::Error>([&BADPYTHON](){
        //                 LLLeap::create("bad exe", BADPYTHON);
        //             });
        //         ensure_contains("LLLeap::create() didn't throw", threw, "failed");
        //         log.messageWith("failed");
        //         log.messageWith(BADPYTHON);
        //         // try the suppress-exception variant
        //         ensure("bad launch returned non-NULL", ! LLLeap::create("bad exe", BADPYTHON, false));
        //     }
    }

    TUT_CASE("llleap_test::object_test_8")
    {
        DOCTEST_FAIL("TODO: convert llleap_test.cpp::object::test<8> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<8>()
        //     {
        //         set_test_name("round trip");
        //         AckAPI api;
        //         Result result;
        //         NamedExtTempFile script("py",
        //                                 [&](std::ostream& out){ out <<
        //                                 "from " << reader_module << " import *\n"
        //                                 // make a request on our little API
        //                                 "request(pump='" << api.getName() << "', data={})\n"
        //                                 // wait for its response
        //                                 "resp = get()\n"
        //                                 "result = '' if resp == dict(pump=replypump(), data='ack')\\\n"
        //                                 "            else 'bad: ' + str(resp)\n"
        //                                 "send(pump='" << result.getName() << "', data=result)\n";});
        //         waitfor(LLLeap::create(get_test_name(),
        //                                StringVec{PYTHON, script.getName()}));
        //         result.ensure();
        //     }
    }

    TUT_CASE("llleap_test::object_test_9")
    {
        DOCTEST_FAIL("TODO: convert llleap_test.cpp::object::test<9> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<9>()
        //     {
        //         set_test_name("many small messages");
        //         // It's not clear to me whether there's value in iterating many times
        //         // over a send/receive loop -- I don't think that will exercise any
        //         // interesting corner cases. This test first sends a large number of
        //         // messages, then receives all the responses. The intent is to ensure
        //         // that some of that data stream crosses buffer boundaries, loop
        //         // iterations etc. in OS pipes and the LLLeap/LLProcess implementation.
        //         ReqIDAPI api;
        //         Result result;
        //         NamedExtTempFile script("py",
        //                                 [&](std::ostream& out){ out <<
        //                                 "import sys\n"
        //                                 "from " << reader_module << " import *\n"
        //                                 // Note that since reader imports llsd, this
        //                                 // 'import *' gets us llsd too.
        //                                 "sample = llsd.format_notation(dict(pump='" <<
        //                                 api.getName() << "', data=dict(reqid=999999, reply=replypump())))\n"
        //                                 // The whole packet has length prefix too: "len:data"
        //                                 "samplen = len(str(len(sample))) + 1 + len(sample)\n"
        //                                 // guess how many messages it will take to
        //                                 // accumulate BUFFERED_LENGTH
        //                                 "count = int(" << BUFFERED_LENGTH << "/samplen)\n"
        //                                 "print('Sending %s requests' % count, file=sys.stderr)\n"
        //                                 "for i in range(count):\n"
        //                                 "    request('" << api.getName() << "', dict(reqid=i))\n"
        //                                 // The assumption in this specific test that
        //                                 // replies will arrive in the same order as
        //                                 // requests is ONLY valid because the API we're
        //                                 // invoking sends replies instantly. If the API
        //                                 // had to wait for some external event before
        //                                 // sending its reply, replies could arrive in
        //                                 // arbitrary order, and we'd have to tick them
        //                                 // off from a set.
        //                                 "result = ''\n"
        //                                 "for i in range(count):\n"
        //                                 "    resp = get()\n"
        //                                 "    if resp['data']['reqid'] != i:\n"
        //                                 "        result = 'expected reqid=%s in %s' % (i, resp)\n"
        //                                 "        break\n"
        //                                 "send(pump='" << result.getName() << "', data=result)\n";});
        //         waitfor(LLLeap::create(get_test_name(), StringVec{PYTHON, script.getName()}),
        //                 300);               // needs more realtime than most tests
        //         result.ensure();
        //     }
    }

    TUT_CASE("llleap_test::object_test_10")
    {
        DOCTEST_FAIL("TODO: convert llleap_test.cpp::object::test<10> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<10>()
        //     {
        //         set_test_name("very large message");
        //         test_or_split(PYTHON, reader_module, get_test_name(), BUFFERED_LENGTH);
        //     }
    }

}

