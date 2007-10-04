/** 
 * @file llstreamtools.h
 * @brief some helper functions for parsing legacy simstate and asset files.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
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

#ifndef LL_STREAM_TOOLS_H
#define LL_STREAM_TOOLS_H

#include <iostream>
#include <string>

// unless specifed otherwise these all return input_stream.good()

// skips spaces and tabs
bool skip_whitespace(std::istream& input_stream);

// skips whitespace and newlines
bool skip_emptyspace(std::istream& input_stream);

// skips emptyspace and lines that start with a #
bool skip_comments_and_emptyspace(std::istream& input_stream);

// skips to character after next newline
bool skip_line(std::istream& input_stream);

// skips to beginning of next non-emptyspace
bool skip_to_next_word(std::istream& input_stream);

// skips to character after the end of next keyword 
// a 'keyword' is defined as the first word on a line
bool skip_to_end_of_next_keyword(const char* keyword, std::istream& input_stream);

// skip_to_start_of_next_keyword() is disabled -- might tickle corruption bug 
// in windows iostream
// skips to beginning of next keyword
// a 'keyword' is defined as the first word on a line
//bool skip_to_start_of_next_keyword(const char* keyword, std::istream& input_stream);

// characters are pulled out of input_stream and appended to output_string
// returns result of input_stream.good() after characters are pulled
bool get_word(std::string& output_string, std::istream& input_stream);
bool get_line(std::string& output_string, std::istream& input_stream);

// characters are pulled out of input_stream (up to a max of 'n')
// and appended to output_string 
// returns result of input_stream.good() after characters are pulled
bool get_word(std::string& output_string, std::istream& input_stream, int n);
bool get_line(std::string& output_string, std::istream& input_stream, int n);

// unget_line() is disabled -- might tickle corruption bug in windows iostream
//// backs up the input_stream by line_size + 1 characters
//bool unget_line(const std::string& line, std::istream& input_stream);

// TODO -- move these string manipulator functions to a different file

// removes the last char in 'line' if it matches 'c'
// returns true if removed last char
bool remove_last_char(char c, std::string& line);

// replaces escaped characters with the correct characters from left to right
// "\\" ---> '\\' 
// "\n" ---> '\n' 
void unescape_string(std::string& line);

// replaces unescaped characters with expanded equivalents from left to right
// '\\' ---> "\\" 
// '\n' ---> "\n" 
void escape_string(std::string& line);

// replaces each '\n' character with ' '
void replace_newlines_with_whitespace(std::string& line);

// returns 1 for solitary "{"
// returns -1 for solitary "}"
// otherwise returns 0
int get_brace_count(const std::string& line);

// erases any double-quote characters in line
void remove_double_quotes(std::string& line);

// the 'keyword' is defined as the first word on a line
// the 'value' is everything after the keyword on the same line
// starting at the first non-whitespace and ending right before the newline
void get_keyword_and_value(std::string& keyword, 
						   std::string& value, 
						   const std::string& line);

// continue to read from the stream until you really can't
// read anymore or until we hit the count.  Some istream
// implimentations have a max that they will read.
std::istream& fullread(std::istream& str, char *buf, std::streamsize requested);

std::istream& operator>>(std::istream& str, const char *tocheck);

#endif


