/** 
 * @file net.cpp
 * @brief Cross-platform routines for sending and receiving packets.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "net.h"

// system library includes
#include <stdexcept>
#include <stdio.h>

#if !LL_WINDOWS					//  Windows Versions 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#endif

// linden library includes
#include "network.h"
#include "llerror.h"
#include "llhost.h"
#include "lltimer.h"
#include "indra_constants.h"


// Globals
#if LL_WINDOWS

SOCKADDR_IN stDstAddr;
SOCKADDR_IN stSrcAddr;
SOCKADDR_IN stLclAddr;
static WSADATA stWSAData;

#else

struct sockaddr_in stDstAddr;
struct sockaddr_in stSrcAddr;
struct sockaddr_in stLclAddr;

#if LL_DARWIN
#ifndef _SOCKLEN_T
#define _SOCKLEN_T
typedef int socklen_t;
#endif
#endif

#endif


const char* LOOPBACK_ADDRESS_STRING = "127.0.0.1";

#if LL_DARWIN
	// Mac OS X returns an error when trying to set these to 400000.  Smaller values succeed.
	const int	SEND_BUFFER_SIZE	= 200000;
	const int	RECEIVE_BUFFER_SIZE	= 200000;
#else // LL_DARWIN
	const int	SEND_BUFFER_SIZE	= 400000;
	const int	RECEIVE_BUFFER_SIZE	= 400000;
#endif // LL_DARWIN

// universal functions (cross-platform)

LLHost get_sender()
{
	return LLHost(stSrcAddr.sin_addr.s_addr, ntohs(stSrcAddr.sin_port));
}

U32 get_sender_ip(void) 
{
	return stSrcAddr.sin_addr.s_addr;
}

U32 get_sender_port() 
{
	return ntohs(stSrcAddr.sin_port);
}

const char* u32_to_ip_string(U32 ip)
{
	static char buffer[MAXADDRSTR];	 /* Flawfinder: ignore */ 

	// Convert the IP address into a string
	in_addr in;
	in.s_addr = ip;
	char* result = inet_ntoa(in);

	// NULL indicates error in conversion
	if (result != NULL)
	{
		strncpy( buffer, result, MAXADDRSTR );	 /* Flawfinder: ignore */ 
		buffer[MAXADDRSTR-1] = '\0';
		return buffer;
	}
	else
	{
		return "(bad IP addr)";
	}
}


// Returns ip_string if successful, NULL if not.  Copies into ip_string
char *u32_to_ip_string(U32 ip, char *ip_string)
{
	char *result;
	in_addr in;

	// Convert the IP address into a string
	in.s_addr = ip;
	result = inet_ntoa(in);

	// NULL indicates error in conversion
	if (result != NULL)
	{
		//the function signature needs to change to pass in the lengfth of first and last.
		strcpy(ip_string, result);
		return ip_string;
	}
	else
	{
		return NULL;
	}
}


// Wrapper for inet_addr()
U32 ip_string_to_u32(const char* ip_string)
{
	return inet_addr(ip_string);
}


//////////////////////////////////////////////////////////////////////////////////////////
// Windows Versions
//////////////////////////////////////////////////////////////////////////////////////////

#if LL_WINDOWS
 
S32 start_net(S32& socket_out, int& nPort) 
{			
	// Create socket, make non-blocking
    // Init WinSock 
	int nRet;
	int hSocket;

	int snd_size = SEND_BUFFER_SIZE;
	int rec_size = RECEIVE_BUFFER_SIZE;
	int buff_size = 4;
 
	// Initialize windows specific stuff
	if(WSAStartup(0x0202, &stWSAData))
	{
		S32 err = WSAGetLastError();
		WSACleanup();
		llwarns << "Windows Sockets initialization failed, err " << err << llendl;
		return 1;
	}

	// Get a datagram socket
    hSocket = (int)socket(AF_INET, SOCK_DGRAM, 0);
    if (hSocket == INVALID_SOCKET)
	{
		S32 err = WSAGetLastError();
		WSACleanup();
		llwarns << "socket() failed, err " << err << llendl;
		return 2;
	}

	// Name the socket (assign the local port number to receive on)
	stLclAddr.sin_family      = AF_INET;
	stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	stLclAddr.sin_port        = htons(nPort);

	S32 attempt_port = nPort;
	llinfos << "attempting to connect on port " << attempt_port << llendl;
	nRet = bind(hSocket, (struct sockaddr*) &stLclAddr, sizeof(stLclAddr));

	if (nRet == SOCKET_ERROR)
	{
		// If we got an address in use error...
		if (WSAGetLastError() == WSAEADDRINUSE)
		{
			// Try all ports from PORT_DISCOVERY_RANGE_MIN to PORT_DISCOVERY_RANGE_MAX
			for(attempt_port = PORT_DISCOVERY_RANGE_MIN;
				attempt_port <= PORT_DISCOVERY_RANGE_MAX;
				attempt_port++)
			{
				stLclAddr.sin_port = htons(attempt_port);
				llinfos << "trying port " << attempt_port << llendl;
				nRet = bind(hSocket, (struct sockaddr*) &stLclAddr, sizeof(stLclAddr));

				if (!(nRet == SOCKET_ERROR && 
					WSAGetLastError() == WSAEADDRINUSE))
				{
					break;
				}
			}

			if (nRet == SOCKET_ERROR)
			{
				llwarns << "startNet() : Couldn't find available network port." << llendl;
				// Fail gracefully here in release
				return 3;
			}
		}
		else
		// Some other socket error
		{
			llwarns << llformat("bind() port: %d failed, Err: %d\n", nPort, WSAGetLastError()) << llendl;
			// Fail gracefully in release.
			return 4;
		}
	}
	llinfos << "connected on port " << attempt_port << llendl;
	nPort = attempt_port;
	
	// Set socket to be non-blocking
	unsigned long argp = 1;
	nRet = ioctlsocket (hSocket, FIONBIO, &argp);
	if (nRet == SOCKET_ERROR) 
	{
		printf("Failed to set socket non-blocking, Err: %d\n", 
		WSAGetLastError());
	}

	// set a large receive buffer
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_RCVBUF, (char *)&rec_size, buff_size);
	if (nRet)
	{
		llinfos << "Can't set receive buffer size!" << llendl;
	}

	nRet = setsockopt(hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&snd_size, buff_size);
	if (nRet)
	{
		llinfos << "Can't set send buffer size!" << llendl;
	}

	getsockopt(hSocket, SOL_SOCKET, SO_RCVBUF, (char *)&rec_size, &buff_size);
	getsockopt(hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&snd_size, &buff_size);

	llinfos << "startNet - receive buffer size : " << rec_size << llendl;
	llinfos << "startNet - send buffer size    : " << snd_size << llendl;

	//  Setup a destination address
	char achMCAddr[MAXADDRSTR] = " ";	/* Flawfinder: ignore */ 
	stDstAddr.sin_family =      AF_INET;
    stDstAddr.sin_addr.s_addr = inet_addr(achMCAddr);
    stDstAddr.sin_port =        htons(nPort);

	socket_out = hSocket;
	return 0;
}

void end_net()
{
	WSACleanup();
}

S32 receive_packet(int hSocket, char * receiveBuffer)
{
	//  Receives data asynchronously from the socket set by initNet().
	//  Returns the number of bytes received into dataReceived, or zero
	//  if there is no data received.
	int nRet;
	int addr_size = sizeof(struct sockaddr_in);

	nRet = recvfrom(hSocket, receiveBuffer, NET_BUFFER_SIZE, 0, (struct sockaddr*)&stSrcAddr, &addr_size);
	if (nRet == SOCKET_ERROR ) 
	{
		if (WSAEWOULDBLOCK == WSAGetLastError())
			return 0;
		if (WSAECONNRESET == WSAGetLastError())
			return 0;
		llinfos << "receivePacket() failed, Error: " << WSAGetLastError() << llendl;
	}
	
	return nRet;
}

// Returns TRUE on success.
BOOL send_packet(int hSocket, const char *sendBuffer, int size, U32 recipient, int nPort)
{
	//  Sends a packet to the address set in initNet
	//  
	int nRet = 0;
	U32 last_error = 0;

	stDstAddr.sin_addr.s_addr = recipient;
	stDstAddr.sin_port = htons(nPort);
	do
	{
		nRet = sendto(hSocket, sendBuffer, size, 0, (struct sockaddr*)&stDstAddr, sizeof(stDstAddr));					

		if (nRet == SOCKET_ERROR ) 
		{
			last_error = WSAGetLastError();
			if (last_error != WSAEWOULDBLOCK)
			{
				// WSAECONNRESET - I think this is caused by an ICMP "connection refused"
				// message being sent back from a Linux box...  I'm not finding helpful
				// documentation or web pages on this.  The question is whether the packet
				// actually got sent or not.  Based on the structure of this code, I would
				// assume it is.  JNC 2002.01.18
				if (WSAECONNRESET == WSAGetLastError())
				{
					return TRUE;
				}
				llinfos << "sendto() failed to " << u32_to_ip_string(recipient) << ":" << nPort 
					<< ", Error " << last_error << llendl;
			}
		}
	} while (  (nRet == SOCKET_ERROR)
			 &&(last_error == WSAEWOULDBLOCK));

	return (nRet != SOCKET_ERROR);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Linux Versions
//////////////////////////////////////////////////////////////////////////////////////////

#else

//  Create socket, make non-blocking
S32 start_net(S32& socket_out, int& nPort)
{
	int hSocket, nRet;
	int snd_size = SEND_BUFFER_SIZE;
	int rec_size = RECEIVE_BUFFER_SIZE;

	socklen_t buff_size = 4;
    
	//  Create socket
    hSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (hSocket < 0)
	{
		llwarns << "socket() failed" << llendl;
		return 1;
	}

	// Don't bind() if we want the operating system to assign our ports for 
	// us.
	if (NET_USE_OS_ASSIGNED_PORT == nPort)
	{
		// Do nothing; the operating system will do it for us.
	}
	else
	{
	    // Name the socket (assign the local port number to receive on)
		stLclAddr.sin_family      = AF_INET;
		stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		stLclAddr.sin_port        = htons(nPort);
		U32 attempt_port = nPort;
		llinfos << "attempting to connect on port " << attempt_port << llendl;

		nRet = bind(hSocket, (struct sockaddr*) &stLclAddr, sizeof(stLclAddr));
		if (nRet < 0)
		{
			// If we got an address in use error...
			if (errno == EADDRINUSE)
			{
				// Try all ports from PORT_DISCOVERY_RANGE_MIN to PORT_DISCOVERY_RANGE_MAX
				for(attempt_port = PORT_DISCOVERY_RANGE_MIN;
					attempt_port <= PORT_DISCOVERY_RANGE_MAX;
					attempt_port++)
				{
					stLclAddr.sin_port = htons(attempt_port);
					llinfos << "trying port " << attempt_port << llendl;
					nRet = bind(hSocket, (struct sockaddr*) &stLclAddr, sizeof(stLclAddr));
					if (!((nRet < 0) && (errno == EADDRINUSE)))
					{
						break;
					}
				}
				if (nRet < 0)
				{
					llwarns << "startNet() : Couldn't find available network port." << llendl;
					// Fail gracefully in release.
					return 3;
				}
			}
			// Some other socket error
			else
			{
				llwarns << llformat ("bind() port: %d failed, Err: %s\n", nPort, strerror(errno)) << llendl;
				// Fail gracefully in release.
				return 4;
			}
		}
		llinfos << "connected on port " << attempt_port << llendl;
		nPort = attempt_port;
	}
	// Set socket to be non-blocking
 	fcntl(hSocket, F_SETFL, O_NONBLOCK);
	// set a large receive buffer
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_RCVBUF, (char *)&rec_size, buff_size);
	if (nRet)
	{
		llinfos << "Can't set receive size!" << llendl;
	}
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&snd_size, buff_size);
	if (nRet)
	{
		llinfos << "Can't set send size!" << llendl;
	}
	getsockopt(hSocket, SOL_SOCKET, SO_RCVBUF, (char *)&rec_size, &buff_size);
	getsockopt(hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&snd_size, &buff_size);

	llinfos << "startNet - receive buffer size : " << rec_size << llendl;
	llinfos << "startNet - send buffer size    : " << snd_size << llendl;

	//  Setup a destination address
	char achMCAddr[MAXADDRSTR] = "127.0.0.1";	/* Flawfinder: ignore */ 
	stDstAddr.sin_family =      AF_INET;
        stDstAddr.sin_addr.s_addr = inet_addr(achMCAddr);
        stDstAddr.sin_port =        htons(nPort);

	socket_out = hSocket;
	return 0;
}

void end_net()
{
}

int receive_packet(int hSocket, char * receiveBuffer)
{
	//  Receives data asynchronously from the socket set by initNet().
	//  Returns the number of bytes received into dataReceived, or zero
	//  if there is no data received.
	// or -1 if an error occured!
	int nRet;
	socklen_t addr_size = sizeof(struct sockaddr_in);

	nRet = recvfrom(hSocket, receiveBuffer, NET_BUFFER_SIZE, 0, (struct sockaddr*)&stSrcAddr, &addr_size);

	if (nRet == -1)
	{
		// To maintain consistency with the Windows implementation, return a zero for size on error.
		return 0;
	}

	return nRet;
}

BOOL send_packet(int hSocket, const char * sendBuffer, int size, U32 recipient, int nPort)
{
	int		ret;
	BOOL	success;
	BOOL	resend;
	S32		send_attempts = 0;

	stDstAddr.sin_addr.s_addr = recipient;
	stDstAddr.sin_port = htons(nPort);

	do
	{
		ret = sendto(hSocket, sendBuffer, size, 0,	(struct sockaddr*)&stDstAddr, sizeof(stDstAddr));
		send_attempts++;

		if (ret >= 0)
		{
			// successful send
			success = TRUE;
			resend = FALSE;
		}
		else
		{
			// send failed, check to see if we should resend
			success = FALSE;

			if (errno == EAGAIN)
			{
				// say nothing, just repeat send
				llinfos << "sendto() reported buffer full, resending (attempt " << send_attempts << ")" << llendl;
				llinfos << inet_ntoa(stDstAddr.sin_addr) << ":" << nPort << llendl;
				resend = TRUE;
			}
			else if (errno == ECONNREFUSED)
			{
				// response to ICMP connection refused message on earlier send
				llinfos << "sendto() reported connection refused, resending (attempt " << send_attempts << ")" << llendl;
				llinfos << inet_ntoa(stDstAddr.sin_addr) << ":" << nPort << llendl;
				resend = TRUE;
			}
			else
			{
				// some other error
				llinfos << "sendto() failed: " << errno << ", " << strerror(errno) << llendl;
				llinfos << inet_ntoa(stDstAddr.sin_addr) << ":" << nPort << llendl;
				resend = FALSE;
			}
		}
	}
	while ( resend && send_attempts < 3);

	if (send_attempts >= 3)
	{
		llinfos << "sendPacket() bailed out of send!" << llendl;
		return FALSE;
	}

	return success;
}

#endif

//EOF
