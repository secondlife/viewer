/**
 * @file llfunctorregistry.h
 * @author Kent Quirk
 * @brief Maintains a registry of named callback functors taking a single LLSD parameter
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#ifndef LL_LLFUNCTORREGISTRY_H
#define LL_LLFUNCTORREGISTRY_H

#include <string>
#include <map>

#include <boost/function.hpp>

#include "llsd.h"
#include "llsingleton.h"

/**
 * @class LLFunctorRegistry
 * @brief Maintains a collection of named functors for remote binding
 * (mainly for use in callbacks from notifications and other signals)
 * @see LLNotifications
 *
 * This class maintains a collection of named functors in a singleton.
 * We wanted to be able to persist notifications with their callbacks
 * across restarts of the viewer; we couldn't store functors that way.
 * Using this registry, systems that require a functor to be maintained
 * long term can register it at system startup, and then pass in the
 * functor by name. 
 */

template <typename FUNCTOR_TYPE>
class LLFunctorRegistry : public LLSingleton<LLFunctorRegistry<FUNCTOR_TYPE> >
{
	friend class LLSingleton<LLFunctorRegistry>;
	LOG_CLASS(LLFunctorRegistry);
private:
	LLFunctorRegistry() : LOGFUNCTOR("LogFunctor"), DONOTHING("DoNothing")
	{
		mMap[LOGFUNCTOR] = log_functor;
		mMap[DONOTHING] = do_nothing;
	}

public:
	typedef FUNCTOR_TYPE ResponseFunctor;
	typedef typename std::map<std::string, FUNCTOR_TYPE> FunctorMap;
	
	bool registerFunctor(const std::string& name, ResponseFunctor f)
	{
		bool retval = true;
		if (mMap.count(name) == 0)
		{
			mMap[name] = f;
		}
		else
		{
			llerrs << "attempt to store duplicate name '" << name << "' in LLFunctorRegistry. NOT ADDED." << llendl;
			retval = false;
		}
		
		return retval;
	}

	bool unregisterFunctor(const std::string& name)
	{
		if (mMap.count(name) == 0)
		{
			llwarns << "trying to remove '" << name << "' from LLFunctorRegistry but it's not there." << llendl;
			return false;
		}
		mMap.erase(name);
		return true;
	}

	FUNCTOR_TYPE getFunctor(const std::string& name)
	{
		if (mMap.count(name) != 0)
		{
			return mMap[name];
		}
		else
		{
			lldebugs << "tried to find '" << name << "' in LLFunctorRegistry, but it wasn't there." << llendl;
			return mMap[LOGFUNCTOR];
		}
	}

	const std::string LOGFUNCTOR;
	const std::string DONOTHING;
	
private:

	static void log_functor(const LLSD& notification, const LLSD& payload)
	{
		lldebugs << "log_functor called with payload: " << payload << llendl;
	}

	static void do_nothing(const LLSD& notification, const LLSD& payload)
	{
		// what the sign sez
	}

	FunctorMap mMap;
};

template <typename FUNCTOR_TYPE>
class LLFunctorRegistration
{
public:
	LLFunctorRegistration(const std::string& name, FUNCTOR_TYPE functor) 
	{
		LLFunctorRegistry<FUNCTOR_TYPE>::instance().registerFunctor(name, functor);
	}
};

#endif//LL_LLFUNCTORREGISTRY_H

