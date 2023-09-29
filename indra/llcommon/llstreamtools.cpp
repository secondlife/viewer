/** 
 * @file llstreamtools.cpp
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

#include "linden_common.h"

#include <iostream>
#include <string>

#include "llstreamtools.h"


// ----------------------------------------------------------------------------
// some std::istream helper functions
// ----------------------------------------------------------------------------

// skips spaces and tabs
bool skip_whitespace(std::istream& input_stream)
{
	int c = input_stream.peek();
	while (('\t' == c || ' ' == c) && input_stream.good())
	{
		input_stream.get();
		c = input_stream.peek();
	}
	return input_stream.good();
}

// skips whitespace, newlines, and carriage returns
bool skip_emptyspace(std::istream& input_stream)
{
	int c = input_stream.peek();
	while ( input_stream.good()
			&& ('\t' == c || ' ' == c || '\n' == c || '\r' == c) )
	{
		input_stream.get();
		c = input_stream.peek();
	}
	return input_stream.good();
}

// skips emptyspace and lines that start with a #
bool skip_comments_and_emptyspace(std::istream& input_stream)
{
	while (skip_emptyspace(input_stream))
	{
		int c = input_stream.peek();
		if ('#' == c )
		{
			while ('\n' != c && input_stream.good())
			{
				c = input_stream.get();
			}
		}
		else
		{
			break;
		}
	}
	return input_stream.good();
}

bool skip_line(std::istream& input_stream)
{
	int c;
	do
	{
		c = input_stream.get();
	} while ('\n' != c  &&  input_stream.good());
	return input_stream.good();
}

bool skip_to_next_word(std::istream& input_stream)
{
	int c = input_stream.peek();
	while ( input_stream.good()
			&& (   (c >= 'a' && c <= 'z')
		   		|| (c >= 'A' && c <= 'Z')
				|| (c >= '0' && c <= '9')
				|| '_' == c ) )
	{
		input_stream.get();
		c = input_stream.peek();
	}
	while ( input_stream.good()
			&& !(   (c >= 'a' && c <= 'z')
		   		 || (c >= 'A' && c <= 'Z')
				 || (c >= '0' && c <= '9')
				 || '_' == c ) )
	{
		input_stream.get();
		c = input_stream.peek();
	}
	return input_stream.good();
}

bool skip_to_end_of_next_keyword(const char* keyword, std::istream& input_stream)
{
	auto key_length = strlen(keyword);	 /*Flawfinder: ignore*/
	if (0 == key_length)
	{
		return false;
	}
	while (input_stream.good())
	{
		skip_emptyspace(input_stream);
		int c = input_stream.get();
		if (keyword[0] != c)
		{
			skip_line(input_stream);
		}
		else
		{
			int key_index = 1;
			while ( key_index < key_length
					&&	keyword[key_index - 1] == c 
			   		&& input_stream.good())
			{
				key_index++;
				c = input_stream.get();
			} 

			if (key_index == key_length
				&& keyword[key_index-1] == c)
			{
				c = input_stream.peek();
				if (' ' == c || '\t' == c || '\r' == c || '\n' == c)
				{ 
					return true;
				}
				else
				{
					skip_line(input_stream);
				}
			}
			else
			{
				skip_line(input_stream);
			}
		}
	}
	return false;
}

/* skip_to_start_of_next_keyword() is disabled -- might tickle corruption bug in windows iostream
bool skip_to_start_of_next_keyword(const char* keyword, std::istream& input_stream)
{
	int key_length = strlen(keyword);
	if (0 == key_length)
	{
		return false;
	}
	while (input_stream.good())
	{
		skip_emptyspace(input_stream);
		int c = input_stream.get();
		if (keyword[0] != c)
		{
			skip_line(input_stream);
		}
		else
		{
			int key_index = 1;
			while ( key_index < key_length
					&&	keyword[key_index - 1] == c 
			   		&& input_stream.good())
			{
				key_index++;
				c = input_stream.get();
			} 

			if (key_index == key_length
				&& keyword[key_index-1] == c)
			{
				c = input_stream.peek();
				if (' ' == c || '\t' == c || '\r' == c || '\n' == c)
				{ 
					// put the keyword back onto the stream
					for (int index = key_length - 1; index >= 0; index--)
					{
						input_stream.putback(keyword[index]);
					}
					return true;
				}
				else
				{
					skip_line(input_stream);
					break;
				}
			}
			else
			{
				skip_line(input_stream);
			}
		}
	}
	return false;
}
*/

bool get_word(std::string& output_string, std::istream& input_stream)
{
	skip_emptyspace(input_stream);
	int c = input_stream.peek();
	while ( !isspace(c) 
			&& '\n' != c 
			&& '\r' != c 
			&& input_stream.good() )
	{
		output_string += c;
		input_stream.get();
		c = input_stream.peek();
	}
	return input_stream.good();
}

bool get_word(std::string& output_string, std::istream& input_stream, int n)
{
	skip_emptyspace(input_stream);
	int char_count = 0;
	int c = input_stream.peek();
	while (!isspace(c) 
			&& '\n' != c 
			&& '\r' != c 
			&& input_stream.good() 
			&& char_count < n)
	{
		char_count++;
		output_string += c;
		input_stream.get();
		c = input_stream.peek();
	}
	return input_stream.good();
}

// get everything up to and including the next newline
bool get_line(std::string& output_string, std::istream& input_stream)
{
	output_string.clear();
	int c = input_stream.get();
	while (input_stream.good())
	{
		output_string += c;
		if ('\n' == c)
		{
			break;
		}
		c = input_stream.get();
	} 
	return input_stream.good();
}

// get everything up to and including the next newline
// up to the next n characters.  
// add a newline on the end if bail before actual line ending
bool get_line(std::string& output_string, std::istream& input_stream, int n)
{
	output_string.clear();
	int char_count = 0;
	int c = input_stream.get();
	while (input_stream.good() && char_count < n)
	{
		char_count++;
		output_string += c;
		if ('\n' == c)
		{
			break;
		}
		if (char_count >= n)
		{
			output_string.append("\n");
			break;
		}
		c = input_stream.get();
	} 
	return input_stream.good();
}

/* disabled -- might tickle bug in windows iostream
// backs up the input_stream by line_size + 1 characters
bool unget_line(const std::string& line, std::istream& input_stream)
{
	input_stream.putback('\n');	// unget the newline
	for (int line_index = line.size()-1; line_index >= 0; line_index--)
	{ 
		input_stream.putback(line[line_index]);
	}
	return input_stream.good();
}
*/

// removes the last char in 'line' if it matches 'c'
// returns true if removed last char
bool remove_last_char(char c, std::string& line)
{
	auto line_size = line.size();
	if (line_size > 1
		&& c == line[line_size - 1])
	{
		line.replace(line_size - 1, 1, "");
		return true;
	}
	return false;
}

// replaces escaped characters with the correct characters from left to right
// "\\\\" ---> '\\' (two backslahes become one)
// "\\n" ---> '\n' (backslash n becomes carriage return)
void unescape_string(std::string& line)
{
	auto line_size = line.size();
	for (size_t index = 0; line_size >= 1 && index < line_size - 1; ++index)
	{
		if ('\\' == line[index])
		{
			if ('\\' == line[index + 1])
			{
				line.replace(index, 2, "\\");
				line_size--;
			}
			else if ('n' == line[index + 1])
			{
				line.replace(index, 2, "\n");
				line_size--;
			}
		}
	}
}

// replaces unescaped characters with expanded equivalents from left to right
// '\\' ---> "\\\\" (one backslash becomes two)
// '\n' ---> "\\n"  (carriage return becomes backslash n)
void escape_string(std::string& line)
{
	auto line_size = line.size();
	for (size_t index = 0; index < line_size; ++index)
	{
		if ('\\' == line[index])
		{
			line.replace(index, 1, "\\\\");
			line_size++;
			index++;
		}
		else if ('\n' == line[index])
		{
			line.replace(index, 1, "\\n"); 
			line_size++;
			index++;
		}
	}
}

// removes '\n' characters
void replace_newlines_with_whitespace(std::string& line)
{
	auto line_size = line.size();
	for (size_t index = 0; index < line_size; ++index)
	{
		if ('\n' == line[index])
		{
			line.replace(index, 1, " ");
		}
	}
}

// erases any double-quote characters in 'line'
void remove_double_quotes(std::string& line)
{
	auto line_size = line.size();
	for (size_t index = 0; index < line_size; )
	{
		if ('"' == line[index])
		{
			int count = 1;
			while (index + count < line_size
				   && '"' == line[index + count])
			{
				count++;
			}
			line.replace(index, count, "");
			line_size -= count;
		}
		else
		{
			index++;
		}
	}
}

// the 'keyword' is defined as the first word on a line
// the 'value' is everything after the keyword on the same line
// starting at the first non-whitespace and ending right before the newline
void get_keyword_and_value(std::string& keyword, 
						   std::string& value, 
						   const std::string& line)
{
	// skip initial whitespace
	auto line_size = line.size();
	size_t line_index = 0;
	char c;
	for ( ; line_index < line_size; ++line_index)
	{
		c = line[line_index];
		if (!LLStringOps::isSpace(c))
		{
			break;
		}
	}

	// get the keyword
	keyword.clear();
	for ( ; line_index < line_size; ++line_index)
	{
		c = line[line_index];
		if (LLStringOps::isSpace(c) || '\r' == c || '\n' == c)
		{
			break;
		}
		keyword += c;
	}

	// get the value
	value.clear();
	if (keyword.size() > 0
		&& '\r' != line[line_index]
		&& '\n' != line[line_index])

	{
		// discard initial white spaces
		while (line_index < line_size
				&& (' ' == line[line_index] 
					|| '\t' == line[line_index]) )
		{
			line_index++;
		}

		for ( ; line_index < line_size; ++line_index)
		{
			c = line[line_index];
			if ('\r' == c || '\n' == c)
			{
				break;
			}
			value += c;
		}
	}
}

std::streamsize fullread(
	std::istream& istr,
	char* buf,
	std::streamsize requested)
{
	std::streamsize got;
	std::streamsize total = 0;

	istr.read(buf, requested);	 /*Flawfinder: ignore*/
	got = istr.gcount();
	total += got;
	while(got && total < requested)
	{
		if(istr.fail())
		{
			// If bad is true, not much we can doo -- it implies loss
			// of stream integrity. Bail in that case, and otherwise
			// clear and attempt to continue.
			if(istr.bad()) return total;
			istr.clear();
		}
		istr.read(buf + total, requested - total);	 /*Flawfinder: ignore*/
		got = istr.gcount();
		total += got;
	}
	return total;
}

std::istream& operator>>(std::istream& str, const char *tocheck)
{
	char c = '\0';
	const char *p;
	p = tocheck;
	while (*p && !str.bad())
	{
		str.get(c);
		if (c != *p)
		{
			str.setstate(std::ios::failbit);		/*Flawfinder: ignore*/
			break;
		}
		p++;
	}
	return str;
}

int cat_streambuf::underflow()
{
    if (gptr() == egptr())
    {
        // here because our buffer is empty
        std::streamsize size = 0;
        // Until we've run out of mInputs, try reading the first of them
        // into mBuffer. If that fetches some characters, break the loop.
        while (! mInputs.empty()
               && ! (size = mInputs.front()->sgetn(mBuffer.data(), mBuffer.size())))
        {
            // We tried to read mInputs.front() but got zero characters.
            // Discard the first streambuf and try the next one.
            mInputs.pop_front();
        }
        // Either we ran out of mInputs or we succeeded in reading some
        // characters, that is, size != 0. Tell base class what we have.
        setg(mBuffer.data(), mBuffer.data(), mBuffer.data() + size);
    }
    // If we fell out of the above loop with mBuffer still empty, return
    // eof(), otherwise return the next character.
    return (gptr() == egptr())
        ? std::char_traits<char>::eof()
        : std::char_traits<char>::to_int_type(*gptr());
}
