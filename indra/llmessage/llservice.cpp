/** 
 * @file llservice.cpp
 * @author Phoenix
 * @date 2005-04-20
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
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

