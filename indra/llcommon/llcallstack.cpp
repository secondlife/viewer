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

#include "linden_common.h"

#include "llcommon.h"
#include "llcallstack.h"
#include "StackWalker.h"
#include "llthreadlocalstorage.h"

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

bool LLCallStack::contains(const std::string& str)
{
    for (std::vector<std::string>::const_iterator it = m_strings.begin();
         it != m_strings.end(); ++it)
    {
        if (it->find(str) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

std::ostream& operator<<(std::ostream& s, const LLCallStack& call_stack)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
    std::vector<std::string>::const_iterator it;
    for (it=call_stack.m_strings.begin(); it!=call_stack.m_strings.end(); ++it)
    {
        s << *it;
    }
#else
    s << "UNAVAILABLE IN RELEASE";
#endif
    return s;
}

LLContextStrings::LLContextStrings()
{
}

// static
LLContextStrings* LLContextStrings::getThreadLocalInstance()
{
	LLContextStrings *cons = LLThreadLocalSingletonPointer<LLContextStrings>::getInstance();
    if (!cons)
    {
        LLThreadLocalSingletonPointer<LLContextStrings>::setInstance(new LLContextStrings);
    }
	return LLThreadLocalSingletonPointer<LLContextStrings>::getInstance();
}

// static
void LLContextStrings::addContextString(const std::string& str)
{
	LLContextStrings *cons = getThreadLocalInstance();
    //LL_INFOS() << "CTX " << (S32)cons << " ADD " << str << " CNT " << cons->m_contextStrings[str] << LL_ENDL;
	cons->m_contextStrings[str]++;
}

// static
void LLContextStrings::removeContextString(const std::string& str)
{
	LLContextStrings *cons = getThreadLocalInstance();
	cons->m_contextStrings[str]--;
    //LL_INFOS() << "CTX " << (S32)cons << " REMOVE " << str << " CNT " << cons->m_contextStrings[str] << LL_ENDL;
    if (cons->m_contextStrings[str] == 0)
    {
        cons->m_contextStrings.erase(str);
    }
}

// static
bool LLContextStrings::contains(const std::string& str)
{
    const std::map<std::string,S32>& strings =
        LLThreadLocalSingletonPointer<LLContextStrings>::getInstance()->m_contextStrings;
    for (std::map<std::string,S32>::const_iterator it = strings.begin(); it!=strings.end(); ++it)
    {
        if (it->first.find(str) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

// static
void LLContextStrings::output(std::ostream& os)
{
    const std::map<std::string,S32>& strings =
        LLThreadLocalSingletonPointer<LLContextStrings>::getInstance()->m_contextStrings;
    for (std::map<std::string,S32>::const_iterator it = strings.begin(); it!=strings.end(); ++it)
    {
        os << it->first << "[" << it->second << "]" << "\n";
    }
}

// static 
std::ostream& operator<<(std::ostream& s, const LLContextStatus& context_status)
{
    LLThreadLocalSingletonPointer<LLContextStrings>::getInstance()->output(s);
    return s;
}

bool LLContextStatus::contains(const std::string& str)
{
    return LLThreadLocalSingletonPointer<LLContextStrings>::getInstance()->contains(str);
}
