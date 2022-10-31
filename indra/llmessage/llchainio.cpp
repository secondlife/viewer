/** 
 * @file llchainio.cpp
 * @author Phoenix
 * @date 2005-08-04
 * @brief Implementaiton of the chain factory.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
