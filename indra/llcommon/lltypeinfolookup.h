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

#include "llsortedvector.h"
#include <typeinfo>

/**
 * LLTypeInfoLookup is specifically designed for use cases for which you might
 * consider std::map<std::type_info*, VALUE>. We have several such data
 * structures in the viewer. The trouble with them is that at least on Linux,
 * you can't rely on always getting the same std::type_info* for a given type:
 * different load modules will produce different std::type_info*.
 * LLTypeInfoLookup contains a workaround to address this issue.
 *
 * Specifically, when we don't find the passed std::type_info*,
 * LLTypeInfoLookup performs a linear search over registered entries to
 * compare name() strings. Presuming that this succeeds, we cache the new
 * (previously unrecognized) std::type_info* to speed future lookups.
 *
 * This worst-case fallback search (linear search with string comparison)
 * should only happen the first time we look up a given type from a particular
 * load module other than the one from which we initially registered types.
 * (However, a lookup which wouldn't succeed anyway will always have
 * worst-case performance.) This class is probably best used with less than a
 * few dozen different types.
 */
template <typename VALUE>
class LLTypeInfoLookup
{
public:
    typedef LLTypeInfoLookup<VALUE> self;
    typedef LLSortedVector<const std::type_info*, VALUE> vector_type;
    typedef typename vector_type::key_type key_type;
    typedef typename vector_type::mapped_type mapped_type;
    typedef typename vector_type::value_type value_type;
    typedef typename vector_type::iterator iterator;
    typedef typename vector_type::const_iterator const_iterator;

    LLTypeInfoLookup() {}

    iterator begin() { return mVector.begin(); }
    iterator end()   { return mVector.end(); }
    const_iterator begin() const { return mVector.begin(); }
    const_iterator end()   const { return mVector.end(); }
    bool empty() const { return mVector.empty(); }
    std::size_t size() const { return mVector.size(); }

    std::pair<iterator, bool> insert(const std::type_info* key, const VALUE& value)
    {
        return insert(value_type(key, value));
    }

    std::pair<iterator, bool> insert(const value_type& pair)
    {
        return mVector.insert(pair);
    }

    // const find() forwards to non-const find(): this can alter mVector!
    const_iterator find(const std::type_info* key) const
    {
        return const_cast<self*>(this)->find(key);
    }

    // non-const find() caches previously-unknown type_info* to speed future
    // lookups.
    iterator find(const std::type_info* key)
    {
        iterator found = mVector.find(key);
        if (found != mVector.end())
        {
            // If LLSortedVector::find() found, great, we're done.
            return found;
        }
        // Here we didn't find the passed type_info*. On Linux, though, even
        // for the same type, typeid(sametype) produces a different type_info*
        // when used in different load modules. So the fact that we didn't
        // find the type_info* we seek doesn't mean this type isn't
        // registered. Scan for matching name() string.
        for (typename vector_type::iterator ti(mVector.begin()), tend(mVector.end());
             ti != tend; ++ti)
        {
            if (std::string(ti->first->name()) == key->name())
            {
                // This unrecognized 'key' is for the same type as ti->first.
                // To speed future lookups, insert a new entry that lets us
                // look up ti->second using this same 'key'.
                return insert(key, ti->second).first;
            }
        }
        // We simply have never seen a type with this type_info* from any load
        // module.
        return mVector.end();
    }

private:
    vector_type mVector;
};

#endif /* ! defined(LL_LLTYPEINFOLOOKUP_H) */
