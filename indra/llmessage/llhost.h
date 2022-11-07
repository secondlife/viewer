/** 
 * @file llhost.h
 * @brief a LLHost uniquely defines a host (Simulator, Proxy or other)
 * across the network
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLHOST_H
#define LL_LLHOST_H

#include <iostream>
#include <string>

#include "net.h"

const U32 INVALID_PORT = 0;
const U32 INVALID_HOST_IP_ADDRESS = 0x0;

class LLHost {
protected:
    U32         mPort;
    U32         mIP;
    std::string mUntrustedSimCap;
public:

    // CREATORS
    LLHost()
    :   mPort(INVALID_PORT),
        mIP(INVALID_HOST_IP_ADDRESS)
    { } // STL's hash_map expect this T()

    LLHost( U32 ipv4_addr, U32 port )
    :   mPort( port ) 
    {
        mIP = ipv4_addr;
    }

    LLHost( const std::string& ipv4_addr, U32 port )
    :   mPort( port )
    { 
        mIP = ip_string_to_u32(ipv4_addr.c_str());
    }

    explicit LLHost(const U64 ip_port)
    {
        U32 ip = (U32)(ip_port >> 32);
        U32 port = (U32)(ip_port & (U64)0xFFFFFFFF);
        mIP = ip;
        mPort = port;
    }

    explicit LLHost(const std::string& ip_and_port);

    ~LLHost()
    { }

    // MANIPULATORS
    void    set( U32 ip, U32 port )             { mIP = ip; mPort = port; }
    void    set( const std::string& ipstr, U32 port )   { mIP = ip_string_to_u32(ipstr.c_str()); mPort = port; }
    void    setAddress( const std::string& ipstr )      { mIP = ip_string_to_u32(ipstr.c_str()); }
    void    setAddress( U32 ip )                { mIP = ip; }
    void    setPort( U32 port )                 { mPort = port; }
    BOOL    setHostByName(const std::string& hname);

    LLHost& operator=(const LLHost &rhs);
    void    invalidate()                        { mIP = INVALID_HOST_IP_ADDRESS; mPort = INVALID_PORT;};

    // READERS
    U32     getAddress() const                          { return mIP; }
    U32     getPort() const                             { return mPort; }
    bool    isOk() const                                { return (mIP != INVALID_HOST_IP_ADDRESS) && (mPort != INVALID_PORT); }
    bool    isInvalid()                                 { return (mIP == INVALID_HOST_IP_ADDRESS) || (mPort == INVALID_PORT); }
    size_t  hash() const                                { return (mIP << 16) | (mPort & 0xffff); }
    std::string getString() const;
    std::string getIPString() const;
    std::string getHostName() const;
    std::string getIPandPort() const;

    std::string getUntrustedSimulatorCap() const        { return mUntrustedSimCap; }
    void setUntrustedSimulatorCap(const std::string &capurl) { mUntrustedSimCap = capurl; }

    friend std::ostream& operator<< (std::ostream& os, const LLHost &hh);

    // This operator is not well defined. does it expect a
    // "192.168.1.1:80" notation or "int int" format? Phoenix 2007-05-18
    //friend std::istream& operator>> (std::istream& is, LLHost &hh);

    friend bool operator==( const LLHost &lhs, const LLHost &rhs );
    friend bool operator!=( const LLHost &lhs, const LLHost &rhs );
    friend bool operator<(const LLHost &lhs, const LLHost &rhs);
};


// Function Object required for STL templates using LLHost as key 
class LLHostHash
{
public:
    size_t operator() (const LLHost &hh) const { return hh.hash(); }
};


inline bool operator==( const LLHost &lhs, const LLHost &rhs )
{
    return (lhs.mIP == rhs.mIP) && (lhs.mPort == rhs.mPort);
}

inline bool operator!=( const LLHost &lhs, const LLHost &rhs )
{
    return (lhs.mIP != rhs.mIP) || (lhs.mPort != rhs.mPort);
}

inline bool operator<(const LLHost &lhs, const LLHost &rhs)
{
    if (lhs.mIP < rhs.mIP)
    {
        return true;
    }
    if (lhs.mIP > rhs.mIP)
    {
        return false;
    }

    if (lhs.mPort < rhs.mPort)
    {
        return true;
    }
    else
    {
        return false;
    }
}


#endif // LL_LLHOST_H
