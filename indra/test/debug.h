/**
 * @file   debug.h
 * @author Nat Goodspeed
 * @date   2009-05-28
 * @brief  Debug output for unit test code
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_DEBUG_H)
#define LL_DEBUG_H

#include <iostream>

/*****************************************************************************
*   Debugging stuff
*****************************************************************************/
// This class is intended to illuminate entry to a given block, exit from the
// same block and checkpoints along the way. It also provides a convenient
// place to turn std::cout output on and off.
class Debug
{
public:
    Debug(const std::string& block):
        mBlock(block)
    {
        (*this)("entry");
    }

    ~Debug()
    {
        (*this)("exit");
    }

    void operator()(const std::string& status)
    {
#if defined(DEBUG_ON)
        std::cout << mBlock << ' ' << status << std::endl;
#endif
    }

private:
    const std::string mBlock;
};

// It's often convenient to use the name of the enclosing function as the name
// of the Debug block.
#define DEBUG Debug debug(__FUNCTION__)

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
