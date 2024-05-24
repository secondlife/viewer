/**
 * @file machine.cpp
 * @brief LLMachine class header file
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "linden_common.h"

#include "machine.h"

#include "llerror.h"

void LLMachine::setMachinePort(S32 port)
{
    if (port < 0)
    {
        LL_INFOS() << "Can't assign a negative number to LLMachine::mPort" << LL_ENDL;
        mHost.setPort(0);
    }
    else
    {
        mHost.setPort(port);
    }
}

void LLMachine::setControlPort( S32 port )
{
    if (port < 0)
    {
        LL_INFOS() << "Can't assign a negative number to LLMachine::mControlPort" << LL_ENDL;
        mControlPort = 0;
    }
    else
    {
        mControlPort = port;
    }
}
