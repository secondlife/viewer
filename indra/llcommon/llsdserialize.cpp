/** 
 * @file llsdserialize.cpp
 * @author Phoenix
 * @date 2006-03-05
 * @brief Implementation of LLSD parsers and formatters
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
#include "llsdserialize.h"
#include "llpointer.h"
#include "llstreamtools.h" // for fullread

#include <iostream>
#include "apr_base64.h"

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#ifdef LL_USESYSTEMLIBS
# include <zlib.h>
#else
# include "zlib-ng/zlib.h"  // for davep's dirty little zip functions
#endif

#if !LL_WINDOWS
#include <netinet/in.h> // htonl & ntohl
#endif

#include "lldate.h"
#include "llmemorystream.h"
#include "llsd.h"
#include "llstring.h"
#include "lluri.h"

// File constants
static const size_t MAX_HDR_LEN = 20;
static const S32 UNZIP_LLSD_MAX_DEPTH = 96;
static const char LEGACY_NON_HEADER[] = "<llsd>";
const std::string LLSD_BINARY_HEADER("LLSD/Binary");
const std::string LLSD_XML_HEADER("LLSD/XML");
const std::string LLSD_NOTATION_HEADER("llsd/notation");

//used to deflate a gzipped asset (currently used for navmeshes)
#define windowBits 15
#define ENABLE_ZLIB_GZIP 32

// If we published this in llsdserialize.h, we could use it in the
// implementation of LLSDOStreamer's operator<<().
template <class Formatter>
void format_using(const LLSD& data, std::ostream& ostr,
                  LLSDFormatter::EFormatterOptions options=LLSDFormatter::OPTIONS_PRETTY_BINARY)
{
    LLPointer<Formatter> f{ new Formatter };
    f->format(data, ostr, options);
}

template <class Parser>
S32 parse_using(std::istream& istr, LLSD& data, size_t max_bytes, S32 max_depth=-1)
{
    LLPointer<Parser> p{ new Parser };
    return p->parse(istr, data, max_bytes, max_depth);
}

/**
 * LLSDSerialize
 */

// static
void LLSDSerialize::serialize(const LLSD& sd, std::ostream& str, ELLSD_Serialize type,
							  LLSDFormatter::EFormatterOptions options)
{
	LLPointer<LLSDFormatter> f = NULL;

	switch (type)
	{
	case LLSD_BINARY:
		str << "<? " << LLSD_BINARY_HEADER << " ?>\n";
		f = new LLSDBinaryFormatter;
		break;

	case LLSD_XML:
		str << "<? " << LLSD_XML_HEADER << " ?>\n";
		f = new LLSDXMLFormatter;
		break;

	case LLSD_NOTATION:
		str << "<? " << LLSD_NOTATION_HEADER << " ?>\n";
		f = new LLSDNotationFormatter;
		break;

	default:
		LL_WARNS() << "serialize request for unknown ELLSD_Serialize" << LL_ENDL;
	}

	if (f.notNull())
	{
		f->format(sd, str, options);
	}
}

// static
bool LLSDSerialize::deserialize(LLSD& sd, std::istream& str, llssize max_bytes)
{
	char hdr_buf[MAX_HDR_LEN + 1] = ""; /* Flawfinder: ignore */
	bool fail_if_not_legacy = false;

    /*
     * Get the first line before anything. Don't read more than max_bytes:
     * this get() overload reads no more than (count-1) bytes into the
     * specified buffer. In the usual case when max_bytes exceeds
     * sizeof(hdr_buf), get() will read no more than sizeof(hdr_buf)-2.
     */
    llssize max_hdr_read = MAX_HDR_LEN;
	if (max_bytes != LLSDSerialize::SIZE_UNLIMITED)
	{
        max_hdr_read = llmin(max_bytes + 1, max_hdr_read);
	}
    str.get(hdr_buf, max_hdr_read, '\n');
	auto inbuf = str.gcount();

	// https://en.cppreference.com/w/cpp/io/basic_istream/get
	// When the get() above sees the specified delimiter '\n', it stops there
	// without pulling it from the stream. If it turns out that the stream
	// does NOT contain a header, and the content includes meaningful '\n',
	// it's important to pull that into hdr_buf too.
	if (inbuf < max_bytes && str.get(hdr_buf[inbuf]))
	{
		// got the delimiting '\n'
		++inbuf;
		// None of the following requires that hdr_buf contain a final '\0'
		// byte. We could store one if needed, since even the incremented
		// inbuf won't exceed sizeof(hdr_buf)-1, but there's no need.
	}
	std::string header{ hdr_buf, static_cast<std::string::size_type>(inbuf) };
	if (str.fail())
	{
		str.clear();
		fail_if_not_legacy = true;
	}

	if (!strncasecmp(LEGACY_NON_HEADER, hdr_buf, strlen(LEGACY_NON_HEADER))) /* Flawfinder: ignore */
	{	// Create a LLSD XML parser, and parse the first chunk read above.
		LLSDXMLParser x;
		x.parsePart(hdr_buf, inbuf);	// Parse the first part that was already read
		auto parsed = x.parse(str, sd, max_bytes - inbuf); // Parse the rest of it
		// Formally we should probably check (parsed != PARSE_FAILURE &&
		// parsed > 0), but since PARSE_FAILURE is -1, this suffices.
		return (parsed > 0);
	}

	if (fail_if_not_legacy)
	{
		LL_WARNS() << "deserialize LLSD parse failure" << LL_ENDL;
		return false;
	}

	/*
	* Remove the newline chars
	*/
	std::string::size_type lastchar = header.find_last_not_of("\r\n");
	if (lastchar != std::string::npos)
	{
		// It's important that find_last_not_of() returns size_type, which is
		// why lastchar explicitly declares the type above. erase(size_type)
		// erases from that offset to the end of the string, whereas
		// erase(iterator) erases only a single character.
		header.erase(lastchar+1);
	}

	// trim off the <? ... ?> header syntax
	auto start = header.find_first_not_of("<? ");
	if (start != std::string::npos)
	{
		auto end = header.find_first_of(" ?", start);
		if (end != std::string::npos)
		{
			header = header.substr(start, end - start);
			ws(str);
		}
	}
	/*
	 * Create the parser as appropriate
	 */
	if (0 == LLStringUtil::compareInsensitive(header, LLSD_BINARY_HEADER))
	{
		return (parse_using<LLSDBinaryParser>(str, sd, max_bytes-inbuf) > 0);
	}
	else if (0 == LLStringUtil::compareInsensitive(header, LLSD_XML_HEADER))
	{
		return (parse_using<LLSDXMLParser>(str, sd, max_bytes-inbuf) > 0);
	}
	else if (0 == LLStringUtil::compareInsensitive(header, LLSD_NOTATION_HEADER))
	{
		return (parse_using<LLSDNotationParser>(str, sd, max_bytes-inbuf) > 0);
	}
	else // no header we recognize
	{
		LLPointer<LLSDParser> p;
		if (inbuf && hdr_buf[0] == '<')
		{
			// looks like XML
			LL_DEBUGS() << "deserialize request with no header, assuming XML" << LL_ENDL;
			p = new LLSDXMLParser;
		}
		else
		{
			// assume notation
			LL_DEBUGS() << "deserialize request with no header, assuming notation" << LL_ENDL;
			p = new LLSDNotationParser;
		}
		// Since we've already read 'inbuf' bytes into 'hdr_buf', prepend that
		// data to whatever remains in 'str'.
		LLMemoryStreamBuf already(reinterpret_cast<const U8*>(hdr_buf), inbuf);
		cat_streambuf prebuff(&already, str.rdbuf());
		std::istream  prepend(&prebuff);
#if 1
		return (p->parse(prepend, sd, max_bytes) > 0);
#else
		// debugging the reconstituted 'prepend' stream
		// allocate a buffer that we hope is big enough for the whole thing
		std::vector<char> wholemsg((max_bytes == size_t(SIZE_UNLIMITED))? 1024 : max_bytes);
		prepend.read(wholemsg.data(), std::min(max_bytes, wholemsg.size()));
		LLMemoryStream replay(reinterpret_cast<const U8*>(wholemsg.data()), prepend.gcount());
		auto success{ p->parse(replay, sd, prepend.gcount()) > 0 };
		{
			LL_DEBUGS() << (success? "parsed: $$" : "failed: '")
						<< std::string(wholemsg.data(), llmin(prepend.gcount(), 100)) << "$$"
						<< LL_ENDL;
		}
		return success;
#endif
	}
}

/**
 * Endian handlers
 */
#if LL_BIG_ENDIAN
U64 ll_htonll(U64 hostlonglong) { return hostlonglong; }
U64 ll_ntohll(U64 netlonglong) { return netlonglong; }
F64 ll_htond(F64 hostlonglong) { return hostlonglong; }
F64 ll_ntohd(F64 netlonglong) { return netlonglong; }
#else
// I read some comments one a indicating that doing an integer add
// here would be faster than a bitwise or. For now, the or has
// programmer clarity, since the intended outcome matches the
// operation.
U64 ll_htonll(U64 hostlonglong)
{
	return ((U64)(htonl((U32)((hostlonglong >> 32) & 0xFFFFFFFF))) |
			((U64)(htonl((U32)(hostlonglong & 0xFFFFFFFF))) << 32));
}
U64 ll_ntohll(U64 netlonglong)
{
	return ((U64)(ntohl((U32)((netlonglong >> 32) & 0xFFFFFFFF))) |
			((U64)(ntohl((U32)(netlonglong & 0xFFFFFFFF))) << 32));
}
union LLEndianSwapper
{
	F64 d;
	U64 i;
};
F64 ll_htond(F64 hostdouble)
{
	LLEndianSwapper tmp;
	tmp.d = hostdouble;
	tmp.i = ll_htonll(tmp.i);
	return tmp.d;
}
F64 ll_ntohd(F64 netdouble)
{
	LLEndianSwapper tmp;
	tmp.d = netdouble;
	tmp.i = ll_ntohll(tmp.i);
	return tmp.d;
}
#endif

/**
 * Local functions.
 */
/**
 * @brief Figure out what kind of string it is (raw or delimited) and handoff.
 *
 * @param istr The stream to read from.
 * @param value [out] The string which was found.
 * @param max_bytes The maximum possible length of the string. Passing in
 * a negative value will skip this check.
 * @return Returns number of bytes read off of the stream. Returns
 * PARSE_FAILURE (-1) on failure.
 */
llssize deserialize_string(std::istream& istr, std::string& value, llssize max_bytes);

/**
 * @brief Parse a delimited string. 
 *
 * @param istr The stream to read from, with the delimiter already popped.
 * @param value [out] The string which was found.
 * @param d The delimiter to use.
 * @return Returns number of bytes read off of the stream. Returns
 * PARSE_FAILURE (-1) on failure.
 */
llssize deserialize_string_delim(std::istream& istr, std::string& value, char d);

/**
 * @brief Read a raw string off the stream.
 *
 * @param istr The stream to read from, with the (len) parameter
 * leading the stream.
 * @param value [out] The string which was found.
 * @param d The delimiter to use.
 * @param max_bytes The maximum possible length of the string. Passing in
 * a negative value will skip this check.
 * @return Returns number of bytes read off of the stream. Returns
 * PARSE_FAILURE (-1) on failure.
 */
llssize deserialize_string_raw(
	std::istream& istr,
	std::string& value,
	llssize max_bytes);

/**
 * @brief helper method for dealing with the different notation boolean format.
 *
 * @param istr The stream to read from with the leading character stripped.
 * @param data [out] the result of the parse.
 * @param compare The string to compare the boolean against
 * @param vale The value to assign to data if the parse succeeds.
 * @return Returns number of bytes read off of the stream. Returns
 * PARSE_FAILURE (-1) on failure.
 */
llssize deserialize_boolean(
	std::istream& istr,
	LLSD& data,
	const std::string& compare,
	bool value);

/**
 * @brief Do notation escaping of a string to an ostream.
 *
 * @param value The string to escape and serialize
 * @param str The stream to serialize to.
 */
void serialize_string(const std::string& value, std::ostream& str);


/**
 * Local constants.
 */
static const std::string NOTATION_TRUE_SERIAL("true");
static const std::string NOTATION_FALSE_SERIAL("false");

static const char BINARY_TRUE_SERIAL = '1';
static const char BINARY_FALSE_SERIAL = '0';


/**
 * LLSDParser
 */
LLSDParser::LLSDParser()
	: mCheckLimits(true), mMaxBytesLeft(0), mParseLines(false)
{
}

// virtual
LLSDParser::~LLSDParser()
{ }

S32 LLSDParser::parse(std::istream& istr, LLSD& data, llssize max_bytes, S32 max_depth)
{
	mCheckLimits = (LLSDSerialize::SIZE_UNLIMITED == max_bytes) ? false : true;
	mMaxBytesLeft = max_bytes;
	return doParse(istr, data, max_depth);
}


// Parse using routine to get() lines, faster than parse()
S32 LLSDParser::parseLines(std::istream& istr, LLSD& data)
{
	mCheckLimits = false;
	mParseLines = true;
	return doParse(istr, data);
}


int LLSDParser::get(std::istream& istr) const
{
	if(mCheckLimits) --mMaxBytesLeft;
	return istr.get();
}

std::istream& LLSDParser::get(
	std::istream& istr,
	char* s,
	std::streamsize n,
	char delim) const
{
	istr.get(s, n, delim);
	if(mCheckLimits) mMaxBytesLeft -= istr.gcount();
	return istr;
}

std::istream& LLSDParser::get(
		std::istream& istr,
		std::streambuf& sb,
		char delim) const		
{
	istr.get(sb, delim);
	if(mCheckLimits) mMaxBytesLeft -= istr.gcount();
	return istr;
}

std::istream& LLSDParser::ignore(std::istream& istr) const
{
	istr.ignore();
	if(mCheckLimits) --mMaxBytesLeft;
	return istr;
}

std::istream& LLSDParser::putback(std::istream& istr, char c) const
{
	istr.putback(c);
	if(mCheckLimits) ++mMaxBytesLeft;
	return istr;
}

std::istream& LLSDParser::read(
	std::istream& istr,
	char* s,
	std::streamsize n) const
{
	istr.read(s, n);
	if(mCheckLimits) mMaxBytesLeft -= istr.gcount();
	return istr;
}

void LLSDParser::account(llssize bytes) const
{
	if(mCheckLimits) mMaxBytesLeft -= bytes;
}


/**
 * LLSDNotationParser
 */
LLSDNotationParser::LLSDNotationParser()
{
}	

// virtual
LLSDNotationParser::~LLSDNotationParser()
{ }

// virtual
S32 LLSDNotationParser::doParse(std::istream& istr, LLSD& data, S32 max_depth) const
{
	// map: { string:object, string:object }
	// array: [ object, object, object ]
	// undef: !
	// boolean: true | false | 1 | 0 | T | F | t | f | TRUE | FALSE
	// integer: i####
	// real: r####
	// uuid: u####
	// string: "g'day" | 'have a "nice" day' | s(size)"raw data"
	// uri: l"escaped"
	// date: d"YYYY-MM-DDTHH:MM:SS.FFZ"
	// binary: b##"ff3120ab1" | b(size)"raw data"
	char c;
	c = istr.peek();
	if (max_depth == 0)
	{
		return PARSE_FAILURE;
	}
	while(isspace(c))
	{
		// pop the whitespace.
		c = get(istr);
		c = istr.peek();
		continue;
	}
	if(!istr.good())
	{
		return 0;
	}
	S32 parse_count = 1;
	switch(c)
	{
	case '{':
	{
		S32 child_count = parseMap(istr, data, max_depth - 1);
		if((child_count == PARSE_FAILURE) || data.isUndefined())
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			parse_count += child_count;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading map." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case '[':
	{
		S32 child_count = parseArray(istr, data, max_depth - 1);
		if((child_count == PARSE_FAILURE) || data.isUndefined())
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			parse_count += child_count;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading array." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case '!':
		c = get(istr);
		data.clear();
		break;

	case '0':
		c = get(istr);
		data = false;
		break;

	case 'F':
	case 'f':
		ignore(istr);
		c = istr.peek();
		if(isalpha(c))
		{
			auto cnt = deserialize_boolean(
				istr,
				data,
				NOTATION_FALSE_SERIAL,
				false);
			if(PARSE_FAILURE == cnt) parse_count = cnt;
			else account(cnt);
		}
		else
		{
			data = false;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading boolean." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;

	case '1':
		c = get(istr);
		data = true;
		break;

	case 'T':
	case 't':
		ignore(istr);
		c = istr.peek();
		if(isalpha(c))
		{
			auto cnt = deserialize_boolean(istr,data,NOTATION_TRUE_SERIAL,true);
			if(PARSE_FAILURE == cnt) parse_count = cnt;
			else account(cnt);
		}
		else
		{
			data = true;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading boolean." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;

	case 'i':
	{
		c = get(istr);
		S32 integer = 0;
		istr >> integer;
		data = integer;
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading integer." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case 'r':
	{
		c = get(istr);
		F64 real = 0.0;
		istr >> real;
		data = real;
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading real." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case 'u':
	{
		c = get(istr);
		LLUUID id;
		istr >> id;
		data = id;
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading uuid." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case '\"':
	case '\'':
	case 's':
		if(!parseString(istr, data))
		{
			parse_count = PARSE_FAILURE;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading string." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;

	case 'l':
	{
		c = get(istr); // pop the 'l'
		c = get(istr); // pop the delimiter
		std::string str;
		auto cnt = deserialize_string_delim(istr, str, c);
		if(PARSE_FAILURE == cnt)
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			data = LLURI(str);
			account(cnt);
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading link." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case 'd':
	{
		c = get(istr); // pop the 'd'
		c = get(istr); // pop the delimiter
		std::string str;
		auto cnt = deserialize_string_delim(istr, str, c);
		if(PARSE_FAILURE == cnt)
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			data = LLDate(str);
			account(cnt);
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading date." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case 'b':
		if(!parseBinary(istr, data))
		{
			parse_count = PARSE_FAILURE;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading data." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;

	default:
		parse_count = PARSE_FAILURE;
		LL_INFOS() << "Unrecognized character while parsing: int(" << int(c)
			<< ")" << LL_ENDL;
		break;
	}
	if(PARSE_FAILURE == parse_count)
	{
		data.clear();
	}
	return parse_count;
}

S32 LLSDNotationParser::parseMap(std::istream& istr, LLSD& map, S32 max_depth) const
{
	// map: { string:object, string:object }
	map = LLSD::emptyMap();
	S32 parse_count = 0;
	char c = get(istr);
	if(c == '{')
	{
		// eat commas, white
		bool found_name = false;
		std::string name;
		c = get(istr);
		while(c != '}' && istr.good())
		{
			if(!found_name)
			{
				if((c == '\"') || (c == '\'') || (c == 's'))
				{
					putback(istr, c);
					found_name = true;
					auto count = deserialize_string(istr, name, mMaxBytesLeft);
					if(PARSE_FAILURE == count) return PARSE_FAILURE;
					account(count);
				}
				c = get(istr);
			}
			else
			{
				if(isspace(c) || (c == ':'))
				{
					c = get(istr);
					continue;
				}
				putback(istr, c);
				LLSD child;
				S32 count = doParse(istr, child, max_depth);
				if(count > 0)
				{
					// There must be a value for every key, thus
					// child_count must be greater than 0.
					parse_count += count;
					map.insert(name, child);
				}
				else
				{
					return PARSE_FAILURE;
				}
				found_name = false;
				c = get(istr);
			}
		}
		if(c != '}')
		{
			map.clear();
			return PARSE_FAILURE;
		}
	}
	return parse_count;
}

S32 LLSDNotationParser::parseArray(std::istream& istr, LLSD& array, S32 max_depth) const
{
	// array: [ object, object, object ]
	array = LLSD::emptyArray();
	S32 parse_count = 0;
	char c = get(istr);
	if(c == '[')
	{
		// eat commas, white
		c = get(istr);
		while((c != ']') && istr.good())
		{
			LLSD child;
			if(isspace(c) || (c == ','))
			{
				c = get(istr);
				continue;
			}
			putback(istr, c);
			S32 count = doParse(istr, child, max_depth);
			if(PARSE_FAILURE == count)
			{
				return PARSE_FAILURE;
			}
			else
			{
				parse_count += count;
				array.append(child);
			}
			c = get(istr);
		}
		if(c != ']')
		{
			return PARSE_FAILURE;
		}
	}
	return parse_count;
}

bool LLSDNotationParser::parseString(std::istream& istr, LLSD& data) const
{
	std::string value;
	auto count = deserialize_string(istr, value, mMaxBytesLeft);
	if(PARSE_FAILURE == count) return false;
	account(count);
	data = value;
	return true;
}

bool LLSDNotationParser::parseBinary(std::istream& istr, LLSD& data) const
{
	// binary: b##"ff3120ab1"
	// or: b(len)"..."

	// I want to manually control those values here to make sure the
	// parser doesn't break when someone changes a constant somewhere
	// else.
	const U32 BINARY_BUFFER_SIZE = 256;
	const U32 STREAM_GET_COUNT = 255;

	// need to read the base out.
	char buf[BINARY_BUFFER_SIZE];		/* Flawfinder: ignore */
	get(istr, buf, STREAM_GET_COUNT, '"');
	char c = get(istr);
	if(c != '"') return false;
	if(0 == strncmp("b(", buf, 2))
	{
		// We probably have a valid raw binary stream. determine
		// the size, and read it.
		auto len = strtol(buf + 2, NULL, 0);
		if(mCheckLimits && (len > mMaxBytesLeft)) return false;
		std::vector<U8> value;
		if(len)
		{
			value.resize(len);
			account(fullread(istr, (char *)&value[0], len));
		}
		c = get(istr); // strip off the trailing double-quote
		data = value;
	}
	else if(0 == strncmp("b64", buf, 3))
	{
		// *FIX: A bit inefficient, but works for now. To make the
		// format better, I would need to add a hint into the
		// serialization format that indicated how long it was.
		std::stringstream coded_stream;
		get(istr, *(coded_stream.rdbuf()), '\"');
		c = get(istr);
		std::string encoded(coded_stream.str());
		S32 len = apr_base64_decode_len(encoded.c_str());
		std::vector<U8> value;
		if(len)
		{
			value.resize(len);
			len = apr_base64_decode_binary(&value[0], encoded.c_str());
			value.resize(len);
		}
		data = value;
	}
	else if(0 == strncmp("b16", buf, 3))
	{
		// yay, base 16. We pop the next character which is either a
		// double quote or base 16 data. If it's a double quote, we're
		// done parsing. If it's not, put the data back, and read the
		// stream until the next double quote.
		char* read;	 /*Flawfinder: ignore*/
		U8 byte;
		U8 byte_buffer[BINARY_BUFFER_SIZE];
		U8* write;
		std::vector<U8> value;
		c = get(istr);
		while(c != '"')
		{
			putback(istr, c);
			read = buf;
			write = byte_buffer;
			get(istr, buf, STREAM_GET_COUNT, '"');
			c = get(istr);
			while(*read != '\0')	 /*Flawfinder: ignore*/
			{
				byte = hex_as_nybble(*read++);
				byte = byte << 4;
				byte |= hex_as_nybble(*read++);
				*write++ = byte;
			}
			// copy the data out of the byte buffer
			value.insert(value.end(), byte_buffer, write);
		}
		data = value;
	}
	else
	{
		return false;
	}
	return true;
}


/**
 * LLSDBinaryParser
 */
LLSDBinaryParser::LLSDBinaryParser()
{
}

// virtual
LLSDBinaryParser::~LLSDBinaryParser()
{
}

// virtual
S32 LLSDBinaryParser::doParse(std::istream& istr, LLSD& data, S32 max_depth) const
{
/**
 * Undefined: '!'<br>
 * Boolean: '1' for true '0' for false<br>
 * Integer: 'i' + 4 bytes network byte order<br>
 * Real: 'r' + 8 bytes IEEE double<br>
 * UUID: 'u' + 16 byte unsigned integer<br>
 * String: 's' + 4 byte integer size + string<br>
 *  strings also secretly support the notation format
 * Date: 'd' + 8 byte IEEE double for seconds since epoch<br>
 * URI: 'l' + 4 byte integer size + string uri<br>
 * Binary: 'b' + 4 byte integer size + binary data<br>
 * Array: '[' + 4 byte integer size  + all values + ']'<br>
 * Map: '{' + 4 byte integer size  every(key + value) + '}'<br>
 *  map keys are serialized as s + 4 byte integer size + string or in the
 *  notation format.
 */
	char c;
	c = get(istr);
	if(!istr.good())
	{
		return 0;
	}
	if (max_depth == 0)
	{
		return PARSE_FAILURE;
	}
	S32 parse_count = 1;
	switch(c)
	{
	case '{':
	{
		S32 child_count = parseMap(istr, data, max_depth - 1);
		if((child_count == PARSE_FAILURE) || data.isUndefined())
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			parse_count += child_count;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary map." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case '[':
	{
		S32 child_count = parseArray(istr, data, max_depth - 1);
		if((child_count == PARSE_FAILURE) || data.isUndefined())
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			parse_count += child_count;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary array." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case '!':
		data.clear();
		break;

	case '0':
		data = false;
		break;

	case '1':
		data = true;
		break;

	case 'i':
	{
		U32 value_nbo = 0;
		read(istr, (char*)&value_nbo, sizeof(U32));	 /*Flawfinder: ignore*/
		data = (S32)ntohl(value_nbo);
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary integer." << LL_ENDL;
		}
		break;
	}

	case 'r':
	{
		F64 real_nbo = 0.0;
		read(istr, (char*)&real_nbo, sizeof(F64));	 /*Flawfinder: ignore*/
		data = ll_ntohd(real_nbo);
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary real." << LL_ENDL;
		}
		break;
	}

	case 'u':
	{
		LLUUID id;
		read(istr, (char*)(&id.mData), UUID_BYTES);	 /*Flawfinder: ignore*/
		data = id;
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary uuid." << LL_ENDL;
		}
		break;
	}

	case '\'':
	case '"':
	{
		std::string value;
		auto cnt = deserialize_string_delim(istr, value, c);
		if(PARSE_FAILURE == cnt)
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			data = value;
			account(cnt);
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary (notation-style) string."
				<< LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case 's':
	{
		std::string value;
		if(parseString(istr, value))
		{
			data = value;
		}
		else
		{
			parse_count = PARSE_FAILURE;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary string." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case 'l':
	{
		std::string value;
		if(parseString(istr, value))
		{
			data = LLURI(value);
		}
		else
		{
			parse_count = PARSE_FAILURE;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary link." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case 'd':
	{
		F64 real = 0.0;
		read(istr, (char*)&real, sizeof(F64));	 /*Flawfinder: ignore*/
		data = LLDate(real);
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary date." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	case 'b':
	{
		// We probably have a valid raw binary stream. determine
		// the size, and read it.
		U32 size_nbo = 0;
		read(istr, (char*)&size_nbo, sizeof(U32));	/*Flawfinder: ignore*/
		S32 size = (S32)ntohl(size_nbo);
		if(mCheckLimits && (size > mMaxBytesLeft))
		{
			parse_count = PARSE_FAILURE;
		}
		else
		{
			std::vector<U8> value;
			if(size > 0)
			{
				value.resize(size);
				account(fullread(istr, (char*)&value[0], size));
			}
			data = value;
		}
		if(istr.fail())
		{
			LL_INFOS() << "STREAM FAILURE reading binary." << LL_ENDL;
			parse_count = PARSE_FAILURE;
		}
		break;
	}

	default:
		parse_count = PARSE_FAILURE;
		LL_INFOS() << "Unrecognized character while parsing: int(" << int(c)
			<< ")" << LL_ENDL;
		break;
	}
	if(PARSE_FAILURE == parse_count)
	{
		data.clear();
	}
	return parse_count;
}

S32 LLSDBinaryParser::parseMap(std::istream& istr, LLSD& map, S32 max_depth) const
{
	map = LLSD::emptyMap();
	U32 value_nbo = 0;
	read(istr, (char*)&value_nbo, sizeof(U32));		 /*Flawfinder: ignore*/
	S32 size = (S32)ntohl(value_nbo);
	S32 parse_count = 0;
	S32 count = 0;
	char c = get(istr);
	while(c != '}' && (count < size) && istr.good())
	{
		std::string name;
		switch(c)
		{
		case 'k':
			if(!parseString(istr, name))
			{
				return PARSE_FAILURE;
			}
			break;
		case '\'':
		case '"':
		{
			auto cnt = deserialize_string_delim(istr, name, c);
			if(PARSE_FAILURE == cnt) return PARSE_FAILURE;
			account(cnt);
			break;
		}
		}
		LLSD child;
		S32 child_count = doParse(istr, child, max_depth);
		if(child_count > 0)
		{
			// There must be a value for every key, thus child_count
			// must be greater than 0.
			parse_count += child_count;
			map.insert(name, child);
		}
		else
		{
			return PARSE_FAILURE;
		}
		++count;
		c = get(istr);
	}
	if((c != '}') || (count < size))
	{
		// Make sure it is correctly terminated and we parsed as many
		// as were said to be there.
		return PARSE_FAILURE;
	}
	return parse_count;
}

S32 LLSDBinaryParser::parseArray(std::istream& istr, LLSD& array, S32 max_depth) const
{
	array = LLSD::emptyArray();
	U32 value_nbo = 0;
	read(istr, (char*)&value_nbo, sizeof(U32));		 /*Flawfinder: ignore*/
	S32 size = (S32)ntohl(value_nbo);

	// *FIX: This would be a good place to reserve some space in the
	// array...

	S32 parse_count = 0;
	S32 count = 0;
	char c = istr.peek();
	while((c != ']') && (count < size) && istr.good())
	{
		LLSD child;
		S32 child_count = doParse(istr, child, max_depth);
		if(PARSE_FAILURE == child_count)
		{
			return PARSE_FAILURE;
		}
		if(child_count)
		{
			parse_count += child_count;
			array.append(child);
		}
		++count;
		c = istr.peek();
	}
	c = get(istr);
	if((c != ']') || (count < size))
	{
		// Make sure it is correctly terminated and we parsed as many
		// as were said to be there.
		return PARSE_FAILURE;
	}
	return parse_count;
}

bool LLSDBinaryParser::parseString(
	std::istream& istr,
	std::string& value) const
{
	// *FIX: This is memory inefficient.
	U32 value_nbo = 0;
	read(istr, (char*)&value_nbo, sizeof(U32));		 /*Flawfinder: ignore*/
	S32 size = (S32)ntohl(value_nbo);
	if(mCheckLimits && (size > mMaxBytesLeft)) return false;
	if(size < 0) return false;
	std::vector<char> buf;
	if(size)
	{
		buf.resize(size);
		account(fullread(istr, &buf[0], size));
		value.assign(buf.begin(), buf.end());
	}
	return true;
}


/**
 * LLSDFormatter
 */
LLSDFormatter::LLSDFormatter(bool boolAlpha, const std::string& realFmt, EFormatterOptions options):
    mOptions(options)
{
    boolalpha(boolAlpha);
    realFormat(realFmt);
}

// virtual
LLSDFormatter::~LLSDFormatter()
{ }

void LLSDFormatter::boolalpha(bool alpha)
{
	mBoolAlpha = alpha;
}

void LLSDFormatter::realFormat(const std::string& format)
{
	mRealFormat = format;
}

S32 LLSDFormatter::format(const LLSD& data, std::ostream& ostr) const
{
    // pass options captured by constructor
    return format(data, ostr, mOptions);
}

S32 LLSDFormatter::format(const LLSD& data, std::ostream& ostr, EFormatterOptions options) const
{
    return format_impl(data, ostr, options, 0);
}

void LLSDFormatter::formatReal(LLSD::Real real, std::ostream& ostr) const
{
	std::string buffer = llformat(mRealFormat.c_str(), real);
	ostr << buffer;
}

/**
 * LLSDNotationFormatter
 */
LLSDNotationFormatter::LLSDNotationFormatter(bool boolAlpha, const std::string& realFormat,
                                             EFormatterOptions options):
    LLSDFormatter(boolAlpha, realFormat, options)
{
}

// virtual
LLSDNotationFormatter::~LLSDNotationFormatter()
{ }

// static
std::string LLSDNotationFormatter::escapeString(const std::string& in)
{
	std::ostringstream ostr;
	serialize_string(in, ostr);
	return ostr.str();
}

S32 LLSDNotationFormatter::format_impl(const LLSD& data, std::ostream& ostr,
									   EFormatterOptions options, U32 level) const
{
	S32 format_count = 1;
	std::string pre;
	std::string post;

	if (options & LLSDFormatter::OPTIONS_PRETTY)
	{
		for (U32 i = 0; i < level; i++)
		{
			pre += "    ";
		}
		post = "\n";
	}

	switch(data.type())
	{
	case LLSD::TypeMap:
	{
		if (0 != level) ostr << post << pre;
		ostr << "{";
		std::string inner_pre;
		if (options & LLSDFormatter::OPTIONS_PRETTY)
		{
			inner_pre = pre + "    ";
		}

		bool need_comma = false;
		LLSD::map_const_iterator iter = data.beginMap();
		LLSD::map_const_iterator end = data.endMap();
		for(; iter != end; ++iter)
		{
			if(need_comma) ostr << ",";
			need_comma = true;
			ostr << post << inner_pre << '\'';
			serialize_string((*iter).first, ostr);
			ostr << "':";
			format_count += format_impl((*iter).second, ostr, options, level + 2);
		}
		ostr << post << pre << "}";
		break;
	}

	case LLSD::TypeArray:
	{
		ostr << post << pre << "[";
		bool need_comma = false;
		LLSD::array_const_iterator iter = data.beginArray();
		LLSD::array_const_iterator end = data.endArray();
		for(; iter != end; ++iter)
		{
			if(need_comma) ostr << ",";
			need_comma = true;
			format_count += format_impl(*iter, ostr, options, level + 1);
		}
		ostr << "]";
		break;
	}

	case LLSD::TypeUndefined:
		ostr << "!";
		break;

	case LLSD::TypeBoolean:
		if(mBoolAlpha ||
#if( LL_WINDOWS || __GNUC__ > 2)
		   (ostr.flags() & std::ios::boolalpha)
#else
		   (ostr.flags() & 0x0100)
#endif
			)
		{
			ostr << (data.asBoolean()
					 ? NOTATION_TRUE_SERIAL : NOTATION_FALSE_SERIAL);
		}
		else
		{
			ostr << (data.asBoolean() ? 1 : 0);
		}
		break;

	case LLSD::TypeInteger:
		ostr << "i" << data.asInteger();
		break;

	case LLSD::TypeReal:
		ostr << "r";
		if(mRealFormat.empty())
		{
			ostr << data.asReal();
		}
		else
		{
			formatReal(data.asReal(), ostr);
		}
		break;

	case LLSD::TypeUUID:
		ostr << "u" << data.asUUID();
		break;

	case LLSD::TypeString:
		ostr << '\'';
		serialize_string(data.asStringRef(), ostr);
		ostr << '\'';
		break;

	case LLSD::TypeDate:
		ostr << "d\"" << data.asDate() << "\"";
		break;

	case LLSD::TypeURI:
		ostr << "l\"";
		serialize_string(data.asString(), ostr);
		ostr << "\"";
		break;

	case LLSD::TypeBinary:
	{
		// *FIX: memory inefficient.
		const std::vector<U8>& buffer = data.asBinary();
		if (options & LLSDFormatter::OPTIONS_PRETTY_BINARY)
		{
			ostr << "b16\"";
			if (! buffer.empty())
			{
				std::ios_base::fmtflags old_flags = ostr.flags();
				ostr.setf( std::ios::hex, std::ios::basefield );
				// It shouldn't strictly matter whether the emitted hex digits
				// are uppercase; LLSDNotationParser handles either; but as of
				// 2020-05-13, Python's llbase.llsd requires uppercase hex.
				ostr << std::uppercase;
				auto oldfill(ostr.fill('0'));
				auto oldwidth(ostr.width());
				for (size_t i = 0; i < buffer.size(); i++)
				{
					// have to restate setw() before every conversion
					ostr << std::setw(2) << (int) buffer[i];
				}
				ostr.width(oldwidth);
				ostr.fill(oldfill);
				ostr.flags(old_flags);
			}
		}
		else                        // ! OPTIONS_PRETTY_BINARY
		{
			ostr << "b(" << buffer.size() << ")\"";
			if (! buffer.empty())
			{
				ostr.write((const char*)&buffer[0], buffer.size());
			}
		}
		ostr << "\"";
		break;
	}

	default:
		// *NOTE: This should never happen.
		ostr << "!";
		break;
	}
	return format_count;
}

/**
 * LLSDBinaryFormatter
 */
LLSDBinaryFormatter::LLSDBinaryFormatter(bool boolAlpha, const std::string& realFormat,
                                         EFormatterOptions options):
    LLSDFormatter(boolAlpha, realFormat, options)
{
}

// virtual
LLSDBinaryFormatter::~LLSDBinaryFormatter()
{ }

// virtual
S32 LLSDBinaryFormatter::format_impl(const LLSD& data, std::ostream& ostr,
									 EFormatterOptions options, U32 level) const
{
	S32 format_count = 1;
	switch(data.type())
	{
	case LLSD::TypeMap:
	{
		ostr.put('{');
		U32 size_nbo = htonl(data.size());
		ostr.write((const char*)(&size_nbo), sizeof(U32));
		LLSD::map_const_iterator iter = data.beginMap();
		LLSD::map_const_iterator end = data.endMap();
		for(; iter != end; ++iter)
		{
			ostr.put('k');
			formatString((*iter).first, ostr);
			format_count += format_impl((*iter).second, ostr, options, level+1);
		}
		ostr.put('}');
		break;
	}

	case LLSD::TypeArray:
	{
		ostr.put('[');
		U32 size_nbo = htonl(data.size());
		ostr.write((const char*)(&size_nbo), sizeof(U32));
		LLSD::array_const_iterator iter = data.beginArray();
		LLSD::array_const_iterator end = data.endArray();
		for(; iter != end; ++iter)
		{
			format_count += format_impl(*iter, ostr, options, level+1);
		}
		ostr.put(']');
		break;
	}

	case LLSD::TypeUndefined:
		ostr.put('!');
		break;

	case LLSD::TypeBoolean:
		if(data.asBoolean()) ostr.put(BINARY_TRUE_SERIAL);
		else ostr.put(BINARY_FALSE_SERIAL);
		break;

	case LLSD::TypeInteger:
	{
		ostr.put('i');
		U32 value_nbo = htonl(data.asInteger());
		ostr.write((const char*)(&value_nbo), sizeof(U32));
		break;
	}

	case LLSD::TypeReal:
	{
		ostr.put('r');
		F64 value_nbo = ll_htond(data.asReal());
		ostr.write((const char*)(&value_nbo), sizeof(F64));
		break;
	}

	case LLSD::TypeUUID:
	{
		ostr.put('u');
		LLUUID temp = data.asUUID();
		ostr.write((const char*)(&(temp.mData)), UUID_BYTES);
		break;
	}

	case LLSD::TypeString:
		ostr.put('s');
		formatString(data.asStringRef(), ostr);
		break;

	case LLSD::TypeDate:
	{
		ostr.put('d');
		F64 value = data.asReal();
		ostr.write((const char*)(&value), sizeof(F64));
		break;
	}

	case LLSD::TypeURI:
		ostr.put('l');
		formatString(data.asString(), ostr);
		break;

	case LLSD::TypeBinary:
	{
		ostr.put('b');
		const std::vector<U8>& buffer = data.asBinary();
		U32 size_nbo = htonl(buffer.size());
		ostr.write((const char*)(&size_nbo), sizeof(U32));
		if(buffer.size()) ostr.write((const char*)&buffer[0], buffer.size());
		break;
	}

	default:
		// *NOTE: This should never happen.
		ostr.put('!');
		break;
	}
	return format_count;
}

void LLSDBinaryFormatter::formatString(
	const std::string& string,
	std::ostream& ostr) const
{
	U32 size_nbo = htonl(string.size());
	ostr.write((const char*)(&size_nbo), sizeof(U32));
	ostr.write(string.c_str(), string.size());
}

/**
 * local functions
 */
llssize deserialize_string(std::istream& istr, std::string& value, llssize max_bytes)
{
	int c = istr.get();
	if(istr.fail())
	{
		// No data in stream, bail out but mention the character we
		// grabbed.
		return LLSDParser::PARSE_FAILURE;
	}

	llssize rv = LLSDParser::PARSE_FAILURE;
	switch(c)
	{
	case '\'':
	case '"':
		rv = deserialize_string_delim(istr, value, c);
		break;
	case 's':
		// technically, less than max_bytes, but this is just meant to
		// catch egregious protocol errors. parse errors will be
		// caught in the case of incorrect counts.
		rv = deserialize_string_raw(istr, value, max_bytes);
		break;
	default:
		break;
	}
	if(LLSDParser::PARSE_FAILURE == rv) return rv;
	return rv + 1; // account for the character grabbed at the top.
}

llssize deserialize_string_delim(
	std::istream& istr,
	std::string& value,
	char delim)
{
	std::ostringstream write_buffer;
	bool found_escape = false;
	bool found_hex = false;
	bool found_digit = false;
	U8 byte = 0;
	llssize count = 0;

	while (true)
	{
		int next_byte = istr.get();
		++count;

		if(istr.fail())
		{
			// If our stream is empty, break out
			value = write_buffer.str();
			return LLSDParser::PARSE_FAILURE;
		}

		char next_char = (char)next_byte; // Now that we know it's not EOF

		if(found_escape)
		{
			// next character(s) is a special sequence.
			if(found_hex)
			{
				if(found_digit)
				{
					found_digit = false;
					found_hex = false;
					found_escape = false;
					byte = byte << 4;
					byte |= hex_as_nybble(next_char);
					write_buffer << byte;
					byte = 0;
				}
				else
				{
					// next character is the first nybble of
					//
					found_digit = true;
					byte = hex_as_nybble(next_char);
				}
			}
			else if(next_char == 'x')
			{
				found_hex = true;
			}
			else
			{
				switch(next_char)
				{
				case 'a':
					write_buffer << '\a';
					break;
				case 'b':
					write_buffer << '\b';
					break;
				case 'f':
					write_buffer << '\f';
					break;
				case 'n':
					write_buffer << '\n';
					break;
				case 'r':
					write_buffer << '\r';
					break;
				case 't':
					write_buffer << '\t';
					break;
				case 'v':
					write_buffer << '\v';
					break;
				default:
					write_buffer << next_char;
					break;
				}
				found_escape = false;
			}
		}
		else if(next_char == '\\')
		{
			found_escape = true;
		}
		else if(next_char == delim)
		{
			break;
		}
		else
		{
			write_buffer << next_char;
		}
	}

	value = write_buffer.str();
	return count;
}

llssize deserialize_string_raw(
	std::istream& istr,
	std::string& value,
	llssize max_bytes)
{
	llssize count = 0;
	const S32 BUF_LEN = 20;
	char buf[BUF_LEN];		/* Flawfinder: ignore */
	istr.get(buf, BUF_LEN - 1, ')');
	count += istr.gcount();
	int c = istr.get();
	c = istr.get();
	count += 2;
	if(((c == '"') || (c == '\'')) && (buf[0] == '('))
	{
		// We probably have a valid raw string. determine
		// the size, and read it.
		// *FIX: This is memory inefficient.
		auto len = strtol(buf + 1, NULL, 0);
		if((max_bytes>0)&&(len>max_bytes)) return LLSDParser::PARSE_FAILURE;
		std::vector<char> buf;
		if(len)
		{
			buf.resize(len);
			count += fullread(istr, (char *)&buf[0], len);
			value.assign(buf.begin(), buf.end());
		}
		c = istr.get();
		++count;
		if(!((c == '"') || (c == '\'')))
		{
			return LLSDParser::PARSE_FAILURE;
		}
	}
	else
	{
		return LLSDParser::PARSE_FAILURE;
	}
	return count;
}

static const char* NOTATION_STRING_CHARACTERS[256] =
{
	"\\x00",	// 0
	"\\x01",	// 1
	"\\x02",	// 2
	"\\x03",	// 3
	"\\x04",	// 4
	"\\x05",	// 5
	"\\x06",	// 6
	"\\a",		// 7
	"\\b",		// 8
	"\\t",		// 9
	"\\n",		// 10
	"\\v",		// 11
	"\\f",		// 12
	"\\r",		// 13
	"\\x0e",	// 14
	"\\x0f",	// 15
	"\\x10",	// 16
	"\\x11",	// 17
	"\\x12",	// 18
	"\\x13",	// 19
	"\\x14",	// 20
	"\\x15",	// 21
	"\\x16",	// 22
	"\\x17",	// 23
	"\\x18",	// 24
	"\\x19",	// 25
	"\\x1a",	// 26
	"\\x1b",	// 27
	"\\x1c",	// 28
	"\\x1d",	// 29
	"\\x1e",	// 30
	"\\x1f",	// 31
	" ",		// 32
	"!",		// 33
	"\"",		// 34
	"#",		// 35
	"$",		// 36
	"%",		// 37
	"&",		// 38
	"\\'",		// 39
	"(",		// 40
	")",		// 41
	"*",		// 42
	"+",		// 43
	",",		// 44
	"-",		// 45
	".",		// 46
	"/",		// 47
	"0",		// 48
	"1",		// 49
	"2",		// 50
	"3",		// 51
	"4",		// 52
	"5",		// 53
	"6",		// 54
	"7",		// 55
	"8",		// 56
	"9",		// 57
	":",		// 58
	";",		// 59
	"<",		// 60
	"=",		// 61
	">",		// 62
	"?",		// 63
	"@",		// 64
	"A",		// 65
	"B",		// 66
	"C",		// 67
	"D",		// 68
	"E",		// 69
	"F",		// 70
	"G",		// 71
	"H",		// 72
	"I",		// 73
	"J",		// 74
	"K",		// 75
	"L",		// 76
	"M",		// 77
	"N",		// 78
	"O",		// 79
	"P",		// 80
	"Q",		// 81
	"R",		// 82
	"S",		// 83
	"T",		// 84
	"U",		// 85
	"V",		// 86
	"W",		// 87
	"X",		// 88
	"Y",		// 89
	"Z",		// 90
	"[",		// 91
	"\\\\",		// 92
	"]",		// 93
	"^",		// 94
	"_",		// 95
	"`",		// 96
	"a",		// 97
	"b",		// 98
	"c",		// 99
	"d",		// 100
	"e",		// 101
	"f",		// 102
	"g",		// 103
	"h",		// 104
	"i",		// 105
	"j",		// 106
	"k",		// 107
	"l",		// 108
	"m",		// 109
	"n",		// 110
	"o",		// 111
	"p",		// 112
	"q",		// 113
	"r",		// 114
	"s",		// 115
	"t",		// 116
	"u",		// 117
	"v",		// 118
	"w",		// 119
	"x",		// 120
	"y",		// 121
	"z",		// 122
	"{",		// 123
	"|",		// 124
	"}",		// 125
	"~",		// 126
	"\\x7f",	// 127
	"\\x80",	// 128
	"\\x81",	// 129
	"\\x82",	// 130
	"\\x83",	// 131
	"\\x84",	// 132
	"\\x85",	// 133
	"\\x86",	// 134
	"\\x87",	// 135
	"\\x88",	// 136
	"\\x89",	// 137
	"\\x8a",	// 138
	"\\x8b",	// 139
	"\\x8c",	// 140
	"\\x8d",	// 141
	"\\x8e",	// 142
	"\\x8f",	// 143
	"\\x90",	// 144
	"\\x91",	// 145
	"\\x92",	// 146
	"\\x93",	// 147
	"\\x94",	// 148
	"\\x95",	// 149
	"\\x96",	// 150
	"\\x97",	// 151
	"\\x98",	// 152
	"\\x99",	// 153
	"\\x9a",	// 154
	"\\x9b",	// 155
	"\\x9c",	// 156
	"\\x9d",	// 157
	"\\x9e",	// 158
	"\\x9f",	// 159
	"\\xa0",	// 160
	"\\xa1",	// 161
	"\\xa2",	// 162
	"\\xa3",	// 163
	"\\xa4",	// 164
	"\\xa5",	// 165
	"\\xa6",	// 166
	"\\xa7",	// 167
	"\\xa8",	// 168
	"\\xa9",	// 169
	"\\xaa",	// 170
	"\\xab",	// 171
	"\\xac",	// 172
	"\\xad",	// 173
	"\\xae",	// 174
	"\\xaf",	// 175
	"\\xb0",	// 176
	"\\xb1",	// 177
	"\\xb2",	// 178
	"\\xb3",	// 179
	"\\xb4",	// 180
	"\\xb5",	// 181
	"\\xb6",	// 182
	"\\xb7",	// 183
	"\\xb8",	// 184
	"\\xb9",	// 185
	"\\xba",	// 186
	"\\xbb",	// 187
	"\\xbc",	// 188
	"\\xbd",	// 189
	"\\xbe",	// 190
	"\\xbf",	// 191
	"\\xc0",	// 192
	"\\xc1",	// 193
	"\\xc2",	// 194
	"\\xc3",	// 195
	"\\xc4",	// 196
	"\\xc5",	// 197
	"\\xc6",	// 198
	"\\xc7",	// 199
	"\\xc8",	// 200
	"\\xc9",	// 201
	"\\xca",	// 202
	"\\xcb",	// 203
	"\\xcc",	// 204
	"\\xcd",	// 205
	"\\xce",	// 206
	"\\xcf",	// 207
	"\\xd0",	// 208
	"\\xd1",	// 209
	"\\xd2",	// 210
	"\\xd3",	// 211
	"\\xd4",	// 212
	"\\xd5",	// 213
	"\\xd6",	// 214
	"\\xd7",	// 215
	"\\xd8",	// 216
	"\\xd9",	// 217
	"\\xda",	// 218
	"\\xdb",	// 219
	"\\xdc",	// 220
	"\\xdd",	// 221
	"\\xde",	// 222
	"\\xdf",	// 223
	"\\xe0",	// 224
	"\\xe1",	// 225
	"\\xe2",	// 226
	"\\xe3",	// 227
	"\\xe4",	// 228
	"\\xe5",	// 229
	"\\xe6",	// 230
	"\\xe7",	// 231
	"\\xe8",	// 232
	"\\xe9",	// 233
	"\\xea",	// 234
	"\\xeb",	// 235
	"\\xec",	// 236
	"\\xed",	// 237
	"\\xee",	// 238
	"\\xef",	// 239
	"\\xf0",	// 240
	"\\xf1",	// 241
	"\\xf2",	// 242
	"\\xf3",	// 243
	"\\xf4",	// 244
	"\\xf5",	// 245
	"\\xf6",	// 246
	"\\xf7",	// 247
	"\\xf8",	// 248
	"\\xf9",	// 249
	"\\xfa",	// 250
	"\\xfb",	// 251
	"\\xfc",	// 252
	"\\xfd",	// 253
	"\\xfe",	// 254
	"\\xff"		// 255
};

void serialize_string(const std::string& value, std::ostream& str)
{
	std::string::const_iterator it = value.begin();
	std::string::const_iterator end = value.end();
	U8 c;
	for(; it != end; ++it)
	{
		c = (U8)(*it);
		str << NOTATION_STRING_CHARACTERS[c];
	}
}

llssize deserialize_boolean(
	std::istream& istr,
	LLSD& data,
	const std::string& compare,
	bool value)
{
	//
	// this method is a little goofy, because it gets the stream at
	// the point where the t or f has already been
	// consumed. Basically, parse for a patch to the string passed in
	// starting at index 1. If it's a match:
	//  * assign data to value
	//  * return the number of bytes read
	// otherwise:
	//  * set data to LLSD::null
	//  * return LLSDParser::PARSE_FAILURE (-1)
	//
	llssize bytes_read = 0;
	std::string::size_type ii = 0;
	char c = istr.peek();
	while((++ii < compare.size())
		  && (tolower(c) == (int)compare[ii])
		  && istr.good())
	{
		istr.ignore();
		++bytes_read;
		c = istr.peek();
	}
	if(compare.size() != ii)
	{
		data.clear();
		return LLSDParser::PARSE_FAILURE;
	}
	data = value;
	return bytes_read;
}

std::ostream& operator<<(std::ostream& s, const LLSD& llsd)
{
	s << LLSDNotationStreamer(llsd);
	return s;
}


//dirty little zippers -- yell at davep if these are horrid

//return a string containing gzipped bytes of binary serialized LLSD
// VERY inefficient -- creates several copies of LLSD block in memory
std::string zip_llsd(LLSD& data)
{ 
	std::stringstream llsd_strm;

	LLSDSerialize::toBinary(data, llsd_strm);

	const U32 CHUNK = 65536;

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	S32 ret = deflateInit(&strm, Z_BEST_COMPRESSION);
	if (ret != Z_OK)
	{
		LL_WARNS() << "Failed to compress LLSD block." << LL_ENDL;
		return std::string();
	}

	std::string source = llsd_strm.str();

	U8 out[CHUNK];

	strm.avail_in = narrow(source.size());
	strm.next_in = (U8*) source.data();
	U8* output = NULL;

	U32 cur_size = 0;

	U32 have = 0;

	do
	{
		strm.avail_out = CHUNK;
		strm.next_out = out;

		ret = deflate(&strm, Z_FINISH);
		if (ret == Z_OK || ret == Z_STREAM_END)
		{ //copy result into output
			if (strm.avail_out >= CHUNK)
			{
				deflateEnd(&strm);
				if(output)
					free(output);
				LL_WARNS() << "Failed to compress LLSD block." << LL_ENDL;
				return std::string();
			}

			have = CHUNK-strm.avail_out;
			U8* new_output = (U8*) realloc(output, cur_size+have);
			if (new_output == NULL)
			{
				LL_WARNS() << "Failed to compress LLSD block: can't reallocate memory, current size: " << cur_size << " bytes; requested " << cur_size + have << " bytes." << LL_ENDL;
				deflateEnd(&strm);
				if (output)
				{
					free(output);
				}
				return std::string();
			}
			output = new_output;
			memcpy(output+cur_size, out, have);
			cur_size += have;
		}
		else 
		{
			deflateEnd(&strm);
			if(output)
				free(output);
			LL_WARNS() << "Failed to compress LLSD block." << LL_ENDL;
			return std::string();
		}
	}
	while (ret == Z_OK);

	std::string::size_type size = cur_size;

	std::string result((char*) output, size);
	deflateEnd(&strm);
	if(output)
		free(output);

	return result;
}

//decompress a block of LLSD from provided istream
// not very efficient -- creats a copy of decompressed LLSD block in memory
// and deserializes from that copy using LLSDSerialize
LLUZipHelper::EZipRresult LLUZipHelper::unzip_llsd(LLSD& data, std::istream& is, S32 size)
{
	std::unique_ptr<U8[]> in = std::unique_ptr<U8[]>(new(std::nothrow) U8[size]);
	if (!in)
	{
		return ZR_MEM_ERROR;
	}
	is.read((char*) in.get(), size); 

	return unzip_llsd(data, in.get(), size);
}

LLUZipHelper::EZipRresult LLUZipHelper::unzip_llsd(LLSD& data, const U8* in, S32 size)
{
	U8* result = NULL;
	llssize cur_size = 0;
	z_stream strm;

	constexpr U32 CHUNK = 1024 * 512;

	static thread_local std::unique_ptr<U8[]> out;
	if (!out)
	{
		out = std::unique_ptr<U8[]>(new(std::nothrow) U8[CHUNK]);
	}
		
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = size;
	strm.next_in = const_cast<U8*>(in);

	S32 ret = inflateInit(&strm);
	
	do
	{
		strm.avail_out = CHUNK;
		strm.next_out = out.get();
		ret = inflate(&strm, Z_NO_FLUSH);
		switch (ret)
		{
		case Z_NEED_DICT:
		case Z_DATA_ERROR:
		{
			inflateEnd(&strm);
			free(result);
			return ZR_DATA_ERROR;
		}
		case Z_STREAM_ERROR:
		case Z_BUF_ERROR:
		{
			inflateEnd(&strm);
			free(result);
			return ZR_BUFFER_ERROR;
		}

		case Z_MEM_ERROR:
		{
			inflateEnd(&strm);
			free(result);
			return ZR_MEM_ERROR;
		}
		}

		U32 have = CHUNK-strm.avail_out;

		U8* new_result = (U8*)realloc(result, cur_size + have);
		if (new_result == NULL)
		{
			inflateEnd(&strm);
			if (result)
			{
				free(result);
			}
			return ZR_MEM_ERROR;
		}
		result = new_result;
		memcpy(result+cur_size, out.get(), have);
		cur_size += have;

	} while (ret == Z_OK && ret != Z_STREAM_END);

	inflateEnd(&strm);

	if (ret != Z_STREAM_END)
	{
		free(result);
		return ZR_DATA_ERROR;
	}

	//result now points to the decompressed LLSD block
	{
		char* result_ptr = strip_deprecated_header((char*)result, cur_size);

		boost::iostreams::stream<boost::iostreams::array_source> istrm(result_ptr, cur_size);
		
		if (!LLSDSerialize::fromBinary(data, istrm, cur_size, UNZIP_LLSD_MAX_DEPTH))
		{
			free(result);
			return ZR_PARSE_ERROR;
		}
	}

	free(result);
	return ZR_OK;
}
//This unzip function will only work with a gzip header and trailer - while the contents
//of the actual compressed data is the same for either format (gzip vs zlib ), the headers
//and trailers are different for the formats.
U8* unzip_llsdNavMesh( bool& valid, size_t& outsize, std::istream& is, S32 size )
{
	if (size == 0)
	{
		LL_WARNS() << "No data to unzip." << LL_ENDL;
		return NULL;
	}

	U8* result = NULL;
	U32 cur_size = 0;
	z_stream strm;
		
	const U32 CHUNK = 0x4000;

	U8 *in = new(std::nothrow) U8[size];
	if (in == NULL)
	{
		LL_WARNS() << "Memory allocation failure." << LL_ENDL;
		return NULL;
	}
	is.read((char*) in, size); 

	U8 out[CHUNK];
		
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = size;
	strm.next_in = in;

	
	S32 ret = inflateInit2(&strm,  windowBits | ENABLE_ZLIB_GZIP );
	do
	{
		strm.avail_out = CHUNK;
		strm.next_out = out;
		ret = inflate(&strm, Z_NO_FLUSH);
		if (ret == Z_STREAM_ERROR)
		{
			inflateEnd(&strm);
			free(result);
			delete [] in;
			valid = false;
		}
		
		switch (ret)
		{
		case Z_NEED_DICT:
			ret = Z_DATA_ERROR;
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			inflateEnd(&strm);
			free(result);
			delete [] in;
			valid = false;
			break;
		}

		U32 have = CHUNK-strm.avail_out;

		U8* new_result = (U8*) realloc(result, cur_size + have);
		if (new_result == NULL)
		{
			LL_WARNS() << "Failed to unzip LLSD NavMesh block: can't reallocate memory, current size: " << cur_size
				<< " bytes; requested " << cur_size + have
				<< " bytes; total syze: ." << size << " bytes."
				<< LL_ENDL;
			inflateEnd(&strm);
			if (result)
			{
				free(result);
			}
			delete[] in;
			valid = false;
			return NULL;
		}
		result = new_result;
		memcpy(result+cur_size, out, have);
		cur_size += have;

	} while (ret == Z_OK);

	inflateEnd(&strm);
	delete [] in;

	if (ret != Z_STREAM_END)
	{
		free(result);
		valid = false;
		return NULL;
	}

	//result now points to the decompressed LLSD block
	{
		outsize= cur_size;
		valid = true;		
	}

	return result;
}

char* strip_deprecated_header(char* in, llssize& cur_size, llssize* header_size)
{
	const char* deprecated_header = "<? LLSD/Binary ?>";
	constexpr size_t deprecated_header_size = 17;

	if (cur_size > deprecated_header_size
		&& memcmp(in, deprecated_header, deprecated_header_size) == 0)
	{
		in = in + deprecated_header_size;
		cur_size = cur_size - deprecated_header_size;
		if (header_size)
		{
			*header_size = deprecated_header_size + 1;
		}
	}

	return in;
}

