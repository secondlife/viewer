/**
 * @file lldiskcache.cpp
 * @brief The SQLite based disk cache implementation.
 * @Note  Eventually, this component might split into an interface
 *        file and multiple implemtations but for now, this is the
 *        only one so I think it's okay to combine everything. 
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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

#if (defined(LL_WINDOWS) || defined(LL_LINUX)  || defined(LL_DARWIN))
#include "linden_common.h"
#endif

#include "lldiskcache.h"

#include <string>
#include <sstream>
#include <random>
#include <algorithm>
#include <fstream>

///////////////////////////////////////////////////////////////////////////////
//
llDiskCache::llDiskCache() :
    mDataStorePath("")
{
}

llDiskCache::~llDiskCache()
{
}

///////////////////////////////////////////////////////////////////////////////
// Opens the database - typically done when the application starts
// and is complementary to close() which is called when the application
// is finisahed and exits.
// Pass in the folder and filename of the SQLite database you want to
// use or create (file doesn't have to exist but the folder must)
// Returns true or false and writes a message to console on error
bool llDiskCache::open(const std::string db_folder, const std::string db_filename)
{
    mDataStorePath = db_folder;
    std::string db_pathname = makeFullPath(db_filename);

    // simple flags for the moment - these will likely be extended
    // later on to support the SQLite mutex model for reading/writing
    // simultaneously - perhaps when we look at supporting textures too
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

    if (sqlite3_open_v2(
                db_pathname.c_str(),
                &mDb,
                flags,
                nullptr // Name of VFS module to use
            ) != SQLITE_OK)
    {
        printError(__FUNCDNAME__ , "open_v2", true);
        close();
        return false;
    }

    // I elected to store the code that generates the statement
    // in sepsrate functions throughout - this seemed like a cleaner
    // approach than having hundreds of stmt << "REPLACE AS" lines
    // interspersed in the logic code. They are all prefixed with
    // 'sqlCompose' and followed by a short description.
    const std::string stmt = sqlComposeCreateTable();
    if (! sqliteExec(stmt, __FUNCDNAME__ ))
    {
        // Not sure if we need close here - if open fails, then we
        // perhaps don't need to close it - TODO: look in SQLite docs
        close();
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Determines if an entry exists.
// Pass in the id and a variable that will indicate existance
// Returns true/false if function succeeded and the boolean
// you pass in will be set to true/false if entry exists or not
bool llDiskCache::exists(const std::string id, bool& exists)
{
    if (!mDb)
    {
        printError(__FUNCDNAME__ , "mDb is invalid", false);
        return false;
    }

    if (id.empty())
    {
        printError(__FUNCDNAME__ , "id is empty", false);
        return false;
    }

    // As well as the separate function to compose the SQL statement,
    // worth pointing out that the code to 'prepare' and 'step' the
    // SQLite "prepared statement" has been factored out into its own
    // function and used in several other functions.
    const std::string stmt = sqlComposeExists(id);
    sqlite3_stmt* prepared_statement = sqlitePrepareStep(__FUNCDNAME__ , stmt, SQLITE_ROW);
    if (! prepared_statement)
    {
        return false;
    }

    int result_column_index = 0;
    int result_count = sqlite3_column_int(prepared_statement, result_column_index);
    if (sqlite3_finalize(prepared_statement) != SQLITE_OK)
    {
        printError(__FUNCDNAME__ , "sqlite3_finalize()", true);
        return false;
    }

    // given the uniqueness of the ID, this can only ever be
    // either 1 or 0 so this might be a bit confusing
    exists = result_count > 0 ? true : false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Given an id (likely a UUID + decoration but can be any string), a
// pointer to a blob of binary data along with its length, write the
// entry to the cache
// Returns true/false for success/failure
bool llDiskCache::put(const std::string id,
                      char* binary_data,
                      int binary_data_size)
{
    if (!mDb)
    {
        printError(__FUNCDNAME__ , "mDb is invalid", false);
        return false;
    }

    if (id.empty())
    {
        printError(__FUNCDNAME__ , "id is empty", false);
        return false;
    }

    // < 0 is obvious but we assert the data must be at least 1 byte long
    if (binary_data_size <= 0)
    {
        printError(__FUNCDNAME__ , "size of binary file to write is invalid", false);
        return false;
    }

    // we generate a unique filename for the actual data itself
    // which is stored on disk directly and not in the database.
    // TODO: consider making the filename more like the ID passed in
    // although the problem with that is we would have to parse the id
    // to remove invalid filename chars, consider length etc.  As it
    // stands, we can write a simple SQL statement to return the filename
    // given the ID.
    const std::string filename = makeUniqueFilename();
    const std::string filepath = makeFullPath(filename);
    std::ofstream file(filepath, std::ios::out | std::ios::binary);
    if (! file.is_open())
    {
        std::ostringstream error;
        error << "Unable to open " << filepath << " for writing";
        printError(__FUNCDNAME__ , error.str(), false);
        return false;
    }

    file.write((char*)binary_data, binary_data_size);
    file.close();

    // I think is a catchall "wasn't able to write the file to disk"
    // conditional but should revisit when we hook this up to viewer
    // code to make sure - we never want to write bad/no data to the
    // disk and not indicate something has gone wrong
    if (file.bad())
    {
        std::ostringstream error;
        error << "Unable to write " << binary_data_size;
        error << " bytes to " << filepath;
        printError(__FUNCDNAME__ , error.str(), false);

        return false;
    }

    // this is where the filename/size is written to the database along
    // with the current date/time for the created/last access times
    const std::string stmt = sqlComposePut(id, filename, binary_data_size);
    if (! sqlitePrepareStep(__FUNCDNAME__ , stmt, SQLITE_DONE))
    {
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Given an id (likely a UUID + decoration but can be any string), the
// address of a pointer that will be used to allocate memory and a
// varialble that will contain the length of the data, get an item from
// the cache. Note that the memory buffer returned belongs to the calling
// function and it is its responsiblity to clean it up when it's no
// longer needed.
// Returns true/false for success/failure
const bool llDiskCache::get(const std::string id,
                            char** mem_buffer,
                            int& mem_buffer_size)
{
    // Check if the entry exists first to avoid dealing with a bunch
    // of conditions that look like failure but aren't in the main code.
    // Note exists() is a public method and also tests for mDB and id
    // being valid so we can safely put this about the same tests
    // in this function
    bool get_exists = false;
    if (! exists(id, get_exists))
    {
        return false;
    }
    if (! get_exists)
    {
        return false;
    }

    if (!mDb)
    {
        printError(__FUNCDNAME__ , "mDb is invalid", false);
        return false;
    }

    if (id.empty())
    {
        printError(__FUNCDNAME__ , "id is empty", false);
        return false;
    }

    const std::string stmt_select = sqlComposeGetSelect(id);
    sqlite3_stmt* prepared_statement = sqlitePrepareStep(__FUNCDNAME__ , stmt_select, SQLITE_ROW);
    if (! prepared_statement)
    {
        return false;
    }

    int result_column_index = 0;
    const unsigned char* text = sqlite3_column_text(prepared_statement, result_column_index);
    if (text == nullptr)
    {
        printError(__FUNCDNAME__ , "filename is nullptr", true);
        return false;
    }
    const std::string filename = std::string(reinterpret_cast<const char*>(text));
    const std::string filepath = makeFullPath(filename);

    result_column_index = 1;
    int filesize_db = sqlite3_column_int(prepared_statement, result_column_index);
    if (filesize_db <= 0)
    {
        printError(__FUNCDNAME__ , "filesize is invalid", true);
        return false;
    }

    if (sqlite3_finalize(prepared_statement) != SQLITE_OK)
    {
        printError(__FUNCDNAME__ , "sqlite3_finalize()", true);
        return false;
    }

    // Now we have the fiename, we can read the file from disk
    std::ifstream file(filepath, std::ios::in | std::ios::binary | std::ios::ate);
    if (! file.is_open())
    {
        std::ostringstream error;
        error << "Unable to open " << filepath << " for reading";
        printError(__FUNCDNAME__ , error.str(), false);
        return false;
    }

    // we get the expected filesize from the database but we can also
    // get it (easily) when we read the file from the disk. We compare
    // the two and return false if they don't match
    std::streampos filesize_file = file.tellg();
    if (filesize_db != filesize_file)
    {
        std::ostringstream error;
        error << "File size from DB (" << filesize_db << ")";
        error << " and ";
        error << "file size from file (" << filesize_file << ")";
        error << " in file " << filepath << " are different";
        printError(__FUNCDNAME__ , error.str(), false);

        return false;
    }

    // doest matter if we choose DB or file version - they must be
    // identical if we get this far - just used for clarity
    int filesize = filesize_db;

    // allocate a block of memory that we pass back for the calling
    // function to use then delete when it's no longer needed
    *mem_buffer = new char[filesize];
    mem_buffer_size = filesize;

    file.seekg(0, std::ios::beg);
    file.read(*mem_buffer, filesize);
    file.close();

    if (file.bad())
    {
        std::ostringstream error;
        error << "Unable to read " << filesize;
        error << " bytes from " << filepath;
        printError(__FUNCDNAME__ , error.str(), false);

        return false;
    }

    // here we update the count of times the file is accessed so
    // we can keep track of how many times it's been requested.
    // This will be useful for metrics and perhaps determining
    // if a file should not be purged even though its age
    // might suggest that it should.
    // In addition, this is where the time of last access is updated
    // in the database and that us used to determine what is purged
    // in an LRU fashion when the purge function is called.
    const std::string stmt_update = sqlComposeGetUpdate(id);
    if (! sqliteExec(stmt_update, __FUNCDNAME__ ))
    {
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Purges the database of older entries using an LRU approach.
// Pass in the number of entries to retain.
// This is called now after open to "clean up" the cache when the
// application starts.
// TODO: IMPORTANT: compose a list of files that will be deleted
// and delete them from disk too - not just from the DB
bool llDiskCache::purge(int num_entries)
{
    if (num_entries < 0)
    {
        printError(__FUNCDNAME__ , "number of entries to purge is invalid", false);
        return false;
    }

    // find the rows affected and get the filenames for them
//swww

    // delete oldest entries leaving the correct number in place
    const std::string stmt = sqlComposePurge(num_entries);
    if (! sqliteExec(stmt, __FUNCDNAME__ ))
    {
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Call at application shutdown
void llDiskCache::close()
{
    sqlite3_close(mDb);
}

///////////////////////////////////////////////////////////////////////////////
// Determine the version of SQLite in use
// TODO: make this a static so we can get to it from the Viewer About
// box without instantiating the whole thing.
const std::string llDiskCache::dbVersion()
{
    std::ostringstream version;

    version << sqlite3_libversion();

    return version.str();
}

///////////////////////////////////////////////////////////////////////////////
// Given an id, return the matching filename
const std::string llDiskCache::getFilenameById(const std::string id)
{
    // TODO:
    return std::string();
}

///////////////////////////////////////////////////////////////////////////////
// Given an id, return the number of times that entry has been
// accessed from the cache
const int llDiskCache::getAccessCountById(const std::string id)
{
    // TODO:
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
// Return the number of entries currently in the cache as well as
// the maximum possible entries.
void llDiskCache::getNumEntries(int& num_entries, int& max_entries)
{
    num_entries = -1;
    max_entries = -1;
}

///////////////////////////////////////////////////////////////////////////////
// Wraps the sqlite3_exec(..) used in many places
bool llDiskCache::sqliteExec(const std::string stmt,
                             const std::string funcname)
{
    if (sqlite3_exec(
                mDb,
                stmt.c_str(),
                nullptr,    // Callback function (unused)
                nullptr,    // 1st argument to callback (unused)
                nullptr     // Error msg written here (unused)
            ) != SQLITE_OK)
    {
        printError(funcname, "sqlite3_exec", true);
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Wraps the sqlite3_prepare_v2 and sqlite3_step calls used in many places
sqlite3_stmt* llDiskCache::sqlitePrepareStep(const std::string funcname,
        const std::string stmt,
        int sqlite_success_condition)
{
    sqlite3_stmt* prepared_statement;

    if (sqlite3_prepare_v2(
                mDb,
                stmt.c_str(),
                -1,                     // Maximum length of zSql in bytes.
                &prepared_statement,
                0                       // OUT: Pointer to unused portion of zSql
            ) != SQLITE_OK)
    {
        printError(funcname, "sqlite3_prepare_v2", true);
        return nullptr;
    }

    if (sqlite3_step(prepared_statement) != sqlite_success_condition)
    {
        printError(funcname, "sqlite3_step", true);
        sqlite3_finalize(prepared_statement);
        return nullptr;
    }
    return prepared_statement;
}

///////////////////////////////////////////////////////////////////////////////
// When an "error" occurss - e.g. database cannot be found, file cannot be
// written, invalid argument passed into a function etc. a message is
// written to stderr that should end up in the viewer log
// TODO: Set the verbosity using the usual Viewer mechanism
void llDiskCache::printError(const std::string funcname,
                             const std::string desc,
                             bool is_sqlite_err)
{
    std::ostringstream err_msg;

    err_msg << "llDiskCache error in ";
    err_msg << __FUNCDNAME__  << "(...) ";
    err_msg << desc;

    if (is_sqlite_err)
    {
        err_msg << " - ";
        err_msg << std::string(sqlite3_errmsg(mDb));
    }

    // TODO: set via viewer verbosity level
    const int verbosity = 1;
    if (verbosity > 0)
    {
        std::cerr << err_msg.str() << std::endl;
    }
}

///////////////////////////////////////////////////////////////////////////////
// wrapper for SQLite code to begin an SQL transaction - not used yet but
// it will be eventually
bool llDiskCache::beginTransaction()
{
    const std::string stmt("BEGIN TRANSACTION");
    if (! sqliteExec(stmt, __FUNCDNAME__ ))
    {
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// wrapper for SQLite code to end an SQL transaction - not used yet but
// it will be eventually
bool llDiskCache::endTransaction()
{
    const std::string stmt("COMMIT");
    if (! sqliteExec(stmt, __FUNCDNAME__ ))
    {
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Build a unique filename that will be used to store the actual file
// on disk (as opposed to the meta data in the database)
// TODO: I think this needs more work once we move it to the viewer
// and espcially to make it cross platform - see 'std::tmpnam'
// depreciation comments in compiler output for example
std::string llDiskCache::makeUniqueFilename()
{
    // TODO: replace with boost - this is marked as deprecated?
    std::string base = std::tmpnam(nullptr);

    // C++11 random number generation!!!
    static std::random_device dev;
    static std::mt19937 rng(dev());
    std::uniform_int_distribution<int> dist(100000, 999999);

    // currently the tmp filename from std::tmpnam() on macOS
    // is of the form `/tmp/foo/bar.12345 and the following code
    // strips all the preceding dirs - we likely want a more
    // robust (and cross platform solution) when we move to the
    // viewer code
    std::size_t found = base.rfind(systemSeparator());
    if (found != std::string::npos && found < base.size() - 2)
    {
        base = base.substr(found + 1);
    }
    else
    {
        base = "";
    }

    // we mix in a random number for some more entropy..
    // (i know, i know...)
    std::ostringstream unique_filename;
    unique_filename << base;
    unique_filename << ".";
    unique_filename <<      dist(rng);

    return unique_filename.str();
}

///////////////////////////////////////////////////////////////////////////////
// Return system file/path separator - likely replaced by the version
// in the viewer
const std::string llDiskCache::systemSeparator()
{
// TODO: replace in viewer with relevant call
#ifdef _WIN32
    return "\\";
#else
    return "/";
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Given a filename, compose a full path based on the path name passed
// in when the database was opened and the separator in play.
const std::string llDiskCache::makeFullPath(const std::string filename)
{
    std::string pathname = mDataStorePath + systemSeparator() + filename;

    return pathname;
}

///////////////////////////////////////////////////////////////////////////////
//
const std::string llDiskCache::sqlComposeCreateTable()
{
    std::ostringstream stmt;
    stmt << "CREATE TABLE IF NOT EXISTS ";
    stmt << mTableName;
    stmt << "(";
    stmt << mIdFieldName;
    stmt << " TEXT PRIMARY KEY, ";
    stmt << mFilenameFieldName;
    stmt << " TEXT, ";
    stmt << mFilesizeFieldName;
    stmt << " INTEGER DEFAULT 0, ";
    stmt << mInsertionDateTimeFieldName;
    stmt << " TEXT, ";
    stmt << mLastAccessDateTimeFieldName;
    stmt << " TEXT, ";
    stmt << mAccessCountFieldName;
    stmt << " INTEGER DEFAULT 0";
    stmt << ")";

#ifdef SHOW_STATEMENTS
    std::cout << stmt.str() << std::endl;
#endif

    return stmt.str();
}

///////////////////////////////////////////////////////////////////////////////
//
const std::string llDiskCache::sqlComposeExists(const std::string id)
{
    std::ostringstream stmt;
    stmt << "SELECT COUNT(*) FROM ";
    stmt << mTableName;
    stmt << " WHERE ";
    stmt << mIdFieldName;
    stmt << "='";
    stmt << id;
    stmt << "'";

#ifdef SHOW_STATEMENTS
    std::cout << stmt.str() << std::endl;
#endif

    return stmt.str();
}

///////////////////////////////////////////////////////////////////////////////
// SQL statement to write an entry to the DB
// Saves id, filename (generated), file size and create/last access date
const std::string llDiskCache::sqlComposePut(const std::string id,
        const std::string filename,
        int binary_data_size)
{
    std::ostringstream stmt;
    stmt << "REPLACE INTO ";
    stmt << mTableName;
    stmt << "(";
    stmt << mIdFieldName;
    stmt << ",";
    stmt << mFilenameFieldName;
    stmt << ",";
    stmt << mFilesizeFieldName;
    stmt << ",";
    stmt << mInsertionDateTimeFieldName;
    stmt << ",";
    stmt << mLastAccessDateTimeFieldName;
    stmt << ") ";
    stmt << "VALUES(";
    stmt << "'";
    stmt << id;
    stmt << "', ";
    stmt << "'";
    stmt << filename;
    stmt << "', ";
    stmt << binary_data_size;
    stmt << ", ";
    stmt << "strftime('%Y-%m-%d %H:%M:%f', 'now')";
    stmt << ", ";
    stmt << "strftime('%Y-%m-%d %H:%M:%f', 'now')";
    stmt << ")";

#ifdef SHOW_STATEMENTS
    std::cout << stmt.str() << std::endl;
#endif

    return stmt.str();
}

///////////////////////////////////////////////////////////////////////////////
// SQL statement used in get() to look up the filename and file size
const std::string llDiskCache::sqlComposeGetSelect(const std::string id)
{
    std::ostringstream stmt;
    stmt << "SELECT ";
    stmt << mFilenameFieldName;
    stmt << ", ";
    stmt << mFilesizeFieldName;
    stmt << " FROM ";
    stmt << mTableName;
    stmt << " WHERE ";
    stmt << mIdFieldName;
    stmt << "='";
    stmt << id;
    stmt << "'";

#ifdef SHOW_STATEMENTS
    std::cout << stmt.str() << std::endl;
#endif

    return stmt.str();
}

///////////////////////////////////////////////////////////////////////////////
// SQL statement to update the date/time of last access as well as the
// count of number of times the file has been accessed.
// Note: the more accurate representation of date/time is used to
// ensure ms accuracy vs the standard INTEGER days since epoch approach
const std::string llDiskCache::sqlComposeGetUpdate(const std::string id)
{
    std::ostringstream stmt;
    stmt << "UPDATE ";
    stmt << mTableName;
    stmt << " SET ";
    stmt << mAccessCountFieldName;
    stmt << "=";
    stmt << mAccessCountFieldName;
    stmt << "+1";
    stmt << ", ";
    stmt << mLastAccessDateTimeFieldName;
    stmt << "=";
    stmt << "strftime('%Y-%m-%d %H:%M:%f', 'now')";
    stmt << " WHERE ";
    stmt << mIdFieldName;
    stmt << "='";
    stmt << id;
    stmt << "'";

#ifdef SHOW_STATEMENTS
    std::cout << stmt.str() << std::endl;
#endif

    return stmt.str();
}

///////////////////////////////////////////////////////////////////////////////
// SQL statement to remove items from the database that are older
// than the newest num_elements entries
const std::string llDiskCache::sqlComposePurge(int num_entries)
{
    std::ostringstream stmt;
    stmt << "DELETE FROM ";
    stmt << mTableName;
    stmt << " WHERE ";
    stmt << mLastAccessDateTimeFieldName;
    stmt << " NOT IN ";
    stmt << "(";
    stmt << "SELECT ";
    stmt << mLastAccessDateTimeFieldName;
    stmt << " FROM ";
    stmt << mTableName;
    stmt << " ORDER BY ";
    stmt << mLastAccessDateTimeFieldName;
    stmt << " DESC";
    stmt << " LIMIT ";
    stmt << num_entries;
    stmt << ")";

//#ifdef SHOW_STATEMENTS
    std::cout << stmt.str() << std::endl;
//#endif

    return stmt.str();
}
