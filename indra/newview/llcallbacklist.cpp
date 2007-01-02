/** 
 * @file llcallbacklist.cpp
 * @brief A simple list of callback functions to call.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llcallbacklist.h"

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
