/** 
 * @file llchainio.cpp
 * @author Phoenix
 * @date 2005-08-04
 * @brief Implementaiton of the chain factory.
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
