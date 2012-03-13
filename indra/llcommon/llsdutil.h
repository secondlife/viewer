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

class LLSD;

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
LL_COMMON_API bool llsd_equals(const LLSD& lhs, const LLSD& rhs, unsigned bits=-1);

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

/*****************************************************************************
*   LLSDArray
*****************************************************************************/
/**
 * Construct an LLSD::Array inline, with implicit conversion to LLSD. Usage:
 *
 * @code
 * void somefunc(const LLSD&);
 * ...
 * somefunc(LLSDArray("text")(17)(3.14));
 * @endcode
 *
 * For completeness, LLSDArray() with no args constructs an empty array, so
 * <tt>LLSDArray()("text")(17)(3.14)</tt> produces an array equivalent to the
 * above. But for most purposes, LLSD() is already equivalent to an empty
 * array, and if you explicitly want an empty isArray(), there's
 * LLSD::emptyArray(). However, supporting a no-args LLSDArray() constructor
 * follows the principle of least astonishment.
 */
class LLSDArray
{
public:
    LLSDArray():
        _data(LLSD::emptyArray())
    {}

    /**
     * Need an explicit copy constructor. Consider the following:
     *
     * @code
     * LLSD array_of_arrays(LLSDArray(LLSDArray(17)(34))
     *                               (LLSDArray("x")("y")));
     * @endcode
     *
     * The coder intends to construct [[17, 34], ["x", "y"]].
     *
     * With the compiler's implicit copy constructor, s/he gets instead
     * [17, 34, ["x", "y"]].
     *
     * The expression LLSDArray(17)(34) constructs an LLSDArray with those two
     * values. The reader assumes it should be converted to LLSD, as we always
     * want with LLSDArray, before passing it to the @em outer LLSDArray
     * constructor! This copy constructor makes that happen.
     */
    LLSDArray(const LLSDArray& inner):
        _data(LLSD::emptyArray())
    {
        _data.append(inner);
    }

    LLSDArray(const LLSD& value):
        _data(LLSD::emptyArray())
    {
        _data.append(value);
    }

    LLSDArray& operator()(const LLSD& value)
    {
        _data.append(value);
        return *this;
    }

    operator LLSD() const { return _data; }
    LLSD get() const { return _data; }

private:
    LLSD _data;
};

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

/*****************************************************************************
*   LLSDParam
*****************************************************************************/
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
class LLSDParam
{
public:
    /**
     * Default implementation converts to T on construction, saves converted
     * value for later retrieval
     */
    LLSDParam(const LLSD& value):
        _value(value)
    {}

    operator T() const { return _value; }

private:
    T _value;
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
class LLSDParam<T>                              \
{                                               \
public:                                         \
    LLSDParam(const LLSD& value):               \
        _value((T)value.AS())                      \
    {}                                          \
                                                \
    operator T() const { return _value; }       \
                                                \
private:                                        \
    T _value;                                   \
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
class LLSDParam<const char*>
{
private:
    // The difference here is that we store a std::string rather than a const
    // char*. It's important that the LLSDParam object own the std::string.
    std::string _value;
    // We don't bother storing the incoming LLSD object, but we do have to
    // distinguish whether _value is an empty string because the LLSD object
    // contains an empty string or because it's isUndefined().
    bool _undefined;

public:
    LLSDParam(const LLSD& value):
        _value(value),
        _undefined(value.isUndefined())
    {}

    // The const char* we retrieve is for storage owned by our _value member.
    // That's how we guarantee that the const char* is valid for the lifetime
    // of this LLSDParam object. Constructing your LLSDParam in the argument
    // list should ensure that the LLSDParam object will persist for the
    // duration of the function call.
    operator const char*() const
    {
        if (_undefined)
        {
            // By default, an isUndefined() LLSD object's asString() method
            // will produce an empty string. But for a function accepting
            // const char*, it's often important to be able to pass NULL, and
            // isUndefined() seems like the best way. If you want to pass an
            // empty string, you can still pass LLSD(""). Without this special
            // case, though, no LLSD value could pass NULL.
            return NULL;
        }
        return _value.c_str();
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

#endif // LL_LLSDUTIL_H
