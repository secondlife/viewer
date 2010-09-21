/** 
 * @file net.h
 * @brief Cross platform UDP network code.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
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
void	end_net(S32& socket_out);

// returns size of packet or -1 in case of error
S32		receive_packet(int hSocket, char * receiveBuffer);

BOOL	send_packet(int hSocket, const char *sendBuffer, int size, U32 recipient, int nPort);	// Returns TRUE on success.

//void	get_sender(char * tmp);
LLHost  get_sender();
U32		get_sender_port();
U32		get_sender_ip(void);
LLHost  get_receiving_interface();
U32		get_receiving_interface_ip(void);

const char*	u32_to_ip_string(U32 ip);					// Returns pointer to internal string buffer, "(bad IP addr)" on failure, cannot nest calls 
char*		u32_to_ip_string(U32 ip, char *ip_string);	// NULL on failure, ip_string on success, you must allocate at least MAXADDRSTR chars
U32			ip_string_to_u32(const char* ip_string);	// Wrapper for inet_addr()

extern const char* LOOPBACK_ADDRESS_STRING;
extern const char* BROADCAST_ADDRESS_STRING;


// useful MTU consts

const S32	MTUBYTES = 1200;	// 1500 = standard Ethernet MTU
const S32	ETHERNET_MTU_BYTES = 1500;
const S32	MTUBITS = MTUBYTES*8;
const S32	MTUU32S = MTUBITS/32;


#endif
