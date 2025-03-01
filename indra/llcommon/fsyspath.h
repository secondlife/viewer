/**
 * @file   fsyspath.h
 * @author Nat Goodspeed
 * @date   2024-04-03
 * @brief  Adapt our UTF-8 std::strings for std::filesystem::path
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_FSYSPATH_H)
#define LL_FSYSPATH_H

#include <filesystem>

// While std::filesystem::path can be directly constructed from std::string on
// both Posix and Windows, that's not what we want on Windows. Per
// https://en.cppreference.com/w/cpp/filesystem/path/path:

// ... the method of conversion to the native character set depends on the
// character type used by source.
//
// * If the source character type is char, the encoding of the source is
//   assumed to be the native narrow encoding (so no conversion takes place on
//   POSIX systems).
// * If the source character type is char8_t, conversion from UTF-8 to native
//   filesystem encoding is used. (since C++20)
// * If the source character type is wchar_t, the input is assumed to be the
//   native wide encoding (so no conversion takes places on Windows).

// The trouble is that on Windows, from std::string ("source character type is
// char"), the "native narrow encoding" isn't UTF-8, so file paths containing
// non-ASCII characters get mangled.
//
// Once we're building with C++20, we could pass a UTF-8 std::string through a
// vector<char8_t> to engage std::filesystem::path's own UTF-8 conversion. But
// sigh, as of 2024-04-03 we're not yet there.
//
// Anyway, encapsulating the important UTF-8 conversions in our own subclass
// allows us to migrate forward to C++20 conventions without changing
// referencing code.

class fsyspath: public std::filesystem::path
{
    using super = std::filesystem::path;

public:
    // default
    fsyspath() {}
    // construct from UTF-8 encoded std::string
    fsyspath(const std::string& path): super(std::filesystem::u8path(path)) {}
    // construct from UTF-8 encoded const char*
    fsyspath(const char* path): super(std::filesystem::u8path(path)) {}
    // construct from existing path
    fsyspath(const super& path): super(path) {}

    fsyspath& operator=(const super& p) { super::operator=(p); return *this; }
    fsyspath& operator=(const std::string& p)
    {
        super::operator=(std::filesystem::u8path(p));
        return *this;
    }
    fsyspath& operator=(const char* p)
    {
        super::operator=(std::filesystem::u8path(p));
        return *this;
    }

    // shadow base-class string() method with UTF-8 aware method
    std::string string() const
    {
        auto u8 = super::u8string();
        return std::string(u8.begin(), u8.end());
    }
    // On Posix systems, where value_type is already char, this operator
    // std::string() method shadows the base class operator string_type()
    // method. But on Windows, where value_type is wchar_t, the base class
    // doesn't have operator std::string(). Provide it.
    operator std::string() const { return string(); }
};

#endif /* ! defined(LL_FSYSPATH_H) */
