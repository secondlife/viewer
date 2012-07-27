/** 
 * @file llstring.h
 * @brief String utility functions and std::string class.
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

#ifndef LL_LLSTRING_H
#define LL_LLSTRING_H

#include <string>
#include <cstdio>
#include <locale>
#include <iomanip>
#include "llsd.h"
#include "llfasttimer.h"

#if LL_LINUX || LL_SOLARIS
#include <wctype.h>
#include <wchar.h>
#endif

#include <string.h>
#include <boost/scoped_ptr.hpp>

#if LL_SOLARIS
// stricmp and strnicmp do not exist on Solaris:
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

const char LL_UNKNOWN_CHAR = '?';

#if LL_DARWIN || LL_LINUX || LL_SOLARIS
// Template specialization of char_traits for U16s. Only necessary on Mac and Linux (exists on Windows already)
#include <cstring>

namespace std
{
template<>
struct char_traits<U16>
{
	typedef U16 		char_type;
	typedef int 	    int_type;
	typedef streampos 	pos_type;
	typedef streamoff 	off_type;
	typedef mbstate_t 	state_type;
	
	static void 
		assign(char_type& __c1, const char_type& __c2)
	{ __c1 = __c2; }
	
	static bool 
		eq(const char_type& __c1, const char_type& __c2)
	{ return __c1 == __c2; }
	
	static bool 
		lt(const char_type& __c1, const char_type& __c2)
	{ return __c1 < __c2; }
	
	static int 
		compare(const char_type* __s1, const char_type* __s2, size_t __n)
	{ return memcmp(__s1, __s2, __n * sizeof(char_type)); }
	
	static size_t
		length(const char_type* __s)
	{
		const char_type *cur_char = __s;
		while (*cur_char != 0)
		{
			++cur_char;
		}
		return cur_char - __s;
	}
	
	static const char_type* 
		find(const char_type* __s, size_t __n, const char_type& __a)
	{ return static_cast<const char_type*>(memchr(__s, __a, __n * sizeof(char_type))); }
	
	static char_type* 
		move(char_type* __s1, const char_type* __s2, size_t __n)
	{ return static_cast<char_type*>(memmove(__s1, __s2, __n * sizeof(char_type))); }
	
	static char_type* 
		copy(char_type* __s1, const char_type* __s2, size_t __n)
	{  return static_cast<char_type*>(memcpy(__s1, __s2, __n * sizeof(char_type))); }	/* Flawfinder: ignore */
	
	static char_type* 
		assign(char_type* __s, size_t __n, char_type __a)
	{ 
		// This isn't right.
		//return static_cast<char_type*>(memset(__s, __a, __n * sizeof(char_type))); 
		
		// I don't think there's a standard 'memset' for 16-bit values.
		// Do this the old-fashioned way.
		
		size_t __i;
		for(__i = 0; __i < __n; __i++)
		{
			__s[__i] = __a;
		}
		return __s; 
	}
	
	static char_type 
		to_char_type(const int_type& __c)
	{ return static_cast<char_type>(__c); }
	
	static int_type 
		to_int_type(const char_type& __c)
	{ return static_cast<int_type>(__c); }
	
	static bool 
		eq_int_type(const int_type& __c1, const int_type& __c2)
	{ return __c1 == __c2; }
	
	static int_type 
		eof() { return static_cast<int_type>(EOF); }
	
	static int_type 
		not_eof(const int_type& __c)
      { return (__c == eof()) ? 0 : __c; }
  };
};
#endif

class LL_COMMON_API LLStringOps
{
private:
	static long sPacificTimeOffset;
	static long sLocalTimeOffset;
	static bool sPacificDaylightTime;

	static std::map<std::string, std::string> datetimeToCodes;

public:
	static std::vector<std::string> sWeekDayList;
	static std::vector<std::string> sWeekDayShortList;
	static std::vector<std::string> sMonthList;
	static std::vector<std::string> sMonthShortList;
	static std::string sDayFormat;

	static std::string sAM;
	static std::string sPM;

	static char toUpper(char elem) { return toupper((unsigned char)elem); }
	static llwchar toUpper(llwchar elem) { return towupper(elem); }
	
	static char toLower(char elem) { return tolower((unsigned char)elem); }
	static llwchar toLower(llwchar elem) { return towlower(elem); }

	static bool isSpace(char elem) { return isspace((unsigned char)elem) != 0; }
	static bool isSpace(llwchar elem) { return iswspace(elem) != 0; }

	static bool isUpper(char elem) { return isupper((unsigned char)elem) != 0; }
	static bool isUpper(llwchar elem) { return iswupper(elem) != 0; }

	static bool isLower(char elem) { return islower((unsigned char)elem) != 0; }
	static bool isLower(llwchar elem) { return iswlower(elem) != 0; }

	static bool isDigit(char a) { return isdigit((unsigned char)a) != 0; }
	static bool isDigit(llwchar a) { return iswdigit(a) != 0; }

	static bool isPunct(char a) { return ispunct((unsigned char)a) != 0; }
	static bool isPunct(llwchar a) { return iswpunct(a) != 0; }

	static bool isAlpha(char a) { return isalpha((unsigned char)a) != 0; }
	static bool isAlpha(llwchar a) { return iswalpha(a) != 0; }

	static bool isAlnum(char a) { return isalnum((unsigned char)a) != 0; }
	static bool isAlnum(llwchar a) { return iswalnum(a) != 0; }

	static S32	collate(const char* a, const char* b) { return strcoll(a, b); }
	static S32	collate(const llwchar* a, const llwchar* b);

	static void setupDatetimeInfo(bool pacific_daylight_time);

	static void setupWeekDaysNames(const std::string& data);
	static void setupWeekDaysShortNames(const std::string& data);
	static void setupMonthNames(const std::string& data);
	static void setupMonthShortNames(const std::string& data);
	static void setupDayFormat(const std::string& data);


	static long getPacificTimeOffset(void) { return sPacificTimeOffset;}
	static long getLocalTimeOffset(void) { return sLocalTimeOffset;}
	// Is the Pacific time zone (aka server time zone)
	// currently in daylight savings time?
	static bool getPacificDaylightTime(void) { return sPacificDaylightTime;}

	static std::string getDatetimeCode (std::string key);
};

/**
 * @brief Return a string constructed from in without crashing if the
 * pointer is NULL.
 */
LL_COMMON_API std::string ll_safe_string(const char* in);
LL_COMMON_API std::string ll_safe_string(const char* in, S32 maxlen);


// Allowing assignments from non-strings into format_map_t is apparently
// *really* error-prone, so subclass std::string with just basic c'tors.
class LLFormatMapString
{
public:
	LLFormatMapString() {};
	LLFormatMapString(const char* s) : mString(ll_safe_string(s)) {};
	LLFormatMapString(const std::string& s) : mString(s) {};
	operator std::string() const { return mString; }
	bool operator<(const LLFormatMapString& rhs) const { return mString < rhs.mString; }
	std::size_t length() const { return mString.length(); }
	
private:
	std::string mString;
};

template <class T>
class LLStringUtilBase
{
private:
	static std::string sLocale;

public:
	typedef std::basic_string<T> string_type;
	typedef typename string_type::size_type size_type;
	
public:
	/////////////////////////////////////////////////////////////////////////////////////////
	// Static Utility functions that operate on std::strings

	static const string_type null;
	
	typedef std::map<LLFormatMapString, LLFormatMapString> format_map_t;
	/// considers any sequence of delims as a single field separator
	LL_COMMON_API static void getTokens(const string_type& instr,
										std::vector<string_type >& tokens,
										const string_type& delims);
	/// like simple scan overload, but returns scanned vector
	static std::vector<string_type> getTokens(const string_type& instr,
											  const string_type& delims);
	/// add support for keep_delims and quotes (either could be empty string)
	static void getTokens(const string_type& instr,
						  std::vector<string_type>& tokens,
						  const string_type& drop_delims,
						  const string_type& keep_delims,
						  const string_type& quotes=string_type());
	/// like keep_delims-and-quotes overload, but returns scanned vector
	static std::vector<string_type> getTokens(const string_type& instr,
											  const string_type& drop_delims,
											  const string_type& keep_delims,
											  const string_type& quotes=string_type());
	/// add support for escapes (could be empty string)
	static void getTokens(const string_type& instr,
						  std::vector<string_type>& tokens,
						  const string_type& drop_delims,
						  const string_type& keep_delims,
						  const string_type& quotes,
						  const string_type& escapes);
	/// like escapes overload, but returns scanned vector
	static std::vector<string_type> getTokens(const string_type& instr,
											  const string_type& drop_delims,
											  const string_type& keep_delims,
											  const string_type& quotes,
											  const string_type& escapes);

	LL_COMMON_API static void formatNumber(string_type& numStr, string_type decimals);
	LL_COMMON_API static bool formatDatetime(string_type& replacement, string_type token, string_type param, S32 secFromEpoch);
	LL_COMMON_API static S32 format(string_type& s, const format_map_t& substitutions);
	LL_COMMON_API static S32 format(string_type& s, const LLSD& substitutions);
	LL_COMMON_API static bool simpleReplacement(string_type& replacement, string_type token, const format_map_t& substitutions);
	LL_COMMON_API static bool simpleReplacement(string_type& replacement, string_type token, const LLSD& substitutions);
	LL_COMMON_API static void setLocale (std::string inLocale);
	LL_COMMON_API static std::string getLocale (void);
	
	static bool isValidIndex(const string_type& string, size_type i)
	{
		return !string.empty() && (0 <= i) && (i <= string.size());
	}

	static bool contains(const string_type& string, T c, size_type i=0)
	{
		return string.find(c, i) != string_type::npos;
	}

	static void	trimHead(string_type& string);
	static void	trimTail(string_type& string);
	static void	trim(string_type& string)	{ trimHead(string); trimTail(string); }
	static void truncate(string_type& string, size_type count);

	static void	toUpper(string_type& string);
	static void	toLower(string_type& string);
	
	// True if this is the head of s.
	static BOOL	isHead( const string_type& string, const T* s ); 

	/**
	 * @brief Returns true if string starts with substr
	 *
	 * If etither string or substr are empty, this method returns false.
	 */
	static bool startsWith(
		const string_type& string,
		const string_type& substr);

	/**
	 * @brief Returns true if string ends in substr
	 *
	 * If etither string or substr are empty, this method returns false.
	 */
	static bool endsWith(
		const string_type& string,
		const string_type& substr);

	static void	addCRLF(string_type& string);
	static void	removeCRLF(string_type& string);

	static void	replaceTabsWithSpaces( string_type& string, size_type spaces_per_tab );
	static void	replaceNonstandardASCII( string_type& string, T replacement );
	static void	replaceChar( string_type& string, T target, T replacement );
	static void replaceString( string_type& string, string_type target, string_type replacement );
	
	static BOOL	containsNonprintable(const string_type& string);
	static void	stripNonprintable(string_type& string);

	/**
	 * Double-quote an argument string if needed, unless it's already
	 * double-quoted. Decide whether it's needed based on the presence of any
	 * character in @a triggers (default space or double-quote). If we quote
	 * it, escape any embedded double-quote with the @a escape string (default
	 * backslash).
	 *
	 * Passing triggers="" means always quote, unless it's already double-quoted.
	 */
	static string_type quote(const string_type& str,
							 const string_type& triggers=" \"",
							 const string_type& escape="\\");

	/**
	 * @brief Unsafe way to make ascii characters. You should probably
	 * only call this when interacting with the host operating system.
	 * The 1 byte std::string does not work correctly.
	 * The 2 and 4 byte std::string probably work, so LLWStringUtil::_makeASCII
	 * should work.
	 */
	static void _makeASCII(string_type& string);

	// Conversion to other data types
	static BOOL	convertToBOOL(const string_type& string, BOOL& value);
	static BOOL	convertToU8(const string_type& string, U8& value);
	static BOOL	convertToS8(const string_type& string, S8& value);
	static BOOL	convertToS16(const string_type& string, S16& value);
	static BOOL	convertToU16(const string_type& string, U16& value);
	static BOOL	convertToU32(const string_type& string, U32& value);
	static BOOL	convertToS32(const string_type& string, S32& value);
	static BOOL	convertToF32(const string_type& string, F32& value);
	static BOOL	convertToF64(const string_type& string, F64& value);

	/////////////////////////////////////////////////////////////////////////////////////////
	// Utility functions for working with char*'s and strings

	// Like strcmp but also handles empty strings. Uses
	// current locale.
	static S32		compareStrings(const T* lhs, const T* rhs);
	static S32		compareStrings(const string_type& lhs, const string_type& rhs);
	
	// case insensitive version of above. Uses current locale on
	// Win32, and falls back to a non-locale aware comparison on
	// Linux.
	static S32		compareInsensitive(const T* lhs, const T* rhs);
	static S32		compareInsensitive(const string_type& lhs, const string_type& rhs);

	// Case sensitive comparison with good handling of numbers.  Does not use current locale.
	// a.k.a. strdictcmp()
	static S32		compareDict(const string_type& a, const string_type& b);

	// Case *in*sensitive comparison with good handling of numbers.  Does not use current locale.
	// a.k.a. strdictcmp()
	static S32		compareDictInsensitive(const string_type& a, const string_type& b);

	// Puts compareDict() in a form appropriate for LL container classes to use for sorting.
	static BOOL		precedesDict( const string_type& a, const string_type& b );

	// A replacement for strncpy.
	// If the dst buffer is dst_size bytes long or more, ensures that dst is null terminated and holds
	// up to dst_size-1 characters of src.
	static void		copy(T* dst, const T* src, size_type dst_size);
	
	// Copies src into dst at a given offset.  
	static void		copyInto(string_type& dst, const string_type& src, size_type offset);
	
	static bool		isPartOfWord(T c) { return (c == (T)'_') || LLStringOps::isAlnum(c); }


#ifdef _DEBUG	
	LL_COMMON_API static void		testHarness();
#endif

private:
	LL_COMMON_API static size_type getSubstitution(const string_type& instr, size_type& start, std::vector<string_type >& tokens);
};

template<class T> const std::basic_string<T> LLStringUtilBase<T>::null;
template<class T> std::string LLStringUtilBase<T>::sLocale;

typedef LLStringUtilBase<char> LLStringUtil;
typedef LLStringUtilBase<llwchar> LLWStringUtil;
typedef std::basic_string<llwchar> LLWString;

//@ Use this where we want to disallow input in the form of "foo"
//  This is used to catch places where english text is embedded in the code
//  instead of in a translatable XUI file.
class LLStringExplicit : public std::string
{
public:
	explicit LLStringExplicit(const char* s) : std::string(s) {}
	LLStringExplicit(const std::string& s) : std::string(s) {}
	LLStringExplicit(const std::string& s, size_type pos, size_type n = std::string::npos) : std::string(s, pos, n) {}
};

struct LLDictionaryLess
{
public:
	bool operator()(const std::string& a, const std::string& b)
	{
		return (LLStringUtil::precedesDict(a, b) ? true : false);
	}
};


/**
 * Simple support functions
 */

/**
 * @brief chop off the trailing characters in a string.
 *
 * This function works on bytes rather than glyphs, so this will
 * incorrectly truncate non-single byte strings.
 * Use utf8str_truncate() for utf8 strings
 * @return a copy of in string minus the trailing count bytes.
 */
inline std::string chop_tail_copy(
	const std::string& in,
	std::string::size_type count)
{
	return std::string(in, 0, in.length() - count);
}

/**
 * @brief This translates a nybble stored as a hex value from 0-f back
 * to a nybble in the low order bits of the return byte.
 */
LL_COMMON_API U8 hex_as_nybble(char hex);

/**
 * @brief read the contents of a file into a string.
 *
 * Since this function has no concept of character encoding, most
 * anything you do with this method ill-advised. Please avoid.
 * @param str [out] The string which will have.
 * @param filename The full name of the file to read.
 * @return Returns true on success. If false, str is unmodified.
 */
LL_COMMON_API bool _read_file_into_string(std::string& str, const std::string& filename);
LL_COMMON_API bool iswindividual(llwchar elem);

/**
 * Unicode support
 */

// Make the incoming string a utf8 string. Replaces any unknown glyph
// with the UNKNOWN_CHARACTER. Once any unknown glyph is found, the rest
// of the data may not be recovered.
LL_COMMON_API std::string rawstr_to_utf8(const std::string& raw);

//
// We should never use UTF16 except when communicating with Win32!
//
typedef std::basic_string<U16> llutf16string;

LL_COMMON_API LLWString utf16str_to_wstring(const llutf16string &utf16str, S32 len);
LL_COMMON_API LLWString utf16str_to_wstring(const llutf16string &utf16str);

LL_COMMON_API llutf16string wstring_to_utf16str(const LLWString &utf32str, S32 len);
LL_COMMON_API llutf16string wstring_to_utf16str(const LLWString &utf32str);

LL_COMMON_API llutf16string utf8str_to_utf16str ( const std::string& utf8str, S32 len);
LL_COMMON_API llutf16string utf8str_to_utf16str ( const std::string& utf8str );

LL_COMMON_API LLWString utf8str_to_wstring(const std::string &utf8str, S32 len);
LL_COMMON_API LLWString utf8str_to_wstring(const std::string &utf8str);
// Same function, better name. JC
inline LLWString utf8string_to_wstring(const std::string& utf8_string) { return utf8str_to_wstring(utf8_string); }

//
LL_COMMON_API S32 wchar_to_utf8chars(llwchar inchar, char* outchars);

LL_COMMON_API std::string wstring_to_utf8str(const LLWString &utf32str, S32 len);
LL_COMMON_API std::string wstring_to_utf8str(const LLWString &utf32str);

LL_COMMON_API std::string utf16str_to_utf8str(const llutf16string &utf16str, S32 len);
LL_COMMON_API std::string utf16str_to_utf8str(const llutf16string &utf16str);

// Length of this UTF32 string in bytes when transformed to UTF8
LL_COMMON_API S32 wstring_utf8_length(const LLWString& wstr); 

// Length in bytes of this wide char in a UTF8 string
LL_COMMON_API S32 wchar_utf8_length(const llwchar wc); 

LL_COMMON_API std::string utf8str_tolower(const std::string& utf8str);

// Length in llwchar (UTF-32) of the first len units (16 bits) of the given UTF-16 string.
LL_COMMON_API S32 utf16str_wstring_length(const llutf16string &utf16str, S32 len);

// Length in utf16string (UTF-16) of wlen wchars beginning at woffset.
LL_COMMON_API S32 wstring_utf16_length(const LLWString & wstr, S32 woffset, S32 wlen);

// Length in wstring (i.e., llwchar count) of a part of a wstring specified by utf16 length (i.e., utf16 units.)
LL_COMMON_API S32 wstring_wstring_length_from_utf16_length(const LLWString & wstr, S32 woffset, S32 utf16_length, BOOL *unaligned = NULL);

/**
 * @brief Properly truncate a utf8 string to a maximum byte count.
 * 
 * The returned string may be less than max_len if the truncation
 * happens in the middle of a glyph. If max_len is longer than the
 * string passed in, the return value == utf8str.
 * @param utf8str A valid utf8 string to truncate.
 * @param max_len The maximum number of bytes in the return value.
 * @return Returns a valid utf8 string with byte count <= max_len.
 */
LL_COMMON_API std::string utf8str_truncate(const std::string& utf8str, const S32 max_len);

LL_COMMON_API std::string utf8str_trim(const std::string& utf8str);

LL_COMMON_API S32 utf8str_compare_insensitive(
	const std::string& lhs,
	const std::string& rhs);

/**
 * @brief Replace all occurences of target_char with replace_char
 *
 * @param utf8str A utf8 string to process.
 * @param target_char The wchar to be replaced
 * @param replace_char The wchar which is written on replace
 */
LL_COMMON_API std::string utf8str_substChar(
	const std::string& utf8str,
	const llwchar target_char,
	const llwchar replace_char);

LL_COMMON_API std::string utf8str_makeASCII(const std::string& utf8str);

// Hack - used for evil notecards.
LL_COMMON_API std::string mbcsstring_makeASCII(const std::string& str); 

LL_COMMON_API std::string utf8str_removeCRLF(const std::string& utf8str);


#if LL_WINDOWS
/* @name Windows string helpers
 */
//@{

/**
 * @brief Implementation the expected snprintf interface.
 *
 * If the size of the passed in buffer is not large enough to hold the string,
 * two bad things happen:
 * 1. resulting formatted string is NOT null terminated
 * 2. Depending on the platform, the return value could be a) the required
 *    size of the buffer to copy the entire formatted string or b) -1.
 *    On Windows with VS.Net 2003, it returns -1 e.g. 
 *
 * safe_snprintf always adds a NULL terminator so that the caller does not
 * need to check for return value or need to add the NULL terminator.
 * It does not, however change the return value - to let the caller know
 * that the passed in buffer size was not large enough to hold the
 * formatted string.
 *
 */

// Deal with the differeneces on Windows
namespace snprintf_hack
{
	LL_COMMON_API int snprintf(char *str, size_t size, const char *format, ...);
}

using snprintf_hack::snprintf;

/**
 * @brief Convert a wide string to std::string
 *
 * This replaces the unsafe W2A macro from ATL.
 */
LL_COMMON_API std::string ll_convert_wide_to_string(const wchar_t* in, unsigned int code_page);

/**
 * Converts a string to wide string.
 *
 * It will allocate memory for result string with "new []". Don't forget to release it with "delete []".
 */
LL_COMMON_API wchar_t* ll_convert_string_to_wide(const std::string& in, unsigned int code_page);

/**
 * Converts incoming string into urf8 string
 *
 */
LL_COMMON_API std::string ll_convert_string_to_utf8_string(const std::string& in);

//@}
#endif // LL_WINDOWS

/**
 * Many of the 'strip' and 'replace' methods of LLStringUtilBase need
 * specialization to work with the signed char type.
 * Sadly, it is not possible (AFAIK) to specialize a single method of
 * a template class.
 * That stuff should go here.
 */
namespace LLStringFn
{
	/**
	 * @brief Replace all non-printable characters with replacement in
	 * string.
	 * NOTE - this will zap non-ascii
	 *
	 * @param [in,out] string the to modify. out value is the string
	 * with zero non-printable characters.
	 * @param The replacement character. use LL_UNKNOWN_CHAR if unsure.
	 */
	LL_COMMON_API void replace_nonprintable_in_ascii(
		std::basic_string<char>& string,
		char replacement);


	/**
	 * @brief Replace all non-printable characters and pipe characters
	 * with replacement in a string.
	 * NOTE - this will zap non-ascii
	 *
	 * @param [in,out] the string to modify. out value is the string
	 * with zero non-printable characters and zero pipe characters.
	 * @param The replacement character. use LL_UNKNOWN_CHAR if unsure.
	 */
	LL_COMMON_API void replace_nonprintable_and_pipe_in_ascii(std::basic_string<char>& str,
									   char replacement);


	/**
	 * @brief Remove all characters that are not allowed in XML 1.0.
	 * Returns a copy of the string with those characters removed.
	 * Works with US ASCII and UTF-8 encoded strings.  JC
	 */
	LL_COMMON_API std::string strip_invalid_xml(const std::string& input);


	/**
	 * @brief Replace all control characters (0 <= c < 0x20) with replacement in
	 * string.   This is safe for utf-8
	 *
	 * @param [in,out] string the to modify. out value is the string
	 * with zero non-printable characters.
	 * @param The replacement character. use LL_UNKNOWN_CHAR if unsure.
	 */
	LL_COMMON_API void replace_ascii_controlchars(
		std::basic_string<char>& string,
		char replacement);
}

////////////////////////////////////////////////////////////
// NOTE: LLStringUtil::format, getTokens, and support functions moved to llstring.cpp.
// There is no LLWStringUtil::format implementation currently.
// Calling these for anything other than LLStringUtil will produce link errors.

////////////////////////////////////////////////////////////

// static
template <class T>
std::vector<typename LLStringUtilBase<T>::string_type>
LLStringUtilBase<T>::getTokens(const string_type& instr, const string_type& delims)
{
	std::vector<string_type> tokens;
	getTokens(instr, tokens, delims);
	return tokens;
}

// static
template <class T>
std::vector<typename LLStringUtilBase<T>::string_type>
LLStringUtilBase<T>::getTokens(const string_type& instr,
							   const string_type& drop_delims,
							   const string_type& keep_delims,
							   const string_type& quotes)
{
	std::vector<string_type> tokens;
	getTokens(instr, tokens, drop_delims, keep_delims, quotes);
	return tokens;
}

// static
template <class T>
std::vector<typename LLStringUtilBase<T>::string_type>
LLStringUtilBase<T>::getTokens(const string_type& instr,
							   const string_type& drop_delims,
							   const string_type& keep_delims,
							   const string_type& quotes,
							   const string_type& escapes)
{
	std::vector<string_type> tokens;
	getTokens(instr, tokens, drop_delims, keep_delims, quotes, escapes);
	return tokens;
}

namespace LLStringUtilBaseImpl
{

/**
 * Input string scanner helper for getTokens(), or really any other
 * character-parsing routine that may have to deal with escape characters.
 * This implementation defines the concept (also an interface, should you
 * choose to implement the concept by subclassing) and provides trivial
 * implementations for a string @em without escape processing.
 */
template <class T>
struct InString
{
	typedef std::basic_string<T> string_type;
	typedef typename string_type::const_iterator const_iterator;

	InString(const_iterator b, const_iterator e):
		mIter(b),
		mEnd(e)
	{}
	virtual ~InString() {}

	bool done() const { return mIter == mEnd; }
	/// Is the current character (*mIter) escaped? This implementation can
	/// answer trivially because it doesn't support escapes.
	virtual bool escaped() const { return false; }
	/// Obtain the current character and advance @c mIter.
	virtual T next() { return *mIter++; }
	/// Does the current character match specified character?
	virtual bool is(T ch) const { return (! done()) && *mIter == ch; }
	/// Is the current character any one of the specified characters?
	virtual bool oneof(const string_type& delims) const
	{
		return (! done()) && LLStringUtilBase<T>::contains(delims, *mIter);
	}

	/**
	 * Scan forward from @from until either @a delim or end. This is primarily
	 * useful for processing quoted substrings.
	 *
	 * If we do see @a delim, append everything from @from until (excluding)
	 * @a delim to @a into, advance @c mIter to skip @a delim, and return @c
	 * true.
	 *
	 * If we do not see @a delim, do not alter @a into or @c mIter and return
	 * @c false. Do not pass GO, do not collect $200.
	 *
	 * @note The @c false case described above implements normal getTokens()
	 * treatment of an unmatched open quote: treat the quote character as if
	 * escaped, that is, simply collect it as part of the current token. Other
	 * plausible behaviors directly affect the way getTokens() deals with an
	 * unmatched quote: e.g. throwing an exception to treat it as an error, or
	 * assuming a close quote beyond end of string (in which case return @c
	 * true).
	 */
	virtual bool collect_until(string_type& into, const_iterator from, T delim)
	{
		const_iterator found = std::find(from, mEnd, delim);
		// If we didn't find delim, change nothing, just tell caller.
		if (found == mEnd)
			return false;
		// Found delim! Append everything between from and found.
		into.append(from, found);
		// advance past delim in input
		mIter = found + 1;
		return true;
	}

	const_iterator mIter, mEnd;
};

/// InString subclass that handles escape characters
template <class T>
class InEscString: public InString<T>
{
public:
	typedef InString<T> super;
	typedef typename super::string_type string_type;
	typedef typename super::const_iterator const_iterator;
	using super::done;
	using super::mIter;
	using super::mEnd;

	InEscString(const_iterator b, const_iterator e, const string_type& escapes):
		super(b, e),
		mEscapes(escapes)
	{
		// Even though we've already initialized 'mIter' via our base-class
		// constructor, set it again to check for initial escape char.
		setiter(b);
	}

	/// This implementation uses the answer cached by setiter().
	virtual bool escaped() const { return mIsEsc; }
	virtual T next()
	{
		// If we're looking at the escape character of an escape sequence,
		// skip that character. This is the one time we can modify 'mIter'
		// without using setiter: for this one case we DO NOT CARE if the
		// escaped character is itself an escape.
		if (mIsEsc)
			++mIter;
		// If we were looking at an escape character, this is the escaped
		// character; otherwise it's just the next character.
		T result(*mIter);
		// Advance mIter, checking for escape sequence.
		setiter(mIter + 1);
		return result;
	}

	virtual bool is(T ch) const
	{
		// Like base-class is(), except that an escaped character matches
		// nothing.
		return (! done()) && (! mIsEsc) && *mIter == ch;
	}

	virtual bool oneof(const string_type& delims) const
	{
		// Like base-class oneof(), except that an escaped character matches
		// nothing.
		return (! done()) && (! mIsEsc) && LLStringUtilBase<T>::contains(delims, *mIter);
	}

	virtual bool collect_until(string_type& into, const_iterator from, T delim)
	{
		// Deal with escapes in the characters we collect; that is, an escaped
		// character must become just that character without the preceding
		// escape. Collect characters in a separate string rather than
		// directly appending to 'into' in case we do not find delim, in which
		// case we're supposed to leave 'into' unmodified.
		string_type collected;
		// For scanning purposes, we're going to work directly with 'mIter'.
		// Save its current value in case we fail to see delim.
		const_iterator save_iter(mIter);
		// Okay, set 'mIter', checking for escape.
		setiter(from);
		while (! done())
		{
			// If we see an unescaped delim, stop and report success.
			if ((! mIsEsc) && *mIter == delim)
			{
				// Append collected chars to 'into'.
				into.append(collected);
				// Don't forget to advance 'mIter' past delim.
				setiter(mIter + 1);
				return true;
			}
			// We're not at end, and either we're not looking at delim or it's
			// escaped. Collect this character and keep going.
			collected.push_back(next());
		}
		// Here we hit 'mEnd' without ever seeing delim. Restore mIter and tell
		// caller.
		setiter(save_iter);
		return false;
	}

private:
	void setiter(const_iterator i)
	{
		mIter = i;

		// Every time we change 'mIter', set 'mIsEsc' to be able to repetitively
		// answer escaped() without having to rescan 'mEscapes'. mIsEsc caches
		// contains(mEscapes, *mIter).

		// We're looking at an escaped char if we're not already at end (that
		// is, *mIter is even meaningful); if *mIter is in fact one of the
		// specified escape characters; and if there's one more character
		// following it. That is, if an escape character is the very last
		// character of the input string, it loses its special meaning.
		mIsEsc = (! done()) &&
				LLStringUtilBase<T>::contains(mEscapes, *mIter) &&
				(mIter+1) != mEnd;
	}

	const string_type mEscapes;
	bool mIsEsc;
};

/// getTokens() implementation based on InString concept
template <typename INSTRING, typename string_type>
void getTokens(INSTRING& instr, std::vector<string_type>& tokens,
			   const string_type& drop_delims, const string_type& keep_delims,
			   const string_type& quotes)
{
	// There are times when we want to match either drop_delims or
	// keep_delims. Concatenate them up front to speed things up.
	string_type all_delims(drop_delims + keep_delims);
	// no tokens yet
	tokens.clear();

	// try for another token
	while (! instr.done())
	{
		// scan past any drop_delims
		while (instr.oneof(drop_delims))
		{
			// skip this drop_delim
			instr.next();
			// but if that was the end of the string, done
			if (instr.done())
				return;
		}
		// found the start of another token: make a slot for it
		tokens.push_back(string_type());
		if (instr.oneof(keep_delims))
		{
			// *iter is a keep_delim, a token of exactly 1 character. Append
			// that character to the new token and proceed.
			tokens.back().push_back(instr.next());
			continue;
		}
		// Here we have a non-delimiter token, which might consist of a mix of
		// quoted and unquoted parts. Use bash rules for quoting: you can
		// embed a quoted substring in the midst of an unquoted token (e.g.
		// ~/"sub dir"/myfile.txt); you can ram two quoted substrings together
		// to make a single token (e.g. 'He said, "'"Don't."'"'). We diverge
		// from bash in that bash considers an unmatched quote an error. Our
		// param signature doesn't allow for errors, so just pretend it's not
		// a quote and embed it.
		// At this level, keep scanning until we hit the next delimiter of
		// either type (drop_delims or keep_delims).
		while (! instr.oneof(all_delims))
		{
			// If we're looking at an open quote, search forward for
			// a close quote, collecting characters along the way.
			if (instr.oneof(quotes) &&
				instr.collect_until(tokens.back(), instr.mIter+1, *instr.mIter))
			{
				// collect_until is cleverly designed to do exactly what we
				// need here. No further action needed if it returns true.
			}
			else
			{
				// Either *iter isn't a quote, or there's no matching close
				// quote: in other words, just an ordinary char. Append it to
				// current token.
				tokens.back().push_back(instr.next());
			}
			// having scanned that segment of this token, if we've reached the
			// end of the string, we're done
			if (instr.done())
				return;
		}
	}
}

} // namespace LLStringUtilBaseImpl

// static
template <class T>
void LLStringUtilBase<T>::getTokens(const string_type& string, std::vector<string_type>& tokens,
									const string_type& drop_delims, const string_type& keep_delims,
									const string_type& quotes)
{
	// Because this overload doesn't support escapes, use simple InString to
	// manage input range.
	LLStringUtilBaseImpl::InString<T> instring(string.begin(), string.end());
	LLStringUtilBaseImpl::getTokens(instring, tokens, drop_delims, keep_delims, quotes);
}

// static
template <class T>
void LLStringUtilBase<T>::getTokens(const string_type& string, std::vector<string_type>& tokens,
									const string_type& drop_delims, const string_type& keep_delims,
									const string_type& quotes, const string_type& escapes)
{
	// This overload must deal with escapes. Delegate that to InEscString
	// (unless there ARE no escapes).
	boost::scoped_ptr< LLStringUtilBaseImpl::InString<T> > instrp;
	if (escapes.empty())
		instrp.reset(new LLStringUtilBaseImpl::InString<T>(string.begin(), string.end()));
	else
		instrp.reset(new LLStringUtilBaseImpl::InEscString<T>(string.begin(), string.end(), escapes));
	LLStringUtilBaseImpl::getTokens(*instrp, tokens, drop_delims, keep_delims, quotes);
}

// static
template<class T> 
S32 LLStringUtilBase<T>::compareStrings(const T* lhs, const T* rhs)
{	
	S32 result;
	if( lhs == rhs )
	{
		result = 0;
	}
	else
	if ( !lhs || !lhs[0] )
	{
		result = ((!rhs || !rhs[0]) ? 0 : 1);
	}
	else
	if ( !rhs || !rhs[0])
	{
		result = -1;
	}
	else
	{
		result = LLStringOps::collate(lhs, rhs);
	}
	return result;
}

//static 
template<class T> 
S32 LLStringUtilBase<T>::compareStrings(const string_type& lhs, const string_type& rhs)
{
	return LLStringOps::collate(lhs.c_str(), rhs.c_str());
}

// static
template<class T> 
S32 LLStringUtilBase<T>::compareInsensitive(const T* lhs, const T* rhs )
{
	S32 result;
	if( lhs == rhs )
	{
		result = 0;
	}
	else
	if ( !lhs || !lhs[0] )
	{
		result = ((!rhs || !rhs[0]) ? 0 : 1);
	}
	else
	if ( !rhs || !rhs[0] )
	{
		result = -1;
	}
	else
	{
		string_type lhs_string(lhs);
		string_type rhs_string(rhs);
		LLStringUtilBase<T>::toUpper(lhs_string);
		LLStringUtilBase<T>::toUpper(rhs_string);
		result = LLStringOps::collate(lhs_string.c_str(), rhs_string.c_str());
	}
	return result;
}

//static 
template<class T> 
S32 LLStringUtilBase<T>::compareInsensitive(const string_type& lhs, const string_type& rhs)
{
	string_type lhs_string(lhs);
	string_type rhs_string(rhs);
	LLStringUtilBase<T>::toUpper(lhs_string);
	LLStringUtilBase<T>::toUpper(rhs_string);
	return LLStringOps::collate(lhs_string.c_str(), rhs_string.c_str());
}

// Case sensitive comparison with good handling of numbers.  Does not use current locale.
// a.k.a. strdictcmp()

//static 
template<class T>
S32 LLStringUtilBase<T>::compareDict(const string_type& astr, const string_type& bstr)
{
	const T* a = astr.c_str();
	const T* b = bstr.c_str();
	T ca, cb;
	S32 ai, bi, cnt = 0;
	S32 bias = 0;

	ca = *(a++);
	cb = *(b++);
	while( ca && cb ){
		if( bias==0 ){
			if( LLStringOps::isUpper(ca) ){ ca = LLStringOps::toLower(ca); bias--; }
			if( LLStringOps::isUpper(cb) ){ cb = LLStringOps::toLower(cb); bias++; }
		}else{
			if( LLStringOps::isUpper(ca) ){ ca = LLStringOps::toLower(ca); }
			if( LLStringOps::isUpper(cb) ){ cb = LLStringOps::toLower(cb); }
		}
		if( LLStringOps::isDigit(ca) ){
			if( cnt-->0 ){
				if( cb!=ca ) break;
			}else{
				if( !LLStringOps::isDigit(cb) ) break;
				for(ai=0; LLStringOps::isDigit(a[ai]); ai++);
				for(bi=0; LLStringOps::isDigit(b[bi]); bi++);
				if( ai<bi ){ ca=0; break; }
				if( bi<ai ){ cb=0; break; }
				if( ca!=cb ) break;
				cnt = ai;
			}
		}else if( ca!=cb ){   break;
		}
		ca = *(a++);
		cb = *(b++);
	}
	if( ca==cb ) ca += bias;
	return ca-cb;
}

// static
template<class T>
S32 LLStringUtilBase<T>::compareDictInsensitive(const string_type& astr, const string_type& bstr)
{
	const T* a = astr.c_str();
	const T* b = bstr.c_str();
	T ca, cb;
	S32 ai, bi, cnt = 0;

	ca = *(a++);
	cb = *(b++);
	while( ca && cb ){
		if( LLStringOps::isUpper(ca) ){ ca = LLStringOps::toLower(ca); }
		if( LLStringOps::isUpper(cb) ){ cb = LLStringOps::toLower(cb); }
		if( LLStringOps::isDigit(ca) ){
			if( cnt-->0 ){
				if( cb!=ca ) break;
			}else{
				if( !LLStringOps::isDigit(cb) ) break;
				for(ai=0; LLStringOps::isDigit(a[ai]); ai++);
				for(bi=0; LLStringOps::isDigit(b[bi]); bi++);
				if( ai<bi ){ ca=0; break; }
				if( bi<ai ){ cb=0; break; }
				if( ca!=cb ) break;
				cnt = ai;
			}
		}else if( ca!=cb ){   break;
		}
		ca = *(a++);
		cb = *(b++);
	}
	return ca-cb;
}

// Puts compareDict() in a form appropriate for LL container classes to use for sorting.
// static 
template<class T> 
BOOL LLStringUtilBase<T>::precedesDict( const string_type& a, const string_type& b )
{
	if( a.size() && b.size() )
	{
		return (LLStringUtilBase<T>::compareDict(a.c_str(), b.c_str()) < 0);
	}
	else
	{
		return (!b.empty());
	}
}

//static
template<class T> 
void LLStringUtilBase<T>::toUpper(string_type& string)	
{ 
	if( !string.empty() )
	{ 
		std::transform(
			string.begin(),
			string.end(),
			string.begin(),
			(T(*)(T)) &LLStringOps::toUpper);
	}
}

//static
template<class T> 
void LLStringUtilBase<T>::toLower(string_type& string)
{ 
	if( !string.empty() )
	{ 
		std::transform(
			string.begin(),
			string.end(),
			string.begin(),
			(T(*)(T)) &LLStringOps::toLower);
	}
}

//static
template<class T> 
void LLStringUtilBase<T>::trimHead(string_type& string)
{			
	if( !string.empty() )
	{
		size_type i = 0;
		while( i < string.length() && LLStringOps::isSpace( string[i] ) )
		{
			i++;
		}
		string.erase(0, i);
	}
}

//static
template<class T> 
void LLStringUtilBase<T>::trimTail(string_type& string)
{			
	if( string.size() )
	{
		size_type len = string.length();
		size_type i = len;
		while( i > 0 && LLStringOps::isSpace( string[i-1] ) )
		{
			i--;
		}

		string.erase( i, len - i );
	}
}


// Replace line feeds with carriage return-line feed pairs.
//static
template<class T>
void LLStringUtilBase<T>::addCRLF(string_type& string)
{
	const T LF = 10;
	const T CR = 13;

	// Count the number of line feeds
	size_type count = 0;
	size_type len = string.size();
	size_type i;
	for( i = 0; i < len; i++ )
	{
		if( string[i] == LF )
		{
			count++;
		}
	}

	// Insert a carriage return before each line feed
	if( count )
	{
		size_type size = len + count;
		T *t = new T[size];
		size_type j = 0;
		for( i = 0; i < len; ++i )
		{
			if( string[i] == LF )
			{
				t[j] = CR;
				++j;
			}
			t[j] = string[i];
			++j;
		}

		string.assign(t, size);
		delete[] t;
	}
}

// Remove all carriage returns
//static
template<class T> 
void LLStringUtilBase<T>::removeCRLF(string_type& string)
{
	const T CR = 13;

	size_type cr_count = 0;
	size_type len = string.size();
	size_type i;
	for( i = 0; i < len - cr_count; i++ )
	{
		if( string[i+cr_count] == CR )
		{
			cr_count++;
		}

		string[i] = string[i+cr_count];
	}
	string.erase(i, cr_count);
}

//static
template<class T> 
void LLStringUtilBase<T>::replaceChar( string_type& string, T target, T replacement )
{
	size_type found_pos = 0;
	while( (found_pos = string.find(target, found_pos)) != string_type::npos ) 
	{
		string[found_pos] = replacement;
		found_pos++; // avoid infinite defeat if target == replacement
	}
}

//static
template<class T> 
void LLStringUtilBase<T>::replaceString( string_type& string, string_type target, string_type replacement )
{
	size_type found_pos = 0;
	while( (found_pos = string.find(target, found_pos)) != string_type::npos )
	{
		string.replace( found_pos, target.length(), replacement );
		found_pos += replacement.length(); // avoid infinite defeat if replacement contains target
	}
}

//static
template<class T> 
void LLStringUtilBase<T>::replaceNonstandardASCII( string_type& string, T replacement )
{
	const char LF = 10;
	const S8 MIN = 32;
//	const S8 MAX = 127;

	size_type len = string.size();
	for( size_type i = 0; i < len; i++ )
	{
		// No need to test MAX < mText[i] because we treat mText[i] as a signed char,
		// which has a max value of 127.
		if( ( S8(string[i]) < MIN ) && (string[i] != LF) )
		{
			string[i] = replacement;
		}
	}
}

//static
template<class T> 
void LLStringUtilBase<T>::replaceTabsWithSpaces( string_type& str, size_type spaces_per_tab )
{
	const T TAB = '\t';
	const T SPACE = ' ';

	string_type out_str;
	// Replace tabs with spaces
	for (size_type i = 0; i < str.length(); i++)
	{
		if (str[i] == TAB)
		{
			for (size_type j = 0; j < spaces_per_tab; j++)
				out_str += SPACE;
		}
		else
		{
			out_str += str[i];
		}
	}
	str = out_str;
}

//static
template<class T> 
BOOL LLStringUtilBase<T>::containsNonprintable(const string_type& string)
{
	const char MIN = 32;
	BOOL rv = FALSE;
	for (size_type i = 0; i < string.size(); i++)
	{
		if(string[i] < MIN)
		{
			rv = TRUE;
			break;
		}
	}
	return rv;
}

//static
template<class T> 
void LLStringUtilBase<T>::stripNonprintable(string_type& string)
{
	const char MIN = 32;
	size_type j = 0;
	if (string.empty())
	{
		return;
	}
	size_t src_size = string.size();
	char* c_string = new char[src_size + 1];
	if(c_string == NULL)
	{
		return;
	}
	copy(c_string, string.c_str(), src_size+1);
	char* write_head = &c_string[0];
	for (size_type i = 0; i < src_size; i++)
	{
		char* read_head = &string[i];
		write_head = &c_string[j];
		if(!(*read_head < MIN))
		{
			*write_head = *read_head;
			++j;
		}
	}
	c_string[j]= '\0';
	string = c_string;
	delete []c_string;
}

template<class T>
std::basic_string<T> LLStringUtilBase<T>::quote(const string_type& str,
												const string_type& triggers,
												const string_type& escape)
{
	size_type len(str.length());
	// If the string is already quoted, assume user knows what s/he's doing.
	if (len >= 2 && str[0] == '"' && str[len-1] == '"')
	{
		return str;
	}

	// Not already quoted: do we need to? triggers.empty() is a special case
	// meaning "always quote."
	if ((! triggers.empty()) && str.find_first_of(triggers) == string_type::npos)
	{
		// no trigger characters, don't bother quoting
		return str;
	}

	// For whatever reason, we must quote this string.
	string_type result;
	result.push_back('"');
	for (typename string_type::const_iterator ci(str.begin()), cend(str.end()); ci != cend; ++ci)
	{
		if (*ci == '"')
		{
			result.append(escape);
		}
		result.push_back(*ci);
	}
	result.push_back('"');
	return result;
}

template<class T> 
void LLStringUtilBase<T>::_makeASCII(string_type& string)
{
	// Replace non-ASCII chars with LL_UNKNOWN_CHAR
	for (size_type i = 0; i < string.length(); i++)
	{
		if (string[i] > 0x7f)
		{
			string[i] = LL_UNKNOWN_CHAR;
		}
	}
}

// static
template<class T> 
void LLStringUtilBase<T>::copy( T* dst, const T* src, size_type dst_size )
{
	if( dst_size > 0 )
	{
		size_type min_len = 0;
		if( src )
		{
			min_len = llmin( dst_size - 1, strlen( src ) );  /* Flawfinder: ignore */
			memcpy(dst, src, min_len * sizeof(T));		/* Flawfinder: ignore */
		}
		dst[min_len] = '\0';
	}
}

// static
template<class T> 
void LLStringUtilBase<T>::copyInto(string_type& dst, const string_type& src, size_type offset)
{
	if ( offset == dst.length() )
	{
		// special case - append to end of string and avoid expensive
		// (when strings are large) string manipulations
		dst += src;
	}
	else
	{
		string_type tail = dst.substr(offset);

		dst = dst.substr(0, offset);
		dst += src;
		dst += tail;
	};
}

// True if this is the head of s.
//static
template<class T> 
BOOL LLStringUtilBase<T>::isHead( const string_type& string, const T* s ) 
{ 
	if( string.empty() )
	{
		// Early exit
		return FALSE;
	}
	else
	{
		return (strncmp( s, string.c_str(), string.size() ) == 0);
	}
}

// static
template<class T> 
bool LLStringUtilBase<T>::startsWith(
	const string_type& string,
	const string_type& substr)
{
	if(string.empty() || (substr.empty())) return false;
	if(0 == string.find(substr)) return true;
	return false;
}

// static
template<class T> 
bool LLStringUtilBase<T>::endsWith(
	const string_type& string,
	const string_type& substr)
{
	if(string.empty() || (substr.empty())) return false;
	std::string::size_type idx = string.rfind(substr);
	if(std::string::npos == idx) return false;
	return (idx == (string.size() - substr.size()));
}


template<class T> 
BOOL LLStringUtilBase<T>::convertToBOOL(const string_type& string, BOOL& value)
{
	if( string.empty() )
	{
		return FALSE;
	}

	string_type temp( string );
	trim(temp);
	if( 
		(temp == "1") || 
		(temp == "T") || 
		(temp == "t") || 
		(temp == "TRUE") || 
		(temp == "true") || 
		(temp == "True") )
	{
		value = TRUE;
		return TRUE;
	}
	else
	if( 
		(temp == "0") || 
		(temp == "F") || 
		(temp == "f") || 
		(temp == "FALSE") || 
		(temp == "false") || 
		(temp == "False") )
	{
		value = FALSE;
		return TRUE;
	}

	return FALSE;
}

template<class T> 
BOOL LLStringUtilBase<T>::convertToU8(const string_type& string, U8& value) 
{
	S32 value32 = 0;
	BOOL success = convertToS32(string, value32);
	if( success && (U8_MIN <= value32) && (value32 <= U8_MAX) )
	{
		value = (U8) value32;
		return TRUE;
	}
	return FALSE;
}

template<class T> 
BOOL LLStringUtilBase<T>::convertToS8(const string_type& string, S8& value) 
{
	S32 value32 = 0;
	BOOL success = convertToS32(string, value32);
	if( success && (S8_MIN <= value32) && (value32 <= S8_MAX) )
	{
		value = (S8) value32;
		return TRUE;
	}
	return FALSE;
}

template<class T> 
BOOL LLStringUtilBase<T>::convertToS16(const string_type& string, S16& value) 
{
	S32 value32 = 0;
	BOOL success = convertToS32(string, value32);
	if( success && (S16_MIN <= value32) && (value32 <= S16_MAX) )
	{
		value = (S16) value32;
		return TRUE;
	}
	return FALSE;
}

template<class T> 
BOOL LLStringUtilBase<T>::convertToU16(const string_type& string, U16& value) 
{
	S32 value32 = 0;
	BOOL success = convertToS32(string, value32);
	if( success && (U16_MIN <= value32) && (value32 <= U16_MAX) )
	{
		value = (U16) value32;
		return TRUE;
	}
	return FALSE;
}

template<class T> 
BOOL LLStringUtilBase<T>::convertToU32(const string_type& string, U32& value) 
{
	if( string.empty() )
	{
		return FALSE;
	}

	string_type temp( string );
	trim(temp);
	U32 v;
	std::basic_istringstream<T> i_stream((string_type)temp);
	if(i_stream >> v)
	{
		value = v;
		return TRUE;
	}
	return FALSE;
}

template<class T> 
BOOL LLStringUtilBase<T>::convertToS32(const string_type& string, S32& value) 
{
	if( string.empty() )
	{
		return FALSE;
	}

	string_type temp( string );
	trim(temp);
	S32 v;
	std::basic_istringstream<T> i_stream((string_type)temp);
	if(i_stream >> v)
	{
		//TODO: figure out overflow and underflow reporting here
		//if((LONG_MAX == v) || (LONG_MIN == v))
		//{
		//	// Underflow or overflow
		//	return FALSE;
		//}

		value = v;
		return TRUE;
	}
	return FALSE;
}

template<class T> 
BOOL LLStringUtilBase<T>::convertToF32(const string_type& string, F32& value) 
{
	F64 value64 = 0.0;
	BOOL success = convertToF64(string, value64);
	if( success && (-F32_MAX <= value64) && (value64 <= F32_MAX) )
	{
		value = (F32) value64;
		return TRUE;
	}
	return FALSE;
}

template<class T> 
BOOL LLStringUtilBase<T>::convertToF64(const string_type& string, F64& value)
{
	if( string.empty() )
	{
		return FALSE;
	}

	string_type temp( string );
	trim(temp);
	F64 v;
	std::basic_istringstream<T> i_stream((string_type)temp);
	if(i_stream >> v)
	{
		//TODO: figure out overflow and underflow reporting here
		//if( ((-HUGE_VAL == v) || (HUGE_VAL == v))) )
		//{
		//	// Underflow or overflow
		//	return FALSE;
		//}

		value = v;
		return TRUE;
	}
	return FALSE;
}

template<class T> 
void LLStringUtilBase<T>::truncate(string_type& string, size_type count)
{
	size_type cur_size = string.size();
	string.resize(count < cur_size ? count : cur_size);
}

#endif  // LL_STRING_H
