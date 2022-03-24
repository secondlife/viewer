/**
 * @file   llmake.h
 * @author Nat Goodspeed
 * @date   2015-12-18
 * @brief  Generic llmake<Template>(arg) function to instantiate
 *         Template<decltype(arg)>(arg).
 *
 *         Many of our class templates have an accompanying helper function to
 *         make an instance with arguments of arbitrary type. llmake()
 *         eliminates the need to declare a new helper function for every such
 *         class template.
 * 
 *         also relevant:
 *
 *         Template argument deduction for class templates (C++17)
 *         https://en.cppreference.com/w/cpp/language/class_template_argument_deduction
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Copyright (c) 2015, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLMAKE_H)
#define LL_LLMAKE_H

/**
 * Usage: llmake<SomeTemplate>(args...)
 *
 * Deduces the types T... of 'args' and returns an instance of
 * SomeTemplate<T...>(args...).
 */
template <template<typename...> class CLASS_TEMPLATE, typename... ARGS>
CLASS_TEMPLATE<ARGS...> llmake(ARGS && ... args)
{
    return CLASS_TEMPLATE<ARGS...>(std::forward<ARGS>(args)...);
}

/// dumb pointer template just in case that's what's wanted
template <typename T>
using dumb_pointer = T*;

/**
 * Same as llmake(), but returns a pointer to a new heap instance of
 * SomeTemplate<T...>(args...) using the pointer of your choice.
 *
 * @code
 * auto* dumb  = llmake_heap<SomeTemplate>(args...);
 * auto shared = llmake_heap<SomeTemplate, std::shared_ptr>(args...);
 * auto unique = llmake_heap<SomeTemplate, std::unique_ptr>(args...);
 * @endcode
 */
// POINTER_TEMPLATE is characterized as template<typename...> rather than as
// template<typename T> because (e.g.) std::unique_ptr has multiple template
// arguments. Even though we only engage one, std::unique_ptr doesn't match a
// template template parameter that itself takes only one template parameter.
template <template<typename...> class CLASS_TEMPLATE,
          template<typename...> class POINTER_TEMPLATE=dumb_pointer,
          typename... ARGS>
POINTER_TEMPLATE<CLASS_TEMPLATE<ARGS...>> llmake_heap(ARGS&&... args)
{
    return POINTER_TEMPLATE<CLASS_TEMPLATE<ARGS...>>(
        new CLASS_TEMPLATE<ARGS...>(std::forward<ARGS>(args)...));
}

#endif /* ! defined(LL_LLMAKE_H) */
