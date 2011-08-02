/**
 * @file llproxy.cpp
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

#include "linden_common.h"

#include "llproxy.h"

#include <string>
#include <curl/curl.h>

#include "llapr.h"
#include "llcurl.h"
#include "llhost.h"
#include "message.h"
#include "net.h"

// Static class variable instances

// We want this to be static to avoid excessive indirection on every
// incoming packet just to do a simple bool test. The getter for this
// member is also static
bool LLProxy::sUDPProxyEnabled = false;

// Some helpful TCP functions
static LLSocket::ptr_t tcp_open_channel(apr_pool_t* pool, LLHost host); // Open a TCP channel to a given host
static void tcp_close_channel(LLSocket::ptr_t* handle_ptr); // Close an open TCP channel
static S32 tcp_handshake(LLSocket::ptr_t handle, char * dataout, apr_size_t outlen, char * datain, apr_size_t maxinlen); // Do a TCP data handshake

LLProxy::LLProxy():
		mHTTPProxyEnabled(false),
		mProxyMutex(0),
		mUDPProxy(),
		mTCPProxy(),
		mPool(gAPRPoolp),
		mHTTPProxy(),
		mProxyType(LLPROXY_SOCKS),
		mAuthMethodSelected(METHOD_NOAUTH),
		mSocksUsername(),
		mSocksPassword()
{
}

LLProxy::~LLProxy()
{
	stopSOCKSProxy();
	sUDPProxyEnabled  = false;
	mHTTPProxyEnabled = false;
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
	socks_auth_request.methods     = getSelectedAuthMethod(); // Send only the selected method.

	result = tcp_handshake(mProxyControlChannel, (char*)&socks_auth_request, sizeof(socks_auth_request), (char*)&socks_auth_response, sizeof(socks_auth_response));
	if (result != 0)
	{
		LL_WARNS("Proxy") << "SOCKS authentication request failed, error on TCP control channel : " << result << LL_ENDL;
		stopSOCKSProxy();
		return SOCKS_CONNECT_ERROR;
	}

	if (socks_auth_response.method == AUTH_NOT_ACCEPTABLE)
	{
		LL_WARNS("Proxy") << "SOCKS 5 server refused all our authentication methods" << LL_ENDL;
		stopSOCKSProxy();
		return SOCKS_NOT_ACCEPTABLE;
	}

	// SOCKS 5 USERNAME/PASSWORD authentication
	if (socks_auth_response.method == METHOD_PASSWORD)
	{
		// The server has requested a username/password combination
		std::string socks_username(getSocksUser());
		std::string socks_password(getSocksPwd());
		U32 request_size = socks_username.size() + socks_password.size() + 3;
		char * password_auth = new char[request_size];
		password_auth[0] = 0x01;
		password_auth[1] = socks_username.size();
		memcpy(&password_auth[2], socks_username.c_str(), socks_username.size());
		password_auth[socks_username.size() + 2] = socks_password.size();
		memcpy(&password_auth[socks_username.size()+3], socks_password.c_str(), socks_password.size());

		authmethod_password_reply_t password_reply;

		result = tcp_handshake(mProxyControlChannel, password_auth, request_size, (char*)&password_reply, sizeof(password_reply));
		delete[] password_auth;

		if (result != 0)
		{
			LL_WARNS("Proxy") << "SOCKS authentication failed, error on TCP control channel : " << result << LL_ENDL;
			stopSOCKSProxy();
			return SOCKS_CONNECT_ERROR;
		}

		if (password_reply.status != AUTH_SUCCESS)
		{
			LL_WARNS("Proxy") << "SOCKS authentication failed" << LL_ENDL;
			stopSOCKSProxy();
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

	result = tcp_handshake(mProxyControlChannel, (char*)&connect_request, sizeof(connect_request), (char*)&connect_reply, sizeof(connect_reply));
	if (result != 0)
	{
		LL_WARNS("Proxy") << "SOCKS connect request failed, error on TCP control channel : " << result << LL_ENDL;
		stopSOCKSProxy();
		return SOCKS_CONNECT_ERROR;
	}

	if (connect_reply.reply != REPLY_REQUEST_GRANTED)
	{
		LL_WARNS("Proxy") << "Connection to SOCKS 5 server failed, UDP forward request not granted" << LL_ENDL;
		stopSOCKSProxy();
		return SOCKS_UDP_FWD_NOT_GRANTED;
	}

	mUDPProxy.setPort(ntohs(connect_reply.port)); // reply port is in network byte order
	mUDPProxy.setAddress(proxy.getAddress());
	// The connection was successful. We now have the UDP port to send requests that need forwarding to.
	LL_INFOS("Proxy") << "SOCKS 5 UDP proxy connected on " << mUDPProxy << LL_ENDL;
	return SOCKS_OK;
}

S32 LLProxy::startSOCKSProxy(LLHost host)
{
	S32 status = SOCKS_OK;

	if (host.isOk())
	{
		mTCPProxy = host;
	}
	else
	{
		status = SOCKS_INVALID_HOST;
	}

	if (mProxyControlChannel && status == SOCKS_OK)
	{
		tcp_close_channel(&mProxyControlChannel);
	}

	if (status == SOCKS_OK)
	{
		mProxyControlChannel = tcp_open_channel(mPool, mTCPProxy);
		if (!mProxyControlChannel)
		{
			status = SOCKS_HOST_CONNECT_FAILED;
		}
	}

	if (status == SOCKS_OK)
	{
		status = proxyHandshake(mTCPProxy, (U32)gMessageSystem->mPort);
	}
	if (status == SOCKS_OK)
	{
		sUDPProxyEnabled = true;
	}
	else
	{
		stopSOCKSProxy();
	}
	return status;
}

void LLProxy::stopSOCKSProxy()
{
	sUDPProxyEnabled = false;

	// If the SOCKS proxy is requested to stop and we are using that for HTTP as well
	// then we must shut down any HTTP proxy operations. But it is allowable if web
	// proxy is being used to continue proxying HTTP.

	if (LLPROXY_SOCKS == getHTTPProxyType())
	{
		void disableHTTPProxy();
	}

	if (mProxyControlChannel)
	{
		tcp_close_channel(&mProxyControlChannel);
	}
}

void LLProxy::setAuthNone()
{
	LLMutexLock lock(&mProxyMutex);

	mAuthMethodSelected = METHOD_NOAUTH;
}

bool LLProxy::setAuthPassword(const std::string &username, const std::string &password)
{
	if (username.length() > SOCKSMAXUSERNAMELEN || password.length() > SOCKSMAXPASSWORDLEN ||
			username.length() < SOCKSMINUSERNAMELEN || password.length() < SOCKSMINPASSWORDLEN)
	{
		LL_WARNS("Proxy") << "Invalid SOCKS 5 password or username length." << LL_ENDL;
		return false;
	}

	LLMutexLock lock(&mProxyMutex);

	mAuthMethodSelected = METHOD_PASSWORD;
	mSocksUsername      = username;
	mSocksPassword      = password;

	return true;
}

bool LLProxy::enableHTTPProxy(LLHost httpHost, LLHttpProxyType type)
{
	if (!httpHost.isOk())
	{
		LL_WARNS("Proxy") << "Invalid SOCKS 5 Server" << LL_ENDL;
		return false;
	}

	LLMutexLock lock(&mProxyMutex);

	mHTTPProxy        = httpHost;
	mProxyType        = type;

	mHTTPProxyEnabled = true;

	return true;
}

bool LLProxy::enableHTTPProxy()
{
	LLMutexLock lock(&mProxyMutex);

	mHTTPProxyEnabled = true;

	return true;
}

void LLProxy::disableHTTPProxy()
{
	LLMutexLock lock(&mProxyMutex);

	mHTTPProxyEnabled = false;
}

// Get the HTTP proxy address and port
LLHost LLProxy::getHTTPProxy() const
{
	LLMutexLock lock(&mProxyMutex);
	return mHTTPProxy;
}

// Get the currently selected HTTP proxy type
LLHttpProxyType LLProxy::getHTTPProxyType() const
{
	LLMutexLock lock(&mProxyMutex);
	return mProxyType;
}

std::string LLProxy::getSocksPwd() const
{
	LLMutexLock lock(&mProxyMutex);
	return mSocksPassword;
}

std::string LLProxy::getSocksUser() const
{
	LLMutexLock lock(&mProxyMutex);
	return mSocksUsername;
}

// get the currently selected auth method
LLSocks5AuthType LLProxy::getSelectedAuthMethod() const
{
	LLMutexLock lock(&mProxyMutex);
	return mAuthMethodSelected;
}

//static
void LLProxy::cleanupClass()
{
	getInstance()->stopSOCKSProxy();
	deleteSingleton();
}

// Apply proxy settings to CuRL request if either type of HTTP proxy is enabled.
void LLProxy::applyProxySettings(LLCurlEasyRequest* handle)
{
	applyProxySettings(handle->getEasy());
}


void LLProxy::applyProxySettings(LLCurl::Easy* handle)
{
	applyProxySettings(handle->getCurlHandle());
}

void LLProxy::applyProxySettings(CURL* handle)
{
	// Do a faster unlocked check to see if we are supposed to proxy.
	if (mHTTPProxyEnabled)
	{
		// We think we should proxy, lock the proxy mutex.
		LLMutexLock lock(&mProxyMutex);
		// Now test again to verify that the proxy wasn't disabled between the first check and the lock.
		if (mHTTPProxyEnabled)
		{
			check_curl_code(curl_easy_setopt(handle, CURLOPT_PROXY, mHTTPProxy.getIPString().c_str()));
			check_curl_code(curl_easy_setopt(handle, CURLOPT_PROXYPORT, mHTTPProxy.getPort()));

			if (mProxyType == LLPROXY_SOCKS)
			{
				check_curl_code(curl_easy_setopt(handle, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5));
				if (mAuthMethodSelected == METHOD_PASSWORD)
				{
					std::string auth_string = mSocksUsername + ":" + mSocksPassword;
					check_curl_code(curl_easy_setopt(handle, CURLOPT_PROXYUSERPWD, auth_string.c_str()));
				}
			}
			else
			{
				check_curl_code(curl_easy_setopt(handle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP));
			}
		}
	}
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
		LL_WARNS("Proxy") << "Error sending data to proxy control channel" << LL_ENDL;
		ll_apr_warn_status(rv);
	}
	else if (expected_len != outlen)
	{
		LL_WARNS("Proxy") << "Error sending data to proxy control channel" << LL_ENDL;
		rv = -1;
	}

	if (APR_SUCCESS == rv)
	{
		expected_len = maxinlen;
		rv = apr_socket_recv(apr_socket, datain, &maxinlen);
		if (rv != APR_SUCCESS)
		{
			LL_WARNS("Proxy") << "Error receiving data from proxy control channel, status: " << rv << LL_ENDL;
			ll_apr_warn_status(rv);
		}
		else if (expected_len != maxinlen)
		{
			LL_WARNS("Proxy") << "Received incorrect amount of data in proxy control channel" << LL_ENDL;
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
	LL_DEBUGS("Proxy") << "Resetting proxy LLSocket handle, use_count == " << handle_ptr->use_count() << LL_ENDL;
	handle_ptr->reset();
}
