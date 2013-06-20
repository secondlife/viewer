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

	void* get() const;

	void initStorage();
	void destroyStorage();

protected:
	struct apr_threadkey_t*	mThreadKey;
	static bool				sInitialized;
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

	bool isNull() const { return !sInitialized || get() == NULL; }

	bool notNull() const { return sInitialized && get() != NULL; }
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
#if LL_DARWIN
        pthread_setspecific(sInstanceKey, NULL);
#else
        sInstance = NULL;
#endif
		setInitState(DELETED);
	}

	static void deleteSingleton()
	{
		delete getIfExists();
	}

	static DERIVED_TYPE* getInstance()
	{
        EInitState init_state = getInitState();
		if (init_state == CONSTRUCTING)
		{
			llerrs << "Tried to access singleton " << typeid(DERIVED_TYPE).name() << " from singleton constructor!" << llendl;
		}

		if (init_state == DELETED)
		{
			llwarns << "Trying to access deleted singleton " << typeid(DERIVED_TYPE).name() << " creating new instance" << llendl;
		}

#if LL_DARWIN
        createTLSInstance();
#endif
		if (!getIfExists())
		{
			setInitState(CONSTRUCTING);
            DERIVED_TYPE* instancep = new DERIVED_TYPE();
#if LL_DARWIN
            S32 result = pthread_setspecific(sInstanceKey, (void*)instancep);
            if (result != 0)
            {
                llerrs << "Could not set thread local storage" << llendl;
            }
#else
			sInstance = instancep;
#endif
			setInitState(INITIALIZING);
			instancep->initSingleton();
			setInitState(INITIALIZED);
		}

        return getIfExists();
	}

	static DERIVED_TYPE* getIfExists()
	{
#if LL_DARWIN
        return (DERIVED_TYPE*)pthread_getspecific(sInstanceKey);
#else
		return sInstance;
#endif
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
		return getInitState() == INITIALIZED;
	}

	// Has this singleton already been deleted?
	// Use this to avoid accessing singletons from a static object's destructor
	static bool destroyed()
	{
		return getInitState() == DELETED;
	}
private:
#if LL_DARWIN
    static void createTLSInitState()
    {
        static S32 key_created = pthread_key_create(&sInitStateKey, NULL);
        if (key_created != 0)
        {
            llerrs << "Could not create thread local storage" << llendl;
        }
    }
    
    static void createTLSInstance()
    {
        static S32 key_created = pthread_key_create(&sInstanceKey, NULL);
        if (key_created != 0)
        {
            llerrs << "Could not create thread local storage" << llendl;
        }
    }
#endif
    static EInitState getInitState()
    {
#if LL_DARWIN
        createTLSInitState();
        return (EInitState)(int)pthread_getspecific(sInitStateKey);
#else
        return sInitState;
#endif
    }
    
    static void setInitState(EInitState state)
    {
#if LL_DARWIN
        createTLSInitState();
        pthread_setspecific(sInitStateKey, (void*)state);
#else
        sInitState = state;
#endif
    }
	LLThreadLocalSingleton(const LLThreadLocalSingleton& other);
	virtual void initSingleton() {}

#ifdef LL_WINDOWS
	static __declspec(thread) DERIVED_TYPE* sInstance;
	static __declspec(thread) EInitState sInitState;
#elif LL_LINUX
	static __thread DERIVED_TYPE* sInstance;
	static __thread EInitState sInitState;
#elif LL_DARWIN
    static pthread_key_t sInstanceKey;
    static pthread_key_t sInitStateKey;
#endif
};

#if LL_WINDOWS
template<typename DERIVED_TYPE>
__declspec(thread) DERIVED_TYPE* LLThreadLocalSingleton<DERIVED_TYPE>::sInstance = NULL;

template<typename DERIVED_TYPE>
__declspec(thread) typename LLThreadLocalSingleton<DERIVED_TYPE>::EInitState LLThreadLocalSingleton<DERIVED_TYPE>::sInitState = LLThreadLocalSingleton<DERIVED_TYPE>::UNINITIALIZED;
#elif LL_LINUX
template<typename DERIVED_TYPE>
__thread DERIVED_TYPE* LLThreadLocalSingleton<DERIVED_TYPE>::sInstance = NULL;

template<typename DERIVED_TYPE>
__thread typename LLThreadLocalSingleton<DERIVED_TYPE>::EInitState LLThreadLocalSingleton<DERIVED_TYPE>::sInitState = LLThreadLocalSingleton<DERIVED_TYPE>::UNINITIALIZED;
#elif LL_DARWIN
template<typename DERIVED_TYPE>
pthread_key_t LLThreadLocalSingleton<DERIVED_TYPE>::sInstanceKey;

template<typename DERIVED_TYPE>
pthread_key_t LLThreadLocalSingleton<DERIVED_TYPE>::sInitStateKey;

#endif

template<typename DERIVED_TYPE>
class LLThreadLocalSingletonPointer
{
public:
	LL_FORCE_INLINE static DERIVED_TYPE* getInstance()
	{
#if LL_DARWIN
        createTLSKey();
        return (DERIVED_TYPE*)pthread_getspecific(sInstanceKey);
#else
		return sInstance;
#endif
	}

	static void setInstance(DERIVED_TYPE* instance)
	{
#if LL_DARWIN
        createTLSKey();
        pthread_setspecific(sInstanceKey, (void*)instance);
#else
		sInstance = instance;
#endif
	}

private:

#if LL_WINDOWS
	static __declspec(thread) DERIVED_TYPE* sInstance;
#elif LL_LINUX
	static __thread DERIVED_TYPE* sInstance;
#elif LL_DARWIN
    static void TLSError()
    {
        llerrs << "Could not create thread local storage" << llendl;
    }
    static void createTLSKey()
    {
        static S32 key_created = pthread_key_create(&sInstanceKey, NULL);
        if (key_created != 0)
        {
            llerrs << "Could not create thread local storage" << llendl;
        }
    }
    static pthread_key_t sInstanceKey;
#endif
};

#if LL_WINDOWS
template<typename DERIVED_TYPE>
__declspec(thread) DERIVED_TYPE* LLThreadLocalSingletonPointer<DERIVED_TYPE>::sInstance = NULL;
#elif LL_LINUX
template<typename DERIVED_TYPE>
__thread DERIVED_TYPE* LLThreadLocalSingletonPointer<DERIVED_TYPE>::sInstance = NULL;
#elif LL_DARWIN
template<typename DERIVED_TYPE>
pthread_key_t LLThreadLocalSingletonPointer<DERIVED_TYPE>::sInstanceKey;
#endif

#endif // LL_LLTHREADLOCALSTORAGE_H
