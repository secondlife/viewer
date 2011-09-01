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

#include "llerror.h"	// *TODO: eliminate this

#include <typeinfo>
#include <boost/noncopyable.hpp>

/// @brief A global registry of all singletons to prevent duplicate allocations
/// across shared library boundaries
class LL_COMMON_API LLSingletonRegistry {
	private:
		typedef std::map<std::string, void *> TypeMap;
		static TypeMap * sSingletonMap;

		static void checkInit()
		{
			if(sSingletonMap == NULL)
			{
				sSingletonMap = new TypeMap();
			}
		}

	public:
		template<typename T> static void * & get()
		{
			std::string name(typeid(T).name());

			checkInit();

			// the first entry of the pair returned by insert will be either the existing
			// iterator matching our key, or the newly inserted NULL initialized entry
			// see "Insert element" in http://www.sgi.com/tech/stl/UniqueAssociativeContainer.html
			TypeMap::iterator result =
				sSingletonMap->insert(std::make_pair(name, (void*)NULL)).first;

			return result->second;
		}
};

// LLSingleton implements the getInstance() method part of the Singleton
// pattern. It can't make the derived class constructors protected, though, so
// you have to do that yourself.
//
// There are two ways to use LLSingleton. The first way is to inherit from it
// while using the typename that you'd like to be static as the template
// parameter, like so:
//
//   class Foo: public LLSingleton<Foo>{};
//
//   Foo& instance = Foo::instance();
//
// The second way is to use the singleton class directly, without inheritance:
//
//   typedef LLSingleton<Foo> FooSingleton;
//
//   Foo& instance = FooSingleton::instance();
//
// In this case, the class being managed as a singleton needs to provide an
// initSingleton() method since the LLSingleton virtual method won't be
// available
//
// As currently written, it is not thread-safe.

template <typename DERIVED_TYPE>
class LLSingleton : private boost::noncopyable
{
	
private:
	typedef enum e_init_state
	{
		UNINITIALIZED,
		CONSTRUCTING,
		INITIALIZING,
		INITIALIZED,
		DELETED
	} EInitState;
	
	// stores pointer to singleton instance
	// and tracks initialization state of singleton
	struct SingletonInstanceData
	{
		EInitState		mInitState;
		DERIVED_TYPE*	mSingletonInstance;
		
		SingletonInstanceData()
		:	mSingletonInstance(NULL),
			mInitState(UNINITIALIZED)
		{}

		~SingletonInstanceData()
		{
			if (mInitState != DELETED)
			{
				deleteSingleton();
			}
		}
	};
	
public:
	virtual ~LLSingleton()
	{
		SingletonInstanceData& data = getData();
		data.mSingletonInstance = NULL;
		data.mInitState = DELETED;
	}

	/**
	 * @brief Immediately delete the singleton.
	 *
	 * A subsequent call to LLProxy::getInstance() will construct a new
	 * instance of the class.
	 *
	 * LLSingletons are normally destroyed after main() has exited and the C++
	 * runtime is cleaning up statically-constructed objects. Some classes
	 * derived from LLSingleton have objects that are part of a runtime system
	 * that is terminated before main() exits. Calling the destructor of those
	 * objects after the termination of their respective systems can cause
	 * crashes and other problems during termination of the project. Using this
	 * method to destroy the singleton early can prevent these crashes.
	 *
	 * An example where this is needed is for a LLSingleton that has an APR
	 * object as a member and make APR calls on destruction. The APR system is
	 * shut down explicitly before main() exits. This causes a crash on exit.
	 * Using this method before the call to apr_terminate() and NOT calling
	 * getInstance() again will prevent the crash.
	 */
	static void deleteSingleton()
	{
		delete getData().mSingletonInstance;
		getData().mSingletonInstance = NULL;
		getData().mInitState = DELETED;
	}

	static SingletonInstanceData& getData()
	{
		// this is static to cache the lookup results
		static void * & registry = LLSingletonRegistry::get<DERIVED_TYPE>();

		// *TODO - look into making this threadsafe
		if(NULL == registry)
		{
			static SingletonInstanceData data;
			registry = &data;
		}

		return *static_cast<SingletonInstanceData *>(registry);
	}

	static DERIVED_TYPE* getInstance()
	{
		SingletonInstanceData& data = getData();

		if (data.mInitState == CONSTRUCTING)
		{
			llerrs << "Tried to access singleton " << typeid(DERIVED_TYPE).name() << " from singleton constructor!" << llendl;
		}

		if (data.mInitState == DELETED)
		{
			llwarns << "Trying to access deleted singleton " << typeid(DERIVED_TYPE).name() << " creating new instance" << llendl;
		}
		
		if (!data.mSingletonInstance) 
		{
			data.mInitState = CONSTRUCTING;
			data.mSingletonInstance = new DERIVED_TYPE(); 
			data.mInitState = INITIALIZING;
			data.mSingletonInstance->initSingleton(); 
			data.mInitState = INITIALIZED;	
		}
		
		return data.mSingletonInstance;
	}

	// Reference version of getInstance()
	// Preferred over getInstance() as it disallows checking for NULL
	static DERIVED_TYPE& instance()
	{
		return *getInstance();
	}
	
	// Has this singleton been created uet?
	// Use this to avoid accessing singletons before the can safely be constructed
	static bool instanceExists()
	{
		return getData().mInitState == INITIALIZED;
	}
	
	// Has this singleton already been deleted?
	// Use this to avoid accessing singletons from a static object's destructor
	static bool destroyed()
	{
		return getData().mInitState == DELETED;
	}

private:
	virtual void initSingleton() {}
};

#endif
