/**
 * @file   llsortedvector.h
 * @author Nat Goodspeed
 * @date   2012-04-08
 * @brief  LLSortedVector class wraps a vector that we maintain in sorted
 *         order so we can perform binary-search lookups.
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLSORTEDVECTOR_H)
#define LL_LLSORTEDVECTOR_H

#include <vector>
#include <algorithm>

/**
 * LLSortedVector contains a std::vector<std::pair> that we keep sorted on the
 * first of the pair. This makes insertion somewhat more expensive than simple
 * std::vector::push_back(), but allows us to use binary search for lookups.
 * It's intended for small aggregates where lookup is far more performance-
 * critical than insertion; in such cases a binary search on a small, sorted
 * std::vector can be more performant than a std::map lookup.
 */
template <typename KEY, typename VALUE>
class LLSortedVector
{
public:
    typedef LLSortedVector<KEY, VALUE> self;
    typedef KEY key_type;
    typedef VALUE mapped_type;
    typedef std::pair<key_type, mapped_type> value_type;
    typedef std::vector<value_type> PairVector;
    typedef typename PairVector::iterator iterator;
    typedef typename PairVector::const_iterator const_iterator;

    /// Empty
    LLSortedVector() {}

    /// Fixed initial size
    LLSortedVector(std::size_t size):
        mVector(size)
    {}

    /// Bulk load
    template <typename ITER>
    LLSortedVector(ITER begin, ITER end):
        mVector(begin, end)
    {
        // Allow caller to dump in a bunch of (pairs convertible to)
        // value_type if desired, but make sure we sort afterwards.
        std::sort(mVector.begin(), mVector.end());
    }

    /// insert(key, value)
    std::pair<iterator, bool> insert(const key_type& key, const mapped_type& value)
    {
        return insert(value_type(key, value));
    }

    /// insert(value_type)
    std::pair<iterator, bool> insert(const value_type& pair)
    {
        typedef std::pair<iterator, bool> iterbool;
        iterator found = std::lower_bound(mVector.begin(), mVector.end(), pair,
                                          less<value_type>());
        // have to check for end() before it's even valid to dereference
        if (found == mVector.end())
        {
            std::size_t index(mVector.size());
            mVector.push_back(pair);
            // don't forget that push_back() invalidates 'found'
            return iterbool(mVector.begin() + index, true);
        }
        if (found->first == pair.first)
        {
            return iterbool(found, false);
        }
        // remember that insert() invalidates 'found' -- save index
        std::size_t index(found - mVector.begin());
        mVector.insert(found, pair);
        // okay, convert from index back to iterator
        return iterbool(mVector.begin() + index, true);
    }

    iterator begin() { return mVector.begin(); }
    iterator end()   { return mVector.end(); }
    const_iterator begin() const { return mVector.begin(); }
    const_iterator end()   const { return mVector.end(); }

    bool empty() const { return mVector.empty(); }
    std::size_t size() const { return mVector.size(); }

    /// find
    iterator find(const key_type& key)
    {
        iterator found = std::lower_bound(mVector.begin(), mVector.end(),
                                          value_type(key, mapped_type()),
                                          less<value_type>());
        if (found == mVector.end() || found->first != key)
            return mVector.end();
        return found;
    }

    const_iterator find(const key_type& key) const
    {
        return const_cast<self*>(this)->find(key);
    }

private:
    // Define our own 'less' comparator so we can specialize without messing
    // with std::less.
    template <typename T>
    struct less: public std::less<T> {};

    // Specialize 'less' for an LLSortedVector::value_type involving
    // std::type_info*. This is one of LLSortedVector's foremost use cases. We
    // specialize 'less' rather than just defining a specific comparator
    // because LLSortedVector should be usable for other key_types as well.
    template <typename T>
    struct less< std::pair<std::type_info*, T> >:
        public std::binary_function<std::pair<std::type_info*, T>,
                                    std::pair<std::type_info*, T>,
                                    bool>
    {
        bool operator()(const std::pair<std::type_info*, T>& lhs,
                        const std::pair<std::type_info*, T>& rhs) const
        {
            return lhs.first->before(*rhs.first);
        }
    };

    // Same as above, but with const std::type_info*.
    template <typename T>
    struct less< std::pair<const std::type_info*, T> >:
        public std::binary_function<std::pair<const std::type_info*, T>,
                                    std::pair<const std::type_info*, T>,
                                    bool>
    {
        bool operator()(const std::pair<const std::type_info*, T>& lhs,
                        const std::pair<const std::type_info*, T>& rhs) const
        {
            return lhs.first->before(*rhs.first);
        }
    };

    PairVector mVector;
};

#endif /* ! defined(LL_LLSORTEDVECTOR_H) */
