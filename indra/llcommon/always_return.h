/**
 * @file   always_return.h
 * @author Nat Goodspeed
 * @date   2023-01-20
 * @brief  Call specified callable with arbitrary arguments, but always return
 *         specified type.
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Copyright (c) 2023, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_ALWAYS_RETURN_H)
#define LL_ALWAYS_RETURN_H

#include <type_traits>              // std::enable_if, std::is_convertible

namespace LL
{

#if __cpp_lib_is_invocable >= 201703L // C++17
    template <typename CALLABLE, typename... ARGS>
    using invoke_result = std::invoke_result<CALLABLE, ARGS...>;
#else  // C++14
    template <typename CALLABLE, typename... ARGS>
    using invoke_result = std::result_of<CALLABLE(ARGS...)>;
#endif // C++14

    /**
     * AlwaysReturn<T>()(some_function, some_args...) calls
     * some_function(some_args...). It is guaranteed to return a value of type
     * T, regardless of the return type of some_function(). If some_function()
     * returns a type convertible to T, it will convert and return that value.
     * Otherwise (notably if some_function() is void), AlwaysReturn returns
     * T().
     *
     * When some_function() returns a type not convertible to T, if
     * you want AlwaysReturn to return some T value other than
     * default-constructed T(), pass that value to AlwaysReturn's constructor.
     */
    template <typename DESIRED>
    class AlwaysReturn
    {
    public:
        /// pass explicit default value if other than default-constructed type
        AlwaysReturn(const DESIRED& dft=DESIRED()): mDefault(dft) {}

        // callable returns a type not convertible to DESIRED, return default
        template <typename CALLABLE, typename... ARGS,
                  typename std::enable_if<
                      ! std::is_convertible<
                          typename invoke_result<CALLABLE, ARGS...>::type,
                          DESIRED
                      >::value,
                      bool
                  >::type=true>
        DESIRED operator()(CALLABLE&& callable, ARGS&&... args)
        {
            // discard whatever callable(args) returns
            std::forward<CALLABLE>(callable)(std::forward<ARGS>(args)...);
            return mDefault;
        }

        // callable returns a type convertible to DESIRED
        template <typename CALLABLE, typename... ARGS,
                  typename std::enable_if<
                      std::is_convertible<
                          typename invoke_result<CALLABLE, ARGS...>::type,
                          DESIRED
                      >::value,
                      bool
                  >::type=true>
        DESIRED operator()(CALLABLE&& callable, ARGS&&... args)
        {
            return { std::forward<CALLABLE>(callable)(std::forward<ARGS>(args)...) };
        }

    private:
        DESIRED mDefault;
    };

    /**
     * always_return<T>(some_function, some_args...) calls
     * some_function(some_args...). It is guaranteed to return a value of type
     * T, regardless of the return type of some_function(). If some_function()
     * returns a type convertible to T, it will convert and return that value.
     * Otherwise (notably if some_function() is void), always_return() returns
     * T().
     */
    template <typename DESIRED, typename CALLABLE, typename... ARGS>
    DESIRED always_return(CALLABLE&& callable, ARGS&&... args)
    {
        return AlwaysReturn<DESIRED>()(std::forward<CALLABLE>(callable),
                                       std::forward<ARGS>(args)...);
    }

    /**
     * make_always_return<T>(some_function) returns a callable which, when
     * called with appropriate some_function() arguments, always returns a
     * value of type T, regardless of the return type of some_function(). If
     * some_function() returns a type convertible to T, the returned callable
     * will convert and return that value. Otherwise (notably if
     * some_function() is void), the returned callable returns T().
     *
     * When some_function() returns a type not convertible to T, if
     * you want the returned callable to return some T value other than
     * default-constructed T(), pass that value to make_always_return() as its
     * optional second argument.
     */
    template <typename DESIRED, typename CALLABLE>
    auto make_always_return(CALLABLE&& callable, const DESIRED& dft=DESIRED())
    {
        return
            [dft, callable = std::forward<CALLABLE>(callable)]
            (auto&&... args)
            {
                return AlwaysReturn<DESIRED>(dft)(callable,
                                                  std::forward<decltype(args)>(args)...);
            };
    }

} // namespace LL

#endif /* ! defined(LL_ALWAYS_RETURN_H) */
