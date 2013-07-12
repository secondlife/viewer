/** 
 * @file llcallbacklist.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llcallbacklist.h"
#include "lleventtimer.h"

// Library includes
#include "llerror.h"


//
// Globals
//
LLCallbackList gIdleCallbacks;

//
// Member functions
//

LLCallbackList::LLCallbackList()
{
	// nothing
}

LLCallbackList::~LLCallbackList()
{
}


void LLCallbackList::addFunction( callback_t func, void *data)
{
	if (!func)
	{
		llerrs << "LLCallbackList::addFunction - function is NULL" << llendl;
		return;
	}

	// only add one callback per func/data pair
	callback_pair_t t(func, data);
	callback_list_t::iterator iter = std::find(mCallbackList.begin(), mCallbackList.end(), t);
	if (iter == mCallbackList.end())
	{
		mCallbackList.push_back(t);
	}
}


BOOL LLCallbackList::containsFunction( callback_t func, void *data)
{
	callback_pair_t t(func, data);
	callback_list_t::iterator iter = std::find(mCallbackList.begin(), mCallbackList.end(), t);
	if (iter != mCallbackList.end())
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


BOOL LLCallbackList::deleteFunction( callback_t func, void *data)
{
	callback_pair_t t(func, data);
	callback_list_t::iterator iter = std::find(mCallbackList.begin(), mCallbackList.end(), t);
	if (iter != mCallbackList.end())
	{
		mCallbackList.erase(iter);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


void LLCallbackList::deleteAllFunctions()
{
	mCallbackList.clear();
}


void LLCallbackList::callFunctions()
{
	for (callback_list_t::iterator iter = mCallbackList.begin(); iter != mCallbackList.end();  )
	{
		callback_list_t::iterator curiter = iter++;
		curiter->first(curiter->second);
	}
}

// Shim class to allow arbitrary boost::bind
// expressions to be run as one-time idle callbacks.
class OnIdleCallbackOneTime
{
public:
	OnIdleCallbackOneTime(nullary_func_t callable):
		mCallable(callable)
	{
	}
	static void onIdle(void *data)
	{
		gIdleCallbacks.deleteFunction(onIdle, data);
		OnIdleCallbackOneTime* self = reinterpret_cast<OnIdleCallbackOneTime*>(data);
		self->call();
		delete self;
	}
	void call()
	{
		mCallable();
	}
private:
	nullary_func_t mCallable;
};

void doOnIdleOneTime(nullary_func_t callable)
{
	OnIdleCallbackOneTime* cb_functor = new OnIdleCallbackOneTime(callable);
	gIdleCallbacks.addFunction(&OnIdleCallbackOneTime::onIdle,cb_functor);
}

// Shim class to allow generic boost functions to be run as
// recurring idle callbacks.  Callable should return true when done,
// false to continue getting called.
class OnIdleCallbackRepeating
{
public:
	OnIdleCallbackRepeating(bool_func_t callable):
		mCallable(callable)
	{
	}
	// Will keep getting called until the callable returns true.
	static void onIdle(void *data)
	{
		OnIdleCallbackRepeating* self = reinterpret_cast<OnIdleCallbackRepeating*>(data);
		bool done = self->call();
		if (done)
		{
			gIdleCallbacks.deleteFunction(onIdle, data);
			delete self;
		}
	}
	bool call()
	{
		return mCallable();
	}
private:
	bool_func_t mCallable;
};

void doOnIdleRepeating(bool_func_t callable)
{
	OnIdleCallbackRepeating* cb_functor = new OnIdleCallbackRepeating(callable);
	gIdleCallbacks.addFunction(&OnIdleCallbackRepeating::onIdle,cb_functor);
}

class NullaryFuncEventTimer: public LLEventTimer
{
public:
	NullaryFuncEventTimer(nullary_func_t callable, F32 seconds):
		LLEventTimer(seconds),
		mCallable(callable)
	{
	}

private:
	BOOL tick()
	{
		mCallable();
		return TRUE;
	}

	nullary_func_t mCallable;
};

// Call a given callable once after specified interval.
void doAfterInterval(nullary_func_t callable, F32 seconds)
{
	new NullaryFuncEventTimer(callable, seconds);
}

class BoolFuncEventTimer: public LLEventTimer
{
public:
	BoolFuncEventTimer(bool_func_t callable, F32 seconds):
		LLEventTimer(seconds),
		mCallable(callable)
	{
	}
private:
	BOOL tick()
	{
		return mCallable();
	}

	bool_func_t mCallable;
};

// Call a given callable every specified number of seconds, until it returns true.
void doPeriodically(bool_func_t callable, F32 seconds)
{
	new BoolFuncEventTimer(callable, seconds);
}

#ifdef _DEBUG

void test1(void *data)
{
	S32 *s32_data = (S32 *)data;
	llinfos << "testfunc1 " << *s32_data << llendl;
}


void test2(void *data)
{
	S32 *s32_data = (S32 *)data;
	llinfos << "testfunc2 " << *s32_data << llendl;
}


void
LLCallbackList::test()
{
	S32 a = 1;
	S32 b = 2;
	LLCallbackList *list = new LLCallbackList;

	llinfos << "Testing LLCallbackList" << llendl;

	if (!list->deleteFunction(NULL))
	{
		llinfos << "passed 1" << llendl;
	}
	else
	{
		llinfos << "error, removed function from empty list" << llendl;
	}

	// llinfos << "This should crash" << llendl;
	// list->addFunction(NULL);

	list->addFunction(&test1, &a);
	list->addFunction(&test1, &a);

	llinfos << "Expect: test1 1, test1 1" << llendl;
	list->callFunctions();

	list->addFunction(&test1, &b);
	list->addFunction(&test2, &b);

	llinfos << "Expect: test1 1, test1 1, test1 2, test2 2" << llendl;
	list->callFunctions();

	if (list->deleteFunction(&test1, &b))
	{
		llinfos << "passed 3" << llendl;
	}
	else
	{
		llinfos << "error removing function" << llendl;
	}

	llinfos << "Expect: test1 1, test1 1, test2 2" << llendl;
	list->callFunctions();

	list->deleteAllFunctions();

	llinfos << "Expect nothing" << llendl;
	list->callFunctions();

	llinfos << "nothing :-)" << llendl;

	delete list;

	llinfos << "test complete" << llendl;
}

#endif  // _DEBUG
