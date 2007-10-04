/** 
 * @file lldepthstack.h
 * @brief Declaration of the LLDepthStack class
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
