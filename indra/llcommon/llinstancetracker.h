/** 
 * @file llinstancetracker.h
 * @brief LLInstanceTracker is a mixin class that automatically tracks object
 *        instances with or without an associated key
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLINSTANCETRACKER_H
#define LL_LLINSTANCETRACKER_H

#include <map>
#include <set>
#include <vector>
#include <typeinfo>
#include <memory>
#include <type_traits>

#if LL_WINDOWS
#pragma warning (push)
#pragma warning (disable:4265)
#endif
// 'std::_Pad' : class has virtual functions, but destructor is not virtual
#include <mutex>

#if LL_WINDOWS
#pragma warning (pop)
#endif

#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/filter_iterator.hpp>

/*****************************************************************************
*   LLInstanceTrackerBase
*****************************************************************************/
/**
 * Base class manages "class-static" data that must actually have singleton
 * semantics: one instance per process, rather than one instance per module as
 * sometimes happens with data simply declared static.
 */
namespace LLInstanceTrackerStuff
{
    struct StaticBase
    {
        // We need to be able to lock static data while manipulating it.
        typedef std::mutex mutex_t;
        mutex_t mMutex;
    };
} // namespace LLInstanceTrackerStuff

template <class Static>
class LL_COMMON_API LLInstanceTrackerBase
{
protected:
    typedef Static StaticData;

    // Instantiate this class to obtain a pointer to the canonical static
    // instance of class Static while holding a lock on that instance. Use of
    // Static::mMutex presumes either that Static is derived from StaticBase,
    // or that Static declares some other suitable mMutex.
    class LockStatic
    {
        typedef std::unique_lock<decltype(Static::mMutex)> lock_t;
    public:
        LockStatic():
            mData(getStatic()),
            mLock(mData->mMutex)
        {}
        Static* get() const { return mData; }
        operator Static*() const { return get(); }
        Static* operator->() const { return get(); }
        // sometimes we must explicitly unlock...
        void unlock()
        {
            // but once we do, access is no longer permitted
            mData = nullptr;
            mLock.unlock();
        }
    protected:
        Static* mData;
        lock_t mLock;
    private:
        Static* getStatic()
        {
            static Static sData;
            return &sData;
        }
    };
};

/*****************************************************************************
*   LLInstanceTracker with key
*****************************************************************************/
enum EInstanceTrackerAllowKeyCollisions
{
    LLInstanceTrackerErrorOnCollision,
    LLInstanceTrackerReplaceOnCollision
};

namespace LLInstanceTrackerStuff
{
    template <typename KEY, typename VALUE>
    struct StaticMap: public StaticBase
    {
        typedef std::map<KEY, VALUE> InstanceMap;
        InstanceMap mMap;
    };
} // LLInstanceTrackerStuff

/// This mix-in class adds support for tracking all instances of the specified class parameter T
/// The (optional) key associates a value of type KEY with a given instance of T, for quick lookup
/// If KEY is not provided, then instances are stored in a simple set
/// @NOTE: see explicit specialization below for default KEY==void case
template<typename T, typename KEY = void,
         EInstanceTrackerAllowKeyCollisions KEY_COLLISION_BEHAVIOR = LLInstanceTrackerErrorOnCollision>
class LLInstanceTracker :
    public LLInstanceTrackerBase<LLInstanceTrackerStuff::StaticMap<KEY, std::shared_ptr<T>>>
{
    typedef LLInstanceTrackerBase<LLInstanceTrackerStuff::StaticMap<KEY, std::shared_ptr<T>>> super;
    using typename super::StaticData;
    using typename super::LockStatic;
    typedef typename StaticData::InstanceMap InstanceMap;

public:
    // snapshot of std::pair<const KEY, std::shared_ptr<T>> pairs
    class snapshot
    {
        // It's very important that what we store in this snapshot are
        // weak_ptrs, NOT shared_ptrs. That's how we discover whether any
        // instance has been deleted during the lifespan of a snapshot.
        typedef std::vector<std::pair<const KEY, std::weak_ptr<T>>> VectorType;
        // Dereferencing our iterator produces a std::shared_ptr for each
        // instance that still exists. Since we store weak_ptrs, that involves
        // two chained transformations:
        // - a transform_iterator to lock the weak_ptr and return a shared_ptr
        // - a filter_iterator to skip any shared_ptr that has become invalid.
        // It is very important that we filter lazily, that is, during
        // traversal. Any one of our stored weak_ptrs might expire during
        // traversal.
        typedef std::pair<const KEY, std::shared_ptr<T>> strong_pair;
        // Note for future reference: nat has not yet had any luck (up to
        // Boost 1.67) trying to use boost::transform_iterator with a hand-
        // coded functor, only with actual functions. In my experience, an
        // internal boost::result_of() operation fails, even with an explicit
        // result_type typedef. But this works.
        static strong_pair strengthen(typename VectorType::value_type& pair)
        {
            return { pair.first, pair.second.lock() };
        }
        static bool dead_skipper(const strong_pair& pair)
        {
            return bool(pair.second);
        }

    public:
        snapshot():
            // populate our vector with a snapshot of (locked!) InstanceMap
            // note, this assigns pair<KEY, shared_ptr> to pair<KEY, weak_ptr>
            mData(mLock->mMap.begin(), mLock->mMap.end())
        {
            // release the lock once we've populated mData
            mLock.unlock();
        }

        // You can't make a transform_iterator (or anything else) that
        // literally stores a C++ function (decltype(strengthen)) -- but you
        // can make a transform_iterator based on a _function pointer._
        typedef boost::transform_iterator<decltype(strengthen)*,
                                          typename VectorType::iterator> strong_iterator;
        typedef boost::filter_iterator<decltype(dead_skipper)*, strong_iterator> iterator;

        iterator begin() { return make_iterator(mData.begin()); }
        iterator end()   { return make_iterator(mData.end()); }

    private:
        iterator make_iterator(typename VectorType::iterator iter)
        {
            // transform_iterator only needs the base iterator and the transform.
            // filter_iterator wants the predicate and both ends of the range.
            return iterator(dead_skipper,
                            strong_iterator(iter, strengthen),
                            strong_iterator(mData.end(), strengthen));
        }

        LockStatic mLock;           // lock static data during construction
        VectorType mData;
    };

    // iterate over this for references to each instance
    class instance_snapshot: public snapshot
    {
    private:
        static T& instance_getter(typename snapshot::iterator::reference pair)
        {
            return *pair.second;
        }
    public:
        typedef boost::transform_iterator<decltype(instance_getter)*,
                                          typename snapshot::iterator> iterator;
        iterator begin() { return iterator(snapshot::begin(), instance_getter); }
        iterator end()   { return iterator(snapshot::end(),   instance_getter); }

        void deleteAll()
        {
            for (auto it(snapshot::begin()), end(snapshot::end()); it != end; ++it)
            {
                delete it->second.get();
            }
        }
    };                   

    // iterate over this for each key
    class key_snapshot: public snapshot
    {
    private:
        static KEY key_getter(typename snapshot::iterator::reference pair)
        {
            return pair.first;
        }
    public:
        typedef boost::transform_iterator<decltype(key_getter)*,
                                          typename snapshot::iterator> iterator;
        iterator begin() { return iterator(snapshot::begin(), key_getter); }
        iterator end()   { return iterator(snapshot::end(),   key_getter); }
    };

    static T* getInstance(const KEY& k)
    {
        LockStatic lock;
        const InstanceMap& map(lock->mMap);
        typename InstanceMap::const_iterator found = map.find(k);
        return (found == map.end()) ? NULL : found->second.get();
    }

    static S32 instanceCount() 
    { 
        return LockStatic()->mMap.size(); 
    }

protected:
    LLInstanceTracker(const KEY& key) 
    {
        // We do not intend to manage the lifespan of this object with
        // shared_ptr, so give it a no-op deleter. We store shared_ptrs in our
        // InstanceMap specifically so snapshot can store weak_ptrs so we can
        // detect deletions during traversals.
        std::shared_ptr<T> ptr(static_cast<T*>(this), [](T*){});
        LockStatic lock;
        add_(lock, key, ptr);
    }
public:
    virtual ~LLInstanceTracker()
    {
        LockStatic lock;
        remove_(lock);
    }
protected:
    virtual void setKey(KEY key)
    {
        LockStatic lock;
        // Even though the shared_ptr we store in our map has a no-op deleter
        // for T itself, letting the use count decrement to 0 will still
        // delete the use-count object. Capture the shared_ptr we just removed
        // and re-add it to the map with the new key.
        auto ptr = remove_(lock);
        add_(lock, key, ptr);
    }
public:
    virtual const KEY& getKey() const { return mInstanceKey; }

private:
    LLInstanceTracker( const LLInstanceTracker& ) = delete;
    LLInstanceTracker& operator=( const LLInstanceTracker& ) = delete;

    // for logging
    template <typename K>
    static K report(K key) { return key; }
    static std::string report(const std::string& key) { return "'" + key + "'"; }
    static std::string report(const char* key) { return report(std::string(key)); }

    // caller must instantiate LockStatic
    void add_(LockStatic& lock, const KEY& key, const std::shared_ptr<T>& ptr) 
    { 
        mInstanceKey = key; 
        InstanceMap& map = lock->mMap;
        switch(KEY_COLLISION_BEHAVIOR)
        {
        case LLInstanceTrackerErrorOnCollision:
        {
            // map stores shared_ptr to self
            auto pair = map.emplace(key, ptr);
            if (! pair.second)
            {
                LL_ERRS("LLInstanceTracker") << "Instance with key " << report(key)
                                             << " already exists!" << LL_ENDL;
            }
            break;
        }
        case LLInstanceTrackerReplaceOnCollision:
            map[key] = ptr;
            break;
        default:
            break;
        }
    }
    std::shared_ptr<T> remove_(LockStatic& lock)
    {
        InstanceMap& map = lock->mMap;
        typename InstanceMap::iterator iter = map.find(mInstanceKey);
        if (iter != map.end())
        {
            auto ret = iter->second;
            map.erase(iter);
            return ret;
        }
        return {};
    }

private:
    KEY mInstanceKey;
};

/*****************************************************************************
*   LLInstanceTracker without key
*****************************************************************************/
namespace LLInstanceTrackerStuff
{
    template <typename VALUE>
    struct StaticSet: public StaticBase
    {
        typedef std::set<VALUE> InstanceSet;
        InstanceSet mSet;
    };
} // LLInstanceTrackerStuff

// TODO:
// - For the case of omitted KEY template parameter, consider storing
//   std::map<T*, std::shared_ptr<T>> instead of std::set<std::shared_ptr<T>>.
//   That might let us share more of the implementation between KEY and
//   non-KEY LLInstanceTracker subclasses.
// - Even if not that, consider trying to unify the snapshot implementations.
//   The trouble is that the 'iterator' published by each (and by their
//   subclasses) must reflect the specific type of the callables that
//   distinguish them. (Maybe make instance_snapshot() and key_snapshot()
//   factory functions that pass lambdas to a factory function for the generic
//   template class?)

/// explicit specialization for default case where KEY is void
/// use a simple std::set<T*>
template<typename T, EInstanceTrackerAllowKeyCollisions KEY_COLLISION_BEHAVIOR>
class LLInstanceTracker<T, void, KEY_COLLISION_BEHAVIOR> :
    public LLInstanceTrackerBase<LLInstanceTrackerStuff::StaticSet<std::shared_ptr<T>>>
{
    typedef LLInstanceTrackerBase<LLInstanceTrackerStuff::StaticSet<std::shared_ptr<T>>> super;
    using typename super::StaticData;
    using typename super::LockStatic;
    typedef typename StaticData::InstanceSet InstanceSet;

public:
    /**
     * Storing a dumb T* somewhere external is a bad idea, since
     * LLInstanceTracker subclasses are explicitly destroyed rather than
     * managed by smart pointers. It's legal to declare stack instances of an
     * LLInstanceTracker subclass. But it's reasonable to store a
     * std::weak_ptr<T>, which will become invalid when the T instance is
     * destroyed.
     */
    std::weak_ptr<T> getWeak()
    {
        return mSelf;
    }
    
    static S32 instanceCount() { return LockStatic()->mSet.size(); }

    // snapshot of std::shared_ptr<T> pointers
    class snapshot
    {
        // It's very important that what we store in this snapshot are
        // weak_ptrs, NOT shared_ptrs. That's how we discover whether any
        // instance has been deleted during the lifespan of a snapshot.
        typedef std::vector<std::weak_ptr<T>> VectorType;
        // Dereferencing our iterator produces a std::shared_ptr for each
        // instance that still exists. Since we store weak_ptrs, that involves
        // two chained transformations:
        // - a transform_iterator to lock the weak_ptr and return a shared_ptr
        // - a filter_iterator to skip any shared_ptr that has become invalid.
        typedef std::shared_ptr<T> strong_ptr;
        static strong_ptr strengthen(typename VectorType::value_type& ptr)
        {
            return ptr.lock();
        }
        static bool dead_skipper(const strong_ptr& ptr)
        {
            return bool(ptr);
        }

    public:
        snapshot():
            // populate our vector with a snapshot of (locked!) InstanceSet
            // note, this assigns stored shared_ptrs to weak_ptrs for snapshot
            mData(mLock->mSet.begin(), mLock->mSet.end())
        {
            // release the lock once we've populated mData
            mLock.unlock();
        }

        typedef boost::transform_iterator<decltype(strengthen)*,
                                          typename VectorType::iterator> strong_iterator;
        typedef boost::filter_iterator<decltype(dead_skipper)*, strong_iterator> iterator;

        iterator begin() { return make_iterator(mData.begin()); }
        iterator end()   { return make_iterator(mData.end()); }

    private:
        iterator make_iterator(typename VectorType::iterator iter)
        {
            // transform_iterator only needs the base iterator and the transform.
            // filter_iterator wants the predicate and both ends of the range.
            return iterator(dead_skipper,
                            strong_iterator(iter, strengthen),
                            strong_iterator(mData.end(), strengthen));
        }

        LockStatic mLock;           // lock static data during construction
        VectorType mData;
    };

    // iterate over this for references to each instance
    struct instance_snapshot: public snapshot
    {
        typedef boost::indirect_iterator<typename snapshot::iterator> iterator;
        iterator begin() { return iterator(snapshot::begin()); }
        iterator end()   { return iterator(snapshot::end()); }

        void deleteAll()
        {
            for (auto it(snapshot::begin()), end(snapshot::end()); it != end; ++it)
            {
                delete it->get();
            }
        }
    };

protected:
    LLInstanceTracker()
    {
        // Since we do not intend for this shared_ptr to manage lifespan, give
        // it a no-op deleter.
        std::shared_ptr<T> ptr(static_cast<T*>(this), [](T*){});
        // save corresponding weak_ptr for future reference
        mSelf = ptr;
        // Also store it in our class-static set to track this instance.
        LockStatic()->mSet.emplace(ptr);
    }
public:
    virtual ~LLInstanceTracker()
    {
        // convert weak_ptr to shared_ptr because that's what we store in our
        // InstanceSet
        LockStatic()->mSet.erase(mSelf.lock());
    }
protected:
    LLInstanceTracker(const LLInstanceTracker& other):
        LLInstanceTracker()
    {}

private:
    // Storing a weak_ptr to self is a bit like deriving from
    // std::enable_shared_from_this(), except more explicit.
    std::weak_ptr<T> mSelf;
};

#endif
