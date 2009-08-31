/**
 * @file   llptrto.h
 * @author Nat Goodspeed
 * @date   2008-08-19
 * @brief  LLPtrTo<TARGET> is a template helper to pick either TARGET* or -- when
 *         TARGET is a subclass of LLRefCount or LLThreadSafeRefCount --
 *         LLPointer<TARGET>. LLPtrTo<> chooses whichever pointer type is best.
 * 
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLPTRTO_H)
#define LL_LLPTRTO_H

#include "llpointer.h"
#include "llrefcount.h"             // LLRefCount
#include "llthread.h"               // LLThreadSafeRefCount
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
