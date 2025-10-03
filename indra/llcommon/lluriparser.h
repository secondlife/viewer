/**
 * @file lluriparser.h
 * @author Protey
 * @date 20146-10-07
 * @brief Declaration of the UriParser class.
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

#ifndef LL_LLURIPARSER_H
#define LL_LLURIPARSER_H

#include <string>
#include "boost/url.hpp"

class LL_COMMON_API LLUriParser
{
public:
    LLUriParser(const std::string& u);
    ~LLUriParser();

    const std::string& scheme() const;
    void scheme (const std::string& s);

    const std::string& port() const;
    void port (const std::string& s);

    const std::string& host() const;
    void host (const std::string& s);

    const std::string& path() const;
    void path (const std::string& s);

    const std::string& query() const;
    void query (const std::string& s);

    const std::string& fragment() const;
    void fragment (const std::string& s);

    const std::string& normalizedUri() const;

    void extractParts();
    void glue(std::string& uri) const;
    void glueFirst(std::string& uri, bool use_scheme = true) const;
    void glueSecond(std::string& uri) const;
    bool test() const;
    bool normalize();

private:
    bool parse();
    std::string mScheme;
    std::string mHost;
    std::string mPort;
    std::string mPath;
    std::string mQuery;
    std::string mFragment;
    std::string mNormalizedUri;

    boost::url mUri;

    bool mRes;
    bool mTmpScheme;
    bool mNormalizedTmp;
};

#endif // LL_LLURIPARSER_H
