/** 
 * @file llcipher.h
 * @brief Abstract base class for encryption ciphers.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LLCIPHER_H
#define LLCIPHER_H

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLCipher
//
// Abstract base class for a cipher object.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLCipher
{
public:
	virtual ~LLCipher() {}

	// encrypt src and place result into dst. returns TRUE if
	// Returns number of bytes written into dst, or 0 on error.
	virtual U32 encrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len) = 0;

	// decrypt src and place result into dst. 
	// Returns number of bytes written into dst, or 0 on error.
	virtual U32 decrypt(const U8* src, U32 src_len, U8* dst, U32 dst_len) = 0;

	// returns the minimum amount of space required to encrypt for a 
	// unencrypted source buffer of length len.
	// *NOTE: This is estimated space and you should check the return value
	// of the encrypt function.
	virtual U32 requiredEncryptionSpace(U32 src_len) const = 0 ;
};

#endif
