/** 
 * @file llchainio.cpp
 * @author Phoenix
 * @date 2005-08-04
 * @brief Implementaiton of the chain factory.
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llchainio.h"

#include "lliopipe.h"
#include "llioutil.h"


/**
 * LLDeferredChain
 */
// static
bool LLDeferredChain::addToPump(
	LLPumpIO* pump,
	F32 in_seconds,
	const LLPumpIO::chain_t& deferred_chain,
	F32 chain_timeout)
{
	if(!pump) return false;
	LLPumpIO::chain_t sleep_chain;
	sleep_chain.push_back(LLIOPipe::ptr_t(new LLIOSleep(in_seconds)));
	sleep_chain.push_back(
		LLIOPipe::ptr_t(new LLIOAddChain(deferred_chain, chain_timeout)));

	// give it a litle bit of padding.
	pump->addChain(sleep_chain, in_seconds + 10.0f);
	return true;
}

/**
 * LLChainIOFactory
 */
LLChainIOFactory::LLChainIOFactory()
{
}

// virtual
LLChainIOFactory::~LLChainIOFactory()
{
}

#if 0
bool LLChainIOFactory::build(LLIOPipe* in, LLIOPipe* out) const
{
	if(!in || !out)
	{
		return false;
	}
	LLIOPipe* first = NULL;
	LLIOPipe* last = NULL;
	if(build_impl(first, last) && first && last)
	{
		in->connect(first);
		last->connect(out);
		return true;
	}
	LLIOPipe::ptr_t foo(first);
	LLIOPipe::ptr_t bar(last);
	return false;
}
#endif
