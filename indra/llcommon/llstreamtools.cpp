/** 
 * @file llstreamtools.cpp
 * @brief some helper functions for parsing legacy simstate and asset files.
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	char c = input_stream.peek();
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
	char c = input_stream.peek();
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
		char c = input_stream.peek();
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
	char c;
	do
	{
		c = input_stream.get();
	} while ('\n' != c  &&  input_stream.good());
	return input_stream.good();
}

bool skip_to_next_word(std::istream& input_stream)
{
	char c = input_stream.peek();
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
	int key_length = strlen(keyword);	 /*Flawfinder: ignore*/
	if (0 == key_length)
	{
		return false;
	}
	while (input_stream.good())
	{
		skip_emptyspace(input_stream);
		char c = input_stream.get();
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
		char c = input_stream.get();
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
	char c = input_stream.peek();
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
	char c = input_stream.peek();
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
	char c = input_stream.get();
	while (input_stream.good())
	{
		if ('\r' == c)
		{
			// skip carriage returns
		}
		else
		{
			output_string += c;
			if ('\n' == c)
			{
				break;
			}
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
	int char_count = 0;
	char c = input_stream.get();
	while (input_stream.good() && char_count < n)
	{
		char_count++;
		output_string += c;
		if ('\r' == c)
		{
			// skip carriage returns
		}
		else
		{
			if ('\n' == c)
			{
				break;
			}
			if (char_count >= n)
			{
				output_string.append("\n");
				break;
			}
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
	int line_size = line.size();
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
	int line_size = line.size();
	int index = 0;
	while (index < line_size - 1)
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
		index++;
	}
}

// replaces unescaped characters with expanded equivalents from left to right
// '\\' ---> "\\\\" (one backslash becomes two)
// '\n' ---> "\\n"  (carriage return becomes backslash n)
void escape_string(std::string& line)
{
	int line_size = line.size();
	int index = 0;
	while (index < line_size)
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
		index++;
	}
}

// removes '\n' characters
void replace_newlines_with_whitespace(std::string& line)
{
	int line_size = line.size();
	int index = 0;
	while (index < line_size)
	{
		if ('\n' == line[index])
		{
			line.replace(index, 1, " ");
		}
		index++;
	}
}

// returns 1 for solitary "{"
// returns -1 for solitary "}"
// otherwise returns 0
int get_brace_count(const std::string& line)
{
	int index = 0;
	int line_size = line.size();
	char c = 0;
	while (index < line_size)
	{
		c = line[index];
		index++;
		if (!isspace(c))
		{
			break;
		}
	}
	char brace = c;
	// make sure the rest of the line is whitespace
	while (index < line_size)
	{
		c = line[index];
		if (!isspace(c))
		{
			break;
		}
		index++;
	}
	if ('\n' != c)
	{
		return 0;
	}
	if ('{' == brace)
	{
		return 1;
	}
	else if ('}' == brace)
	{
		return -1;
	}
	return 0;
}

// erases any double-quote characters in 'line'
void remove_double_quotes(std::string& line)
{
	int index = 0;
	int line_size = line.size();
	while (index < line_size)
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
	int line_size = line.size();
	int line_index = 0;
	char c;
	while (line_index < line_size)
	{
		c = line[line_index];
		if (!isspace(c))
		{
			break;
		}
		line_index++;
	}

	// get the keyword
	keyword.assign("");
	while (line_index < line_size)
	{
		c = line[line_index];
		if (isspace(c) || '\r' == c || '\n' == c)
		{
			break;
		}
		keyword += c;
		line_index++;
	}

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

		// get the value
		value.assign("");
		while (line_index < line_size)
		{
			c = line[line_index];
			if ('\r' == c || '\n' == c)
			{
				break;
			}
			value += c;
			line_index++;
		}
	}
}

std::istream& fullread(std::istream& str, char *buf, std::streamsize requested)
{
	std::streamsize got;
	std::streamsize total = 0;

	str.read(buf, requested);	 /*Flawfinder: ignore*/
	got = str.gcount();
	total += got;
	while (got && total < requested)
	{
		if (str.fail())
			str.clear();
		str.read(buf + total, requested - total);	 /*Flawfinder: ignore*/
		got = str.gcount();
		total += got;
	}
	return str;
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
