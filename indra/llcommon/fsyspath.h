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

#include <boost/iterator/transform_iterator.hpp>
#include <filesystem>
#include <string>
#include <string_view>

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
// Encapsulating the important UTF-8 conversions in our own subclass allows us
// to migrate forward to C++20 conventions without changing referencing code.

class fsyspath: public std::filesystem::path
{
    using super = std::filesystem::path;

    // In C++20 (__cpp_lib_char8_t), std::filesystem::u8path() is deprecated.
    // std::filesystem::path(iter, iter) performs UTF-8 conversions when the
    // value_type of the iterators is char8_t. While we could copy into a
    // temporary std::u8string and from there into std::filesystem::path, to
    // minimize string copying we'll define a transform_iterator that accepts
    // a std::string_view::iterator and dereferences to char8_t.
    struct u8ify
    {
        char8_t operator()(char c) const { return char8_t(c); }
    };
    using u8iter = boost::transform_iterator<u8ify, std::string_view::iterator>;

public:
    // default
    fsyspath() {}
    // construct from UTF-8 encoded string
    fsyspath(const std::string& path): fsyspath(std::string_view(path)) {}
    fsyspath(const char* path):        fsyspath(std::string_view(path)) {}
    fsyspath(std::string_view path):
        super(u8iter(path.begin(), u8ify()), u8iter(path.end(), u8ify()))
    {}
    // construct from existing path
    fsyspath(const super& path): super(path) {}

    fsyspath& operator=(const super& p)       { super::operator=(p); return *this; }
    fsyspath& operator=(const std::string& p) { return (*this) = std::string_view(p); }
    fsyspath& operator=(const char* p)        { return (*this) = std::string_view(p); }
    fsyspath& operator=(std::string_view p)
    {
        assign(u8iter(p.begin(), u8ify()), u8iter(p.end(), u8ify()));
        return *this;
    }

    // shadow base-class string() method with UTF-8 aware method
    std::string string() const
    {
        // Short of forbidden type punning, I see no way to avoid copying this
        // std::u8string to a std::string.
        auto u8str{ super::u8string() };
        // from https://github.com/tahonermann/char8_t-remediation/blob/master/char8_t-remediation.h#L180-L182
        return { u8str.begin(), u8str.end() };
    }
    // On Posix systems, where value_type is already char, this operator
    // std::string() method shadows the base class operator string_type()
    // method. But on Windows, where value_type is wchar_t, the base class
    // doesn't have operator std::string(). Provide it.
    operator std::string() const { return string(); }
};

#endif /* ! defined(LL_FSYSPATH_H) */
