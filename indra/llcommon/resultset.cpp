/**
 * @file   resultset.cpp
 * @author Nat Goodspeed
 * @date   2024-09-03
 * @brief  Implementation for resultset.
 * 
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "resultset.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llerror.h"
#include "llsdutil.h"

namespace LL
{

LLSD ResultSet::getKeyLength() const
{
    return llsd::array(getKey(), getLength());
}

std::pair<LLSD, int> ResultSet::getSliceStart(int index, int count) const
{
    // only call getLength() once
    auto length = getLength();
    // Adjust bounds [start, end) to overlap the actual result set from
    // [0, getLength()). Permit negative index; e.g. with a result set
    // containing 5 entries, getSlice(-2, 5) will adjust start to 0 and
    // end to 3.
    int start = llclamp(index, 0, length);
    int end = llclamp(index + count, 0, length);
    LLSD result{ LLSD::emptyArray() };
    // beware of count <= 0, or an [index, count) range that doesn't even
    // overlap [0, length) at all
    if (end > start)
    {
        // right away expand the result array to the size we'll need
        result[end - 1] = LLSD();
        for (int i = start; i < end; ++i)
        {
            result[i] = getSingle(i);
        }
    }
    return { result, start };
}

LLSD ResultSet::getSlice(int index, int count) const
{
    return getSliceStart(index, count).first;
}

/*==========================================================================*|
LLSD ResultSet::getSingle(int index) const
{
    if (0 <= index && index < getLength())
    {
        return getSingle_(index);
    }
    else
    {
        return {};
    }
}
|*==========================================================================*/

ResultSet::ResultSet(const std::string& name):
    mName(name)
{
    LL_DEBUGS("Lua") << *this << LL_ENDL;
}

ResultSet::~ResultSet()
{
    // We want to be able to observe that the consuming script eventually
    // destroys each of these ResultSets.
    LL_DEBUGS("Lua") << "~" << *this << LL_ENDL;
}

} // namespace LL

std::ostream& operator<<(std::ostream& out, const LL::ResultSet& self)
{
    return out << "ResultSet(" << self.mName << ", " << self.getKey() << ")";
}
