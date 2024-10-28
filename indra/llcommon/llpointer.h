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

#include <boost/functional/hash.hpp>
#include <string_view>
#include <utility>                  // std::swap()

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

class LLPointerBase
{
protected:
    // alert the coder that a referenced type's destructor did something very
    // strange -- this is in a non-template base class so we can hide the
    // implementation in llpointer.cpp
    static void wild_dtor(std::string_view msg);
};

// Note: relies on Type having ref() and unref() methods
template <class Type>
class LLPointer: public LLPointerBase
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

    // Even though the template constructors below accepting
    // (const LLPointer<Subclass>&) and (LLPointer<Subclass>&&) appear to
    // subsume these specific (const LLPointer<Type>&) and (LLPointer<Type>&&)
    // constructors, the compiler recognizes these as The Copy Constructor and
    // The Move Constructor, respectively. In other words, even in the
    // presence of the LLPointer<Subclass> constructors, we still must specify
    // the LLPointer<Type> constructors.
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
        // copy-and-swap idiom, see http://gotw.ca/gotw/059.htm
        LLPointer temp(ptr);
        using std::swap;            // per Swappable convention
        swap(*this, temp);
        return *this;
    }

    // Even though the template assignment operators below accepting
    // (const LLPointer<Subclass>&) and (LLPointer<Subclass>&&) appear to
    // subsume these specific (const LLPointer<Type>&) and (LLPointer<Type>&&)
    // assignment operators, the compiler recognizes these as Copy Assignment
    // and Move Assignment, respectively. In other words, even in the presence
    // of the LLPointer<Subclass> assignment operators, we still must specify
    // the LLPointer<Type> operators.
    LLPointer<Type>& operator =(const LLPointer<Type>& ptr)
    {
        LLPointer temp(ptr);
        using std::swap;            // per Swappable convention
        swap(*this, temp);
        return *this;
    }

    LLPointer<Type>& operator =(LLPointer<Type>&& ptr)
    {
        LLPointer temp(std::move(ptr));
        using std::swap;            // per Swappable convention
        swap(*this, temp);
        return *this;
    }

    // support assignment up the type hierarchy. See Item 45 in Effective C++, 3rd Ed.
    template<typename Subclass>
    LLPointer<Type>& operator =(const LLPointer<Subclass>& ptr)
    {
        LLPointer temp(ptr);
        using std::swap;            // per Swappable convention
        swap(*this, temp);
        return *this;
    }

    template<typename Subclass>
    LLPointer<Type>& operator =(LLPointer<Subclass>&& ptr)
    {
        LLPointer temp(std::move(ptr));
        using std::swap;            // per Swappable convention
        swap(*this, temp);
        return *this;
    }

    // Just exchange the pointers, which will not change the reference counts.
    static void swap(LLPointer<Type>& a, LLPointer<Type>& b)
    {
        using std::swap;            // per Swappable convention
        swap(a.mPointer, b.mPointer);
    }

    // Put swap() overload in the global namespace, per Swappable convention
    friend void swap(LLPointer<Type>& a, LLPointer<Type>& b)
    {
        LLPointer<Type>::swap(a, b);
    }

protected:
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
                wild_dtor("Unreference did assignment to non-NULL because of destructor");
                unref();
            }
        }
    }

protected:
    Type*   mPointer;
};

template <typename Type>
using LLConstPointer = LLPointer<const Type>;

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
