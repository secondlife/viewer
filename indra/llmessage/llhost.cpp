/** 
 * @file llhost.cpp
 * @brief Encapsulates an IP address and a port.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llhost.h"

#include "llerror.h"

#if LL_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
#else
	#include <netdb.h>
	#include <netinet/in.h>	// ntonl()
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
#endif

LLHost LLHost::invalid(INVALID_PORT,INVALID_HOST_IP_ADDRESS);

LLHost::LLHost(const std::string& ip_and_port)
{
	std::string::size_type colon_index = ip_and_port.find(":");
	if (colon_index != std::string::npos)
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

void LLHost::getString(char* buffer, S32 length) const
{
	if (((U32) length) < MAXADDRSTR + 1 + 5)
	{
		llerrs << "LLHost::getString - string too short" << llendl;
		return;
	}

	snprintf(buffer, length, "%s:%u", u32_to_ip_string(mIP), mPort);  /*Flawfinder: ignore*/
}

void LLHost::getIPString(char* buffer, S32 length) const
{
	if ( ((U32) length) < MAXADDRSTR)
	{
		llerrs << "LLHost::getIPString - string too short" << llendl;
		return;
	}

	snprintf(buffer, length, "%s", u32_to_ip_string(mIP)); /*Flawfinder: ignore*/
}


std::string LLHost::getIPandPort() const
{
	char buffer[MAXADDRSTR + 1 + 5];	/*Flawfinder: ignore*/
	getString(buffer, sizeof(buffer));
	return buffer;
}


std::string LLHost::getIPString() const
{
	return std::string( u32_to_ip_string( mIP ) );
}


void LLHost::getHostName(char *buf, S32 len) const
{
	hostent *he;

	if (INVALID_HOST_IP_ADDRESS == mIP)
	{
		llwarns << "LLHost::getHostName() : Invalid IP address" << llendl;
		buf[0] = '\0';
		return;
	}
	he = gethostbyaddr((char *)&mIP, sizeof(mIP), AF_INET);
	if (!he)
	{
#if LL_WINDOWS
		llwarns << "LLHost::getHostName() : Couldn't find host name for address " << mIP << ", Error: " 
			<< WSAGetLastError() << llendl;
#else
		llwarns << "LLHost::getHostName() : Couldn't find host name for address " << mIP << ", Error: " 
			<< h_errno << llendl;
#endif
		buf[0] = '\0';						
	}
	else
	{
		strncpy(buf, he->h_name, len); /*Flawfinder: ignore*/
		buf[len-1] = '\0';
	}
}

LLString LLHost::getHostName() const
{
	hostent *he;

	if (INVALID_HOST_IP_ADDRESS == mIP)
	{
		llwarns << "LLHost::getHostName() : Invalid IP address" << llendl;
		return "";
	}
	he = gethostbyaddr((char *)&mIP, sizeof(mIP), AF_INET);
	if (!he)
	{
#if LL_WINDOWS
		llwarns << "LLHost::getHostName() : Couldn't find host name for address " << mIP << ", Error: " 
			<< WSAGetLastError() << llendl;
#else
		llwarns << "LLHost::getHostName() : Couldn't find host name for address " << mIP << ", Error: " 
			<< h_errno << llendl;
#endif
		return "";
	}
	else
	{
		LLString hostname = he->h_name;
		return hostname;
	}
}

BOOL LLHost::setHostByName(const char *string)
{
	hostent *he;
	char local_name[MAX_STRING];  /*Flawfinder: ignore*/

	if (strlen(string)+1 > MAX_STRING) /*Flawfinder: ignore*/
	{
		llerrs << "LLHost::setHostByName() : Address string is too long: " 
			<< string << llendl;
	}

	strncpy(local_name, string,MAX_STRING);  /*Flawfinder: ignore*/
	local_name[MAX_STRING-1] = '\0';
#if LL_WINDOWS
	// We may need an equivalent for Linux, but not sure - djs
	_strupr(local_name);
#endif

	he = gethostbyname(local_name);	
	if(!he) 
	{
		U32 ip_address = inet_addr(string);
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
			case TRY_AGAIN:	// XXX how to handle this case? 
				llwarns << "LLHost::setAddress(): try again" << llendl;
				break;
			case HOST_NOT_FOUND:
			case NO_ADDRESS:	// NO_DATA
				llwarns << "LLHost::setAddress(): host not found" << llendl;
				break;
			case NO_RECOVERY:
				llwarns << "LLHost::setAddress(): unrecoverable error" << llendl;
				break;
			default:
				llwarns << "LLHost::setAddress(): unknown error - " << error_number << llendl;
				break;
		}
		return FALSE;
	}
}

LLHost&	LLHost::operator=(const LLHost &rhs)
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


std::istream& operator>> (std::istream& is, LLHost &rh)
{
	is >> rh.mIP;
    is >> rh.mPort;
    return is;
}
