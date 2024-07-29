/**
 * @file lluriparser.cpp
 * @author Protey
 * @date 2014-10-07
 * @brief Implementation of the LLUriParser class.
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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
#include "lluriparser.h"

LLUriParser::LLUriParser(const std::string& u) : mTmpScheme(false), mNormalizedTmp(false), mRes(false)
{
    if (u.find("://") == std::string::npos)
    {
        mNormalizedUri = "http://";
        mTmpScheme = true;
    }

    mNormalizedUri.append(u);

    mRes = parse();
}

LLUriParser::~LLUriParser()
{
}

bool LLUriParser::parse()
{
    try
    {
        auto res = boost::urls::parse_uri(mNormalizedUri);
        if (res)
        {
            mUri = *res;
            mRes = true;
        }
        else
        {
            mRes = false;
        }
    }
    catch (const std::length_error&)
    {
        LL_WARNS() << "Failed to parse uri due to exceeding uri_view max_size" << LL_ENDL;
        mRes = false;
    }
    return mRes;
}

const std::string& LLUriParser::scheme() const
{
    return mScheme;
}

void LLUriParser::scheme(const std::string& s)
{
    mTmpScheme = !s.size();
    mScheme = s;
}

const std::string& LLUriParser::port() const
{
    return mPort;
}

void LLUriParser::port(const std::string& s)
{
    mPort = s;
}

const std::string& LLUriParser::host() const
{
    return mHost;
}

void LLUriParser::host(const std::string& s)
{
    mHost = s;
}

const std::string& LLUriParser::path() const
{
    return mPath;
}

void LLUriParser::path(const std::string& s)
{
    mPath = s;
}

const std::string& LLUriParser::query() const
{
    return mQuery;
}

void LLUriParser::query(const std::string& s)
{
    mQuery = s;
}

const std::string& LLUriParser::fragment() const
{
    return mFragment;
}

void LLUriParser::fragment(const std::string& s)
{
    mFragment = s;
}

void LLUriParser::extractParts()
{
    if (mTmpScheme || mNormalizedTmp)
    {
        mScheme.clear();
    }
    else
    {
        mScheme = mUri.scheme();
    }

    mHost = mUri.host();
    mPort = mUri.port();
    mQuery = mUri.query();
    mFragment = mUri.fragment();
    mPath = mUri.path();
}

bool LLUriParser::normalize()
{
    mNormalizedTmp = mTmpScheme;
    if (mRes)
    {
        mUri.normalize_scheme().normalize_authority();
        mNormalizedUri = mUri.buffer().substr(mTmpScheme ? 7 : 0);
        mTmpScheme = false;
    }

    if(mTmpScheme && mNormalizedUri.size() > 7)
    {
        mNormalizedUri = mNormalizedUri.substr(7);
        mTmpScheme = false;
    }

    return mRes;
}

void LLUriParser::glue(std::string& uri) const
{
    std::string first_part;
    glueFirst(first_part);

    std::string second_part;
    glueSecond(second_part);

    uri = first_part + second_part;
}

void LLUriParser::glueFirst(std::string& uri, bool use_scheme) const
{
    if (use_scheme && mScheme.size())
    {
        uri = mScheme;
        uri += "://";
    }
    else
    {
        uri.clear();
    }

    uri += mHost;
}

void LLUriParser::glueSecond(std::string& uri) const
{
    if (mPort.size())
    {
        uri = ':';
        uri += mPort;
    }
    else
    {
        uri.clear();
    }

    uri += mPath;

    if (mQuery.size())
    {
        uri += '?';
        uri += mQuery;
    }

    if (mFragment.size())
    {
        uri += '#';
        uri += mFragment;
    }
}

bool LLUriParser::test() const
{
    std::string uri;
    glue(uri);

    return uri == mNormalizedUri;
}

const std::string& LLUriParser::normalizedUri() const
{
    return mNormalizedUri;
}
