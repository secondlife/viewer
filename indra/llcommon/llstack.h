/** 
 * @file llstack.h
 * @brief LLStack template class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSTACK_H
#define LL_LLSTACK_H

#include "linked_lists.h"

template <class DATA_TYPE> class LLStack
{
private:
	LLLinkedList<DATA_TYPE> mStack;

public:
	LLStack()	{}
	~LLStack()	{}

	void push(DATA_TYPE *data)	{ mStack.addData(data); }
	DATA_TYPE *pop()			{ DATA_TYPE *tempp = mStack.getFirstData(); mStack.removeCurrentData(); return tempp; }
	void deleteAllData()		{ mStack.deleteAllData(); }
	void removeAllNodes()		{ mStack.removeAllNodes(); }
};

#endif

