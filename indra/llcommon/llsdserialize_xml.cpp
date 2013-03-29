/** 
 * @file llsdserialize_xml.cpp
 * @brief XML parsers and formatters for LLSD
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
#include "llsdserialize_xml.h"

#include <iostream>
#include <deque>

#include "apr_base64.h"
#include <boost/regex.hpp>

extern "C"
{
#ifdef LL_STANDALONE
# include <expat.h>
#else
# include "expat/expat.h"
#endif
}

/**
 * LLSDXMLFormatter
 */
LLSDXMLFormatter::LLSDXMLFormatter()
{
}

// virtual
LLSDXMLFormatter::~LLSDXMLFormatter()
{
}

// virtual
S32 LLSDXMLFormatter::format(const LLSD& data, std::ostream& ostr, U32 options) const
{
	std::streamsize old_precision = ostr.precision(25);

	std::string post;
	if (options & LLSDFormatter::OPTIONS_PRETTY)
	{
		post = "\n";
	}
	ostr << "<llsd>" << post;
	S32 rv = format_impl(data, ostr, options, 1);
	ostr << "</llsd>\n";

	ostr.precision(old_precision);
	return rv;
}

S32 LLSDXMLFormatter::format_impl(const LLSD& data, std::ostream& ostr, U32 options, U32 level) const
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
		if(0 == data.size())
		{
			ostr << pre << "<map />" << post;
		}
		else
		{
			ostr << pre << "<map>" << post;
			LLSD::map_const_iterator iter = data.beginMap();
			LLSD::map_const_iterator end = data.endMap();
			for(; iter != end; ++iter)
			{
				ostr << pre << "<key>" << escapeString((*iter).first) << "</key>" << post;
				format_count += format_impl((*iter).second, ostr, options, level + 1);
			}
			ostr << pre <<  "</map>" << post;
		}
		break;

	case LLSD::TypeArray:
		if(0 == data.size())
		{
			ostr << pre << "<array />" << post;
		}
		else
		{
			ostr << pre << "<array>" << post;
			LLSD::array_const_iterator iter = data.beginArray();
			LLSD::array_const_iterator end = data.endArray();
			for(; iter != end; ++iter)
			{
				format_count += format_impl(*iter, ostr, options, level + 1);
			}
			ostr << pre << "</array>" << post;
		}
		break;

	case LLSD::TypeUndefined:
		ostr << pre << "<undef />" << post;
		break;

	case LLSD::TypeBoolean:
		ostr << pre << "<boolean>";
		if(mBoolAlpha ||
		   (ostr.flags() & std::ios::boolalpha)
		   )
		{
			ostr << (data.asBoolean() ? "true" : "false");
		}
		else
		{
			ostr << (data.asBoolean() ? 1 : 0);
		}
		ostr << "</boolean>" << post;
		break;

	case LLSD::TypeInteger:
		ostr << pre << "<integer>" << data.asInteger() << "</integer>" << post;
		break;

	case LLSD::TypeReal:
		ostr << pre << "<real>";
		if(mRealFormat.empty())
		{
			ostr << data.asReal();
		}
		else
		{
			formatReal(data.asReal(), ostr);
		}
		ostr << "</real>" << post;
		break;

	case LLSD::TypeUUID:
		if(data.asUUID().isNull()) ostr << pre << "<uuid />" << post;
		else ostr << pre << "<uuid>" << data.asUUID() << "</uuid>" << post;
		break;

	case LLSD::TypeString:
		if(data.asString().empty()) ostr << pre << "<string />" << post;
		else ostr << pre << "<string>" << escapeString(data.asString()) <<"</string>" << post;
		break;

	case LLSD::TypeDate:
		ostr << pre << "<date>" << data.asDate() << "</date>" << post;
		break;

	case LLSD::TypeURI:
		ostr << pre << "<uri>" << escapeString(data.asString()) << "</uri>" << post;
		break;

	case LLSD::TypeBinary:
	{
		LLSD::Binary buffer = data.asBinary();
		if(buffer.empty())
		{
			ostr << pre << "<binary />" << post;
		}
		else
		{
			// *FIX: memory inefficient.
			// *TODO: convert to use LLBase64
			ostr << pre << "<binary encoding=\"base64\">";
			int b64_buffer_length = apr_base64_encode_len(buffer.size());
			char* b64_buffer = new char[b64_buffer_length];
			b64_buffer_length = apr_base64_encode_binary(
				b64_buffer,
				&buffer[0],
				buffer.size());
			ostr.write(b64_buffer, b64_buffer_length - 1);
			delete[] b64_buffer;
			ostr << "</binary>" << post;
		}
		break;
	}
	default:
		// *NOTE: This should never happen.
		ostr << pre << "<undef />" << post;
		break;
	}
	return format_count;
}

// static
std::string LLSDXMLFormatter::escapeString(const std::string& in)
{
	std::ostringstream out;
	std::string::const_iterator it = in.begin();
	std::string::const_iterator end = in.end();
	for(; it != end; ++it)
	{
		switch((*it))
		{
		case '<':
			out << "&lt;";
			break;
		case '>':
			out << "&gt;";
			break;
		case '&':
			out << "&amp;";
			break;
		case '\'':
			out << "&apos;";
			break;
		case '"':
			out << "&quot;";
			break;
		default:
			out << (*it);
			break;
		}
	}
	return out.str();
}



class LLSDXMLParser::Impl
{
public:
	Impl();
	~Impl();
	
	S32 parse(std::istream& input, LLSD& data);
	S32 parseLines(std::istream& input, LLSD& data);

	void parsePart(const char *buf, int len);
	
	void reset();

private:
	void startElementHandler(const XML_Char* name, const XML_Char** attributes);
	void endElementHandler(const XML_Char* name);
	void characterDataHandler(const XML_Char* data, int length);
	
	static void sStartElementHandler(
		void* userData, const XML_Char* name, const XML_Char** attributes);
	static void sEndElementHandler(
		void* userData, const XML_Char* name);
	static void sCharacterDataHandler(
		void* userData, const XML_Char* data, int length);

	void startSkipping();
	
	enum Element {
		ELEMENT_LLSD,
		ELEMENT_UNDEF,
		ELEMENT_BOOL,
		ELEMENT_INTEGER,
		ELEMENT_REAL,
		ELEMENT_STRING,
		ELEMENT_UUID,
		ELEMENT_DATE,
		ELEMENT_URI,
		ELEMENT_BINARY,
		ELEMENT_MAP,
		ELEMENT_ARRAY,
		ELEMENT_KEY,
		ELEMENT_UNKNOWN
	};
	static Element readElement(const XML_Char* name);
	
	static const XML_Char* findAttribute(const XML_Char* name, const XML_Char** pairs);
	

	XML_Parser	mParser;

	LLSD mResult;
	S32 mParseCount;
	
	bool mInLLSDElement;			// true if we're on LLSD
	bool mGracefullStop;			// true if we found the </llsd
	
	typedef std::deque<LLSD*> LLSDRefStack;
	LLSDRefStack mStack;
	
	int mDepth;
	bool mSkipping;
	int mSkipThrough;
	
	std::string mCurrentKey;		// Current XML <tag>
	std::string mCurrentContent;	// String data between <tag> and </tag>
};


LLSDXMLParser::Impl::Impl()
{
	mParser = XML_ParserCreate(NULL);
	reset();
}

LLSDXMLParser::Impl::~Impl()
{
	XML_ParserFree(mParser);
}

inline bool is_eol(char c)
{
	return (c == '\n' || c == '\r');
}

void clear_eol(std::istream& input)
{
	char c = input.peek();
	while (input.good() && is_eol(c))
	{
		input.get(c);
		c = input.peek();
	}
}

static unsigned get_till_eol(std::istream& input, char *buf, unsigned bufsize)
{
	unsigned count = 0;
	while (count < bufsize && input.good())
	{
		char c = input.get();
		buf[count++] = c;
		if (is_eol(c))
			break;
	}
	return count;
}

S32 LLSDXMLParser::Impl::parse(std::istream& input, LLSD& data)
{
	XML_Status status;
	
	static const int BUFFER_SIZE = 1024;
	void* buffer = NULL;	
	int count = 0;
	while (input.good() && !input.eof())
	{
		buffer = XML_GetBuffer(mParser, BUFFER_SIZE);

		/*
		 * If we happened to end our last buffer right at the end of the llsd, but the
		 * stream is still going we will get a null buffer here.  Check for mGracefullStop.
		 */
		if (!buffer)
		{
			break;
		}
		{
		
			count = get_till_eol(input, (char *)buffer, BUFFER_SIZE);
			if (!count)
			{
				break;
			}
		}
		status = XML_ParseBuffer(mParser, count, false);

		if (status == XML_STATUS_ERROR)
		{
			break;
		}
	}
	
	// *FIX.: This code is buggy - if the stream was empty or not
	// good, there is not buffer to parse, both the call to
	// XML_ParseBuffer and the buffer manipulations are illegal
	// futhermore, it isn't clear that the expat buffer semantics are
	// preserved

	status = XML_ParseBuffer(mParser, 0, true);
	if (status == XML_STATUS_ERROR && !mGracefullStop)
	{
		if (buffer)
		{
			((char*) buffer)[count ? count - 1 : 0] = '\0';
		}
		llinfos << "LLSDXMLParser::Impl::parse: XML_STATUS_ERROR parsing:" << (char*) buffer << llendl;
		data = LLSD();
		return LLSDParser::PARSE_FAILURE;
	}

	clear_eol(input);
	data = mResult;
	return mParseCount;
}


S32 LLSDXMLParser::Impl::parseLines(std::istream& input, LLSD& data)
{
	XML_Status status = XML_STATUS_OK;

	data = LLSD();

	static const int BUFFER_SIZE = 1024;

	//static char last_buffer[ BUFFER_SIZE ];
	//std::streamsize last_num_read;

	// Must get rid of any leading \n, otherwise the stream gets into an error/eof state
	clear_eol(input);

	while( !mGracefullStop
		&& input.good() 
		&& !input.eof())
	{
		void* buffer = XML_GetBuffer(mParser, BUFFER_SIZE);
		/*
		 * If we happened to end our last buffer right at the end of the llsd, but the
		 * stream is still going we will get a null buffer here.  Check for mGracefullStop.
		 * -- I don't think this is actually true - zero 2008-05-09
		 */
		if (!buffer)
		{
			break;
		}
		
		// Get one line
		input.getline((char*)buffer, BUFFER_SIZE);
		std::streamsize num_read = input.gcount();

		//memcpy( last_buffer, buffer, num_read );
		//last_num_read = num_read;

		if ( num_read > 0 )
		{
			if (!input.good() )
			{	// Clear state that's set when we run out of buffer
				input.clear();
			}
		
			// Re-insert with the \n that was absorbed by getline()
			char * text = (char *) buffer;
			if ( text[num_read - 1] == 0)
			{
				text[num_read - 1] = '\n';
			}
		}

		status = XML_ParseBuffer(mParser, (int)num_read, false);
		if (status == XML_STATUS_ERROR)
		{
			break;
		}
	}

	if (status != XML_STATUS_ERROR
		&& !mGracefullStop)
	{	// Parse last bit
		status = XML_ParseBuffer(mParser, 0, true);
	}
	
	if (status == XML_STATUS_ERROR  
		&& !mGracefullStop)
	{
		llinfos << "LLSDXMLParser::Impl::parseLines: XML_STATUS_ERROR" << llendl;
		return LLSDParser::PARSE_FAILURE;
	}

	clear_eol(input);
	data = mResult;
	return mParseCount;
}


void LLSDXMLParser::Impl::reset()
{
	mResult.clear();
	mParseCount = 0;

	mInLLSDElement = false;
	mDepth = 0;

	mGracefullStop = false;

	mStack.clear();
	
	mSkipping = false;
	
	mCurrentKey.clear();
	
	XML_ParserReset(mParser, "utf-8");
	XML_SetUserData(mParser, this);
	XML_SetElementHandler(mParser, sStartElementHandler, sEndElementHandler);
	XML_SetCharacterDataHandler(mParser, sCharacterDataHandler);
}


void LLSDXMLParser::Impl::startSkipping()
{
	mSkipping = true;
	mSkipThrough = mDepth;
}

const XML_Char*
LLSDXMLParser::Impl::findAttribute(const XML_Char* name, const XML_Char** pairs)
{
	while (NULL != pairs && NULL != *pairs)
	{
		if(0 == strcmp(name, *pairs))
		{
			return *(pairs + 1);
		}
		pairs += 2;
	}
	return NULL;
}

void LLSDXMLParser::Impl::parsePart(const char* buf, int len)
{
	if ( buf != NULL 
		&& len > 0 )
	{
		XML_Status status = XML_Parse(mParser, buf, len, false);
		if (status == XML_STATUS_ERROR)
		{
			llinfos << "Unexpected XML parsing error at start" << llendl;
		}
	}
}

// Performance testing code
//#define	XML_PARSER_PERFORMANCE_TESTS

#ifdef XML_PARSER_PERFORMANCE_TESTS

extern U64 totalTime();
U64	readElementTime = 0;
U64 startElementTime = 0;
U64 endElementTime = 0;
U64 charDataTime = 0;
U64 parseTime = 0;

class XML_Timer
{
public:
	XML_Timer( U64 * sum ) : mSum( sum )
	{
		mStart = totalTime();
	}
	~XML_Timer()
	{
		*mSum += (totalTime() - mStart);
	}

	U64 * mSum;
	U64 mStart;
};
#endif // XML_PARSER_PERFORMANCE_TESTS

void LLSDXMLParser::Impl::startElementHandler(const XML_Char* name, const XML_Char** attributes)
{
	#ifdef XML_PARSER_PERFORMANCE_TESTS
	XML_Timer timer( &startElementTime );
	#endif // XML_PARSER_PERFORMANCE_TESTS
	
	++mDepth;
	if (mSkipping)
	{
		return;
	}

	Element element = readElement(name);
	
	mCurrentContent.clear();

	switch (element)
	{
		case ELEMENT_LLSD:
			if (mInLLSDElement) { return startSkipping(); }
			mInLLSDElement = true;
			return;
	
		case ELEMENT_KEY:
			if (mStack.empty()  ||  !(mStack.back()->isMap()))
			{
				return startSkipping();
			}
			return;

		case ELEMENT_BINARY:
		{
			const XML_Char* encoding = findAttribute("encoding", attributes);
			if(encoding && strcmp("base64", encoding) != 0) { return startSkipping(); }
			break;
		}
		
		default:
			// all rest are values, fall through
			;
	}
	

	if (!mInLLSDElement) { return startSkipping(); }
	
	if (mStack.empty())
	{
		mStack.push_back(&mResult);
	}
	else if (mStack.back()->isMap())
	{
		if (mCurrentKey.empty()) { return startSkipping(); }
		
		LLSD& map = *mStack.back();
		LLSD& newElement = map[mCurrentKey];
		mStack.push_back(&newElement);		

		mCurrentKey.clear();
	}
	else if (mStack.back()->isArray())
	{
		LLSD& array = *mStack.back();
		array.append(LLSD());
		LLSD& newElement = array[array.size()-1];
		mStack.push_back(&newElement);
	}
	else {
		// improperly nested value in a non-structure
		return startSkipping();
	}

	++mParseCount;
	switch (element)
	{
		case ELEMENT_MAP:
			*mStack.back() = LLSD::emptyMap();
			break;
		
		case ELEMENT_ARRAY:
			*mStack.back() = LLSD::emptyArray();
			break;
			
		default:
			// all the other values will be set in the end element handler
			;
	}
}

void LLSDXMLParser::Impl::endElementHandler(const XML_Char* name)
{
	#ifdef XML_PARSER_PERFORMANCE_TESTS
	XML_Timer timer( &endElementTime );
	#endif // XML_PARSER_PERFORMANCE_TESTS

	--mDepth;
	if (mSkipping)
	{
		if (mDepth < mSkipThrough)
		{
			mSkipping = false;
		}
		return;
	}
	
	Element element = readElement(name);
	
	switch (element)
	{
		case ELEMENT_LLSD:
			if (mInLLSDElement)
			{
				mInLLSDElement = false;
				mGracefullStop = true;
				XML_StopParser(mParser, false);
			}
			return;
	
		case ELEMENT_KEY:
			mCurrentKey = mCurrentContent;
			return;
			
		default:
			// all rest are values, fall through
			;
	}
	
	if (!mInLLSDElement) { return; }

	LLSD& value = *mStack.back();
	mStack.pop_back();
	
	switch (element)
	{
		case ELEMENT_UNDEF:
			value.clear();
			break;
		
		case ELEMENT_BOOL:
			value = (mCurrentContent == "true" || mCurrentContent == "1");
			break;
		
		case ELEMENT_INTEGER:
			{
				S32 i;
				// sscanf okay here with different locales - ints don't change for different locale settings like floats do.
				if ( sscanf(mCurrentContent.c_str(), "%d", &i ) == 1 )
				{	// See if sscanf works - it's faster
					value = i;
				}
				else
				{
					value = LLSD(mCurrentContent).asInteger();
				}
			}
			break;
		
		case ELEMENT_REAL:
			{
				value = LLSD(mCurrentContent).asReal();
				// removed since this breaks when locale has decimal separator that isn't '.'
				// investigated changing local to something compatible each time but deemed higher
				// risk that just using LLSD.asReal() each time.
				//F64 r;
				//if ( sscanf(mCurrentContent.c_str(), "%lf", &r ) == 1 )
				//{	// See if sscanf works - it's faster
				//	value = r;
				//}
				//else
				//{
				//	value = LLSD(mCurrentContent).asReal();
				//}
			}
			break;
		
		case ELEMENT_STRING:
			value = mCurrentContent;
			break;
		
		case ELEMENT_UUID:
			value = LLSD(mCurrentContent).asUUID();
			break;
		
		case ELEMENT_DATE:
			value = LLSD(mCurrentContent).asDate();
			break;
		
		case ELEMENT_URI:
			value = LLSD(mCurrentContent).asURI();
			break;
		
		case ELEMENT_BINARY:
		{
			// Regex is expensive, but only fix for whitespace in base64,
			// created by python and other non-linden systems - DEV-39358
			// Fortunately we have very little binary passing now,
			// so performance impact shold be negligible. + poppy 2009-09-04
			boost::regex r;
			r.assign("\\s");
			std::string stripped = boost::regex_replace(mCurrentContent, r, "");
			S32 len = apr_base64_decode_len(stripped.c_str());
			std::vector<U8> data;
			data.resize(len);
			len = apr_base64_decode_binary(&data[0], stripped.c_str());
			data.resize(len);
			value = data;
			break;
		}
		
		case ELEMENT_UNKNOWN:
			value.clear();
			break;
			
		default:
			// other values, map and array, have already been set
			break;
	}

	mCurrentContent.clear();
}

void LLSDXMLParser::Impl::characterDataHandler(const XML_Char* data, int length)
{
	#ifdef XML_PARSER_PERFORMANCE_TESTS
	XML_Timer timer( &charDataTime );
	#endif	// XML_PARSER_PERFORMANCE_TESTS

	mCurrentContent.append(data, length);
}


void LLSDXMLParser::Impl::sStartElementHandler(
	void* userData, const XML_Char* name, const XML_Char** attributes)
{
	((LLSDXMLParser::Impl*)userData)->startElementHandler(name, attributes);
}

void LLSDXMLParser::Impl::sEndElementHandler(
	void* userData, const XML_Char* name)
{
	((LLSDXMLParser::Impl*)userData)->endElementHandler(name);
}

void LLSDXMLParser::Impl::sCharacterDataHandler(
	void* userData, const XML_Char* data, int length)
{
	((LLSDXMLParser::Impl*)userData)->characterDataHandler(data, length);
}


/*
	This code is time critical

	This is a sample of tag occurances of text in simstate file with ~8000 objects.
	A tag pair (<key>something</key>) counts is counted as two:

		key     - 2680178
		real    - 1818362
		integer -  906078
		array   -  295682
		map     -  191818
		uuid    -  177903
		binary  -  175748
		string  -   53482
		undef   -   40353
		boolean -   33874
		llsd    -   16332
		uri     -      38
		date    -       1
*/
LLSDXMLParser::Impl::Element LLSDXMLParser::Impl::readElement(const XML_Char* name)
{
	#ifdef XML_PARSER_PERFORMANCE_TESTS
	XML_Timer timer( &readElementTime );
	#endif // XML_PARSER_PERFORMANCE_TESTS

	XML_Char c = *name;
	switch (c)
	{
		case 'k':
			if (strcmp(name, "key") == 0) { return ELEMENT_KEY; }
			break;
		case 'r':
			if (strcmp(name, "real") == 0) { return ELEMENT_REAL; }
			break;
		case 'i':
			if (strcmp(name, "integer") == 0) { return ELEMENT_INTEGER; }
			break;
		case 'a':
			if (strcmp(name, "array") == 0) { return ELEMENT_ARRAY; }
			break;
		case 'm':
			if (strcmp(name, "map") == 0) { return ELEMENT_MAP; }
			break;
		case 'u':
			if (strcmp(name, "uuid") == 0) { return ELEMENT_UUID; }
			if (strcmp(name, "undef") == 0) { return ELEMENT_UNDEF; }
			if (strcmp(name, "uri") == 0) { return ELEMENT_URI; }
			break;
		case 'b':
			if (strcmp(name, "binary") == 0) { return ELEMENT_BINARY; }
			if (strcmp(name, "boolean") == 0) { return ELEMENT_BOOL; }
			break;
		case 's':
			if (strcmp(name, "string") == 0) { return ELEMENT_STRING; }
			break;
		case 'l':
			if (strcmp(name, "llsd") == 0) { return ELEMENT_LLSD; }
			break;
		case 'd':
			if (strcmp(name, "date") == 0) { return ELEMENT_DATE; }
			break;
	}
	return ELEMENT_UNKNOWN;
}





/**
 * LLSDXMLParser
 */
LLSDXMLParser::LLSDXMLParser() : impl(* new Impl)
{
}

LLSDXMLParser::~LLSDXMLParser()
{
	delete &impl;
}

void LLSDXMLParser::parsePart(const char *buf, int len)
{
	impl.parsePart(buf, len);
}

// virtual
S32 LLSDXMLParser::doParse(std::istream& input, LLSD& data) const
{
	#ifdef XML_PARSER_PERFORMANCE_TESTS
	XML_Timer timer( &parseTime );
	#endif	// XML_PARSER_PERFORMANCE_TESTS

	if (mParseLines)
	{
		// Use line-based reading (faster code)
		return impl.parseLines(input, data);
	}

	return impl.parse(input, data);
}

//	virtual 
void LLSDXMLParser::doReset()
{
	impl.reset();
}
