/**
 * @file llsocks5.cpp
 * @brief SOCKS 5 implementation
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

#include "linden_common.h"

#include "llproxy.h"

#include <string>

#include "llapr.h"
#include "llhost.h"
#include "message.h"
#include "net.h"

// Static class variable instances

// We want this to be static to avoid excessive indirection on every
// incoming packet just to do a simple bool test. The getter for this
// member is also static
bool LLProxy::sUDPProxyEnabled = false;
bool LLProxy::sHTTPProxyEnabled = false;

// Some helpful TCP functions
static LLSocket::ptr_t tcp_open_channel(apr_pool_t* pool, LLHost host); // Open a TCP channel to a given host
static void tcp_close_channel(LLSocket::ptr_t* handle_ptr); // Close an open TCP channel
static S32 tcp_handshake(LLSocket::ptr_t handle, char * dataout, apr_size_t outlen, char * datain, apr_size_t maxinlen); // Do a TCP data handshake

LLProxy::LLProxy():
		mProxyType(LLPROXY_SOCKS),
		mUDPProxy(),
		mTCPProxy(),
		mHTTPProxy(),
		mAuthMethodSelected(METHOD_NOAUTH),
		mSocksUsername(),
		mSocksPassword(),
		mPool(gAPRPoolp)
{
}

LLProxy::~LLProxy()
{
	stopProxy();
	sUDPProxyEnabled  = false;
	sHTTPProxyEnabled = false;
}

// Perform a SOCKS 5 authentication and UDP association to the proxy
// specified by proxy, and associate UDP port message_port
S32 LLProxy::proxyHandshake(LLHost proxy, U32 message_port)
{
	S32 result;

	/* SOCKS 5 Auth request */
	socks_auth_request_t  socks_auth_request;
	socks_auth_response_t socks_auth_response;

	socks_auth_request.version     = SOCKS_VERSION;       // SOCKS version 5
	socks_auth_request.num_methods = 1;                   // Sending 1 method.
	socks_auth_request.methods     = mAuthMethodSelected; // Send only the selected method.

	result = tcp_handshake(mProxyControlChannel, (char*)&socks_auth_request, sizeof(socks_auth_request), (char*)&socks_auth_response, sizeof(socks_auth_response));
	if (result != 0)
	{
		llwarns << "SOCKS authentication request failed, error on TCP control channel : " << result << llendl;
		stopProxy();
		return SOCKS_CONNECT_ERROR;
	}

	if (socks_auth_response.method == AUTH_NOT_ACCEPTABLE)
	{
		llwarns << "SOCKS 5 server refused all our authentication methods" << llendl;
		stopProxy();
		return SOCKS_NOT_ACCEPTABLE;
	}

	// SOCKS 5 USERNAME/PASSWORD authentication
	if (socks_auth_response.method == METHOD_PASSWORD)
	{
		// The server has requested a username/password combination
		U32 request_size = mSocksUsername.size() + mSocksPassword.size() + 3;
		char * password_auth = new char[request_size];
		password_auth[0] = 0x01;
		password_auth[1] = mSocksUsername.size();
		memcpy(&password_auth[2], mSocksUsername.c_str(), mSocksUsername.size());
		password_auth[mSocksUsername.size() + 2] = mSocksPassword.size();
		memcpy(&password_auth[mSocksUsername.size()+3], mSocksPassword.c_str(), mSocksPassword.size());

		authmethod_password_reply_t password_reply;

		result = tcp_handshake(mProxyControlChannel, password_auth, request_size, (char*)&password_reply, sizeof(authmethod_password_reply_t));
		delete[] password_auth;

		if (result != 0)
		{
			llwarns << "SOCKS authentication failed, error on TCP control channel : " << result << llendl;
			stopProxy();
			return SOCKS_CONNECT_ERROR;
		}

		if (password_reply.status != AUTH_SUCCESS)
		{
			llwarns << "SOCKS authentication failed" << llendl;
			stopProxy();
			return SOCKS_AUTH_FAIL;
		}
	}

	/* SOCKS5 connect request */

	socks_command_request_t  connect_request;
	socks_command_response_t connect_reply;

	connect_request.version		= SOCKS_VERSION;         // SOCKS V5
	connect_request.command		= COMMAND_UDP_ASSOCIATE; // Associate UDP
	connect_request.reserved	= FIELD_RESERVED;
	connect_request.atype		= ADDRESS_IPV4;
	connect_request.address		= htonl(0); // 0.0.0.0
	connect_request.port		= htons(0); // 0
	// "If the client is not in possession of the information at the time of the UDP ASSOCIATE,
	//  the client MUST use a port number and address of all zeros. RFC 1928"

	result = tcp_handshake(mProxyControlChannel, (char*)&connect_request, sizeof(socks_command_request_t), (char*)&connect_reply, sizeof(socks_command_response_t));
	if (result != 0)
	{
		llwarns << "SOCKS connect request failed, error on TCP control channel : " << result << llendl;
		stopProxy();
		return SOCKS_CONNECT_ERROR;
	}

	if (connect_reply.reply != REPLY_REQUEST_GRANTED)
	{
		llwarns << "Connection to SOCKS 5 server failed, UDP forward request not granted" << llendl;
		stopProxy();
		return SOCKS_UDP_FWD_NOT_GRANTED;
	}

	mUDPProxy.setPort(ntohs(connect_reply.port)); // reply port is in network byte order
	mUDPProxy.setAddress(proxy.getAddress());
	// The connection was successful. We now have the UDP port to send requests that need forwarding to.
	llinfos << "SOCKS 5 UDP proxy connected on " << mUDPProxy << llendl;
	return SOCKS_OK;
}

S32 LLProxy::startProxy(std::string host, U32 port)
{
	mTCPProxy.setHostByName(host);
	mTCPProxy.setPort(port);

	S32 status;

	if (mProxyControlChannel)
	{
		tcp_close_channel(&mProxyControlChannel);
	}

	mProxyControlChannel = tcp_open_channel(mPool, mTCPProxy);
	if (!mProxyControlChannel)
	{
		return SOCKS_HOST_CONNECT_FAILED;
	}
	status = proxyHandshake(mTCPProxy, (U32)gMessageSystem->mPort);
	if (status == SOCKS_OK)
	{
		sUDPProxyEnabled = true;
	}
	else
	{
		stopProxy();
	}
	return status;

}

void LLProxy::stopProxy()
{
	sUDPProxyEnabled = false;

	// If the SOCKS proxy is requested to stop and we are using that for HTTP as well
	// then we must shut down any HTTP proxy operations. But it is allowable if web
	// proxy is being used to continue proxying HTTP.

	if(LLPROXY_SOCKS == mProxyType)
	{
		sHTTPProxyEnabled = false;
	}

	if (mProxyControlChannel)
	{
		tcp_close_channel(&mProxyControlChannel);
	}
}

void LLProxy::setAuthNone()
{
	mAuthMethodSelected = METHOD_NOAUTH;
}

void LLProxy::setAuthPassword(const std::string &username, const std::string &password)
{
	mAuthMethodSelected = METHOD_PASSWORD;
	mSocksUsername      = username;
	mSocksPassword      = password;
}

void LLProxy::enableHTTPProxy(LLHost httpHost, LLHttpProxyType type)
{ 
	sHTTPProxyEnabled = true;
	mHTTPProxy        = httpHost;
	mProxyType        = type;
}

//static
void LLProxy::cleanupClass()
{
	LLProxy::getInstance()->stopProxy();
}

static S32 tcp_handshake(LLSocket::ptr_t handle, char * dataout, apr_size_t outlen, char * datain, apr_size_t maxinlen)
{

	apr_socket_t* apr_socket = handle->getSocket();
	apr_status_t rv = APR_SUCCESS;

	apr_size_t expected_len = outlen;

	handle->setBlocking(1000);

  	rv = apr_socket_send(apr_socket, dataout, &outlen);
	if (APR_SUCCESS != rv || expected_len != outlen)
	{
		llwarns << "Error sending data to proxy control channel" << llendl;
		ll_apr_warn_status(rv);
	}
	else if (expected_len != outlen)
	{
		llwarns << "Error sending data to proxy control channel" << llendl;
		rv = -1;
	}

	if (APR_SUCCESS == rv)
	{
		expected_len = maxinlen;
		rv = apr_socket_recv(apr_socket, datain, &maxinlen);
		if (rv != APR_SUCCESS)
		{
			llwarns << "Error receiving data from proxy control channel, status: " << rv << llendl;
			ll_apr_warn_status(rv);
		}
		else if (expected_len != maxinlen)
		{
			llwarns << "Received incorrect amount of data in proxy control channel" << llendl;
			rv = -1;
		}
	}

	handle->setNonBlocking();

	return rv;
}

static LLSocket::ptr_t tcp_open_channel(apr_pool_t* pool, LLHost host)
{
	LLSocket::ptr_t socket = LLSocket::create(gAPRPoolp, LLSocket::STREAM_TCP);
	bool connected = socket->blockingConnect(host);
	if (!connected)
	{
		tcp_close_channel(&socket);
	}

	return socket;
}

// Pass a pointer-to-pointer to avoid changing use_count().
static void tcp_close_channel(LLSocket::ptr_t* handle_ptr)
{
	lldebugs << "Resetting proxy LLSocket handle, use_count == " << handle_ptr->use_count() << llendl;
	handle_ptr->reset();
}
