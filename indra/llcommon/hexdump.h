/**
 * @file   hexdump.h
 * @author Nat Goodspeed
 * @date   2023-10-03
 * @brief  iostream manipulators to stream hex, or string with nonprinting chars
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Copyright (c) 2023, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_HEXDUMP_H)
#define LL_HEXDUMP_H

#include <cctype>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace LL
{

// Format a given byte string as 2-digit hex values, no separators
// Usage: std::cout << hexdump(somestring) << ...
class hexdump
{
public:
    hexdump(const std::string_view& data):
        hexdump(data.data(), data.length())
    {}

    hexdump(const char* data, size_t len):
        hexdump(reinterpret_cast<const unsigned char*>(data), len)
    {}

    hexdump(const std::vector<unsigned char>& data):
        hexdump(data.data(), data.size())
    {}

    hexdump(const unsigned char* data, size_t len):
        mData(data, data + len)
    {}

    friend std::ostream& operator<<(std::ostream& out, const hexdump& self)
    {
        auto oldfmt{ out.flags() };
        auto oldfill{ out.fill() };
        out.setf(std::ios_base::hex, std::ios_base::basefield);
        out.fill('0');
        for (auto c : self.mData)
        {
            out << std::setw(2) << unsigned(c);
        }
        out.setf(oldfmt, std::ios_base::basefield);
        out.fill(oldfill);
        return out;
    }

private:
    std::vector<unsigned char> mData;
};

// Format a given byte string as a mix of printable characters and, for each
// non-printable character, "\xnn"
// Usage: std::cout << hexmix(somestring) << ...
class hexmix
{
public:
    hexmix(const std::string_view& data):
        mData(data)
    {}

    hexmix(const char* data, size_t len):
        mData(data, len)
    {}

    friend std::ostream& operator<<(std::ostream& out, const hexmix& self)
    {
        auto oldfmt{ out.flags() };
        auto oldfill{ out.fill() };
        out.setf(std::ios_base::hex, std::ios_base::basefield);
        out.fill('0');
        for (auto c : self.mData)
        {
            // std::isprint() must be passed an unsigned char!
            if (std::isprint(static_cast<unsigned char>(c)))
            {
                out << c;
            }
            else
            {
                out << "\\x" << std::setw(2) << unsigned(c);
            }
        }
        out.setf(oldfmt, std::ios_base::basefield);
        out.fill(oldfill);
        return out;
    }

private:
    std::string mData;
};

} // namespace LL

#endif /* ! defined(LL_HEXDUMP_H) */
