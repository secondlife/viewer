/** 
 * @file llcallbacklist.h
 * @brief A simple list of callback functions to call.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

extern LLCallbackList gIdleCallbacks;

#endif
