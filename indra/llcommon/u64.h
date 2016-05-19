/** 
 * @file u64.h
 * @brief Utilities to deal with U64s.
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

#ifndef LL_U64_H
#define LL_U64_H

/**
 * @brief Forgivingly parse a null terminated character array.
 *
 * @param str The string to parse.
 * @return Returns the first U64 value found in the string or 0 on failure.
 */
LL_COMMON_API U64 str_to_U64(const std::string& str);

/**
 * @brief Given a U64 value, return a printable representation.
 * @param value The U64 to turn into a printable character array.
 * @return Returns the result string.
 */
LL_COMMON_API std::string U64_to_str(U64 value);

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
LL_COMMON_API char* U64_to_str(U64 value, char* result, S32 result_size);

/**
 * @brief Convert a U64 to the closest F64 value.
 */
LL_COMMON_API F64 U64_to_F64(const U64 value);

/**
 * @brief Helper function to wrap strtoull() which is not available on windows.
 */
LL_COMMON_API U64 llstrtou64(const char* str, char** end, S32 base);

#endif
