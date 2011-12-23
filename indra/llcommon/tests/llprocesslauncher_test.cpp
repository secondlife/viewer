/**
 * @file   llprocesslauncher_test.cpp
 * @author Nat Goodspeed
 * @date   2011-12-19
 * @brief  Test for llprocesslauncher.
 * 
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Copyright (c) 2011, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#define WIN32_LEAN_AND_MEAN
#include "llprocesslauncher.h"
// STL headers
#include <vector>
#include <list>
// std headers
#include <errno.h>
// external library headers
#include "llapr.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include <boost/foreach.hpp>
// other Linden headers
#include "../test/lltut.h"
#include "stringize.h"

#if defined(LL_WINDOWS)
#define sleep(secs) _sleep((secs) * 1000)
#define EOL "\r\n"
#else
#define EOL "\n"
#include <sys/wait.h>
#endif

class APR
{
public:
    APR():
        pool(NULL)
    {
        apr_initialize();
        apr_pool_create(&pool, NULL);
    }

    ~APR()
    {
        apr_terminate();
    }

    std::string strerror(apr_status_t rv)
    {
        char errbuf[256];
        apr_strerror(rv, errbuf, sizeof(errbuf));
        return errbuf;
    }

    apr_pool_t *pool;
};

#define ensure_equals_(left, right) \
        ensure_equals(STRINGIZE(#left << " != " << #right), (left), (right))
#define aprchk(expr) aprchk_(#expr, (expr))

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llprocesslauncher_data
    {
        void aprchk_(const char* call, apr_status_t rv, apr_status_t expected=APR_SUCCESS)
        {
            ensure_equals(STRINGIZE(call << " => " << rv << ": " << apr.strerror(rv)),
                          rv, expected);
        }

        APR apr;
    };
    typedef test_group<llprocesslauncher_data> llprocesslauncher_group;
    typedef llprocesslauncher_group::object object;
    llprocesslauncher_group llprocesslaunchergrp("llprocesslauncher");

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
        const char* tempdir = NULL;
        aprchk(apr_temp_dir_get(&tempdir, apr.pool));

        // Construct a temp filename template in that directory.
        char *tempname = NULL;
        aprchk(apr_filepath_merge(&tempname, tempdir, "testXXXXXX", 0, apr.pool));

        // Create a temp file from that template.
        apr_file_t* fp = NULL;
        aprchk(apr_file_mktemp(&fp, tempname, APR_CREATE | APR_WRITE | APR_EXCL, apr.pool));

        // Write it.
        const char script[] =
            "import sys" EOL
            "import time" EOL
            EOL
            "time.sleep(2)" EOL
            "print >>sys.stdout, 'stdout after wait'" EOL
            "sys.stdout.flush()" EOL
            "time.sleep(2)" EOL
            "print >>sys.stderr, 'stderr after wait'" EOL
            "sys.stderr.flush()" EOL
            ;
        apr_size_t len(sizeof(script)-1);
        aprchk(apr_file_write(fp, script, &len));
        aprchk(apr_file_close(fp));

        // Arrange to track the history of our interaction with child: what we
        // fetched, which pipe it came from, how many tries it took before we
        // got it.
        std::vector<Item> history;
        history.push_back(Item());

        // Run the child process.
        apr_procattr_t *procattr = NULL;
        aprchk(apr_procattr_create(&procattr, apr.pool));
        aprchk(apr_procattr_io_set(procattr, APR_CHILD_BLOCK, APR_CHILD_BLOCK, APR_CHILD_BLOCK));
        aprchk(apr_procattr_cmdtype_set(procattr, APR_PROGRAM_PATH));

        std::vector<const char*> argv;
        apr_proc_t child;
        argv.push_back("python");
        argv.push_back(tempname);
        argv.push_back(NULL);

        aprchk(apr_proc_create(&child, argv[0],
                               &argv[0],
                               NULL, // if we wanted to pass explicit environment
                               procattr,
                               apr.pool));

        // We do not want this child process to outlive our APR pool. On
        // destruction of the pool, forcibly kill the process. Tell APR to try
        // SIGTERM and wait 3 seconds. If that didn't work, use SIGKILL.
        apr_pool_note_subprocess(apr.pool, &child, APR_KILL_AFTER_TIMEOUT);

        // arrange to call child_status_callback()
        WaitInfo wi(&child);
        apr_proc_other_child_register(&child, child_status_callback, &wi, child.in, apr.pool);

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
//                  std::cout << "(waiting; apr_file_gets(" << dfli->first << ") => " << rv << ": " << apr.strerror(rv) << ")\n";
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
//      std::cout << "child done: rv = " << rv << " (" << apr.strerror(rv) << "), why = " << why << ", rc = " << rc << '\n';
        aprchk_("apr_proc_wait(wi->child, &wi->rc, &wi->why, APR_NOWAIT)", wi.rv, APR_CHILD_DONE);
        ensure_equals_(wi.why, APR_PROC_EXIT);
        ensure_equals_(wi.rc, 0);

        // Remove temp script file
        aprchk(apr_file_remove(tempname, apr.pool));

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
} // namespace tut
