/** 
 * @file llstring.cpp
 * @brief String utility functions and the std::string class.
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

#include "llstring.h"
#include "llerror.h"

#if LL_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <winnls.h> // for WideCharToMultiByte
#endif

LLFastTimer::DeclareTimer FT_STRING_FORMAT("String Format");


std::string ll_safe_string(const char* in)
{
	if(in) return std::string(in);
	return std::string();
}

std::string ll_safe_string(const char* in, S32 maxlen)
{
	if(in && maxlen > 0 ) return std::string(in, maxlen);

	return std::string();
}

U8 hex_as_nybble(char hex)
{
	if((hex >= '0') && (hex <= '9'))
	{
		return (U8)(hex - '0');
	}
	else if((hex >= 'a') && (hex <='f'))
	{
		return (U8)(10 + hex - 'a');
	}
	else if((hex >= 'A') && (hex <='F'))
	{
		return (U8)(10 + hex - 'A');
	}
	return 0; // uh - oh, not hex any more...
}

bool iswindividual(llwchar elem)
{   
	U32 cur_char = (U32)elem;
	bool result = false;
	if (0x2E80<= cur_char && cur_char <= 0x9FFF)
	{
		result = true;
	}
	else if (0xAC00<= cur_char && cur_char <= 0xD7A0 )
	{
		result = true;
	}
	else if (0xF900<= cur_char && cur_char <= 0xFA60 )
	{
		result = true;
	}
	return result;
}

bool _read_file_into_string(std::string& str, const std::string& filename)
{
	llifstream ifs(filename, llifstream::binary);
	if (!ifs.is_open())
	{
		llinfos << "Unable to open file " << filename << llendl;
		return false;
	}

	std::ostringstream oss;

	oss << ifs.rdbuf();
	str = oss.str();
	ifs.close();
	return true;
}




// See http://www.unicode.org/Public/BETA/CVTUTF-1-2/ConvertUTF.c
// for the Unicode implementation - this doesn't match because it was written before finding
// it.


std::ostream& operator<<(std::ostream &s, const LLWString &wstr)
{
	std::string utf8_str = wstring_to_utf8str(wstr);
	s << utf8_str;
	return s;
}

std::string rawstr_to_utf8(const std::string& raw)
{
	LLWString wstr(utf8str_to_wstring(raw));
	return wstring_to_utf8str(wstr);
}

S32 wchar_to_utf8chars(llwchar in_char, char* outchars)
{
	U32 cur_char = (U32)in_char;
	char* base = outchars;
	if (cur_char < 0x80)
	{
		*outchars++ = (U8)cur_char;
	}
	else if (cur_char < 0x800)
	{
		*outchars++ = 0xC0 | (cur_char >> 6);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else if (cur_char < 0x10000)
	{
		*outchars++ = 0xE0 | (cur_char >> 12);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else if (cur_char < 0x200000)
	{
		*outchars++ = 0xF0 | (cur_char >> 18);
		*outchars++ = 0x80 | ((cur_char >> 12) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else if (cur_char < 0x4000000)
	{
		*outchars++ = 0xF8 | (cur_char >> 24);
		*outchars++ = 0x80 | ((cur_char >> 18) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 12) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else if (cur_char < 0x80000000)
	{
		*outchars++ = 0xFC | (cur_char >> 30);
		*outchars++ = 0x80 | ((cur_char >> 24) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 18) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 12) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else
	{
		llwarns << "Invalid Unicode character " << cur_char << "!" << llendl;
		*outchars++ = LL_UNKNOWN_CHAR;
	}
	return outchars - base;
}	

S32 utf16chars_to_wchar(const U16* inchars, llwchar* outchar)
{
	const U16* base = inchars;
	U16 cur_char = *inchars++;
	llwchar char32 = cur_char;
	if ((cur_char >= 0xD800) && (cur_char <= 0xDFFF))
	{
		// Surrogates
		char32 = ((llwchar)(cur_char - 0xD800)) << 10;
		cur_char = *inchars++;
		char32 += (llwchar)(cur_char - 0xDC00) + 0x0010000UL;
	}
	else
	{
		char32 = (llwchar)cur_char;
	}
	*outchar = char32;
	return inchars - base;
}

llutf16string wstring_to_utf16str(const LLWString &utf32str, S32 len)
{
	llutf16string out;

	S32 i = 0;
	while (i < len)
	{
		U32 cur_char = utf32str[i];
		if (cur_char > 0xFFFF)
		{
			out += (0xD7C0 + (cur_char >> 10));
			out += (0xDC00 | (cur_char & 0x3FF));
		}
		else
		{
			out += cur_char;
		}
		i++;
	}
	return out;
}

llutf16string wstring_to_utf16str(const LLWString &utf32str)
{
	const S32 len = (S32)utf32str.length();
	return wstring_to_utf16str(utf32str, len);
}

llutf16string utf8str_to_utf16str ( const std::string& utf8str )
{
	LLWString wstr = utf8str_to_wstring ( utf8str );
	return wstring_to_utf16str ( wstr );
}


LLWString utf16str_to_wstring(const llutf16string &utf16str, S32 len)
{
	LLWString wout;
	if((len <= 0) || utf16str.empty()) return wout;

	S32 i = 0;
	// craziness to make gcc happy (llutf16string.c_str() is tweaked on linux):
	const U16* chars16 = &(*(utf16str.begin()));
	while (i < len)
	{
		llwchar cur_char;
		i += utf16chars_to_wchar(chars16+i, &cur_char);
		wout += cur_char;
	}
	return wout;
}

LLWString utf16str_to_wstring(const llutf16string &utf16str)
{
	const S32 len = (S32)utf16str.length();
	return utf16str_to_wstring(utf16str, len);
}

// Length in llwchar (UTF-32) of the first len units (16 bits) of the given UTF-16 string.
S32 utf16str_wstring_length(const llutf16string &utf16str, const S32 utf16_len)
{
	S32 surrogate_pairs = 0;
	// ... craziness to make gcc happy (llutf16string.c_str() is tweaked on linux):
	const U16 *const utf16_chars = &(*(utf16str.begin()));
	S32 i = 0;
	while (i < utf16_len)
	{
		const U16 c = utf16_chars[i++];
		if (c >= 0xD800 && c <= 0xDBFF)		// See http://en.wikipedia.org/wiki/UTF-16
		{   // Have first byte of a surrogate pair
			if (i >= utf16_len)
			{
				break;
			}
			const U16 d = utf16_chars[i];
			if (d >= 0xDC00 && d <= 0xDFFF)
			{   // Have valid second byte of a surrogate pair
				surrogate_pairs++;
				i++;
			}
		}
	}
	return utf16_len - surrogate_pairs;
}

// Length in utf16string (UTF-16) of wlen wchars beginning at woffset.
S32 wstring_utf16_length(const LLWString &wstr, const S32 woffset, const S32 wlen)
{
	const S32 end = llmin((S32)wstr.length(), woffset + wlen);
	if (end < woffset)
	{
		return 0;
	}
	else
	{
		S32 length = end - woffset;
		for (S32 i = woffset; i < end; i++)
		{
			if (wstr[i] >= 0x10000)
			{
				length++;
			}
		}
		return length;
	}
}

// Given a wstring and an offset in it, returns the length as wstring (i.e.,
// number of llwchars) of the longest substring that starts at the offset
// and whose equivalent utf-16 string does not exceeds the given utf16_length.
S32 wstring_wstring_length_from_utf16_length(const LLWString & wstr, const S32 woffset, const S32 utf16_length, BOOL *unaligned)
{
	const S32 end = wstr.length();
	BOOL u = FALSE;
	S32 n = woffset + utf16_length;
	S32 i = woffset;
	while (i < end)
	{
		if (wstr[i] >= 0x10000)
		{
			--n;
		}
		if (i >= n)
		{
			u = (i > n);
			break;
		}
		i++;
	}
	if (unaligned)
	{
		*unaligned = u;
	}
	return i - woffset;
}

S32 wchar_utf8_length(const llwchar wc)
{
	if (wc < 0x80)
	{
		// This case will also catch negative values which are
		// technically invalid.
		return 1;
	}
	else if (wc < 0x800)
	{
		return 2;
	}
	else if (wc < 0x10000)
	{
		return 3;
	}
	else if (wc < 0x200000)
	{
		return 4;
	}
	else if (wc < 0x4000000)
	{
		return 5;
	}
	else
	{
		return 6;
	}
}


S32 wstring_utf8_length(const LLWString& wstr)
{
	S32 len = 0;
	for (S32 i = 0; i < (S32)wstr.length(); i++)
	{
		len += wchar_utf8_length(wstr[i]);
	}
	return len;
}


LLWString utf8str_to_wstring(const std::string& utf8str, S32 len)
{
	LLWString wout;

	S32 i = 0;
	while (i < len)
	{
		llwchar unichar;
		U8 cur_char = utf8str[i];

		if (cur_char < 0x80)
		{
			// Ascii character, just add it
			unichar = cur_char;
		}
		else
		{
			S32 cont_bytes = 0;
			if ((cur_char >> 5) == 0x6)			// Two byte UTF8 -> 1 UTF32
			{
				unichar = (0x1F&cur_char);
				cont_bytes = 1;
			}
			else if ((cur_char >> 4) == 0xe)	// Three byte UTF8 -> 1 UTF32
			{
				unichar = (0x0F&cur_char);
				cont_bytes = 2;
			}
			else if ((cur_char >> 3) == 0x1e)	// Four byte UTF8 -> 1 UTF32
			{
				unichar = (0x07&cur_char);
				cont_bytes = 3;
			}
			else if ((cur_char >> 2) == 0x3e)	// Five byte UTF8 -> 1 UTF32
			{
				unichar = (0x03&cur_char);
				cont_bytes = 4;
			}
			else if ((cur_char >> 1) == 0x7e)	// Six byte UTF8 -> 1 UTF32
			{
				unichar = (0x01&cur_char);
				cont_bytes = 5;
			}
			else
			{
				wout += LL_UNKNOWN_CHAR;
				++i;
				continue;
			}

			// Check that this character doesn't go past the end of the string
			S32 end = (len < (i + cont_bytes)) ? len : (i + cont_bytes);
			do
			{
				++i;

				cur_char = utf8str[i];
				if ( (cur_char >> 6) == 0x2 )
				{
					unichar <<= 6;
					unichar += (0x3F&cur_char);
				}
				else
				{
					// Malformed sequence - roll back to look at this as a new char
					unichar = LL_UNKNOWN_CHAR;
					--i;
					break;
				}
			} while(i < end);

			// Handle overlong characters and NULL characters
			if ( ((cont_bytes == 1) && (unichar < 0x80))
				|| ((cont_bytes == 2) && (unichar < 0x800))
				|| ((cont_bytes == 3) && (unichar < 0x10000))
				|| ((cont_bytes == 4) && (unichar < 0x200000))
				|| ((cont_bytes == 5) && (unichar < 0x4000000)) )
			{
				unichar = LL_UNKNOWN_CHAR;
			}
		}

		wout += unichar;
		++i;
	}
	return wout;
}

LLWString utf8str_to_wstring(const std::string& utf8str)
{
	const S32 len = (S32)utf8str.length();
	return utf8str_to_wstring(utf8str, len);
}

std::string wstring_to_utf8str(const LLWString& utf32str, S32 len)
{
	std::string out;

	S32 i = 0;
	while (i < len)
	{
		char tchars[8];		/* Flawfinder: ignore */
		S32 n = wchar_to_utf8chars(utf32str[i], tchars);
		tchars[n] = 0;
		out += tchars;
		i++;
	}
	return out;
}

std::string wstring_to_utf8str(const LLWString& utf32str)
{
	const S32 len = (S32)utf32str.length();
	return wstring_to_utf8str(utf32str, len);
}

std::string utf16str_to_utf8str(const llutf16string& utf16str)
{
	return wstring_to_utf8str(utf16str_to_wstring(utf16str));
}

std::string utf16str_to_utf8str(const llutf16string& utf16str, S32 len)
{
	return wstring_to_utf8str(utf16str_to_wstring(utf16str, len), len);
}

std::string utf8str_trim(const std::string& utf8str)
{
	LLWString wstr = utf8str_to_wstring(utf8str);
	LLWStringUtil::trim(wstr);
	return wstring_to_utf8str(wstr);
}


std::string utf8str_tolower(const std::string& utf8str)
{
	LLWString out_str = utf8str_to_wstring(utf8str);
	LLWStringUtil::toLower(out_str);
	return wstring_to_utf8str(out_str);
}


S32 utf8str_compare_insensitive(const std::string& lhs, const std::string& rhs)
{
	LLWString wlhs = utf8str_to_wstring(lhs);
	LLWString wrhs = utf8str_to_wstring(rhs);
	return LLWStringUtil::compareInsensitive(wlhs, wrhs);
}

std::string utf8str_truncate(const std::string& utf8str, const S32 max_len)
{
	if (0 == max_len)
	{
		return std::string();
	}
	if ((S32)utf8str.length() <= max_len)
	{
		return utf8str;
	}
	else
	{
		S32 cur_char = max_len;

		// If we're ASCII, we don't need to do anything
		if ((U8)utf8str[cur_char] > 0x7f)
		{
			// If first two bits are (10), it's the tail end of a multibyte char.  We need to shift back
			// to the first character
			while (0x80 == (0xc0 & utf8str[cur_char]))
			{
				cur_char--;
				// Keep moving forward until we hit the first char;
				if (cur_char == 0)
				{
					// Make sure we don't trash memory if we've got a bogus string.
					break;
				}
			}
		}
		// The byte index we're on is one we want to get rid of, so we only want to copy up to (cur_char-1) chars
		return utf8str.substr(0, cur_char);
	}
}

std::string utf8str_substChar(
	const std::string& utf8str,
	const llwchar target_char,
	const llwchar replace_char)
{
	LLWString wstr = utf8str_to_wstring(utf8str);
	LLWStringUtil::replaceChar(wstr, target_char, replace_char);
	//wstr = wstring_substChar(wstr, target_char, replace_char);
	return wstring_to_utf8str(wstr);
}

std::string utf8str_makeASCII(const std::string& utf8str)
{
	LLWString wstr = utf8str_to_wstring(utf8str);
	LLWStringUtil::_makeASCII(wstr);
	return wstring_to_utf8str(wstr);
}

std::string mbcsstring_makeASCII(const std::string& wstr)
{
	// Replace non-ASCII chars with replace_char
	std::string out_str = wstr;
	for (S32 i = 0; i < (S32)out_str.length(); i++)
	{
		if ((U8)out_str[i] > 0x7f)
		{
			out_str[i] = LL_UNKNOWN_CHAR;
		}
	}
	return out_str;
}
std::string utf8str_removeCRLF(const std::string& utf8str)
{
	if (0 == utf8str.length())
	{
		return std::string();
	}
	const char CR = 13;

	std::string out;
	out.reserve(utf8str.length());
	const S32 len = (S32)utf8str.length();
	for( S32 i = 0; i < len; i++ )
	{
		if( utf8str[i] != CR )
		{
			out.push_back(utf8str[i]);
		}
	}
	return out;
}

#if LL_WINDOWS
// documentation moved to header. Phoenix 2007-11-27
namespace snprintf_hack
{
	int snprintf(char *str, size_t size, const char *format, ...)
	{
		va_list args;
		va_start(args, format);

		int num_written = _vsnprintf(str, size, format, args); /* Flawfinder: ignore */
		va_end(args);
		
		str[size-1] = '\0'; // always null terminate
		return num_written;
	}
}

std::string ll_convert_wide_to_string(const wchar_t* in, unsigned int code_page)
{
	std::string out;
	if(in)
	{
		int len_in = wcslen(in);
		int len_out = WideCharToMultiByte(
			code_page,
			0,
			in,
			len_in,
			NULL,
			0,
			0,
			0);
		// We will need two more bytes for the double NULL ending
		// created in WideCharToMultiByte().
		char* pout = new char [len_out + 2];
		memset(pout, 0, len_out + 2);
		if(pout)
		{
			WideCharToMultiByte(
				code_page,
				0,
				in,
				len_in,
				pout,
				len_out,
				0,
				0);
			out.assign(pout);
			delete[] pout;
		}
	}
	return out;
}

wchar_t* ll_convert_string_to_wide(const std::string& in, unsigned int code_page)
{
	// From review:
	// We can preallocate a wide char buffer that is the same length (in wchar_t elements) as the utf8 input,
	// plus one for a null terminator, and be guaranteed to not overflow.

	//	Normally, I'd call that sort of thing premature optimization,
	// but we *are* seeing string operations taking a bunch of time, especially when constructing widgets.
//	int output_str_len = MultiByteToWideChar(code_page, 0, in.c_str(), in.length(), NULL, 0);

	// reserve place to NULL terminator
	int output_str_len = in.length();
	wchar_t* w_out = new wchar_t[output_str_len + 1];

	memset(w_out, 0, output_str_len + 1);
	int real_output_str_len = MultiByteToWideChar (code_page, 0, in.c_str(), in.length(), w_out, output_str_len);

	//looks like MultiByteToWideChar didn't add null terminator to converted string, see EXT-4858.
	w_out[real_output_str_len] = 0;

	return w_out;
}

std::string ll_convert_string_to_utf8_string(const std::string& in)
{
	wchar_t* w_mesg = ll_convert_string_to_wide(in, CP_ACP);
	std::string out_utf8(ll_convert_wide_to_string(w_mesg, CP_UTF8));
	delete[] w_mesg;

	return out_utf8;
}
#endif // LL_WINDOWS

long LLStringOps::sPacificTimeOffset = 0;
long LLStringOps::sLocalTimeOffset = 0;
bool LLStringOps::sPacificDaylightTime = 0;
std::map<std::string, std::string> LLStringOps::datetimeToCodes;

std::vector<std::string> LLStringOps::sWeekDayList;
std::vector<std::string> LLStringOps::sWeekDayShortList;
std::vector<std::string> LLStringOps::sMonthList;
std::vector<std::string> LLStringOps::sMonthShortList;


std::string LLStringOps::sDayFormat;
std::string LLStringOps::sAM;
std::string LLStringOps::sPM;


S32	LLStringOps::collate(const llwchar* a, const llwchar* b)
{ 
	#if LL_WINDOWS
		// in Windows, wide string functions operator on 16-bit strings, 
		// not the proper 32 bit wide string
		return strcmp(wstring_to_utf8str(LLWString(a)).c_str(), wstring_to_utf8str(LLWString(b)).c_str());
	#else
		return wcscoll(a, b);
	#endif
}

void LLStringOps::setupDatetimeInfo (bool daylight)
{
	time_t nowT, localT, gmtT;
	struct tm * tmpT;

	nowT = time (NULL);

	tmpT = gmtime (&nowT);
	gmtT = mktime (tmpT);

	tmpT = localtime (&nowT);
	localT = mktime (tmpT);
	
	sLocalTimeOffset = (long) (gmtT - localT);
	if (tmpT->tm_isdst)
	{
		sLocalTimeOffset -= 60 * 60;	// 1 hour
	}

	sPacificDaylightTime = daylight;
	sPacificTimeOffset = (sPacificDaylightTime? 7 : 8 ) * 60 * 60;

	datetimeToCodes["wkday"]	= "%a";		// Thu
	datetimeToCodes["weekday"]	= "%A";		// Thursday
	datetimeToCodes["year4"]	= "%Y";		// 2009
	datetimeToCodes["year"]		= "%Y";		// 2009
	datetimeToCodes["year2"]	= "%y";		// 09
	datetimeToCodes["mth"]		= "%b";		// Aug
	datetimeToCodes["month"]	= "%B";		// August
	datetimeToCodes["mthnum"]	= "%m";		// 08
	datetimeToCodes["day"]		= "%d";		// 31
	datetimeToCodes["sday"]		= "%-d";	// 9
	datetimeToCodes["hour24"]	= "%H";		// 14
	datetimeToCodes["hour"]		= "%H";		// 14
	datetimeToCodes["hour12"]	= "%I";		// 02
	datetimeToCodes["min"]		= "%M";		// 59
	datetimeToCodes["ampm"]		= "%p";		// AM
	datetimeToCodes["second"]	= "%S";		// 59
	datetimeToCodes["timezone"]	= "%Z";		// PST
}

void tokenizeStringToArray(const std::string& data, std::vector<std::string>& output)
{
	output.clear();
	size_t length = data.size();
	
	// tokenize it and put it in the array
	std::string cur_word;
	for(size_t i = 0; i < length; ++i)
	{
		if(data[i] == ':')
		{
			output.push_back(cur_word);
			cur_word.clear();
		}
		else
		{
			cur_word.append(1, data[i]);
		}
	}
	output.push_back(cur_word);
}

void LLStringOps::setupWeekDaysNames(const std::string& data)
{
	tokenizeStringToArray(data,sWeekDayList);
}
void LLStringOps::setupWeekDaysShortNames(const std::string& data)
{
	tokenizeStringToArray(data,sWeekDayShortList);
}
void LLStringOps::setupMonthNames(const std::string& data)
{
	tokenizeStringToArray(data,sMonthList);
}
void LLStringOps::setupMonthShortNames(const std::string& data)
{
	tokenizeStringToArray(data,sMonthShortList);
}
void LLStringOps::setupDayFormat(const std::string& data)
{
	sDayFormat = data;
}


std::string LLStringOps::getDatetimeCode (std::string key)
{
	std::map<std::string, std::string>::iterator iter;

	iter = datetimeToCodes.find (key);
	if (iter != datetimeToCodes.end())
	{
		return iter->second;
	}
	else
	{
		return std::string("");
	}
}


namespace LLStringFn
{
	// NOTE - this restricts output to ascii
	void replace_nonprintable_in_ascii(std::basic_string<char>& string, char replacement)
	{
		const char MIN = 0x20;
		std::basic_string<char>::size_type len = string.size();
		for(std::basic_string<char>::size_type ii = 0; ii < len; ++ii)
		{
			if(string[ii] < MIN)
			{
				string[ii] = replacement;
			}
		}
	}


	// NOTE - this restricts output to ascii
	void replace_nonprintable_and_pipe_in_ascii(std::basic_string<char>& str,
									   char replacement)
	{
		const char MIN  = 0x20;
		const char PIPE = 0x7c;
		std::basic_string<char>::size_type len = str.size();
		for(std::basic_string<char>::size_type ii = 0; ii < len; ++ii)
		{
			if( (str[ii] < MIN) || (str[ii] == PIPE) )
			{
				str[ii] = replacement;
			}
		}
	}

	// https://wiki.lindenlab.com/wiki/Unicode_Guidelines has details on
	// allowable code points for XML. Specifically, they are:
	// 0x09, 0x0a, 0x0d, and 0x20 on up.  JC
	std::string strip_invalid_xml(const std::string& instr)
	{
		std::string output;
		output.reserve( instr.size() );
		std::string::const_iterator it = instr.begin();
		while (it != instr.end())
		{
			// Must compare as unsigned for >=
			// Test most likely match first
			const unsigned char c = (unsigned char)*it;
			if (   c >= (unsigned char)0x20   // SPACE
				|| c == (unsigned char)0x09   // TAB
				|| c == (unsigned char)0x0a   // LINE_FEED
				|| c == (unsigned char)0x0d ) // CARRIAGE_RETURN
			{
				output.push_back(c);
			}
			++it;
		}
		return output;
	}

	/**
	 * @brief Replace all control characters (c < 0x20) with replacement in
	 * string.
	 */
	void replace_ascii_controlchars(std::basic_string<char>& string, char replacement)
	{
		const unsigned char MIN = 0x20;
		std::basic_string<char>::size_type len = string.size();
		for(std::basic_string<char>::size_type ii = 0; ii < len; ++ii)
		{
			const unsigned char c = (unsigned char) string[ii];
			if(c < MIN)
			{
				string[ii] = replacement;
			}
		}
	}
}

////////////////////////////////////////////////////////////

// Forward specialization of LLStringUtil::format before use in LLStringUtil::formatDatetime.
template<>
S32 LLStringUtil::format(std::string& s, const format_map_t& substitutions);

//static
template<> 
void LLStringUtil::getTokens(const std::string& instr, std::vector<std::string >& tokens, const std::string& delims)
{
	// Starting at offset 0, scan forward for the next non-delimiter. We're
	// done when the only characters left in 'instr' are delimiters.
	for (std::string::size_type begIdx, endIdx = 0;
		 (begIdx = instr.find_first_not_of (delims, endIdx)) != std::string::npos; )
	{
		// Found a non-delimiter. After that, find the next delimiter.
		endIdx = instr.find_first_of (delims, begIdx);
		if (endIdx == std::string::npos)
		{
			// No more delimiters: this token extends to the end of the string.
			endIdx = instr.length();
		}

		// extract the token between begIdx and endIdx; substr() needs length
		std::string currToken(instr.substr(begIdx, endIdx - begIdx));
		LLStringUtil::trim (currToken);
		tokens.push_back(currToken);
		// next scan past delimiters starts at endIdx
	}
}

template<> 
LLStringUtil::size_type LLStringUtil::getSubstitution(const std::string& instr, size_type& start, std::vector<std::string>& tokens)
{
	const std::string delims (",");
	
	// Find the first [
	size_type pos1 = instr.find('[', start);
	if (pos1 == std::string::npos)
		return std::string::npos;

	//Find the first ] after the initial [
	size_type pos2 = instr.find(']', pos1);
	if (pos2 == std::string::npos)
		return std::string::npos;

	// Find the last [ before ] in case of nested [[]]
	pos1 = instr.find_last_of('[', pos2-1);
	if (pos1 == std::string::npos || pos1 < start)
		return std::string::npos;
	
	getTokens(std::string(instr,pos1+1,pos2-pos1-1), tokens, delims);
	start = pos2+1;
	
	return pos1;
}

// static
template<> 
bool LLStringUtil::simpleReplacement(std::string &replacement, std::string token, const format_map_t& substitutions)
{
	// see if we have a replacement for the bracketed string (without the brackets)
	// test first using has() because if we just look up with operator[] we get back an
	// empty string even if the value is missing. We want to distinguish between 
	// missing replacements and deliberately empty replacement strings.
	format_map_t::const_iterator iter = substitutions.find(token);
	if (iter != substitutions.end())
	{
		replacement = iter->second;
		return true;
	}
	// if not, see if there's one WITH brackets
	iter = substitutions.find(std::string("[" + token + "]"));
	if (iter != substitutions.end())
	{
		replacement = iter->second;
		return true;
	}

	return false;
}

// static
template<> 
bool LLStringUtil::simpleReplacement(std::string &replacement, std::string token, const LLSD& substitutions)
{
	// see if we have a replacement for the bracketed string (without the brackets)
	// test first using has() because if we just look up with operator[] we get back an
	// empty string even if the value is missing. We want to distinguish between 
	// missing replacements and deliberately empty replacement strings.
	if (substitutions.has(token))
	{
		replacement = substitutions[token].asString();
		return true;
	}
	// if not, see if there's one WITH brackets
	else if (substitutions.has(std::string("[" + token + "]")))
	{
		replacement = substitutions[std::string("[" + token + "]")].asString();
		return true;
	}

	return false;
}

//static
template<>
void LLStringUtil::setLocale(std::string inLocale)
{
	sLocale = inLocale;
};

//static
template<>
std::string LLStringUtil::getLocale(void)
{
	return sLocale;
};

// static
template<> 
void LLStringUtil::formatNumber(std::string& numStr, std::string decimals)
{
	std::stringstream strStream;
	S32 intDecimals = 0;

	convertToS32 (decimals, intDecimals);
	if (!sLocale.empty())
	{
		// std::locale() throws if the locale is unknown! (EXT-7926)
		try
		{
			strStream.imbue(std::locale(sLocale.c_str()));
		} catch (const std::exception &)
		{
			LL_WARNS_ONCE("Locale") << "Cannot set locale to " << sLocale << LL_ENDL;
		}
	}

	if (!intDecimals)
	{
		S32 intStr;

		if (convertToS32(numStr, intStr))
		{
			strStream << intStr;
			numStr = strStream.str();
		}
	}
	else
	{
		F32 floatStr;

		if (convertToF32(numStr, floatStr))
		{
			strStream << std::fixed << std::showpoint << std::setprecision(intDecimals) << floatStr;
			numStr = strStream.str();
		}
	}
}

// static
template<> 
bool LLStringUtil::formatDatetime(std::string& replacement, std::string token,
								  std::string param, S32 secFromEpoch)
{
	if (param == "local")   // local
	{
		secFromEpoch -= LLStringOps::getLocalTimeOffset();
	}
	else if (param != "utc") // slt
	{
		secFromEpoch -= LLStringOps::getPacificTimeOffset();
	}
		
	// if never fell into those two ifs above, param must be utc
	if (secFromEpoch < 0) secFromEpoch = 0;

	LLDate datetime((F64)secFromEpoch);
	std::string code = LLStringOps::getDatetimeCode (token);

	// special case to handle timezone
	if (code == "%Z") {
		if (param == "utc")
		{
			replacement = "GMT";
		}
		else if (param == "local")
		{
			replacement = "";		// user knows their own timezone
		}
		else
		{
			// "slt" = Second Life Time, which is deprecated.
			// If not utc or user local time, fallback to Pacific time
			replacement = LLStringOps::getPacificDaylightTime() ? "PDT" : "PST";
		}
		return true;
	}

	//EXT-7013
	//few codes are not suppotred by strtime function (example - weekdays for Japanise)
	//so use predefined ones
	
	//if sWeekDayList is not empty than current locale doesn't support
        //weekday name.
	time_t loc_seconds = (time_t) secFromEpoch;
	if(LLStringOps::sWeekDayList.size() == 7 && code == "%A")
	{
		struct tm * gmt = gmtime (&loc_seconds);
		replacement = LLStringOps::sWeekDayList[gmt->tm_wday];
	}
	else if(LLStringOps::sWeekDayShortList.size() == 7 && code == "%a")
	{
		struct tm * gmt = gmtime (&loc_seconds);
		replacement = LLStringOps::sWeekDayShortList[gmt->tm_wday];
	}
	else if(LLStringOps::sMonthList.size() == 12 && code == "%B")
	{
		struct tm * gmt = gmtime (&loc_seconds);
		replacement = LLStringOps::sMonthList[gmt->tm_mon];
	}
	else if( !LLStringOps::sDayFormat.empty() && code == "%d" )
	{
		struct tm * gmt = gmtime (&loc_seconds);
		LLStringUtil::format_map_t args;
		args["[MDAY]"] = llformat ("%d", gmt->tm_mday);
		replacement = LLStringOps::sDayFormat;
		LLStringUtil::format(replacement, args);
	}
	else if (code == "%-d")
	{
		struct tm * gmt = gmtime (&loc_seconds);
		replacement = llformat ("%d", gmt->tm_mday); // day of the month without leading zero
	}
	else if( !LLStringOps::sAM.empty() && !LLStringOps::sPM.empty() && code == "%p" )
	{
		struct tm * gmt = gmtime (&loc_seconds);
		if(gmt->tm_hour<12)
		{
			replacement = LLStringOps::sAM;
		}
		else
		{
			replacement = LLStringOps::sPM;
		}
	}
	else
	{
		replacement = datetime.toHTTPDateString(code);
	}

	// *HACK: delete leading zero from hour string in case 'hour12' (code = %I) time format
	// to show time without leading zero, e.g. 08:16 -> 8:16 (EXT-2738).
	// We could have used '%l' format instead, but it's not supported by Windows.
	if(code == "%I" && token == "hour12" && replacement.at(0) == '0')
	{
		replacement = replacement.at(1);
	}

	return !code.empty();
}

// LLStringUtil::format recogizes the following patterns.
// All substitutions *must* be encased in []'s in the input string.
// The []'s are optional in the substitution map.
// [FOO_123]
// [FOO,number,precision]
// [FOO,datetime,format]


// static
template<> 
S32 LLStringUtil::format(std::string& s, const format_map_t& substitutions)
{
	LLFastTimer ft(FT_STRING_FORMAT);
	S32 res = 0;

	std::string output;
	std::vector<std::string> tokens;

	std::string::size_type start = 0;
	std::string::size_type prev_start = 0;
	std::string::size_type key_start = 0;
	while ((key_start = getSubstitution(s, start, tokens)) != std::string::npos)
	{
		output += std::string(s, prev_start, key_start-prev_start);
		prev_start = start;
		
		bool found_replacement = false;
		std::string replacement;

		if (tokens.size() == 0)
		{
			found_replacement = false;
		}
		else if (tokens.size() == 1)
		{
			found_replacement = simpleReplacement (replacement, tokens[0], substitutions);
		}
		else if (tokens[1] == "number")
		{
			std::string param = "0";

			if (tokens.size() > 2) param = tokens[2];
			found_replacement = simpleReplacement (replacement, tokens[0], substitutions);
			if (found_replacement) formatNumber (replacement, param);
		}
		else if (tokens[1] == "datetime")
		{
			std::string param;
			if (tokens.size() > 2) param = tokens[2];
			
			format_map_t::const_iterator iter = substitutions.find("datetime");
			if (iter != substitutions.end())
			{
				S32 secFromEpoch = 0;
				BOOL r = LLStringUtil::convertToS32(iter->second, secFromEpoch);
				if (r)
				{
					found_replacement = formatDatetime(replacement, tokens[0], param, secFromEpoch);
				}
			}
		}

		if (found_replacement)
		{
			output += replacement;
			res++;
		}
		else
		{
			// we had no replacement, use the string as is
			// e.g. "hello [MISSING_REPLACEMENT]" or "-=[Stylized Name]=-"
			output += std::string(s, key_start, start-key_start);
		}
		tokens.clear();
	}
	// send the remainder of the string (with no further matches for bracketed names)
	output += std::string(s, start);
	s = output;
	return res;
}

//static
template<> 
S32 LLStringUtil::format(std::string& s, const LLSD& substitutions)
{
	LLFastTimer ft(FT_STRING_FORMAT);
	S32 res = 0;

	if (!substitutions.isMap()) 
	{
		return res;
	}

	std::string output;
	std::vector<std::string> tokens;

	std::string::size_type start = 0;
	std::string::size_type prev_start = 0;
	std::string::size_type key_start = 0;
	while ((key_start = getSubstitution(s, start, tokens)) != std::string::npos)
	{
		output += std::string(s, prev_start, key_start-prev_start);
		prev_start = start;
		
		bool found_replacement = false;
		std::string replacement;

		if (tokens.size() == 0)
		{
			found_replacement = false;
		}
		else if (tokens.size() == 1)
		{
			found_replacement = simpleReplacement (replacement, tokens[0], substitutions);
		}
		else if (tokens[1] == "number")
		{
			std::string param = "0";

			if (tokens.size() > 2) param = tokens[2];
			found_replacement = simpleReplacement (replacement, tokens[0], substitutions);
			if (found_replacement) formatNumber (replacement, param);
		}
		else if (tokens[1] == "datetime")
		{
			std::string param;
			if (tokens.size() > 2) param = tokens[2];
			
			S32 secFromEpoch = (S32) substitutions["datetime"].asInteger();
			found_replacement = formatDatetime (replacement, tokens[0], param, secFromEpoch);
		}

		if (found_replacement)
		{
			output += replacement;
			res++;
		}
		else
		{
			// we had no replacement, use the string as is
			// e.g. "hello [MISSING_REPLACEMENT]" or "-=[Stylized Name]=-"
			output += std::string(s, key_start, start-key_start);
		}
		tokens.clear();
	}
	// send the remainder of the string (with no further matches for bracketed names)
	output += std::string(s, start);
	s = output;
	return res;
}

////////////////////////////////////////////////////////////
// Testing

#ifdef _DEBUG

template<class T> 
void LLStringUtilBase<T>::testHarness()
{
	std::string s1;
	
	llassert( s1.c_str() == NULL );
	llassert( s1.size() == 0 );
	llassert( s1.empty() );
	
	std::string s2( "hello");
	llassert( !strcmp( s2.c_str(), "hello" ) );
	llassert( s2.size() == 5 ); 
	llassert( !s2.empty() );
	std::string s3( s2 );

	llassert( "hello" == s2 );
	llassert( s2 == "hello" );
	llassert( s2 > "gello" );
	llassert( "gello" < s2 );
	llassert( "gello" != s2 );
	llassert( s2 != "gello" );

	std::string s4 = s2;
	llassert( !s4.empty() );
	s4.empty();
	llassert( s4.empty() );
	
	std::string s5("");
	llassert( s5.empty() );
	
	llassert( isValidIndex(s5, 0) );
	llassert( !isValidIndex(s5, 1) );
	
	s3 = s2;
	s4 = "hello again";
	
	s4 += "!";
	s4 += s4;
	llassert( s4 == "hello again!hello again!" );
	
	
	std::string s6 = s2 + " " + s2;
	std::string s7 = s6;
	llassert( s6 == s7 );
	llassert( !( s6 != s7) );
	llassert( !(s6 < s7) );
	llassert( !(s6 > s7) );
	
	llassert( !(s6 == "hi"));
	llassert( s6 == "hello hello");
	llassert( s6 < "hi");
	
	llassert( s6[1] == 'e' );
	s6[1] = 'f';
	llassert( s6[1] == 'f' );
	
	s2.erase( 4, 1 );
	llassert( s2 == "hell");
	s2.insert( 0, 'y' );
	llassert( s2 == "yhell");
	s2.erase( 1, 3 );
	llassert( s2 == "yl");
	s2.insert( 1, "awn, don't yel");
	llassert( s2 == "yawn, don't yell");
	
	std::string s8 = s2.substr( 6, 5 );
	llassert( s8 == "don't"  );
	
	std::string s9 = "   \t\ntest  \t\t\n  ";
	trim(s9);
	llassert( s9 == "test"  );

	s8 = "abc123&*(ABC";

	s9 = s8;
	toUpper(s9);
	llassert( s9 == "ABC123&*(ABC"  );

	s9 = s8;
	toLower(s9);
	llassert( s9 == "abc123&*(abc"  );


	std::string s10( 10, 'x' );
	llassert( s10 == "xxxxxxxxxx" );

	std::string s11( "monkey in the middle", 7, 2 );
	llassert( s11 == "in" );

	std::string s12;  //empty
	s12 += "foo";
	llassert( s12 == "foo" );

	std::string s13;  //empty
	s13 += 'f';
	llassert( s13 == "f" );
}


#endif  // _DEBUG
