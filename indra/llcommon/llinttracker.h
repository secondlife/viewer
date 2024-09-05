/**
 * @file   llinttracker.h
 * @author Nat Goodspeed
 * @date   2024-08-30
 * @brief  LLIntTracker isa LLInstanceTracker with generated int keys.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLINTTRACKER_H)
#define LL_LLINTTRACKER_H

#include "llinstancetracker.h"

template <typename T>
class LLIntTracker: public LLInstanceTracker<T, int>
{
    using super = LLInstanceTracker<T, int>;
public:
    LLIntTracker():
        super(getUniqueKey())
    {}

private:
    static int getUniqueKey()
    {
        // Find a random key that does NOT already correspond to an instance.
        // Passing a duplicate key to LLInstanceTracker would do Bad Things.
        int key;
        do
        {
            key = std::rand();
        } while (super::getInstance(key));
        // This could be racy, if we were instantiating new LLIntTracker<T>s
        // on multiple threads. If we need that, have to lock between checking
        // getInstance() and constructing the new super.
        return key;
    }
};

#endif /* ! defined(LL_LLINTTRACKER_H) */
