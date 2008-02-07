/** 
 * @file llsdserialize_xml.cpp
 * @brief XML parsers and formatters for LLSD
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
#include "llsdserialize_xml.h"

#include <iostream>
#include <deque>

#include "apr-1/apr_base64.h"

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

	LLString post = "";
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
	LLString pre = "";
	LLString post = "";

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
#if( LL_WINDOWS || __GNUC__ > 2)
		   (ostr.flags() & std::ios::boolalpha)
#else
		   (ostr.flags() & 0x0100)
#endif
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

	void parsePart(const char *buf, int len);
	
private:
	void reset();
	
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
	
	bool mInLLSDElement;
	bool mGracefullStop;
	
	typedef std::deque<LLSD*> LLSDRefStack;
	LLSDRefStack mStack;
	
	int mDepth;
	bool mSkipping;
	int mSkipThrough;
	
	std::string mCurrentKey;
	std::ostringstream mCurrentContent;

	bool mPreStaged;
};


LLSDXMLParser::Impl::Impl()
{
	mParser = XML_ParserCreate(NULL);
	mPreStaged = false;
	reset();
}

LLSDXMLParser::Impl::~Impl()
{
	XML_ParserFree(mParser);
}

bool is_eol(char c)
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
		input.get(buf[count]);
		count++;
		if (is_eol(buf[count - 1]))
			break;
	}
	return count;
}

S32 LLSDXMLParser::Impl::parse(std::istream& input, LLSD& data)
{
	reset();
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
		count = get_till_eol(input, (char *)buffer, BUFFER_SIZE);
		if (!count)
		{
			break;
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

void LLSDXMLParser::Impl::reset()
{
	if (mPreStaged)
	{
		mPreStaged = false;
		return;
	}

	mResult.clear();
	mParseCount = 0;

	mInLLSDElement = false;
	mDepth = 0;

	mGracefullStop = false;

	mStack.clear();
	
	mSkipping = false;
	
#if( LL_WINDOWS || __GNUC__ > 2)
	mCurrentKey.clear();
#else
	mCurrentKey = std::string();
#endif

	
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
	void * buffer = XML_GetBuffer(mParser, len);
	if (buffer != NULL && buf != NULL)
	{
		memcpy(buffer, buf, len);
	}
	XML_ParseBuffer(mParser, len, false);

	mPreStaged = true;
}

void LLSDXMLParser::Impl::startElementHandler(const XML_Char* name, const XML_Char** attributes)
{
	++mDepth;
	if (mSkipping)
	{
		return;
	}
	
	Element element = readElement(name);
	mCurrentContent.str("");

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

#if( LL_WINDOWS || __GNUC__ > 2)
		mCurrentKey.clear();
#else
		mCurrentKey = std::string();
#endif
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
			mCurrentKey = mCurrentContent.str();
			return;
			
		default:
			// all rest are values, fall through
			;
	}
	
	if (!mInLLSDElement) { return; }

	LLSD& value = *mStack.back();
	mStack.pop_back();
	
	std::string content = mCurrentContent.str();
	mCurrentContent.str("");

	switch (element)
	{
		case ELEMENT_UNDEF:
			value.clear();
			break;
		
		case ELEMENT_BOOL:
			value = content == "true" || content == "1";
			break;
		
		case ELEMENT_INTEGER:
			value = LLSD(content).asInteger();
			break;
		
		case ELEMENT_REAL:
			value = LLSD(content).asReal();
			break;
		
		case ELEMENT_STRING:
			value = content;
			break;
		
		case ELEMENT_UUID:
			value = LLSD(content).asUUID();
			break;
		
		case ELEMENT_DATE:
			value = LLSD(content).asDate();
			break;
		
		case ELEMENT_URI:
			value = LLSD(content).asURI();
			break;
		
		case ELEMENT_BINARY:
		{
			S32 len = apr_base64_decode_len(content.c_str());
			std::vector<U8> data;
			data.resize(len);
			len = apr_base64_decode_binary(&data[0], content.c_str());
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
}

void LLSDXMLParser::Impl::characterDataHandler(const XML_Char* data, int length)
{
	mCurrentContent.write(data, length);
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


LLSDXMLParser::Impl::Element LLSDXMLParser::Impl::readElement(const XML_Char* name)
{
	if (strcmp(name, "llsd") == 0) { return ELEMENT_LLSD; }
	if (strcmp(name, "undef") == 0) { return ELEMENT_UNDEF; }
	if (strcmp(name, "boolean") == 0) { return ELEMENT_BOOL; }
	if (strcmp(name, "integer") == 0) { return ELEMENT_INTEGER; }
	if (strcmp(name, "real") == 0) { return ELEMENT_REAL; }
	if (strcmp(name, "string") == 0) { return ELEMENT_STRING; }
	if (strcmp(name, "uuid") == 0) { return ELEMENT_UUID; }
	if (strcmp(name, "date") == 0) { return ELEMENT_DATE; }
	if (strcmp(name, "uri") == 0) { return ELEMENT_URI; }
	if (strcmp(name, "binary") == 0) { return ELEMENT_BINARY; }
	if (strcmp(name, "map") == 0) { return ELEMENT_MAP; }
	if (strcmp(name, "array") == 0) { return ELEMENT_ARRAY; }
	if (strcmp(name, "key") == 0) { return ELEMENT_KEY; }
	
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
	return impl.parse(input, data);	
}
