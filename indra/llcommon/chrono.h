/**
 * @file   chrono.h
 * @author Nat Goodspeed
 * @date   2021-10-05
 * @brief  supplement <chrono> with utility functions
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Copyright (c) 2021, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_CHRONO_H)
#define LL_CHRONO_H

#include <chrono>
#include <type_traits>              // std::enable_if

namespace LL
{

// time_point_cast() is derived from https://stackoverflow.com/a/35293183
// without the iteration: we think errors in the ~1 microsecond range are
// probably acceptable.

// This variant is for the optimal case when the source and dest use the same
// clock: that case is handled by std::chrono.
template <typename DestTimePoint, typename SrcTimePoint,
          typename std::enable_if<std::is_same<typename DestTimePoint::clock,
                                               typename SrcTimePoint::clock>::value,
                                  bool>::type = true>
DestTimePoint time_point_cast(const SrcTimePoint& time)
{
    return std::chrono::time_point_cast<typename DestTimePoint::duration>(time);
}

// This variant is for when the source and dest use different clocks -- see
// the linked StackOverflow answer, also Howard Hinnant's, for more context.
template <typename DestTimePoint, typename SrcTimePoint,
          typename std::enable_if<! std::is_same<typename DestTimePoint::clock,
                                                 typename SrcTimePoint::clock>::value,
                                  bool>::type = true>
DestTimePoint time_point_cast(const SrcTimePoint& time)
{
    // The basic idea is that we must adjust the passed time_point by the
    // difference between the clocks' epochs. But since time_point doesn't
    // expose its epoch, we fall back on what each of them thinks is now().
    // However, since we necessarily make sequential calls to those now()
    // functions, the answers differ not only by the cycles spent executing
    // those calls, but by potential OS interruptions between them. Try to
    // reduce that error by capturing the source clock time both before and
    // after the dest clock, and splitting the difference. Of course an
    // interruption between two of these now() calls without a comparable
    // interruption between the other two will skew the result, but better is
    // more expensive.
    const auto src_before = typename SrcTimePoint::clock::now();
    const auto dest_now   = typename DestTimePoint::clock::now();
    const auto src_after  = typename SrcTimePoint::clock::now();
    const auto src_diff   = src_after - src_before;
    const auto src_now    = src_before + src_diff / 2;
    return dest_now + (time - src_now);
}

} // namespace LL

#endif /* ! defined(LL_CHRONO_H) */
