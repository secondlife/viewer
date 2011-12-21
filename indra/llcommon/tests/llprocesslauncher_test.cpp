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
#include "llprocesslauncher.h"
// STL headers
#include <vector>
// std headers
// external library headers
#include "llapr.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
// other Linden headers
#include "../test/lltut.h"

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

    template<> template<>
    void object::test<1>()
    {
        set_test_name("raw APR nonblocking I/O");

        apr_procattr_t *procattr = NULL;
        aprchk(apr_procattr_create(&procattr, apr.pool));
        aprchk(apr_procattr_io_set(procattr, APR_CHILD_BLOCK, APR_CHILD_BLOCK, APR_CHILD_BLOCK));
        aprchk(apr_procattr_cmdtype_set(procattr, APR_PROGRAM_PATH));

        std::vector<const char*> argv;
        apr_proc_t child;
        argv.push_back("python");
        argv.push_back("-c");
        argv.push_back("raise RuntimeError('Hello from Python!')");
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
                    std::cout << "(EOF on " << iterfiles[i].first << ")\n";
                    outfiles.erase(outfiles.begin() + i);
                    continue;
                }
                if (rv != APR_SUCCESS)
                {
                    std::cout << "(waiting; apr_file_gets(" << iterfiles[i].first << ") => " << rv << ": " << apr.strerror(rv) << ")\n";
                    continue;
                }
                // Is it even possible to get APR_SUCCESS but read 0 bytes?
                // Hope not, but defend against that anyway.
                if (buf[0])
                {
                    std::cout << iterfiles[i].first << ": " << buf;
                    // Just for pretty output... if we only read a partial
                    // line, terminate it.
                    if (buf[strlen(buf) - 1] != '\n')
                        std::cout << "...\n";
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
            std::cout << "child not done (" << rv << "): " << apr.strerror(rv) << '\n';
            sleep(0.5);
        }
        std::cout << "child done: rv = " << rv << " (" << apr.strerror(rv) << "), why = " << why << ", rc = " << rc << '\n';
    }
} // namespace tut
