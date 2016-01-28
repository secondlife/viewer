/** 
 * @file llcallstack.h
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

class LLCallStackImpl;

class LLCallStack
{
public:
    LLCallStack(S32 skip_count=0, bool verbose=false);
    std::vector<std::string> m_strings;
    bool m_verbose;
private:
    static LLCallStackImpl *s_impl;
    S32 m_skipCount;
};

LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLCallStack& call_stack);
