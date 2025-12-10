/**
 * @file llfile_tut.cpp
 * @author Frederick Martian
 * @date 2025-11
 * @brief LLFile test cases.
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
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

#include <tut/tut.hpp>
#include "lltut.h"
#include "linden_common.h"
#include "llfile.h"

namespace tut
{
    static void clear_entire_dir(std::filesystem::path &dir)
    {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    static std::filesystem::path append_filename(const std::filesystem::path& dir, const std::string& element)
    {
        std::filesystem::path path = dir;
        return path.append(element);
    }

    static std::filesystem::path get_testdir(const std::filesystem::path& tempdir)
    {
        return append_filename(tempdir, std::string("test_dir"));
    }

    struct llfile_test
    {
        std::filesystem::path tempdir = LLFile::tmpdir();
        std::filesystem::path testdir = get_testdir(tempdir);
    };
    typedef test_group<llfile_test> llfile_test_t;
    typedef llfile_test_t::object   llfile_test_object_t;
    tut::llfile_test_t              tut_llfile_test("llfile_test");

    template<> template<>
    void llfile_test_object_t::test<1>()
    {
        // Test creating directories and files and deleting them and checking if the
        // relevant status functions work as expected
        ensure("LLFile::tmpdir() empty", !tempdir.empty());
        ensure("LLFile::tmpdir() doesn't exist", LLFile::exists(tempdir.string()));
        ensure("LLFile::tmpdir() is not a directory", LLFile::isdir(tempdir.string()));
        ensure("LLFile::tmpdir() should not be a file", !LLFile::isfile(tempdir.string()));

        // Make sure there is nothing left from a previous test run
        clear_entire_dir(testdir);
        ensure("llfile_test should not exist anymore", !LLFile::exists(testdir.string()));

        int rc = LLFile::mkdir(testdir.string());
        ensure("LLFile::mkdir() failed", rc == 0);
        ensure("llfile_test should be a directory", LLFile::isdir(testdir.string()));
        rc = LLFile::mkdir(testdir.string());
        ensure("LLFile::mkdir() should not fail when the directory already exists", rc == 0);

        std::filesystem::path testfile1 = testdir;
        testfile1.append("llfile_test.dat");
        ensure("llfile_test1.dat should not yet exist", !LLFile::exists(testfile1.string()));

        const char* testdata = "testdata";
        std::time_t current = time(nullptr);
        S64 bytes = LLFile::write(testfile1.string(), testdata, 0, sizeof(testdata));
        ensure("LLFile::write() did not write correctly", bytes == sizeof(testdata));

        rc = LLFile::remove(testfile1.string());
        ensure("LLFile::remove() for file test_file.dat", rc == 0);
        ensure("llfile_test.dat should not exist anymore", !LLFile::exists(testfile1.string()));
        ensure("llfile_test.dat should not be a file", !LLFile::isfile(testfile1.string()));
        ensure("llfile_test.dat should not be a directory", !LLFile::isdir(testfile1.string()));
        ensure("llfile_test.dat should not be a symlink", !LLFile::islink(testfile1.string()));

        rc = LLFile::remove(testdir.string());
        ensure("LLFile::remove() for directory llfile_test failed", rc == 0);
        ensure("llfile_test should not exist anymore", !LLFile::exists(testdir.string()));
    }

    template<> template<>
    void llfile_test_object_t::test<2>()
    {
        // High level static file IO functions to read and write data files
        LLFile::mkdir(testdir.string());
        ensure("llfile_test should exist", LLFile::isdir(testdir.string()));

        std::filesystem::path testfile1 = testdir;
        testfile1.append("llfile_test.dat");

        std::string testdata1("testdata");
        std::string testdata2("datateststuff");
        std::time_t current = time(nullptr);
        S64 bytes = LLFile::write(testfile1.string(), testdata1.c_str(), 0, testdata1.length());
        ensure("LLFile::write() did not write correctly", bytes == testdata1.length());
        ensure("llfile_test.dat should exist", LLFile::exists(testfile1.string()));
        ensure("llfile_test.dat should be a file", LLFile::isfile(testfile1.string()));
        ensure("llfile_test.dat should not be a directory", !LLFile::isdir(testfile1.string()));

        bytes = LLFile::size(testfile1.string());
        ensure("LLFile::size() did not return the correct size", bytes == testdata1.length());

        std::string data = LLFile::getContents(testfile1.string());
        ensure("LLFile::getContents() did not return the correct size data", data.length() == testdata1.length());
        ensure_memory_matches("LLFile::getContents() did not read correct data", testdata1.c_str(), (U32)testdata1.length(), data.c_str(), (U32)data.length());

        std::time_t ctime = LLFile::getCreationTime(testfile1.string());
        ensure_approximately_equals_range("LLFile::getCreationTime() did not return correct time", (F32)(ctime - current), 0.f, 1);

        std::time_t mtime = LLFile::getModificationTime(testfile1.string());
        ensure_approximately_equals_range("LLFile::getModificationTime() did not return correct time", (F32)(mtime - current), 0.f, 1);

        char buffer[1024];
        bytes = LLFile::read(testfile1.string(), buffer, 0, testdata1.length());
        ensure("LLFile:read() did not return the correct size", bytes == testdata1.length());
        ensure_memory_matches("LLFile::read() did not read correct data", testdata1.c_str(), (U32)bytes, buffer, (U32)bytes);

        // What if we try to read more data than there is in the file?
        bytes = LLFile::read(testfile1.string(), buffer, 0, bytes + 10);
        ensure("LLFile:read() did not correctly stop on eof", bytes == testdata1.length());
        ensure_memory_matches("LLFile::read() did not read correct data", testdata1.c_str(), (U32)bytes, buffer, (U32)bytes);

        // Let's append more data
        bytes = LLFile::write(testfile1.string(), testdata2.c_str(), -1, testdata2.length());
        ensure("LLFile::write() did not write correctly", bytes == testdata2.length());

        bytes = LLFile::size(testfile1.string());
        ensure("LLFile::size() did not return the correct size", bytes == testdata1.length() + testdata2.length());
        bytes = LLFile::read(testfile1.string(), buffer, 0, bytes);
        ensure("LLFile:read() did not read correct number of bytes", bytes == testdata1.length() + testdata2.length());
        ensure_memory_matches("LLFile:read() did not read correct testdata1", testdata1.c_str(), (U32)testdata1.length(), buffer, (U32)testdata1.length());
        ensure_memory_matches("LLFile:read() did not read correct testdata2", testdata2.c_str(), (U32)testdata2.length(), buffer + testdata1.length(), (U32)testdata2.length());
    }

    template<> template<>
    void llfile_test_object_t::test<3>()
    {
        const size_t numints = 1024;

        // Testing the LLFile class implementation
        std::filesystem::path testfile = testdir;
        testfile.append("llfile_test.bin");

        int data[numints];
        for (int &t : data)
        {
            t = rand();
        }

        std::error_code ec;
        LLFile fileout(testfile.string(), LLFile::out, ec);
        ensure("LLFile constructor did not open correctly", (bool)fileout);
        ensure("error_code from LLFile constructor should not indicate an error", !ec);
        if (fileout)
        {
            S64 length = fileout.size(ec);
            ensure("freshly created file should be empty", length == 0);
            ensure("error_code from LLFile::size() should not indicate an error", !ec);
            S64 bytes = fileout.write(data, sizeof(data), ec);
            ensure("LLFile::write() did not write correctly", bytes == sizeof(data));
            ensure("error_code from LLFile::write() should not indicate an error", !ec);
            bytes = fileout.write(data, sizeof(data), ec);
            ensure("LLFile::write() did not write correctly", bytes == sizeof(data));
            ensure("error_code from LLFile::write() should not indicate an error", !ec);
            bytes = fileout.size(ec);
            ensure("LLFile::size() returned wrong size", bytes == 2 * sizeof(data));
            ensure("error_code from LLFile::size() should not indicate an error", !ec);
            fileout.close();
        }

        LLFile filein(testfile.string(), LLFile::in, ec);
        ensure("LLFile constructor did not open correctly", (bool)filein);
        ensure("error_code from LLFile constructor should not indicate an error", !ec);
        if (filein)
        {
            S64 length = filein.size(ec);
            ensure("LLFile::size() returned wrong size", length == 2 * sizeof(data));
            ensure("error_code from LLFile::size() should not indicate an error", !ec);
            char* buffer = (char*)malloc(length);
            S64 bytes  = filein.read(buffer, length, ec);
            ensure("LLFile::read() did not read correctly", bytes == length);
            ensure("error_code from LLFile::read() should not indicate an error", !ec);
            ensure_memory_matches("LLFile:read() did not read correct data1", data, (U32)sizeof(data), buffer, (U32)sizeof(data));
            ensure_memory_matches("LLFile:read() did not read correct data2", data, (U32)sizeof(data), buffer + sizeof(data), (U32)sizeof(data));
            S64 offset = filein.tell(ec);
            ensure("LLFile::tell() returned a bad offset", offset == length);
            ensure("error_code from LLFile::read() should not indicate an error", !ec);
            offset = sizeof(data) / 2;
            int rc = filein.seek(offset, ec);
            ensure("LLFile::seek() indicated an error", rc == 0);
            ensure("error_code from LLFile::seek() should not indicate an error", !ec);
            bytes = filein.read(buffer, 2 * sizeof(data), ec);
            ensure("LLFile::read() did not read correctly", bytes == sizeof(data) + offset);
            ensure("error_code from LLFile::read() should not indicate an error", !ec);
            ensure_memory_matches("LLFile:read() did not read correct data3", (char*)data + offset, (U32)offset, buffer, (U32)offset);
            ensure_memory_matches("LLFile:read() did not read correct data4", (char*)data, (U32)sizeof(data), buffer + offset, (U32)sizeof(data));
            filein.close();

            free(buffer);
        }
    }

    template<> template<>
    void llfile_test_object_t::test<4>()
    {
        // Testing the LLFile class implementation with wrong paths and parameters
        std::filesystem::path testfile = testdir;
        testfile.append("llfile_test.bin");

        std::error_code ec;
        LLFile file(testfile.string(), LLFile::out | LLFile::noreplace, ec);
        ensure("LLFile constructor should not have opened the already existing file", !file);
        ensure("error_code from LLFile constructor should indicate an error", (bool)ec);

        LLFile::remove(testfile.string());
        file = LLFile(testfile.string(), LLFile::out | LLFile::app | LLFile::trunc, ec);
        ensure("LLFile constructor should not have opened the file with conflicting flags", !file);
        ensure("error_code from LLFile constructor should indicate an error", (bool)ec);

        file = LLFile(testfile.string(), LLFile::out | LLFile::app | LLFile::noreplace, ec);
        ensure("LLFile constructor should not have opened the file with conflicting flags", !file);
        ensure("error_code from LLFile constructor should indicate an error", (bool)ec);

        testfile = testdir;
        testfile.append("llfile_test");
        testfile.append("llfile_test.bin");

        file = LLFile(testfile.string(), LLFile::in, ec);
        ensure("LLFile constructor should not have been able to open the file in the non-existing directory", !file);
        ensure("error_code from LLFile constructor should indicate an error", (bool)ec);
    }
} // namespace tut
