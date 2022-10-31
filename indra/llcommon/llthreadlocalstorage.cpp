/** 
 * @file llthreadlocalstorage.cpp
 * @author Richard
 * @date 2013-1-11
 * @brief implementation of thread local storage utility classes
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
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

#include "linden_common.h"
#include "llthreadlocalstorage.h"
#include "llapr.h"

//
//LLThreadLocalPointerBase
//
bool LLThreadLocalPointerBase::sInitialized = false;

void LLThreadLocalPointerBase::set( void* value )
{
    llassert(sInitialized && mThreadKey);

    apr_status_t result = apr_threadkey_private_set((void*)value, mThreadKey);
    if (result != APR_SUCCESS)
    {
        ll_apr_warn_status(result);
        LL_ERRS() << "Failed to set thread local data" << LL_ENDL;
    }
}

void* LLThreadLocalPointerBase::get() const
{
    // llassert(sInitialized);
    void* ptr;
    apr_status_t result =
        apr_threadkey_private_get(&ptr, mThreadKey);
    if (result != APR_SUCCESS)
    {
        ll_apr_warn_status(result);
        LL_ERRS() << "Failed to get thread local data" << LL_ENDL;
    }
    return ptr;
}


void LLThreadLocalPointerBase::initStorage( )
{
    apr_status_t result = apr_threadkey_private_create(&mThreadKey, NULL, gAPRPoolp);
    if (result != APR_SUCCESS)
    {
        ll_apr_warn_status(result);
        LL_ERRS() << "Failed to allocate thread local data" << LL_ENDL;
    }
}

void LLThreadLocalPointerBase::destroyStorage()
{
    if (sInitialized)
    {
        if (mThreadKey)
        {
            apr_status_t result = apr_threadkey_private_delete(mThreadKey);
            if (result != APR_SUCCESS)
            {
                ll_apr_warn_status(result);
                LL_ERRS() << "Failed to delete thread local data" << LL_ENDL;
            }
        }
    }
}

//static
void LLThreadLocalPointerBase::initAllThreadLocalStorage()
{
    if (!sInitialized)
    {
        for (auto& base : instance_snapshot())
        {
            base.initStorage();
        }
        sInitialized = true;
    }
}

//static
void LLThreadLocalPointerBase::destroyAllThreadLocalStorage()
{
    if (sInitialized)
    {
        //for (auto& base : instance_snapshot())
        //{
        //  base.destroyStorage();
        //}
        sInitialized = false;
    }
}
