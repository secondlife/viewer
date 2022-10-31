/** 
 * @file llstreamtools.h
 * @brief some helper functions for parsing legacy simstate and asset files.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LL_STREAM_TOOLS_H
#define LL_STREAM_TOOLS_H

#include <iostream>
#include <string>

// unless specifed otherwise these all return input_stream.good()

// skips spaces and tabs
LL_COMMON_API bool skip_whitespace(std::istream& input_stream);

// skips whitespace and newlines
LL_COMMON_API bool skip_emptyspace(std::istream& input_stream);

// skips emptyspace and lines that start with a #
LL_COMMON_API bool skip_comments_and_emptyspace(std::istream& input_stream);

// skips to character after next newline
LL_COMMON_API bool skip_line(std::istream& input_stream);

// skips to beginning of next non-emptyspace
LL_COMMON_API bool skip_to_next_word(std::istream& input_stream);

// skips to character after the end of next keyword 
// a 'keyword' is defined as the first word on a line
LL_COMMON_API bool skip_to_end_of_next_keyword(const char* keyword, std::istream& input_stream);

// skip_to_start_of_next_keyword() is disabled -- might tickle corruption bug 
// in windows iostream
// skips to beginning of next keyword
// a 'keyword' is defined as the first word on a line
//bool skip_to_start_of_next_keyword(const char* keyword, std::istream& input_stream);

// characters are pulled out of input_stream and appended to output_string
// returns result of input_stream.good() after characters are pulled
LL_COMMON_API bool get_word(std::string& output_string, std::istream& input_stream);
LL_COMMON_API bool get_line(std::string& output_string, std::istream& input_stream);

// characters are pulled out of input_stream (up to a max of 'n')
// and appended to output_string 
// returns result of input_stream.good() after characters are pulled
LL_COMMON_API bool get_word(std::string& output_string, std::istream& input_stream, int n);
LL_COMMON_API bool get_line(std::string& output_string, std::istream& input_stream, int n);

// unget_line() is disabled -- might tickle corruption bug in windows iostream
//// backs up the input_stream by line_size + 1 characters
//bool unget_line(const std::string& line, std::istream& input_stream);

// TODO -- move these string manipulator functions to a different file

// removes the last char in 'line' if it matches 'c'
// returns true if removed last char
LL_COMMON_API bool remove_last_char(char c, std::string& line);

// replaces escaped characters with the correct characters from left to right
// "\\" ---> '\\' 
// "\n" ---> '\n' 
LL_COMMON_API void unescape_string(std::string& line);

// replaces unescaped characters with expanded equivalents from left to right
// '\\' ---> "\\" 
// '\n' ---> "\n" 
LL_COMMON_API void escape_string(std::string& line);

// replaces each '\n' character with ' '
LL_COMMON_API void replace_newlines_with_whitespace(std::string& line);

// erases any double-quote characters in line
LL_COMMON_API void remove_double_quotes(std::string& line);

// the 'keyword' is defined as the first word on a line
// the 'value' is everything after the keyword on the same line
// starting at the first non-whitespace and ending right before the newline
LL_COMMON_API void get_keyword_and_value(std::string& keyword, 
                           std::string& value, 
                           const std::string& line);

// continue to read from the stream until you really can't
// read anymore or until we hit the count.  Some istream
// implimentations have a max that they will read.
// Returns the number of bytes read.
LL_COMMON_API std::streamsize fullread(
    std::istream& istr,
    char* buf,
    std::streamsize requested);


LL_COMMON_API std::istream& operator>>(std::istream& str, const char *tocheck);

#endif


