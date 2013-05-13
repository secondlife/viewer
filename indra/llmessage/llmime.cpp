/** 
 * @file llmime.cpp
 * @author Phoenix
 * @date 2006-12-20
 * @brief Implementation of mime tools.
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
#include "llmime.h"

#include <vector>

#include "llmemorystream.h"

/**
 * Useful constants.
 */
// Headers specified in rfc-2045 will be canonicalized below.
static const std::string CONTENT_LENGTH("Content-Length");
static const std::string CONTENT_TYPE("Content-Type");
static const S32 KNOWN_HEADER_COUNT = 6;
static const std::string KNOWN_HEADER[KNOWN_HEADER_COUNT] =
{
	CONTENT_LENGTH,
	CONTENT_TYPE,
	std::string("MIME-Version"),
	std::string("Content-Transfer-Encoding"),
	std::string("Content-ID"),
	std::string("Content-Description"),
};

// parser helpers
static const std::string MULTIPART("multipart");
static const std::string BOUNDARY("boundary");
static const std::string END_OF_CONTENT_PARAMETER("\r\n ;\t");
static const std::string SEPARATOR_PREFIX("--");
//static const std::string SEPARATOR_SUFFIX("\r\n");

/*
Content-Type: multipart/mixed; boundary="segment"
Content-Length: 24832

--segment
Content-Type: image/j2c
Content-Length: 23715

<data>

--segment
Content-Type: text/xml; charset=UTF-8

<meta data>
EOF

*/

/**
 * LLMimeIndex
 */

/** 
 * @class LLMimeIndex::Impl
 * @brief Implementation details of the mime index class.
 * @see LLMimeIndex
 */
class LLMimeIndex::Impl
{
public:
	Impl() : mOffset(-1), mUseCount(1)
	{}
	Impl(LLSD headers, S32 offset) :
		mHeaders(headers), mOffset(offset), mUseCount(1)
	{}
public:
	LLSD mHeaders;
	S32 mOffset;
	S32 mUseCount;

	typedef std::vector<LLMimeIndex> sub_part_t;
	sub_part_t mAttachments;
};

LLSD LLMimeIndex::headers() const
{
	return mImpl->mHeaders;
}

S32 LLMimeIndex::offset() const
{
	return mImpl->mOffset;
}

S32 LLMimeIndex::contentLength() const
{
	// Find the content length in the headers.
	S32 length = -1;
	LLSD content_length = mImpl->mHeaders[CONTENT_LENGTH];
	if(content_length.isDefined())
	{
		length = content_length.asInteger();
	}
	return length;
}

std::string LLMimeIndex::contentType() const
{
	std::string type;
	LLSD content_type = mImpl->mHeaders[CONTENT_TYPE];
	if(content_type.isDefined())
	{
		type = content_type.asString();
	}
	return type;
}

bool LLMimeIndex::isMultipart() const
{
	bool multipart = false;
	LLSD content_type = mImpl->mHeaders[CONTENT_TYPE];
	if(content_type.isDefined())
	{
		std::string type = content_type.asString();
		int comp = type.compare(0, MULTIPART.size(), MULTIPART);
		if(0 == comp)
		{
			multipart = true;
		}
	}
	return multipart;
}

S32 LLMimeIndex::subPartCount() const
{
	return mImpl->mAttachments.size();
}

LLMimeIndex LLMimeIndex::subPart(S32 index) const
{
	LLMimeIndex part;
	if((index >= 0) && (index < (S32)mImpl->mAttachments.size()))
	{
		part = mImpl->mAttachments[index];
	}
	return part;
}

LLMimeIndex::LLMimeIndex() : mImpl(new LLMimeIndex::Impl)
{
}

LLMimeIndex::LLMimeIndex(LLSD headers, S32 content_offset) :
	mImpl(new LLMimeIndex::Impl(headers, content_offset))
{
}

LLMimeIndex::LLMimeIndex(const LLMimeIndex& mime) :
	mImpl(mime.mImpl)
{
	++mImpl->mUseCount;
}

LLMimeIndex::~LLMimeIndex()
{
	if(0 == --mImpl->mUseCount)
	{
		delete mImpl;
	}
}

LLMimeIndex& LLMimeIndex::operator=(const LLMimeIndex& mime)
{
	// Increment use count first so that we handle self assignment
	// automatically.
	++mime.mImpl->mUseCount;
	if(0 == --mImpl->mUseCount)
	{
		delete mImpl;
	}
	mImpl = mime.mImpl;
	return *this;
}

bool LLMimeIndex::attachSubPart(LLMimeIndex sub_part)
{
	// *FIX: Should we check for multi-part?
	if(mImpl->mAttachments.size() < S32_MAX)
	{
		mImpl->mAttachments.push_back(sub_part);
		return true;
	}
	return false;
}

/**
 * LLMimeParser
 */
/** 
 * @class LLMimeParser::Impl
 * @brief Implementation details of the mime parser class.
 * @see LLMimeParser
 */
class LLMimeParser::Impl
{
public:
	// @brief Constructor.
	Impl();

	// @brief Reset this for a new parse.
	void reset();

	/** 
	 * @brief Parse a mime entity to find the index information.
	 *
	 * This method will scan the istr until a single complete mime
	 * entity is read, an EOF, or limit bytes have been scanned. The
	 * istr will be modified by this parsing, so pass in a temporary
	 * stream or rewind/reset the stream after this call.
	 * @param istr An istream which contains a mime entity.
	 * @param limit The maximum number of bytes to scan.
	 * @param separator The multipart separator if it is known.
	 * @param is_subpart Set true if parsing a multipart sub part.
	 * @param index[out] The parsed output.
	 * @return Returns true if an index was parsed and no errors occurred.
	 */
	bool parseIndex(
		std::istream& istr,
		S32 limit,
		const std::string& separator,
		bool is_subpart,
		LLMimeIndex& index);

protected:
	/**
	 * @brief parse the headers.
	 *
	 * At the end of a successful parse, mScanCount will be at the
	 * start of the content.
	 * @param istr The input stream.
	 * @param limit maximum number of bytes to process
	 * @param headers[out] A map of the headers found.
	 * @return Returns true if the parse was successful.
	 */
	bool parseHeaders(std::istream& istr, S32 limit, LLSD& headers);

	/**
	 * @brief Figure out the separator string from a content type header.
	 * 
	 * @param multipart_content_type The content type value from the headers.
	 * @return Returns the separator string.
	 */
	std::string findSeparator(std::string multipart_content_type);

	/**
	 * @brief Scan through istr past the separator.
	 *
	 * @param istr The input stream.
	 * @param limit Maximum number of bytes to scan.
	 * @param separator The multipart separator.
	 */
	void scanPastSeparator(
		std::istream& istr,
		S32 limit,
		const std::string& separator);

	/**
	 * @brief Scan through istr past the content of the current mime part.
	 *
	 * @param istr The input stream.
	 * @param limit Maximum number of bytes to scan.
	 * @param headers The headers for this mime part.
	 * @param separator The multipart separator if known.
	 */
	void scanPastContent(
		std::istream& istr,
		S32 limit,
		LLSD headers,
		const std::string separator);

	/**
	 * @brief Eat CRLF.
	 *
	 * This method has no concept of the limit, so ensure you have at
	 * least 2 characters left to eat before hitting the limit. This
	 * method will increment mScanCount as it goes.
	 * @param istr The input stream.
	 * @return Returns true if CRLF was found and consumed off of istr.
	 */
	bool eatCRLF(std::istream& istr);

	// @brief Returns true if parsing should continue.
	bool continueParse() const { return (!mError && mContinue); }

	// @brief anonymous enumeration for parse buffer size.
	enum
	{
		LINE_BUFFER_LENGTH = 1024
	};

protected:
	S32 mScanCount;
	bool mContinue;
	bool mError;
	char mBuffer[LINE_BUFFER_LENGTH];
};

LLMimeParser::Impl::Impl()
{
	reset();
}

void LLMimeParser::Impl::reset()
{
	mScanCount = 0;
	mContinue = true;
	mError = false;
	mBuffer[0] = '\0';
}

bool LLMimeParser::Impl::parseIndex(
	std::istream& istr,
	S32 limit,
	const std::string& separator,
	bool is_subpart,
	LLMimeIndex& index)
{
	LLSD headers;
	bool parsed_something = false;
	if(parseHeaders(istr, limit, headers))
	{
		parsed_something = true;
		LLMimeIndex mime(headers, mScanCount);
		index = mime;
		if(index.isMultipart())
		{
			// Figure out the separator, scan past it, and recurse.
			std::string ct = headers[CONTENT_TYPE].asString();
			std::string sep = findSeparator(ct);
			scanPastSeparator(istr, limit, sep);
			while(continueParse() && parseIndex(istr, limit, sep, true, mime))
			{
				index.attachSubPart(mime);
			}
		}
		else
		{
			// Scan to the end of content.
			scanPastContent(istr, limit, headers, separator);
			if(is_subpart)
			{
				scanPastSeparator(istr, limit, separator);
			}
		}
	}
	if(mError) return false;
	return parsed_something;
}

bool LLMimeParser::Impl::parseHeaders(
	std::istream& istr,
	S32 limit,
	LLSD& headers)
{
	while(continueParse())
	{
		// Get the next line.
		// We subtract 1 from the limit so that we make sure
		// not to read past limit when we get() the newline.
		S32 max_get = llmin((S32)LINE_BUFFER_LENGTH, limit - mScanCount - 1);
		istr.getline(mBuffer, max_get, '\r');
		mScanCount += (S32)istr.gcount();
		int c = istr.get();
		if(EOF == c)
		{
			mContinue = false;
			return false;
		}
		++mScanCount;
		if(c != '\n')
		{
			mError = true;
			return false;
		}
		if(mScanCount >= limit)
		{
			mContinue = false;
		}

		// Check if that's the end of headers.
		if('\0' == mBuffer[0])
		{
			break;
		}

		// Split out the name and value.
		// *NOTE: The use of strchr() here is safe since mBuffer is
		// guaranteed to be NULL terminated from the call to getline()
		// above.
		char* colon = strchr(mBuffer, ':');
		if(!colon)
		{
			mError = true;
			return false;
		}

		// Cononicalize the name part, and store the name: value in
		// the headers structure. We do this by iterating through
		// 'known' headers and replacing the value found with the
		// correct one.
		// *NOTE: Not so efficient, but iterating through a small
		// subset should not be too much of an issue.
		std::string name(mBuffer, colon++ - mBuffer);
		while(isspace(*colon)) ++colon;
		std::string value(colon);
		for(S32 ii = 0; ii < KNOWN_HEADER_COUNT; ++ii)
		{
			if(0 == LLStringUtil::compareInsensitive(name, KNOWN_HEADER[ii]))
			{
				name = KNOWN_HEADER[ii];
				break;
			}
		}
		headers[name] = value;
	}
	if(headers.isUndefined()) return false;
	return true;
}

std::string LLMimeParser::Impl::findSeparator(std::string header)
{
	//                               01234567890
	//Content-Type: multipart/mixed; boundary="segment"
	std::string separator;
	std::string::size_type pos = header.find(BOUNDARY);
	if(std::string::npos == pos) return separator;
	pos += BOUNDARY.size() + 1;
	std::string::size_type end;
	if(header[pos] == '"')
	{
		// the boundary is quoted, find the end from pos, and take the
		// substring.
		end = header.find('"', ++pos);
		if(std::string::npos == end)
		{
			// poorly formed boundary.
			mError = true;
		}
	}
	else
	{
		// otherwise, it's every character until a whitespace, end of
		// line, or another parameter begins.
		end = header.find_first_of(END_OF_CONTENT_PARAMETER, pos);
		if(std::string::npos == end)
		{
			// it goes to the end of the string.
			end = header.size();
		}
	}
	if(!mError) separator = header.substr(pos, end - pos);
	return separator;
}

void LLMimeParser::Impl::scanPastSeparator(
	std::istream& istr,
	S32 limit,
	const std::string& sep)
{
	std::ostringstream ostr;
	ostr << SEPARATOR_PREFIX << sep;
	std::string separator = ostr.str();
	bool found_separator = false;
	while(!found_separator && continueParse())
	{
		// Subtract 1 from the limit so that we make sure not to read
		// past limit when we get() the newline.
		S32 max_get = llmin((S32)LINE_BUFFER_LENGTH, limit - mScanCount - 1);
		istr.getline(mBuffer, max_get, '\r');
		mScanCount += (S32)istr.gcount();
		if(istr.gcount() >= LINE_BUFFER_LENGTH - 1)
		{
			// that's way too long to be a separator, so ignore it.
			continue;
		}
		int c = istr.get();
		if(EOF == c)
		{
			mContinue = false;
			return;
		}
		++mScanCount;
		if(c != '\n')
		{
			mError = true;
			return;
		}
		if(mScanCount >= limit)
		{
			mContinue = false;
		}
		if(0 == LLStringUtil::compareStrings(std::string(mBuffer), separator))
		{
			found_separator = true;
		}
	}
}

void LLMimeParser::Impl::scanPastContent(
	std::istream& istr,
	S32 limit,
	LLSD headers,
	const std::string separator)
{
	if(headers.has(CONTENT_LENGTH))
	{
		S32 content_length = headers[CONTENT_LENGTH].asInteger();
		// Subtract 2 here for the \r\n after the content.
		S32 max_skip = llmin(content_length, limit - mScanCount - 2);
		istr.ignore(max_skip);
		mScanCount += max_skip;

		// *NOTE: Check for hitting the limit and eof here before
		// checking for the trailing EOF, because our mime parser has
		// to gracefully handle incomplete mime entites.
		if((mScanCount >= limit) || istr.eof())
		{
			mContinue = false;
		}
		else if(!eatCRLF(istr))
		{
			mError = true;
			return;
		}
	}
}

bool LLMimeParser::Impl::eatCRLF(std::istream& istr)
{
	int c = istr.get();
	++mScanCount;
	if(c != '\r')
	{
		return false;
	}
	c = istr.get();
	++mScanCount;
	if(c != '\n')
	{
		return false;
	}
	return true;
}
	

LLMimeParser::LLMimeParser() : mImpl(* new LLMimeParser::Impl)
{
}

LLMimeParser::~LLMimeParser()
{
	delete & mImpl;
}

void LLMimeParser::reset()
{
	mImpl.reset();
}

bool LLMimeParser::parseIndex(std::istream& istr, LLMimeIndex& index)
{
	std::string separator;
	return mImpl.parseIndex(istr, S32_MAX, separator, false, index);
}

bool LLMimeParser::parseIndex(
	const std::vector<U8>& buffer,
	LLMimeIndex& index)
{
	LLMemoryStream mstr(&buffer[0], buffer.size());
	return parseIndex(mstr, buffer.size() + 1, index);
}

bool LLMimeParser::parseIndex(
	std::istream& istr,
	S32 limit,
	LLMimeIndex& index)
{
	std::string separator;
	return mImpl.parseIndex(istr, limit, separator, false, index);
}

bool LLMimeParser::parseIndex(const U8* buffer, S32 length, LLMimeIndex& index)
{
	LLMemoryStream mstr(buffer, length);
	return parseIndex(mstr, length + 1, index);
}

/*
bool LLMimeParser::verify(std::istream& isr, LLMimeIndex& index) const
{
	return false;
}

bool LLMimeParser::verify(U8* buffer, S32 length, LLMimeIndex& index) const
{
	LLMemoryStream mstr(buffer, length);
	return verify(mstr, index);
}
*/
