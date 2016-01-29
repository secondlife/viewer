/** 
 * @file llcallstack.cpp
 * @brief run-time extraction of the current callstack
 *
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2016, Linden Research, Inc.
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

#include "llcommon.h"
#include "llcallstack.h"
#include "llstacktrace.h"
#include "StackWalker.h"

#if LL_WINDOWS
class LLCallStackImpl: public StackWalker
{
public:
    LLCallStackImpl():
        StackWalker(false,0) // non-verbose, options = 0
    {
    }
    ~LLCallStackImpl()
    {
    }
    void getStack(std::vector<std::string>& stack, S32 skip_count=0, bool verbose=false)
    {
        m_stack.clear();
        ShowCallstack(verbose);
        // Skip the first few lines because they're just bookkeeping for LLCallStack,
        // plus any additional lines requested to skip.
        S32 first_line = skip_count + 3;
        for (S32 i=first_line; i<m_stack.size(); ++i)
        {
            stack.push_back(m_stack[i]);
        }
    }
protected:
    virtual void OnOutput(LPCSTR szText)
    {
        m_stack.push_back(szText);
    }
    std::vector<std::string> m_stack;
};
#else
// Stub - not implemented currently on other platforms.
class LLCallStackImpl
{
public:
    LLCallStackImpl() {}
    ~LLCallStackImpl() {}
    void getStack(std::vector<std::string>& stack, S32 skip_count=0, bool verbose=false)
    {
        stack.clear();
    }
};
#endif

LLCallStackImpl *LLCallStack::s_impl = NULL;

LLCallStack::LLCallStack(S32 skip_count, bool verbose):
    m_skipCount(skip_count),
    m_verbose(verbose)
{
    if (!s_impl)
    {
        s_impl = new LLCallStackImpl;
    }
    LLTimer t;
    s_impl->getStack(m_strings, m_skipCount, m_verbose);
}

std::ostream& operator<<(std::ostream& s, const LLCallStack& call_stack)
{
    std::vector<std::string>::const_iterator it;
    for (it=call_stack.m_strings.begin(); it!=call_stack.m_strings.end(); ++it)
    {
        s << *it;
    }
    std::vector<std::string> og_lines;
    ll_get_stack_trace(og_lines);
    s << "\nOLD SCHOOL STACK\n";
    for (it=og_lines.begin(); it!=og_lines.end(); ++it)
    {
        s << *it;
    }
    
    return s;
}
