/** 
 * @file llcallbacklist.h
 * @brief A simple list of callback functions to call.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLCALLBACKLIST_H
#define LL_LLCALLBACKLIST_H

#include "llstl.h"

class LLCallbackList
{
public:
	typedef void (*callback_t)(void*);
	
	LLCallbackList();
	~LLCallbackList();

	void addFunction( callback_t func, void *data = NULL );		// register a callback, which will be called as func(data)
	BOOL containsFunction( callback_t func, void *data = NULL );	// true if list already contains the function/data pair
	BOOL deleteFunction( callback_t func, void *data = NULL );		// removes the first instance of this function/data pair from the list, false if not found
	void callFunctions();												// calls all functions
	void deleteAllFunctions();

	static void test();

protected:
	// Use a list so that the callbacks are ordered in case that matters
	typedef std::pair<callback_t,void*> callback_pair_t;
	typedef std::list<callback_pair_t > callback_list_t;
	callback_list_t	mCallbackList;
};

typedef boost::function<void ()> nullary_func_t;
typedef boost::function<bool ()> bool_func_t;

// Call a given callable once in idle loop.
void doOnIdleOneTime(nullary_func_t callable);

// Repeatedly call a callable in idle loop until it returns true.
void doOnIdleRepeating(bool_func_t callable);

// Call a given callable once after specified interval.
void doAfterInterval(nullary_func_t callable, F32 seconds);

// Call a given callable every specified number of seconds, until it returns true.
void doPeriodically(bool_func_t callable, F32 seconds);

extern LLCallbackList gIdleCallbacks;

#endif
