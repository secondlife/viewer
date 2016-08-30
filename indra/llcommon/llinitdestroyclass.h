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

#include "llerror.h"
#include "llsingleton.h"
#include <boost/function.hpp>
#include <boost/signals2/signal.hpp>
#include <typeinfo>

/**
 * LLCallbackRegistry is an implementation detail base class for
 * LLInitClassList and LLDestroyClassList. It's a very thin wrapper around a
 * Boost.Signals2 signal object.
 */
class LLCallbackRegistry
{
public:
	typedef boost::signals2::signal<void()> callback_signal_t;
	
	void registerCallback(const callback_signal_t::slot_type& slot)
	{
		mCallbacks.connect(slot);
	}

	void fireCallbacks()
	{
		mCallbacks();
	}

private:
	callback_signal_t mCallbacks;
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
	friend class LLSingleton<LLInitClassList>;
private:
	LLInitClassList() {}
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
	friend class LLSingleton<LLDestroyClassList>;
private:
	LLDestroyClassList() {}
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
	LLRegisterWith(boost::function<void ()> func)
	{
		T::instance().registerCallback(func);
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
private:

	// Provide a default initClass() method in case subclass misspells (or
	// omits) initClass(). This turns a potential build error into a fatal
	// runtime error.
	static void initClass()
	{
		LL_ERRS() << "No static initClass() method defined for " << typeid(T).name() << LL_ENDL;
	}
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
private:

	// Provide a default destroyClass() method in case subclass misspells (or
	// omits) destroyClass(). This turns a potential build error into a fatal
	// runtime error.
	static void destroyClass()
	{
		LL_ERRS() << "No static destroyClass() method defined for " << typeid(T).name() << LL_ENDL;
	}
};

// Here's where LLInitClass<T> specifies the subclass initClass() method.
template <typename T> LLRegisterWith<LLInitClassList> LLInitClass<T>::sRegister(&T::initClass);
// Here's where LLDestroyClass<T> specifies the subclass destroyClass() method.
template <typename T> LLRegisterWith<LLDestroyClassList> LLDestroyClass<T>::sRegister(&T::destroyClass);

#endif /* ! defined(LL_LLINITDESTROYCLASS_H) */
