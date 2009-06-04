/**
 * @file   lllazy.h
 * @author Nat Goodspeed
 * @date   2009-01-22
 * @brief  Lazy instantiation of specified type. Useful in conjunction with
 *         Michael Feathers's "Extract and Override Getter" ("Working
 *         Effectively with Legacy Code", p. 352).
 *
 * Quoting his synopsis of steps on p.355:
 *
 * 1. Identify the object you need a getter for.
 * 2. Extract all of the logic needed to create the object into a getter.
 * 3. Replace all uses of the object with calls to the getter, and initialize
 *    the reference that holds the object to null in all constructors.
 * 4. Add the first-time logic to the getter so that the object is constructed
 *    and assigned to the reference whenever the reference is null.
 * 5. Subclass the class and override the getter to provide an alternative
 *    object for testing.
 *
 * It's the second half of bullet 3 (3b, as it were) that bothers me. I find
 * it all too easy to imagine adding pointer initializers to all but one
 * constructor... the one not exercised by my tests. That suggested using
 * (e.g.) boost::scoped_ptr<MyObject> so you don't have to worry about
 * destroying it either.
 *
 * However, introducing additional machinery allows us to encapsulate bullet 4
 * as well.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLLAZY_H)
#define LL_LLLAZY_H

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/lambda/construct.hpp>
#include <stdexcept>

/// LLLazyCommon simply factors out of LLLazy<T> things that don't depend on
/// its template parameter.
class LLLazyCommon
{
public:
    /**
     * This exception is thrown if you try to replace an LLLazy<T>'s factory
     * (or T* instance) after it already has an instance in hand. Since T
     * might well be stateful, we can't know the effect of silently discarding
     * and replacing an existing instance, so we disallow it. This facility is
     * intended for testing, and in a test scenario we can definitely control
     * that.
     */
    struct InstanceChange: public std::runtime_error
    {
        InstanceChange(const std::string& what): std::runtime_error(what) {}
    };

protected:
    /**
     * InstanceChange might be appropriate in a couple of different LLLazy<T>
     * methods. Factor out the common logic.
     */
    template <typename PTR>
    static void ensureNoInstance(const PTR& ptr)
    {
        if (ptr)
        {
            // Too late: we've already instantiated the lazy object. We don't
            // know whether it's stateful or not, so it's not safe to discard
            // the existing instance in favor of a replacement.
            throw InstanceChange("Too late to replace LLLazy instance");
        }
    }
};

/**
 * LLLazy<T> is useful when you have an outer class Outer that you're trying
 * to bring under unit test, that contains a data member difficult to
 * instantiate in a test harness. Typically the data member's class Inner has
 * many thorny dependencies. Feathers generally advocates "Extract and
 * Override Factory Method" (p. 350). But in C++, you can't call a derived
 * class override of a virtual method from the derived class constructor,
 * which limits applicability of "Extract and Override Factory Method." For
 * such cases Feathers presents "Extract and Override Getter" (p. 352).
 *
 * So we'll assume that your class Outer contains a member like this:
 * @code
 * Inner mInner;
 * @endcode
 *
 * LLLazy<Inner> can be used to replace this member. You can directly declare:
 * @code
 * LLLazy<Inner> mInner;
 * @endcode
 * and change references to mInner accordingly.
 *
 * (Alternatively, you can add a base class of the form
 * <tt>LLLazyBase<Inner></tt>. This is discussed further in the LLLazyBase<T>
 * documentation.)
 *
 * LLLazy<T> binds a <tt>boost::scoped_ptr<T></tt> and a factory functor
 * returning T*. You can either bind that functor explicitly or let it default
 * to the expression <tt>new T()</tt>.
 *
 * As long as LLLazy<T> remains unreferenced, its T remains uninstantiated.
 * The first time you use get(), <tt>operator*()</tt> or <tt>operator->()</tt>
 * it will instantiate its T and thereafter behave like a pointer to it.
 *
 * Thus, any existing reference to <tt>mInner.member</tt> should be replaced
 * with <tt>mInner->member</tt>. Any simple reference to @c mInner should be
 * replaced by <tt>*mInner</tt>.
 *
 * (If the original declaration was a pointer initialized in Outer's
 * constructor, e.g. <tt>Inner* mInner</tt>, so much the better. In that case
 * you should be able to drop in <tt>LLLazy<Inner></tt> without much change.)
 *
 * The support for "Extract and Override Getter" lies in the fact that you can
 * replace the factory functor -- or provide an explicit T*. Presumably this
 * is most useful from a test subclass -- which suggests that your @c mInner
 * member should be @c protected.
 *
 * Note that <tt>boost::lambda::new_ptr<T>()</tt> makes a dandy factory
 * functor, for either the set() method or LLLazy<T>'s constructor. If your T
 * requires constructor arguments, use an expression more like
 * <tt>boost::lambda::bind(boost::lambda::new_ptr<T>(), arg1, arg2, ...)</tt>.
 *
 * Of course the point of replacing the functor is to substitute a class that,
 * though referenced as Inner*, is not an Inner; presumably this is a testing
 * subclass of Inner (e.g. TestInner). Thus your test subclass TestOuter for
 * the containing class Outer will contain something like this:
 * @code
 * class TestOuter: public Outer
 * {
 * public:
 *     TestOuter()
 *     {
 *         // mInner must be 'protected' rather than 'private'
 *         mInner.set(boost::lambda::new_ptr<TestInner>());
 *     }
 *     ...
 * };
 * @endcode
 */
template <typename T>
class LLLazy: public LLLazyCommon
{
public:
    /// Any nullary functor returning T* will work as a Factory
    typedef boost::function<T* ()> Factory;

    /// The default LLLazy constructor uses <tt>new T()</tt> as its Factory
    LLLazy():
        mFactory(boost::lambda::new_ptr<T>())
    {}

    /// Bind an explicit Factory functor
    LLLazy(const Factory& factory):
        mFactory(factory)
    {}

    /// Reference T, instantiating it if this is the first access
    const T& get() const
    {
        if (! mInstance)
        {
            // use the bound Factory functor
            mInstance.reset(mFactory());
        }
        return *mInstance;
    }

    /// non-const get()
    T& get()
    {
        return const_cast<T&>(const_cast<const LLLazy<T>*>(this)->get());
    }

    /// operator*() is equivalent to get()
    const T& operator*() const { return get(); }
    /// operator*() is equivalent to get()
    T& operator*() { return get(); }

    /**
     * operator->() must return (something resembling) T*. It's tempting to
     * return the underlying boost::scoped_ptr<T>, but that would require
     * breaking out the lazy-instantiation logic from get() into a common
     * private method. Assume the pointer used for operator->() access is very
     * short-lived.
     */
    const T* operator->() const { return &get(); }
    /// non-const operator->()
    T* operator->() { return &get(); }

    /// set(Factory). This will throw InstanceChange if mInstance has already
    /// been set.
    void set(const Factory& factory)
    {
        ensureNoInstance(mInstance);
        mFactory = factory;
    }

    /// set(T*). This will throw InstanceChange if mInstance has already been
    /// set.
    void set(T* instance)
    {
        ensureNoInstance(mInstance);
        mInstance.reset(instance);
    }

private:
    Factory mFactory;
    // Consider an LLLazy<T> member of a class we're accessing by const
    // reference. We want to allow even const methods to touch the LLLazy<T>
    // member. Hence the actual pointer must be mutable because such access
    // might assign it.
    mutable boost::scoped_ptr<T> mInstance;
};

#if (! defined(__GNUC__)) || (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ > 3)
// Not gcc at all, or a gcc more recent than gcc 3.3
#define GCC33 0
#else
#define GCC33 1
#endif

/**
 * LLLazyBase<T> wraps LLLazy<T>, giving you an alternative way to replace
 * <tt>Inner mInner;</tt>. Instead of coding <tt>LLLazy<Inner> mInner</tt>,
 * you can add LLLazyBase<Inner> to your Outer class's bases, e.g.:
 * @code
 * class Outer: public LLLazyBase<Inner>
 * {
 *     ...
 * };
 * @endcode
 *
 * This gives you @c public get() and @c protected set() methods without
 * having to make your LLLazy<Inner> member @c protected. The tradeoff is that
 * you must access the wrapped LLLazy<Inner> using get() and set() rather than
 * with <tt>operator*()</tt> or <tt>operator->()</tt>.
 *
 * This mechanism can be used for more than one member, but only if they're of
 * different types. That is, you can replace:
 * @code
 * DifficultClass mDifficult;
 * AwkwardType    mAwkward;
 * @endcode
 * with:
 * @code
 * class Outer: public LLLazyBase<DifficultClass>, public LLLazyBase<AwkwardType>
 * {
 *     ...
 * };
 * @endcode
 * but for a situation like this:
 * @code
 * DifficultClass mMainDifficult, mAuxDifficult;
 * @endcode
 * you should directly embed LLLazy<DifficultClass> (q.v.).
 *
 * For multiple LLLazyBase bases, e.g. the <tt>LLLazyBase<DifficultClass>,
 * LLLazyBase<AwkwardType></tt> example above, access the relevant get()/set()
 * as (e.g.) <tt>LLLazyBase<DifficultClass>::get()</tt>. (This is why you
 * can't have multiple LLLazyBase<T> of the same T.) For a bit of syntactic
 * sugar, please see getLazy()/setLazy().
 */
template <typename T>
class LLLazyBase
{
public:
    /// invoke default LLLazy constructor
    LLLazyBase() {}
    /// make wrapped LLLazy bind an explicit Factory
    LLLazyBase(const typename LLLazy<T>::Factory& factory):
        mInstance(factory)
    {}

    /// access to LLLazy::get()
    T& get() { return *mInstance; }
    /// access to LLLazy::get()
    const T& get() const { return *mInstance; }

protected:
    // see getLazy()/setLazy()
    #if (! GCC33)
    template <typename T2, class MYCLASS> friend T2& getLazy(MYCLASS* this_);
    template <typename T2, class MYCLASS> friend const T2& getLazy(const MYCLASS* this_);
    #else // gcc 3.3
    template <typename T2, class MYCLASS> friend T2& getLazy(const MYCLASS* this_);
    #endif // gcc 3.3
    template <typename T2, class MYCLASS> friend void setLazy(MYCLASS* this_, T2* instance);
    template <typename T2, class MYCLASS>
    friend void setLazy(MYCLASS* this_, const typename LLLazy<T2>::Factory& factory);

    /// access to LLLazy::set(Factory)
    void set(const typename LLLazy<T>::Factory& factory)
    {
        mInstance.set(factory);
    }

    /// access to LLLazy::set(T*)
    void set(T* instance)
    {
        mInstance.set(instance);
    }

private:
    LLLazy<T> mInstance;
};

/**
 * @name getLazy()/setLazy()
 * Suppose you have something like the following:
 * @code
 * class Outer: public LLLazyBase<DifficultClass>, public LLLazyBase<AwkwardType>
 * {
 *     ...
 * };
 * @endcode
 *
 * Your methods can reference the @c DifficultClass instance using
 * <tt>LLLazyBase<DifficultClass>::get()</tt>, which is admittedly a bit ugly.
 * Alternatively, you can write <tt>getLazy<DifficultClass>(this)</tt>, which
 * is somewhat more straightforward to read.
 *
 * Similarly,
 * @code
 * LLLazyBase<DifficultClass>::set(new TestDifficultClass());
 * @endcode
 * could instead be written:
 * @code
 * setLazy<DifficultClass>(this, new TestDifficultClass());
 * @endcode
 *
 * @note
 * I wanted to provide getLazy() and setLazy() without explicitly passing @c
 * this. That would imply making them methods on a base class rather than free
 * functions. But if <tt>LLLazyBase<T></tt> derives normally from (say) @c
 * LLLazyGrandBase providing those methods, then unqualified getLazy() would
 * be ambiguous: you'd have to write <tt>LLLazyBase<T>::getLazy<T>()</tt>,
 * which is even uglier than <tt>LLLazyBase<T>::get()</tt>, and therefore
 * pointless. You can make the compiler not care which @c LLLazyGrandBase
 * instance you're talking about by making @c LLLazyGrandBase a @c virtual
 * base class of @c LLLazyBase. But in that case,
 * <tt>LLLazyGrandBase::getLazy<T>()</tt> can't access
 * <tt>LLLazyBase<T>::get()</tt>!
 *
 * We want <tt>getLazy<T>()</tt> to access <tt>LLLazyBase<T>::get()</tt> as if
 * in the lexical context of some subclass method. Ironically, free functions
 * let us do that better than methods on a @c virtual base class -- but that
 * implies passing @c this explicitly. So be it.
 */
//@{
#if (! GCC33)
template <typename T, class MYCLASS>
T& getLazy(MYCLASS* this_) { return this_->LLLazyBase<T>::get(); }
template <typename T, class MYCLASS>
const T& getLazy(const MYCLASS* this_) { return this_->LLLazyBase<T>::get(); }
#else // gcc 3.3
// For const-correctness, we really should have two getLazy() variants: one
// accepting const MYCLASS* and returning const T&, the other accepting
// non-const MYCLASS* and returning non-const T&. This works fine on the Mac
// (gcc 4.0.1) and Windows (MSVC 8.0), but fails on our Linux 32-bit Debian
// Sarge stations (gcc 3.3.5). Since I really don't know how to beat that aging
// compiler over the head to make it do the right thing, I'm going to have to
// move forward with the wrong thing: a single getLazy() function that accepts
// const MYCLASS* and returns non-const T&.
template <typename T, class MYCLASS>
T& getLazy(const MYCLASS* this_) { return const_cast<MYCLASS*>(this_)->LLLazyBase<T>::get(); }
#endif // gcc 3.3
template <typename T, class MYCLASS>
void setLazy(MYCLASS* this_, T* instance) { this_->LLLazyBase<T>::set(instance); }
template <typename T, class MYCLASS>
void setLazy(MYCLASS* this_, const typename LLLazy<T>::Factory& factory)
{
    this_->LLLazyBase<T>::set(factory);
}
//@}

#endif /* ! defined(LL_LLLAZY_H) */
