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

#include "llexception.h"
#include <boost/type_traits/function_traits.hpp>
#include <functional>               // std::mem_fn()
#include <tuple>
#include <type_traits>              // std::is_member_pointer

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
*   invoke()
*****************************************************************************/
#if __cpp_lib_invoke >= 201411L

// C++17 implementation
using std::invoke;

#else  // no std::invoke

// Use invoke() to handle pointer-to-method:
// derived from https://stackoverflow.com/a/38288251
template<typename Fn, typename... Args,
         typename std::enable_if<std::is_member_pointer<typename std::decay<Fn>::type>::value,
                                 int>::type = 0 >
auto invoke(Fn&& f, Args&&... args)
{
    return std::mem_fn(std::forward<Fn>(f))(std::forward<Args>(args)...);
}

template<typename Fn, typename... Args,
         typename std::enable_if<!std::is_member_pointer<typename std::decay<Fn>::type>::value,
                                 int>::type = 0 >
auto invoke(Fn&& f, Args&&... args)
{
    return std::forward<Fn>(f)(std::forward<Args>(args)...);
}

#endif // no std::invoke

/*****************************************************************************
*   apply(function, tuple); apply(function, array)
*****************************************************************************/
#if __cpp_lib_apply >= 201603L

// C++17 implementation
// We don't just say 'using std::apply;' because that template is too general:
// it also picks up the apply(function, vector) case, which we want to handle
// below.
template <typename CALLABLE, typename... ARGS>
auto apply(CALLABLE&& func, const std::tuple<ARGS...>& args)
{
    return std::apply(std::forward<CALLABLE>(func), args);
}

#else // C++14

// Derived from https://stackoverflow.com/a/20441189
// and https://en.cppreference.com/w/cpp/utility/apply
template <typename CALLABLE, typename... ARGS, std::size_t... I>
auto apply_impl(CALLABLE&& func, const std::tuple<ARGS...>& args, std::index_sequence<I...>)
{
    // We accept const std::tuple& so a caller can construct an tuple on the
    // fly. But std::get<I>(const tuple) adds a const qualifier to everything
    // it extracts. Get a non-const ref to this tuple so we can extract
    // without the extraneous const.
    auto& non_const_args{ const_cast<std::tuple<ARGS...>&>(args) };

    // call func(unpacked args)
    return invoke(std::forward<CALLABLE>(func),
                  std::forward<ARGS>(std::get<I>(non_const_args))...);
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

// per https://stackoverflow.com/a/57510428/5533635
template <typename CALLABLE, typename T, size_t SIZE>
auto apply(CALLABLE&& func, const std::array<T, SIZE>& args)
{
    return apply(std::forward<CALLABLE>(func), std::tuple_cat(args));
}

/*****************************************************************************
*   bind_front()
*****************************************************************************/
// To invoke a non-static member function with a tuple, you need a callable
// that binds your member function with an instance pointer or reference.
// std::bind_front() is perfect: std::bind_front(&cls::method, instance).
// Unfortunately bind_front() only enters the standard library in C++20.
#if __cpp_lib_bind_front >= 201907L

// C++20 implementation
using std::bind_front;

#else  // no std::bind_front()

template<typename Fn, typename... Args,
         typename std::enable_if<!std::is_member_pointer<typename std::decay<Fn>::type>::value,
                                 int>::type = 0 >
auto bind_front(Fn&& f, Args&&... args)
{
    // Don't use perfect forwarding for f or args: we must bind them for later.
    return [f, pfx_args=std::make_tuple(args...)]
        (auto&&... sfx_args)
    {
        // Use perfect forwarding for sfx_args because we use them as soon as
        // we receive them.
        return apply(
            f,
            std::tuple_cat(pfx_args,
                           std::make_tuple(std::forward<decltype(sfx_args)>(sfx_args)...)));
    };
}

template<typename Fn, typename... Args,
         typename std::enable_if<std::is_member_pointer<typename std::decay<Fn>::type>::value,
                                 int>::type = 0 >
auto bind_front(Fn&& f, Args&&... args)
{
    return bind_front(std::mem_fn(std::forward<Fn>(f)), std::forward<Args>(args)...);
}

#endif // C++20 with std::bind_front()

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

// produce suitable error if apply(func, vector) is the wrong size for func()
void apply_validate_size(size_t size, size_t arity);

/// possible exception from apply() validation
struct apply_error: public LLException
{
    apply_error(const std::string& what): LLException(what) {}
};

template <size_t ARITY, typename CALLABLE, typename T>
auto apply_n(CALLABLE&& func, const std::vector<T>& args)
{
    apply_validate_size(args.size(), ARITY);
    return apply_impl(std::forward<CALLABLE>(func),
                      args,
                      std::make_index_sequence<ARITY>());
}

/**
 * apply(function, std::vector) goes beyond C++17 std::apply(). For this case
 * @a function @emph cannot be variadic: the compiler must know at compile
 * time how many arguments to pass. This isn't Python. (But see apply_n() to
 * pass a specific number of args to a variadic function.)
 */
template <typename CALLABLE, typename T>
auto apply(CALLABLE&& func, const std::vector<T>& args)
{
    // infer arity from the definition of func
    constexpr auto arity = boost::function_traits<CALLABLE>::arity;
    // now that we have a compile-time arity, apply_n() works
    return apply_n<arity>(std::forward<CALLABLE>(func), args);
}

} // namespace LL

#endif /* ! defined(LL_APPLY_H) */
