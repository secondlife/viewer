/** 
 * @file llstring.cpp
 * @brief String utility functions and the LLString class.
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

#include "linden_common.h"

#include "llstring.h"
#include "llerror.h"

#if LL_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <winnls.h> // for WideCharToMultiByte
#endif

std::string ll_safe_string(const char* in)
{
	if(in) return std::string(in);
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


bool _read_file_into_string(std::string& str, const char* filename)
{
	llifstream ifs(filename, llifstream::binary);
	if (!ifs.is_open())
	{
		llinfos << "Unable to open file" << filename << llendl;
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
		*outchars++ = 0x80 | cur_char & 0x3F;
	}
	else if (cur_char < 0x4000000)
	{
		*outchars++ = 0xF8 | (cur_char >> 24);
		*outchars++ = 0x80 | ((cur_char >> 18) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 12) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | cur_char & 0x3F;
	}
	else if (cur_char < 0x80000000)
	{
		*outchars++ = 0xFC | (cur_char >> 30);
		*outchars++ = 0x80 | ((cur_char >> 24) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 18) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 12) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | cur_char & 0x3F;
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

S32 utf16chars_to_utf8chars(const U16* inchars, char* outchars, S32* nchars8p)
{
	// Get 32 bit char32
	llwchar char32;
	S32 nchars16 = utf16chars_to_wchar(inchars, &char32);
	// Convert to utf8
	S32 nchars8  = wchar_to_utf8chars(char32, outchars);
	if (nchars8p)
	{
		*nchars8p = nchars8;
	}
	return nchars16;
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

llutf16string utf8str_to_utf16str ( const LLString& utf8str )
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


//LLWString wstring_truncate(const LLWString &wstr, const S32 max_len)
//{
//	return wstr.substr(0, llmin((S32)wstr.length(), max_len));
//}
//
//
//LLWString wstring_trim(const LLWString &wstr)
//{
//	LLWString outstr;
//	outstr = wstring_trimhead(wstr);
//	outstr = wstring_trimtail(outstr);
//	return outstr;
//}
//
//
//LLWString wstring_trimhead(const LLWString &wstr)
//{
//	if(wstr.empty())
//	{
//		return wstr;
//	}
//
//    S32 i = 0;
//	while((i < (S32)wstr.length()) && iswspace(wstr[i]))
//	{
//		i++;
//	}
//	return wstr.substr(i, wstr.length() - i);
//}
//
//
//LLWString wstring_trimtail(const LLWString &wstr)
//{			
//	if(wstr.empty())
//	{
//		return wstr;
//	}
//
//	S32 len = (S32)wstr.length();
//
//	S32 i = len - 1;
//	while (i >= 0 && iswspace(wstr[i]))
//	{
//		i--;
//	}
//
//	if (i >= 0)
//	{
//		return wstr.substr(0, i + 1);
//	}
//	return wstr;
//}
//
//
//LLWString wstring_copyinto(const LLWString &dest, const LLWString &src, const S32 insert_offset)
//{
//	llassert( insert_offset <= (S32)dest.length() );
//
//	LLWString out_str = dest.substr(0, insert_offset);
//	out_str += src;
//	LLWString tail = dest.substr(insert_offset);
//	out_str += tail;
//
//	return out_str;
//}


//LLWString wstring_detabify(const LLWString &wstr, const S32 num_spaces)
//{
//	LLWString out_str;
//	// Replace tabs with spaces
//	for (S32 i = 0; i < (S32)wstr.length(); i++)
//	{
//		if (wstr[i] == '\t')
//		{
//			for (S32 j = 0; j < num_spaces; j++)
//				out_str += ' ';
//		}
//		else
//		{
//			out_str += wstr[i];
//		}
//	}
//	return out_str;
//}


//LLWString wstring_makeASCII(const LLWString &wstr)
//{
//	// Replace non-ASCII chars with replace_char
//	LLWString out_str = wstr;
//	for (S32 i = 0; i < (S32)out_str.length(); i++)
//	{
//		if (out_str[i] > 0x7f)
//		{
//			out_str[i] = LL_UNKNOWN_CHAR;
//		}
//	}
//	return out_str;
//}


//LLWString wstring_substChar(const LLWString &wstr, const llwchar target_char, const llwchar replace_char)
//{
//	// Replace all occurences of target_char with replace_char
//	LLWString out_str = wstr;
//	for (S32 i = 0; i < (S32)out_str.length(); i++)
//	{
//		if (out_str[i] == target_char)
//		{
//			out_str[i] = replace_char;
//		}
//	}
//	return out_str;
//}
//
//
//LLWString wstring_tolower(const LLWString &wstr)
//{
//	LLWString out_str = wstr;
//	for (S32 i = 0; i < (S32)out_str.length(); i++)
//	{
//		out_str[i] = towlower(out_str[i]);
//	}
//	return out_str;
//}
//
//
//LLWString wstring_convert_to_lf(const LLWString &wstr)
//{
//	const llwchar CR = 13;
//	// Remove carriage returns from string with CRLF
//	LLWString out_str;
//
//	for (S32 i = 0; i < (S32)wstr.length(); i++)
//	{
//		if (wstr[i] != CR)
//		{
//			out_str += wstr[i];
//		}
//	}
//	return out_str;
//}
//
//
//LLWString wstring_convert_to_crlf(const LLWString &wstr)
//{
//	const llwchar LF = 10;
//	const llwchar CR = 13;
//	// Remove carriage returns from string with CRLF
//	LLWString out_str;
//
//	for (S32 i = 0; i < (S32)wstr.length(); i++)
//	{
//		if (wstr[i] == LF)
//		{
//			out_str += CR;
//		}
//		out_str += wstr[i];
//	}
//	return out_str;
//}


//S32	wstring_compare_insensitive(const LLWString &lhs, const LLWString &rhs)
//{
//
//	if (lhs == rhs)
//	{
//		return 0;
//	}
//
//	if (lhs.empty())
//	{
//		return rhs.empty() ? 0 : 1;
//	}
//
//	if (rhs.empty())
//	{
//		return -1;
//	}
//
//#ifdef LL_LINUX
//	// doesn't work because gcc 2.95 doesn't correctly implement c_str().  Sigh...
//	llerrs << "wstring_compare_insensitive doesn't work on Linux!" << llendl;
//	return 0;
//#else
//	LLWString lhs_lower = lhs;
//	LLWString::toLower(lhs_lower);
//	std::string lhs_lower = wstring_to_utf8str(lhs_lower);
//	LLWString rhs_lower = lhs;
//	LLWString::toLower(rhs_lower);
//	std::string rhs_lower = wstring_to_utf8str(rhs_lower);
//
//	return strcmp(lhs_lower.c_str(), rhs_lower.c_str());
//#endif
//}


std::string utf8str_trim(const std::string& utf8str)
{
	LLWString wstr = utf8str_to_wstring(utf8str);
	LLWString::trim(wstr);
	return wstring_to_utf8str(wstr);
}


std::string utf8str_tolower(const std::string& utf8str)
{
	LLWString out_str = utf8str_to_wstring(utf8str);
	LLWString::toLower(out_str);
	return wstring_to_utf8str(out_str);
}


S32 utf8str_compare_insensitive(const std::string& lhs, const std::string& rhs)
{
	LLWString wlhs = utf8str_to_wstring(lhs);
	LLWString wrhs = utf8str_to_wstring(rhs);
	return LLWString::compareInsensitive(wlhs.c_str(), wrhs.c_str());
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
	LLWString::replaceChar(wstr, target_char, replace_char);
	//wstr = wstring_substChar(wstr, target_char, replace_char);
	return wstring_to_utf8str(wstr);
}

std::string utf8str_makeASCII(const std::string& utf8str)
{
	LLWString wstr = utf8str_to_wstring(utf8str);
	LLWString::_makeASCII(wstr);
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

std::string ll_convert_wide_to_string(const wchar_t* in)
{
	std::string out;
	if(in)
	{
		int len_in = wcslen(in);
		int len_out = WideCharToMultiByte(
			CP_ACP,
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
				CP_ACP,
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
#endif // LL_WINDOWS

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

namespace LLStringFn
{
	void replace_nonprintable(std::basic_string<char>& string, char replacement)
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

	void replace_nonprintable(
		std::basic_string<llwchar>& string,
		llwchar replacement)
	{
		const llwchar MIN = 0x20;
		const llwchar MAX = 0x7f;
		std::basic_string<llwchar>::size_type len = string.size();
		for(std::basic_string<llwchar>::size_type ii = 0; ii < len; ++ii)
		{
			if((string[ii] < MIN) || (string[ii] > MAX))
			{
				string[ii] = replacement;
			}
		}
	}

	void replace_nonprintable_and_pipe(std::basic_string<char>& str,
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

	void replace_nonprintable_and_pipe(std::basic_string<llwchar>& str,
									   llwchar replacement)
	{
		const llwchar MIN  = 0x20;
		const llwchar MAX  = 0x7f;
		const llwchar PIPE = 0x7c;
		std::basic_string<llwchar>::size_type len = str.size();
		for(std::basic_string<llwchar>::size_type ii = 0; ii < len; ++ii)
		{
			if( (str[ii] < MIN) || (str[ii] > MAX) || (str[ii] == PIPE) )
			{
				str[ii] = replacement;
			}
		}
	}
}


////////////////////////////////////////////////////////////
// Testing

#ifdef _DEBUG

template<class T> 
void LLStringBase<T>::testHarness()
{
	LLString s1;
	
	llassert( s1.c_str() == NULL );
	llassert( s1.size() == 0 );
	llassert( s1.empty() );
	
	LLString s2( "hello");
	llassert( !strcmp( s2.c_str(), "hello" ) );
	llassert( s2.size() == 5 ); 
	llassert( !s2.empty() );
	LLString s3( s2 );

	llassert( "hello" == s2 );
	llassert( s2 == "hello" );
	llassert( s2 > "gello" );
	llassert( "gello" < s2 );
	llassert( "gello" != s2 );
	llassert( s2 != "gello" );

	LLString s4 = s2;
	llassert( !s4.empty() );
	s4.empty();
	llassert( s4.empty() );
	
	LLString s5("");
	llassert( s5.empty() );
	
	llassert( isValidIndex(s5, 0) );
	llassert( !isValidIndex(s5, 1) );
	
	s3 = s2;
	s4 = "hello again";
	
	s4 += "!";
	s4 += s4;
	llassert( s4 == "hello again!hello again!" );
	
	
	LLString s6 = s2 + " " + s2;
	LLString s7 = s6;
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
	
	LLString s8 = s2.substr( 6, 5 );
	llassert( s8 == "don't"  );
	
	LLString s9 = "   \t\ntest  \t\t\n  ";
	trim(s9);
	llassert( s9 == "test"  );

	s8 = "abc123&*(ABC";

	s9 = s8;
	toUpper(s9);
	llassert( s9 == "ABC123&*(ABC"  );

	s9 = s8;
	toLower(s9);
	llassert( s9 == "abc123&*(abc"  );


	LLString s10( 10, 'x' );
	llassert( s10 == "xxxxxxxxxx" );

	LLString s11( "monkey in the middle", 7, 2 );
	llassert( s11 == "in" );

	LLString s12;  //empty
	s12 += "foo";
	llassert( s12 == "foo" );

	LLString s13;  //empty
	s13 += 'f';
	llassert( s13 == "f" );
}


#endif  // _DEBUG
