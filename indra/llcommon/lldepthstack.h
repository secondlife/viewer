/** 
 * @file lldepthstack.h
 * @brief Declaration of the LLDepthStack class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDEPTHSTACK_H
#define LL_LLDEPTHSTACK_H

#include "linked_lists.h"

template <class DATA_TYPE> class LLDepthStack
{
private:
	LLLinkedList<DATA_TYPE> mStack;
	U32						mCurrentDepth;
	U32						mMaxDepth;

public:
	LLDepthStack() : mCurrentDepth(0), mMaxDepth(0) {}
	~LLDepthStack()	{}

	void setDepth(U32 depth)
	{
		mMaxDepth = depth;
	}

	U32 getDepth(void) const
	{
		return mCurrentDepth;
	}

	void push(DATA_TYPE *data)
	{ 
		if (mCurrentDepth < mMaxDepth)
		{
			mStack.addData(data); 
			mCurrentDepth++;
		}
		else
		{
			// the last item falls off stack and is deleted
			mStack.getLastData();
			mStack.deleteCurrentData();	
			mStack.addData(data);
		}
	}
	
	DATA_TYPE *pop()
	{ 
		DATA_TYPE *tempp = mStack.getFirstData(); 
		if (tempp)
		{
			mStack.removeCurrentData(); 
			mCurrentDepth--;
		}
		return tempp; 
	}
	
	DATA_TYPE *check()
	{ 
		DATA_TYPE *tempp = mStack.getFirstData(); 
		return tempp; 
	}
	
	void deleteAllData()
	{ 
		mCurrentDepth = 0;
		mStack.deleteAllData(); 
	}
	
	void removeAllNodes()
	{ 
		mCurrentDepth = 0;
		mStack.removeAllNodes(); 
	}
};

#endif
