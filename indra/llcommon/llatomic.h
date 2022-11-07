/** 
 * @file llatomic.h
 * @brief Base classes for atomic.
 *
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2018, Linden Research, Inc.
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

#ifndef LL_LLATOMIC_H
#define LL_LLATOMIC_H

#include "stdtypes.h"

#include <atomic>

template <typename Type, typename AtomicType = std::atomic< Type > > class LLAtomicBase
{
public:
    LLAtomicBase() {};
    LLAtomicBase(Type x) { mData.store(x); }
    ~LLAtomicBase() {};

    operator const Type() { return mData; }

    Type    CurrentValue() const { return mData; }

    Type operator =(const Type& x) { mData.store(x); return mData; }
    void operator -=(Type x) { mData -= x; }
    void operator +=(Type x) { mData += x; }
    Type operator ++(int) { return mData++; }
    Type operator --(int) { return mData--; }

    Type operator ++() { return ++mData; }
    Type operator --() { return --mData; }

private:
    AtomicType mData;
};

// Typedefs for specialized versions. Using std::atomic_(u)int32_t to get the optimzed implementation.
#ifdef LL_WINDOWS
typedef LLAtomicBase<U32, std::atomic_uint32_t> LLAtomicU32;
typedef LLAtomicBase<S32, std::atomic_int32_t> LLAtomicS32;
#else
typedef LLAtomicBase<U32, std::atomic_uint> LLAtomicU32;
typedef LLAtomicBase<S32, std::atomic_int> LLAtomicS32;
#endif

typedef LLAtomicBase<bool, std::atomic_bool> LLAtomicBool;

#endif // LL_LLATOMIC_H
