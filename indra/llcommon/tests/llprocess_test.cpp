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
#include "../test/catch_and_store_what_in.h"
#include "stringize.h"
#include "llsdutil.h"
#include "llevents.h"
#include "wrapllerrs.h"

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

/// Looping on LLProcess::isRunning() must now be accompanied by pumping
/// "mainloop" -- otherwise the status won't update and you get an infinite
/// loop.
void yield(int seconds=1)
{
    // This function simulates waiting for another viewer frame
    sleep(seconds);
    LLEventPumps::instance().obtain("mainloop").post(LLSD());
}

void waitfor(LLProcess& proc, int timeout=60)
{
    int i = 0;
    for ( ; i < timeout && proc.isRunning(); ++i)
    {
        yield();
    }
    tut::ensure(STRINGIZE("process took longer than " << timeout << " seconds to terminate"),
                i < timeout);
}

void waitfor(LLProcess::handle h, const std::string& desc, int timeout=60)
{
    int i = 0;
    for ( ; i < timeout && LLProcess::isRunning(h, desc); ++i)
    {
        yield();
    }
    tut::ensure(STRINGIZE("process took longer than " << timeout << " seconds to terminate"),
                i < timeout);
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

        mParams.desc = desc + " script";
        mParams.executable = PYTHON;
        mParams.args.add(mScript.getName());
    }

    /// Launch Python script; verify that it launched
    void launch()
    {
        mPy = LLProcess::create(mParams);
        tut::ensure(STRINGIZE("Couldn't launch " << mDesc << " script"), mPy);
    }

    /// Run Python script and wait for it to complete.
    void run()
    {
        launch();
        // One of the irritating things about LLProcess is that
        // there's no API to wait for the child to terminate -- but given
        // its use in our graphics-intensive interactive viewer, it's
        // understandable.
        waitfor(*mPy);
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
                         "    f.write(os.path.normcase(os.path.normpath(os.path.realpath(tempfile.mkdtemp()))))\n"))
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
        PythonProcessLauncher py(get_test_name(),
                                 "from __future__ import with_statement\n"
                                 "import os, sys\n"
                                 "with open(sys.argv[1], 'w') as f:\n"
                                 "    f.write(os.path.normcase(os.path.normpath(os.getcwd())))\n");
        // Before running, call setWorkingDirectory()
        py.mParams.cwd = tempdir.getName();
        ensure_equals("os.getcwd()", py.run_read(), tempdir.getName());
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("arguments");
        PythonProcessLauncher py(get_test_name(),
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
        set_test_name("exit(0)");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.exit(0)\n");
        py.run();
        ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        ensure_equals("Status.mData",  py.mPy->getStatus().mData,  0);
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("exit(2)");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.exit(2)\n");
        py.run();
        ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        ensure_equals("Status.mData",  py.mPy->getStatus().mData,  2);
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("syntax_error:");
        PythonProcessLauncher py(get_test_name(),
                                 "syntax_error:\n");
        py.mParams.files.add(LLProcess::FileParam()); // inherit stdin
        py.mParams.files.add(LLProcess::FileParam()); // inherit stdout
        py.mParams.files.add(LLProcess::FileParam().type("pipe")); // pipe for stderr
        py.run();
        ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        ensure_equals("Status.mData",  py.mPy->getStatus().mData,  1);
        std::istream& rpipe(py.mPy->getReadPipe(LLProcess::STDERR).get_istream());
        std::vector<char> buffer(4096);
        rpipe.read(&buffer[0], buffer.size());
        std::streamsize got(rpipe.gcount());
        ensure("Nothing read from stderr pipe", got);
        std::string data(&buffer[0], got);
        ensure("Didn't find 'SyntaxError:'", data.find("\nSyntaxError:") != std::string::npos);
    }

    template<> template<>
    void object::test<7>()
    {
        set_test_name("explicit kill()");
        PythonProcessLauncher py(get_test_name(),
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
        py.launch();
        // Wait for the script to wake up and do its first write
        int i = 0, timeout = 60;
        for ( ; i < timeout; ++i)
        {
            yield();
            if (readfile(out.getName(), "from kill() script") == "ok")
                break;
        }
        // If we broke this loop because of the counter, something's wrong
        ensure("script never started", i < timeout);
        // script has performed its first write and should now be sleeping.
        py.mPy->kill();
        // wait for the script to terminate... one way or another.
        waitfor(*py.mPy);
#if LL_WINDOWS
        ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        ensure_equals("Status.mData",  py.mPy->getStatus().mData,  -1);
#else
        ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::KILLED);
        ensure_equals("Status.mData",  py.mPy->getStatus().mData,  SIGTERM);
#endif
        // If kill() failed, the script would have woken up on its own and
        // overwritten the file with 'bad'. But if kill() succeeded, it should
        // not have had that chance.
        ensure_equals(get_test_name() + " script output", readfile(out.getName()), "ok");
    }

    template<> template<>
    void object::test<8>()
    {
        set_test_name("implicit kill()");
        NamedTempFile out("out", "not started");
        LLProcess::handle phandle(0);
        {
            PythonProcessLauncher py(get_test_name(),
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
            py.launch();
            // Capture handle for later
            phandle = py.mPy->getProcessHandle();
            // Wait for the script to wake up and do its first write
            int i = 0, timeout = 60;
            for ( ; i < timeout; ++i)
            {
                yield();
                if (readfile(out.getName(), "from kill() script") == "ok")
                    break;
            }
            // If we broke this loop because of the counter, something's wrong
            ensure("script never started", i < timeout);
            // Script has performed its first write and should now be sleeping.
            // Destroy the LLProcess, which should kill the child.
        }
        // wait for the script to terminate... one way or another.
        waitfor(phandle, "kill() script");
        // If kill() failed, the script would have woken up on its own and
        // overwritten the file with 'bad'. But if kill() succeeded, it should
        // not have had that chance.
        ensure_equals(get_test_name() + " script output", readfile(out.getName()), "ok");
    }

    template<> template<>
    void object::test<9>()
    {
        set_test_name("autokill=false");
        NamedTempFile from("from", "not started");
        NamedTempFile to("to", "");
        LLProcess::handle phandle(0);
        {
            PythonProcessLauncher py(get_test_name(),
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
            py.launch();
            // Capture handle for later
            phandle = py.mPy->getProcessHandle();
            // Wait for the script to wake up and do its first write
            int i = 0, timeout = 60;
            for ( ; i < timeout; ++i)
            {
                yield();
                if (readfile(from.getName(), "from autokill script") == "ok")
                    break;
            }
            // If we broke this loop because of the counter, something's wrong
            ensure("script never started", i < timeout);
            // Now destroy the LLProcess, which should NOT kill the child!
        }
        // If the destructor killed the child anyway, give it time to die
        yield(2);
        // How do we know it's not terminated? By making it respond to
        // a specific stimulus in a specific way.
        {
            std::ofstream outf(to.getName().c_str());
            outf << "go";
        } // flush and close.
        // now wait for the script to terminate... one way or another.
        waitfor(phandle, "autokill script");
        // If the LLProcess destructor implicitly called kill(), the
        // script could not have written 'ack' as we expect.
        ensure_equals(get_test_name() + " script output", readfile(from.getName()), "ack");
    }

    template<> template<>
    void object::test<10>()
    {
        set_test_name("'bogus' test");
        CaptureLog recorder;
        PythonProcessLauncher py(get_test_name(),
                                 "print 'Hello world'\n");
        py.mParams.files.add(LLProcess::FileParam("bogus"));
        py.mPy = LLProcess::create(py.mParams);
        ensure("should have rejected 'bogus'", ! py.mPy);
        std::string message(recorder.messageWith("bogus"));
        ensure_contains("did not name 'stdin'", message, "stdin");
    }

    template<> template<>
    void object::test<11>()
    {
        set_test_name("'file' test");
        // Replace this test with one or more real 'file' tests when we
        // implement 'file' support
        PythonProcessLauncher py(get_test_name(),
                                 "print 'Hello world'\n");
        py.mParams.files.add(LLProcess::FileParam());
        py.mParams.files.add(LLProcess::FileParam("file"));
        py.mPy = LLProcess::create(py.mParams);
        ensure("should have rejected 'file'", ! py.mPy);
    }

    template<> template<>
    void object::test<12>()
    {
        set_test_name("'tpipe' test");
        // Replace this test with one or more real 'tpipe' tests when we
        // implement 'tpipe' support
        CaptureLog recorder;
        PythonProcessLauncher py(get_test_name(),
                                 "print 'Hello world'\n");
        py.mParams.files.add(LLProcess::FileParam());
        py.mParams.files.add(LLProcess::FileParam("tpipe"));
        py.mPy = LLProcess::create(py.mParams);
        ensure("should have rejected 'tpipe'", ! py.mPy);
        std::string message(recorder.messageWith("tpipe"));
        ensure_contains("did not name 'stdout'", message, "stdout");
    }

    template<> template<>
    void object::test<13>()
    {
        set_test_name("'npipe' test");
        // Replace this test with one or more real 'npipe' tests when we
        // implement 'npipe' support
        CaptureLog recorder;
        PythonProcessLauncher py(get_test_name(),
                                 "print 'Hello world'\n");
        py.mParams.files.add(LLProcess::FileParam());
        py.mParams.files.add(LLProcess::FileParam());
        py.mParams.files.add(LLProcess::FileParam("npipe"));
        py.mPy = LLProcess::create(py.mParams);
        ensure("should have rejected 'npipe'", ! py.mPy);
        std::string message(recorder.messageWith("npipe"));
        ensure_contains("did not name 'stderr'", message, "stderr");
    }

    template<> template<>
    void object::test<14>()
    {
        set_test_name("internal pipe name warning");
        CaptureLog recorder;
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.exit(7)\n");
        py.mParams.files.add(LLProcess::FileParam("pipe", "somename"));
        py.run();                   // verify that it did launch anyway
        ensure_equals("Status.mState", py.mPy->getStatus().mState, LLProcess::EXITED);
        ensure_equals("Status.mData",  py.mPy->getStatus().mData,  7);
        std::string message(recorder.messageWith("not yet supported"));
        ensure_contains("log message did not mention internal pipe name",
                        message, "somename");
    }

    /*-------------- support for "get*Pipe() validation" test --------------*/
#define TEST_getPipe(PROCESS, GETPIPE, GETOPTPIPE, VALID, NOPIPE, BADPIPE) \
    do                                                                  \
    {                                                                   \
        std::string threw;                                              \
        /* Both the following calls should work. */                     \
        (PROCESS).GETPIPE(VALID);                                       \
        ensure(#GETOPTPIPE "(" #VALID ") failed", (PROCESS).GETOPTPIPE(VALID)); \
        /* pass obviously bogus PIPESLOT */                             \
        CATCH_IN(threw, LLProcess::NoPipe, (PROCESS).GETPIPE(LLProcess::FILESLOT(4))); \
        ensure_contains("didn't reject bad slot", threw, "no slot");    \
        ensure_contains("didn't mention bad slot num", threw, "4");     \
        EXPECT_FAIL_WITH_LOG(threw, (PROCESS).GETOPTPIPE(LLProcess::FILESLOT(4))); \
        /* pass NOPIPE */                                               \
        CATCH_IN(threw, LLProcess::NoPipe, (PROCESS).GETPIPE(NOPIPE));  \
        ensure_contains("didn't reject non-pipe", threw, "not a monitored"); \
        EXPECT_FAIL_WITH_LOG(threw, (PROCESS).GETOPTPIPE(NOPIPE));      \
        /* pass BADPIPE: FILESLOT isn't empty but wrong direction */    \
        CATCH_IN(threw, LLProcess::NoPipe, (PROCESS).GETPIPE(BADPIPE)); \
        /* sneaky: GETPIPE is getReadPipe or getWritePipe */            \
        /* so skip "get" to obtain ReadPipe or WritePipe  :-P  */       \
        ensure_contains("didn't reject wrong pipe", threw, (#GETPIPE)+3); \
        EXPECT_FAIL_WITH_LOG(threw, (PROCESS).GETOPTPIPE(BADPIPE));     \
    } while (0)

/// For expecting exceptions. Execute CODE, catch EXCEPTION, store its what()
/// in std::string THREW, ensure it's not empty (i.e. EXCEPTION did happen).
#define CATCH_IN(THREW, EXCEPTION, CODE)                                \
    do                                                                  \
    {                                                                   \
        (THREW).clear();                                                \
        try                                                             \
        {                                                               \
            CODE;                                                       \
        }                                                               \
        CATCH_AND_STORE_WHAT_IN(THREW, EXCEPTION)                       \
        ensure("failed to throw " #EXCEPTION ": " #CODE, ! (THREW).empty()); \
    } while (0)

#define EXPECT_FAIL_WITH_LOG(EXPECT, CODE)                              \
    do                                                                  \
    {                                                                   \
        CaptureLog recorder;                                            \
        ensure(#CODE " succeeded", ! (CODE));                           \
        recorder.messageWith(EXPECT);                                   \
    } while (0)

    template<> template<>
    void object::test<15>()
    {
        set_test_name("get*Pipe() validation");
        PythonProcessLauncher py(get_test_name(),
                                 "print 'this output is expected'\n");
        py.mParams.files.add(LLProcess::FileParam("pipe")); // pipe for  stdin
        py.mParams.files.add(LLProcess::FileParam());       // inherit stdout
        py.mParams.files.add(LLProcess::FileParam("pipe")); // pipe for stderr
        py.run();
        TEST_getPipe(*py.mPy, getWritePipe, getOptWritePipe,
                     LLProcess::STDIN,   // VALID
                     LLProcess::STDOUT,  // NOPIPE
                     LLProcess::STDERR); // BADPIPE
        TEST_getPipe(*py.mPy, getReadPipe,  getOptReadPipe,
                     LLProcess::STDERR,  // VALID
                     LLProcess::STDOUT,  // NOPIPE
                     LLProcess::STDIN);  // BADPIPE
    }

    template<> template<>
    void object::test<16>()
    {
        set_test_name("talk to stdin/stdout");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys, time\n"
                                 "print 'ok'\n"
                                 "sys.stdout.flush()\n"
                                 "# wait for 'go' from test program\n"
                                 "go = sys.stdin.readline()\n"
                                 "if go != 'go\\n':\n"
                                 "    sys.exit('expected \"go\", saw %r' % go)\n"
                                 "print 'ack'\n");
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdin
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        py.launch();
        LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        int i, timeout = 60;
        for (i = 0; i < timeout && py.mPy->isRunning() && childout.size() < 3; ++i)
        {
            yield();
        }
        ensure("script never started", i < timeout);
        ensure_equals("bad wakeup from stdin/stdout script",
                      childout.getline(), "ok");
        // important to get the implicit flush from std::endl
        py.mPy->getWritePipe().get_ostream() << "go" << std::endl;
        for (i = 0; i < timeout && py.mPy->isRunning() && ! childout.contains("\n"); ++i)
        {
            yield();
        }
        ensure("script never replied", childout.contains("\n"));
        ensure_equals("child didn't ack", childout.getline(), "ack");
        ensure_equals("bad child termination", py.mPy->getStatus().mState, LLProcess::EXITED);
        ensure_equals("bad child exit code",   py.mPy->getStatus().mData,  0);
    }

    struct EventListener: public boost::noncopyable
    {
        EventListener(LLEventPump& pump)
        {
            mConnection = 
                pump.listen("EventListener", boost::bind(&EventListener::tick, this, _1));
        }

        bool tick(const LLSD& data)
        {
            mHistory.push_back(data);
            return false;
        }

        std::list<LLSD> mHistory;
        LLTempBoundListener mConnection;
    };

    static bool ack(std::ostream& out, const LLSD& data)
    {
        out << "continue" << std::endl;
        return false;
    }

    template<> template<>
    void object::test<17>()
    {
        set_test_name("listen for ReadPipe events");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.stdout.write('abc')\n"
                                 "sys.stdout.flush()\n"
                                 "sys.stdin.readline()\n"
                                 "sys.stdout.write('def')\n"
                                 "sys.stdout.flush()\n"
                                 "sys.stdin.readline()\n"
                                 "sys.stdout.write('ghi\\n')\n"
                                 "sys.stdout.flush()\n"
                                 "sys.stdin.readline()\n"
                                 "sys.stdout.write('second line\\n')\n");
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdin
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        py.launch();
        std::ostream& childin(py.mPy->getWritePipe(LLProcess::STDIN).get_ostream());
        LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        // lift the default limit; allow event to carry (some of) the actual data
        childout.setLimit(20);
        // listen for incoming data on childout
        EventListener listener(childout.getPump());
        // also listen with a function that prompts the child to continue
        // every time we see output
        LLTempBoundListener connection(
            childout.getPump().listen("ack", boost::bind(ack, boost::ref(childin), _1)));
        int i, timeout = 60;
        // wait through stuttering first line
        for (i = 0; i < timeout && py.mPy->isRunning() && ! childout.contains("\n"); ++i)
        {
            yield();
        }
        ensure("couldn't get first line", i < timeout);
        // disconnect from listener
        listener.mConnection.disconnect();
        // finish out the run
        waitfor(*py.mPy);
        // now verify history
        std::list<LLSD>::const_iterator li(listener.mHistory.begin()),
                                        lend(listener.mHistory.end());
        ensure("no events", li != lend);
        ensure_equals("history[0]", (*li)["data"].asString(), "abc");
        ensure_equals("history[0] len", (*li)["len"].asInteger(), 3);
        ++li;
        ensure("only 1 event", li != lend);
        ensure_equals("history[1]", (*li)["data"].asString(), "abcdef");
        ensure_equals("history[0] len", (*li)["len"].asInteger(), 6);
        ++li;
        ensure("only 2 events", li != lend);
        ensure_equals("history[2]", (*li)["data"].asString(), "abcdefghi" EOL);
        ensure_equals("history[0] len", (*li)["len"].asInteger(), 9 + sizeof(EOL) - 1);
        ++li;
        // We DO NOT expect a whole new event for the second line because we
        // disconnected.
        ensure("more than 3 events", li == lend);
    }

    template<> template<>
    void object::test<18>()
    {
        set_test_name("ReadPipe \"eof\" event");
        PythonProcessLauncher py(get_test_name(),
                                 "print 'Hello from Python!'\n");
        py.mParams.files.add(LLProcess::FileParam()); // stdin
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        py.launch();
        LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        EventListener listener(childout.getPump());
        waitfor(*py.mPy);
        // We can't be positive there will only be a single event, if the OS
        // (or any other intervening layer) does crazy buffering. What we want
        // to ensure is that there was exactly ONE event with "eof" true, and
        // that it was the LAST event.
        std::list<LLSD>::const_reverse_iterator rli(listener.mHistory.rbegin()),
                                                rlend(listener.mHistory.rend());
        ensure("no events", rli != rlend);
        ensure("last event not \"eof\"", (*rli)["eof"].asBoolean());
        while (++rli != rlend)
        {
            ensure("\"eof\" event not last", ! (*rli)["eof"].asBoolean());
        }
    }

    template<> template<>
    void object::test<19>()
    {
        set_test_name("setLimit()");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.stdout.write(sys.argv[1])\n");
        std::string abc("abcdefghijklmnopqrstuvwxyz");
        py.mParams.args.add(abc);
        py.mParams.files.add(LLProcess::FileParam()); // stdin
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        py.launch();
        LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        // listen for incoming data on childout
        EventListener listener(childout.getPump());
        // but set limit
        childout.setLimit(10);
        ensure_equals("getLimit() after setlimit(10)", childout.getLimit(), 10);
        // okay, pump I/O to pick up output from child
        waitfor(*py.mPy);
        ensure("no events", ! listener.mHistory.empty());
        // For all we know, that data could have arrived in several different
        // bursts... probably not, but anyway, only check the last one.
        ensure_equals("event[\"len\"]",
                      listener.mHistory.back()["len"].asInteger(), abc.length());
        ensure_equals("length of setLimit(10) data",
                      listener.mHistory.back()["data"].asString().length(), 10);
    }

    template<> template<>
    void object::test<20>()
    {
        set_test_name("peek() ReadPipe data");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.stdout.write(sys.argv[1])\n");
        std::string abc("abcdefghijklmnopqrstuvwxyz");
        py.mParams.args.add(abc);
        py.mParams.files.add(LLProcess::FileParam()); // stdin
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        py.launch();
        LLProcess::ReadPipe& childout(py.mPy->getReadPipe(LLProcess::STDOUT));
        // okay, pump I/O to pick up output from child
        waitfor(*py.mPy);
        // peek() with substr args
        ensure_equals("peek()", childout.peek(), abc);
        ensure_equals("peek(23)", childout.peek(23), abc.substr(23));
        ensure_equals("peek(5, 3)", childout.peek(5, 3), abc.substr(5, 3));
        ensure_equals("peek(27, 2)", childout.peek(27, 2), "");
        ensure_equals("peek(23, 5)", childout.peek(23, 5), "xyz");
        // contains() -- we don't exercise as thoroughly as find() because the
        // contains() implementation is trivially (and visibly) based on find()
        ensure("contains(\":\")", ! childout.contains(":"));
        ensure("contains(':')",   ! childout.contains(':'));
        ensure("contains(\"d\")", childout.contains("d"));
        ensure("contains('d')",   childout.contains('d'));
        ensure("contains(\"klm\")", childout.contains("klm"));
        ensure("contains(\"klx\")", ! childout.contains("klx"));
        // find()
        ensure("find(\":\")", childout.find(":") == LLProcess::ReadPipe::npos);
        ensure("find(':')",   childout.find(':') == LLProcess::ReadPipe::npos);
        ensure_equals("find(\"d\")", childout.find("d"), 3);
        ensure_equals("find('d')",   childout.find('d'), 3);
        ensure_equals("find(\"d\", 3)", childout.find("d", 3), 3);
        ensure_equals("find('d', 3)",   childout.find('d', 3), 3);
        ensure("find(\"d\", 4)", childout.find("d", 4) == LLProcess::ReadPipe::npos);
        ensure("find('d', 4)",   childout.find('d', 4) == LLProcess::ReadPipe::npos);
        // The case of offset == end and offset > end are different. In the
        // first case, we can form a valid (albeit empty) iterator range and
        // search that. In the second, guard logic in the implementation must
        // realize we can't form a valid iterator range.
        ensure("find(\"d\", 26)", childout.find("d", 26) == LLProcess::ReadPipe::npos);
        ensure("find('d', 26)",   childout.find('d', 26) == LLProcess::ReadPipe::npos);
        ensure("find(\"d\", 27)", childout.find("d", 27) == LLProcess::ReadPipe::npos);
        ensure("find('d', 27)",   childout.find('d', 27) == LLProcess::ReadPipe::npos);
        ensure_equals("find(\"ghi\")", childout.find("ghi"), 6);
        ensure_equals("find(\"ghi\", 6)", childout.find("ghi"), 6);
        ensure("find(\"ghi\", 7)", childout.find("ghi", 7) == LLProcess::ReadPipe::npos);
        ensure("find(\"ghi\", 26)", childout.find("ghi", 26) == LLProcess::ReadPipe::npos);
        ensure("find(\"ghi\", 27)", childout.find("ghi", 27) == LLProcess::ReadPipe::npos);
    }

    template<> template<>
    void object::test<21>()
    {
        set_test_name("bad postend");
        std::string pumpname("postend");
        EventListener listener(LLEventPumps::instance().obtain(pumpname));
        LLProcess::Params params;
        params.desc = get_test_name();
        params.postend = pumpname;
        LLProcessPtr child = LLProcess::create(params);
        ensure("shouldn't have launched", ! child);
        ensure_equals("number of postend events", listener.mHistory.size(), 1);
        LLSD postend(listener.mHistory.front());
        ensure("has id", ! postend.has("id"));
        ensure_equals("desc", postend["desc"].asString(), std::string(params.desc));
        ensure_equals("state", postend["state"].asInteger(), LLProcess::UNSTARTED);
        ensure("has data", ! postend.has("data"));
        std::string error(postend["string"]);
        // All we get from canned parameter validation is a bool, so the
        // "validation failed" message we ourselves generate can't mention
        // "executable" by name. Just check that it's nonempty.
        //ensure_contains("error", error, "executable");
        ensure("string", ! error.empty());
    }

    template<> template<>
    void object::test<22>()
    {
        set_test_name("good postend");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 "sys.exit(35)\n");
        std::string pumpname("postend");
        EventListener listener(LLEventPumps::instance().obtain(pumpname));
        py.mParams.postend = pumpname;
        py.launch();
        LLProcess::id childid(py.mPy->getProcessID());
        // Don't use waitfor(), which calls isRunning(); instead wait for an
        // event on pumpname.
        int i, timeout = 60;
        for (i = 0; i < timeout && listener.mHistory.empty(); ++i)
        {
            yield();
        }
        ensure("no postend event", i < timeout);
        ensure_equals("number of postend events", listener.mHistory.size(), 1);
        LLSD postend(listener.mHistory.front());
        ensure_equals("id",    postend["id"].asInteger(), childid);
        ensure("desc empty", ! postend["desc"].asString().empty());
        ensure_equals("state", postend["state"].asInteger(), LLProcess::EXITED);
        ensure_equals("data",  postend["data"].asInteger(),  35);
        std::string str(postend["string"]);
        ensure_contains("string", str, "exited");
        ensure_contains("string", str, "35");
    }

    struct PostendListener
    {
        PostendListener(LLProcess::ReadPipe& rpipe,
                        const std::string& pumpname,
                        const std::string& expect):
            mReadPipe(rpipe),
            mExpect(expect),
            mTriggered(false)
        {
            LLEventPumps::instance().obtain(pumpname)
                .listen("PostendListener", boost::bind(&PostendListener::postend, this, _1));
        }

        bool postend(const LLSD&)
        {
            mTriggered = true;
            ensure_equals("postend listener", mReadPipe.read(mReadPipe.size()), mExpect);
            return false;
        }

        LLProcess::ReadPipe& mReadPipe;
        std::string mExpect;
        bool mTriggered;
    };

    template<> template<>
    void object::test<23>()
    {
        set_test_name("all data visible at postend");
        PythonProcessLauncher py(get_test_name(),
                                 "import sys\n"
                                 // note, no '\n' in written data
                                 "sys.stdout.write('partial line')\n");
        std::string pumpname("postend");
        py.mParams.files.add(LLProcess::FileParam()); // stdin
        py.mParams.files.add(LLProcess::FileParam("pipe")); // stdout
        py.mParams.postend = pumpname;
        py.launch();
        PostendListener listener(py.mPy->getReadPipe(LLProcess::STDOUT),
                                 pumpname,
                                 "partial line");
        waitfor(*py.mPy);
        ensure("postend never triggered", listener.mTriggered);
    }
} // namespace tut
