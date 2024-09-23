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
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <memory>                   // std::shared_ptr, std::unique_ptr
#include <type_traits>

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
struct LLPtrTo<T, typename std::enable_if< boost::is_base_of<LLRefCount, T>::value >::type>
{
    typedef LLPointer<T> type;
};

/// specialize for subclasses of LLThreadSafeRefCount
template <class T>
struct LLPtrTo<T, typename std::enable_if< boost::is_base_of<LLThreadSafeRefCount, T>::value >::type>
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

namespace LL
{

/*****************************************************************************
*   get_ref()
*****************************************************************************/
    template <typename T>
    struct GetRef
    {
        // return const ref or non-const ref, depending on whether we can bind
        // a non-const lvalue ref to the argument
        const auto& operator()(const T& obj) const { return obj; }
        auto& operator()(T& obj) const { return obj; }
    };

    template <typename T>
    struct GetRef<const T*>
    {
        const auto& operator()(const T* ptr) const { return *ptr; }
    };

    template <typename T>
    struct GetRef<T*>
    {
        auto& operator()(T* ptr) const { return *ptr; }
    };

    template <typename T>
    struct GetRef< LLPointer<T> >
    {
        auto& operator()(LLPointer<T> ptr) const { return *ptr; }
    };

    /// whether we're passed a pointer or a reference, return a reference
    template <typename T>
    auto& get_ref(T& ptr_or_ref)
    {
        return GetRef<typename std::decay<T>::type>()(ptr_or_ref);
    }

    template <typename T>
    const auto& get_ref(const T& ptr_or_ref)
    {
        return GetRef<typename std::decay<T>::type>()(ptr_or_ref);
    }

/*****************************************************************************
*   get_ptr()
*****************************************************************************/
    // if T is any pointer type we recognize, return it unchanged
    template <typename T>
    const T* get_ptr(const T* ptr) { return ptr; }

    template <typename T>
    T* get_ptr(T* ptr) { return ptr; }

    template <typename T>
    const std::shared_ptr<T>& get_ptr(const std::shared_ptr<T>& ptr) { return ptr; }

    template <typename T>
    const std::unique_ptr<T>& get_ptr(const std::unique_ptr<T>& ptr) { return ptr; }

    template <typename T>
    const boost::shared_ptr<T>& get_ptr(const boost::shared_ptr<T>& ptr) { return ptr; }

    template <typename T>
    const boost::intrusive_ptr<T>& get_ptr(const boost::intrusive_ptr<T>& ptr) { return ptr; }

    template <typename T>
    const LLPointer<T>& get_ptr(const LLPointer<T>& ptr) { return ptr; }

    // T is not any pointer type we recognize, take a pointer to the parameter
    template <typename T>
    const T* get_ptr(const T& obj) { return &obj; }

    template <typename T>
    T* get_ptr(T& obj) { return &obj; }
} // namespace LL

#endif /* ! defined(LL_LLPTRTO_H) */
