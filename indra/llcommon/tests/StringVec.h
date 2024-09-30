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

#if defined(LL_LLTUT_H)
// Modern compilers require us to define operator<<(std::ostream&, StringVec)
// before the definition of the ensure() template that engages it. The error
// stating that the compiler can't find a viable operator<<() is so perplexing
// that even though I've obviously hit it a couple times before, a new
// instance still caused much head-scratching. This warning is intended to
// demystify any inadvertent future recurrence.
#warning "StringVec.h must be #included BEFORE lltut.h for ensure() to work"
#endif

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
