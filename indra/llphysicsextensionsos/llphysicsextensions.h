/**
* @file   llphysicsextensions.h
* @author nyx@lindenlab.com
* @brief  LLPhysicsExtensions core shared initialization
*         routines
*
* $LicenseInfo:firstyear=2012&license=lgpl$
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

#ifndef LL_PHYSICS_EXTENSIONS
#define LL_PHYSICS_EXTENSIONS

#include "llpreprocessor.h"
#include "llsd.h"
#include "v3dmath.h"

#define LLPHYSICSEXTENSIONS_VERSION "1.0"

typedef int bool32;

class LLPhysicsExtensions
{

public:
    // Obtain a pointer to the actual implementation
    static LLPhysicsExtensions* getInstance();

    /// @returns false if this is the stub
    static bool isFunctional();

    static bool initSystem();
    static bool quitSystem();

private:
    static bool s_isInitialized;
};

#endif //LL_PATHING_LIBRARY


