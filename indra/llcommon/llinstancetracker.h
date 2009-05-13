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

// This mix-in class adds support for tracking all instances of the specified class parameter T
// The (optional) key associates a value of type KEY with a given instance of T, for quick lookup
// If KEY is not provided, then instances are stored in a simple set
template<typename T, typename KEY = T*>
class LLInstanceTracker : boost::noncopyable
{
public:
	typedef typename std::map<KEY, T*>::iterator instance_iter;
	typedef typename std::map<KEY, T*>::const_iterator instance_const_iter;

	static T* getInstance(const KEY& k) { instance_iter found = getMap().find(k); return (found == getMap().end()) ? NULL : found->second; }

	static instance_iter beginInstances() { return getMap().begin(); }
	static instance_iter endInstances() { return getMap().end(); }
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

    static std::map<KEY, T*>& getMap()
    {
        if (! sInstances)
        {
            sInstances = new std::map<KEY, T*>;
        }
        return *sInstances;
    }

private:

	KEY mKey;
	static std::map<KEY, T*>* sInstances;
};

template<typename T>
class LLInstanceTracker<T, T*>
{
public:
	typedef typename std::set<T*>::iterator instance_iter;
	typedef typename std::set<T*>::const_iterator instance_const_iter;

	static instance_iter instancesBegin() { return getSet().begin(); }
	static instance_iter instancesEnd() { return getSet().end(); }
	static S32 instanceCount() { return getSet().size(); }

protected:
	LLInstanceTracker() { getSet().insert(static_cast<T*>(this)); }
	virtual ~LLInstanceTracker() { getSet().erase(static_cast<T*>(this)); }

	LLInstanceTracker(const LLInstanceTracker& other) { getSet().insert(static_cast<T*>(this)); }

    static std::set<T*>& getSet()   // called after getReady() but before go()
    {
        if (! sInstances)
        {
            sInstances = new std::set<T*>;
        }
        return *sInstances;
    }

	static std::set<T*>* sInstances;
};

template <typename T, typename KEY> std::map<KEY, T*>* LLInstanceTracker<T, KEY>::sInstances = NULL;
template <typename T> std::set<T*>* LLInstanceTracker<T, T*>::sInstances = NULL;

#endif
