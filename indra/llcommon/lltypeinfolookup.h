/**
 * @file   lltypeinfolookup.h
 * @author Nat Goodspeed
 * @date   2012-04-08
 * @brief  Template data structure like std::map<std::type_info*, T>
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLTYPEINFOLOOKUP_H)
#define LL_LLTYPEINFOLOOKUP_H

#include <boost/unordered_map.hpp>
#include <boost/functional/hash.hpp>
#include <boost/optional.hpp>
#include <functional>               // std::binary_function
#include <typeinfo>

/**
 * The following helper classes are based on the Boost.Unordered documentation:
 * http://www.boost.org/doc/libs/1_45_0/doc/html/unordered/hash_equality.html
 */

/**
 * Compute hash for a string passed as const char*
 */
struct const_char_star_hash: public std::unary_function<const char*, std::size_t>
{
    std::size_t operator()(const char* str) const
    {
        std::size_t seed = 0;
        for ( ; *str; ++str)
        {
            boost::hash_combine(seed, *str);
        }
        return seed;
    }
};

/**
 * Compute equality for strings passed as const char*
 *
 * I (nat) suspect that this is where the default behavior breaks for the
 * const char* values returned from std::type_info::name(). If you compare the
 * two const char* pointer values, as a naive, unspecialized implementation
 * will surely do, they'll compare unequal.
 */
struct const_char_star_equal: public std::binary_function<const char*, const char*, bool>
{
    bool operator()(const char* lhs, const char* rhs) const
    {
        return strcmp(lhs, rhs) == 0;
    }
};

/**
 * LLTypeInfoLookup is specifically designed for use cases for which you might
 * consider std::map<std::type_info*, VALUE>. We have several such data
 * structures in the viewer. The trouble with them is that at least on Linux,
 * you can't rely on always getting the same std::type_info* for a given type:
 * different load modules will produce different std::type_info*.
 * LLTypeInfoLookup contains a workaround to address this issue.
 *
 * The API deliberately diverges from std::map in several respects:
 * * It avoids iterators, not only begin()/end() but also as return values
 *   from insert() and find(). This bypasses transform_iterator overhead.
 * * Since we literally use compile-time types as keys, the essential insert()
 *   and find() methods accept the key type as a @em template parameter,
 *   accepting and returning value_type as a normal runtime value. This is to
 *   permit future optimization (e.g. compile-time type hashing) without
 *   changing the API.
 */
template <typename VALUE>
class LLTypeInfoLookup
{
    // Use this for our underlying implementation: lookup by
    // std::type_info::name() string. This is one of the rare cases in which I
    // dare use const char* directly, rather than std::string, because I'm
    // sure that every value returned by std::type_info::name() is static.
    // HOWEVER, specify our own hash + equality functors: naively comparing
    // distinct const char* values won't work.
    typedef boost::unordered_map<const char*, VALUE,
                                 const_char_star_hash, const_char_star_equal> impl_map_type;

public:
    typedef VALUE value_type;

    LLTypeInfoLookup() {}

    bool empty() const { return mMap.empty(); }
    std::size_t size() const { return mMap.size(); }

    template <typename KEY>
    bool insert(const value_type& value)
    {
        // Obtain and store the std::type_info::name() string as the key.
        // Return just the bool from std::map::insert()'s return pair.
        return mMap.insert(typename impl_map_type::value_type(typeid(KEY).name(), value)).second;
    }

    template <typename KEY>
    boost::optional<value_type> find() const
    {
        // Use the std::type_info::name() string as the key.
        typename impl_map_type::const_iterator found = mMap.find(typeid(KEY).name());
        if (found == mMap.end())
            return boost::optional<value_type>();
        return found->second;
    }

private:
    impl_map_type mMap;
};

#endif /* ! defined(LL_LLTYPEINFOLOOKUP_H) */
