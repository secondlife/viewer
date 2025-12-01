/**
 * @file win_wstring_shim.cpp
 * @brief doctest helpers for wide-string output on Windows.
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025,
 * Linden Research, Inc.
 * $/LicenseInfo$
 */

#include "doctest.h"

#include <string>

namespace
{
#ifdef _WIN32
doctest::String wstring_to_doctest(const std::wstring& value)
{
    if (value.empty())
    {
        return doctest::String();
    }

    std::string narrow;
    narrow.reserve(value.size());

    for (wchar_t ch : value)
    {
        if (ch >= 0 && ch <= 0x7f)
        {
            narrow.push_back(static_cast<char>(ch));
        }
        else
        {
            narrow.push_back('?');
        }
    }

    return doctest::String(narrow.c_str());
}
#endif
}

namespace doctest
{
#ifdef _WIN32
String toString(const std::wstring& value)
{
    return wstring_to_doctest(value);
}
#endif
}

