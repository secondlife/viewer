/**
 * @file   apply.h
 * @author Nat Goodspeed
 * @date   2022-06-18
 * @brief  C++14 version of std::apply()
 * 
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_APPLY_H)
#define LL_APPLY_H

#include <tuple>

namespace LL
{

#if __cplusplus >= 201703L

// C++17 implementation
using std::apply;

#else // C++14

// Derived from https://stackoverflow.com/a/20441189
// and https://en.cppreference.com/w/cpp/utility/apply
template <typename CALLABLE, typename TUPLE, std::size_t... I>
auto apply_impl(CALLABLE&& func, TUPLE&& args, std::index_sequence<I...>)
{
    // call func(unpacked args)
    return std::forward<CALLABLE>(func)(std::move(std::get<I>(args))...);
}

template <typename CALLABLE, typename... ARGS>
auto apply(CALLABLE&& func, std::tuple<ARGS...>&& args)
{
    // std::index_sequence_for is the magic sauce here, generating an argument
    // pack of indexes for each entry in args. apply_impl() can then pass
    // those to std::get() to unpack args into individual arguments.
    return apply_impl(std::forward<CALLABLE>(func),
                      std::forward<std::tuple<ARGS...>>(args),
                      std::index_sequence_for<ARGS...>{});
}

#endif // C++14

} // namespace LL

#endif /* ! defined(LL_APPLY_H) */
