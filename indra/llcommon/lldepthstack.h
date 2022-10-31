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

#include "llstl.h"

template <class DATA_TYPE> class LLDepthStack
{
private:
    std::deque<DATA_TYPE*>  mStack;
    U32                     mCurrentDepth;
    U32                     mMaxDepth;

public:
    LLDepthStack() 
    :   mCurrentDepth(0), mMaxDepth(0) 
    {}

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
            mStack.push_back(data); 
            mCurrentDepth++;
        }
        else
        {
            // the last item falls off stack and is deleted
            if (!mStack.empty())
            {
                mStack.pop_front();
            }
            mStack.push_back(data);
        }
    }
    
    DATA_TYPE *pop()
    { 
        DATA_TYPE *tempp = NULL;
        if (!mStack.empty())
        {
            tempp = mStack.back();
            mStack.pop_back(); 
            mCurrentDepth--;
        }
        return tempp; 
    }
    
    DATA_TYPE *check()
    { 
        return mStack.empty() ? NULL : mStack.back();
    }

    void removeAllNodes()
    { 
        mCurrentDepth = 0;
        mStack.clear(); 
    }
};

#endif
