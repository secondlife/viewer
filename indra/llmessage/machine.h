/** 
 * @file machine.h
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

#ifndef LL_MACHINE_H
#define LL_MACHINE_H

#include "net.h"
#include "llhost.h"

typedef enum e_machine_type
{
    MT_NULL,
    MT_SIMULATOR,
    MT_VIEWER,
    MT_SPACE_SERVER,
    MT_OBJECT_REPOSITORY,
    MT_PROXY,
    MT_EOF
} EMachineType;

const U32 ADDRESS_STRING_SIZE = 12;

class LLMachine
{
public:
    LLMachine() 
        : mMachineType(MT_NULL), mControlPort(0) {} 

    LLMachine(EMachineType machine_type, U32 ip, S32 port) 
        : mMachineType(machine_type), mControlPort(0), mHost(ip,port) {}

    LLMachine(EMachineType machine_type, const LLHost &host) 
        : mMachineType(machine_type) {mHost = host; mControlPort = 0;}

    ~LLMachine()    {}

    // get functions
    EMachineType    getMachineType()    const { return mMachineType; }
    U32             getMachineIP()      const { return mHost.getAddress(); }
    S32             getMachinePort()    const { return mHost.getPort(); }
    const LLHost    &getMachineHost()   const { return mHost; }
    // The control port is the listen port of the parent process that
    // launched this machine. 0 means none or not known.
    const S32       &getControlPort()   const { return mControlPort; }
    BOOL            isValid()           const { return (mHost.getPort() != 0); }    // TRUE if corresponds to functioning machine

    // set functions
    void            setMachineType(EMachineType machine_type)   { mMachineType = machine_type; }
    void            setMachineIP(U32 ip)                        { mHost.setAddress(ip); }
    void            setMachineHost(const LLHost &host)          { mHost = host; }

    void            setMachinePort(S32 port);
    void            setControlPort( S32 port );


    // member variables

// Someday these should be made private.
// When they are, some of the code that breaks should 
// become member functions of LLMachine -- Leviathan
//private:

    // I fixed the others, somebody should fix these! - djs
    EMachineType    mMachineType;

protected:

    S32             mControlPort;
    LLHost          mHost;
};

#endif
