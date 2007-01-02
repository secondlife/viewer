/** 
 * @file net.h
 * @brief Cross platform UDP network code.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_NET_H					
#define LL_NET_H

class LLTimer;
class LLHost;

#define NET_BUFFER_SIZE (0x2000)

// Request a free local port from the operating system
#define NET_USE_OS_ASSIGNED_PORT 0

// Returns 0 on success, non-zero on error.
// Sets socket handler/descriptor, changes nPort if port requested is unavailable.
S32		start_net(S32& socket_out, int& nPort);								
void	end_net();

// returns size of packet or -1 in case of error
S32		receive_packet(int hSocket, char * receiveBuffer);

BOOL	send_packet(int hSocket, const char *sendBuffer, int size, U32 recipient, int nPort);	// Returns TRUE on success.

//void	get_sender(char * tmp);
LLHost  get_sender();
U32		get_sender_port();
U32		get_sender_ip(void);

const char*	u32_to_ip_string(U32 ip);					// Returns pointer to internal string buffer, "(bad IP addr)" on failure, cannot nest calls 
char*		u32_to_ip_string(U32 ip, char *ip_string);	// NULL on failure, ip_string on success, you must allocate at least MAXADDRSTR chars
U32			ip_string_to_u32(const char* ip_string);	// Wrapper for inet_addr()

extern const char* LOOPBACK_ADDRESS_STRING;


// useful MTU consts

const S32	MTUBYTES = 1200;	// 1500 = standard Ethernet MTU
const S32	ETHERNET_MTU_BYTES = 1500;
const S32	MTUBITS = MTUBYTES*8;
const S32	MTUU32S = MTUBITS/32;


#endif
