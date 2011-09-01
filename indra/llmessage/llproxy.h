/**
 * @file llproxy.h
 * @brief UDP and HTTP proxy communications
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

// SOCKS error codes returned from the StartProxy method
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

/**
 * @brief Manage SOCKS 5 UDP proxy and HTTP proxy.
 *
 * This class is responsible for managing two interconnected tasks,
 * connecting to a SOCKS 5 proxy for use by LLPacketRing to send UDP
 * packets and managing proxy settings for HTTP requests.
 *
 * <h1>Threading:</h1>
 * Because HTTP requests can be generated in threads outside the
 * main thread, it is necessary for some of the information stored
 * by this class to be available to other threads. The members that
 * need to be read across threads are in a labeled section below.
 * To protect those members, a mutex, mProxyMutex should be locked
 * before reading or writing those members.  Methods that can lock
 * mProxyMutex are in a labeled section below. Those methods should
 * not be called while the mutex is already locked.
 *
 * There is also a LLAtomic type flag (mHTTPProxyEnabled) that is used
 * to track whether the HTTP proxy is currently enabled. This allows
 * for faster unlocked checks to see if the proxy is enabled.  This
 * allows us to cut down on the performance hit when the proxy is
 * disabled compared to before this class was introduced.
 *
 * <h1>UDP Proxying:</h1>
 * UDP datagrams are proxied via a SOCKS 5 proxy with the UDP associate
 * command.  To initiate the proxy, a TCP socket connection is opened
 * to the SOCKS 5 host, and after a handshake exchange, the server
 * returns a port and address to send the UDP traffic that is to be
 * proxied to. The LLProxy class tracks this address and port after the
 * exchange and provides it to LLPacketRing when required to. All UDP
 * proxy management occurs in the main thread.
 *
 * <h1>HTTP Proxying:</h1>
 * This class allows all viewer HTTP packets to be sent through a proxy.
 * The user can select whether to send HTTP packets through a standard
 * "web" HTTP proxy, through a SOCKS 5 proxy, or to not proxy HTTP
 * communication. This class does not manage the integrated web browser
 * proxy, which is handled in llviewermedia.cpp.
 *
 * The implementation of HTTP proxying is handled by libcurl. LLProxy
 * is responsible for managing the HTTP proxy options and provides a
 * thread-safe method to apply those options to a curl request
 * (LLProxy::applyProxySettings()). This method is overloaded
 * to accommodate the various abstraction libcurl layers that exist
 * throughout the viewer (LLCurlEasyRequest, LLCurl::Easy, and CURL).
 *
 * If you are working with LLCurl or LLCurlEasyRequest objects,
 * the configured proxy settings will be applied in the constructors
 * of those request handles.  If you are working with CURL objects
 * directly, you will need to pass the handle of the request to
 * applyProxySettings() before issuing the request.
 *
 * To ensure thread safety, all LLProxy members that relate to the HTTP
 * proxy require the LLProxyMutex to be locked before accessing.
 */
class LLProxy: public LLSingleton<LLProxy>
{
	LOG_CLASS(LLProxy);
public:
	/*###########################################################################################
	METHODS THAT DO NOT LOCK mProxyMutex!
	###########################################################################################*/
	LLProxy();

	// static check for enabled status for UDP packets
	static bool isSOCKSProxyEnabled() { return sUDPProxyEnabled; }

	// check for enabled status for HTTP packets
	// mHTTPProxyEnabled is atomic, so no locking is required for thread safety.
	bool isHTTPProxyEnabled() const { return mHTTPProxyEnabled; }

	// Get the UDP proxy address and port
	LLHost getUDPProxy() const { return mUDPProxy; }

	// Get the SOCKS 5 TCP control channel address and port
	LLHost getTCPProxy() const { return mTCPProxy; }

	/*###########################################################################################
	END OF NON-LOCKING METHODS
	###########################################################################################*/

	/*###########################################################################################
	METHODS THAT DO LOCK mProxyMutex! DO NOT CALL WHILE mProxyMutex IS LOCKED!
	###########################################################################################*/
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

	// Get the currently selected auth method.
	LLSocks5AuthType getSelectedAuthMethod() const;

	// Proxy HTTP packets via httpHost, which can be a SOCKS 5 or a HTTP proxy
	// as specified in type
	bool enableHTTPProxy(LLHost httpHost, LLHttpProxyType type);
	bool enableHTTPProxy();

	// Stop proxying HTTP packets
	void disableHTTPProxy();

	// Apply the current proxy settings to a curl request. Doesn't do anything if mHTTPProxyEnabled is false.
	void applyProxySettings(CURL* handle);
	void applyProxySettings(LLCurl::Easy* handle);
	void applyProxySettings(LLCurlEasyRequest* handle);

	// Get the HTTP proxy address and port
	LLHost getHTTPProxy() const;

	// Get the currently selected HTTP proxy type
	LLHttpProxyType getHTTPProxyType() const;

	std::string getSocksPwd() const;
	std::string getSocksUser() const;

	/*###########################################################################################
	END OF LOCKING METHODS
	###########################################################################################*/
private:
	// Open a communication channel to the SOCKS 5 proxy proxy, at port messagePort.
	S32 proxyHandshake(LLHost proxy);

private:
	// Is the HTTP proxy enabled?
	// Safe to read in any thread, do not write directly,
	// use enableHTTPProxy() and disableHTTPProxy() instead.
	mutable LLAtomic32<bool> mHTTPProxyEnabled;

	// Mutex to protect shared members in non-main thread calls to applyProxySettings()
	mutable LLMutex mProxyMutex;

	/*###########################################################################################
	MEMBERS READ AND WRITTEN ONLY IN THE MAIN THREAD. DO NOT SHARE!
	###########################################################################################*/

	// Is the UDP proxy enabled?
	static bool sUDPProxyEnabled;

	// UDP proxy address and port
	LLHost mUDPProxy;
	// TCP proxy control channel address and port
	LLHost mTCPProxy;

	// socket handle to proxy TCP control channel
	LLSocket::ptr_t mProxyControlChannel;

	// APR pool for the socket
	apr_pool_t* mPool;

	/*###########################################################################################
	END OF UNSHARED MEMBERS
	###########################################################################################*/

	/*###########################################################################################
	MEMBERS WRITTEN IN MAIN THREAD AND READ IN ANY THREAD. ONLY READ OR WRITE AFTER LOCKING mProxyMutex!
	###########################################################################################*/

	// HTTP proxy address and port
	LLHost mHTTPProxy;

	// Currently selected HTTP proxy type. Can be web or socks.
	LLHttpProxyType mProxyType;

	// SOCKS 5 selected authentication method.
	LLSocks5AuthType mAuthMethodSelected;

	// SOCKS 5 username
	std::string mSocksUsername;
	// SOCKS 5 password
	std::string mSocksPassword;

	/*###########################################################################################
	END OF SHARED MEMBERS
	###########################################################################################*/
};

#endif
