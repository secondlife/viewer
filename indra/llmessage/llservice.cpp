/** 
 * @file llservice.cpp
 * @author Phoenix
 * @date 2005-04-20
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
    LL_INFOS() << "LLService::registerCreator(" << name << ")" << LL_ENDL;
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
        LL_INFOS() << "LLService::activate - no service specified." << LL_ENDL;
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
            LL_WARNS() << "LLService::activate - unable to build chain: " << name
                    << LL_ENDL;
            chain.clear();
        }
    }
    else
    {
        LL_WARNS() << "LLService::activate - unable find factory: " << name
                << LL_ENDL;
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

