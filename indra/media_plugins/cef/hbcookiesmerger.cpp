/**
 * @file hbcookiesmerger.cpp
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

#include "linden_common.h"

#include "sqlite3.h"

#include "hbcookiesmerger.h"

// These defines might need to be changed with future CEF versions, should
// their "Cookies" database scheme change; in the latter case, make sure to
// check for the fields type (currently, HOST_FIELD and COOKIE_FIELD are UTF-8
// strings while DATE_FIELD is a 64 bits integer).
#define COOKIES_TABLE "cookies"
#define HOST_FIELD "host_key"
#define COOKIE_FIELD "name"
#define DATE_FIELD "last_update_utc"

HBCookiesMerger::HBCookiesMerger(const std::string& source_db,
                                 const std::string& dest_db,
                                 const std::string& debug_log)
:   mSrcFileName(source_db),
    mDstFileName(dest_db),
    mLogFileName(debug_log),
    mSrcDb(nullptr),
    mDstDb(nullptr),
    mLogStream(nullptr)
{
}

HBCookiesMerger::~HBCookiesMerger()
{
    // Do not log on destruction: the latter could happen after the consumer
    // used that same log file, and file pointers would disagree then...
    if (mLogStream)
    {
        delete mLogStream;
    }
    close();
}

void HBCookiesMerger::close()
{
    if (mDstDb)
    {
        sqlite3_close(mDstDb);
        if (mLogStream)
        {
            *mLogStream << "Closing destination database." << std::endl;
        }
        mDstDb = nullptr;
    }
    if (mSrcDb)
    {
        if (mLogStream)
        {
            *mLogStream << "Closing source database." << std::endl;
        }
        sqlite3_close(mSrcDb);
        mSrcDb = nullptr;
    }
}

bool HBCookiesMerger::hasError(sqlite3* db, int result)
{
    if (result == SQLITE_OK || result == SQLITE_DONE)
    {
        mErrMsg.clear();
        return false;
    }
    mErrMsg.assign(sqlite3_errmsg(db));
    if (mLogStream)
    {
        *mLogStream << std::endl << "SQLite error: " << mErrMsg << std::endl;
    }
    return true;
}

std::set<std::string> HBCookiesMerger::getTables()
{
    // Get all tables excepted internal SQLite ones
    static const char* sql =
        "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';";

    std::set<std::string> tables;

    sqlite3_stmt* stmt = nullptr;
    if (!hasError(mSrcDb, sqlite3_prepare_v2(mSrcDb, sql, -1, &stmt, 0)))
    {
        int result;
        while ((result = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            tables.emplace((const char*)sqlite3_column_text(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }

    return tables;
}

static void bind_row_values(sqlite3_stmt* src, sqlite3_stmt* dst, int columns)
{
    for (int i = 0; i < columns; ++i)
    {
        switch (sqlite3_column_type(src, i))
        {
            case SQLITE_INTEGER:
                sqlite3_bind_int64(dst, i + 1, sqlite3_column_int64(src, i));
                break;

            case SQLITE_FLOAT:
                sqlite3_bind_double(dst, i + 1, sqlite3_column_double(src, i));
                break;

            case SQLITE_TEXT:
                sqlite3_bind_text(dst, i + 1,
                                  (const char*)sqlite3_column_text(src, i),
                                  -1, SQLITE_STATIC);
                break;

            case SQLITE_BLOB:
                sqlite3_bind_blob(dst, i + 1, sqlite3_column_blob(src, i),
                                  sqlite3_column_bytes(src, i), SQLITE_STATIC);
                break;

            case SQLITE_NULL:
                sqlite3_bind_null(dst, i + 1);

            default:        // Should never happen. Just do not care.
                break;
        }
    }
}

bool HBCookiesMerger::mergeTable(const std::string& table)
{
    if (hasError(mDstDb,
                 sqlite3_exec(mDstDb, "BEGIN TRANSACTION;", nullptr, nullptr,
                              nullptr)))
    {
        return false;
    }

    std::string sql = "SELECT * FROM " + table;
    sqlite3_stmt* read_stmt = nullptr;
    if (hasError(mSrcDb,
                 sqlite3_prepare_v2(mSrcDb, sql.c_str(), -1, &read_stmt, 0)))
    {
        return false;
    }
    int columns = sqlite3_column_count(read_stmt);

    int host_field_index = -1;
    int cookie_field_index = -1;
    int date_field_index = -1;
    std::string replace_sql = "INSERT OR REPLACE INTO " + table + " VALUES (";
    for (int i = 0; i < columns; ++i)
    {
        replace_sql += "?";
        if (i < columns - 1)
        {
            replace_sql += ",";
        }
        // Find the columns numbers for the cookies site (host_key), name and
        // last update time stamp.
        const char* cname = (const char*)sqlite3_column_name(read_stmt, i);
        if (strcmp(cname, HOST_FIELD) == 0)
        {
            host_field_index = i;
        }
        else if (strcmp(cname, COOKIE_FIELD) == 0)
        {
            cookie_field_index = i;
        }
        else if (strcmp(cname, DATE_FIELD) == 0)
        {
            date_field_index = i;
        }
    }
    replace_sql += ")";

    if (host_field_index == -1 || cookie_field_index == -1 ||
        date_field_index == -1)
    {
        mErrMsg = "Missing column in the cookies table.";
        sqlite3_finalize(read_stmt);
        return false;
    }

    std::string check_sql = "SELECT " DATE_FIELD " FROM " + table +
                            " WHERE " HOST_FIELD " = ? AND " COOKIE_FIELD
                            " = ?";
    sqlite3_stmt* check_stmt = nullptr;
    int result;
    while ((result = sqlite3_step(read_stmt)) == SQLITE_ROW)
    {
        // For each row in the source table, grab the 'HOST_FIELD and cookie
        // COOKIE_FIELD strings (which should identify each unique cookie), as
        // well as the DATE_FIELD timestamp.
        const char* host_key =
            (const char*)sqlite3_column_text(read_stmt, host_field_index);
        const char* name =
            (const char*)sqlite3_column_text(read_stmt, cookie_field_index);
        auto last_update_utc = sqlite3_column_int64(read_stmt,
                                                    date_field_index);
        if (mLogStream)
        {
            *mLogStream << "Cookie: " << host_key << " / " << name
                        << " - Last updated: " << last_update_utc;
        }

        // Check if the cookie already exists in the destination database
        if (check_stmt)
        {
            sqlite3_reset(check_stmt);    // Reset for next check
        }
        if (hasError(mDstDb,
                     sqlite3_prepare_v2(mDstDb, check_sql.c_str(), -1,
                                        &check_stmt, 0)))
        {
            sqlite3_finalize(read_stmt);
            return false;
        }

        sqlite3_bind_text(check_stmt, 1, host_key, -1, SQLITE_STATIC);
        sqlite3_bind_text(check_stmt, 2, name, -1, SQLITE_STATIC);
        result = sqlite3_step(check_stmt);
        if (result == SQLITE_DONE)
        {
            // The cookie does not exists in destination table: insert it.
            if (mLogStream)
            {
                *mLogStream << " - Cookie does not exist: inserting it.";
            }
        }
        else if (result == SQLITE_ROW)
        {
            // The cookie is already in destination table: see if it needs to
            // be updated, based on last update timestamp.
            if (last_update_utc <= sqlite3_column_int64(check_stmt, 0))
            {
                if (mLogStream)
                {
                    *mLogStream << " - Cookie is up to date." << std::endl;
                }
                continue;
            }
            if (mLogStream)
            {
                *mLogStream << " - Cookie needs updating.";
            }
        }
        else
        {
            if (mLogStream)
            {
                *mLogStream << std::endl;
            }
            // Nothing to do.
            continue;
        }

        // Insert or replace the cookie
        sqlite3_stmt* replace_stmt = nullptr;
        if (hasError(mDstDb,
                     sqlite3_prepare_v2(mDstDb, replace_sql.c_str(), -1,
                                        &replace_stmt, 0)))
        {
            sqlite3_finalize(check_stmt);
            sqlite3_finalize(read_stmt);
            return false;
        }
        bind_row_values(read_stmt, replace_stmt, columns);
        if (hasError(mDstDb, sqlite3_step(replace_stmt)))
        {
            sqlite3_finalize(replace_stmt);
            sqlite3_finalize(check_stmt);
            sqlite3_finalize(read_stmt);
            return false;
        }
        sqlite3_finalize(replace_stmt);
        if (mLogStream)
        {
            *mLogStream << " - Cookie updated." << std::endl;
        }
    }

    if (check_stmt)
    {
        sqlite3_finalize(check_stmt);
    }
    sqlite3_finalize(read_stmt);

    if (mLogStream)
    {
        *mLogStream << "Cookies merged." << std::endl;
    }

    if (hasError(mDstDb,
                 sqlite3_exec(mDstDb, "COMMIT;", nullptr, nullptr, nullptr)))
    {
        return false;
    }
    return true;
}

bool HBCookiesMerger::merge()
{
    mErrMsg.clear();

    if (!mLogFileName.empty())
    {
        mLogStream = new llofstream(mLogFileName,
                                    std::ios::out | std::ios::app);
    }
    if (mLogStream)
    {
        *mLogStream << "Merging cookies database '" << mSrcFileName
                    << "' into database '" << mDstFileName << '"' << std::endl;
    }
    int result = sqlite3_open(mSrcFileName.c_str(), &mSrcDb);
    if (result != SQLITE_OK)
    {
        mErrMsg = "Failed to open source database '" + mSrcFileName +
                  "' with error: " + std::string(sqlite3_errstr(result));
        if (mLogStream)
        {
            *mLogStream << mErrMsg << std::endl;
            delete mLogStream;
            mLogStream = nullptr;
        }
        return false;
    }
    result = sqlite3_open(mDstFileName.c_str(), &mDstDb);
    if (result != SQLITE_OK)
    {
        mErrMsg = "Failed to open source database '" + mDstFileName +
                  "' with error: " + std::string(sqlite3_errstr(result));
        if (mLogStream)
        {
            *mLogStream << mErrMsg << std::endl;
            delete mLogStream;
            mLogStream = nullptr;
        }
        close();
        return false;
    }

    std::set<std::string> tables = getTables();
    if (!tables.count(COOKIES_TABLE))
    {
        mErrMsg = "No '" COOKIES_TABLE "' table in database: " + mSrcFileName;
        if (mLogStream)
        {
            *mLogStream << mErrMsg << std::endl;
            delete mLogStream;
            mLogStream = nullptr;
        }
        close();
        return false;
    }

    bool success = mergeTable(COOKIES_TABLE);
    close();
    if (mLogStream)
    {
        delete mLogStream;
        mLogStream = nullptr;
    }
    return success;
}
