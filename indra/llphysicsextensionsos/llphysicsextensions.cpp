/**
* @file   llphysicsextensions.cpp
* @author nyx@lindenlab.com
* @brief  LLPhysicsExtensions core initialization methods
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

#include "linden_common.h"

#include "llphysicsextensions.h"
#include "LLPhysicsExtensionsStubImpl.h"


//disable the undefined symbol optimization
//#pragma warning (disable : 4221)

//=============================================================================

/*static */bool LLPhysicsExtensions::s_isInitialized = false;


/*static*/bool LLPhysicsExtensions::isFunctional()
{
    return false;
}

//=============================================================================

/*static*/LLPhysicsExtensions* LLPhysicsExtensions::getInstance()
{
    if ( !s_isInitialized )
    {
        return NULL;
    }
    else
    {
        return LLPhysicsExtensionsImpl::getInstance();
    }
}

//=============================================================================

/*static */bool LLPhysicsExtensions::initSystem()
{
    bool result = LLPhysicsExtensionsImpl::initSystem();
    if ( result )
    {
        s_isInitialized = true;
    }
    return result;
}
//=============================================================================
/*static */bool LLPhysicsExtensions::quitSystem()
{
    return LLPhysicsExtensionsImpl::quitSystem();
}
//=============================================================================

