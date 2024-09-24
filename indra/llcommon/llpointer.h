/**
 * @file llpointer.h
 * @brief A reference-counted pointer for objects derived from LLRefCount
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#ifndef LLPOINTER_H
#define LLPOINTER_H

#include "llerror.h"    // *TODO: consider eliminating this
#include "llmutex.h"

//----------------------------------------------------------------------------
// RefCount objects should generally only be accessed by way of LLPointer<>'s
// NOTE: LLPointer<LLFoo> x = new LLFoo(); MAY NOT BE THREAD SAFE
//   if LLFoo::LLFoo() does anything like put itself in an update queue.
//   The queue may get accessed before it gets assigned to x.
// The correct implementation is:
//   LLPointer<LLFoo> x = new LLFoo; // constructor does not do anything interesting
//   x->instantiate(); // does stuff like place x into an update queue

// see llthread.h for LLThreadSafeRefCount

//----------------------------------------------------------------------------

// Note: relies on Type having ref() and unref() methods
template <class Type> class LLPointer
{
public:
    template<typename Subclass>
    friend class LLPointer;

    LLPointer() :
        mPointer(nullptr)
    {
    }

    LLPointer(Type* ptr) :
        mPointer(ptr)
    {
        ref();
    }

    LLPointer(const LLPointer<Type>& ptr) :
        mPointer(ptr.mPointer)
    {
        ref();
    }

    LLPointer(LLPointer<Type>&& ptr) noexcept
    {
        mPointer = ptr.mPointer;
        ptr.mPointer = nullptr;
    }

    // Support conversion up the type hierarchy. See Item 45 in Effective C++, 3rd Ed.
    template<typename Subclass>
    LLPointer(const LLPointer<Subclass>& ptr) :
        mPointer(ptr.get())
    {
        ref();
    }

    template<typename Subclass>
    LLPointer(LLPointer<Subclass>&& ptr) noexcept :
        mPointer(ptr.get())
    {
        ptr.mPointer = nullptr;
    }

    ~LLPointer()
    {
        unref();
    }

    Type*   get() const                         { return mPointer; }
    const Type* operator->() const              { return mPointer; }
    Type*   operator->()                        { return mPointer; }
    const Type& operator*() const               { return *mPointer; }
    Type&   operator*()                         { return *mPointer; }

    operator BOOL() const                       { return (mPointer != nullptr); }
    operator bool() const                       { return (mPointer != nullptr); }
    bool operator!() const                      { return (mPointer == nullptr); }
    bool isNull() const                         { return (mPointer == nullptr); }
    bool notNull() const                        { return (mPointer != nullptr); }

    operator Type*() const                      { return mPointer; }
    template <typename Type1>
    bool operator !=(Type1* ptr) const          { return (mPointer != ptr); }
    template <typename Type1>
    bool operator ==(Type1* ptr) const          { return (mPointer == ptr); }
    template <typename Type1>
    bool operator !=(const LLPointer<Type1>& ptr) const { return (mPointer != ptr.mPointer); }
    template <typename Type1>
    bool operator ==(const LLPointer<Type1>& ptr) const { return (mPointer == ptr.mPointer); }
    bool operator < (const LLPointer<Type>& ptr)  const { return (mPointer < ptr.mPointer); }
    bool operator > (const LLPointer<Type>& ptr)  const { return (mPointer > ptr.mPointer); }

    LLPointer<Type>& operator =(Type* ptr)
    {
        assign(ptr);
        return *this;
    }

    LLPointer<Type>& operator =(const LLPointer<Type>& ptr)
    {
        assign(ptr);
        return *this;
    }

    LLPointer<Type>& operator =(LLPointer<Type>&& ptr)
    {
        if (mPointer != ptr.mPointer)
        {
            unref();
            mPointer = ptr.mPointer;
            ptr.mPointer = nullptr;
        }
        return *this;
    }

    // support assignment up the type hierarchy. See Item 45 in Effective C++, 3rd Ed.
    template<typename Subclass>
    LLPointer<Type>& operator =(const LLPointer<Subclass>& ptr)
    {
        assign(ptr.get());
        return *this;
    }

    template<typename Subclass>
    LLPointer<Type>& operator =(LLPointer<Subclass>&& ptr)
    {
        if (mPointer != ptr.mPointer)
        {
            unref();
            mPointer = ptr.mPointer;
            ptr.mPointer = nullptr;
        }
        return *this;
    }

    // Just exchange the pointers, which will not change the reference counts.
    static void swap(LLPointer<Type>& a, LLPointer<Type>& b)
    {
        Type* temp = a.mPointer;
        a.mPointer = b.mPointer;
        b.mPointer = temp;
    }

protected:
#ifdef LL_LIBRARY_INCLUDE
    void ref();
    void unref();
#else
    void ref()
    {
        if (mPointer)
        {
            mPointer->ref();
        }
    }

    void unref()
    {
        if (mPointer)
        {
            Type *temp = mPointer;
            mPointer = nullptr;
            temp->unref();
            if (mPointer != nullptr)
            {
                LL_WARNS() << "Unreference did assignment to non-NULL because of destructor" << LL_ENDL;
                unref();
            }
        }
    }
#endif // LL_LIBRARY_INCLUDE

    void assign(const LLPointer<Type>& ptr)
    {
        if (mPointer != ptr.mPointer)
        {
            unref();
            mPointer = ptr.mPointer;
            ref();
        }
    }

protected:
    Type*   mPointer;
};

template <class Type> class LLConstPointer
{
    template<typename Subclass>
    friend class LLConstPointer;
public:
    LLConstPointer() :
        mPointer(nullptr)
    {
    }

    LLConstPointer(const Type* ptr) :
        mPointer(ptr)
    {
        ref();
    }

    LLConstPointer(const LLConstPointer<Type>& ptr) :
        mPointer(ptr.mPointer)
    {
        ref();
    }

    LLConstPointer(LLConstPointer<Type>&& ptr) noexcept
    {
        mPointer = ptr.mPointer;
        ptr.mPointer = nullptr;
    }

    // support conversion up the type hierarchy.  See Item 45 in Effective C++, 3rd Ed.
    template<typename Subclass>
    LLConstPointer(const LLConstPointer<Subclass>& ptr) :
        mPointer(ptr.get())
    {
        ref();
    }

    template<typename Subclass>
    LLConstPointer(LLConstPointer<Subclass>&& ptr) noexcept :
        mPointer(ptr.get())
    {
        ptr.mPointer = nullptr;
    }

    ~LLConstPointer()
    {
        unref();
    }

    const Type* get() const                     { return mPointer; }
    const Type* operator->() const              { return mPointer; }
    const Type& operator*() const               { return *mPointer; }

    operator BOOL() const                       { return (mPointer != nullptr); }
    operator bool() const                       { return (mPointer != nullptr); }
    bool operator!() const                      { return (mPointer == nullptr); }
    bool isNull() const                         { return (mPointer == nullptr); }
    bool notNull() const                        { return (mPointer != nullptr); }

    operator const Type*() const                { return mPointer; }
    bool operator !=(const Type* ptr) const     { return (mPointer != ptr); }
    bool operator ==(const Type* ptr) const     { return (mPointer == ptr); }
    bool operator ==(const LLConstPointer<Type>& ptr) const { return (mPointer == ptr.mPointer); }
    bool operator < (const LLConstPointer<Type>& ptr) const { return (mPointer < ptr.mPointer); }
    bool operator > (const LLConstPointer<Type>& ptr) const { return (mPointer > ptr.mPointer); }

    LLConstPointer<Type>& operator =(const Type* ptr)
    {
        if( mPointer != ptr )
        {
            unref();
            mPointer = ptr;
            ref();
        }

        return *this;
    }

    LLConstPointer<Type>& operator =(const LLConstPointer<Type>& ptr)
    {
        if( mPointer != ptr.mPointer )
        {
            unref();
            mPointer = ptr.mPointer;
            ref();
        }
        return *this;
    }

    LLConstPointer<Type>& operator =(LLConstPointer<Type>&& ptr)
    {
        if (mPointer != ptr.mPointer)
        {
            unref();
            mPointer = ptr.mPointer;
            ptr.mPointer = nullptr;
        }
        return *this;
    }

    // support assignment up the type hierarchy. See Item 45 in Effective C++, 3rd Ed.
    template<typename Subclass>
    LLConstPointer<Type>& operator =(const LLConstPointer<Subclass>& ptr)
    {
        if( mPointer != ptr.get() )
        {
            unref();
            mPointer = ptr.get();
            ref();
        }
        return *this;
    }

    template<typename Subclass>
    LLConstPointer<Type>& operator =(LLConstPointer<Subclass>&& ptr)
    {
        if (mPointer != ptr.mPointer)
        {
            unref();
            mPointer = ptr.mPointer;
            ptr.mPointer = nullptr;
        }
        return *this;
    }

    // Just exchange the pointers, which will not change the reference counts.
    static void swap(LLConstPointer<Type>& a, LLConstPointer<Type>& b)
    {
        const Type* temp = a.mPointer;
        a.mPointer = b.mPointer;
        b.mPointer = temp;
    }

protected:
#ifdef LL_LIBRARY_INCLUDE
    void ref();
    void unref();
#else // LL_LIBRARY_INCLUDE
    void ref()
    {
        if (mPointer)
        {
            mPointer->ref();
        }
    }

    void unref()
    {
        if (mPointer)
        {
            const Type *temp = mPointer;
            mPointer = nullptr;
            temp->unref();
            if (mPointer != nullptr)
            {
                LL_WARNS() << "Unreference did assignment to non-NULL because of destructor" << LL_ENDL;
                unref();
            }
        }
    }
#endif // LL_LIBRARY_INCLUDE

protected:
    const Type* mPointer;
};

template<typename Type>
class LLCopyOnWritePointer : public LLPointer<Type>
{
public:
    typedef LLCopyOnWritePointer<Type> self_t;
    typedef LLPointer<Type> pointer_t;

    LLCopyOnWritePointer()
    :   mStayUnique(false)
    {}

    LLCopyOnWritePointer(Type* ptr)
    :   LLPointer<Type>(ptr),
        mStayUnique(false)
    {}

    LLCopyOnWritePointer(LLPointer<Type>& ptr)
    :   LLPointer<Type>(ptr),
        mStayUnique(false)
    {
        if (ptr.mStayUnique)
        {
            makeUnique();
        }
    }

    Type* write()
    {
        makeUnique();
        return pointer_t::mPointer;
    }

    void makeUnique()
    {
        if (pointer_t::notNull() && pointer_t::mPointer->getNumRefs() > 1)
        {
            *(pointer_t* )(this) = new Type(*pointer_t::mPointer);
        }
    }

    const Type* operator->() const  { return pointer_t::mPointer; }
    const Type& operator*() const   { return *pointer_t::mPointer; }

    void setStayUnique(bool stay) { makeUnique(); mStayUnique = stay; }
private:
    bool mStayUnique;
};

template<typename Type0, typename Type1>
bool operator!=(Type0* lhs, const LLPointer<Type1>& rhs)
{
    return (lhs != rhs.get());
}

template<typename Type0, typename Type1>
bool operator==(Type0* lhs, const LLPointer<Type1>& rhs)
{
    return (lhs == rhs.get());
}

// boost hash adapter
template <class Type>
struct boost::hash<LLPointer<Type>>
{
    typedef LLPointer<Type> argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const
    {
        return (std::size_t) s.get();
    }
};

// Adapt boost hash to std hash
namespace std
{
    template<class Type> struct hash<LLPointer<Type>>
    {
        std::size_t operator()(LLPointer<Type> const& s) const noexcept
        {
            return boost::hash<LLPointer<Type>>()(s);
        }
    };
}
#endif
