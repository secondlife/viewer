/** 
 * @file llregistry.h
 * @brief template classes for registering name, value pairs in nested scopes, statically, etc.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLREGISTRY_H
#define LL_LLREGISTRY_H

#include <list>

#include "llsingleton.h"
#include "llstl.h"

template <typename T>
struct LLRegistryDefaultComparator
{
	bool operator()(const T& lhs, const T& rhs) const
	{
		using std::less;
		return less<T>()(lhs, rhs);
	}
};

template <typename KEY, typename VALUE, typename COMPARATOR = LLRegistryDefaultComparator<KEY> >
class LLRegistry
{
public:
	typedef LLRegistry<KEY, VALUE, COMPARATOR>		registry_t;
	typedef const KEY& 								ref_const_key_t;
	typedef const VALUE&							ref_const_value_t;
	typedef const VALUE*							ptr_const_value_t;
	typedef VALUE*									ptr_value_t;

	class Registrar
	{
		friend class LLRegistry<KEY, VALUE, COMPARATOR>;
	public:
		typedef std::map<KEY, VALUE, COMPARATOR> registry_map_t;

		bool add(ref_const_key_t key, ref_const_value_t value)
		{
			if (mMap.insert(std::make_pair(key, value)).second == false)
			{
				LL_WARNS() << "Tried to register " << key << " but it was already registered!" << LL_ENDL;
				return false;
			}
			return true;
		}

		void remove(ref_const_key_t key)
		{
			mMap.erase(key);
		}

		void replace(ref_const_key_t key, ref_const_value_t value)
		{
			mMap[key] = value;
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
		for(Registrar* scope : mActiveScopes)
		{
			ptr_value_t valuep = scope->getValue(key);
			if (valuep != NULL) return valuep;
		}
		return mDefaultRegistrar.getValue(key);
	}

	ptr_const_value_t getValue(ref_const_key_t key) const
	{
		for(const Registrar* scope : mActiveScopes)
		{
			ptr_const_value_t valuep = scope->getValue(key);
			if (valuep != NULL) return valuep;
		}
		return mDefaultRegistrar.getValue(key);
	}

	bool exists(ref_const_key_t key) const
	{
		for(const Registrar* scope : mActiveScopes)
		{
			if (scope->exists(key)) return true;
		}

		return mDefaultRegistrar.exists(key);
	}

	bool empty() const
	{
		for(const Registrar* scope : mActiveScopes)
		{
			if (!scope->empty()) return false;
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
	// This LLRegistrySingleton doesn't use LLSINGLETON(LLRegistrySingleton)
	// because the concrete class is actually DERIVED_TYPE, not
	// LLRegistrySingleton. So each concrete subclass needs
	// LLSINGLETON(whatever) -- not this intermediate base class.
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
			if (singleton_t::instanceExists())
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
			if (singleton_t::instance().exists(key))
			{
				LL_ERRS() << "Duplicate registry entry under key \"" << key << "\"" << LL_ENDL;
			}
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

// helper macro for doing static registration
#define GLUED_TOKEN(x, y) x ## y
#define GLUE_TOKENS(x, y) GLUED_TOKEN(x, y)
#define LLREGISTER_STATIC(REGISTRY, KEY, VALUE) static REGISTRY::StaticRegistrar GLUE_TOKENS(reg, __LINE__)(KEY, VALUE);

#endif
