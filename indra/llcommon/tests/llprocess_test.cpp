/**
 * @file   llprocess_test.cpp
 * @author Nat Goodspeed
 * @date   2011-12-19
 * @brief  Test for llprocess.
 * 
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Copyright (c) 2011, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llprocess.h"
// STL headers
#include <vector>
#include <list>
// std headers
#include <fstream>
// external library headers
#include "llapr.h"
#include "apr_thread_proc.h"
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
//#include <boost/lambda/lambda.hpp>
//#include <boost/lambda/bind.hpp>
// other Linden headers
#include "../test/lltut.h"
#include "../test/manageapr.h"
#include "../test/namedtempfile.h"
#include "stringize.h"
#include "llsdutil.h"

#if defined(LL_WINDOWS)
#define sleep(secs) _sleep((secs) * 1000)
#define EOL "\r\n"
#else
#define EOL "\n"
#include <sys/wait.h>
#endif

//namespace lambda = boost::lambda;

// static instance of this manages APR init/cleanup
static ManageAPR manager;

/*****************************************************************************
*   Helpers
*****************************************************************************/

#define ensure_equals_(left, right) \
        ensure_equals(STRINGIZE(#left << " != " << #right), (left), (right))

#define aprchk(expr) aprchk_(#expr, (expr))
static void aprchk_(const char* call, apr_status_t rv, apr_status_t expected=APR_SUCCESS)
{
    tut::ensure_equals(STRINGIZE(call << " => " << rv << ": " << manager.strerror(rv)),
                       rv, expected);
}

/**
 * Read specified file using std::getline(). It is assumed to be an error if
 * the file is empty: don't use this function if that's an acceptable case.
 * Last line will not end with '\n'; this is to facilitate the usual case of
 * string compares with a single line of output.
 * @param pathname The file to read.
 * @param desc Optional description of the file for error message;
 * defaults to "in <pathname>"
 */
static std::string readfile(const std::string& pathname, const std::string& desc="")
{
    std::string use_desc(desc);
    if (use_desc.empty())
    {
        use_desc = STRINGIZE("in " << pathname);
    }
    std::ifstream inf(pathname.c_str());
    std::string output;
    tut::ensure(STRINGIZE("No output " << use_desc), std::getline(inf, output));
    std::string more;
    while (std::getline(inf, more))
    {
        output += '\n' + more;
    }
    return output;
}

/**
 * Construct an LLProcess to run a Python script.
 */
struct PythonProcessLauncher
{
    /**
     * @param desc Arbitrary description for error messages
     * @param script Python script, any form acceptable to NamedTempFile,
     * typically either a std::string or an expression of the form
     * (lambda::_1 << "script content with " << variable_data)
     */
    template <typename CONTENT>
    PythonProcessLauncher(const std::string& desc, const CONTENT& script):
        mDesc(desc),
        mScript("py", script)
    {
        const char* PYTHON(getenv("PYTHON"));
        tut::ensure("Set $PYTHON to the Python interpreter", PYTHON);

        mParams.executable = PYTHON;
        mParams.args.add(mScript.getName());
    }

    /// Run Python script and wait for it to complete.
    void run()
    {
        mPy = LLProcess::create(mParams);
        tut::ensure(STRINGIZE("Couldn't launch " << mDesc << " script"), mPy);
        // One of the irritating things about LLProcess is that
        // there's no API to wait for the child to terminate -- but given
        // its use in our graphics-intensive interactive viewer, it's
        // understandable.
        while (mPy->isRunning())
        {
            sleep(1);
        }
    }

    /**
     * Run a Python script using LLProcess, expecting that it will
     * write to the file passed as its sys.argv[1]. Retrieve that output.
     *
     * Until January 2012, LLProcess provided distressingly few
     * mechanisms for a child process to communicate back to its caller --
     * not even its return code. We've introduced a convention by which we
     * create an empty temp file, pass the name of that file to our child
     * as sys.argv[1] and expect the script to write its output to that
     * file. This function implements the C++ (parent process) side of
     * that convention.
     */
    std::string run_read()
    {
        NamedTempFile out("out", ""); // placeholder
        // pass name of this temporary file to the script
        mParams.args.add(out.getName());
        run();
        // assuming the script wrote to that file, read it
        return readfile(out.getName(), STRINGIZE("from " << mDesc << " script"));
    }

    LLProcess::Params mParams;
    LLProcessPtr mPy;
    std::string mDesc;
    NamedTempFile mScript;
};

/// convenience function for PythonProcessLauncher::run()
template <typename CONTENT>
static void python(const std::string& desc, const CONTENT& script)
{
    PythonProcessLauncher py(desc, script);
    py.run();
}

/// convenience function for PythonProcessLauncher::run_read()
template <typename CONTENT>
static std::string python_out(const std::string& desc, const CONTENT& script)
{
    PythonProcessLauncher py(desc, script);
    return py.run_read();
}

/// Create a temporary directory and clean it up later.
class NamedTempDir: public boost::noncopyable
{
public:
    // Use python() function to create a temp directory: I've found
    // nothing in either Boost.Filesystem or APR quite like Python's
    // tempfile.mkdtemp().
    // Special extra bonus: on Mac, mkdtemp() reports a pathname
    // starting with /var/folders/something, whereas that's really a
    // symlink to /private/var/folders/something. Have to use
    // realpath() to compare properly.
    NamedTempDir():
        mPath(python_out("mkdtemp()",
                         "from __future__ import with_statement\n"
                         "import os.path, sys, tempfile\n"
                         "with open(sys.argv[1], 'w') as f:\n"
                         "    f.write(os.path.realpath(tempfile.mkdtemp()))\n"))
    {}

    ~NamedTempDir()
    {
        aprchk(apr_dir_remove(mPath.c_str(), gAPRPoolp));
    }

    std::string getName() const { return mPath; }

private:
    std::string mPath;
};

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llprocess_data
    {
        LLAPRPool pool;
    };
    typedef test_group<llprocess_data> llprocess_group;
    typedef llprocess_group::object object;
    llprocess_group llprocessgrp("llprocess");

    struct Item
    {
        Item(): tries(0) {}
        unsigned    tries;
        std::string which;
        std::string what;
    };

/*==========================================================================*|
#define tabent(symbol) { symbol, #symbol }
    static struct ReasonCode
    {
        int code;
        const char* name;
    } reasons[] =
    {
        tabent(APR_OC_REASON_DEATH),
        tabent(APR_OC_REASON_UNWRITABLE),
        tabent(APR_OC_REASON_RESTART),
        tabent(APR_OC_REASON_UNREGISTER),
        tabent(APR_OC_REASON_LOST),
        tabent(APR_OC_REASON_RUNNING)
    };
#undef tabent
|*==========================================================================*/

    struct WaitInfo
    {
        WaitInfo(apr_proc_t* child_):
            child(child_),
            rv(-1),                 // we haven't yet called apr_proc_wait()
            rc(0),
            why(apr_exit_why_e(0))
        {}
        apr_proc_t* child;          // which subprocess
        apr_status_t rv;            // return from apr_proc_wait()
        int rc;                     // child's exit code
        apr_exit_why_e why;         // APR_PROC_EXIT, APR_PROC_SIGNAL, APR_PROC_SIGNAL_CORE
    };

    void child_status_callback(int reason, void* data, int status)
    {
/*==========================================================================*|
        std::string reason_str;
        BOOST_FOREACH(const ReasonCode& rcp, reasons)
        {
            if (reason == rcp.code)
            {
                reason_str = rcp.name;
                break;
            }
        }
        if (reason_str.empty())
        {
            reason_str = STRINGIZE("unknown reason " << reason);
        }
        std::cout << "child_status_callback(" << reason_str << ")\n";
|*==========================================================================*/

        if (reason == APR_OC_REASON_DEATH || reason == APR_OC_REASON_LOST)
        {
            // Somewhat oddly, APR requires that you explicitly unregister
            // even when it already knows the child has terminated.
            apr_proc_other_child_unregister(data);

            WaitInfo* wi(static_cast<WaitInfo*>(data));
            // It's just wrong to call apr_proc_wait() here. The only way APR
            // knows to call us with APR_OC_REASON_DEATH is that it's already
            // reaped this child process, so calling wait() will only produce
            // "huh?" from the OS. We must rely on the status param passed in,
            // which unfortunately comes straight from the OS wait() call.
//          wi->rv = apr_proc_wait(wi->child, &wi->rc, &wi->why, APR_NOWAIT);
            wi->rv = APR_CHILD_DONE; // fake apr_proc_wait() results
#if defined(LL_WINDOWS)
            wi->why = APR_PROC_EXIT;
            wi->rc  = status;         // no encoding on Windows (no signals)
#else  // Posix
            if (WIFEXITED(status))
            {
                wi->why = APR_PROC_EXIT;
                wi->rc  = WEXITSTATUS(status);
            }
            else if (WIFSIGNALED(status))
            {
                wi->why = APR_PROC_SIGNAL;
                wi->rc  = WTERMSIG(status);
            }
            else                    // uh, shouldn't happen?
            {
                wi->why = APR_PROC_EXIT;
                wi->rc  = status;   // someone else will have to decode
            }
#endif // Posix
        }
    }

    template<> template<>
    void object::test<1>()
    {
        set_test_name("raw APR nonblocking I/O");

        // Create a script file in a temporary place.
        NamedTempFile script("py",
            "import sys" EOL
            "import time" EOL
            EOL
            "time.sleep(2)" EOL
            "print >>sys.stdout, 'stdout after wait'" EOL
            "sys.stdout.flush()" EOL
            "time.sleep(2)" EOL
            "print >>sys.stderr, 'stderr after wait'" EOL
            "sys.stderr.flush()" EOL
            );

        // Arrange to track the history of our interaction with child: what we
        // fetched, which pipe it came from, how many tries it took before we
        // got it.
        std::vector<Item> history;
        history.push_back(Item());

        // Run the child process.
        apr_procattr_t *procattr = NULL;
        aprchk(apr_procattr_create(&procattr, pool.getAPRPool()));
        aprchk(apr_procattr_io_set(procattr, APR_CHILD_BLOCK, APR_CHILD_BLOCK, APR_CHILD_BLOCK));
        aprchk(apr_procattr_cmdtype_set(procattr, APR_PROGRAM_PATH));

        std::vector<const char*> argv;
        apr_proc_t child;
        argv.push_back("python");
        // Have to have a named copy of this std::string so its c_str() value
        // will persist.
        std::string scriptname(script.getName());
        argv.push_back(scriptname.c_str());
        argv.push_back(NULL);

        aprchk(apr_proc_create(&child, argv[0],
                               &argv[0],
                               NULL, // if we wanted to pass explicit environment
                               procattr,
                               pool.getAPRPool()));

        // We do not want this child process to outlive our APR pool. On
        // destruction of the pool, forcibly kill the process. Tell APR to try
        // SIGTERM and wait 3 seconds. If that didn't work, use SIGKILL.
        apr_pool_note_subprocess(pool.getAPRPool(), &child, APR_KILL_AFTER_TIMEOUT);

        // arrange to call child_status_callback()
        WaitInfo wi(&child);
        apr_proc_other_child_register(&child, child_status_callback, &wi, child.in, pool.getAPRPool());

        // TODO:
        // Stuff child.in until it (would) block to verify EWOULDBLOCK/EAGAIN.
        // Have child script clear it later, then write one more line to prove
        // that it gets through.

        // Monitor two different output pipes. Because one will be closed
        // before the other, keep them in a list so we can drop whichever of
        // them is closed first.
        typedef std::pair<std::string, apr_file_t*> DescFile;
        typedef std::list<DescFile> DescFileList;
        DescFileList outfiles;
        outfiles.push_back(DescFile("out", child.out));
        outfiles.push_back(DescFile("err", child.err));

        while (! outfiles.empty())
        {
            // This peculiar for loop is designed to let us erase(dfli). With
            // a list, that invalidates only dfli itself -- but even so, we
            // lose the ability to increment it for the next item. So at the
            // top of every loop, while dfli is still valid, increment
            // dflnext. Then before the next iteration, set dfli to dflnext.
            for (DescFileList::iterator
                     dfli(outfiles.begin()), dflnext(outfiles.begin()), dflend(outfiles.end());
                 dfli != dflend; dfli = dflnext)
            {
                // Only valid to increment dflnext once we're sure it's not
                // already at dflend.
                ++dflnext;

                char buf[4096];

                apr_status_t rv = apr_file_gets(buf, sizeof(buf), dfli->second);
                if (APR_STATUS_IS_EOF(rv))
                {
//                  std::cout << "(EOF on " << dfli->first << ")\n";
//                  history.back().which = dfli->first;
//                  history.back().what  = "*eof*";
//                  history.push_back(Item());
                    outfiles.erase(dfli);
                    continue;
                }
                if (rv == EWOULDBLOCK || rv == EAGAIN)
                {
//                  std::cout << "(waiting; apr_file_gets(" << dfli->first << ") => " << rv << ": " << manager.strerror(rv) << ")\n";
                    ++history.back().tries;
                    continue;
                }
                aprchk_("apr_file_gets(buf, sizeof(buf), dfli->second)", rv);
                // Is it even possible to get APR_SUCCESS but read 0 bytes?
                // Hope not, but defend against that anyway.
                if (buf[0])
                {
//                  std::cout << dfli->first << ": " << buf;
                    history.back().which = dfli->first;
                    history.back().what.append(buf);
                    if (buf[strlen(buf) - 1] == '\n')
                        history.push_back(Item());
                    else
                    {
                        // Just for pretty output... if we only read a partial
                        // line, terminate it.
//                      std::cout << "...\n";
                    }
                }
            }
            // Do this once per tick, as we expect the viewer will
            apr_proc_other_child_refresh_all(APR_OC_REASON_RUNNING);
            sleep(1);
        }
        apr_file_close(child.in);
        apr_file_close(child.out);
        apr_file_close(child.err);

        // Okay, we've broken the loop because our pipes are all closed. If we
        // haven't yet called wait, give the callback one more chance. This
        // models the fact that unlike this small test program, the viewer
        // will still be running.
        if (wi.rv == -1)
        {
            std::cout << "last gasp apr_proc_other_child_refresh_all()\n";
            apr_proc_other_child_refresh_all(APR_OC_REASON_RUNNING);
        }

        if (wi.rv == -1)
        {
            std::cout << "child_status_callback(APR_OC_REASON_DEATH) wasn't called" << std::endl;
            wi.rv = apr_proc_wait(wi.child, &wi.rc, &wi.why, APR_NOWAIT);
        }
//      std::cout << "child done: rv = " << rv << " (" << manager.strerror(rv) << "), why = " << why << ", rc = " << rc << '\n';
        aprchk_("apr_proc_wait(wi->child, &wi->rc, &wi->why, APR_NOWAIT)", wi.rv, APR_CHILD_DONE);
        ensure_equals_(wi.why, APR_PROC_EXIT);
        ensure_equals_(wi.rc, 0);

        // Beyond merely executing all the above successfully, verify that we
        // obtained expected output -- and that we duly got control while
        // waiting, proving the non-blocking nature of these pipes.
        try
        {
            unsigned i = 0;
            ensure("blocking I/O on child pipe (0)", history[i].tries);
            ensure_equals_(history[i].which, "out");
            ensure_equals_(history[i].what,  "stdout after wait" EOL);
//          ++i;
//          ensure_equals_(history[i].which, "out");
//          ensure_equals_(history[i].what,  "*eof*");
            ++i;
            ensure("blocking I/O on child pipe (1)", history[i].tries);
            ensure_equals_(history[i].which, "err");
            ensure_equals_(history[i].what,  "stderr after wait" EOL);
//          ++i;
//          ensure_equals_(history[i].which, "err");
//          ensure_equals_(history[i].what,  "*eof*");
        }
        catch (const failure&)
        {
            std::cout << "History:\n";
            BOOST_FOREACH(const Item& item, history)
            {
                std::string what(item.what);
                if ((! what.empty()) && what[what.length() - 1] == '\n')
                {
                    what.erase(what.length() - 1);
                    if ((! what.empty()) && what[what.length() - 1] == '\r')
                    {
                        what.erase(what.length() - 1);
                        what.append("\\r");
                    }
                    what.append("\\n");
                }
                std::cout << "  " << item.which << ": '" << what << "' ("
                          << item.tries << " tries)\n";
            }
            std::cout << std::flush;
            // re-raise same error; just want to enrich the output
            throw;
        }
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("setWorkingDirectory()");
        // We want to test setWorkingDirectory(). But what directory is
        // guaranteed to exist on every machine, under every OS? Have to
        // create one. Naturally, ensure we clean it up when done.
        NamedTempDir tempdir;
        PythonProcessLauncher py("getcwd()",
                                 "from __future__ import with_statement\n"
                                 "import os, sys\n"
                                 "with open(sys.argv[1], 'w') as f:\n"
                                 "    f.write(os.getcwd())\n");
        // Before running, call setWorkingDirectory()
        py.mParams.cwd = tempdir.getName();
        ensure_equals("os.getcwd()", py.run_read(), tempdir.getName());
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("arguments");
        PythonProcessLauncher py("args",
                                 "from __future__ import with_statement\n"
                                 "import sys\n"
                                 // note nonstandard output-file arg!
                                 "with open(sys.argv[3], 'w') as f:\n"
                                 "    for arg in sys.argv[1:]:\n"
                                 "        print >>f, arg\n");
        // We expect that PythonProcessLauncher has already appended
        // its own NamedTempFile to mParams.args (sys.argv[0]).
        py.mParams.args.add("first arg");          // sys.argv[1]
        py.mParams.args.add("second arg");         // sys.argv[2]
        // run_read() appends() one more argument, hence [3]
        std::string output(py.run_read());
        boost::split_iterator<std::string::const_iterator>
            li(output, boost::first_finder("\n")), lend;
        ensure("didn't get first arg", li != lend);
        std::string arg(li->begin(), li->end());
        ensure_equals(arg, "first arg");
        ++li;
        ensure("didn't get second arg", li != lend);
        arg.assign(li->begin(), li->end());
        ensure_equals(arg, "second arg");
        ++li;
        ensure("didn't get output filename?!", li != lend);
        arg.assign(li->begin(), li->end());
        ensure("output filename empty?!", ! arg.empty());
        ++li;
        ensure("too many args", li == lend);
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("explicit kill()");
        PythonProcessLauncher py("kill()",
                                 "from __future__ import with_statement\n"
                                 "import sys, time\n"
                                 "with open(sys.argv[1], 'w') as f:\n"
                                 "    f.write('ok')\n"
                                 "# now sleep; expect caller to kill\n"
                                 "time.sleep(120)\n"
                                 "# if caller hasn't managed to kill by now, bad\n"
                                 "with open(sys.argv[1], 'w') as f:\n"
                                 "    f.write('bad')\n");
        NamedTempFile out("out", "not started");
        py.mParams.args.add(out.getName());
        py.mPy = LLProcess::create(py.mParams);
        ensure("couldn't launch kill() script", py.mPy);
        // Wait for the script to wake up and do its first write
        int i = 0, timeout = 60;
        for ( ; i < timeout; ++i)
        {
            sleep(1);
            if (readfile(out.getName(), "from kill() script") == "ok")
                break;
        }
        // If we broke this loop because of the counter, something's wrong
        ensure("script never started", i < timeout);
        // script has performed its first write and should now be sleeping.
        py.mPy->kill();
        // wait for the script to terminate... one way or another.
        while (py.mPy->isRunning())
        {
            sleep(1);
        }
        // If kill() failed, the script would have woken up on its own and
        // overwritten the file with 'bad'. But if kill() succeeded, it should
        // not have had that chance.
        ensure_equals("kill() script output", readfile(out.getName()), "ok");
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("implicit kill()");
        NamedTempFile out("out", "not started");
        LLProcess::id pid(0);
        {
            PythonProcessLauncher py("kill()",
                                     "from __future__ import with_statement\n"
                                     "import sys, time\n"
                                     "with open(sys.argv[1], 'w') as f:\n"
                                     "    f.write('ok')\n"
                                     "# now sleep; expect caller to kill\n"
                                     "time.sleep(120)\n"
                                     "# if caller hasn't managed to kill by now, bad\n"
                                     "with open(sys.argv[1], 'w') as f:\n"
                                     "    f.write('bad')\n");
            py.mParams.args.add(out.getName());
            py.mPy = LLProcess::create(py.mParams);
            ensure("couldn't launch kill() script", py.mPy);
            // Capture id for later
            pid = py.mPy->getProcessID();
            // Wait for the script to wake up and do its first write
            int i = 0, timeout = 60;
            for ( ; i < timeout; ++i)
            {
                sleep(1);
                if (readfile(out.getName(), "from kill() script") == "ok")
                    break;
            }
            // If we broke this loop because of the counter, something's wrong
            ensure("script never started", i < timeout);
            // Script has performed its first write and should now be sleeping.
            // Destroy the LLProcess, which should kill the child.
        }
        // wait for the script to terminate... one way or another.
        while (LLProcess::isRunning(pid))
        {
            sleep(1);
        }
        // If kill() failed, the script would have woken up on its own and
        // overwritten the file with 'bad'. But if kill() succeeded, it should
        // not have had that chance.
        ensure_equals("kill() script output", readfile(out.getName()), "ok");
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("autokill");
        NamedTempFile from("from", "not started");
        NamedTempFile to("to", "");
        LLProcess::id pid(0);
        {
            PythonProcessLauncher py("autokill",
                                     "from __future__ import with_statement\n"
                                     "import sys, time\n"
                                     "with open(sys.argv[1], 'w') as f:\n"
                                     "    f.write('ok')\n"
                                     "# wait for 'go' from test program\n"
                                     "for i in xrange(60):\n"
                                     "    time.sleep(1)\n"
                                     "    with open(sys.argv[2]) as f:\n"
                                     "        go = f.read()\n"
                                     "    if go == 'go':\n"
                                     "        break\n"
                                     "else:\n"
                                     "    with open(sys.argv[1], 'w') as f:\n"
                                     "        f.write('never saw go')\n"
                                     "    sys.exit(1)\n"
                                     "# okay, saw 'go', write 'ack'\n"
                                     "with open(sys.argv[1], 'w') as f:\n"
                                     "    f.write('ack')\n");
            py.mParams.args.add(from.getName());
            py.mParams.args.add(to.getName());
            py.mParams.autokill = false;
            py.mPy = LLProcess::create(py.mParams);
            ensure("couldn't launch kill() script", py.mPy);
            // Capture id for later
            pid = py.mPy->getProcessID();
            // Wait for the script to wake up and do its first write
            int i = 0, timeout = 60;
            for ( ; i < timeout; ++i)
            {
                sleep(1);
                if (readfile(from.getName(), "from autokill script") == "ok")
                    break;
            }
            // If we broke this loop because of the counter, something's wrong
            ensure("script never started", i < timeout);
            // Now destroy the LLProcess, which should NOT kill the child!
        }
        // If the destructor killed the child anyway, give it time to die
        sleep(2);
        // How do we know it's not terminated? By making it respond to
        // a specific stimulus in a specific way.
        {
            std::ofstream outf(to.getName().c_str());
            outf << "go";
        } // flush and close.
        // now wait for the script to terminate... one way or another.
        while (LLProcess::isRunning(pid))
        {
            sleep(1);
        }
        // If the LLProcess destructor implicitly called kill(), the
        // script could not have written 'ack' as we expect.
        ensure_equals("autokill script output", readfile(from.getName()), "ack");
    }
} // namespace tut
