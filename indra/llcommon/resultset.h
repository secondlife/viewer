/**
 * @file   resultset.h
 * @author Nat Goodspeed
 * @date   2024-09-03
 * @brief  ResultSet is an abstract base class to allow scripted access to
 *         potentially large collections representable as LLSD arrays.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_RESULTSET_H)
#define LL_RESULTSET_H

#include "llinttracker.h"
#include "llsd.h"
#include <iosfwd>                   // std::ostream
#include <utility>                  // std::pair
#include <vector>

namespace LL
{

// This abstract base class defines an interface by which a large collection
// of items representable as an LLSD array can be retrieved in slices. It isa
// LLIntTracker so we can pass its unique int key to a consuming script via
// LLSD.
struct ResultSet: public LLIntTracker<ResultSet>
{
    // Get the length of the result set. Indexes are 0-relative.
    virtual int getLength() const = 0;
    // Get conventional LLSD { key, length } pair.
    LLSD getKeyLength() const;
    // Retrieve LLSD corresponding to a single entry from the result set,
    // once we're sure the index is valid.
    virtual LLSD getSingle(int index) const = 0;
    // Retrieve LLSD corresponding to a "slice" of the result set: a
    // contiguous sub-array starting at index. The returned LLSD array might
    // be shorter than count entries if count > MAX_ITEM_LIMIT, or if the
    // specified slice contains the end of the result set.
    LLSD getSlice(int index, int count) const;
    // Like getSlice(), but also return adjusted start position.
    std::pair<LLSD, int> getSliceStart(int index, int count) const;

    /*---------------- the rest is solely for debug logging ----------------*/
    std::string mName;

    ResultSet(const std::string& name);
    virtual ~ResultSet();
};

// VectorResultSet is for the simple case of a ResultSet managing a single
// std::vector<T>.
template <typename T>
struct VectorResultSet: public ResultSet
{
    using super = VectorResultSet<T>;

    VectorResultSet(const std::string& name): ResultSet(name) {}
    int getLength() const override { return narrow(mVector.size()); }
    LLSD getSingle(int index) const override { return getSingleFrom(mVector[index]); }
    virtual LLSD getSingleFrom(const T&) const = 0;

    std::vector<T> mVector;
};

} // namespace LL

std::ostream& operator<<(std::ostream& out, const LL::ResultSet& self);

#endif /* ! defined(LL_RESULTSET_H) */
