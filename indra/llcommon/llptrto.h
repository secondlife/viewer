/**
 * @file   llptrto.h
 * @author Nat Goodspeed
 * @date   2008-08-19
 * @brief  LLPtrTo<TARGET> is a template helper to pick either TARGET* or -- when
 *         TARGET is a subclass of LLRefCount or LLThreadSafeRefCount --
 *         LLPointer<TARGET>. LLPtrTo<> chooses whichever pointer type is best.
 * 
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#if ! defined(LL_LLPTRTO_H)
#define LL_LLPTRTO_H

#include "llpointer.h"
#include "llrefcount.h"             // LLRefCount
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/utility/enable_if.hpp>

/**
 * LLPtrTo<TARGET>::type is either of two things:
 *
 * * When TARGET is a subclass of either LLRefCount or LLThreadSafeRefCount,
 *   LLPtrTo<TARGET>::type is LLPointer<TARGET>.
 * * Otherwise, LLPtrTo<TARGET>::type is TARGET*.
 *
 * This way, a class template can use LLPtrTo<TARGET>::type to select an
 * appropriate pointer type to store.
 */
template <class T, class ENABLE=void>
struct LLPtrTo
{
    typedef T* type;
};

/// specialize for subclasses of LLRefCount
template <class T>
struct LLPtrTo<T, typename boost::enable_if< boost::is_base_of<LLRefCount, T> >::type>
{
    typedef LLPointer<T> type;
};

/// specialize for subclasses of LLThreadSafeRefCount
template <class T>
struct LLPtrTo<T, typename boost::enable_if< boost::is_base_of<LLThreadSafeRefCount, T> >::type>
{
    typedef LLPointer<T> type;
};

/**
 * LLRemovePointer<PTRTYPE>::type gets you the underlying (pointee) type.
 */
template <typename PTRTYPE>
struct LLRemovePointer
{
    typedef typename boost::remove_pointer<PTRTYPE>::type type;
};

/// specialize for LLPointer<SOMECLASS>
template <typename SOMECLASS>
struct LLRemovePointer< LLPointer<SOMECLASS> >
{
    typedef SOMECLASS type;
};

#endif /* ! defined(LL_LLPTRTO_H) */
