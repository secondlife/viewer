/**
 * @file   debug.h
 * @author Nat Goodspeed
 * @date   2009-05-28
 * @brief  Debug output for unit test code
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#if ! defined(LL_DEBUG_H)
#define LL_DEBUG_H

#include "print.h"

/*****************************************************************************
*   Debugging stuff
*****************************************************************************/
/**
 * This class is intended to illuminate entry to a given block, exit from the
 * same block and checkpoints along the way. It also provides a convenient
 * place to turn std::cerr output on and off.
 *
 * If the environment variable LOGTEST is non-empty, each Debug instance will
 * announce its construction and destruction, presumably at entry and exit to
 * the block in which it's declared. Moreover, any arguments passed to its
 * operator()() will be streamed to std::cerr, prefixed by the block
 * description.
 *
 * The variable LOGTEST is used because that's the environment variable
 * checked by test.cpp, our TUT main() program, to turn on LLError logging. It
 * is expected that Debug is solely for use in test programs.
 */
class Debug
{
public:
    Debug(const std::string& block):
        mBlock(block),
        mLOGTEST(getenv("LOGTEST")),
        // debug output enabled when LOGTEST is set AND non-empty
        mEnabled(mLOGTEST && *mLOGTEST)
    {
        (*this)("entry");
    }

    // non-copyable
    Debug(const Debug&) = delete;

    ~Debug()
    {
        (*this)("exit");
    }

    template <typename... ARGS>
    void operator()(ARGS&&... args)
    {
        if (mEnabled)
        {
            print(mBlock, ' ', std::forward<ARGS>(args)...);
        }
    }

private:
    const std::string mBlock;
    const char* mLOGTEST;
    bool mEnabled;
};

// It's often convenient to use the name of the enclosing function as the name
// of the Debug block.
#define DEBUG Debug debug(LL_PRETTY_FUNCTION)

// These BEGIN/END macros are specifically for debugging output -- please
// don't assume you must use such for coroutines in general! They only help to
// make control flow (as well as exception exits) explicit.
#define BEGIN                                   \
{                                               \
    DEBUG;                                      \
    try

#define END                                     \
    catch (...)                                 \
    {                                           \
        debug("*** exceptional ");              \
        throw;                                  \
    }                                           \
}

#endif /* ! defined(LL_DEBUG_H) */
