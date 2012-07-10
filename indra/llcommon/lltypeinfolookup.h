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
#include <boost/function.hpp>
#include <boost/mem_fn.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <typeinfo>
#include <map>

/**
 * LLTypeInfoLookup is specifically designed for use cases for which you might
 * consider std::map<std::type_info*, VALUE>. We have several such data
 * structures in the viewer. The trouble with them is that at least on Linux,
 * you can't rely on always getting the same std::type_info* for a given type:
 * different load modules will produce different std::type_info*.
 * LLTypeInfoLookup contains a workaround to address this issue.
 */
template <typename VALUE>
class LLTypeInfoLookup
{
    // We present an interface like this:
    typedef std::map<const std::type_info*, VALUE> intf_map_type;
    // Use this for our underlying implementation: lookup by
    // std::type_info::name() string. Note that we must store a std::pair<const
    // std::type_info*, VALUE> -- in other words, an intf_map_type::value_type
    // pair -- so we can present iterators that do the right thing.
    // (This might result in a lookup with one std::type_info* returning an
    // iterator to a different std::type_info*, but frankly, my dear, we don't
    // give a damn.)
    typedef boost::unordered_map<std::string, typename intf_map_type::value_type> impl_map_type;
    // Iterator shorthand
    typedef typename intf_map_type::iterator       intf_iterator;
    typedef typename intf_map_type::const_iterator intf_const_iterator;
    typedef typename intf_map_type::value_type     intf_value_type;
    typedef typename impl_map_type::iterator       impl_iterator;
    typedef typename impl_map_type::const_iterator impl_const_iterator;
    typedef typename impl_map_type::value_type     impl_value_type;
    // Type of function that transforms impl_value_type to intf_value_type
    typedef boost::function<intf_value_type&(impl_value_type&)> iterfunc;
    typedef boost::function<const intf_value_type&(const impl_value_type&)> const_iterfunc;

public:
    typedef LLTypeInfoLookup<VALUE> self;
    typedef typename intf_map_type::key_type    key_type;
    typedef typename intf_map_type::mapped_type mapped_type;
    typedef intf_value_type                     value_type;

    // Iterators are different because we have to transform
    // impl_map_type::iterator to intf_map_type::iterator.
    typedef boost::transform_iterator<iterfunc, impl_iterator> iterator;
    typedef boost::transform_iterator<const_iterfunc, impl_const_iterator> const_iterator;

    LLTypeInfoLookup() {}

    iterator begin() { return transform(mMap.begin()); }
    iterator end()   { return transform(mMap.end()); }
    const_iterator begin() const { return transform(mMap.begin()); }
    const_iterator end() const   { return transform(mMap.end()); }
    bool empty() const { return mMap.empty(); }
    std::size_t size() const { return mMap.size(); }

    // Shorthand -- I've always wished std::map supported this signature.
    std::pair<iterator, bool> insert(const std::type_info* key, const VALUE& value)
    {
        return insert(value_type(key, value));
    }

    std::pair<iterator, bool> insert(const value_type& pair)
    {
        // Obtain and store the std::type_info::name() string as the key.
        // Save the whole incoming pair as the value!
        std::pair<impl_iterator, bool>
            inserted(mMap.insert(impl_value_type(pair.first->name(), pair)));
        // Have to transform the iterator before returning.
        return std::pair<iterator, bool>(transform(inserted.first), inserted.second);
    }

    iterator find(const std::type_info* key)
    {
        return transform(mMap.find(key->name()));
    }

    const_iterator find(const std::type_info* key) const
    {
        return transform(mMap.find(key->name()));
    }

private:
    iterator transform(impl_iterator iter)
    {
        return iterator(iter, boost::mem_fn(&impl_value_type::second));
    }
    const_iterator transform(impl_const_iterator iter)
    {
        return const_iterator(iter, boost::mem_fn(&impl_value_type::second));
    }

    impl_map_type mMap;
};

#endif /* ! defined(LL_LLTYPEINFOLOOKUP_H) */
