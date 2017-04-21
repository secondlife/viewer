/** 
 * @file llsingleton.h
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
#ifndef LLSINGLETON_H
#define LLSINGLETON_H

#include <boost/noncopyable.hpp>
#include <boost/unordered_set.hpp>
#include <boost/intrusive_ptr.hpp>
#include <list>
#include <vector>
#include <typeinfo>

class LLSingletonBase: private boost::noncopyable
{
public:
    class MasterList;
    class MasterRefcount;
    typedef boost::intrusive_ptr<MasterRefcount> ref_ptr_t;

private:
    // All existing LLSingleton instances are tracked in this master list.
    typedef std::list<LLSingletonBase*> list_t;
    static list_t& get_master();
    // This, on the other hand, is a stack whose top indicates the LLSingleton
    // currently being initialized.
    static list_t& get_initializing();
    static list_t& get_initializing_from(MasterList*);
    // Produce a vector<LLSingletonBase*> of master list, in dependency order.
    typedef std::vector<LLSingletonBase*> vec_t;
    static vec_t dep_sort();

    bool mCleaned;                  // cleanupSingleton() has been called
    // we directly depend on these other LLSingletons
    typedef boost::unordered_set<LLSingletonBase*> set_t;
    set_t mDepends;

protected:
    typedef enum e_init_state
    {
        UNINITIALIZED = 0,          // must be default-initialized state
        CONSTRUCTING,
        INITIALIZING,
        INITIALIZED,
        DELETED
    } EInitState;

    // Define tag<T> to pass to our template constructor. You can't explicitly
    // invoke a template constructor with ordinary template syntax:
    // http://stackoverflow.com/a/3960925/5533635
    template <typename T>
    struct tag
    {
        typedef T type;
    };

    // Base-class constructor should only be invoked by the DERIVED_TYPE
    // constructor, which passes tag<DERIVED_TYPE> for various purposes.
    template <typename DERIVED_TYPE>
    LLSingletonBase(tag<DERIVED_TYPE>);
    virtual ~LLSingletonBase();

    // Every new LLSingleton should be added to/removed from the master list
    void add_master();
    void remove_master();
    // with a little help from our friends.
    template <class T> friend struct LLSingleton_manage_master;

    // Maintain a stack of the LLSingleton subclass instance currently being
    // initialized. We use this to notice direct dependencies: we want to know
    // if A requires B. We deduce a dependency if while initializing A,
    // control reaches B::getInstance().
    // We want &A to be at the top of that stack during both A::A() and
    // A::initSingleton(), since a call to B::getInstance() might occur during
    // either.
    // Unfortunately the desired timespan does not correspond neatly with a
    // single C++ scope, else we'd use RAII to track it. But we do know that
    // LLSingletonBase's constructor definitely runs just before
    // LLSingleton's, which runs just before the specific subclass's.
    void push_initializing(const char*);
    // LLSingleton is, and must remain, the only caller to initSingleton().
    // That being the case, we control exactly when it happens -- and we can
    // pop the stack immediately thereafter.
    void pop_initializing();
private:
    // logging
    static void log_initializing(const char* verb, const char* name);
protected:
    // If a given call to B::getInstance() happens during either A::A() or
    // A::initSingleton(), record that A directly depends on B.
    void capture_dependency(list_t& initializing, EInitState);

    // delegate LL_ERRS() logging to llsingleton.cpp
    static void logerrs(const char* p1, const char* p2="",
                        const char* p3="", const char* p4="");
    // delegate LL_WARNS() logging to llsingleton.cpp
    static void logwarns(const char* p1, const char* p2="",
                         const char* p3="", const char* p4="");
    static std::string demangle(const char* mangled);

    // obtain canonical ref_ptr_t
    static ref_ptr_t get_master_refcount();

    // Default methods in case subclass doesn't declare them.
    virtual void initSingleton() {}
    virtual void cleanupSingleton() {}

    // deleteSingleton() isn't -- and shouldn't be -- a virtual method. It's a
    // class static. However, given only Foo*, deleteAll() does need to be
    // able to reach Foo::deleteSingleton(). Make LLSingleton (which declares
    // deleteSingleton()) store a pointer here. Since we know it's a static
    // class method, a classic-C function pointer will do.
    void (*mDeleteSingleton)();

public:
    /**
     * Call this to call the cleanupSingleton() method for every LLSingleton
     * constructed since the start of the last cleanupAll() call. (Any
     * LLSingleton constructed DURING a cleanupAll() call won't be cleaned up
     * until the next cleanupAll() call.) cleanupSingleton() neither deletes
     * nor destroys its LLSingleton; therefore it's safe to include logic that
     * might take significant realtime or even throw an exception.
     *
     * The most important property of cleanupAll() is that cleanupSingleton()
     * methods are called in dependency order, leaf classes last. Thus, given
     * two LLSingleton subclasses A and B, if A's dependency on B is properly
     * expressed as a B::getInstance() or B::instance() call during either
     * A::A() or A::initSingleton(), B will be cleaned up after A.
     *
     * If a cleanupSingleton() method throws an exception, the exception is
     * logged, but cleanupAll() attempts to continue calling the rest of the
     * cleanupSingleton() methods.
     */
    static void cleanupAll();
    /**
     * Call this to call the deleteSingleton() method for every LLSingleton
     * constructed since the start of the last deleteAll() call. (Any
     * LLSingleton constructed DURING a deleteAll() call won't be cleaned up
     * until the next deleteAll() call.) deleteSingleton() deletes and
     * destroys its LLSingleton. Any cleanup logic that might take significant
     * realtime -- or throw an exception -- must not be placed in your
     * LLSingleton's destructor, but rather in its cleanupSingleton() method.
     *
     * The most important property of deleteAll() is that deleteSingleton()
     * methods are called in dependency order, leaf classes last. Thus, given
     * two LLSingleton subclasses A and B, if A's dependency on B is properly
     * expressed as a B::getInstance() or B::instance() call during either
     * A::A() or A::initSingleton(), B will be cleaned up after A.
     *
     * If a deleteSingleton() method throws an exception, the exception is
     * logged, but deleteAll() attempts to continue calling the rest of the
     * deleteSingleton() methods.
     */
    static void deleteAll();
};

// support ref_ptr_t
void intrusive_ptr_add_ref(LLSingletonBase::MasterRefcount*);
void intrusive_ptr_release(LLSingletonBase::MasterRefcount*);

// Most of the time, we want LLSingleton_manage_master() to forward its
// methods to real LLSingletonBase methods.
template <class T>
struct LLSingleton_manage_master
{
    void add(LLSingletonBase* sb) { sb->add_master(); }
    void remove(LLSingletonBase* sb) { sb->remove_master(); }
    void push_initializing(LLSingletonBase* sb) { sb->push_initializing(typeid(T).name()); }
    void pop_initializing (LLSingletonBase* sb) { sb->pop_initializing(); }
    LLSingletonBase::list_t& get_initializing(T*) { return LLSingletonBase::get_initializing(); }
};

// But for the specific case of LLSingletonBase::MasterList, don't.
template <>
struct LLSingleton_manage_master<LLSingletonBase::MasterList>
{
    void add(LLSingletonBase*) {}
    void remove(LLSingletonBase*) {}
    void push_initializing(LLSingletonBase*) {}
    void pop_initializing (LLSingletonBase*) {}
    LLSingletonBase::list_t& get_initializing(LLSingletonBase::MasterList* instance)
    {
        return LLSingletonBase::get_initializing_from(instance);
    }
};

// Now we can implement LLSingletonBase's template constructor.
template <typename DERIVED_TYPE>
LLSingletonBase::LLSingletonBase(tag<DERIVED_TYPE>):
    mCleaned(false),
    mDeleteSingleton(NULL)
{
    // Make this the currently-initializing LLSingleton.
    LLSingleton_manage_master<DERIVED_TYPE>().push_initializing(this);
}

/**
 * LLSingleton implements the getInstance() method part of the Singleton
 * pattern. It can't make the derived class constructors protected, though, so
 * you have to do that yourself.
 *
 * Derive your class from LLSingleton, passing your subclass name as
 * LLSingleton's template parameter, like so:
 *
 *   class Foo: public LLSingleton<Foo>
 *   {
 *       // use this macro at start of every LLSingleton subclass
 *       LLSINGLETON(Foo);
 *   public:
 *       // ...
 *   };
 *
 *   Foo& instance = Foo::instance();
 *
 * LLSingleton recognizes a couple special methods in your derived class.
 *
 * If you override LLSingleton<T>::initSingleton(), your method will be called
 * immediately after the instance is constructed. This is useful for breaking
 * circular dependencies: if you find that your LLSingleton subclass
 * constructor references other LLSingleton subclass instances in a chain
 * leading back to yours, move the instance reference from your constructor to
 * your initSingleton() method.
 *
 * If you override LLSingleton<T>::cleanupSingleton(), your method will be
 * called if someone calls LLSingletonBase::cleanupAll(). The significant part
 * of this promise is that cleanupAll() will call individual
 * cleanupSingleton() methods in reverse dependency order.
 *
 * That is, consider LLSingleton subclasses C, B and A. A depends on B, which
 * in turn depends on C. These dependencies are expressed as calls to
 * B::instance() or B::getInstance(), and C::instance() or C::getInstance().
 * It shouldn't matter whether these calls appear in A::A() or
 * A::initSingleton(), likewise B::B() or B::initSingleton().
 *
 * We promise that if you later call LLSingletonBase::cleanupAll():
 * 1. A::cleanupSingleton() will be called before
 * 2. B::cleanupSingleton(), which will be called before
 * 3. C::cleanupSingleton().
 * Put differently, if your LLSingleton subclass constructor or
 * initSingleton() method explicitly depends on some other LLSingleton
 * subclass, you may continue to rely on that other subclass in your
 * cleanupSingleton() method.
 *
 * We introduce a special cleanupSingleton() method because cleanupSingleton()
 * operations can involve nontrivial realtime, or might throw an exception. A
 * destructor should do neither!
 *
 * If your cleanupSingleton() method throws an exception, we log that
 * exception but proceed with the remaining cleanupSingleton() calls.
 *
 * Similarly, if at some point you call LLSingletonBase::deleteAll(), all
 * remaining LLSingleton instances will be destroyed in dependency order. (Or
 * call MySubclass::deleteSingleton() to specifically destroy the canonical
 * MySubclass instance.)
 *
 * As currently written, LLSingleton is not thread-safe.
 */
template <typename DERIVED_TYPE>
class LLSingleton : public LLSingletonBase
{
private:
    static DERIVED_TYPE* constructSingleton()
    {
        return new DERIVED_TYPE();
    }

    // We know of no way to instruct the compiler that every subclass
    // constructor MUST be private. However, we can make the LLSINGLETON()
    // macro both declare a private constructor and provide the required
    // friend declaration. How can we ensure that every subclass uses
    // LLSINGLETON()? By making that macro provide a definition for this pure
    // virtual method. If you get "can't instantiate class due to missing pure
    // virtual method" for this method, then add LLSINGLETON(yourclass) in the
    // subclass body.
    virtual void you_must_use_LLSINGLETON_macro() = 0;

    // stores pointer to singleton instance
    struct SingletonLifetimeManager
    {
        SingletonLifetimeManager():
            mMasterRefcount(LLSingletonBase::get_master_refcount())
        {
            construct();
        }

        static void construct()
        {
            sData.mInitState = CONSTRUCTING;
            sData.mInstance = constructSingleton();
            sData.mInitState = INITIALIZING;
        }

        ~SingletonLifetimeManager()
        {
            // The dependencies between LLSingletons, and the arbitrary order
            // of static-object destruction, mean that we DO NOT WANT this
            // destructor to delete this LLSingleton. This destructor will run
            // without regard to any other LLSingleton whose cleanup might
            // depend on its existence. What we really want is to count the
            // runtime's attempts to cleanup LLSingleton static data -- and on
            // the very last one, call LLSingletonBase::deleteAll(). That
            // method will properly honor cross-LLSingleton dependencies. This
            // is why we store an intrusive_ptr to a MasterRefcount: our
            // ref_ptr_t member counts SingletonLifetimeManager instances.
            // Once the runtime destroys the last of these, THEN we can delete
            // every remaining LLSingleton.
        }

        LLSingletonBase::ref_ptr_t mMasterRefcount;
    };

protected:
    // Pass DERIVED_TYPE explicitly to LLSingletonBase's constructor because,
    // until our subclass constructor completes, *this isn't yet a
    // full-fledged DERIVED_TYPE.
    LLSingleton(): LLSingletonBase(LLSingletonBase::tag<DERIVED_TYPE>())
    {
        // populate base-class function pointer with the static
        // deleteSingleton() function for this particular specialization
        mDeleteSingleton = &deleteSingleton;

        // add this new instance to the master list
        LLSingleton_manage_master<DERIVED_TYPE>().add(this);
    }

public:
    virtual ~LLSingleton()
    {
        // remove this instance from the master list
        LLSingleton_manage_master<DERIVED_TYPE>().remove(this);
        sData.mInstance = NULL;
        sData.mInitState = DELETED;
    }

    /**
     * @brief Immediately delete the singleton.
     *
     * A subsequent call to LLProxy::getInstance() will construct a new
     * instance of the class.
     *
     * Without an explicit call to LLSingletonBase::deleteAll(), LLSingletons
     * are implicitly destroyed after main() has exited and the C++ runtime is
     * cleaning up statically-constructed objects. Some classes derived from
     * LLSingleton have objects that are part of a runtime system that is
     * terminated before main() exits. Calling the destructor of those objects
     * after the termination of their respective systems can cause crashes and
     * other problems during termination of the project. Using this method to
     * destroy the singleton early can prevent these crashes.
     *
     * An example where this is needed is for a LLSingleton that has an APR
     * object as a member that makes APR calls on destruction. The APR system is
     * shut down explicitly before main() exits. This causes a crash on exit.
     * Using this method before the call to apr_terminate() and NOT calling
     * getInstance() again will prevent the crash.
     */
    static void deleteSingleton()
    {
        delete sData.mInstance;
        sData.mInstance = NULL;
        sData.mInitState = DELETED;
    }

    static DERIVED_TYPE* getInstance()
    {
        static SingletonLifetimeManager sLifeTimeMgr;

        switch (sData.mInitState)
        {
        case UNINITIALIZED:
            // should never be uninitialized at this point
            logerrs("Uninitialized singleton ",
                    demangle(typeid(DERIVED_TYPE).name()).c_str());
            return NULL;

        case CONSTRUCTING:
            logerrs("Tried to access singleton ",
                    demangle(typeid(DERIVED_TYPE).name()).c_str(),
                    " from singleton constructor!");
            return NULL;

        case INITIALIZING:
            // go ahead and flag ourselves as initialized so we can be
            // reentrant during initialization
            sData.mInitState = INITIALIZED; 
            // initialize singleton after constructing it so that it can
            // reference other singletons which in turn depend on it, thus
            // breaking cyclic dependencies
            sData.mInstance->initSingleton();
            // pop this off stack of initializing singletons
            LLSingleton_manage_master<DERIVED_TYPE>().pop_initializing(sData.mInstance);
            break;

        case INITIALIZED:
            break;

        case DELETED:
            logwarns("Trying to access deleted singleton ",
                     demangle(typeid(DERIVED_TYPE).name()).c_str(),
                     " -- creating new instance");
            SingletonLifetimeManager::construct();
            // same as first time construction
            sData.mInitState = INITIALIZED; 
            sData.mInstance->initSingleton(); 
            // pop this off stack of initializing singletons
            LLSingleton_manage_master<DERIVED_TYPE>().pop_initializing(sData.mInstance);
            break;
        }

        // By this point, if DERIVED_TYPE was pushed onto the initializing
        // stack, it has been popped off. So the top of that stack, if any, is
        // an LLSingleton that directly depends on DERIVED_TYPE. If this call
        // came from another LLSingleton, rather than from vanilla application
        // code, record the dependency.
        sData.mInstance->capture_dependency(
            LLSingleton_manage_master<DERIVED_TYPE>().get_initializing(sData.mInstance),
            sData.mInitState);
        return sData.mInstance;
    }

    // Reference version of getInstance()
    // Preferred over getInstance() as it disallows checking for NULL
    static DERIVED_TYPE& instance()
    {
        return *getInstance();
    }

    // Has this singleton been created yet?
    // Use this to avoid accessing singletons before they can safely be constructed.
    static bool instanceExists()
    {
        return sData.mInitState == INITIALIZED;
    }

    // Has this singleton been deleted? This can be useful during shutdown
    // processing to avoid "resurrecting" a singleton we thought we'd already
    // cleaned up.
    static bool wasDeleted()
    {
        return sData.mInitState == DELETED;
    }

private:
    struct SingletonData
    {
        // explicitly has a default constructor so that member variables are zero initialized in BSS
        // and only changed by singleton logic, not constructor running during startup
        EInitState      mInitState;
        DERIVED_TYPE*   mInstance;
    };
    static SingletonData sData;
};

template<typename T>
typename LLSingleton<T>::SingletonData LLSingleton<T>::sData;

/**
 * Use LLSINGLETON(Foo); at the start of an LLSingleton<Foo> subclass body
 * when you want to declare an out-of-line constructor:
 *
 * @code
 *   class Foo: public LLSingleton<Foo>
 *   {
 *       // use this macro at start of every LLSingleton subclass
 *       LLSINGLETON(Foo);
 *   public:
 *       // ...
 *   };
 *   // ...
 *   [inline]
 *   Foo::Foo() { ... }
 * @endcode
 *
 * Unfortunately, this mechanism does not permit you to define even a simple
 * (but nontrivial) constructor within the class body. If it's literally
 * trivial, use LLSINGLETON_EMPTY_CTOR(); if not, use LLSINGLETON() and define
 * the constructor outside the class body. If you must define it in a header
 * file, use 'inline' (unless it's a template class) to avoid duplicate-symbol
 * errors at link time.
 */
#define LLSINGLETON(DERIVED_CLASS)                                      \
private:                                                                \
    /* implement LLSingleton pure virtual method whose sole purpose */  \
    /* is to remind people to use this macro */                         \
    virtual void you_must_use_LLSINGLETON_macro() {}                    \
    friend class LLSingleton<DERIVED_CLASS>;                            \
    DERIVED_CLASS()

/**
 * Use LLSINGLETON_EMPTY_CTOR(Foo); at the start of an LLSingleton<Foo>
 * subclass body when the constructor is trivial:
 *
 * @code
 *   class Foo: public LLSingleton<Foo>
 *   {
 *       // use this macro at start of every LLSingleton subclass
 *       LLSINGLETON_EMPTY_CTOR(Foo);
 *   public:
 *       // ...
 *   };
 * @endcode
 */
#define LLSINGLETON_EMPTY_CTOR(DERIVED_CLASS)                           \
    /* LLSINGLETON() is carefully implemented to permit exactly this */ \
    LLSINGLETON(DERIVED_CLASS) {}

#endif
