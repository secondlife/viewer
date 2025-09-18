/**
* @file   llconvexdecomposition.cpp
* @author falcon@lindenlab.com
* @brief  Inner implementation of LLConvexDecomposition interface
*
* $LicenseInfo:firstyear=2011&license=lgpl$
* Second Life Viewer Source Code
* Copyright (C) 2011, Linden Research, Inc.
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

#include "llconvexdecompositionvhacd.h"
#include "llconvexdecomposition.h"

bool LLConvexDecomposition::s_isInitialized = false;

// static
bool LLConvexDecomposition::isFunctional()
{
    return LLConvexDecompositionVHACD::isFunctional();
}

// static
LLConvexDecomposition* LLConvexDecomposition::getInstance()
{
    if ( !s_isInitialized )
    {
        return nullptr;
    }
    else
    {
        return LLConvexDecompositionVHACD::getInstance();
    }
}

// static
LLCDResult LLConvexDecomposition::initSystem()
{
    LLCDResult result = LLConvexDecompositionVHACD::initSystem();
    if ( result == LLCD_OK )
    {
        s_isInitialized = true;
    }
    return result;
}

// static
LLCDResult LLConvexDecomposition::initThread()
{
    return LLConvexDecompositionVHACD::initThread();
}

// static
LLCDResult LLConvexDecomposition::quitThread()
{
    return LLConvexDecompositionVHACD::quitThread();
}

// static
LLCDResult LLConvexDecomposition::quitSystem()
{
    return LLConvexDecompositionVHACD::quitSystem();
}


