/** 
 * @file u64.h
 * @brief Utilities to deal with U64s.
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

#ifndef LL_U64_H
#define LL_U64_H

/**
 * @brief Forgivingly parse a null terminated character array.
 *
 * @param str The string to parse.
 * @return Returns the first U64 value found in the string or 0 on failure.
 */
U64 str_to_U64(const std::string& str);

/**
 * @brief Given a U64 value, return a printable representation.
 * @param value The U64 to turn into a printable character array.
 * @return Returns the result string.
 */
std::string U64_to_str(U64 value);

/**
 * @brief Given a U64 value, return a printable representation.
 *
 * The client of this function is expected to provide an allocated
 * buffer. The function then snprintf() into that buffer, so providing
 * NULL has undefined behavior. Providing a buffer which is too small
 * will truncate the printable value, so usually you want to declare
 * the buffer:
 *
 *  char result[U64_BUF];
 *  std::cout << "value: " << U64_to_str(value, result, U64_BUF);
 *
 * @param value The U64 to turn into a printable character array.
 * @param result The buffer to use
 * @param result_size The size of the buffer allocated. Use U64_BUF.
 * @return Returns the result pointer.
 */
char* U64_to_str(U64 value, char* result, S32 result_size);

/**
 * @brief Convert a U64 to the closest F64 value.
 */
F64 U64_to_F64(const U64 value);

/**
 * @brief Helper function to wrap strtoull() which is not available on windows.
 */
U64 llstrtou64(const char* str, char** end, S32 base);

#endif
