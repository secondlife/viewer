/** 
 * @file llsdutil.h
 * @author Phoenix
 * @date 2006-05-24
 * @brief Utility classes, functions, etc, for using structured data.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLSDUTIL_H
#define LL_LLSDUTIL_H

#include "apply.h"                  // LL::invoke()
#include "function_types.h"         // LL::function_arity
#include "llsd.h"
#include <boost/functional/hash.hpp>
#include <cassert>
#include <memory>                   // std::shared_ptr
#include <type_traits>
#include <vector>

// U32
LL_COMMON_API LLSD ll_sd_from_U32(const U32);
LL_COMMON_API U32 ll_U32_from_sd(const LLSD& sd);

// U64
LL_COMMON_API LLSD ll_sd_from_U64(const U64);
LL_COMMON_API U64 ll_U64_from_sd(const LLSD& sd);

// IP Address
LL_COMMON_API LLSD ll_sd_from_ipaddr(const U32);
LL_COMMON_API U32 ll_ipaddr_from_sd(const LLSD& sd);

// Binary to string
LL_COMMON_API LLSD ll_string_from_binary(const LLSD& sd);

//String to binary
LL_COMMON_API LLSD ll_binary_from_string(const LLSD& sd);

// Serializes sd to static buffer and returns pointer, useful for gdb debugging.
LL_COMMON_API char* ll_print_sd(const LLSD& sd);

// Serializes sd to static buffer and returns pointer, using "pretty printing" mode.
LL_COMMON_API char* ll_pretty_print_sd_ptr(const LLSD* sd);
LL_COMMON_API char* ll_pretty_print_sd(const LLSD& sd);

LL_COMMON_API std::string ll_stream_notation_sd(const LLSD& sd);

//compares the structure of an LLSD to a template LLSD and stores the
//"valid" values in a 3rd LLSD. Default values
//are pulled from the template.  Extra keys/values in the test
//are ignored in the resultant LLSD.  Ordering of arrays matters
//Returns false if the test is of same type but values differ in type
//Otherwise, returns true

LL_COMMON_API BOOL compare_llsd_with_template(
	const LLSD& llsd_to_test,
	const LLSD& template_llsd,
	LLSD& resultant_llsd);

// filter_llsd_with_template() is a direct clone (copy-n-paste) of 
// compare_llsd_with_template with the following differences:
// (1) bool vs BOOL return types
// (2) A map with the key value "*" is a special value and maps any key in the
//     test llsd that doesn't have an explicitly matching key in the template.
// (3) The element of an array with exactly one element is taken as a template
//     for *all* the elements of the test array.  If the template array is of
//     different size, compare_llsd_with_template() semantics apply.
bool filter_llsd_with_template(
	const LLSD & llsd_to_test,
	const LLSD & template_llsd,
	LLSD & resultant_llsd);

/**
 * Recursively determine whether a given LLSD data block "matches" another
 * LLSD prototype. The returned string is empty() on success, non-empty() on
 * mismatch.
 *
 * This function tests structure (types) rather than data values. It is
 * intended for when a consumer expects an LLSD block with a particular
 * structure, and must succinctly detect whether the arriving block is
 * well-formed. For instance, a test of the form:
 * @code
 * if (! (data.has("request") && data.has("target") && data.has("modifier") ...))
 * @endcode
 * could instead be expressed by initializing a prototype LLSD map with the
 * required keys and writing:
 * @code
 * if (! llsd_matches(prototype, data).empty())
 * @endcode
 *
 * A non-empty return value is an error-message fragment intended to indicate
 * to (English-speaking) developers where in the prototype structure the
 * mismatch occurred.
 *
 * * If a slot in the prototype isUndefined(), then anything is valid at that
 *   place in the real object. (Passing prototype == LLSD() matches anything
 *   at all.)
 * * An array in the prototype must match a data array at least that large.
 *   (Additional entries in the data array are ignored.) Every isDefined()
 *   entry in the prototype array must match the corresponding entry in the
 *   data array.
 * * A map in the prototype must match a map in the data. Every key in the
 *   prototype map must match a corresponding key in the data map. (Additional
 *   keys in the data map are ignored.) Every isDefined() value in the
 *   prototype map must match the corresponding key's value in the data map.
 * * Scalar values in the prototype are tested for @em type rather than value.
 *   For instance, a String in the prototype matches any String at all. In
 *   effect, storing an Integer at a particular place in the prototype asserts
 *   that the caller intends to apply asInteger() to the corresponding slot in
 *   the data.
 * * A String in the prototype matches String, Boolean, Integer, Real, UUID,
 *   Date and URI, because asString() applied to any of these produces a
 *   meaningful result.
 * * Similarly, a Boolean, Integer or Real in the prototype can match any of
 *   Boolean, Integer or Real in the data -- or even String.
 * * UUID matches UUID or String.
 * * Date matches Date or String.
 * * URI matches URI or String.
 * * Binary in the prototype matches only Binary in the data.
 *
 * @TODO: when a Boolean, Integer or Real in the prototype matches a String in
 * the data, we should examine the String @em value to ensure it can be
 * meaningfully converted to the requested type. The same goes for UUID, Date
 * and URI.
 */
LL_COMMON_API std::string llsd_matches(const LLSD& prototype, const LLSD& data, const std::string& pfx="");

/// Deep equality. If you want to compare LLSD::Real values for approximate
/// equality rather than bitwise equality, pass @a bits as for
/// is_approx_equal_fraction().
LL_COMMON_API bool llsd_equals(const LLSD& lhs, const LLSD& rhs, int bits=-1);
/// If you don't care about LLSD::Real equality
inline bool operator==(const LLSD& lhs, const LLSD& rhs)
{
    return llsd_equals(lhs, rhs);
}
inline bool operator!=(const LLSD& lhs, const LLSD& rhs)
{
    // operator!=() should always be the negation of operator==()
    return ! (lhs == rhs);
}

// Simple function to copy data out of input & output iterators if
// there is no need for casting.
template<typename Input> LLSD llsd_copy_array(Input iter, Input end)
{
	LLSD dest;
	for (; iter != end; ++iter)
	{
		dest.append(*iter);
	}
	return dest;
}

namespace llsd
{

/**
 * Drill down to locate an element in 'blob' according to 'path', where 'path'
 * is one of the following:
 *
 * - LLSD::String: 'blob' is an LLSD::Map. Find the entry with key 'path'.
 * - LLSD::Integer: 'blob' is an LLSD::Array. Find the entry with index 'path'.
 * - Any other 'path' type will be interpreted as LLSD::Array, and 'blob' is a
 *   nested structure. For each element of 'path':
 *   - If it's an LLSD::Integer, select the entry with that index from an
 *     LLSD::Array at that level.
 *   - If it's an LLSD::String, select the entry with that key from an
 *     LLSD::Map at that level.
 *   - Anything else is an error.
 *
 * By implication, if path.isUndefined() or otherwise equivalent to an empty
 * LLSD::Array, drill[_ref]() returns 'blob' as is.
 */
LLSD  drill(const LLSD& blob, const LLSD& path);
LLSD& drill_ref(  LLSD& blob, const LLSD& path);

}

namespace llsd
{

/**
 * Construct an LLSD::Array inline, using modern C++ variadic arguments.
 */

// recursion tail
inline
void array_(LLSD&) {}

// recursive call
template <typename T0, typename... Ts>
void array_(LLSD& data, T0&& v0, Ts&&... vs)
{
    data.append(std::forward<T0>(v0));
    array_(data, std::forward<Ts>(vs)...);
}

// public interface
template <typename... Ts>
LLSD array(Ts&&... vs)
{
    LLSD data;
    array_(data, std::forward<Ts>(vs)...);
    return data;
}

} // namespace llsd

/*****************************************************************************
*   LLSDMap
*****************************************************************************/
/**
 * Construct an LLSD::Map inline, with implicit conversion to LLSD. Usage:
 *
 * @code
 * void somefunc(const LLSD&);
 * ...
 * somefunc(LLSDMap("alpha", "abc")("number", 17)("pi", 3.14));
 * @endcode
 *
 * For completeness, LLSDMap() with no args constructs an empty map, so
 * <tt>LLSDMap()("alpha", "abc")("number", 17)("pi", 3.14)</tt> produces a map
 * equivalent to the above. But for most purposes, LLSD() is already
 * equivalent to an empty map, and if you explicitly want an empty isMap(),
 * there's LLSD::emptyMap(). However, supporting a no-args LLSDMap()
 * constructor follows the principle of least astonishment.
 */
class LLSDMap
{
public:
    LLSDMap():
        _data(LLSD::emptyMap())
    {}
    LLSDMap(const LLSD::String& key, const LLSD& value):
        _data(LLSD::emptyMap())
    {
        _data[key] = value;
    }

    LLSDMap& operator()(const LLSD::String& key, const LLSD& value)
    {
        _data[key] = value;
        return *this;
    }

    operator LLSD() const { return _data; }
    LLSD get() const { return _data; }

private:
    LLSD _data;
};

namespace llsd
{

/**
 * Construct an LLSD::Map inline, using modern C++ variadic arguments.
 */

// recursion tail
inline
void map_(LLSD&) {}

// recursive call
template <typename T0, typename... Ts>
void map_(LLSD& data, const LLSD::String& k0, T0&& v0, Ts&&... vs)
{
    data[k0] = v0;
    map_(data, std::forward<Ts>(vs)...);
}

// public interface
template <typename... Ts>
LLSD map(Ts&&... vs)
{
    LLSD data;
    map_(data, std::forward<Ts>(vs)...);
    return data;
}

} // namespace llsd

/*****************************************************************************
*   LLSDParam
*****************************************************************************/
struct LLSDParamBase
{
    virtual ~LLSDParamBase() {}
};

/**
 * LLSDParam is a customization point for passing LLSD values to function
 * parameters of more or less arbitrary type. LLSD provides a small set of
 * native conversions; but if a generic algorithm explicitly constructs an
 * LLSDParam object in the function's argument list, a consumer can provide
 * LLSDParam specializations to support more different parameter types than
 * LLSD's native conversions.
 *
 * Usage:
 *
 * @code
 * void somefunc(const paramtype&);
 * ...
 * somefunc(..., LLSDParam<paramtype>(someLLSD), ...);
 * @endcode
 */
template <typename T>
class LLSDParam: public LLSDParamBase
{
public:
    /**
     * Default implementation converts to T on construction, saves converted
     * value for later retrieval
     */
    LLSDParam(const LLSD& value):
        value_(value)
    {}

    operator T() const { return value_; }

private:
    T value_;
};

/**
 * LLSDParam<LLSD> is for when you don't already have the target parameter
 * type in hand. Instantiate LLSDParam<LLSD>(your LLSD object), and the
 * templated conversion operator will try to select a more specific LLSDParam
 * specialization.
 */
template <>
class LLSDParam<LLSD>: public LLSDParamBase
{
private:
    LLSD value_;
    // LLSDParam<LLSD>::operator T() works by instantiating an LLSDParam<T> on
    // demand. Returning that engages LLSDParam<T>::operator T(), producing
    // the desired result. But LLSDParam<const char*> owns a std::string whose
    // c_str() is returned by its operator const char*(). If we return a temp
    // LLSDParam<const char*>, the compiler can destroy it right away, as soon
    // as we've called operator const char*(). That's a problem! That
    // invalidates the const char* we've just passed to the subject function.
    // This LLSDParam<LLSD> is presumably guaranteed to survive until the
    // subject function has returned, so we must ensure that any constructed
    // LLSDParam<T> lives just as long as this LLSDParam<LLSD> does. Putting
    // each LLSDParam<T> on the heap and capturing a smart pointer in a vector
    // works. We would have liked to use std::unique_ptr, but vector entries
    // must be copyable.
    // (Alternatively we could assume that every instance of LLSDParam<LLSD>
    // will be asked for at most ONE conversion. We could store a scalar
    // std::unique_ptr and, when constructing an new LLSDParam<T>, assert that
    // the unique_ptr is empty. But some future change in usage patterns, and
    // consequent failure of that assertion, would be very mysterious. Instead
    // of explaining how to fix it, just fix it now.)
    mutable std::vector<std::shared_ptr<LLSDParamBase>> converters_;

public:
    LLSDParam(const LLSD& value): value_(value) {}

    /// if we're literally being asked for an LLSD parameter, avoid infinite
    /// recursion
    operator LLSD() const { return value_; }

    /// otherwise, instantiate a more specific LLSDParam<T> to convert; that
    /// preserves the existing customization mechanism
    template <typename T>
    operator T() const
    {
        // capture 'ptr' with the specific subclass type because converters_
        // only stores LLSDParamBase pointers
        auto ptr{ std::make_shared<LLSDParam<std::decay_t<T>>>(value_) };
        // keep the new converter alive until we ourselves are destroyed
        converters_.push_back(ptr);
        return *ptr;
    }
};

/**
 * Turns out that several target types could accept an LLSD param using any of
 * a few different conversions, e.g. LLUUID's constructor can accept LLUUID or
 * std::string. Therefore, the compiler can't decide which LLSD conversion
 * operator to choose, even though to us it seems obvious. But that's okay, we
 * can specialize LLSDParam for such target types, explicitly specifying the
 * desired conversion -- that's part of what LLSDParam is all about. Turns out
 * we have to do that enough to make it worthwhile generalizing. Use a macro
 * because I need to specify one of the asReal, etc., explicit conversion
 * methods as well as a type. If I'm overlooking a clever way to implement
 * that using a template instead, feel free to reimplement.
 */
#define LLSDParam_for(T, AS)                    \
template <>                                     \
class LLSDParam<T>: public LLSDParamBase        \
{                                               \
public:                                         \
    LLSDParam(const LLSD& value):               \
        value_((T)value.AS())                   \
    {}                                          \
                                                \
    operator T() const { return value_; }       \
                                                \
private:                                        \
    T value_;                                   \
}

LLSDParam_for(float,        asReal);
LLSDParam_for(LLUUID,       asUUID);
LLSDParam_for(LLDate,       asDate);
LLSDParam_for(LLURI,        asURI);
LLSDParam_for(LLSD::Binary, asBinary);

/**
 * LLSDParam<const char*> is an example of the kind of conversion you can
 * support with LLSDParam beyond native LLSD conversions. Normally you can't
 * pass an LLSD object to a function accepting const char* -- but you can
 * safely pass an LLSDParam<const char*>(yourLLSD).
 */
template <>
class LLSDParam<const char*>: public LLSDParamBase
{
private:
    // The difference here is that we store a std::string rather than a const
    // char*. It's important that the LLSDParam object own the std::string.
    std::string value_;
    // We don't bother storing the incoming LLSD object, but we do have to
    // distinguish whether value_ is an empty string because the LLSD object
    // contains an empty string or because it's isUndefined().
    bool undefined_;

public:
    LLSDParam(const LLSD& value):
        value_(value),
        undefined_(value.isUndefined())
    {}

    // The const char* we retrieve is for storage owned by our value_ member.
    // That's how we guarantee that the const char* is valid for the lifetime
    // of this LLSDParam object. Constructing your LLSDParam in the argument
    // list should ensure that the LLSDParam object will persist for the
    // duration of the function call.
    operator const char*() const
    {
        if (undefined_)
        {
            // By default, an isUndefined() LLSD object's asString() method
            // will produce an empty string. But for a function accepting
            // const char*, it's often important to be able to pass NULL, and
            // isUndefined() seems like the best way. If you want to pass an
            // empty string, you can still pass LLSD(""). Without this special
            // case, though, no LLSD value could pass NULL.
            return NULL;
        }
        return value_.c_str();
    }
};

namespace llsd
{

/*****************************************************************************
*   BOOST_FOREACH() helpers for LLSD
*****************************************************************************/
/// Usage: BOOST_FOREACH(LLSD item, inArray(someLLSDarray)) { ... }
class inArray
{
public:
    inArray(const LLSD& array):
        _array(array)
    {}

    typedef LLSD::array_const_iterator const_iterator;
    typedef LLSD::array_iterator iterator;

    iterator begin() { return _array.beginArray(); }
    iterator end()   { return _array.endArray(); }
    const_iterator begin() const { return _array.beginArray(); }
    const_iterator end()   const { return _array.endArray(); }

private:
    LLSD _array;
};

/// MapEntry is what you get from dereferencing an LLSD::map_[const_]iterator.
typedef std::map<LLSD::String, LLSD>::value_type MapEntry;

/// Usage: BOOST_FOREACH([const] MapEntry& e, inMap(someLLSDmap)) { ... }
class inMap
{
public:
    inMap(const LLSD& map):
        _map(map)
    {}

    typedef LLSD::map_const_iterator const_iterator;
    typedef LLSD::map_iterator iterator;

    iterator begin() { return _map.beginMap(); }
    iterator end()   { return _map.endMap(); }
    const_iterator begin() const { return _map.beginMap(); }
    const_iterator end()   const { return _map.endMap(); }

private:
    LLSD _map;
};

} // namespace llsd


// Creates a deep clone of an LLSD object.  Maps, Arrays and binary objects 
// are duplicated, atomic primitives (Boolean, Integer, Real, etc) simply
// use a shared reference. 
// Optionally a filter may be specified to control what is duplicated. The 
// map takes the form "keyname/boolean".
// If the value is true the value will be duplicated otherwise it will be skipped 
// when encountered in a map. A key name of "*" can be specified as a wild card
// and will specify the default behavior.  If no wild card is given and the clone
// encounters a name not in the filter, that value will be skipped.
LLSD llsd_clone(LLSD value, LLSD filter = LLSD());

// Creates a shallow copy of a map or array.  If passed any other type of LLSD 
// object it simply returns that value.  See llsd_clone for a description of 
// the filter parameter.
LLSD llsd_shallow(LLSD value, LLSD filter = LLSD());

namespace llsd
{

// llsd namespace aliases
inline
LLSD clone  (LLSD value, LLSD filter=LLSD()) { return llsd_clone  (value, filter); }
inline
LLSD shallow(LLSD value, LLSD filter=LLSD()) { return llsd_shallow(value, filter); }

} // namespace llsd

// Specialization for generating a hash value from an LLSD block.
namespace boost
{
template <>
struct hash<LLSD>
{
    typedef LLSD argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const 
    {
        result_type seed(0);

        LLSD::Type stype = s.type();
        boost::hash_combine(seed, (S32)stype);

        switch (stype)
        {
        case LLSD::TypeBoolean:
            boost::hash_combine(seed, s.asBoolean());
            break;
        case LLSD::TypeInteger:
            boost::hash_combine(seed, s.asInteger());
            break;
        case LLSD::TypeReal:
            boost::hash_combine(seed, s.asReal());
            break;
        case LLSD::TypeURI:
        case LLSD::TypeString:
            boost::hash_combine(seed, s.asString());
            break;
        case LLSD::TypeUUID:
            boost::hash_combine(seed, s.asUUID());
            break;
        case LLSD::TypeDate:
            boost::hash_combine(seed, s.asDate().secondsSinceEpoch());
            break;
        case LLSD::TypeBinary:
        {
            const LLSD::Binary &b(s.asBinary());
            boost::hash_range(seed, b.begin(), b.end());
            break;
        }
        case LLSD::TypeMap:
        {
            for (LLSD::map_const_iterator itm = s.beginMap(); itm != s.endMap(); ++itm)
            {
                boost::hash_combine(seed, (*itm).first);
                boost::hash_combine(seed, (*itm).second);
            }
            break;
        }
        case LLSD::TypeArray:
            for (LLSD::array_const_iterator ita = s.beginArray(); ita != s.endArray(); ++ita)
            {
                boost::hash_combine(seed, (*ita));
            }
            break;
        case LLSD::TypeUndefined:
        default:
            break;
        }

        return seed;
    }
};
}

namespace LL
{

/*****************************************************************************
*   apply(function, LLSD array)
*****************************************************************************/
// validate incoming LLSD blob, and return an LLSD array suitable to pass to
// the function of interest
LLSD apply_llsd_fix(size_t arity, const LLSD& args);

// Derived from https://stackoverflow.com/a/20441189
// and https://en.cppreference.com/w/cpp/utility/apply .
// We can't simply make a tuple from the LLSD array and then apply() that
// tuple to the function -- how would make_tuple() deduce the correct
// parameter type for each entry? We must go directly to the target function.
template <typename CALLABLE, std::size_t... I>
auto apply_impl(CALLABLE&& func, const LLSD& array, std::index_sequence<I...>)
{
    // call func(unpacked args), using generic LLSDParam<LLSD> to convert each
    // entry in 'array' to the target parameter type
    return std::forward<CALLABLE>(func)(LLSDParam<LLSD>(array[I])...);
}

// use apply_n<ARITY>(function, LLSD) to call a specific arity of a variadic
// function with (that many) items from the passed LLSD array
template <size_t ARITY, typename CALLABLE>
auto apply_n(CALLABLE&& func, const LLSD& args)
{
    return apply_impl(std::forward<CALLABLE>(func),
                      apply_llsd_fix(ARITY, args),
                      std::make_index_sequence<ARITY>());
}

/**
 * apply(function, LLSD) goes beyond C++17 std::apply(). For this case
 * @a function @emph cannot be variadic: the compiler must know at compile
 * time how many arguments to pass. This isn't Python. (But see apply_n() to
 * pass a specific number of args to a variadic function.)
 */
template <typename CALLABLE>
auto apply(CALLABLE&& func, const LLSD& args)
{
    // infer arity from the definition of func
    constexpr auto arity = function_arity<
        typename std::remove_reference<CALLABLE>::type>::value;
    // now that we have a compile-time arity, apply_n() works
    return apply_n<arity>(std::forward<CALLABLE>(func), args);
}

} // namespace LL

#endif // LL_LLSDUTIL_H
