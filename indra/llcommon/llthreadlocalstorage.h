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
    :   mThreadKey(NULL)
    {
        if (sInitialized)
        {
            initStorage();
        }
    }

    LLThreadLocalPointerBase( const LLThreadLocalPointerBase& other)
    :   mThreadKey(NULL)
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
    struct apr_threadkey_t* mThreadKey;
    static bool             sInitialized;
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
    :   LLThreadLocalPointerBase(other)
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
class LLThreadLocalSingletonPointer
{
public:
    LL_FORCE_INLINE static DERIVED_TYPE* getInstance()
    {
        return sInstance;
    }

    static void setInstance(DERIVED_TYPE* instance)
    {
        sInstance = instance;
    }

private:
    static LL_THREAD_LOCAL DERIVED_TYPE* sInstance;
};

template<typename DERIVED_TYPE>
LL_THREAD_LOCAL DERIVED_TYPE* LLThreadLocalSingletonPointer<DERIVED_TYPE>::sInstance = NULL;

#endif // LL_LLTHREADLOCALSTORAGE_H
