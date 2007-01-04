/** 
 * @file llmail.h
 * @brief smtp helper functions.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLMAIL_H
#define LL_LLMAIL_H

typedef struct apr_pool_t apr_pool_t;

// if hostname is NULL, then the host is resolved as 'mail'
void init_mail(const std::string& hostname, apr_pool_t* pool);

// Allow all email transmission to be disabled/enabled.
void enable_mail(bool mail_enabled);

// returns TRUE if the call succeeds, FALSE otherwise.
//
// Results in:
// From: "from_name" <from_address>
// To:   "to_name" <to_address>
// Subject: subject
// message
BOOL send_mail(const char* from_name, const char* from_address,
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
std::string build_smtp_transaction(
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
bool send_mail(
	const std::string& header,
	const std::string& message,
	const char* from_address,
	const char* to_address);

extern const size_t LL_MAX_KNOWN_GOOD_MAIL_SIZE;

#endif
