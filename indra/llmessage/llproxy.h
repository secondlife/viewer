/**
 * @file llsocks5.h
 * @brief Socks 5 implementation
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#ifndef LL_PROXY_H
#define LL_PROXY_H

#include "llcurl.h"
#include "llhost.h"
#include "lliosocket.h"
#include "llmemory.h"
#include "llsingleton.h"
#include "llthread.h"
#include <string>

// Error codes returned from the StartProxy method

#define SOCKS_OK 0
#define SOCKS_CONNECT_ERROR (-1)
#define SOCKS_NOT_PERMITTED (-2)
#define SOCKS_NOT_ACCEPTABLE (-3)
#define SOCKS_AUTH_FAIL (-4)
#define SOCKS_UDP_FWD_NOT_GRANTED (-5)
#define SOCKS_HOST_CONNECT_FAILED (-6)
#define SOCKS_INVALID_HOST (-7)


#ifndef MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN (255 + 1) /* socks5: 255, +1 for len. */
#endif

#define SOCKSMAXUSERNAMELEN 255
#define SOCKSMAXPASSWORDLEN 255

#define SOCKSMINUSERNAMELEN 1
#define SOCKSMINPASSWORDLEN 1

#define SOCKS_VERSION 0x05 // we are using SOCKS 5

#define SOCKS_HEADER_SIZE 10

// SOCKS 5 address/hostname types
#define ADDRESS_IPV4     0x01
#define ADDRESS_HOSTNAME 0x03
#define ADDRESS_IPV6     0x04

// Lets just use our own ipv4 struct rather than dragging in system
// specific headers
union ipv4_address_t {
	U8		octets[4];
	U32		addr32;
};

// SOCKS 5 control channel commands
#define COMMAND_TCP_STREAM    0x01
#define COMMAND_TCP_BIND      0x02
#define COMMAND_UDP_ASSOCIATE 0x03

// SOCKS 5 command replies
#define REPLY_REQUEST_GRANTED     0x00
#define REPLY_GENERAL_FAIL        0x01
#define REPLY_RULESET_FAIL        0x02
#define REPLY_NETWORK_UNREACHABLE 0x03
#define REPLY_HOST_UNREACHABLE    0x04
#define REPLY_CONNECTION_REFUSED  0x05
#define REPLY_TTL_EXPIRED         0x06
#define REPLY_PROTOCOL_ERROR      0x07
#define REPLY_TYPE_NOT_SUPPORTED  0x08

#define FIELD_RESERVED 0x00

// The standard SOCKS 5 request packet
// Push current alignment to stack and set alignment to 1 byte boundary
// This enabled us to use structs directly to set up and receive network packets
// into the correct fields, without fear of boundary alignment causing issues
#pragma pack(push,1)

// SOCKS 5 command packet
struct socks_command_request_t {
	U8		version;
	U8		command;
	U8		reserved;
	U8		atype;
	U32		address;
	U16		port;
};

// Standard SOCKS 5 reply packet
struct socks_command_response_t {
	U8		version;
	U8		reply;
	U8		reserved;
	U8		atype;
	U8		add_bytes[4];
	U16		port;
};

#define AUTH_NOT_ACCEPTABLE 0xFF // reply if preferred methods are not available
#define AUTH_SUCCESS        0x00 // reply if authentication successful

// SOCKS 5 authentication request, stating which methods the client supports
struct socks_auth_request_t {
	U8		version;
	U8		num_methods;
	U8		methods; // We are only using a single method currently
};

// SOCKS 5 authentication response packet, stating server preferred method
struct socks_auth_response_t {
	U8		version;
	U8		method;
};

// SOCKS 5 password reply packet
struct authmethod_password_reply_t {
	U8		version;
	U8		status;
};

// SOCKS 5 UDP packet header
struct proxywrap_t {
	U16		rsv;
	U8		frag;
	U8		atype;
	U32		addr;
	U16		port;
};

#pragma pack(pop) /* restore original alignment from stack */


// Currently selected HTTP proxy type
enum LLHttpProxyType
{
	LLPROXY_SOCKS = 0,
	LLPROXY_HTTP  = 1
};

// Auth types
enum LLSocks5AuthType
{
	METHOD_NOAUTH   = 0x00,	// Client supports no auth
	METHOD_GSSAPI   = 0x01,	// Client supports GSSAPI (Not currently supported)
	METHOD_PASSWORD = 0x02 	// Client supports username/password
};

class LLProxy: public LLSingleton<LLProxy>
{
public:
	LLProxy();
	~LLProxy();

	// Start a connection to the SOCKS 5 proxy
	S32 startSOCKSProxy(LLHost host);

	// Disconnect and clean up any connection to the SOCKS 5 proxy
	void stopSOCKSProxy();

	// Delete LLProxy singleton, destroying the APR pool used by the control channel.
	static void cleanupClass();

	// Set up to use Password auth when connecting to the SOCKS proxy
	bool setAuthPassword(const std::string &username, const std::string &password);

	// Set up to use No Auth when connecting to the SOCKS proxy
	void setAuthNone();

	// get the currently selected auth method
	LLSocks5AuthType getSelectedAuthMethod() const { return mAuthMethodSelected; }

	// static check for enabled status for UDP packets
	static bool isEnabled() { return sUDPProxyEnabled; }

	// static check for enabled status for http packets
	static bool isHTTPProxyEnabled() { return sHTTPProxyEnabled; }

	// Proxy HTTP packets via httpHost, which can be a SOCKS 5 or a HTTP proxy
	// as specified in type
	bool enableHTTPProxy(LLHost httpHost, LLHttpProxyType type);
	bool enableHTTPProxy();

	// Stop proxying HTTP packets
	void disableHTTPProxy();

	// Get the UDP proxy address and port
	LLHost getUDPProxy() const { return mUDPProxy; }

	// Get the SOCKS 5 TCP control channel address and port
	LLHost getTCPProxy() const { return mTCPProxy; }

	// Get the HTTP proxy address and port
	LLHost getHTTPProxy() const { return mHTTPProxy; }

	// Get the currently selected HTTP proxy type
	LLHttpProxyType getHTTPProxyType() const { return mProxyType; }

	// Get the username password in a curl compatible format
	std::string getProxyUserPwdCURL() const { return (mSocksUsername + ":" + mSocksPassword); }

	std::string getSocksPwd() const { return mSocksPassword; }
	std::string getSocksUser() const { return mSocksUsername; }

	// Apply the current proxy settings to a curl request. Doesn't do anything if sHTTPProxyEnabled is false.
	void applyProxySettings(CURL* handle);
	void applyProxySettings(LLCurl::Easy* handle);
	void applyProxySettings(LLCurlEasyRequest* handle);

private:
	// Open a communication channel to the SOCKS 5 proxy proxy, at port messagePort
	S32 proxyHandshake(LLHost proxy, U32 messagePort);

private:
	// socket handle to proxy TCP control channel
	LLSocket::ptr_t mProxyControlChannel;

	// Is the UDP proxy enabled?
	static bool sUDPProxyEnabled;
	// Is the HTTP proxy enabled? 
	// Do not toggle directly, use enableHTTPProxy() and disableHTTPProxy()
	static bool sHTTPProxyEnabled;

	// currently selected http proxy type
	LLHttpProxyType mProxyType;

	// UDP proxy address and port
	LLHost mUDPProxy;
	// TCP proxy control channel address and port
	LLHost mTCPProxy;
	// HTTP proxy address and port
	LLHost mHTTPProxy;

	// SOCKS 5 auth method selected
	LLSocks5AuthType mAuthMethodSelected;

	// SOCKS 5 username
	std::string mSocksUsername;
	// SOCKS 5 password
	std::string mSocksPassword;

	// Vectors to store valid pointers to string options that might have been set on CURL requests.
	// This results in a behavior similar to LLCurl::Easy::setoptstring()
	std::vector<char*> mSOCKSAuthStrings;
	std::vector<char*> mHTTPProxyAddrStrings;

	// Mutex to protect members in cross-thread calls to applyProxySettings()
	LLMutex mProxyMutex;

	// APR pool for the socket
	apr_pool_t* mPool;
};

#endif
