/**
 * @file hbcookiesmerger.h
 * @brief A CEF cookies database merger.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Henri Beauchamp.
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

#ifndef HB_HBCOOKIESMERGER_H
#define HB_HBCOOKIESMERGER_H

#include <set>

#include "llfile.h"

struct sqlite3;

class HBCookiesMerger
{
public:
    HBCookiesMerger(const std::string& source_db, const std::string& dest_db,
                    const std::string& debug_log = LLStringUtil::null);
    ~HBCookiesMerger();

    bool merge();

    // Allows to retreive the last error message (e.g for when debug_log was
    // not used).
    inline const std::string& getErrorMessage() { return mErrMsg; }

private:
    void close();
    bool hasError(sqlite3* db, int result);
    std::set<std::string> getTables();
    bool mergeTable(const std::string& table_name);

private:
    sqlite3*    mSrcDb;
    sqlite3*    mDstDb;
    std::string mErrMsg;
    std::string mSrcFileName;
    std::string mDstFileName;
    std::string mLogFileName;
    llofstream* mLogStream;
};

#endif    // HB_HBCOOKIESMERGER_H
