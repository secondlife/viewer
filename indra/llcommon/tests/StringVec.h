/**
 * @file   StringVec.h
 * @author Nat Goodspeed
 * @date   2012-02-24
 * @brief  Extend TUT ensure_equals() to handle std::vector<std::string>
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_STRINGVEC_H)
#define LL_STRINGVEC_H

#include <vector>
#include <string>
#include <iostream>

typedef std::vector<std::string> StringVec;

std::ostream& operator<<(std::ostream& out, const StringVec& strings)
{
    out << '(';
    StringVec::const_iterator begin(strings.begin()), end(strings.end());
    if (begin != end)
    {
        out << '"' << *begin << '"';
        while (++begin != end)
        {
            out << ", \"" << *begin << '"';
        }
    }
    out << ')';
    return out;
}

#endif /* ! defined(LL_STRINGVEC_H) */
