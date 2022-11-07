/**
 * @file   llinitdestroyclass.h
 * @author Nat Goodspeed
 * @date   2015-05-27
 * @brief  LLInitClass / LLDestroyClass mechanism
 *
 * The LLInitClass template, extracted from llui.h, ensures that control will
 * reach a static initClass() method. LLDestroyClass does the same for a
 * static destroyClass() method.
 *
 * The distinguishing characteristics of these templates are:
 *
 * - All LLInitClass<T>::initClass() methods are triggered by an explicit call
 *   to LLInitClassList::instance().fireCallbacks(). Presumably this call
 *   happens sometime after all static objects in the program have been
 *   initialized. In other words, each LLInitClass<T>::initClass() method
 *   should be able to make some assumptions about global program state.
 *
 * - Similarly, LLDestroyClass<T>::destroyClass() methods are triggered by
 *   LLDestroyClassList::instance().fireCallbacks(). Again, presumably this
 *   happens at a well-defined moment in the program's shutdown sequence.
 *
 * - The initClass() calls happen in an unspecified sequence. You may not rely
 *   on the relative ordering of LLInitClass<T>::initClass() versus another
 *   LLInitClass<U>::initClass() method. If you need such a guarantee, use
 *   LLSingleton instead and make the dependency explicit.
 *
 * - Similarly, LLDestroyClass<T>::destroyClass() may happen either before or
 *   after LLDestroyClass<U>::destroyClass(). You cannot rely on that order.
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Copyright (c) 2015, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLINITDESTROYCLASS_H)
#define LL_LLINITDESTROYCLASS_H

#include "llsingleton.h"
#include <boost/function.hpp>
#include <typeinfo>
#include <vector>
#include <utility>                  // std::pair

/**
 * LLCallbackRegistry is an implementation detail base class for
 * LLInitClassList and LLDestroyClassList. It accumulates the initClass() or
 * destroyClass() callbacks for registered classes.
 */
class LLCallbackRegistry
{
public:
    typedef boost::function<void()> func_t;

    void registerCallback(const std::string& name, const func_t& func)
    {
        mCallbacks.push_back(FuncList::value_type(name, func));
    }

    void fireCallbacks() const;

private:
    // Arguably this should be a boost::signals2::signal, which is, after all,
    // a sequence of callables. We manage it by hand so we can log a name for
    // each registered function we call.
    typedef std::vector< std::pair<std::string, func_t> > FuncList;
    FuncList mCallbacks;
};

/**
 * LLInitClassList is the LLCallbackRegistry for LLInitClass. It stores the
 * registered initClass() methods. It must be an LLSingleton because
 * LLInitClass registers its initClass() method at static construction time
 * (before main()), requiring LLInitClassList to be fully constructed on
 * demand regardless of module initialization order.
 */
class LLInitClassList : 
    public LLCallbackRegistry, 
    public LLSingleton<LLInitClassList>
{
    LLSINGLETON_EMPTY_CTOR(LLInitClassList);
};

/**
 * LLDestroyClassList is the LLCallbackRegistry for LLDestroyClass. It stores
 * the registered destroyClass() methods. It must be an LLSingleton because
 * LLDestroyClass registers its destroyClass() method at static construction
 * time (before main()), requiring LLDestroyClassList to be fully constructed
 * on demand regardless of module initialization order.
 */
class LLDestroyClassList : 
    public LLCallbackRegistry, 
    public LLSingleton<LLDestroyClassList>
{
    LLSINGLETON_EMPTY_CTOR(LLDestroyClassList);
};

/**
 * LLRegisterWith is an implementation detail for LLInitClass and
 * LLDestroyClass. It is intended to be used as a static class member whose
 * constructor registers the specified callback with the LLMumbleClassList
 * singleton registry specified as the template argument.
 */
template<typename T>
class LLRegisterWith
{
public:
    LLRegisterWith(const std::string& name, const LLCallbackRegistry::func_t& func)
    {
        T::instance().registerCallback(name, func);
    }

    // this avoids a MSVC bug where non-referenced static members are "optimized" away
    // even if their constructors have side effects
    S32 reference()
    {
        S32 dummy;
        dummy = 0;
        return dummy;
    }
};

/**
 * Derive MyClass from LLInitClass<MyClass> (the Curiously Recurring Template
 * Pattern) to ensure that the static method MyClass::initClass() will be
 * called (along with all other LLInitClass<T> subclass initClass() methods)
 * when someone calls LLInitClassList::instance().fireCallbacks(). This gives
 * the application specific control over the timing of all such
 * initializations, without having to insert calls for every such class into
 * generic application code.
 */
template<typename T>
class LLInitClass
{
public:
    LLInitClass() { sRegister.reference(); }

    // When this static member is initialized, the subclass initClass() method
    // is registered on LLInitClassList. See sRegister definition below.
    static LLRegisterWith<LLInitClassList> sRegister;
};

/**
 * Derive MyClass from LLDestroyClass<MyClass> (the Curiously Recurring
 * Template Pattern) to ensure that the static method MyClass::destroyClass()
 * will be called (along with other LLDestroyClass<T> subclass destroyClass()
 * methods) when someone calls LLDestroyClassList::instance().fireCallbacks().
 * This gives the application specific control over the timing of all such
 * cleanup calls, without having to insert calls for every such class into
 * generic application code.
 */
template<typename T>
class LLDestroyClass
{
public:
    LLDestroyClass() { sRegister.reference(); }

    // When this static member is initialized, the subclass destroyClass()
    // method is registered on LLInitClassList. See sRegister definition
    // below.
    static LLRegisterWith<LLDestroyClassList> sRegister;
};

// Here's where LLInitClass<T> specifies the subclass initClass() method.
template <typename T>
LLRegisterWith<LLInitClassList>
LLInitClass<T>::sRegister(std::string(typeid(T).name()) + "::initClass",
                          &T::initClass);
// Here's where LLDestroyClass<T> specifies the subclass destroyClass() method.
template <typename T>
LLRegisterWith<LLDestroyClassList>
LLDestroyClass<T>::sRegister(std::string(typeid(T).name()) + "::destroyClass",
                             &T::destroyClass);

#endif /* ! defined(LL_LLINITDESTROYCLASS_H) */
