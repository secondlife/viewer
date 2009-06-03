/**
 * @file   llcoros.cpp
 * @author Nat Goodspeed
 * @date   2009-06-03
 * @brief  Implementation for llcoros.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llcoros.h"
// STL headers
// std headers
// external library headers
#include <boost/bind.hpp>
// other Linden headers
#include "llevents.h"
#include "llerror.h"
#include "stringize.h"

LLCoros::LLCoros()
{
    // Register our cleanup() method for "mainloop" ticks
    LLEventPumps::instance().obtain("mainloop").listen(
        "LLCoros", boost::bind(&LLCoros::cleanup, this, _1));
}

bool LLCoros::cleanup(const LLSD&)
{
    // Walk the mCoros map, checking and removing completed coroutines.
    for (CoroMap::iterator mi(mCoros.begin()), mend(mCoros.end()); mi != mend; )
    {
        // Has this coroutine exited (normal return, exception, exit() call)
        // since last tick?
        if (mi->second->exited())
        {
            LL_INFOS("LLCoros") << "LLCoros: cleaning up coroutine " << mi->first << LL_ENDL;
            // The erase() call will invalidate its passed iterator value --
            // so increment mi FIRST -- but pass its original value to
            // erase(). This is what postincrement is all about.
            mCoros.erase(mi++);
        }
        else
        {
            // Still live, just skip this entry as if incrementing at the top
            // of the loop as usual.
            ++mi;
        }
    }
    return false;
}

std::string LLCoros::generateDistinctName(const std::string& prefix) const
{
    // Allowing empty name would make getName()'s not-found return ambiguous.
    if (prefix.empty())
    {
        LL_ERRS("LLCoros") << "LLCoros::launch(): pass non-empty name string" << LL_ENDL;
    }

    // If the specified name isn't already in the map, just use that.
    std::string name(prefix);

    // Find the lowest numeric suffix that doesn't collide with an existing
    // entry. Start with 2 just to make it more intuitive for any interested
    // parties: e.g. "joe", "joe2", "joe3"...
    for (int i = 2; ; name = STRINGIZE(prefix << i++))
    {
        if (mCoros.find(name) == mCoros.end())
        {
            LL_INFOS("LLCoros") << "LLCoros: launching coroutine " << name << LL_ENDL;
            return name;
        }
    }
}

bool LLCoros::kill(const std::string& name)
{
    CoroMap::iterator found = mCoros.find(name);
    if (found == mCoros.end())
    {
        return false;
    }
    // Because this is a boost::ptr_map, erasing the map entry also destroys
    // the referenced heap object, in this case an LLCoro. That will destroy
    // the contained boost::coroutine object, which will terminate the coroutine.
    mCoros.erase(found);
    return true;
}

std::string LLCoros::getNameByID(const void* self_id) const
{
    // Walk the existing coroutines, looking for one from which the 'self_id'
    // passed to us comes.
    for (CoroMap::const_iterator mi(mCoros.begin()), mend(mCoros.end()); mi != mend; ++mi)
    {
        if (mi->second->owns_self_id(self_id))
        {
            return mi->first;
        }
    }
    return "";
}
