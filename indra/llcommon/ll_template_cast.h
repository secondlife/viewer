/**
 * @file   ll_template_cast.h
 * @author Nat Goodspeed
 * @date   2009-11-21
 * @brief  Define ll_template_cast function
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LL_TEMPLATE_CAST_H)
#define LL_LL_TEMPLATE_CAST_H

/**
 * Implementation for ll_template_cast() (q.v.).
 *
 * Default implementation: trying to cast two completely unrelated types
 * returns 0. Typically you'd specify T and U as pointer types, but in fact T
 * can be any type that can be initialized with 0.
 */
template <typename T, typename U>
struct ll_template_cast_impl
{
    T operator()(U)
    {
        return 0;
    }
};

/**
 * ll_template_cast<T>(some_value) is for use in a template function when
 * some_value might be of arbitrary type, but you want to recognize type T
 * specially.
 *
 * It's designed for use with pointer types. Example:
 * @code
 * struct SpecialClass
 * {
 *     void someMethod(const std::string&) const;
 * };
 *
 * template <class REALCLASS>
 * void somefunc(const REALCLASS& instance)
 * {
 *     const SpecialClass* ptr = ll_template_cast<const SpecialClass*>(&instance);
 *     if (ptr)
 *     {
 *         ptr->someMethod("Call method only available on SpecialClass");
 *     }
 * }
 * @endcode
 *
 * Why is this better than dynamic_cast<>? Because unless OtherClass is
 * polymorphic, the following won't even compile (gcc 4.0.1):
 * @code
 * OtherClass other;
 * SpecialClass* ptr = dynamic_cast<SpecialClass*>(&other);
 * @endcode
 * to say nothing of this:
 * @code
 * void function(int);
 * SpecialClass* ptr = dynamic_cast<SpecialClass*>(&function);
 * @endcode
 * ll_template_cast handles these kinds of cases by returning 0.
 */
template <typename T, typename U>
T ll_template_cast(U value)
{
    return ll_template_cast_impl<T, U>()(value);
}

/**
 * Implementation for ll_template_cast() (q.v.).
 *
 * Implementation for identical types: return same value.
 */
template <typename T>
struct ll_template_cast_impl<T, T>
{
    T operator()(T value)
    {
        return value;
    }
};

/**
 * LL_TEMPLATE_CONVERTIBLE(dest, source) asserts that, for a value @c s of
 * type @c source, <tt>ll_template_cast<dest>(s)</tt> will return @c s --
 * presuming that @c source can be converted to @c dest by the normal rules of
 * C++.
 *
 * By default, <tt>ll_template_cast<dest>(s)</tt> will return 0 unless @c s's
 * type is literally identical to @c dest. (This is because of the
 * straightforward application of template specialization rules.) That can
 * lead to surprising results, e.g.:
 *
 * @code
 * Foo myFoo;
 * const Foo* fooptr = ll_template_cast<const Foo*>(&myFoo);
 * @endcode
 *
 * Here @c fooptr will be 0 because <tt>&myFoo</tt> is of type <tt>Foo*</tt>
 * -- @em not <tt>const Foo*</tt>. (Declaring <tt>const Foo myFoo;</tt> would
 * force the compiler to do the right thing.)
 *
 * More disappointingly:
 * @code
 * struct Base {};
 * struct Subclass: public Base {};
 * Subclass object;
 * Base* ptr = ll_template_cast<Base*>(&object);
 * @endcode
 *
 * Here @c ptr will be 0 because <tt>&object</tt> is of type
 * <tt>Subclass*</tt> rather than <tt>Base*</tt>. We @em want this cast to
 * succeed, but without our help ll_template_cast can't recognize it.
 *
 * The following would suffice:
 * @code
 * LL_TEMPLATE_CONVERTIBLE(Base*, Subclass*);
 * ...
 * Base* ptr = ll_template_cast<Base*>(&object);
 * @endcode
 *
 * However, as noted earlier, this is easily fooled:
 * @code
 * const Base* ptr = ll_template_cast<const Base*>(&object);
 * @endcode
 * would still produce 0 because we haven't yet seen:
 * @code
 * LL_TEMPLATE_CONVERTIBLE(const Base*, Subclass*);
 * @endcode
 *
 * @TODO
 * This macro should use Boost type_traits facilities for stripping and
 * re-adding @c const and @c volatile qualifiers so that invoking
 * LL_TEMPLATE_CONVERTIBLE(dest, source) will automatically generate all
 * permitted permutations. It's really not fair to the coder to require
 * separate:
 * @code
 * LL_TEMPLATE_CONVERTIBLE(Base*, Subclass*);
 * LL_TEMPLATE_CONVERTIBLE(const Base*, Subclass*);
 * LL_TEMPLATE_CONVERTIBLE(const Base*, const Subclass*);
 * @endcode
 *
 * (Naturally we omit <tt>LL_TEMPLATE_CONVERTIBLE(Base*, const Subclass*)</tt>
 * because that's not permitted by normal C++ assignment anyway.)
 */
#define LL_TEMPLATE_CONVERTIBLE(DEST, SOURCE)   \
template <>                                     \
struct ll_template_cast_impl<DEST, SOURCE>      \
{                                               \
    DEST operator()(SOURCE wrapper)             \
    {                                           \
        return wrapper;                         \
    }                                           \
}

#endif /* ! defined(LL_LL_TEMPLATE_CAST_H) */
