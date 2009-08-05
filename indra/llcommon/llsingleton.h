/** 
 * @file llsingleton.h
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#ifndef LLSINGLETON_H
#define LLSINGLETON_H

#include "llerror.h"	// *TODO: eliminate this

#include <typeinfo>
#include <boost/noncopyable.hpp>
#include <boost/any.hpp>

/// @brief A global registry of all singletons to prevent duplicate allocations
/// across shared library boundaries
class LL_COMMON_API LLSingletonRegistry {
	private:
		typedef std::map<std::string, void *> TypeMap;
		static TypeMap sSingletonMap;

	public:
		template<typename T> static void * & get()
		{
			std::string name(typeid(T).name());

			if(0 == sSingletonMap.count(name))
			{
				sSingletonMap[name] = NULL;
			}

			return sSingletonMap[typeid(T).name()];
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
	
	static void deleteSingleton()
	{
		delete getData().mSingletonInstance;
		getData().mSingletonInstance = NULL;
	}
	
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
			deleteSingleton();
		}
	};
	
public:
	virtual ~LLSingleton()
	{
		SingletonInstanceData& data = getData();
		data.mSingletonInstance = NULL;
		data.mInitState = DELETED;
	}

	static SingletonInstanceData& getData()
	{
		void * & registry = LLSingletonRegistry::get<DERIVED_TYPE>();
		static SingletonInstanceData data;

		// *TODO - look into making this threadsafe
		if(NULL == registry)
		{
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
