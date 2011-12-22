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
// std headers
#include <errno.h>
// external library headers
#include "llapr.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
// other Linden headers
#include "../test/lltut.h"

#if defined(LL_WINDOWS)
#define sleep _sleep
#define EOL "\r\n"
#else
#define EOL "\n"
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

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llprocesslauncher_data
    {
        void aprchk(apr_status_t rv)
        {
            ensure_equals(apr.strerror(rv), rv, APR_SUCCESS);
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
            "print >>sys.stdout, \"stdout after wait\"" EOL
            "sys.stdout.flush()" EOL
            "time.sleep(2)" EOL
            "print >>sys.stderr, \"stderr after wait\"" EOL
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
                               static_cast<const char* const*>(&argv[0]),
                               NULL, // if we wanted to pass explicit environment
                               procattr,
                               apr.pool));

        typedef std::pair<std::string, apr_file_t*> DescFile;
        typedef std::vector<DescFile> DescFileVec;
        DescFileVec outfiles;
        outfiles.push_back(DescFile("out", child.out));
        outfiles.push_back(DescFile("err", child.err));

        while (! outfiles.empty())
        {
            DescFileVec iterfiles(outfiles);
            for (size_t i = 0; i < iterfiles.size(); ++i)
            {
                char buf[4096];

                apr_status_t rv = apr_file_gets(buf, sizeof(buf), iterfiles[i].second);
                if (APR_STATUS_IS_EOF(rv))
                {
//                  std::cout << "(EOF on " << iterfiles[i].first << ")\n";
                    history.back().which = iterfiles[i].first;
                    history.back().what  = "*eof*";
                    history.push_back(Item());
                    outfiles.erase(outfiles.begin() + i);
                    continue;
                }
                if (rv == EWOULDBLOCK || rv == EAGAIN)
                {
//                  std::cout << "(waiting; apr_file_gets(" << iterfiles[i].first << ") => " << rv << ": " << apr.strerror(rv) << ")\n";
                    ++history.back().tries;
                    continue;
                }
                ensure_equals(rv, APR_SUCCESS);
                // Is it even possible to get APR_SUCCESS but read 0 bytes?
                // Hope not, but defend against that anyway.
                if (buf[0])
                {
//                  std::cout << iterfiles[i].first << ": " << buf;
                    history.back().which = iterfiles[i].first;
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
            sleep(1);
        }
        apr_file_close(child.in);
        apr_file_close(child.out);
        apr_file_close(child.err);

        int rc = 0;
        apr_exit_why_e why;
        apr_status_t rv;
        while (! APR_STATUS_IS_CHILD_DONE(rv = apr_proc_wait(&child, &rc, &why, APR_NOWAIT)))
        {
//          std::cout << "child not done (" << rv << "): " << apr.strerror(rv) << '\n';
            sleep(0.5);
        }
//      std::cout << "child done: rv = " << rv << " (" << apr.strerror(rv) << "), why = " << why << ", rc = " << rc << '\n';
        ensure_equals(rv, APR_CHILD_DONE);
        ensure_equals(why, APR_PROC_EXIT);
        ensure_equals(rc, 0);

        // Remove temp script file
        aprchk(apr_file_remove(tempname, apr.pool));

        // Beyond merely executing all the above successfully, verify that we
        // obtained expected output -- and that we duly got control while
        // waiting, proving the non-blocking nature of these pipes.
        ensure("blocking I/O on child pipe (0)", history[0].tries);
        ensure_equals(history[0].which, "out");
        ensure_equals(history[0].what,  "stdout after wait" EOL);
        ensure("blocking I/O on child pipe (1)", history[1].tries);
        ensure_equals(history[1].which, "out");
        ensure_equals(history[1].what,  "*eof*");
        ensure_equals(history[2].which, "err");
        ensure_equals(history[2].what,  "stderr after wait" EOL);
        ensure_equals(history[3].which, "err");
        ensure_equals(history[3].what,  "*eof*");
    }
} // namespace tut
