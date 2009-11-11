/** 
 * @file llinstancetracker.h
 * @brief LLInstanceTracker is a mixin class that automatically tracks object
 *        instances with or without an associated key
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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

/// This mix-in class adds support for tracking all instances of the specified class parameter T
/// The (optional) key associates a value of type KEY with a given instance of T, for quick lookup
/// If KEY is not provided, then instances are stored in a simple set
/// @NOTE: see explicit specialization below for default KEY==T* case
template<typename T, typename KEY = T*>
class LLInstanceTracker : boost::noncopyable
{
	typedef typename std::map<KEY, T*> InstanceMap;
	typedef boost::function<const KEY&(typename InstanceMap::value_type&)> KeyGetter;
	typedef boost::function<T*(typename InstanceMap::value_type&)> InstancePtrGetter;
public:
	/// Dereferencing key_iter gives you a const KEY&
	typedef boost::transform_iterator<KeyGetter, typename InstanceMap::iterator> key_iter;
	/// Dereferencing instance_iter gives you a T&
	typedef boost::indirect_iterator< boost::transform_iterator<InstancePtrGetter, typename InstanceMap::iterator> > instance_iter;

	static T* getInstance(const KEY& k)
	{
		typename InstanceMap::const_iterator found = getMap().find(k);
		return (found == getMap().end()) ? NULL : found->second;
	}

	static key_iter beginKeys()
	{
		return boost::make_transform_iterator(getMap().begin(),
											  boost::bind(&InstanceMap::value_type::first, _1));
	}
	static key_iter endKeys()
	{
		return boost::make_transform_iterator(getMap().end(),
											  boost::bind(&InstanceMap::value_type::first, _1));
	}
	static instance_iter beginInstances()
	{
		return instance_iter(boost::make_transform_iterator(getMap().begin(),
															boost::bind(&InstanceMap::value_type::second, _1)));
	}
	static instance_iter endInstances()
	{
		return instance_iter(boost::make_transform_iterator(getMap().end(),
															boost::bind(&InstanceMap::value_type::second, _1)));
	}
	static S32 instanceCount() { return getMap().size(); }
protected:
	LLInstanceTracker(KEY key) { add(key); }
	virtual ~LLInstanceTracker() { remove(); }
	virtual void setKey(KEY key) { remove(); add(key); }
	virtual const KEY& getKey() const { return mKey; }

private:
	void add(KEY key) 
	{ 
		mKey = key; 
		getMap()[key] = static_cast<T*>(this); 
	}
	void remove() { getMap().erase(mKey); }

    static InstanceMap& getMap()
    {
        if (! sInstances)
        {
            sInstances = new InstanceMap;
        }
        return *sInstances;
    }

private:

	KEY mKey;
	static InstanceMap* sInstances;
};

/// explicit specialization for default case where KEY is T*
/// use a simple std::set<T*>
template<typename T>
class LLInstanceTracker<T, T*>
{
	typedef typename std::set<T*> InstanceSet;
public:
	/// Dereferencing key_iter gives you a T* (since T* is the key)
	typedef typename InstanceSet::iterator key_iter;
	/// Dereferencing instance_iter gives you a T&
	typedef boost::indirect_iterator<key_iter> instance_iter;

	/// for completeness of analogy with the generic implementation
	static T* getInstance(T* k) { return k; }
	static key_iter beginKeys() { return getSet().begin(); }
	static key_iter endKeys()   { return getSet().end(); }
	static instance_iter beginInstances() { return instance_iter(getSet().begin()); }
	static instance_iter endInstances()   { return instance_iter(getSet().end()); }
	static S32 instanceCount() { return getSet().size(); }

protected:
	LLInstanceTracker() { getSet().insert(static_cast<T*>(this)); }
	virtual ~LLInstanceTracker() { getSet().erase(static_cast<T*>(this)); }

	LLInstanceTracker(const LLInstanceTracker& other) { getSet().insert(static_cast<T*>(this)); }

    static InstanceSet& getSet()   // called after getReady() but before go()
    {
        if (! sInstances)
        {
            sInstances = new InstanceSet;
        }
        return *sInstances;
    }

	static InstanceSet* sInstances;
};

template <typename T, typename KEY> typename LLInstanceTracker<T, KEY>::InstanceMap* LLInstanceTracker<T, KEY>::sInstances = NULL;
template <typename T> typename LLInstanceTracker<T, T*>::InstanceSet* LLInstanceTracker<T, T*>::sInstances = NULL;

#endif
