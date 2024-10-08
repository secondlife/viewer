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
#include "stringize.h"
#include <exception>                // std::uncaught_exceptions()

/*****************************************************************************
*   Debugging stuff
*****************************************************************************/
/**
 * Return true if the environment variable LOGTEST is non-empty.
 *
 * The variable LOGTEST is used because that's the environment variable
 * checked by test.cpp, our TUT main() program, to turn on LLError logging. It
 * is expected that Debug is solely for use in test programs.
 */
inline
bool LOGTEST_enabled()
{
    auto LOGTEST{ getenv("LOGTEST") };
    // debug output enabled when LOGTEST is set AND non-empty
    return LOGTEST && *LOGTEST;
}

/**
 * This class is intended to illuminate entry to a given block, exit from the
 * same block and checkpoints along the way. It also provides a convenient
 * place to turn std::cerr output on and off.
 *
 * If enabled, each Debug instance will announce its construction and
 * destruction, presumably at entry and exit to the block in which it's
 * declared. Moreover, any arguments passed to its operator()() will be
 * streamed to std::cerr, prefixed by the block description.
 */
class Debug
{
public:
    template <typename... ARGS>
    Debug(ARGS&&... args):
        mBlock(stringize(std::forward<ARGS>(args)...)),
        mEnabled(LOGTEST_enabled())
    {
        (*this)("entry");
    }

    // non-copyable
    Debug(const Debug&) = delete;
    Debug& operator=(const Debug&) = delete;

    ~Debug()
    {
        auto exceptional{ std::uncaught_exceptions()? "exceptional " : "" };
        (*this)(exceptional, "exit");
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
    bool mEnabled;
};

// It's often convenient to use the name of the enclosing function as the name
// of the Debug block.
#define DEBUG Debug debug(LL_PRETTY_FUNCTION)

/// If enabled, debug_expr(expression) gives you output concerning an inline
/// expression such as a class member initializer.
#define debug_expr(expr) debug_expr_(#expr, [&](){ return expr; })

template <typename EXPR>
inline auto debug_expr_(const char* strexpr, EXPR&& lambda)
{
    if (! LOGTEST_enabled())
        return std::forward<EXPR>(lambda)();
    print("Before: ", strexpr);
    auto result{ std::forward<EXPR>(lambda)() };
    print(strexpr, " -> ", result);
    return result;
}

#endif /* ! defined(LL_DEBUG_H) */
