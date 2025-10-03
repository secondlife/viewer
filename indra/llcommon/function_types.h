/**
 * @file   function_types.h
 * @author Nat Goodspeed
 * @date   2023-01-20
 * @brief  Extend boost::function_types to examine boost::function and
 *         std::function
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Copyright (c) 2023, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_FUNCTION_TYPES_H)
#define LL_FUNCTION_TYPES_H

#include <boost/function.hpp>
#include <boost/function_types/function_arity.hpp>
#include <functional>

namespace LL
{

    template <typename F>
    struct function_arity_impl
    {
        static constexpr auto value = boost::function_types::function_arity<F>::value;
    };

    template <typename F>
    struct function_arity_impl<std::function<F>>
    {
        static constexpr auto value = function_arity_impl<F>::value;
    };

    template <typename F>
    struct function_arity_impl<boost::function<F>>
    {
        static constexpr auto value = function_arity_impl<F>::value;
    };

    template <typename F>
    struct function_arity
    {
        static constexpr auto value = function_arity_impl<typename std::decay<F>::type>::value;
    };

} // namespace LL

#endif /* ! defined(LL_FUNCTION_TYPES_H) */
