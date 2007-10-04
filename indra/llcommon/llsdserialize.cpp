/** 
 * @file llsdserialize.cpp
 * @author Phoenix
 * @date 2006-03-05
 * @brief Implementation of LLSD parsers and formatters
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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
#include "llsdserialize.h"
#include "llmemory.h"
#include "llstreamtools.h" // for fullread

#include <iostream>
#include "apr-1/apr_base64.h"

#if !LL_WINDOWS
#include <netinet/in.h> // htonl & ntohl
#endif

#include "lldate.h"
#include "llsd.h"
#include "lluri.h"

// File constants
static const int MAX_HDR_LEN = 20;
static const char LEGACY_NON_HEADER[] = "<llsd>";

//static
const char* LLSDSerialize::LLSDBinaryHeader = "LLSD/Binary";

//static
const char* LLSDSerialize::LLSDXMLHeader =    "LLSD/XML";

// virtual
LLSDParser::~LLSDParser()
{ }

// virtual
LLSDNotationParser::~LLSDNotationParser()
{ }


// static
void LLSDSerialize::serialize(const LLSD& sd, std::ostream& str, ELLSD_Serialize type, U32 options)
{
	LLPointer<LLSDFormatter> f = NULL;

	switch (type)
	{
	case LLSD_BINARY:
		str << "<? " << LLSDBinaryHeader << " ?>\n";
		f = new LLSDBinaryFormatter;
		break;

	case LLSD_XML:
		str << "<? " << LLSDXMLHeader << " ?>\n";
		f = new LLSDXMLFormatter;
		break;

	default:
		llwarns << "serialize request for unkown ELLSD_Serialize" << llendl;
	}

	if (f.notNull())
	{
		f->format(sd, str, options);
	}
}

// static
bool LLSDSerialize::deserialize(LLSD& sd, std::istream& str)
{
	LLPointer<LLSDParser> p = NULL;
	char hdr_buf[MAX_HDR_LEN + 1] = ""; /* Flawfinder: ignore */
	int i;
	int inbuf = 0;
	bool legacy_no_header = false;
	bool fail_if_not_legacy = false;
	std::string header = "";
	
	/*
	 * Get the first line before anything.
	 */
	str.get(hdr_buf, MAX_HDR_LEN, '\n');
	if (str.fail())
	{
		str.clear();
		fail_if_not_legacy = true;
	}

	if (!strncasecmp(LEGACY_NON_HEADER, hdr_buf, strlen(LEGACY_NON_HEADER))) /* Flawfinder: ignore */
	{
		legacy_no_header = true;
		inbuf = str.gcount();
	}
	else
	{
		if (fail_if_not_legacy)
			goto fail;
		/*
		* Remove the newline chars
		*/
		for (i = 0; i < MAX_HDR_LEN; i++)
		{
			if (hdr_buf[i] == 0 || hdr_buf[i] == '\r' ||
				hdr_buf[i] == '\n')
			{
				hdr_buf[i] = 0;
				break;
			}
		}
		header = hdr_buf;

		std::string::size_type start = std::string::npos;
		std::string::size_type end = std::string::npos;
		start = header.find_first_not_of("<? ");
		if (start != std::string::npos)
		{
			end = header.find_first_of(" ?", start);
		}
		if ((start == std::string::npos) || (end == std::string::npos))
			goto fail;

		header = header.substr(start, end - start);
		ws(str);
	}
	/*
	 * Create the parser as appropriate
	 */
	if (legacy_no_header)
	{
		LLSDXMLParser *x = new LLSDXMLParser;
		x->parsePart(hdr_buf, inbuf);
		p = x;
	}
	else if (header == LLSDBinaryHeader)
	{
		p = new LLSDBinaryParser;
	}
	else if (header == LLSDXMLHeader)
	{
		p = new LLSDXMLParser;
	}
	else
	{
		llwarns << "deserialize request for unknown ELLSD_Serialize" << llendl;
	}

	if (p.notNull())
	{
		p->parse(str, sd);
		return true;
	}

fail:
	llwarns << "deserialize LLSD parse failure" << llendl;
	return false;
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
bool deserialize_string(std::istream& str, std::string& value);
bool deserialize_string_delim(std::istream& str, std::string& value, char d);
bool deserialize_string_raw(std::istream& str, std::string& value);
void serialize_string(const std::string& value, std::ostream& str);

/**
 * Local constants.
 */
static const std::string NOTATION_TRUE_SERIAL("true");
static const std::string NOTATION_FALSE_SERIAL("false");

static const char BINARY_TRUE_SERIAL = '1';
static const char BINARY_FALSE_SERIAL = '0';

static const S32 NOTATION_PARSE_FAILURE = -1;

/**
 * LLSDParser
 */
LLSDParser::LLSDParser()
{
}

/**
 * LLSDNotationParser
 */
// virtual
S32 LLSDNotationParser::parse(std::istream& istr, LLSD& data) const
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
	while(isspace(c))
	{
		// pop the whitespace.
		c = istr.get();
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
		parse_count += parseMap(istr, data);
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading map." << llendl;
		}
		if(data.isUndefined())
		{
			parse_count = NOTATION_PARSE_FAILURE;
		}
		break;

	case '[':
		parse_count += parseArray(istr, data);
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading array." << llendl;
		}
		if(data.isUndefined())
		{
			parse_count = NOTATION_PARSE_FAILURE;
		}
		break;

	case '!':
		c = istr.get();
		data.clear();
		break;

	case '0':
		c = istr.get();
		data = false;
		break;

	case 'F':
	case 'f':
		do
		{
			istr.ignore();
			c = istr.peek();
		} while (isalpha(c));
		data = false;
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading boolean." << llendl;
		}
		break;

	case '1':
		c = istr.get();
		data = true;
		break;

	case 'T':
	case 't':
		do
		{
			istr.ignore();
			c = istr.peek();
		} while (isalpha(c));
		data = true;
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading boolean." << llendl;
		}
		break;

	case 'i':
	{
		c = istr.get();
		S32 integer = 0;
		istr >> integer;
		data = integer;
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading integer." << llendl;
		}
		break;
	}

	case 'r':
	{
		c = istr.get();
		F64 real = 0.0;
		istr >> real;
		data = real;
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading real." << llendl;
		}
		break;
	}

	case 'u':
	{
		c = istr.get();
		LLUUID id;
		istr >> id;
		data = id;
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading uuid." << llendl;
		}
		break;
	}

	case '\"':
	case '\'':
	case 's':
		parseString(istr, data);
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading string." << llendl;
		}
		if(data.isUndefined())
		{
			parse_count = NOTATION_PARSE_FAILURE;
		}
		break;

	case 'l':
	{
		c = istr.get(); // pop the 'l'
		c = istr.get(); // pop the delimiter
		std::string str;
		deserialize_string_delim(istr, str, c);
		data = LLURI(str);
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading link." << llendl;
		}
		break;
	}

	case 'd':
	{
		c = istr.get(); // pop the 'd'
		c = istr.get(); // pop the delimiter
		std::string str;
		deserialize_string_delim(istr, str, c);
		data = LLDate(str);
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading date." << llendl;
		}
		break;
	}

	case 'b':
		parseBinary(istr, data);
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading data." << llendl;
		}
		if(data.isUndefined())
		{
			parse_count = NOTATION_PARSE_FAILURE;
		}
		break;

	default:
		data.clear();
		parse_count = NOTATION_PARSE_FAILURE;
		llinfos << "Unrecognized character while parsing: int(" << (int)c
				<< ")" << llendl;
		break;
	}
	return parse_count;
}

// static
LLSD LLSDNotationParser::parse(std::istream& istr)
{
	LLSDNotationParser parser;
	LLSD rv;
	S32 count = parser.parse(istr, rv);
	lldebugs << "LLSDNotationParser::parse parsed " << count << " objects."
		 << llendl;
	return rv;
}

S32 LLSDNotationParser::parseMap(std::istream& istr, LLSD& map) const
{
	// map: { string:object, string:object }
	map = LLSD::emptyMap();
	S32 parse_count = 0;
	char c = istr.get();
	if(c == '{')
	{
		// eat commas, white
		bool found_name = false;
		std::string name;
		c = istr.get();
		while(c != '}' && istr.good())
		{
			if(!found_name)
			{
				if((c == '\"') || (c == '\'') || (c == 's'))
				{
					istr.putback(c);
					found_name = true;
					deserialize_string(istr, name);
				}
				c = istr.get();
			}
			else
			{
				if(isspace(c) || (c == ':'))
				{
					c = istr.get();
					continue;
				}
				istr.putback(c);
				LLSD child;
				S32 count = parse(istr, child);
				if(count > 0)
				{
					parse_count += count;
					map.insert(name, child);
				}
				else
				{
					map.clear();
					return NOTATION_PARSE_FAILURE;
				}
				found_name = false;
				c = istr.get();
			}
		}
	}
	return parse_count;
}

S32 LLSDNotationParser::parseArray(std::istream& istr, LLSD& array) const
{
	// array: [ object, object, object ]
	array = LLSD::emptyArray();
	S32 parse_count = 0;
	char c = istr.get();
	if(c == '[')
	{
		// eat commas, white
		c = istr.get();
		while((c != ']') && istr.good())
		{
			LLSD child;
			if(isspace(c) || (c == ','))
			{
				c = istr.get();
				continue;
			}
			istr.putback(c);
			S32 count = parse(istr, child);
			if(count > 0)
			{
				parse_count += count;
				array.append(child);
			}
			else
			{
				array.clear();
				return NOTATION_PARSE_FAILURE;
			}
			c = istr.get();
		}
	}
	return parse_count;
}

void LLSDNotationParser::parseString(std::istream& istr, LLSD& data) const
{
	std::string value;
	if(deserialize_string(istr, value))
	{
		data = value;
	}
	else
	{
		// failed to parse.
		data.clear();
	}
}

void LLSDNotationParser::parseBinary(std::istream& istr, LLSD& data) const
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
	istr.get(buf, STREAM_GET_COUNT, '"');
	char c = istr.get();
	if((c == '"') && (0 == strncmp("b(", buf, 2)))
	{
		// We probably have a valid raw binary stream. determine
		// the size, and read it.
		// *FIX: Should we set a maximum size?
		S32 len = strtol(buf + 2, NULL, 0);
		std::vector<U8> value;
		if(len)
		{
			value.resize(len);
			fullread(istr, (char *)&value[0], len);
		}
		c = istr.get(); // strip off the trailing double-quote
		data = value;
	}
	else if((c == '"') && (0 == strncmp("b64", buf, 3)))
	{
		// *FIX: A bit inefficient, but works for now. To make the
		// format better, I would need to add a hint into the
		// serialization format that indicated how long it was.
		std::stringstream coded_stream;
		istr.get(*(coded_stream.rdbuf()), '\"');
		c = istr.get();
		std::string encoded(coded_stream.str());
		S32 len = apr_base64_decode_len(encoded.c_str());
		std::vector<U8> value;
		value.resize(len);
		len = apr_base64_decode_binary(&value[0], encoded.c_str());
		value.resize(len);
		data = value;
	}
	else if((c == '"') && (0 == strncmp("b16", buf, 3)))
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
		c = istr.get();
		while(c != '"')
		{
			istr.putback(c);
			read = buf;
			write = byte_buffer;
			istr.get(buf, STREAM_GET_COUNT, '"');
			c = istr.get();
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
		data.clear();
	}
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
S32 LLSDBinaryParser::parse(std::istream& istr, LLSD& data) const
{
/**
 * Undefined: '!'<br>
 * Boolean: 't' for true 'f' for false<br>
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
	c = istr.get();
	if(!istr.good())
	{
		return 0;
	}
	S32 parse_count = 1;
	switch(c)
	{
	case '{':
		parse_count += parseMap(istr, data);
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading binary map." << llendl;
		}
		break;

	case '[':
		parse_count += parseArray(istr, data);
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading binary array." << llendl;
		}
		break;

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
		istr.read((char*)&value_nbo, sizeof(U32));	 /*Flawfinder: ignore*/
		data = (S32)ntohl(value_nbo);
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading binary integer." << llendl;
		}
		break;
	}

	case 'r':
	{
		F64 real_nbo = 0.0;
		istr.read((char*)&real_nbo, sizeof(F64));	 /*Flawfinder: ignore*/
		data = ll_ntohd(real_nbo);
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading binary real." << llendl;
		}
		break;
	}

	case 'u':
	{
		LLUUID id;
		istr.read((char*)(&id.mData), UUID_BYTES);	 /*Flawfinder: ignore*/
		data = id;
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading binary uuid." << llendl;
		}
		break;
	}

	case '\'':
	case '"':
	{
		std::string value;
		deserialize_string_delim(istr, value, c);
		data = value;
		break;
	}

	case 's':
	{
		std::string value;
		parseString(istr, value);
		data = value;
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading binary string." << llendl;
		}
		break;
	}

	case 'l':
	{
		std::string value;
		parseString(istr, value);
		data = LLURI(value);
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading binary link." << llendl;
		}
		break;
	}

	case 'd':
	{
		F64 real = 0.0;
		istr.read((char*)&real, sizeof(F64));	 /*Flawfinder: ignore*/
		data = LLDate(real);
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading binary date." << llendl;
		}
		break;
	}

	case 'b':
	{
		// We probably have a valid raw binary stream. determine
		// the size, and read it.
		// *FIX: Should we set a maximum size?
		U32 size_nbo = 0;
		istr.read((char*)&size_nbo, sizeof(U32));	/*Flawfinder: ignore*/
		S32 size = (S32)ntohl(size_nbo);
		std::vector<U8> value;
		if(size)
		{
			value.resize(size);
			istr.read((char*)&value[0], size);		 /*Flawfinder: ignore*/
		}
		data = value;
		if(istr.fail())
		{
			llinfos << "STREAM FAILURE reading binary." << llendl;
		}
		break;
	}

	default:
		--parse_count;
		llinfos << "Unrecognized character while parsing: int(" << (int)c
				<< ")" << llendl;
		break;
	}
	return parse_count;
}

// static
LLSD LLSDBinaryParser::parse(std::istream& istr)
{
	LLSDBinaryParser parser;
	LLSD rv;
	S32 count = parser.parse(istr, rv);
	lldebugs << "LLSDBinaryParser::parse parsed " << count << " objects."
		 << llendl;
	return rv;
}

S32 LLSDBinaryParser::parseMap(std::istream& istr, LLSD& map) const
{
	map = LLSD::emptyMap();
	U32 value_nbo = 0;
	istr.read((char*)&value_nbo, sizeof(U32));		 /*Flawfinder: ignore*/
	S32 size = (S32)ntohl(value_nbo);
	S32 parse_count = 0;
	S32 count = 0;
	char c = istr.get();
	while(c != '}' && (count < size) && istr.good())
	{
		std::string name;
		switch(c)
		{
		case 'k':
			parseString(istr, name);
			break;
		case '\'':
		case '"':
			deserialize_string_delim(istr, name, c);
			break;
		}
		LLSD child;
		S32 child_count = parse(istr, child);
		if(child_count)
		{
			parse_count += child_count;
			map.insert(name, child);
		}
		++count;
		c = istr.get();
	}
	return parse_count;
}

S32 LLSDBinaryParser::parseArray(std::istream& istr, LLSD& array) const
{
	array = LLSD::emptyArray();
	U32 value_nbo = 0;
	istr.read((char*)&value_nbo, sizeof(U32));		 /*Flawfinder: ignore*/
	S32 size = (S32)ntohl(value_nbo);

	// *FIX: This would be a good place to reserve some space in the
	// array...

	S32 parse_count = 0;
	S32 count = 0;
	char c = istr.peek();
	while((c != ']') && (count < size) && istr.good())
	{
		LLSD child;
		S32 child_count = parse(istr, child);
		if(child_count)
		{
			parse_count += child_count;
			array.append(child);
		}
		++count;
		c = istr.peek();
	}
	c = istr.get();
	return parse_count;
}

void LLSDBinaryParser::parseString(
	std::istream& istr,
	std::string& value) const
{
	// *FIX: This is memory inefficient.
	U32 value_nbo = 0;
	istr.read((char*)&value_nbo, sizeof(U32));		 /*Flawfinder: ignore*/
	S32 size = (S32)ntohl(value_nbo);
	std::vector<char> buf;
	buf.resize(size);
	istr.read(&buf[0], size);		 /*Flawfinder: ignore*/
	value.assign(buf.begin(), buf.end());
}


/**
 * LLSDFormatter
 */
LLSDFormatter::LLSDFormatter() :
	mBoolAlpha(false)
{
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

void LLSDFormatter::formatReal(LLSD::Real real, std::ostream& ostr) const
{
	char buffer[MAX_STRING];		/* Flawfinder: ignore */
	snprintf(buffer, MAX_STRING, mRealFormat.c_str(), real);	/* Flawfinder: ignore */
	ostr << buffer;
}

/**
 * LLSDNotationFormatter
 */
LLSDNotationFormatter::LLSDNotationFormatter()
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

// virtual
S32 LLSDNotationFormatter::format(const LLSD& data, std::ostream& ostr, U32 options) const
{
	S32 format_count = 1;
	switch(data.type())
	{
	case LLSD::TypeMap:
	{
		ostr << "{";
		bool need_comma = false;
		LLSD::map_const_iterator iter = data.beginMap();
		LLSD::map_const_iterator end = data.endMap();
		for(; iter != end; ++iter)
		{
			if(need_comma) ostr << ",";
			need_comma = true;
			ostr << '\'';
			serialize_string((*iter).first, ostr);
			ostr << "':";
			format_count += format((*iter).second, ostr);
		}
		ostr << "}";
		break;
	}

	case LLSD::TypeArray:
	{
		ostr << "[";
		bool need_comma = false;
		LLSD::array_const_iterator iter = data.beginArray();
		LLSD::array_const_iterator end = data.endArray();
		for(; iter != end; ++iter)
		{
			if(need_comma) ostr << ",";
			need_comma = true;
			format_count += format(*iter, ostr);
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
		serialize_string(data.asString(), ostr);
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
		std::vector<U8> buffer = data.asBinary();
		ostr << "b(" << buffer.size() << ")\"";
		if(buffer.size()) ostr.write((const char*)&buffer[0], buffer.size());
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
LLSDBinaryFormatter::LLSDBinaryFormatter()
{
}

// virtual
LLSDBinaryFormatter::~LLSDBinaryFormatter()
{ }

// virtual
S32 LLSDBinaryFormatter::format(const LLSD& data, std::ostream& ostr, U32 options) const
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
			format_count += format((*iter).second, ostr);
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
			format_count += format(*iter, ostr);
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
		ostr.put('u');
		ostr.write((const char*)(&(data.asUUID().mData)), UUID_BYTES);
		break;

	case LLSD::TypeString:
		ostr.put('s');
		formatString(data.asString(), ostr);
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
		// *FIX: memory inefficient.
		ostr.put('b');
		std::vector<U8> buffer = data.asBinary();
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
bool deserialize_string(std::istream& str, std::string& value)
{
	char c = str.get();
	if (str.fail())
	{
		// No data in stream, bail out
		return false;
	}

	bool rv = false;
	switch(c)
	{
	case '\'':
	case '"':
		rv = deserialize_string_delim(str, value, c);
		break;
	case 's':
		rv = deserialize_string_raw(str, value);
		break;
	default:
		break;
	}
	return rv;
}

bool deserialize_string_delim(
	std::istream& str,
	std::string& value,
	char delim)
{
	std::ostringstream write_buffer;
	bool found_escape = false;
	bool found_hex = false;
	bool found_digit = false;
	U8 byte = 0;
	
	while (true)
	{
		char next_char = str.get();
		
		if(str.fail())
		{
			// If our stream is empty, break out
			value = write_buffer.str();
			return false;
		}
		
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
	return true;
}

bool deserialize_string_raw(std::istream& str, std::string& value)
{
	bool ok = false;
	const S32 BUF_LEN = 20;
	char buf[BUF_LEN];		/* Flawfinder: ignore */
	str.get(buf, BUF_LEN - 1, ')');
	char c = str.get();
	c = str.get();
	if(((c == '"') || (c == '\'')) && (buf[0] == '('))
	{
		// We probably have a valid raw string. determine
		// the size, and read it.
		// *FIX: Should we set a maximum size?
		// *FIX: This is memory inefficient.
		S32 len = strtol(buf + 1, NULL, 0);
		std::vector<char> buf;
		buf.resize(len);
		str.read(&buf[0], len);		 /*Flawfinder: ignore*/
		value.assign(buf.begin(), buf.end());
		c = str.get();
		if((c == '"') || (c == '\''))
		{
			ok = true;
		}
	}
	return ok;
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


