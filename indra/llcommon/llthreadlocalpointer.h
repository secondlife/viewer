/** 
 * @file llthreadlocalpointer.h
 * @author Richard
 * @brief Pointer class that manages a distinct value per thread
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLTHREADLOCALPOINTER_H
#define LL_LLTHREADLOCALPOINTER_H

#include "llinstancetracker.h"
#include "llapr.h"

class LLThreadLocalPointerBase : public LLInstanceTracker<LLThreadLocalPointerBase>
{
public:
	LLThreadLocalPointerBase()
	:	mThreadKey(NULL)
	{
		if (sInitialized)
		{
			initStorage();
		}
	}

	LLThreadLocalPointerBase( const LLThreadLocalPointerBase& other)
		:	mThreadKey(NULL)
	{
		if (sInitialized)
		{
			initStorage();
		}
	}

	~LLThreadLocalPointerBase()
	{
		destroyStorage();
	}

	static void initAllThreadLocalStorage();
	static void destroyAllThreadLocalStorage();

protected:
	void set(void* value);

	LL_FORCE_INLINE void* get()
	{
		// llassert(sInitialized);
		void* ptr;
		apr_status_t result =
		apr_threadkey_private_get(&ptr, mThreadKey);
		if (result != APR_SUCCESS)
		{
			ll_apr_warn_status(result);
			llerrs << "Failed to get thread local data" << llendl;
		}
		return ptr;
	}

	LL_FORCE_INLINE const void* get() const
	{
		void* ptr;
		apr_status_t result =
		apr_threadkey_private_get(&ptr, mThreadKey);
		if (result != APR_SUCCESS)
		{
			ll_apr_warn_status(result);
			llerrs << "Failed to get thread local data" << llendl;
		}
		return ptr;
	}

	void initStorage();
	void destroyStorage();

protected:
	apr_threadkey_t* mThreadKey;
	static bool		sInitialized;
};

template <typename T>
class LLThreadLocalPointer : public LLThreadLocalPointerBase
{
public:

	LLThreadLocalPointer()
	{}

	explicit LLThreadLocalPointer(T* value)
	{
		set(value);
	}


	LLThreadLocalPointer(const LLThreadLocalPointer<T>& other)
	:	LLThreadLocalPointerBase(other)
	{
		set(other.get());		
	}

	LL_FORCE_INLINE T* get()
	{
		return (T*)LLThreadLocalPointerBase::get();
	}

	const T* get() const
	{
		return (const T*)LLThreadLocalPointerBase::get();
	}

	T* operator -> ()
	{
		return (T*)get();
	}

	const T* operator -> () const
	{
		return (T*)get();
	}

	T& operator*()
	{
		return *(T*)get();
	}

	const T& operator*() const
	{
		return *(T*)get();
	}

	LLThreadLocalPointer<T>& operator = (T* value)
	{
		set((void*)value);
		return *this;
	}

	bool operator ==(T* other)
	{
		if (!sInitialized) return false;
		return get() == other;
	}
};

#endif // LL_LLTHREADLOCALPOINTER_H
