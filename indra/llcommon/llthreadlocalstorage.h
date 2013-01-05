/** 
 * @file llthreadlocalstorage.h
 * @author Richard
 * @brief Class wrappers for thread local storage
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

#ifndef LL_LLTHREADLOCALSTORAGE_H
#define LL_LLTHREADLOCALSTORAGE_H

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

	LL_FORCE_INLINE void* get() const
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

	LL_FORCE_INLINE T* get() const
	{
		return (T*)LLThreadLocalPointerBase::get();
	}

	T* operator -> () const
	{
		return (T*)get();
	}

	T& operator*() const
	{
		return *(T*)get();
	}

	LLThreadLocalPointer<T>& operator = (T* value)
	{
		set((void*)value);
		return *this;
	}

	bool operator ==(const T* other) const
	{
		if (!sInitialized) return false;
		return get() == other;
	}
};

template<typename DERIVED_TYPE>
class LLThreadLocalSingleton
{
	typedef enum e_init_state
	{
		UNINITIALIZED = 0,
		CONSTRUCTING,
		INITIALIZING,
		INITIALIZED,
		DELETED
	} EInitState;

public:
	LLThreadLocalSingleton()
	{}
	
	virtual ~LLThreadLocalSingleton()
	{
		sInstance = NULL;
		sInitState = DELETED;
	}

	static void deleteSingleton()
	{
		delete sInstance;
		sInstance = NULL;
		sInitState = DELETED;
	}

	static DERIVED_TYPE* getInstance()
	{
		if (sInitState == CONSTRUCTING)
		{
			llerrs << "Tried to access singleton " << typeid(DERIVED_TYPE).name() << " from singleton constructor!" << llendl;
		}

		if (sInitState == DELETED)
		{
			llwarns << "Trying to access deleted singleton " << typeid(DERIVED_TYPE).name() << " creating new instance" << llendl;
		}

		if (!sInstance) 
		{
			sInitState = CONSTRUCTING;
			sInstance = new DERIVED_TYPE(); 
			sInitState = INITIALIZING;
			sInstance->initSingleton(); 
			sInitState = INITIALIZED;	
		}

		return sInstance;
	}

	static DERIVED_TYPE* getIfExists()
	{
		return sInstance;
	}

	// Reference version of getInstance()
	// Preferred over getInstance() as it disallows checking for NULL
	static DERIVED_TYPE& instance()
	{
		return *getInstance();
	}

	// Has this singleton been created uet?
	// Use this to avoid accessing singletons before the can safely be constructed
	static bool instanceExists()
	{
		return sInitState == INITIALIZED;
	}

	// Has this singleton already been deleted?
	// Use this to avoid accessing singletons from a static object's destructor
	static bool destroyed()
	{
		return sInitState == DELETED;
	}
private:
	LLThreadLocalSingleton(const LLThreadLocalSingleton& other);
	virtual void initSingleton() {}

#ifdef LL_WINDOWS
	static __declspec(thread) DERIVED_TYPE* sInstance;
	static __declspec(thread) EInitState sInitState;
#elif LL_LINUX
	static __thread DERIVED_TYPE* sInstance;
	static __thread EInitState sInitState;
#endif
};

#ifdef LL_WINDOWS
template<typename DERIVED_TYPE>
__declspec(thread) DERIVED_TYPE* LLThreadLocalSingleton<DERIVED_TYPE>::sInstance = NULL;

template<typename DERIVED_TYPE>
__declspec(thread) typename LLThreadLocalSingleton<DERIVED_TYPE>::EInitState LLThreadLocalSingleton<DERIVED_TYPE>::sInitState = LLThreadLocalSingleton<DERIVED_TYPE>::UNINITIALIZED;
#elif LL_LINUX
template<typename DERIVED_TYPE>
__thread DERIVED_TYPE* LLThreadLocalSingleton<DERIVED_TYPE>::sInstance = NULL;

template<typename DERIVED_TYPE>
__thread typename LLThreadLocalSingleton<DERIVED_TYPE>::EInitState LLThreadLocalSingleton<DERIVED_TYPE>::sInitState = LLThreadLocalSingleton<DERIVED_TYPE>::UNINITIALIZED;
#endif

template<typename DERIVED_TYPE>
class LLThreadLocalSingletonPointer
{
public:
	void operator =(DERIVED_TYPE* value)
	{
		sInstance = value;
	}

	LL_FORCE_INLINE static DERIVED_TYPE* getInstance()
	{
		return sInstance;
	}

	LL_FORCE_INLINE static void setInstance(DERIVED_TYPE* instance)
	{
		sInstance = instance;
	}

private:
#ifdef LL_WINDOWS
	static __declspec(thread) DERIVED_TYPE* sInstance;
#elif LL_LINUX
	static __thread DERIVED_TYPE* sInstance;
#endif
};

template<typename DERIVED_TYPE>
#ifdef LL_WINDOWS
__declspec(thread) DERIVED_TYPE* LLThreadLocalSingletonPointer<DERIVED_TYPE>::sInstance = NULL;
#elif LL_LINUX
__thread DERIVED_TYPE* LLThreadLocalSingletonPointer<DERIVED_TYPE>::sInstance = NULL;
#endif

#endif // LL_LLTHREADLOCALSTORAGE_H
