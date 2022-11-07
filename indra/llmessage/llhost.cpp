/** 
 * @file llhost.cpp
 * @brief Encapsulates an IP address and a port.
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

#include "linden_common.h"

#include "llhost.h"

#include "llerror.h"

#if LL_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
#else
    #include <netdb.h>
    #include <netinet/in.h> // ntonl()
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
#endif

LLHost::LLHost(const std::string& ip_and_port)
{
    std::string::size_type colon_index = ip_and_port.find(":");
    if (colon_index == std::string::npos)
    {
        mIP = ip_string_to_u32(ip_and_port.c_str());
        mPort = 0;
    }
    else
    {
        std::string ip_str(ip_and_port, 0, colon_index);
        std::string port_str(ip_and_port, colon_index+1);

        mIP = ip_string_to_u32(ip_str.c_str());
        mPort = atol(port_str.c_str());
    }
}

std::string LLHost::getString() const
{
    return llformat("%s:%u", u32_to_ip_string(mIP), mPort);
}


std::string LLHost::getIPandPort() const
{
    return getString();
}


std::string LLHost::getIPString() const
{
    return std::string( u32_to_ip_string( mIP ) );
}


std::string LLHost::getHostName() const
{
    hostent* he;
    if (INVALID_HOST_IP_ADDRESS == mIP)
    {
        LL_WARNS() << "LLHost::getHostName() : Invalid IP address" << LL_ENDL;
        return std::string();
    }
    he = gethostbyaddr((char *)&mIP, sizeof(mIP), AF_INET);
    if (!he)
    {
#if LL_WINDOWS
        LL_WARNS() << "LLHost::getHostName() : Couldn't find host name for address " << mIP << ", Error: " 
            << WSAGetLastError() << LL_ENDL;
#else
        LL_WARNS() << "LLHost::getHostName() : Couldn't find host name for address " << mIP << ", Error: " 
            << h_errno << LL_ENDL;
#endif
        return std::string();
    }
    else
    {
        return ll_safe_string(he->h_name);
    }
}

BOOL LLHost::setHostByName(const std::string& hostname)
{
    hostent *he;
    std::string local_name(hostname);

#if LL_WINDOWS
    // We may need an equivalent for Linux, but not sure - djs
    LLStringUtil::toUpper(local_name);
#endif

    he = gethostbyname(local_name.c_str()); 
    if(!he) 
    {
        U32 ip_address = ip_string_to_u32(hostname.c_str());
        he = gethostbyaddr((char *)&ip_address, sizeof(ip_address), AF_INET);
    }

    if (he)
    {
        mIP = *(U32 *)he->h_addr_list[0];
        return TRUE;
    }
    else 
    {
        setAddress(local_name);

        // In windows, h_errno is a macro for WSAGetLastError(), so store value here
        S32 error_number = h_errno;
        switch(error_number) 
        {
            case TRY_AGAIN: // XXX how to handle this case? 
                LL_WARNS() << "LLHost::setAddress(): try again" << LL_ENDL;
                break;
            case HOST_NOT_FOUND:
            case NO_ADDRESS:    // NO_DATA
                LL_WARNS() << "LLHost::setAddress(): host not found" << LL_ENDL;
                break;
            case NO_RECOVERY:
                LL_WARNS() << "LLHost::setAddress(): unrecoverable error" << LL_ENDL;
                break;
            default:
                LL_WARNS() << "LLHost::setAddress(): unknown error - " << error_number << LL_ENDL;
                break;
        }
        return FALSE;
    }
}

LLHost& LLHost::operator=(const LLHost &rhs)
{ 
    if (this != &rhs)
    {
        set(rhs.getAddress(), rhs.getPort());
    }
    return *this;       
}


std::ostream& operator<< (std::ostream& os, const LLHost &hh)
{
    os << u32_to_ip_string(hh.mIP) << ":" << hh.mPort ;
    return os;
}


//std::istream& operator>> (std::istream& is, LLHost &rh)
//{
//  is >> rh.mIP;
//    is >> rh.mPort;
//    return is;
//}
