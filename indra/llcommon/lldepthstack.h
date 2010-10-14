/** 
 * @file lldepthstack.h
 * @brief Declaration of the LLDepthStack class
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
