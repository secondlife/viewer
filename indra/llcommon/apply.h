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

#include <boost/type_traits/function_traits.hpp>
#include <cassert>
#include <tuple>

namespace LL
{

/**
 * USAGE NOTE:
 * https://stackoverflow.com/a/40523474/5533635
 *
 * If you're trying to pass apply() a variadic function, the compiler
 * complains that it can't deduce the callable type, presumably because it
 * doesn't know which arity to reify to pass.
 *
 * But it works to wrap the variadic function in a generic lambda, e.g.:
 *
 * @CODE
 * LL::apply(
 *     [](auto&&... args)
 *     {
 *         return variadic(std::forward<decltype(args)>(args)...);
 *     },
 *     args);
 * @ENDCODE
 *
 * Presumably this is because there's only one instance of the generic lambda
 * @em type, with a variadic <tt>operator()()</tt>.
 *
 * It's pointless to provide a wrapper @em function that implicitly supplies
 * the generic lambda. You couldn't pass your variadic function to our wrapper
 * function, for the same original reason!
 *
 * Instead we provide a wrapper @em macro. Sorry, Dr. Stroustrup.
 */
#define VAPPLY(FUNC, ARGS)                                          \
    LL::apply(                                                      \
        [](auto&&... args)                                          \
        {                                                           \
            return (FUNC)(std::forward<decltype(args)>(args)...);   \
        },                                                          \
        (ARGS))

/*****************************************************************************
*   apply(function, tuple)
*****************************************************************************/
#if __cplusplus >= 201703L

// C++17 implementation
using std::apply;

#else // C++14

// Derived from https://stackoverflow.com/a/20441189
// and https://en.cppreference.com/w/cpp/utility/apply
template <typename CALLABLE, typename... ARGS, std::size_t... I>
auto apply_impl(CALLABLE&& func, const std::tuple<ARGS...>& args, std::index_sequence<I...>)
{
    // call func(unpacked args)
    return std::forward<CALLABLE>(func)(std::move(std::get<I>(args))...);
}

template <typename CALLABLE, typename... ARGS>
auto apply(CALLABLE&& func, const std::tuple<ARGS...>& args)
{
    // std::index_sequence_for is the magic sauce here, generating an argument
    // pack of indexes for each entry in args. apply_impl() can then pass
    // those to std::get() to unpack args into individual arguments.
    return apply_impl(std::forward<CALLABLE>(func),
                      args,
                      std::index_sequence_for<ARGS...>{});
}

#endif // C++14

/*****************************************************************************
*   apply(function, std::array)
*****************************************************************************/
// per https://stackoverflow.com/a/57510428/5533635
template <typename CALLABLE, typename T, size_t SIZE>
auto apply(CALLABLE&& func, const std::array<T, SIZE>& args)
{
    return apply(std::forward<CALLABLE>(func), std::tuple_cat(args));
}

/*****************************************************************************
*   apply(function, std::vector)
*****************************************************************************/
// per https://stackoverflow.com/a/28411055/5533635
template <typename CALLABLE, typename T, std::size_t... I>
auto apply_impl(CALLABLE&& func, const std::vector<T>& args, std::index_sequence<I...>)
{
    return apply(std::forward<CALLABLE>(func),
                 std::make_tuple(args[I]...));
}

/**
 * apply(function, std::vector) goes beyond C++17 std::apply(). For this case
 * @a function @emph cannot be variadic: the compiler must know at compile
 * time how many arguments to pass. This isn't Python.
 */
template <typename CALLABLE, typename T>
auto apply(CALLABLE&& func, const std::vector<T>& args)
{
    constexpr auto arity = boost::function_traits<CALLABLE>::arity;
    assert(args.size() == arity);
    return apply_impl(std::forward<CALLABLE>(func),
                      args,
                      std::make_index_sequence<arity>());
}

} // namespace LL

#endif /* ! defined(LL_APPLY_H) */
