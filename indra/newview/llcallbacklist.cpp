/** 
 * @file llcallbacklist.cpp
 * @brief A simple list of callback functions to call.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
