/**
 * @file   llheteromap.h
 * @author Nat Goodspeed
 * @date   2016-10-12
 * @brief  Map capable of storing objects of diverse types, looked up by type.
 * 
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Copyright (c) 2016, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLHETEROMAP_H)
#define LL_LLHETEROMAP_H

#include <typeinfo>
#include <utility>                  // std::pair
#include <map>

/**
 * LLHeteroMap addresses an odd requirement. Usually when you want to put
 * objects of different classes into a runtime collection of any kind, you
 * derive them all from a common base class and store pointers to that common
 * base class.
 *
 * LLInitParam::BaseBlock uses disturbing raw-pointer arithmetic to find data
 * members in its subclasses. It seems that no BaseBlock subclass can be
 * stored in a polymorphic class of any kind: the presence of a vtbl pointer
 * in the layout silently throws off the reinterpret_cast arithmetic. Bad
 * Things result. (Many thanks to Nicky D for this analysis!)
 *
 * LLHeteroMap collects objects WITHOUT a common base class, retrieves them by
 * object type and destroys them when the LLHeteroMap is destroyed.
 */
class LLHeteroMap
{
public:
    ~LLHeteroMap();

    /// find or create
    template <class T>
    T& obtain()
    {
        // Look up map entry by typeid(T). We don't simply use mMap[typeid(T)]
        // because that requires default-constructing T on every lookup. For
        // some kinds of T, that could be expensive.
        TypeMap::iterator found = mMap.find(&typeid(T));
        if (found == mMap.end())
        {
            // Didn't find typeid(T). Create an entry. Because we're storing
            // only a void* in the map, discarding type information, make sure
            // we capture that type information in our deleter.
            void* ptr = new T();
            void (*dlfn)(void*) = &deleter<T>;
            std::pair<TypeMap::iterator, bool> inserted =
                mMap.insert(TypeMap::value_type(&typeid(T),
                    TypeMap::mapped_type(ptr, dlfn)));
            // Okay, now that we have an entry, claim we found it.
            found = inserted.first;
        }
        // found->second is the std::pair; second.first is the void*
        // pointer to the object in question. Cast it to correct type and
        // dereference it.
        return *(static_cast<T*>(found->second.first));
    }

private:
    template <class T>
    static
    void deleter(void* p)
    {
        delete static_cast<T*>(p);
    }

    // Comparing two std::type_info* values is tricky, because the standard
    // does not guarantee that there will be only one type_info instance for a
    // given type. In other words, &typeid(A) in one part of the program may
    // not always equal &typeid(A) in some other part. Use special comparator.
    struct type_info_ptr_comp
    {
        bool operator()(const std::type_info* lhs, const std::type_info* rhs) const
        {
            return lhs->before(*rhs);
        }
    };

    // What we actually store is a map from std::type_info (permitting lookup
    // by object type) to a void* pointer to the object PLUS its deleter.
    typedef std::map<
        const std::type_info*, std::pair<void*, void (*)(void*)>,
        type_info_ptr_comp>
    TypeMap;
    TypeMap mMap;
};

#endif /* ! defined(LL_LLHETEROMAP_H) */
