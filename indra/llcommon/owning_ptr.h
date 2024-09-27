/**
 * @file   owning_ptr.h
 * @author Nat Goodspeed
 * @date   2024-09-27
 * @brief  owning_ptr<T> is like std::unique_ptr<T>, but easier to integrate
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_OWNING_PTR_H)
#define LL_OWNING_PTR_H

#include <functional>
#include <memory>

/**
 * owning_ptr<T> adapts std::unique_ptr<T> to make it easier to adopt into
 * older code using dumb pointers.
 *
 * Consider a class Outer with a member Thing* mThing. After the constructor,
 * each time a method wants to assign to mThing, it must test for nullptr and
 * destroy the previous Thing instance. During Outer's lifetime, mThing is
 * passed to legacy domain-specific functions accepting plain Thing*. Finally
 * the destructor must again test for nullptr and destroy the remaining Thing
 * instance.
 *
 * Multiply that by several different Outer members of different types,
 * possibly with different domain-specific destructor functions.
 *
 * Dropping std::unique_ptr<Thing> into Outer is cumbersome for a several
 * reasons. First, if Thing requires a domain-specific destructor function,
 * the unique_ptr declaration of mThing must explicitly state the type of that
 * function (as a function pointer, for a typical legacy function). Second,
 * every Thing* assignment to mThing must be changed to mThing.reset(). Third,
 * every time we call a legacy domain-specific function, we must pass
 * mThing.get().
 *
 * owning_ptr<T> is designed to drop into a situation like this. The domain-
 * specific destructor function, if any, is passed to its constructor; it need
 * not be encoded into the pointer type. owning_ptr<T> supports plain pointer
 * assignment, internally calling std::unique_ptr<T>::reset(). It also
 * supports implicit conversion to plain T*, to pass the owned pointer to
 * legacy domain-specific functions.
 *
 * Obviously owning_ptr<T> must not be used in situations where ownership of
 * the referenced object is passed on to another pointer: use std::unique_ptr
 * for that. Similarly, it is not for shared ownership. It simplifies lifetime
 * management for classes that currently store (and explicitly destroy) plain
 * T* pointers.
 */
template <typename T>
class owning_ptr
{
    using deleter = std::function<void(T*)>;
public:
    owning_ptr(T* p=nullptr, const deleter& d=std::default_delete<T>()):
        mPtr(p, d)
    {}
    void reset(T* p=nullptr) { mPtr.reset(p); }
    owning_ptr& operator=(T* p) { mPtr.reset(p); return *this; }
    operator T*() const { return mPtr.get(); }
    T& operator*() const { return *mPtr; }
    T* operator->() const { return mPtr.operator->(); }

private:
    std::unique_ptr<T, deleter> mPtr;
};

#endif /* ! defined(LL_OWNING_PTR_H) */
