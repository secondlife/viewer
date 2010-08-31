/** 
 * @file llregistry.h
 * @brief template classes for registering name, value pairs in nested scopes, statically, etc.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLREGISTRY_H
#define LL_LLREGISTRY_H

#include <list>

#include <boost/type_traits.hpp>
#include "llsingleton.h"

template <typename T>
class LLRegistryDefaultComparator
{
	bool operator()(const T& lhs, const T& rhs) { return lhs < rhs; }
};

template <typename KEY, typename VALUE, typename COMPARATOR = LLRegistryDefaultComparator<KEY> >
class LLRegistry
{
public:
	typedef LLRegistry<KEY, VALUE, COMPARATOR>											registry_t;
	typedef typename boost::add_reference<typename boost::add_const<KEY>::type>::type	ref_const_key_t;
	typedef typename boost::add_reference<typename boost::add_const<VALUE>::type>::type	ref_const_value_t;
	typedef typename boost::add_reference<VALUE>::type									ref_value_t;
	typedef typename boost::add_pointer<typename boost::add_const<VALUE>::type>::type	ptr_const_value_t;
	typedef typename boost::add_pointer<VALUE>::type									ptr_value_t;

	class Registrar
	{
		friend class LLRegistry<KEY, VALUE, COMPARATOR>;
	public:
		typedef typename std::map<KEY, VALUE> registry_map_t;

		bool add(ref_const_key_t key, ref_const_value_t value)
		{
			if (mMap.insert(std::make_pair(key, value)).second == false)
			{
				llwarns << "Tried to register " << key << " but it was already registered!" << llendl;
				return false;
			}
			return true;
		}

		void remove(ref_const_key_t key)
		{
			mMap.erase(key);
		}

		typename registry_map_t::const_iterator beginItems() const
		{
			return mMap.begin();
		}

		typename registry_map_t::const_iterator endItems() const
		{
			return mMap.end();
		}

	protected:
		ptr_value_t getValue(ref_const_key_t key)
		{
			typename registry_map_t::iterator found_it = mMap.find(key);
			if (found_it != mMap.end())
			{
				return &(found_it->second);
			}
			return NULL;
		}

		ptr_const_value_t getValue(ref_const_key_t key) const
		{
			typename registry_map_t::const_iterator found_it = mMap.find(key);
			if (found_it != mMap.end())
			{
				return &(found_it->second);
			}
			return NULL;
		}

		// if the registry is used to store pointers, and null values are valid entries
		// then use this function to check the existence of an entry
		bool exists(ref_const_key_t key) const
		{
			return mMap.find(key) != mMap.end();
		}

		bool empty() const
		{
			return mMap.empty();
		}

	protected:
		// use currentRegistrar() or defaultRegistrar()
		Registrar() {}
		~Registrar() {}

	private:
		registry_map_t											mMap;
	};
	
	typedef typename std::list<Registrar*> scope_list_t;
	typedef typename std::list<Registrar*>::iterator scope_list_iterator_t;
	typedef typename std::list<Registrar*>::const_iterator scope_list_const_iterator_t;
	
	LLRegistry() 
	{}

	~LLRegistry() {}

	ptr_value_t getValue(ref_const_key_t key)
	{
		for(scope_list_iterator_t it = mActiveScopes.begin();
			it != mActiveScopes.end();
			++it)
		{
			ptr_value_t valuep = (*it)->getValue(key);
			if (valuep != NULL) return valuep;
		}
		return mDefaultRegistrar.getValue(key);
	}

	ptr_const_value_t getValue(ref_const_key_t key) const
	{
		for(scope_list_const_iterator_t it = mActiveScopes.begin();
			it != mActiveScopes.end();
			++it)
		{
			ptr_value_t valuep = (*it)->getValue(key);
			if (valuep != NULL) return valuep;
		}
		return mDefaultRegistrar.getValue(key);
	}

	bool exists(ref_const_key_t key) const
	{
		for(scope_list_const_iterator_t it = mActiveScopes.begin();
			it != mActiveScopes.end();
			++it)
		{
			if ((*it)->exists(key)) return true;
		}

		return mDefaultRegistrar.exists(key);
	}

	bool empty() const
	{
		for(scope_list_const_iterator_t it = mActiveScopes.begin();
			it != mActiveScopes.end();
			++it)
		{
			if (!(*it)->empty()) return false;
		}

		return mDefaultRegistrar.empty();
	}


	Registrar& defaultRegistrar()
	{
		return mDefaultRegistrar;
	}

	const Registrar& defaultRegistrar() const
	{
		return mDefaultRegistrar;
	}


	Registrar& currentRegistrar()
	{
		if (!mActiveScopes.empty()) 
		{
			return *mActiveScopes.front();
		}

		return mDefaultRegistrar;
	}

	const Registrar& currentRegistrar() const
	{
		if (!mActiveScopes.empty()) 
		{
			return *mActiveScopes.front();
		}

		return mDefaultRegistrar;
	}


protected:
	void addScope(Registrar* scope)
	{
		// newer scopes go up front
		mActiveScopes.insert(mActiveScopes.begin(), scope);
	}

	void removeScope(Registrar* scope)
	{
		// O(N) but should be near the beggining and N should be small and this is safer than storing iterators
		scope_list_iterator_t iter = std::find(mActiveScopes.begin(), mActiveScopes.end(), scope);
		if (iter != mActiveScopes.end())
		{
			mActiveScopes.erase(iter);
		}
	}

private:
	scope_list_t	mActiveScopes;
	Registrar		mDefaultRegistrar;
};

template <typename KEY, typename VALUE, typename DERIVED_TYPE, typename COMPARATOR = LLRegistryDefaultComparator<KEY> >
class LLRegistrySingleton
	:	public LLRegistry<KEY, VALUE, COMPARATOR>,
		public LLSingleton<DERIVED_TYPE>
{
	friend class LLSingleton<DERIVED_TYPE>;
public:
	typedef LLRegistry<KEY, VALUE, COMPARATOR>		registry_t;
	typedef const KEY&								ref_const_key_t;
	typedef const VALUE&							ref_const_value_t;
	typedef VALUE*									ptr_value_t;
	typedef const VALUE*							ptr_const_value_t;
	typedef LLSingleton<DERIVED_TYPE>				singleton_t;

	class ScopedRegistrar : public registry_t::Registrar
	{
	public:
		ScopedRegistrar(bool push_scope = true) 
		{
			if (push_scope)
			{
				pushScope();
			}
		}

		~ScopedRegistrar()
		{
			if (!singleton_t::destroyed())
			{
				popScope();
			}
		}

		void pushScope()
		{
			singleton_t::instance().addScope(this);
		}
		
		void popScope()
		{
			singleton_t::instance().removeScope(this);
		}
		
		ptr_value_t getValueFromScope(ref_const_key_t key)
		{
			return getValue(key);
		}

		ptr_const_value_t getValueFromScope(ref_const_key_t key) const
		{
			return getValue(key);
		}

	private:
		typename std::list<typename registry_t::Registrar*>::iterator	mListIt;
	};

	class StaticRegistrar : public registry_t::Registrar
	{
	public:
		virtual ~StaticRegistrar() {}
		StaticRegistrar(ref_const_key_t key, ref_const_value_t value)
		{
			singleton_t::instance().mStaticScope->add(key, value);
		}
	};

	// convenience functions
	typedef typename LLRegistry<KEY, VALUE, COMPARATOR>::Registrar& ref_registrar_t;
	static ref_registrar_t currentRegistrar()
	{
		return singleton_t::instance().registry_t::currentRegistrar();
	}

	static ref_registrar_t defaultRegistrar()
	{
		return singleton_t::instance().registry_t::defaultRegistrar();
	}
	
	static ptr_value_t getValue(ref_const_key_t key)
	{
		return singleton_t::instance().registry_t::getValue(key);
	}

protected:
	// DERIVED_TYPE needs to derive from LLRegistrySingleton
	LLRegistrySingleton()
		: mStaticScope(NULL)
	{}

	virtual void initSingleton()
	{
		mStaticScope = new ScopedRegistrar();
	}

	virtual ~LLRegistrySingleton() 
	{
		delete mStaticScope;
	}

private:
	ScopedRegistrar*	mStaticScope;
};

#endif
