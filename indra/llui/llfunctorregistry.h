/**
 * @file llfunctorregistry.h
 * @author Kent Quirk
 * @brief Maintains a registry of named callback functors taking a single LLSD parameter
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 *
 * Copyright (c) 2003-2007, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLFUNCTORREGISTRY_H
#define LL_LLFUNCTORREGISTRY_H

#include <string>
#include <map>

#include <boost/function.hpp>

#include "llsd.h"
#include "llmemory.h"

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
		typename FunctorMap::iterator it = mMap.find(name);
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
		typename FunctorMap::iterator it = mMap.find(name);
		if (mMap.count(name) != 0)
		{
			return mMap[name];
		}
		else
		{
			llwarns << "tried to find '" << name << "' in LLFunctorRegistry, but it wasn't there." << llendl;
			return mMap[LOGFUNCTOR];
		}
	}

	const std::string LOGFUNCTOR;
	const std::string DONOTHING;
	
private:

	static void log_functor(const LLSD& notification, const LLSD& payload)
	{
		llwarns << "log_functor called with payload: " << payload << llendl;
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

