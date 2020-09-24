/**
 * @file lldiskcache.h
 * @brief Declaration SQLite meta data / file storage API
 * @brief Declaration of the generic disk cache interface
          as well the SQLite header/implementation.
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

#ifndef _LLDISKCACHE
#define _LLDISKCACHE

#include <iostream>
#include <string>

#include "sqlite3.h"

// Enable this line to display each SQL statement when it is executed
// It can lead to a lot of spam but useful for debugging
//#define SHOW_STATEMENTS

// I toyed with the idea of a variety of approaches and thought have
// an abstract base class with which to hang different implementations
// off was a good idea but I think we settled on a single approach
// so I will likely remove this level of indirection if not other 
// interesting implementation ideas present themselves.
class llDiskCacheBase
{
    public:
        llDiskCacheBase() {};
        virtual ~llDiskCacheBase() {};
        virtual bool open(const std::string db_folder,
                          const std::string db_filename) = 0;
        virtual bool exists(const std::string id, bool& exists) = 0;
        virtual bool put(const std::string id,
                         char* binary_data,
                         int binary_data_size) = 0;
        virtual const bool get(const std::string id,
                               char** mem_buffer,
                               int& mem_buffer_size) = 0;
        virtual bool purge(int num_entries) = 0;
        virtual void close() = 0;
        virtual const std::string dbVersion() = 0;
        virtual const std::string getFilenameById(const std::string id) = 0;
        virtual const int getAccessCountById(const std::string id) = 0;
        virtual void getNumEntries(int& num_entries, int& max_entries) = 0;
};

// implementation for the SQLite/disk blended case
// see source file for detailed comments
class llDiskCache :
    public llDiskCacheBase
{
    public:
        llDiskCache();
        virtual ~llDiskCache();
        virtual bool open(const std::string db_folder,
                          const std::string db_filename) override;
        virtual bool exists(const std::string id, bool& exists) override;
        virtual bool put(const std::string id,
                         char* binary_data,
                         int binary_data_size) override;
        virtual const bool get(const std::string id,
                               char** mem_buffer,
                               int& mem_buffer_size) override;
        virtual bool purge(int num_entries) override;
        virtual void close() override;
        virtual const std::string dbVersion() override;
        virtual const std::string getFilenameById(const std::string id) override;
        virtual const int getAccessCountById(const std::string id) override;
        virtual void getNumEntries(int& num_entries, int& max_entries) override;

    private:
        sqlite3* mDb;
        std::string mDataStorePath;
        const std::string mTableName = "lldiskcache";
        const std::string mIdFieldName = "id";
        const std::string mFilenameFieldName = "filename";
        const std::string mFilesizeFieldName = "filesize";
        const std::string mInsertionDateTimeFieldName = "insert_datetime";
        const std::string mLastAccessDateTimeFieldName = "last_access_datetime";
        const std::string mAccessCountFieldName = "access_count";

    private:
        void printError(const std::string funcname,
                        const std::string desc,
                        bool is_sqlite_err);
        bool beginTransaction();
        bool endTransaction();
        std::string makeUniqueFilename();
        const std::string systemSeparator();
        const std::string makeFullPath(const std::string filename);
        bool sqliteExec(const std::string stmt,
                        const std::string funcname);
        sqlite3_stmt* sqlitePrepareStep(const std::string funcname,
                                        const std::string stmt,
                                        int sqlite_success_condition);
        const std::string sqlComposeCreateTable();
        const std::string sqlComposeExists(const std::string id);
        const std::string sqlComposePut(const std::string id,
                                        const std::string filename,
                                        int binary_data_size);
        const std::string sqlComposeGetSelect(const std::string id);
        const std::string sqlComposeGetUpdate(const std::string id);
        const std::string sqlComposePurge(int num_entries);
};

#endif // _LLDISKCACHE
