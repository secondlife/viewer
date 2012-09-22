/** 
 * @file llthreadlocalptr.h
 * @brief manage thread local storage through non-copyable pointer
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_LLTHREAD_LOCAL_PTR_H
#define LL_LLTHREAD_LOCAL_PTR_H

#include "llapr.h"

template <typename T>
class LLThreadLocalPtr
{
public:
	LLThreadLocalPtr(T* value = NULL, apr_pool_t* pool = NULL)
	{
		apr_status_t result = apr_threadkey_private_create(&mThreadKey, cleanup, pool);
		if (result != APR_SUCCESS)
		{
			ll_apr_warn_status(result);
			llerrs << "Failed to allocate thread local data" << llendl;
		}
		set(value);
	}


	~LLThreadLocalPtr()
	{
		apr_status_t result = apr_threadkey_private_delete(mThreadKey);
		if (result != APR_SUCCESS)
		{
			ll_apr_warn_status(result);
			llerrs << "Failed to delete thread local data" << llendl;
		}
	}

	T* operator -> ()
	{
		return get();
	}

	const T* operator -> () const
	{
		return get();
	}

	T& operator*()
	{
		return *get();
	}

	const T& operator*() const
	{
		return *get();
	}

	LLThreadLocalPtr<T>& operator = (T* value)
	{
		set(value);
		return *this;
	}

	void copyFrom(const LLThreadLocalPtr<T>& other)
	{
		set(other.get());
	}

	LL_FORCE_INLINE void set(T* value)
	{
		apr_status_t result = apr_threadkey_private_set((void*)value, mThreadKey);
		if (result != APR_SUCCESS)
		{
			ll_apr_warn_status(result);
			llerrs << "Failed to set thread local data" << llendl;
		}
	}

	LL_FORCE_INLINE T* get()
	{
		T* ptr;
		//apr_status_t result =
		apr_threadkey_private_get((void**)&ptr, mThreadKey);
		//if (result != APR_SUCCESS)
		//{
		//	ll_apr_warn_status(s);
		//	llerrs << "Failed to get thread local data" << llendl;
		//}
		return ptr;
	}

	LL_FORCE_INLINE const T* get() const
	{
		T* ptr;
		//apr_status_t result =
		apr_threadkey_private_get((void**)&ptr, mThreadKey);
		//if (result != APR_SUCCESS)
		//{
		//	ll_apr_warn_status(s);
		//	llerrs << "Failed to get thread local data" << llendl;
		//}
		return ptr;
	}


private:
	static void cleanup(void* ptr)
	{
		delete reinterpret_cast<T*>(ptr);
	}

	LLThreadLocalPtr(const LLThreadLocalPtr<T>& other)
	{
		// do not copy construct
		llassert(false);
	}

	apr_threadkey_t* mThreadKey;
};

#endif // LL_LLTHREAD_LOCAL_PTR_H
