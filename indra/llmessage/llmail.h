/** 
 * @file llmail.h
 * @brief smtp helper functions.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLMAIL_H
#define LL_LLMAIL_H

typedef struct apr_pool_t apr_pool_t;

class LLUUID;

class LLMail
{
public:
	// if hostname is NULL, then the host is resolved as 'mail'
	static void init(const std::string& hostname, apr_pool_t* pool);

	// Allow all email transmission to be disabled/enabled.
	static void enable(bool mail_enabled);

	// returns TRUE if the call succeeds, FALSE otherwise.
	//
	// Results in:
	// From: "from_name" <from_address>
	// To:   "to_name" <to_address>
	// Subject: subject
	// message
	static BOOL send(const char* from_name, const char* from_address,
				const char* to_name, const char* to_address,
				const char* subject, const char* message);

	/**
	* @brief build the complete smtp transaction & header for use in an
	* mail.
	*
	* @param from_name The name of the email sender
	* @param from_address The email address for the sender
	* @param to_name The name of the email recipient
	* @param to_name The email recipient address
	* @param subject The subject of the email
	* @return Returns the complete SMTP transaction mail header.
	*/
	static std::string buildSMTPTransaction(
		const char* from_name,
		const char* from_address,
		const char* to_name,
		const char* to_address,
		const char* subject);

	/**
	* @brief send an email with header and body.
	*
	* @param header The email header. Use build_mail_header().
	* @param message The unescaped email message.
	* @param from_address Used for debugging
	* @param to_address Used for debugging
	* @return Returns true if the message could be sent.
	*/
	static bool send(
		const std::string& header,
		const std::string& message,
		const char* from_address,
		const char* to_address);

	// IM-to-email sessions use a "session id" based on an encrypted
	// combination of from agent_id, to agent_id, and timestamp.  When
	// a user replies to an email we use the from_id to determine the
	// sender's name and the to_id to route the message.  The address
	// is encrypted to prevent users from building addresses to spoof
	// IMs from other users.  The timestamps allow the "sessions" to 
	// expire, in case one of the sessions is stolen/hijacked.
	//
	// indra/tools/mailglue is responsible for parsing the inbound mail.
	//
	// secret: binary blob passed to blowfish, max length 56 bytes
	// secret_size: length of blob, in bytes
	//
	// Returns: "base64" encoded email local-part, with _ and - as the
	// non-alphanumeric characters.  This allows better compatibility
	// with email systems than the default / and + extra chars.  JC
	static std::string encryptIMEmailAddress(
		const LLUUID& from_agent_id,
		const LLUUID& to_agent_id,
		U32 time,
		const U8* secret,
		size_t secret_size);
};

extern const size_t LL_MAX_KNOWN_GOOD_MAIL_SIZE;

#endif
