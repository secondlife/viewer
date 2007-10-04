/** 
 * @file llservice.cpp
 * @author Phoenix
 * @date 2005-04-20
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
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

#include "linden_common.h"
#include "llservice.h"

LLService::creators_t LLService::sCreatorFunctors;

LLService::LLService()
{
}

LLService::~LLService()
{
}

// static
bool LLService::registerCreator(const std::string& name, creator_t fn)
{
	llinfos << "LLService::registerCreator(" << name << ")" << llendl;
	if(name.empty())
	{
		return false;
	}

	creators_t::value_type vt(name, fn);
	std::pair<creators_t::iterator, bool> rv = sCreatorFunctors.insert(vt);
	return rv.second;

	// alternately...
	//std::string name_str(name);
	//sCreatorFunctors[name_str] = fn;
}

// static
LLIOPipe* LLService::activate(
	const std::string& name,
	LLPumpIO::chain_t& chain,
	LLSD context)
{
	if(name.empty())
	{
		llinfos << "LLService::activate - no service specified." << llendl;
		return NULL;
	}
	creators_t::iterator it = sCreatorFunctors.find(name);
	LLIOPipe* rv = NULL;
	if(it != sCreatorFunctors.end())
	{
		if((*it).second->build(chain, context))
		{
			rv = chain[0].get();
		}
		else
		{
			// empty out the chain, because failed service creation
			// should just discard this stuff.
			llwarns << "LLService::activate - unable to build chain: " << name
					<< llendl;
			chain.clear();
		}
	}
	else
	{
		llwarns << "LLService::activate - unable find factory: " << name
				<< llendl;
	}
	return rv;
}

// static
bool LLService::discard(const std::string& name)
{
	if(name.empty())
	{
		return false;
	}
	creators_t::iterator it = sCreatorFunctors.find(name);
	if(it != sCreatorFunctors.end())
	{
		//(*it).second->discard();
		sCreatorFunctors.erase(it);
		return true;
	}
	return false;
}

