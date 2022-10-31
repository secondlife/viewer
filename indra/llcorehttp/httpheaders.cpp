/**
 * @file httpheaders.cpp
 * @brief Implementation of the HTTPHeaders class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2013, Linden Research, Inc.
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

#include "httpheaders.h"

#include "llstring.h"


namespace LLCore
{


HttpHeaders::HttpHeaders()
{}


HttpHeaders::~HttpHeaders()
{}


void
HttpHeaders::clear()
{
    mHeaders.clear();
}


void HttpHeaders::append(const std::string & name, const std::string & value)
{
    mHeaders.push_back(value_type(name, value));
}


void HttpHeaders::append(const char * name, const char * value)
{
    mHeaders.push_back(value_type(name, value));
}


void HttpHeaders::appendNormal(const char * header, size_t size)
{
    std::string name;
    std::string value;

    int col_pos(0);
    for (; col_pos < size; ++col_pos)
    {
        if (':' == header[col_pos])
            break;
    }
    
    if (col_pos < size)
    {
        // Looks like a header, split it and normalize.
        // Name is everything before the colon, may be zero-length.
        name.assign(header, col_pos);

        // Value is everything after the colon, may also be zero-length.
        const size_t val_len(size - col_pos - 1);
        if (val_len)
        {
            value.assign(header + col_pos + 1, val_len);
        }

        // Clean the strings
        LLStringUtil::toLower(name);
        LLStringUtil::trim(name);
        LLStringUtil::trimHead(value);
    }
    else
    {
        // Uncertain what this is, we'll pack it as
        // a name without a value.  Won't clean as we don't
        // know what it is...
        name.assign(header, size);
    }

    mHeaders.push_back(value_type(name, value));
}


// Find from end to simulate a tradition of using single-valued
// std::map for this in the past.
const std::string * HttpHeaders::find(const std::string &name) const
{
    const_reverse_iterator iend(rend());
    for (const_reverse_iterator iter(rbegin()); iend != iter; ++iter)
    {
        if ((*iter).first == name)
        {
            return &(*iter).second;
        }
    }
    return NULL;
}

void HttpHeaders::remove(const char *name)
{
    remove(std::string(name));
}

void HttpHeaders::remove(const std::string &name)
{
    iterator iend(end());
    for (iterator iter(begin()); iend != iter; ++iter)
    {
        if ((*iter).first == name)
        {
            mHeaders.erase(iter);
            return;
        }
    }
}


// Standard Iterators
HttpHeaders::iterator HttpHeaders::begin()
{
    return mHeaders.begin();
}


HttpHeaders::const_iterator HttpHeaders::begin() const
{
    return mHeaders.begin();
}


HttpHeaders::iterator HttpHeaders::end()
{
    return mHeaders.end();
}


HttpHeaders::const_iterator HttpHeaders::end() const
{
    return mHeaders.end();
}


// Standard Reverse Iterators
HttpHeaders::reverse_iterator HttpHeaders::rbegin()
{
    return mHeaders.rbegin();
}


HttpHeaders::const_reverse_iterator HttpHeaders::rbegin() const
{
    return mHeaders.rbegin();
}


HttpHeaders::reverse_iterator HttpHeaders::rend()
{
    return mHeaders.rend();
}


HttpHeaders::const_reverse_iterator HttpHeaders::rend() const
{
    return mHeaders.rend();
}


// Return the raw container to the caller.
//
// To be used FOR UNIT TESTS ONLY.
//
HttpHeaders::container_t & HttpHeaders::getContainerTESTONLY()
{
    return mHeaders;
}


}   // end namespace LLCore

