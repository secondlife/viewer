/** 
 * @file llhost.h
 * @brief a LLHost uniquely defines a host (Simulator, Proxy or other)
 * across the network
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLHOST_H
#define LL_LLHOST_H

#include <iostream>
#include <string>

#include "net.h"

#include "llstring.h"

const U32 INVALID_PORT = 0;
const U32 INVALID_HOST_IP_ADDRESS = 0x0;

class LLHost {
protected:
	U32			mPort;
	U32         mIP;
public:
	
	static LLHost invalid;

	// CREATORS
	LLHost()
	:	mPort(INVALID_PORT),
		mIP(INVALID_HOST_IP_ADDRESS)
	{ } // STL's hash_map expect this T()

	LLHost( U32 ipv4_addr, U32 port )
	:	mPort( port ) 
	{
		mIP = ipv4_addr;
	}

	LLHost( const char *ipv4_addr, U32 port )
	:	mPort( port )
	{ 
		mIP = ip_string_to_u32(ipv4_addr);
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
	void	set( U32 ip, U32 port )				{ mIP = ip; mPort = port; }
	void	set( const char* ipstr, U32 port )	{ mIP = ip_string_to_u32(ipstr); mPort = port; }
	void	setAddress( const char* ipstr )		{ mIP = ip_string_to_u32(ipstr); }
	void	setAddress( U32 ip )				{ mIP = ip; }
	void	setPort( U32 port )					{ mPort = port; }
	BOOL    setHostByName(const char *hname);

	LLHost&	operator=(const LLHost &rhs);
	void    invalidate()                        { mIP = INVALID_HOST_IP_ADDRESS; mPort = INVALID_PORT;};

	// READERS
	U32		getAddress() const							{ return mIP; }
	U32		getPort() const								{ return mPort; }
	BOOL	isOk() const								{ return (mIP != INVALID_HOST_IP_ADDRESS) && (mPort != INVALID_PORT); }
	size_t	hash() const								{ return (mIP << 16) | (mPort & 0xffff); }
	void	getString(char* buffer, S32 length) const;	    // writes IP:port into buffer
	void	getIPString(char* buffer, S32 length) const;	// writes IP into buffer
	std::string getIPString() const;
	void    getHostName(char *buf, S32 len) const;
	LLString getHostName() const;
	std::string getIPandPort() const;

	friend std::ostream& operator<< (std::ostream& os, const LLHost &hh);
	friend std::istream& operator>> (std::istream& is, LLHost &hh);
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
