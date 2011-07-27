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

#include "string_table.h"
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/indirect_iterator.hpp>

class LL_COMMON_API LLInstanceTrackerBase : public boost::noncopyable
{
	protected:
		static void * & getInstances(std::type_info const & info);
};

/// This mix-in class adds support for tracking all instances of the specified class parameter T
/// The (optional) key associates a value of type KEY with a given instance of T, for quick lookup
/// If KEY is not provided, then instances are stored in a simple set
/// @NOTE: see explicit specialization below for default KEY==T* case
template<typename T, typename KEY = T*>
class LLInstanceTracker : public LLInstanceTrackerBase
{
	typedef typename std::map<KEY, T*> InstanceMap;
	typedef LLInstanceTracker<T, KEY> MyT;
public:
	class instance_iter : public boost::iterator_facade<instance_iter, T, boost::forward_traversal_tag>
	{
	public:
		typedef boost::iterator_facade<instance_iter, T, boost::forward_traversal_tag> super_t;
		
		instance_iter(typename const InstanceMap::iterator& it)
		:	mIterator(it)
		{
			++sIterationNestDepth;
		}

		~instance_iter()
		{
			--sIterationNestDepth;
		}


	private:
		friend class boost::iterator_core_access;

		void increment() { mIterator++; }
		bool equal(instance_iter const& other) const
		{
			return mIterator == other.mIterator;
		}

		T& dereference() const
		{
			return *(mIterator->second);
		}

		typename InstanceMap::iterator mIterator;
	};

	class key_iter : public boost::iterator_facade<key_iter, KEY, boost::forward_traversal_tag>
	{
	public:
		typedef boost::iterator_facade<key_iter, KEY, boost::forward_traversal_tag> super_t;

		key_iter(typename InstanceMap::iterator& it)
			:	mIterator(it)
		{
			++sIterationNestDepth;
		}

		key_iter(const key_iter& other)
			:	mIterator(other.mIterator)
		{
			++sIterationNestDepth;
		}

		~key_iter()
		{
			--sIterationNestDepth;
		}


	private:
		friend class boost::iterator_core_access;

		void increment() { mIterator++; }
		bool equal(key_iter const& other) const
		{
			return mIterator == other.mIterator;
		}

		KEY& dereference() const
		{
			return const_cast<KEY&>(mIterator->first);
		}

		typename InstanceMap::iterator mIterator;
	};

	static T* getInstance(const KEY& k)
	{
		typename InstanceMap::const_iterator found = getMap_().find(k);
		return (found == getMap_().end()) ? NULL : found->second;
	}

	static instance_iter beginInstances() 
	{	
		return instance_iter(getMap_().begin()); 
	}

	static instance_iter endInstances() 
	{
		return instance_iter(getMap_().end());
	}

	static S32 instanceCount() { return getMap_().size(); }

	static key_iter beginKeys()
	{
		return key_iter(getMap_().begin());
	}
	static key_iter endKeys()
	{
		return key_iter(getMap_().end());
	}

protected:
	LLInstanceTracker(KEY key) { add_(key); }
	virtual ~LLInstanceTracker() 
	{ 
		// it's unsafe to delete instances of this type while all instances are being iterated over.
		llassert(sIterationNestDepth == 0);
		remove_(); 		
	}
	virtual void setKey(KEY key) { remove_(); add_(key); }
	virtual const KEY& getKey() const { return mInstanceKey; }

private:
	void add_(KEY key) 
	{ 
		mInstanceKey = key; 
		getMap_()[key] = static_cast<T*>(this); 
	}
	void remove_()
	{
		getMap_().erase(mInstanceKey);
	}

    static InstanceMap& getMap_()
    {
		void * & instances = getInstances(typeid(MyT));
        if (! instances)
        {
            instances = new InstanceMap;
        }
        return * static_cast<InstanceMap*>(instances);
    }

private:

	KEY mInstanceKey;
	static S32 sIterationNestDepth;
};

template <typename T, typename KEY> S32 LLInstanceTracker<T, KEY>::sIterationNestDepth = 0;

/// explicit specialization for default case where KEY is T*
/// use a simple std::set<T*>
template<typename T>
class LLInstanceTracker<T, T*> : public LLInstanceTrackerBase
{
	typedef typename std::set<T*> InstanceSet;
	typedef LLInstanceTracker<T, T*> MyT;
public:

	/// for completeness of analogy with the generic implementation
	static T* getInstance(T* k) { return k; }
	static S32 instanceCount() { return getSet_().size(); }

	class instance_iter : public boost::iterator_facade<instance_iter, T, boost::forward_traversal_tag>
	{
	public:
		instance_iter(typename const InstanceSet::iterator& it)
		:	mIterator(it)
		{
			++sIterationNestDepth;
		}

		instance_iter(const instance_iter& other)
		:	mIterator(other.mIterator)
		{
			++sIterationNestDepth;
		}

		~instance_iter()
		{
			--sIterationNestDepth;
		}

	private:
		friend class boost::iterator_core_access;

		void increment() { mIterator++; }
		bool equal(instance_iter const& other) const
		{
			return mIterator == other.mIterator;
		}

		T& dereference() const
		{
			return **mIterator;
		}

		typename InstanceSet::iterator mIterator;
	};

	static instance_iter beginInstances() {	return instance_iter(getSet_().begin()); }
	static instance_iter endInstances() { return instance_iter(getSet_().end()); }

protected:
	LLInstanceTracker()
	{
		// it's safe but unpredictable to create instances of this type while all instances are being iterated over.  I hate unpredictable.  This assert will probably be turned on early in the next development cycle.
		getSet_().insert(static_cast<T*>(this));
	}
	virtual ~LLInstanceTracker()
	{
		// it's unsafe to delete instances of this type while all instances are being iterated over.
		llassert(sIterationNestDepth == 0);
		getSet_().erase(static_cast<T*>(this));
	}

	LLInstanceTracker(const LLInstanceTracker& other)
	{
		getSet_().insert(static_cast<T*>(this));
	}

	static InstanceSet& getSet_()
	{
		void * & instances = getInstances(typeid(MyT));
		if (! instances)
		{
			instances = new InstanceSet;
		}
		return * static_cast<InstanceSet *>(instances);
	}

	static S32 sIterationNestDepth;
};

template <typename T> S32 LLInstanceTracker<T, T*>::sIterationNestDepth = 0;

#endif
