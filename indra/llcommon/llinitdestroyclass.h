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

class LLInitClassList : 
	public LLCallbackRegistry, 
	public LLSingleton<LLInitClassList>
{
	friend class LLSingleton<LLInitClassList>;
private:
	LLInitClassList() {}
};

class LLDestroyClassList : 
	public LLCallbackRegistry, 
	public LLSingleton<LLDestroyClassList>
{
	friend class LLSingleton<LLDestroyClassList>;
private:
	LLDestroyClassList() {}
};

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

template<typename T>
class LLInitClass
{
public:
	LLInitClass() { sRegister.reference(); }

	static LLRegisterWith<LLInitClassList> sRegister;
private:

	static void initClass()
	{
		LL_ERRS() << "No static initClass() method defined for " << typeid(T).name() << LL_ENDL;
	}
};

template<typename T>
class LLDestroyClass
{
public:
	LLDestroyClass() { sRegister.reference(); }

	static LLRegisterWith<LLDestroyClassList> sRegister;
private:

	static void destroyClass()
	{
		LL_ERRS() << "No static destroyClass() method defined for " << typeid(T).name() << LL_ENDL;
	}
};

template <typename T> LLRegisterWith<LLInitClassList> LLInitClass<T>::sRegister(&T::initClass);
template <typename T> LLRegisterWith<LLDestroyClassList> LLDestroyClass<T>::sRegister(&T::destroyClass);

#endif /* ! defined(LL_LLINITDESTROYCLASS_H) */
