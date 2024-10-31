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
        // Right away expand the result array to the size we'll need.
        // (end - start) is that size; (end - start - 1) is the index of the
        // last entry in result.
        result[end - start - 1] = LLSD();
        for (int i = 0; (start + i) < end; ++i)
        {
            // For this to be a slice, set result[0] = getSingle(start), etc.
            result[i] = getSingle(start + i);
        }
    }
    return { result, start };
}

LLSD ResultSet::getSlice(int index, int count) const
{
    return getSliceStart(index, count).first;
}

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
