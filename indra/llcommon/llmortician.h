/** 
 * @file llmortician.h
 * @brief Base class for delayed deletions.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LLMORTICIAN_H
#define LLMORTICIAN_H

#include "stdtypes.h"
#include <list>

class LL_COMMON_API LLMortician 
{
public:
    LLMortician() { mIsDead = FALSE; }
    static U32 graveyardCount() { return sGraveyard.size(); };
    static U32 logClass(std::stringstream &str);
    static void updateClass();
    virtual ~LLMortician();
    void die();
    BOOL isDead() { return mIsDead; }

    // sets destroy immediate true
    static void setZealous(BOOL b);

private:
    static BOOL sDestroyImmediate;

    BOOL mIsDead;

    static std::list<LLMortician*> sGraveyard;
};

#endif

