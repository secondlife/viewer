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

#ifndef LL_SOCKS5_H
#define LL_SOCKS5_H

#include "llhost.h"
#include "llmemory.h"
#include "llsingleton.h"
#include <string>

// Error codes returned from the StartProxy method

#define SOCKS_OK 0
#define SOCKS_CONNECT_ERROR -1
#define SOCKS_NOT_PERMITTED -2
#define SOCKS_NOT_ACCEPTABLE -3
#define SOCKS_AUTH_FAIL -4
#define SOCKS_UDP_FWD_NOT_GRANTED -5
#define SOCKS_HOST_CONNECT_FAILED -6

#ifndef MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN (255 + 1) /* socks5: 255, +1 for len. */
#endif

#define SOCKS_VERSION 0x05 // we are using socks 5

// socks 5 address/hostname types
#define ADDRESS_IPV4     0x01
#define ADDRESS_HOSTNAME 0x03
#define ADDRESS_IPV6     0x04

// Lets just use our own ipv4 struct rather than dragging in system
// specific headers
union ipv4_address_t {
    unsigned char octects[4];
    U32 addr32;
};

// Socks 5 control channel commands
#define COMMAND_TCP_STREAM    0x01
#define COMMAND_TCP_BIND      0x02
#define COMMAND_UDP_ASSOCIATE 0x03

// Socks 5 command replys
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

// The standard socks5 request packet
// Push current alignment to stack and set alignment to 1 byte boundary
// This enabled us to use structs directly to set up and receive network packets
// into the correct fields, without fear of boundary alignment causing issues
#pragma pack(push,1)

// Socks5 command packet
struct socks_command_request_t {
    unsigned char version;
    unsigned char command;
    unsigned char flag;
    unsigned char atype;
    U32           address;
    U16           port;
};

// Standard socks5 reply packet
struct socks_command_response_t {
    unsigned char version;
    unsigned char reply;
    unsigned char flag;
    unsigned char atype;
    unsigned char add_bytes[4];
    U16           port;
};

#define AUTH_NOT_ACCEPTABLE 0xFF // reply if prefered methods are not avaiable
#define AUTH_SUCCESS        0x00 // reply if authentication successfull

// socks 5 authentication request, stating which methods the client supports
struct socks_auth_request_t {
    unsigned char version;
    unsigned char num_methods;
    unsigned char methods; // We are only using a single method currently
};

// socks 5 authentication response packet, stating server prefered method
struct socks_auth_response_t {
    unsigned char version;
    unsigned char method;
};

// socks 5 password reply packet
struct authmethod_password_reply_t {
    unsigned char version;
    unsigned char status;
};

// socks 5 UDP packet header
struct proxywrap_t {
    U16 rsv;
    U8  frag;
    U8  atype;
    U32 addr;
    U16 port;
};

#pragma pack(pop) /* restore original alignment from stack */


// Currently selected http proxy type
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

class LLSocks: public LLSingleton<LLSocks>
{
public:
    LLSocks();

    // Start a connection to the socks 5 proxy
    int startProxy(std::string host,U32 port);
    int startProxy(LLHost proxy,U32 messagePort);

    // Disconnect and clean up any connection to the socks 5 proxy
    void stopProxy();

    // Set up to use Password auth when connecting to the socks proxy
    void setAuthPassword(std::string username,std::string password);

    // Set up to use No Auth when connecting to the socks proxy;
    void setAuthNone();

    // get the currently selected auth method
    LLSocks5AuthType getSelectedAuthMethod() { return mAuthMethodSelected; };

    // static check for enabled status for UDP packets
    static bool isEnabled(){return sUdpProxyEnabled;};

    // static check for enabled status for http packets
    static bool isHttpProxyEnabled(){return sHttpProxyEnabled;};

    // Proxy http packets via httpHost, which can be a Socks5 or a http proxy
    // as specified in type
    void EnableHttpProxy(LLHost httpHost,LLHttpProxyType type);

    // Stop proxying http packets
    void DisableHttpProxy() {sHttpProxyEnabled = false;};

    // get the UDP proxy address and port
    LLHost getUDPProxy(){return mUDPProxy;};

    // get the socks 5 TCP control channel address and port
    LLHost getTCPProxy(){return mTCPProxy;};

    //get the http proxy address and port
    LLHost getHTTPProxy(){return mHTTPProxy;};

    // get the currently selected http proxy type
    LLHttpProxyType getHttpProxyType(){return mProxyType;};

    //Get the username password in a curl compatible format
    std::string getProxyUserPwd(){ return (mSocksUsername + ":" + mSocksPassword);};

private:

    // Open a communication channel to the socks5 proxy proxy, at port messagePort
    int proxyHandshake(LLHost proxy,U32 messagePort);

    // socket handle to proxy tcp control channel
    S32 hProxyControlChannel;

    // is the UDP proxy enabled
    static bool sUdpProxyEnabled;
    // is the http proxy enabled
    static bool sHttpProxyEnabled;

    // currently selected http proxy type
    LLHttpProxyType mProxyType;

    // UDP proxy address and port
    LLHost mUDPProxy;
    // TCP Proxy control channel address and port
    LLHost mTCPProxy;
    // HTTP proxy address and port
    LLHost mHTTPProxy;

    // socks 5 auth method selected
    LLSocks5AuthType mAuthMethodSelected;

    // socks 5 username
    std::string mSocksUsername;
    // socks 5 password
    std::string mSocksPassword;
};

#endif
