/**
 * @file   setenv.h
 * @author Nat Goodspeed
 * @date   2020-04-01
 * @brief  Provide a way for a particular test program to alter the
 *         environment before entry to main().
 * 
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Copyright (c) 2020, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_SETENV_H)
#define LL_SETENV_H

#include <stdlib.h>                 // setenv()

/**
 * Our test.cpp main program responds to environment variables LOGTEST and
 * LOGFAIL. But if you set (e.g.) LOGTEST=DEBUG before a viewer build, @em
 * every test program in the build emits debug log output. This can be so
 * voluminous as to slow down the build.
 *
 * With an integration test program, you can specifically build (e.g.) the
 * INTEGRATION_TEST_llstring target, and set any environment variables you
 * want for that. But with a unit test program, since executing the program is
 * a side effect rather than an explicit target, specifically building (e.g.)
 * PROJECT_lllogin_TEST_lllogin only builds the executable without running it.
 *
 * To set an environment variable for a particular test program, declare a
 * static instance of SetEnv in its .cpp file. SetEnv's constructor takes
 * pairs of strings, e.g.
 *
 * @code
 * static SetEnv sLOGGING("LOGTEST", "INFO");
 * @endcode
 *
 * Declaring a static instance of SetEnv is important because that ensures
 * that the environment variables are set before main() is entered, since it
 * is main() that examines LOGTEST and LOGFAIL.
 */
struct SetEnv
{
    // degenerate constructor, terminate recursion
    SetEnv() {}

    /**
     * SetEnv() accepts an arbitrary number of pairs of strings: variable
     * name, value, variable name, value ... Entering the constructor sets
     * those variables in the process environment using Posix setenv(),
     * overriding any previous value. If static SetEnv declarations in
     * different translation units specify overlapping sets of variable names,
     * it is indeterminate which instance will "win."
     */
    template <typename VAR, typename VAL, typename... ARGS>
    SetEnv(VAR&& var, VAL&& val, ARGS&&... rest):
        // constructor forwarding handles the tail of the list
        SetEnv(std::forward<ARGS>(rest)...)
    {
        // set just the first (variable, value) pair
        // 1 means override previous value if any
        setenv(std::forward<VAR>(var), std::forward<VAL>(val), 1);
    }
};

#endif /* ! defined(LL_SETENV_H) */
