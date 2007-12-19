/** 
 * @file llstring.h
 * @brief String utility functions and LLString class.
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

#ifndef LL_LLSTRING_H
#define LL_LLSTRING_H

#if LL_LINUX || LL_SOLARIS
#include <wctype.h>
#include <wchar.h>
#endif

const char LL_UNKNOWN_CHAR = '?';

class LLVector3;
class LLVector3d;
class LLQuaternion;
class LLUUID;
class LLColor4;
class LLColor4U;

#if (LL_DARWIN || LL_SOLARIS || (LL_LINUX && __GNUC__ > 2))
// Template specialization of char_traits for U16s. Only necessary on Mac for now (exists on Windows, unused/broken on Linux/gcc2.95)
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

class LLStringOps
{
public:
	static char toUpper(char elem) { return toupper(elem); }
	static llwchar toUpper(llwchar elem) { return towupper(elem); }
	
	static char toLower(char elem) { return tolower(elem); }
	static llwchar toLower(llwchar elem) { return towlower(elem); }

	static BOOL isSpace(char elem) { return isspace(elem) != 0; }
	static BOOL isSpace(llwchar elem) { return iswspace(elem) != 0; }

	static BOOL isUpper(char elem) { return isupper(elem) != 0; }
	static BOOL isUpper(llwchar elem) { return iswupper(elem) != 0; }

	static BOOL isLower(char elem) { return islower(elem) != 0; }
	static BOOL isLower(llwchar elem) { return iswlower(elem) != 0; }

	static S32	collate(const char* a, const char* b) { return strcoll(a, b); }
	static S32	collate(const llwchar* a, const llwchar* b);

	static BOOL isDigit(char a) { return isdigit(a) != 0; }
	static BOOL isDigit(llwchar a) { return iswdigit(a) != 0; }
};

//RN: I used a templated base class instead of a pure interface class to minimize code duplication
// but it might be worthwhile to just go with two implementations (LLString and LLWString) of
// an interface class, unless we can think of a good reason to have a std::basic_string polymorphic base

//****************************************************************
// NOTA BENE: do *NOT* dynamically allocate memory inside of LLStringBase as the {*()^#%*)#%W^*)#%*)STL implentation
// of basic_string doesn't provide a virtual destructor.  If we need to allocate resources specific to LLString
// then we should either customize std::basic_string to linden::basic_string or change LLString to be a wrapper
// that contains an instance of std::basic_string.  Similarly, overriding methods defined in std::basic_string will *not*
// be called in a polymorphic manner (passing an instance of basic_string to a particular function)
//****************************************************************

template <class T>
class LLStringBase : public std::basic_string<T> 
{
public:
	typedef typename std::basic_string<T>::size_type size_type;
	
	// naming convention follows those set for LLUUID
// 	static LLStringBase null; // deprecated for std::string compliance
// 	static LLStringBase zero_length; // deprecated for std::string compliance
	
	
	// standard constructors
	LLStringBase() : std::basic_string<T>()	{}
	LLStringBase(const LLStringBase& s): std::basic_string<T>(s) {}
	LLStringBase(const std::basic_string<T>& s) : std::basic_string<T>(s) {}
	LLStringBase(const std::basic_string<T>& s, size_type pos, size_type n = std::basic_string<T>::npos)
		: std::basic_string<T>(s, pos, n) {}
	LLStringBase(size_type count, const T& c) : std::basic_string<T>() { assign(count, c);}
	// custom constructors
	LLStringBase(const T* s);
	LLStringBase(const T* s, size_type n);
	LLStringBase(const T* s, size_type pos, size_type n );
	
#if LL_LINUX || LL_SOLARIS
	void clear() { assign(null); }
	
	LLStringBase<T>& assign(const T* s);
	LLStringBase<T>& assign(const T* s, size_type n); 
	LLStringBase<T>& assign(const LLStringBase& s);
	LLStringBase<T>& assign(size_type n, const T& c);
	LLStringBase<T>& assign(const T* a, const T* b);
	LLStringBase<T>& assign(typename LLStringBase<T>::iterator &it1, typename LLStringBase<T>::iterator &it2);
	LLStringBase<T>& assign(typename LLStringBase<T>::const_iterator &it1, typename LLStringBase<T>::const_iterator &it2);

    // workaround for bug in gcc2 STL headers.
    #if ((__GNUC__ <= 2) && (!defined _STLPORT_VERSION))
    const T* c_str () const
    {
        if (length () == 0)
        {
            static const T zero = 0;
            return &zero;
        }

        //terminate ();
        { string_char_traits<T>::assign(const_cast<T*>(data())[length()], string_char_traits<T>::eos()); }

        return data ();
    }
    #endif
#endif

	bool operator==(const T* _Right) const { return _Right ? (std::basic_string<T>::compare(_Right) == 0) : this->empty(); }
	
public:
	/////////////////////////////////////////////////////////////////////////////////////////
	// Static Utility functions that operate on std::strings

	static LLStringBase null;
	
	typedef std::map<std::string, std::string> format_map_t;
	static S32 format(std::basic_string<T>& s, const format_map_t& fmt_map);
	
	static BOOL	isValidIndex(const std::basic_string<T>& string, size_type i)
	{
		return !string.empty() && (0 <= i) && (i <= string.size());
	}

	static void	trimHead(std::basic_string<T>& string);
	static void	trimTail(std::basic_string<T>& string);
	static void	trim(std::basic_string<T>& string)	{ trimHead(string); trimTail(string); }
	static void truncate(std::basic_string<T>& string, size_type count);

	static void	toUpper(std::basic_string<T>& string);
	static void	toLower(std::basic_string<T>& string);
	
	// True if this is the head of s.
	static BOOL	isHead( const std::basic_string<T>& string, const T* s ); 
	
	static void	addCRLF(std::basic_string<T>& string);
	static void	removeCRLF(std::basic_string<T>& string);

	static void	replaceTabsWithSpaces( std::basic_string<T>& string, size_type spaces_per_tab );
	static void	replaceNonstandardASCII( std::basic_string<T>& string, T replacement );
	static void	replaceChar( std::basic_string<T>& string, T target, T replacement );

	static BOOL	containsNonprintable(const std::basic_string<T>& string);
	static void	stripNonprintable(std::basic_string<T>& string);

	/**
	 * @brief Unsafe way to make ascii characters. You should probably
	 * only call this when interacting with the host operating system.
	 * The 1 byte LLString does not work correctly.
	 * The 2 and 4 byte LLString probably work, so LLWString::_makeASCII
	 * should work.
	 */
	static void _makeASCII(std::basic_string<T>& string);

	// Conversion to other data types
	static BOOL	convertToBOOL(const std::basic_string<T>& string, BOOL& value);
	static BOOL	convertToU8(const std::basic_string<T>& string, U8& value);
	static BOOL	convertToS8(const std::basic_string<T>& string, S8& value);
	static BOOL	convertToS16(const std::basic_string<T>& string, S16& value);
	static BOOL	convertToU16(const std::basic_string<T>& string, U16& value);
	static BOOL	convertToU32(const std::basic_string<T>& string, U32& value);
	static BOOL	convertToS32(const std::basic_string<T>& string, S32& value);
	static BOOL	convertToF32(const std::basic_string<T>& string, F32& value);
	static BOOL	convertToF64(const std::basic_string<T>& string, F64& value);

	/////////////////////////////////////////////////////////////////////////////////////////
	// Utility functions for working with char*'s and strings

	// Like strcmp but also handles empty strings. Uses
	// current locale.
	static S32		compareStrings(const T* lhs, const T* rhs);
	
	// case insensitive version of above. Uses current locale on
	// Win32, and falls back to a non-locale aware comparison on
	// Linux.
	static S32		compareInsensitive(const T* lhs, const T* rhs);

	// Case sensitive comparison with good handling of numbers.  Does not use current locale.
	// a.k.a. strdictcmp()
	static S32		compareDict(const std::basic_string<T>& a, const std::basic_string<T>& b);

	// Case *in*sensitive comparison with good handling of numbers.  Does not use current locale.
	// a.k.a. strdictcmp()
	static S32		compareDictInsensitive(const std::basic_string<T>& a, const std::basic_string<T>& b);

	// Puts compareDict() in a form appropriate for LL container classes to use for sorting.
	static BOOL		precedesDict( const std::basic_string<T>& a, const std::basic_string<T>& b );

	// A replacement for strncpy.
	// If the dst buffer is dst_size bytes long or more, ensures that dst is null terminated and holds
	// up to dst_size-1 characters of src.
	static void		copy(T* dst, const T* src, size_type dst_size);
	
	// Copies src into dst at a given offset.  
	static void		copyInto(std::basic_string<T>& dst, const std::basic_string<T>& src, size_type offset);
	
#ifdef _DEBUG	
	static void		testHarness();
#endif

};

template<class T> LLStringBase<T> LLStringBase<T>::null;

typedef LLStringBase<char> LLString;
typedef LLStringBase<llwchar> LLWString;

//@ Use this where we want to disallow input in the form of "foo"
//  This is used to catch places where english text is embedded in the code
//  instead of in a translatable XUI file.
class LLStringExplicit : public LLString
{
public:
	explicit LLStringExplicit(const char* s) : LLString(s) {}
	LLStringExplicit(const LLString& s) : LLString(s) {}
	LLStringExplicit(const std::string& s) : LLString(s) {}
	LLStringExplicit(const std::string& s, size_type pos, size_type n = std::string::npos) : LLString(s, pos, n) {}
};

struct LLDictionaryLess
{
public:
	bool operator()(const std::string& a, const std::string& b)
	{
		return (LLString::precedesDict(a, b) ? true : false);
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
 * @return a copy of in string minus the trailing count characters.
 */
inline std::string chop_tail_copy(
	const std::string& in,
	std::string::size_type count)
{
	return std::string(in, 0, in.length() - count);
}

/**
 * @brief Return a string constructed from in without crashing if the
 * pointer is NULL.
 */
std::string ll_safe_string(const char* in);

/**
 * @brief This translates a nybble stored as a hex value from 0-f back
 * to a nybble in the low order bits of the return byte.
 */
U8 hex_as_nybble(char hex);

/**
 * @brief read the contents of a file into a string.
 *
 * Since this function has no concept of character encoding, most
 * anything you do with this method ill-advised. Please avoid.
 * @param str [out] The string which will have.
 * @param filename The full name of the file to read.
 * @return Returns true on success. If false, str is unmodified.
 */
bool _read_file_into_string(std::string& str, const char* filename);

/**
 * Unicode support
 */

// Make the incoming string a utf8 string. Replaces any unknown glyph
// with the UNKOWN_CHARACTER. Once any unknown glph is found, the rest
// of the data may not be recovered.
std::string rawstr_to_utf8(const std::string& raw);

//
// We should never use UTF16 except when communicating with Win32!
//
typedef std::basic_string<U16> llutf16string;

LLWString utf16str_to_wstring(const llutf16string &utf16str, S32 len);
LLWString utf16str_to_wstring(const llutf16string &utf16str);

llutf16string wstring_to_utf16str(const LLWString &utf32str, S32 len);
llutf16string wstring_to_utf16str(const LLWString &utf32str);

llutf16string utf8str_to_utf16str ( const LLString& utf8str, S32 len);
llutf16string utf8str_to_utf16str ( const LLString& utf8str );

LLWString utf8str_to_wstring(const std::string &utf8str, S32 len);
LLWString utf8str_to_wstring(const std::string &utf8str);
// Same function, better name. JC
inline LLWString utf8string_to_wstring(const std::string& utf8_string) { return utf8str_to_wstring(utf8_string); }

// Special hack for llfilepicker.cpp:
S32 utf16chars_to_utf8chars(const U16* inchars, char* outchars, S32* nchars8 = 0);
S32 utf16chars_to_wchar(const U16* inchars, llwchar* outchar);
S32 wchar_to_utf8chars(llwchar inchar, char* outchars);

//
std::string wstring_to_utf8str(const LLWString &utf32str, S32 len);
std::string wstring_to_utf8str(const LLWString &utf32str);

std::string utf16str_to_utf8str(const llutf16string &utf16str, S32 len);
std::string utf16str_to_utf8str(const llutf16string &utf16str);

// Length of this UTF32 string in bytes when transformed to UTF8
S32 wstring_utf8_length(const LLWString& wstr); 

// Length in bytes of this wide char in a UTF8 string
S32 wchar_utf8_length(const llwchar wc); 

std::string utf8str_tolower(const std::string& utf8str);

// Length in llwchar (UTF-32) of the first len units (16 bits) of the given UTF-16 string.
S32 utf16str_wstring_length(const llutf16string &utf16str, S32 len);

// Length in utf16string (UTF-16) of wlen wchars beginning at woffset.
S32 wstring_utf16_length(const LLWString & wstr, S32 woffset, S32 wlen);

// Length in wstring (i.e., llwchar count) of a part of a wstring specified by utf16 length (i.e., utf16 units.)
S32 wstring_wstring_length_from_utf16_length(const LLWString & wstr, S32 woffset, S32 utf16_length, BOOL *unaligned = NULL);

/**
 * @brief Properly truncate a utf8 string to a maximum byte count.
 * 
 * The returned string may be less than max_len if the truncation
 * happens in the middle of a glyph. If max_len is longer than the
 * string passed in, the return value == utf8str.
 * @param utf8str A valid utf8 string to truncate.
 * @param max_len The maximum number of bytes in the returne
 * @return Returns a valid utf8 string with byte count <= max_len.
 */
std::string utf8str_truncate(const std::string& utf8str, const S32 max_len);

std::string utf8str_trim(const std::string& utf8str);

S32 utf8str_compare_insensitive(
	const std::string& lhs,
	const std::string& rhs);

/**
 * @brief Replace all occurences of target_char with replace_char
 *
 * @param utf8str A utf8 string to process.
 * @param target_char The wchar to be replaced
 * @param replace_char The wchar which is written on replace
 */
std::string utf8str_substChar(
	const std::string& utf8str,
	const llwchar target_char,
	const llwchar replace_char);

std::string utf8str_makeASCII(const std::string& utf8str);

// Hack - used for evil notecards.
std::string mbcsstring_makeASCII(const std::string& str); 

std::string utf8str_removeCRLF(const std::string& utf8str);


template <class T>
std::ostream& operator<<(std::ostream &s, const LLStringBase<T> &str)
{
	s << ((std::basic_string<T>)str);
	return s;
}

std::ostream& operator<<(std::ostream &s, const LLWString &wstr);

#if LL_WINDOWS
int safe_snprintf(char *str, size_t size, const char *format, ...);
#endif // LL_WINDOWS

/**
 * Many of the 'strip' and 'replace' methods of LLStringBase need
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
	 *
	 * @param [in,out] string the to modify. out value is the string
	 * with zero non-printable characters.
	 * @param The replacement character. use LL_UNKNOWN_CHAR if unsure.
	 */
	void replace_nonprintable(
		std::basic_string<char>& string,
		char replacement);

	/**
	 * @brief Replace all non-printable characters with replacement in
	 * a wide string.
	 *
	 * @param [in,out] string the to modify. out value is the string
	 * with zero non-printable characters.
	 * @param The replacement character. use LL_UNKNOWN_CHAR if unsure.
	 */
	void replace_nonprintable(
		std::basic_string<llwchar>& string,
		llwchar replacement);

	/**
	 * @brief Replace all non-printable characters and pipe characters
	 * with replacement in a string.
	 *
	 * @param [in,out] the string to modify. out value is the string
	 * with zero non-printable characters and zero pipe characters.
	 * @param The replacement character. use LL_UNKNOWN_CHAR if unsure.
	 */
	void replace_nonprintable_and_pipe(std::basic_string<char>& str,
									   char replacement);

	/**
	 * @brief Replace all non-printable characters and pipe characters
	 * with replacement in a wide string.
	 *
	 * @param [in,out] the string to modify. out value is the string
	 * with zero non-printable characters and zero pipe characters.
	 * @param The replacement wide character. use LL_UNKNOWN_CHAR if unsure.
	 */
	void replace_nonprintable_and_pipe(std::basic_string<llwchar>& str,
									   llwchar replacement);
}

////////////////////////////////////////////////////////////

// static
template<class T> 
S32 LLStringBase<T>::format(std::basic_string<T>& s, const format_map_t& fmt_map)
{
	typedef typename std::basic_string<T>::size_type string_size_type_t;
	S32 res = 0;
	for (format_map_t::const_iterator iter = fmt_map.begin(); iter != fmt_map.end(); ++iter)
	{
		U32 fmtlen = iter->first.size();
		string_size_type_t n = 0;
		while (1)
		{
			n = s.find(iter->first, n);
			if (n == std::basic_string<T>::npos)
			{
				break;
			}
			s.erase(n, fmtlen);
			s.insert(n, iter->second);
			n += fmtlen;
			++res;
		}
	}
	return res;
}

// static
template<class T> 
S32 LLStringBase<T>::compareStrings(const T* lhs, const T* rhs)
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

// static
template<class T> 
S32 LLStringBase<T>::compareInsensitive(const T* lhs, const T* rhs )
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
		LLStringBase<T> lhs_string(lhs);
		LLStringBase<T> rhs_string(rhs);
		LLStringBase<T>::toUpper(lhs_string);
		LLStringBase<T>::toUpper(rhs_string);
		result = LLStringOps::collate(lhs_string.c_str(), rhs_string.c_str());
	}
	return result;
}


// Case sensitive comparison with good handling of numbers.  Does not use current locale.
// a.k.a. strdictcmp()

//static 
template<class T>
S32 LLStringBase<T>::compareDict(const std::basic_string<T>& astr, const std::basic_string<T>& bstr)
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

template<class T>
S32 LLStringBase<T>::compareDictInsensitive(const std::basic_string<T>& astr, const std::basic_string<T>& bstr)
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
BOOL LLStringBase<T>::precedesDict( const std::basic_string<T>& a, const std::basic_string<T>& b )
{
	if( a.size() && b.size() )
	{
		return (LLStringBase<T>::compareDict(a.c_str(), b.c_str()) < 0);
	}
	else
	{
		return (!b.empty());
	}
}

// Constructors
template<class T> 
LLStringBase<T>::LLStringBase(const T* s ) : std::basic_string<T>()
{
	if (s) assign(s);
}

template<class T> 
LLStringBase<T>::LLStringBase(const T* s, size_type n ) : std::basic_string<T>()
{
	if (s) assign(s, n);
}

// Init from a substring
template<class T> 
LLStringBase<T>::LLStringBase(const T* s, size_type pos, size_type n ) : std::basic_string<T>()
{
	if( s )
	{
		assign(s + pos, n);
	}
	else
	{
		assign(LLStringBase<T>::null);
	}
}

#if LL_LINUX || LL_SOLARIS
template<class T> 
LLStringBase<T>& LLStringBase<T>::assign(const T* s)
{
	if (s)
	{
		std::basic_string<T>::assign(s);
	}
	else
	{
		assign(LLStringBase<T>::null);
	}
	return *this;
}

template<class T> 
LLStringBase<T>& LLStringBase<T>::assign(const T* s, size_type n)
{
	if (s)
	{
		std::basic_string<T>::assign(s, n);
	}
	else
	{
		assign(LLStringBase<T>::null);
	}
	return *this;
}

template<class T> 
LLStringBase<T>& LLStringBase<T>::assign(const LLStringBase<T>& s)
{
	std::basic_string<T>::assign(s);
	return *this;
}

template<class T> 
LLStringBase<T>& LLStringBase<T>::assign(size_type n, const T& c)
{
    std::basic_string<T>::assign(n, c);
    return *this;
}

template<class T> 
LLStringBase<T>& LLStringBase<T>::assign(const T* a, const T* b)
{
    if (a > b)
        assign(LLStringBase<T>::null);
    else
        assign(a, (size_type) (b-a));
    return *this;
}

template<class T>
LLStringBase<T>& LLStringBase<T>::assign(typename LLStringBase<T>::iterator &it1, typename LLStringBase<T>::iterator &it2)
{
    assign(LLStringBase<T>::null);
    while(it1 != it2)
        *this += *it1++;
    return *this;
}

template<class T>
LLStringBase<T>& LLStringBase<T>::assign(typename LLStringBase<T>::const_iterator &it1, typename LLStringBase<T>::const_iterator &it2)
{
    assign(LLStringBase<T>::null);
    while(it1 != it2)
        *this += *it1++;
    return *this;
}
#endif

//static
template<class T> 
void LLStringBase<T>::toUpper(std::basic_string<T>& string)	
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
void LLStringBase<T>::toLower(std::basic_string<T>& string)
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
void LLStringBase<T>::trimHead(std::basic_string<T>& string)
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
void LLStringBase<T>::trimTail(std::basic_string<T>& string)
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
void LLStringBase<T>::addCRLF(std::basic_string<T>& string)
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
	}
}

// Remove all carriage returns
//static
template<class T> 
void LLStringBase<T>::removeCRLF(std::basic_string<T>& string)
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
void LLStringBase<T>::replaceChar( std::basic_string<T>& string, T target, T replacement )
{
	size_type found_pos = 0;
	for (found_pos = string.find(target, found_pos); 
		found_pos != std::basic_string<T>::npos; 
		found_pos = string.find(target, found_pos))
	{
		string[found_pos] = replacement;
	}
}

//static
template<class T> 
void LLStringBase<T>::replaceNonstandardASCII( std::basic_string<T>& string, T replacement )
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
void LLStringBase<T>::replaceTabsWithSpaces( std::basic_string<T>& str, size_type spaces_per_tab )
{
	const T TAB = '\t';
	const T SPACE = ' ';

	LLStringBase<T> out_str;
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
BOOL LLStringBase<T>::containsNonprintable(const std::basic_string<T>& string)
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
void LLStringBase<T>::stripNonprintable(std::basic_string<T>& string)
{
	const char MIN = 32;
	size_type j = 0;
	if (string.empty())
	{
		return;
	}
	char* c_string = new char[string.size() + 1];
	if(c_string == NULL)
	{
		return;
	}
	strcpy(c_string, string.c_str());	/*Flawfinder: ignore*/
	char* write_head = &c_string[0];
	for (size_type i = 0; i < string.size(); i++)
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
void LLStringBase<T>::_makeASCII(std::basic_string<T>& string)
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
void LLStringBase<T>::copy( T* dst, const T* src, size_type dst_size )
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
void LLStringBase<T>::copyInto(std::basic_string<T>& dst, const std::basic_string<T>& src, size_type offset)
{
	if ( offset == dst.length() )
	{
		// special case - append to end of string and avoid expensive
		// (when strings are large) string manipulations
		dst += src;
	}
	else
	{
		std::basic_string<T> tail = dst.substr(offset);

		dst = dst.substr(0, offset);
		dst += src;
		dst += tail;
	};
}

// True if this is the head of s.
//static
template<class T> 
BOOL LLStringBase<T>::isHead( const std::basic_string<T>& string, const T* s ) 
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

template<class T> 
BOOL LLStringBase<T>::convertToBOOL(const std::basic_string<T>& string, BOOL& value)
{
	if( string.empty() )
	{
		return FALSE;
	}

	LLStringBase<T> temp( string );
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
BOOL LLStringBase<T>::convertToU8(const std::basic_string<T>& string, U8& value) 
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
BOOL LLStringBase<T>::convertToS8(const std::basic_string<T>& string, S8& value) 
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
BOOL LLStringBase<T>::convertToS16(const std::basic_string<T>& string, S16& value) 
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
BOOL LLStringBase<T>::convertToU16(const std::basic_string<T>& string, U16& value) 
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
BOOL LLStringBase<T>::convertToU32(const std::basic_string<T>& string, U32& value) 
{
	if( string.empty() )
	{
		return FALSE;
	}

	LLStringBase<T> temp( string );
	trim(temp);
	U32 v;
	std::basic_istringstream<T> i_stream((std::basic_string<T>)temp);
	if(i_stream >> v)
	{
		//TODO: figure out overflow reporting here
		//if( ULONG_MAX == v )
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
BOOL LLStringBase<T>::convertToS32(const std::basic_string<T>& string, S32& value) 
{
	if( string.empty() )
	{
		return FALSE;
	}

	LLStringBase<T> temp( string );
	trim(temp);
	S32 v;
	std::basic_istringstream<T> i_stream((std::basic_string<T>)temp);
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
BOOL LLStringBase<T>::convertToF32(const std::basic_string<T>& string, F32& value) 
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
BOOL LLStringBase<T>::convertToF64(const std::basic_string<T>& string, F64& value)
{
	if( string.empty() )
	{
		return FALSE;
	}

	LLStringBase<T> temp( string );
	trim(temp);
	F64 v;
	std::basic_istringstream<T> i_stream((std::basic_string<T>)temp);
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
void LLStringBase<T>::truncate(std::basic_string<T>& string, size_type count)
{
	size_type cur_size = string.size();
	string.resize(count < cur_size ? count : cur_size);
}

#endif  // LL_STRING_H
