/**
 * @file   llcond.cpp
 * @author Nat Goodspeed
 * @date   2019-07-17
 * @brief  Implementation for llcond.
 * 
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Copyright (c) 2019, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llcond.h"
// STL headers
// std headers
// external library headers
// other Linden headers

namespace // anonymous
{

// See comments in LLCond::convert(const LLDate&) below
std::time_t compute_lldate_epoch()
{
    LLDate lldate_epoch;
    std::tm tm;
    // It should be noted that calling LLDate::split() to write directly
    // into a std::tm struct depends on S32 being a typedef for int in
    // stdtypes.h: split() takes S32*, whereas tm fields are documented to
    // be int. If you get compile errors here, somebody changed the
    // definition of S32. You'll have to declare six S32 variables,
    // split() into them, then assign them into the relevant tm fields.
    if (! lldate_epoch.split(&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                             &tm.tm_hour, &tm.tm_min, &tm.tm_sec))
    {
        // Theoretically split() could return false. In that case, we
        // don't have valid data, so we can't compute offset, so skip the
        // rest of this.
        return 0;
    }

    tm.tm_isdst = 0;
    std::time_t lldate_epoch_time = std::mktime(&tm);
    if (lldate_epoch_time == -1)
    {
        // Theoretically mktime() could return -1, meaning that the contents
        // of the passed std::tm cannot be represented as a time_t. (Worrisome
        // if LLDate's epoch happened to be exactly 1 tick before
        // std::time_t's epoch...)
        // In the error case, assume offset 0.
        return 0;
    }

    // But if we got this far, lldate_epoch_time is the time_t we want.
    return lldate_epoch_time;
}

} // anonymous namespace

// convert LLDate to a chrono::time_point
std::chrono::system_clock::time_point LLCond::convert(const LLDate& lldate)
{
    // std::chrono::system_clock's epoch MAY be the Unix epoch, namely
    // midnight UTC on 1970-01-01, in fact it probably is. But until C++20,
    // system_clock does not guarantee that. Unfortunately time_t doesn't
    // specify its epoch either, other than to note that it "almost always" is
    // the Unix epoch (https://en.cppreference.com/w/cpp/chrono/c/time_t).
    // LLDate, being based on apr_time_t, does guarantee 1970-01-01T00:00 UTC.
    // http://apr.apache.org/docs/apr/1.5/group__apr__time.html#gadb4bde16055748190eae190c55aa02bb

    // The easy, efficient conversion would be
    // std::chrono::system_clock::from_time_t(std::time_t(LLDate::secondsSinceEpoch())).
    // But that assumes that both time_t and system_clock have the same epoch
    // as LLDate -- an assumption that will work until it unexpectedly doesn't.

    // It would be more formally correct to break out the year, month, day,
    // hour, minute, second (UTC) using LLDate::split() and recombine them
    // into std::time_t using std::mktime(). However, both split() and
    // mktime() have integer second granularity, whereas callers of
    // wait_until() are very likely to be interested in sub-second precision.
    // In that sense std::chrono::system_clock::from_time_t() is still
    // preferred.

    // So use the split() / mktime() mechanism to determine the numeric value
    // of the LLDate / apr_time_t epoch as expressed in time_t. (We assume
    // that the epoch offset can be expressed as integer seconds, per split()
    // and mktime(), which seems plausible.)

    // n.b. A function-static variable is initialized only once in a
    // thread-safe way.
    static std::time_t lldate_epoch_time = compute_lldate_epoch();

    // LLDate::secondsSinceEpoch() gets us, of course, how long it has
    // been since lldate_epoch_time. So adding lldate_epoch_time should
    // give us the correct time_t representation of a given LLDate even if
    // time_t's epoch differs from LLDate's.
    // We don't have to worry about the relative epochs of time_t and
    // system_clock because from_time_t() takes care of that!
    return std::chrono::system_clock::from_time_t(lldate_epoch_time +
                                                  lldate.secondsSinceEpoch());
}

// convert F32Milliseconds to a chrono::duration
std::chrono::milliseconds LLCond::convert(F32Milliseconds)
{
    // extract the F32 milliseconds from F32Milliseconds, construct
    // std::chrono::milliseconds from that value
    return std::chrono::milliseconds(timeout_duration.value());
}
