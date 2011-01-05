/** 
 * @file llrefcount.h
 * @brief Base class for reference counted objects for use with LLPointer
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#ifndef LLREFCOUNT_H
#define LLREFCOUNT_H

#include <boost/noncopyable.hpp>

#define LL_REF_COUNT_DEBUG 0
#if LL_REF_COUNT_DEBUG
class LLMutex ;
#endif

//----------------------------------------------------------------------------
// RefCount objects should generally only be accessed by way of LLPointer<>'s
// see llthread.h for LLThreadSafeRefCount
//----------------------------------------------------------------------------

class LL_COMMON_API LLRefCount
{
protected:
	LLRefCount(const LLRefCount& other);
	LLRefCount& operator=(const LLRefCount&);
	virtual ~LLRefCount(); // use unref()
	
public:
	LLRefCount();

#if LL_REF_COUNT_DEBUG
	void ref() const ;
	S32 unref() const ;
#else
	void LLRefCount::ref() const
	{ 
		mRef++; 
	} 

	S32 LLRefCount::unref() const
	{
		llassert(mRef >= 1);
		if (0 == --mRef) 
		{
			delete this; 
			return 0;
		}
		return mRef;
	}	
#endif

	//NOTE: when passing around a const LLRefCount object, this can return different results
	// at different types, since mRef is mutable
	S32 getNumRefs() const
	{
		return mRef;
	}

private: 
	mutable S32	mRef; 

#if LL_REF_COUNT_DEBUG
	LLMutex*  mMutexp ;
	mutable U32  mLockedThreadID ;
	mutable BOOL mCrashAtUnlock ; 
#endif
};

#endif
