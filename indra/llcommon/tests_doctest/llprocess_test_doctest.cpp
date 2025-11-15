/**
 * @file llprocess_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL process
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

// ---------------------------------------------------------------------------
// Auto-generated from llprocess_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "llprocess.h"
#include <vector>
#include <list>
#include <fstream>
#include "llapr.h"
#include "apr_thread_proc.h"
#include <boost/function.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
#include "../test/namedtempfile.h"
#include "../test/catch_and_store_what_in.h"
#include "stringize.h"
#include "llsdutil.h"
#include "llevents.h"
#include "llstring.h"
// #include <sys/wait.h>  // not available on Windows

TUT_SUITE("llcommon")
{
    TUT_CASE("llprocess_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("raw APR nonblocking I/O");

        //         // Create a script file in a temporary place.
        //         NamedExtTempFile script("py",
        //             "from __future__ import print_function" EOL
        //             "import sys" EOL
        //             "import time" EOL
        //             EOL
        //             "time.sleep(2)" EOL
        //             "print('stdout after wait',file=sys.stdout)" EOL
        //             "sys.stdout.flush()" EOL
        //             "time.sleep(2)" EOL
        //             "print('stderr after wait',file=sys.stderr)" EOL
        //             "sys.stderr.flush()" EOL
        //         );

        //         // Arrange to track the history of our interaction with child: what we
        //         // fetched, which pipe it came from, how many tries it took before we
        //         // got it.
        //         std::vector<Item> history;
        //         history.push_back(Item());

        //         // Run the child process.
        //         apr_procattr_t *procattr = NULL;
        //         aprchk(apr_procattr_create(&procattr, pool.getAPRPool()));
        //         aprchk(apr_procattr_io_set(procattr, APR_CHILD_BLOCK, APR_CHILD_BLOCK, APR_CHILD_BLOCK));
        //         aprchk(apr_procattr_cmdtype_set(procattr, APR_PROGRAM_PATH));

        //         std::vector<const char*> argv;
        //         apr_proc_t child;
        // #if defined(LL_WINDOWS)
        //         argv.push_back("python");
        // #else
        //         argv.push_back("python3");
        // #endif
        //         // Have to have a named copy of this std::string so its c_str() value
        //         // will persist.
        //         std::string scriptname(script.getName());
        //         argv.push_back(scriptname.c_str());
        //         argv.push_back(NULL);

        //         aprchk(apr_proc_create(&child, argv[0],
        //                                &argv[0],
        //                                NULL, // if we wanted to pass explicit environment
        //                                procattr,
        //                                pool.getAPRPool()));

        //         // We do not want this child process to outlive our APR pool. On
        //         // destruction of the pool, forcibly kill the process. Tell APR to try
        //         // SIGTERM and wait 3 seconds. If that didn't work, use SIGKILL.
        //         apr_pool_note_subprocess(pool.getAPRPool(), &child, APR_KILL_AFTER_TIMEOUT);

        //         // arrange to call child_status_callback()
        //         WaitInfo wi(&child);
        //         apr_proc_other_child_register(&child, child_status_callback, &wi, child.in, pool.getAPRPool());

        //         // TODO:
        //         // Stuff child.in until it (would) block to verify EWOULDBLOCK/EAGAIN.
        //         // Have child script clear it later, then write one more line to prove
        //         // that it gets through.

        //         // Monitor two different output pipes. Because one will be closed
        //         // before the other, keep them in a list so we can drop whichever of
        //         // them is closed first.
        //         typedef std::pair<std::string, apr_file_t*> DescFile;
        //         typedef std::list<DescFile> DescFileList;
        //         DescFileList outfiles;
        //         outfiles.push_back(DescFile("out", child.out));
        //         outfiles.push_back(DescFile("err", child.err));

        //         while (! outfiles.empty())
        //         {
        //             // This peculiar for loop is designed to let us erase(dfli). With
        //             // a list, that invalidates only dfli itself -- but even so, we
        //             // lose the ability to increment it for the next item. So at the
        //             // top of every loop, while dfli is still valid, increment
        //             // dflnext. Then before the next iteration, set dfli to dflnext.
        //             for (DescFileList::iterator
        //                      dfli(outfiles.begin()), dflnext(outfiles.begin()), dflend(outfiles.end());
        //                  dfli != dflend; dfli = dflnext)
        //             {
        //                 // Only valid to increment dflnext once we're sure it's not
        //                 // already at dflend.
        //                 ++dflnext;

        //                 char buf[4096];

        //                 apr_status_t rv = apr_file_gets(buf, sizeof(buf), dfli->second);
        //                 if (APR_STATUS_IS_EOF(rv))
        //                 {
        // //                  std::cout << "(EOF on " << dfli->first << ")\n";
        // //                  history.back().which = dfli->first;
        // //                  history.back().what  = "*eof*";
        // //                  history.push_back(Item());
        //                     outfiles.erase(dfli);
        //                     continue;
        //                 }
        //                 if (rv == EWOULDBLOCK || rv == EAGAIN)
        //                 {
        // //                  std::cout << "(waiting; apr_file_gets(" << dfli->first << ") => " << rv << ": " << manager.strerror(rv) << ")\n";
        //                     ++history.back().tries;
        //                     continue;
        //                 }
        //                 aprchk_("apr_file_gets(buf, sizeof(buf), dfli->second)", rv);
        //                 // Is it even possible to get APR_SUCCESS but read 0 bytes?
        //                 // Hope not, but defend against that anyway.
        //                 if (buf[0])
        //                 {
        // //                  std::cout << dfli->first << ": " << buf;
        //                     history.back().which = dfli->first;
        //                     history.back().what.append(buf);
        //                     if (buf[strlen(buf) - 1] == '\n')
        //                         history.push_back(Item());
        //                     else
        //                     {
        //                         // Just for pretty output... if we only read a partial
        //                         // line, terminate it.
        // //                      std::cout << "...\n";
        //                     }
        //                 }
        //             }
        //             // Do this once per tick, as we expect the viewer will
        //             apr_proc_other_child_refresh_all(APR_OC_REASON_RUNNING);
        //             sleep(1);
        //         }
        //         apr_file_close(child.in);
        //         apr_file_close(child.out);
        //         apr_file_close(child.err);

        //         // Okay, we've broken the loop because our pipes are all closed. If we
        //         // haven't yet called wait, give the callback one more chance. This
        //         // models the fact that unlike this small test program, the viewer
        //         // will still be running.
        //         if (wi.rv == -1)
        //         {
        //             std::cout << "last gasp apr_proc_other_child_refresh_all()\n";
        //             apr_proc_other_child_refresh_all(APR_OC_REASON_RUNNING);
        //         }

        //         if (wi.rv == -1)
        //         {
        //             std::cout << "child_status_callback(APR_OC_REASON_DEATH) wasn't called" << std::endl;
        //             wi.rv = apr_proc_wait(wi.child, &wi.rc, &wi.why, APR_NOWAIT);
        //         }
        // //      std::cout << "child done: rv = " << rv << " (" << manager.strerror(rv) << "), why = " << why << ", rc = " << rc << '\n';
        //         aprchk_("apr_proc_wait(wi->child, &wi->rc, &wi->why, APR_NOWAIT)", wi.rv, APR_CHILD_DONE);

        //         // Beyond merely executing all the above successfully, verify that we
        //         // obtained expected output -- and that we duly got control while
        //         // waiting, proving the non-blocking nature of these pipes.
        //         try
        //         {
        //             // Perform these ensure_equals_() within this try/catch so that if
        //             // we don't get expected results, we'll dump whatever we did get
        //             // to help diagnose.
        //             ensure_equals_(wi.why, APR_PROC_EXIT);
        //             ensure_equals_(wi.rc, 0);

        //             unsigned i = 0;
        //             ensure("blocking I/O on child pipe (0)", history[i].tries);
        //             ensure_equals_(history[i].which, "out");
        //             ensure_equals_(history[i].what,  "stdout after wait" EOL);
        // //          ++i;
        // //          ensure_equals_(history[i].which, "out");
        // //          ensure_equals_(history[i].what,  "*eof*");
        //             ++i;
        //             ensure("blocking I/O on child pipe (1)", history[i].tries);
        //             ensure_equals_(history[i].which, "err");
        //             ensure_equals_(history[i].what,  "stderr after wait" EOL);
        // //          ++i;
        // //          ensure_equals_(history[i].which, "err");
        // //          ensure_equals_(history[i].what,  "*eof*");
        //         }
        //         catch (const failure&)
        //         {
        //             std::cout << "History:\n";
        //             for (const Item& item : history)
        //             {
        //                 std::string what(item.what);
        //                 if ((! what.empty()) && what[what.length() - 1] == '\n')
        //                 {
        //                     what.erase(what.length() - 1);
        //                     if ((! what.empty()) && what[what.length() - 1] == '\r')
        //                     {
        //                         what.erase(what.length() - 1);
        //                         what.append("\\r");
        //                     }
        //                     what.append("\\n");
        //                 }
        //                 std::cout << "  " << item.which << ": '" << what << "' ("
        //                           << item.tries << " tries)\n";
        //             }
        //             std::cout << std::flush;
        //             // re-raise same error; just want to enrich the output
        //             throw;
        //         }
        //     }
    }

    TUT_CASE("llprocess_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         set_test_name("setWorkingDirectory()");
        //         // We want to test setWorkingDirectory(). But what directory is
        //         // guaranteed to exist on every machine, under every OS? Have to
        //         // create one. Naturally, ensure we clean it up when done.
        //         NamedTempDir tempdir;
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "from __future__ import with_statement\n"
        //                                  "import os, sys\n"
        //                                  "with open(sys.argv[1], 'w') as f:\n"
        //                                  "    f.write(os.path.normcase(os.path.normpath(os.getcwd())))\n");
        //         // Before running, call setWorkingDirectory()
        //         py.mParams.cwd = tempdir.getName();
        //         std::string expected{ tempdir.getName() };
        // #if LL_WINDOWS
        //         // SIGH, don't get tripped up by "C:" != "c:" --
        //         // but on the Mac, using tolower() fails because "/users" != "/Users"!
        //         expected = utf8str_tolower(expected);
        // #endif
        //         ensure_equals("os.getcwd()", py.run_read(), expected);
        //     }
    }

    TUT_CASE("llprocess_test::object_test_3")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<3>()
        //     {
        //         set_test_name("arguments");
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "from __future__ import with_statement, print_function\n"
        //                                  "import sys\n"
        //                                  // note nonstandard output-file arg!
        //                                  "with open(sys.argv[3], 'w') as f:\n"
        //                                  "    for arg in sys.argv[1:]:\n"
        //                                  "        print(arg,file=f)\n");
        //         // We expect that PythonProcessLauncher has already appended
        //         // its own NamedTempFile to mParams.args (sys.argv[0]).
        //         py.mParams.args.add("first arg");          // sys.argv[1]
        //         py.mParams.args.add("second arg");         // sys.argv[2]
        //         // run_read() appends() one more argument, hence [3]
        //         std::string output(py.run_read());
        //         boost::split_iterator<std::string::const_iterator>
        //             li(output, boost::first_finder("\n")), lend;
        //         ensure("didn't get first arg", li != lend);
        //         std::string arg(li->begin(), li->end());
        //         ensure_equals(arg, "first arg");
        //         ++li;
        //         ensure("didn't get second arg", li != lend);
        //         arg.assign(li->begin(), li->end());
        //         ensure_equals(arg, "second arg");
        //         ++li;
        //         ensure("didn't get output filename?!", li != lend);
        //         arg.assign(li->begin(), li->end());
        //         ensure("output filename empty?!", ! arg.empty());
        //         ++li;
        //         ensure("too many args", li == lend);
        //     }
    }

    TUT_CASE("llprocess_test::object_test_4")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<4> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<4>()
        //     {
        //         set_test_name("exit(0)");
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "import sys\n"
        //                                  "sys.exit(0)\n");
        //         py.run();
        //         ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        //         ensure_equals("Status.mData",  py.mPy->getStatus().mData,  0);
        //     }
    }

    TUT_CASE("llprocess_test::object_test_5")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<5> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<5>()
        //     {
        //         set_test_name("exit(2)");
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "import sys\n"
        //                                  "sys.exit(2)\n");
        //         py.run();
        //         ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        //         ensure_equals("Status.mData",  py.mPy->getStatus().mData,  2);
        //     }
    }

    TUT_CASE("llprocess_test::object_test_6")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<6> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<6>()
        //     {
        //         set_test_name("syntax_error");
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "syntax_error:\n");
        //         py.mParams.files.add(LLProcess::FileParam()); // inherit stdin
        //         py.mParams.files.add(LLProcess::FileParam()); // inherit stdout
        //         py.mParams.files.add(LLProcess::FileParam().type("pipe")); // pipe for stderr
        //         py.run();
        //         ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        //         ensure_equals("Status.mData",  py.mPy->getStatus().mData,  1);
        //         std::istream& rpipe(py.mPy->getReadPipe(LLProcess::STDERR).get_istream());
        //         std::vector<char> buffer(4096);
        //         rpipe.read(&buffer[0], buffer.size());
        //         std::streamsize got(rpipe.gcount());
        //         ensure("Nothing read from stderr pipe", got);
        //         std::string data(&buffer[0], got);
        //         ensure("Didn't find 'SyntaxError:'", data.find("\nSyntaxError:") != std::string::npos);
        //     }
    }

    TUT_CASE("llprocess_test::object_test_7")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<7> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<7>()
        //     {
        //         set_test_name("explicit kill()");
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "from __future__ import with_statement\n"
        //                                  "import sys, time\n"
        //                                  "with open(sys.argv[1], 'w') as f:\n"
        //                                  "    f.write('ok')\n"
        //                                  "# now sleep; expect caller to kill\n"
        //                                  "time.sleep(120)\n"
        //                                  "# if caller hasn't managed to kill by now, bad\n"
        //                                  "with open(sys.argv[1], 'w') as f:\n"
        //                                  "    f.write('bad')\n");
        //         NamedTempFile out("out", "not started");
        //         py.mParams.args.add(out.getName());
        //         py.launch();
        //         // Wait for the script to wake up and do its first write
        //         int i = 0, timeout = 60;
        //         for ( ; i < timeout; ++i)
        //         {
        //             yield();
        //             if (readfile(out.getName(), "from kill() script") == "ok")
        //                 break;
        //         }
        //         // If we broke this loop because of the counter, something's wrong
        //         ensure("script never started", i < timeout);
        //         // script has performed its first write and should now be sleeping.
        //         py.mPy->kill();
        //         // wait for the script to terminate... one way or another.
        //         waitfor(*py.mPy);
        // #if LL_WINDOWS
        //         ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        //         ensure_equals("Status.mData",  py.mPy->getStatus().mData,  -1);
        // #else
        //         ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::KILLED);
        //         ensure_equals("Status.mData",  py.mPy->getStatus().mData,  SIGTERM);
        // #endif
        //         // If kill() failed, the script would have woken up on its own and
        //         // overwritten the file with 'bad'. But if kill() succeeded, it should
        //         // not have had that chance.
        //         ensure_equals(get_test_name() + " script output", readfile(out.getName()), "ok");
        //     }
    }

    TUT_CASE("llprocess_test::object_test_8")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<8> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<8>()
        //     {
        //         set_test_name("implicit kill()");
        //         NamedTempFile out("out", "not started");
        //         LLProcess::handle phandle(0);
        //         {
        //             PythonProcessLauncher py(get_test_name(),
        //                                      "from __future__ import with_statement\n"
        //                                      "import sys, time\n"
        //                                      "with open(sys.argv[1], 'w') as f:\n"
        //                                      "    f.write('ok')\n"
        //                                      "# now sleep; expect caller to kill\n"
        //                                      "time.sleep(120)\n"
        //                                      "# if caller hasn't managed to kill by now, bad\n"
        //                                      "with open(sys.argv[1], 'w') as f:\n"
        //                                      "    f.write('bad')\n");
        //             py.mParams.args.add(out.getName());
        //             py.launch();
        //             // Capture handle for later
        //             phandle = py.mPy->getProcessHandle();
        //             // Wait for the script to wake up and do its first write
        //             int i = 0, timeout = 60;
        //             for ( ; i < timeout; ++i)
        //             {
        //                 yield();
        //                 if (readfile(out.getName(), "from kill() script") == "ok")
        //                     break;
        //             }
        //             // If we broke this loop because of the counter, something's wrong
        //             ensure("script never started", i < timeout);
        //             // Script has performed its first write and should now be sleeping.
        //             // Destroy the LLProcess, which should kill the child.
        //         }
        //         // wait for the script to terminate... one way or another.
        //         waitfor(phandle, "kill() script");
        //         // If kill() failed, the script would have woken up on its own and
        //         // overwritten the file with 'bad'. But if kill() succeeded, it should
        //         // not have had that chance.
        //         ensure_equals(get_test_name() + " script output", readfile(out.getName()), "ok");
        //     }
    }

    TUT_CASE("llprocess_test::object_test_9")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<9> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<9>()
        //     {
        //         set_test_name("autokill=false");
        //         NamedTempFile from("from", "not started");
        //         NamedTempFile to("to", "");
        //         LLProcess::handle phandle(0);
        //         {
        //             PythonProcessLauncher py(get_test_name(),
        //                                      "from __future__ import with_statement\n"
        //                                      "import sys, time\n"
        //                                      "with open(sys.argv[1], 'w') as f:\n"
        //                                      "    f.write('ok')\n"
        //                                      "# wait for 'go' from test program\n"
        //                                      "for i in range(60):\n"
        //                                      "    time.sleep(1)\n"
        //                                      "    with open(sys.argv[2]) as f:\n"
        //                                      "        go = f.read()\n"
        //                                      "    if go == 'go':\n"
        //                                      "        break\n"
        //                                      "else:\n"
        //                                      "    with open(sys.argv[1], 'w') as f:\n"
        //                                      "        f.write('never saw go')\n"
        //                                      "    sys.exit(1)\n"
        //                                      "# okay, saw 'go', write 'ack'\n"
        //                                      "with open(sys.argv[1], 'w') as f:\n"
        //                                      "    f.write('ack')\n");
        //             py.mParams.args.add(from.getName());
        //             py.mParams.args.add(to.getName());
        //             py.mParams.autokill = false;
        //             py.launch();
        //             // Capture handle for later
        //             phandle = py.mPy->getProcessHandle();
        //             // Wait for the script to wake up and do its first write
        //             int i = 0, timeout = 60;
        //             for ( ; i < timeout; ++i)
        //             {
        //                 yield();
        //                 if (readfile(from.getName(), "from autokill script") == "ok")
        //                     break;
        //             }
        //             // If we broke this loop because of the counter, something's wrong
        //             ensure("script never started", i < timeout);
        //             // Now destroy the LLProcess, which should NOT kill the child!
        //         }
        //         // If the destructor killed the child anyway, give it time to die
        //         yield(2);
        //         // How do we know it's not terminated? By making it respond to
        //         // a specific stimulus in a specific way.
        //         {
        //             std::ofstream outf(to.getName().c_str());
        //             outf << "go";
        //         } // flush and close.
        //         // now wait for the script to terminate... one way or another.
        //         waitfor(phandle, "autokill script");
        //         // If the LLProcess destructor implicitly called kill(), the
        //         // script could not have written 'ack' as we expect.
        //         ensure_equals(get_test_name() + " script output", readfile(from.getName()), "ack");
        //     }
    }

    TUT_CASE("llprocess_test::object_test_10")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<10> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<10>()
        //     {
        //         set_test_name("attached=false");
        //         // almost just like autokill=false, except set autokill=true with
        //         // attached=false.
        //         NamedTempFile from("from", "not started");
        //         NamedTempFile to("to", "");
        //         LLProcess::handle phandle(0);
        //         {
        //             PythonProcessLauncher py(get_test_name(),
        //                                      "from __future__ import with_statement\n"
        //                                      "import sys, time\n"
        //                                      "with open(sys.argv[1], 'w') as f:\n"
        //                                      "    f.write('ok')\n"
        //                                      "# wait for 'go' from test program\n"
        //                                      "for i in range(60):\n"
        //                                      "    time.sleep(1)\n"
        //                                      "    with open(sys.argv[2]) as f:\n"
        //                                      "        go = f.read()\n"
        //                                      "    if go == 'go':\n"
        //                                      "        break\n"
        //                                      "else:\n"
        //                                      "    with open(sys.argv[1], 'w') as f:\n"
        //                                      "        f.write('never saw go')\n"
        //                                      "    sys.exit(1)\n"
        //                                      "# okay, saw 'go', write 'ack'\n"
        //                                      "with open(sys.argv[1], 'w') as f:\n"
        //                                      "    f.write('ack')\n");
        //             py.mParams.args.add(from.getName());
        //             py.mParams.args.add(to.getName());
        //             py.mParams.autokill = true;
        //             py.mParams.attached = false;
        //             py.launch();
        //             // Capture handle for later
        //             phandle = py.mPy->getProcessHandle();
        //             // Wait for the script to wake up and do its first write
        //             int i = 0, timeout = 60;
        //             for ( ; i < timeout; ++i)
        //             {
        //                 yield();
        //                 if (readfile(from.getName(), "from autokill script") == "ok")
        //                     break;
        //             }
        //             // If we broke this loop because of the counter, something's wrong
        //             ensure("script never started", i < timeout);
        //             // Now destroy the LLProcess, which should NOT kill the child!
        //         }
        //         // If the destructor killed the child anyway, give it time to die
        //         yield(2);
        //         // How do we know it's not terminated? By making it respond to
        //         // a specific stimulus in a specific way.
        //         {
        //             std::ofstream outf(to.getName().c_str());
        //             outf << "go";
        //         } // flush and close.
        //         // now wait for the script to terminate... one way or another.
        //         waitfor(phandle, "autokill script");
        //         // If the LLProcess destructor implicitly called kill(), the
        //         // script could not have written 'ack' as we expect.
        //         ensure_equals(get_test_name() + " script output", readfile(from.getName()), "ack");
        //     }
    }

    TUT_CASE("llprocess_test::object_test_11")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<11> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<11>()
        //     {
        //         set_test_name("'bogus' test");
        //         CaptureLog recorder;
        //         PythonProcessLauncher py(get_test_name(),
        //             "from __future__ import print_function\n"
        //             "print('Hello world')\n");
        //         py.mParams.files.add(LLProcess::FileParam("bogus"));
        //         py.mPy = LLProcess::create(py.mParams);
        //         ensure("should have rejected 'bogus'", ! py.mPy);
        //         std::string message(recorder.messageWith("bogus"));
        //         ensure_contains("did not name 'stdin'", message, "stdin");
        //     }
    }

    TUT_CASE("llprocess_test::object_test_12")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<12> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<12>()
        //     {
        //         set_test_name("'file' test");
        //         // Replace this test with one or more real 'file' tests when we
        //         // implement 'file' support
        //         PythonProcessLauncher py(get_test_name(),
        //             "from __future__ import print_function\n"
        //             "print('Hello world')\n");
        //         py.mParams.files.add(LLProcess::FileParam());
        //         py.mParams.files.add(LLProcess::FileParam("file"));
        //         py.mPy = LLProcess::create(py.mParams);
        //         ensure("should have rejected 'file'", ! py.mPy);
        //     }
    }

    TUT_CASE("llprocess_test::object_test_13")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<13> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<13>()
        //     {
        //         set_test_name("'tpipe' test");
        //         // Replace this test with one or more real 'tpipe' tests when we
        //         // implement 'tpipe' support
        //         CaptureLog recorder;
        //         PythonProcessLauncher py(get_test_name(),
        //             "from __future__ import print_function\n"
        //             "print('Hello world')\n");
        //         py.mParams.files.add(LLProcess::FileParam());
        //         py.mParams.files.add(LLProcess::FileParam("tpipe"));
        //         py.mPy = LLProcess::create(py.mParams);
        //         ensure("should have rejected 'tpipe'", ! py.mPy);
        //         std::string message(recorder.messageWith("tpipe"));
        //         ensure_contains("did not name 'stdout'", message, "stdout");
        //     }
    }

    TUT_CASE("llprocess_test::object_test_14")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<14> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<14>()
        //     {
        //         set_test_name("'npipe' test");
        //         // Replace this test with one or more real 'npipe' tests when we
        //         // implement 'npipe' support
        //         CaptureLog recorder;
        //         PythonProcessLauncher py(get_test_name(),
        //             "from __future__ import print_function\n"
        //             "print('Hello world')\n");
        //         py.mParams.files.add(LLProcess::FileParam());
        //         py.mParams.files.add(LLProcess::FileParam());
        //         py.mParams.files.add(LLProcess::FileParam("npipe"));
        //         py.mPy = LLProcess::create(py.mParams);
        //         ensure("should have rejected 'npipe'", ! py.mPy);
        //         std::string message(recorder.messageWith("npipe"));
        //         ensure_contains("did not name 'stderr'", message, "stderr");
        //     }
    }

    TUT_CASE("llprocess_test::object_test_15")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<15> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<15>()
        //     {
        //         set_test_name("internal pipe name warning");
        //         CaptureLog recorder;
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "import sys\n"
        //                                  "sys.exit(7)\n");
        //         py.mParams.files.add(LLProcess::FileParam("pipe", "somename"));
        //         py.run();                   // verify that it did launch anyway
        //         ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        //         ensure_equals("Status.mData",  py.mPy->getStatus().mData,  7);
        //         std::string message(recorder.messageWith("not yet supported"));
        //         ensure_contains("log message did not mention internal pipe name",
        //                         message, "somename");
        //     }
    }

    TUT_CASE("llprocess_test::object_test_16")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<16> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<16>()
        //     {
        //         set_test_name("get*Pipe() validation");
        //         PythonProcessLauncher py(get_test_name(),
        //             "from __future__ import print_function\n"
        //             "print('this output is expected')\n");
        //         py.mParams.files.add(LLProcess::FileParam("pipe")); // pipe for  stdin
        //         py.mParams.files.add(LLProcess::FileParam());       // inherit stdout
        //         py.mParams.files.add(LLProcess::FileParam("pipe")); // pipe for stderr
        //         py.run();
        //         TEST_getPipe(*py.mPy, getWritePipe, getOptWritePipe,
        //             LLProcess::STDIN,   // VALID
        //             LLProcess::STDOUT,  // NOPIPE
        //             LLProcess::STDERR); // BADPIPE
        //         TEST_getPipe(*py.mPy, getReadPipe,  getOptReadPipe,
        //             LLProcess::STDERR,  // VALID
        //             LLProcess::STDOUT,  // NOPIPE
        //             LLProcess::STDIN);  // BADPIPE
        //     }
    }

    TUT_CASE("llprocess_test::object_test_17")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<17> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<17>()
        //     {
        //         set_test_name("talk to stdin/stdout");
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "from __future__ import print_function\n"
        //                                  "import sys, time\n"
        //                                  "print('ok')\n"
        //                                  "sys.stdout.flush()\n"
        //                                  "# wait for 'go' from test program\n"
        //                                  "go = sys.stdin.readline()\n"
        //                                  "if go != 'go\\n':\n"
        //                                  "    sys.exit('expected \"go\", saw %r' % go)\n"
        //                                  "print('ack')\n");
        //         py.mParams.files.add(LLProcess::FileParam("pipe")); // stdin
        //         py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        //         py.launch();
        //         LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        //         int i, timeout = 60;
        //         for (i = 0; i < timeout && py.mPy->isRunning() && childout.size() < 3; ++i)
        //         {
        //             yield();
        //         }
        //         ensure("script never started", i < timeout);
        //         ensure_equals("bad wakeup from stdin/stdout script",
        //                       childout.getline(), "ok");
        //         // important to get the implicit flush from std::endl
        //         py.mPy->getWritePipe().get_ostream() << "go" << std::endl;
        //         waitfor(*py.mPy);
        //         ensure("script never replied", childout.contains("\n"));
        //         ensure_equals("child didn't ack", childout.getline(), "ack");
        //         ensure_equals("bad child termination", py.mPy->getStatus().mState, LLProcess::EXITED);
        //         ensure_equals("bad child exit code",   py.mPy->getStatus().mData,  0);
        //     }
    }

    TUT_CASE("llprocess_test::object_test_18")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<18> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<18>()
        //     {
        //         set_test_name("listen for ReadPipe events");
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "import sys\n"
        //                                  "sys.stdout.write('abc')\n"
        //                                  "sys.stdout.flush()\n"
        //                                  "sys.stdin.readline()\n"
        //                                  "sys.stdout.write('def')\n"
        //                                  "sys.stdout.flush()\n"
        //                                  "sys.stdin.readline()\n"
        //                                  "sys.stdout.write('ghi\\n')\n"
        //                                  "sys.stdout.flush()\n"
        //                                  "sys.stdin.readline()\n"
        //                                  "sys.stdout.write('second line\\n')\n");
        //         py.mParams.files.add(LLProcess::FileParam("pipe")); // stdin
        //         py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        //         py.launch();
        //         std::ostream& childin(py.mPy->getWritePipe(LLProcess::STDIN).get_ostream());
        //         LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        //         // lift the default limit; allow event to carry (some of) the actual data
        //         childout.setLimit(20);
        //         // listen for incoming data on childout
        //         EventListener listener(childout.getPump());
        //         // also listen with a function that prompts the child to continue
        //         // every time we see output
        //         LLTempBoundListener connection(
        //             childout.getPump().listen("ack", boost::bind(ack, boost::ref(childin), _1)));
        //         int i, timeout = 60;
        //         // wait through stuttering first line
        //         for (i = 0; i < timeout && py.mPy->isRunning() && ! childout.contains("\n"); ++i)
        //         {
        //             yield();
        //         }
        //         ensure("couldn't get first line", i < timeout);
        //         // disconnect from listener
        //         listener.mConnection.disconnect();
        //         // finish out the run
        //         waitfor(*py.mPy);
        //         // now verify history
        //         listener.checkHistory(
        //             [](const EventListener::Listory& history)
        //             {
        //                 auto li(history.begin()), lend(history.end());
        //                 ensure("no events", li != lend);
        //                 ensure_equals("history[0]", (*li)["data"].asString(), "abc");
        //                 ensure_equals("history[0] len", (*li)["len"].asInteger(), 3);
        //                 ++li;
        //                 ensure("only 1 event", li != lend);
        //                 ensure_equals("history[1]", (*li)["data"].asString(), "abcdef");
        //                 ensure_equals("history[0] len", (*li)["len"].asInteger(), 6);
        //                 ++li;
        //                 ensure("only 2 events", li != lend);
        //                 ensure_equals("history[2]", (*li)["data"].asString(), "abcdefghi" EOL);
        //                 ensure_equals("history[0] len", (*li)["len"].asInteger(), 9 + sizeof(EOL) - 1);
        //                 ++li;
        //                 // We DO NOT expect a whole new event for the second line because we
        //                 // disconnected.
        //                 ensure("more than 3 events", li == lend);
        //             });
        //     }
    }

    TUT_CASE("llprocess_test::object_test_19")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<19> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<19>()
        //     {
        //         set_test_name("ReadPipe \"eof\" event");
        //         PythonProcessLauncher py(get_test_name(),
        //             "from __future__ import print_function\n"
        //             "print('Hello from Python!')\n");
        //         py.mParams.files.add(LLProcess::FileParam()); // stdin
        //         py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        //         py.launch();
        //         LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        //         EventListener listener(childout.getPump());
        //         waitfor(*py.mPy);
        //         // We can't be positive there will only be a single event, if the OS
        //         // (or any other intervening layer) does crazy buffering. What we want
        //         // to ensure is that there was exactly ONE event with "eof" true, and
        //         // that it was the LAST event.
        //         listener.checkHistory(
        //             [](const EventListener::Listory& history)
        //             {
        //                 auto rli(history.rbegin()), rlend(history.rend());
        //                 ensure("no events", rli != rlend);
        //                 ensure("last event not \"eof\"", (*rli)["eof"].asBoolean());
        //                 while (++rli != rlend)
        //                 {
        //                     ensure("\"eof\" event not last", ! (*rli)["eof"].asBoolean());
        //                 }
        //             });
        //     }
    }

    TUT_CASE("llprocess_test::object_test_20")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<20> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<20>()
        //     {
        //         set_test_name("setLimit()");
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "import sys\n"
        //                                  "sys.stdout.write(sys.argv[1])\n");
        //         std::string abc("abcdefghijklmnopqrstuvwxyz");
        //         py.mParams.args.add(abc);
        //         py.mParams.files.add(LLProcess::FileParam()); // stdin
        //         py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        //         py.launch();
        //         LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        //         // listen for incoming data on childout
        //         EventListener listener(childout.getPump());
        //         // but set limit
        //         childout.setLimit(10);
        //         ensure_equals("getLimit() after setlimit(10)", childout.getLimit(), 10);
        //         // okay, pump I/O to pick up output from child
        //         waitfor(*py.mPy);
        //         listener.checkHistory(
        //             [abc](const EventListener::Listory& history)
        //             {
        //                 ensure("no events", ! history.empty());
        //                 // For all we know, that data could have arrived in several different
        //                 // bursts... probably not, but anyway, only check the last one.
        //                 ensure_equals("event[\"len\"]",
        //                               history.back()["len"].asInteger(), abc.length());
        //                 ensure_equals("length of setLimit(10) data",
        //                               history.back()["data"].asString().length(), 10);
        //             });
        //     }
    }

    TUT_CASE("llprocess_test::object_test_21")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<21> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<21>()
        //     {
        //         set_test_name("peek() ReadPipe data");
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "import sys\n"
        //                                  "sys.stdout.write(sys.argv[1])\n");
        //         std::string abc("abcdefghijklmnopqrstuvwxyz");
        //         py.mParams.args.add(abc);
        //         py.mParams.files.add(LLProcess::FileParam()); // stdin
        //         py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        //         py.launch();
        //         LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        //         // okay, pump I/O to pick up output from child
        //         waitfor(*py.mPy);
        //         // peek() with substr args
        //         ensure_equals("peek()", childout.peek(), abc);
        //         ensure_equals("peek(23)", childout.peek(23), abc.substr(23));
        //         ensure_equals("peek(5, 3)", childout.peek(5, 3), abc.substr(5, 3));
        //         ensure_equals("peek(27, 2)", childout.peek(27, 2), "");
        //         ensure_equals("peek(23, 5)", childout.peek(23, 5), "xyz");
        //         // contains() -- we don't exercise as thoroughly as find() because the
        //         // contains() implementation is trivially (and visibly) based on find()
        //         ensure("contains(\":\")", ! childout.contains(":"));
        //         ensure("contains(':')",   ! childout.contains(':'));
        //         ensure("contains(\"d\")", childout.contains("d"));
        //         ensure("contains('d')",   childout.contains('d'));
        //         ensure("contains(\"klm\")", childout.contains("klm"));
        //         ensure("contains(\"klx\")", ! childout.contains("klx"));
        //         // find()
        //         ensure("find(\":\")", childout.find(":") == LLProcess::ReadPipe::npos);
        //         ensure("find(':')",   childout.find(':') == LLProcess::ReadPipe::npos);
        //         ensure_equals("find(\"d\")", childout.find("d"), 3);
        //         ensure_equals("find('d')",   childout.find('d'), 3);
        //         ensure_equals("find(\"d\", 3)", childout.find("d", 3), 3);
        //         ensure_equals("find('d', 3)",   childout.find('d', 3), 3);
        //         ensure("find(\"d\", 4)", childout.find("d", 4) == LLProcess::ReadPipe::npos);
        //         ensure("find('d', 4)",   childout.find('d', 4) == LLProcess::ReadPipe::npos);
        //         // The case of offset == end and offset > end are different. In the
        //         // first case, we can form a valid (albeit empty) iterator range and
        //         // search that. In the second, guard logic in the implementation must
        //         // realize we can't form a valid iterator range.
        //         ensure("find(\"d\", 26)", childout.find("d", 26) == LLProcess::ReadPipe::npos);
        //         ensure("find('d', 26)",   childout.find('d', 26) == LLProcess::ReadPipe::npos);
        //         ensure("find(\"d\", 27)", childout.find("d", 27) == LLProcess::ReadPipe::npos);
        //         ensure("find('d', 27)",   childout.find('d', 27) == LLProcess::ReadPipe::npos);
        //         ensure_equals("find(\"ghi\")", childout.find("ghi"), 6);
        //         ensure_equals("find(\"ghi\", 6)", childout.find("ghi"), 6);
        //         ensure("find(\"ghi\", 7)", childout.find("ghi", 7) == LLProcess::ReadPipe::npos);
        //         ensure("find(\"ghi\", 26)", childout.find("ghi", 26) == LLProcess::ReadPipe::npos);
        //         ensure("find(\"ghi\", 27)", childout.find("ghi", 27) == LLProcess::ReadPipe::npos);
        //     }
    }

    TUT_CASE("llprocess_test::object_test_22")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<22> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<22>()
        //     {
        //         set_test_name("bad postend");
        //         std::string pumpname("postend");
        //         EventListener listener(LLEventPumps::instance().obtain(pumpname));
        //         LLProcess::Params params;
        //         params.desc = get_test_name();
        //         params.postend = pumpname;
        //         LLProcessPtr child = LLProcess::create(params);
        //         ensure("shouldn't have launched", ! child);
        //         listener.checkHistory(
        //             [&params](const EventListener::Listory& history)
        //             {
        //                 ensure_equals("number of postend events", history.size(), 1);
        //                 LLSD postend(history.front());
        //                 ensure("has id", ! postend.has("id"));
        //                 ensure_equals("desc", postend["desc"].asString(), std::string(params.desc));
        //                 ensure_equals("state", postend["state"].asInteger(), LLProcess::UNSTARTED);
        //                 ensure("has data", ! postend.has("data"));
        //                 std::string error(postend["string"]);
        //                 // All we get from canned parameter validation is a bool, so the
        //                 // "validation failed" message we ourselves generate can't mention
        //                 // "executable" by name. Just check that it's nonempty.
        //                 //ensure_contains("error", error, "executable");
        //                 ensure("string", ! error.empty());
        //             });
        //     }
    }

    TUT_CASE("llprocess_test::object_test_23")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<23> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<23>()
        //     {
        //         set_test_name("good postend");
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "import sys\n"
        //                                  "sys.exit(35)\n");
        //         std::string pumpname("postend");
        //         EventListener listener(LLEventPumps::instance().obtain(pumpname));
        //         py.mParams.postend = pumpname;
        //         py.launch();
        //         LLProcess::id childid(py.mPy->getProcessID());
        //         // Don't use waitfor(), which calls isRunning(); instead wait for an
        //         // event on pumpname.
        //         int i, timeout = 60;
        //         for (i = 0; i < timeout && listener.mHistory.empty(); ++i)
        //         {
        //             yield();
        //         }
        //         listener.checkHistory(
        //             [i, timeout, childid](const EventListener::Listory& history)
        //             {
        //                 ensure("no postend event", i < timeout);
        //                 ensure_equals("number of postend events", history.size(), 1);
        //                 LLSD postend(history.front());
        //                 ensure_equals("id",    postend["id"].asInteger(), childid);
        //                 ensure("desc empty", ! postend["desc"].asString().empty());
        //                 ensure_equals("state", postend["state"].asInteger(), LLProcess::EXITED);
        //                 ensure_equals("data",  postend["data"].asInteger(),  35);
        //                 std::string str(postend["string"]);
        //                 ensure_contains("string", str, "exited");
        //                 ensure_contains("string", str, "35");
        //             });
        //     }
    }

    TUT_CASE("llprocess_test::object_test_24")
    {
        DOCTEST_FAIL("TODO: convert llprocess_test.cpp::object::test<24> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<24>()
        //     {
        //         set_test_name("all data visible at postend");
        //         PythonProcessLauncher py(get_test_name(),
        //                                  "import sys\n"
        //                                  // note, no '\n' in written data
        //                                  "sys.stdout.write('partial line')\n");
        //         std::string pumpname("postend");
        //         py.mParams.files.add(LLProcess::FileParam()); // stdin
        //         py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        //         py.mParams.postend = pumpname;
        //         py.launch();
        //         PostendListener listener(py.mPy->getReadPipe(LLProcess::STDOUT),
        //                                  pumpname,
        //                                  "partial line");
        //         waitfor(*py.mPy);
        //         ensure("postend never triggered", listener.mTriggered);
        //     }
    }

}

