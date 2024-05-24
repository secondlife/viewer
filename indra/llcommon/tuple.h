/**
 * @file   tuple.h
 * @author Nat Goodspeed
 * @date   2021-10-04
 * @brief  A couple tuple utilities
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Copyright (c) 2021, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_TUPLE_H)
#define LL_TUPLE_H

#include <tuple>
#include <type_traits>              // std::remove_reference
#include <utility>                  // std::pair

/**
 * tuple_cons() behaves like LISP cons: it uses std::tuple_cat() to prepend a
 * new item of arbitrary type to an existing std::tuple.
 */
template <typename First, typename... Rest, typename Tuple_=std::tuple<Rest...>>
auto tuple_cons(First&& first, Tuple_&& rest)
{
    // All we need to do is make a tuple containing 'first', and let
    // tuple_cat() do the hard part.
    return std::tuple_cat(std::tuple<First>(std::forward<First>(first)),
                          std::forward<Tuple_>(rest));
}

/**
 * tuple_car() behaves like LISP car: it extracts the first item from a
 * std::tuple.
 */
template <typename... Args, typename Tuple_=std::tuple<Args...>>
auto tuple_car(Tuple_&& tuple)
{
    return std::get<0>(std::forward<Tuple_>(tuple));
}

/**
 * tuple_cdr() behaves like LISP cdr: it returns a new tuple containing
 * everything BUT the first item.
 */
// derived from https://stackoverflow.com/a/24046437
template <typename Tuple, std::size_t... Indices>
auto tuple_cdr_(Tuple&& tuple, const std::index_sequence<Indices...>)
{
    // Given an index sequence from [0..N-1), extract tuple items [1..N)
    return std::make_tuple(std::get<Indices+1u>(std::forward<Tuple>(tuple))...);
}

template <typename Tuple>
auto tuple_cdr(Tuple&& tuple)
{
    return tuple_cdr_(
        std::forward<Tuple>(tuple),
        // Pass helper function an index sequence one item shorter than tuple
        std::make_index_sequence<
            std::tuple_size<
                // tuple_size doesn't like reference types
                typename std::remove_reference<Tuple>::type
            >::value - 1u>
        ());
}

/**
 * tuple_split(), the opposite of tuple_cons(), has no direct analog in LISP.
 * It returns a std::pair of tuple_car(), tuple_cdr(). We could call this
 * function tuple_car_cdr(), or tuple_slice() or some such. But tuple_split()
 * feels more descriptive.
 */
template <typename... Args, typename Tuple_=std::tuple<Args...>>
auto tuple_split(Tuple_&& tuple)
{
    // We're not really worried about forwarding multiple times a tuple that
    // might contain move-only items, because the implementation above only
    // applies std::get() exactly once to each item.
    return std::make_pair(tuple_car(std::forward<Tuple_>(tuple)),
                          tuple_cdr(std::forward<Tuple_>(tuple)));
}

#endif /* ! defined(LL_TUPLE_H) */
