/** 
 * @file llmail.cpp
 * @brief smtp helper functions.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llmail.h"

// APR on Windows needs full windows headers
#ifdef LL_WINDOWS
#	undef WIN32_LEAN_AND_MEAN
#	include <winsock2.h>
#	include <windows.h>
#endif

#include <string>
#include <sstream>

#include "apr_pools.h"
#include "apr_network_io.h"

#include "llapr.h"
#include "llbase32.h"	// IM-to-email address
#include "llblowfishcipher.h"
#include "llerror.h"
#include "llhost.h"
#include "llsd.h"
#include "llstring.h"
#include "lluuid.h"
#include "net.h"

//
// constants
//
const size_t LL_MAX_KNOWN_GOOD_MAIL_SIZE = 4096;

static bool gMailEnabled = true;
static apr_pool_t* gMailPool;
static apr_sockaddr_t* gSockAddr;
static apr_socket_t* gMailSocket;

bool connect_smtp();
void disconnect_smtp();
 
//#if LL_WINDOWS
//SOCKADDR_IN gMailDstAddr, gMailSrcAddr, gMailLclAddr;
//#else
//struct sockaddr_in gMailDstAddr, gMailSrcAddr, gMailLclAddr;
//#endif

// Define this for a super-spammy mail mode.
//#define LL_LOG_ENTIRE_MAIL_MESSAGE_ON_SEND 1

bool connect_smtp()
{
	// Prepare an soket to talk smtp
	apr_status_t status;
	status = apr_socket_create(
		&gMailSocket,
		gSockAddr->sa.sin.sin_family,
		SOCK_STREAM,
		APR_PROTO_TCP,
		gMailPool);
	if(ll_apr_warn_status(status)) return false;
	status = apr_socket_connect(gMailSocket, gSockAddr);
	if(ll_apr_warn_status(status))
	{
		status = apr_socket_close(gMailSocket);
		ll_apr_warn_status(status);
		return false;
	}
	return true;
}

void disconnect_smtp()
{
	if(gMailSocket)
	{
		apr_status_t status = apr_socket_close(gMailSocket);
		ll_apr_warn_status(status);
		gMailSocket = NULL;
	}
}

// Returns TRUE on success.
// message should NOT be SMTP escaped.
// static
BOOL LLMail::send(
	const char* from_name,
	const char* from_address,
	const char* to_name,
	const char* to_address,
	const char* subject,
	const char* message,
	const LLSD& headers)
{
	std::string header = buildSMTPTransaction(
		from_name,
		from_address,
		to_name,
		to_address,
		subject,
		headers);
	if(header.empty())
	{
		return FALSE;
	}

	std::string message_str;
	if(message)
	{
		message_str = message;
	}
	bool rv = send(header, message_str, to_address, from_address);
	if(rv) return TRUE;
	return FALSE;
}

// static
void LLMail::init(const std::string& hostname, apr_pool_t* pool)
{
	gMailSocket = NULL;
	if(hostname.empty() || !pool)
	{
		gMailPool = NULL;
		gSockAddr = NULL;
	}
	else
	{
		gMailPool = pool;

		// collect all the information into a socaddr sturcture. the
		// documentation is a bit unclear, but I either have to
		// specify APR_UNSPEC or not specify any flags. I am not sure
		// which option is better.
		apr_status_t status = apr_sockaddr_info_get(
			&gSockAddr,
			hostname.c_str(),
			APR_UNSPEC,
			25,
			APR_IPV4_ADDR_OK,
			gMailPool);
		ll_apr_warn_status(status);
	}
}

// static
void LLMail::enable(bool mail_enabled)
{
	gMailEnabled = mail_enabled;
}

// Test a subject line for RFC2822 compliance.
static bool valid_subject_chars(const char *subject)
{
	for (; *subject != '\0'; subject++)
	{
		unsigned char c = *subject;

		if (c == '\xa' || c == '\xd' || c > '\x7f')
		{
			return false;
		}
	}

	return true;
}

// static
std::string LLMail::buildSMTPTransaction(
	const char* from_name,
	const char* from_address,
	const char* to_name,
	const char* to_address,
	const char* subject,
	const LLSD& headers)
{
	if(!from_address || !to_address)
	{
		llinfos << "send_mail build_smtp_transaction reject: missing to and/or"
			<< " from address." << llendl;
		return std::string();
	}
	if(!valid_subject_chars(subject))
	{
		llinfos << "send_mail build_smtp_transaction reject: bad subject header: "
			<< "to=<" << to_address
			<< ">, from=<" << from_address << ">"
			<< llendl;
		return std::string();
	}
	std::ostringstream from_fmt;
	if(from_name && from_name[0])
	{
		// "My Name" <myaddress@example.com>
		from_fmt << "\"" << from_name << "\" <" << from_address << ">";
	}
	else
	{
		// <myaddress@example.com>
		from_fmt << "<" << from_address << ">";
	}
	std::ostringstream to_fmt;
	if(to_name && to_name[0])
	{
		to_fmt << "\"" << to_name << "\" <" << to_address << ">";
	}
	else
	{
		to_fmt << "<" << to_address << ">";
	}
	std::ostringstream header;
	header
		<< "HELO lindenlab.com\r\n"
		<< "MAIL FROM:<" << from_address << ">\r\n"
		<< "RCPT TO:<" << to_address << ">\r\n"
		<< "DATA\r\n"
		<< "From: " << from_fmt.str() << "\r\n"
		<< "To: " << to_fmt.str() << "\r\n"
		<< "Subject: " << subject << "\r\n";
	
	if(headers.isMap())
	{
		LLSD::map_const_iterator iter = headers.beginMap();
		LLSD::map_const_iterator end = headers.endMap();
		for(; iter != end; ++iter)
		{
			header << (*iter).first << ": " << ((*iter).second).asString()
				<< "\r\n";
		}
	}

	header << "\r\n";
	return header.str();
}

// static
bool LLMail::send(
	const std::string& header,
	const std::string& raw_message,
	const char* from_address,
	const char* to_address)
{
	if(!from_address || !to_address)
	{
		llinfos << "send_mail reject: missing to and/or from address."
			<< llendl;
		return false;
	}

	// remove any "." SMTP commands to prevent injection (DEV-35777)
	// we don't need to worry about "\r\n.\r\n" because of the 
	// "\n" --> "\n\n" conversion going into rfc2822_msg below
	std::string message = raw_message;
	std::string bad_string = "\n.\n";
	std::string good_string = "\n..\n";
	while (1)
	{
		int index = message.find(bad_string);
		if (index == std::string::npos) break;
		message.replace(index, bad_string.size(), good_string);
	}

	// convert all "\n" into "\r\n"
	std::ostringstream rfc2822_msg;
	for(U32 i = 0; i < message.size(); ++i)
	{
		switch(message[i])
		{
		case '\0':
			break;
		case '\n':
			// *NOTE: this is kinda busted if we're fed \r\n
			rfc2822_msg << "\r\n";
			break;
		default:
			rfc2822_msg << message[i];
			break;
		}
	}

	if(!gMailEnabled)
	{
		llinfos << "send_mail reject: mail system is disabled: to=<"
			<< to_address << ">, from=<" << from_address
			<< ">" << llendl;
		// Any future interface to SMTP should return this as an
		// error.  --mark
		return true;
	}
	if(!gSockAddr)
	{
		llwarns << "send_mail reject: mail system not initialized: to=<"
			<< to_address << ">, from=<" << from_address
			<< ">" << llendl;
		return false;
	}

	if(!connect_smtp())
	{
		llwarns << "send_mail reject: SMTP connect failure: to=<"
			<< to_address << ">, from=<" << from_address
			<< ">" << llendl;
		return false;
	}

	std::ostringstream smtp_fmt;
	smtp_fmt << header << rfc2822_msg.str() << "\r\n" << ".\r\n" << "QUIT\r\n";
	std::string smtp_transaction = smtp_fmt.str();
	size_t original_size = smtp_transaction.size();
	apr_size_t send_size = original_size;
	apr_status_t status = apr_socket_send(
		gMailSocket,
		smtp_transaction.c_str(),
		(apr_size_t*)&send_size);
	disconnect_smtp();
	if(ll_apr_warn_status(status))
	{
		llwarns << "send_mail socket failure: unable to write "
			<< "to=<" << to_address
			<< ">, from=<" << from_address << ">"
			<< ", bytes=" << original_size
			<< ", sent=" << send_size << llendl;
		return false;
	}
	if(send_size >= LL_MAX_KNOWN_GOOD_MAIL_SIZE)
	{
		llwarns << "send_mail message has been shown to fail in testing "
			<< "when sending messages larger than " << LL_MAX_KNOWN_GOOD_MAIL_SIZE
			<< " bytes. The next log about success is potentially a lie." << llendl;
	}
	lldebugs << "send_mail success: "
		<< "to=<" << to_address
		<< ">, from=<" << from_address << ">"
		<< ", bytes=" << original_size
		<< ", sent=" << send_size << llendl;

#if LL_LOG_ENTIRE_MAIL_MESSAGE_ON_SEND
	llinfos << rfc2822_msg.str() << llendl;
#endif
	return true;
}


// static
std::string LLMail::encryptIMEmailAddress(const LLUUID& from_agent_id,
											const LLUUID& to_agent_id,
											U32 time,
											const U8* secret,
											size_t secret_size)
{
#if LL_WINDOWS
	return "blowfish-not-supported-on-windows";
#else
	size_t data_size = 4 + UUID_BYTES + UUID_BYTES;
	// Convert input data into a binary blob
	std::vector<U8> data;
	data.resize(data_size);
	// *NOTE: This may suffer from endian issues.  Could be htonmemcpy.
	memcpy(&data[0], &time, 4);
	memcpy(&data[4], &from_agent_id.mData[0], UUID_BYTES);
	memcpy(&data[4 + UUID_BYTES], &to_agent_id.mData[0], UUID_BYTES);
	
	// Encrypt the blob
	LLBlowfishCipher cipher(secret, secret_size);
	size_t encrypted_size = cipher.requiredEncryptionSpace(data.size());
	U8* encrypted = new U8[encrypted_size];
	cipher.encrypt(&data[0], data_size, encrypted, encrypted_size);

	std::string address = LLBase32::encode(encrypted, encrypted_size);

	// Make it more pretty for humans.
	LLStringUtil::toLower(address);

	delete [] encrypted;

	return address;
#endif
}
