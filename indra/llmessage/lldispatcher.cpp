/**
 * @file lldispatcher.cpp
 * @brief Implementation of the dispatcher object.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "lldispatcher.h"

#include <algorithm>
#include <iterator>
#include "llstl.h"
#include "message.h"

///----------------------------------------------------------------------------
/// Class lldispatcher
///----------------------------------------------------------------------------


LLDispatcher::LLDispatcher()
{
}

LLDispatcher::~LLDispatcher()
{
}

bool LLDispatcher::isHandlerPresent(const key_t& name) const
{
    if(mHandlers.find(name) != mHandlers.end())
    {
        return true;
    }
    return false;
}

void LLDispatcher::copyAllHandlerNames(keys_t& names) const
{
    // copy the names onto the vector we are given
    std::transform(
        mHandlers.begin(),
        mHandlers.end(),
        std::back_insert_iterator<keys_t>(names),
        llselect1st<dispatch_map_t::value_type>());
}

bool LLDispatcher::dispatch(
    const key_t& name,
    const LLUUID& invoice,
    const sparam_t& strings) const
{
    dispatch_map_t::const_iterator it = mHandlers.find(name);
    if(it != mHandlers.end())
    {
        LLDispatchHandler* func = (*it).second;
        return (*func)(this, name, invoice, strings);
    }
    LL_WARNS() << "Unable to find handler for Generic message: " << name << LL_ENDL;
    return false;
}

LLDispatchHandler* LLDispatcher::addHandler(
    const key_t& name, LLDispatchHandler* func)
{
    dispatch_map_t::iterator it = mHandlers.find(name);
    LLDispatchHandler* old_handler = NULL;
    if(it != mHandlers.end())
    {
        old_handler = (*it).second;
        mHandlers.erase(it);
    }
    if(func)
    {
        // only non-null handlers so that we don't have to worry about
        // it later.
        mHandlers.insert(dispatch_map_t::value_type(name, func));
    }
    return old_handler;
}

// static
bool LLDispatcher::unpackMessage(
    LLMessageSystem* msg,
    LLDispatcher::key_t& method,
    LLUUID& invoice,
    LLDispatcher::sparam_t& parameters)
{
    char buf[MAX_STRING];   /*Flawfinder: ignore*/
    msg->getStringFast(_PREHASH_MethodData, _PREHASH_Method, method);
    msg->getUUIDFast(_PREHASH_MethodData, _PREHASH_Invoice, invoice);
    S32 size;
    S32 count = msg->getNumberOfBlocksFast(_PREHASH_ParamList);
    for (S32 i = 0; i < count; ++i)
    {
        // we treat the SParam as binary data (since it might be an
        // LLUUID in compressed form which may have embedded \0's,)
        size = msg->getSizeFast(_PREHASH_ParamList, i, _PREHASH_Parameter);
        if (size >= 0)
        {
            msg->getBinaryDataFast(
                _PREHASH_ParamList, _PREHASH_Parameter,
                buf, size, i, MAX_STRING - 1);

            // If the last byte of the data is 0x0, this is either a normally
            // packed string, or a binary packed UUID (which for these messages
            // are packed with a 17th byte 0x0).  Unpack into a std::string
            // without the trailing \0, so "abc\0" becomes std::string("abc", 3)
            // which matches const char* "abc".
            if (size > 0
                && buf[size - 1] == 0x0)
            {
                // special char*/size constructor because UUIDs may have embedded
                // 0x0 bytes.
                std::string binary_data(buf, size - 1);
                parameters.push_back(binary_data);
            }
            else
            {
                // This is either a NULL string, or a string that was packed
                // incorrectly as binary data, without the usual trailing '\0'.
                std::string string_data(buf, size);
                parameters.push_back(string_data);
            }
        }
    }
    return true;
}

// static
bool LLDispatcher::unpackLargeMessage(
    LLMessageSystem* msg,
    LLDispatcher::key_t& method,
    LLUUID& invoice,
    LLDispatcher::sparam_t& parameters)
{
    msg->getStringFast(_PREHASH_MethodData, _PREHASH_Method, method);
    msg->getUUIDFast(_PREHASH_MethodData, _PREHASH_Invoice, invoice);
    S32 count = msg->getNumberOfBlocksFast(_PREHASH_ParamList);
    for (S32 i = 0; i < count; ++i)
    {
        // This method treats all Parameter List params as strings and unpacks
        // them regardless of length. If there is binary data it is the callers
        // responsibility to decode it.
        std::string param;
        msg->getStringFast(_PREHASH_ParamList, _PREHASH_Parameter, param, i);
        parameters.push_back(param);
    }
    return true;
}
