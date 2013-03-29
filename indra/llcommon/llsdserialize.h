/** 
 * @file llsdserialize.h
 * @author Phoenix
 * @date 2006-02-26
 * @brief Declaration of parsers and formatters for LLSD
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

#ifndef LL_LLSDSERIALIZE_H
#define LL_LLSDSERIALIZE_H

#include <iosfwd>
#include "llpointer.h"
#include "llrefcount.h"
#include "llsd.h"

/** 
 * @class LLSDParser
 * @brief Abstract base class for LLSD parsers.
 */
class LL_COMMON_API LLSDParser : public LLRefCount
{
protected:
	/** 
	 * @brief Destructor
	 */
	virtual ~LLSDParser();

public:
	/** 
	 * @brief Anonymous enum to indicate parsing failure.
	 */
	enum
	{
		PARSE_FAILURE = -1
	};

	/** 
	 * @brief Constructor
	 */
	LLSDParser();

	/** 
	 * @brief Call this method to parse a stream for LLSD.
	 *
	 * This method parses the istream for a structured data. This
	 * method assumes that the istream is a complete llsd object --
	 * for example an opened and closed map with an arbitrary nesting
	 * of elements. This method will return after reading one data
	 * object, allowing continued reading from the stream by the
	 * caller.
	 * @param istr The input stream.
	 * @param data[out] The newly parse structured data.
	 * @param max_bytes The maximum number of bytes that will be in
	 * the stream. Pass in LLSDSerialize::SIZE_UNLIMITED (-1) to set no
	 * byte limit.
	 * @return Returns the number of LLSD objects parsed into
	 * data. Returns PARSE_FAILURE (-1) on parse failure.
	 */
	S32 parse(std::istream& istr, LLSD& data, S32 max_bytes);

	/** Like parse(), but uses a different call (istream.getline()) to read by lines
	 *  This API is better suited for XML, where the parse cannot tell
	 *  where the document actually ends.
	 */
	S32 parseLines(std::istream& istr, LLSD& data);

	/** 
	 * @brief Resets the parser so parse() or parseLines() can be called again for another <llsd> chunk.
	 */
	void reset()	{ doReset();	};


protected:
	/** 
	 * @brief Pure virtual base for doing the parse.
	 *
	 * This method parses the istream for a structured data. This
	 * method assumes that the istream is a complete llsd object --
	 * for example an opened and closed map with an arbitrary nesting
	 * of elements. This method will return after reading one data
	 * object, allowing continued reading from the stream by the
	 * caller.
	 * @param istr The input stream.
	 * @param data[out] The newly parse structured data.
	 * @return Returns the number of LLSD objects parsed into
	 * data. Returns PARSE_FAILURE (-1) on parse failure.
	 */
	virtual S32 doParse(std::istream& istr, LLSD& data) const = 0;

	/** 
	 * @brief Virtual default function for resetting the parser
	 */
	virtual void doReset()	{};

	/* @name Simple istream helper methods 
	 *
	 * These helper methods exist to help correctly use the
	 * mMaxBytesLeft without really thinking about it for most simple
	 * operations. Use of the streamtools in llstreamtools.h will
	 * require custom wrapping.
	 */
	//@{
	/** 
	 * @brief get a byte off the stream
	 *
	 * @param istr The istream to work with.
	 * @return returns the next character.
	 */
	int get(std::istream& istr) const;
	
	/** 
	 * @brief get several bytes off the stream into a buffer.
	 *
	 * @param istr The istream to work with.
	 * @param s The buffer to get into
	 * @param n Extract maximum of n-1 bytes and null temrinate.
	 * @param delim Delimiter to get until found.
	 * @return Returns istr.
	 */
	std::istream& get(
		std::istream& istr,
		char* s,
		std::streamsize n,
		char delim) const;

	/** 
	 * @brief get several bytes off the stream into a streambuf
	 *
	 * @param istr The istream to work with.
	 * @param sb The streambuf to read into
	 * @param delim Delimiter to get until found.
	 * @return Returns istr.
	 */
	std::istream& get(
		std::istream& istr,
		std::streambuf& sb,
		char delim) const;

	/** 
	 * @brief ignore the next byte on the istream
	 *
	 * @param istr The istream to work with.
	 * @return Returns istr.
	 */
	std::istream& ignore(std::istream& istr) const;

	/** 
	 * @brief put the last character retrieved back on the stream
	 *
	 * @param istr The istream to work with.
	 * @param c The character to put back
	 * @return Returns istr.
	 */
	std::istream& putback(std::istream& istr, char c) const;

	/** 
	 * @brief read a block of n characters into a buffer
	 *
	 * @param istr The istream to work with.
	 * @param s The buffer to read into
	 * @param n The number of bytes to read.
	 * @return Returns istr.
	 */
	std::istream& read(std::istream& istr, char* s, std::streamsize n) const;
	//@}

protected:
	/**
	 * @brief Accunt for bytes read outside of the istream helpers.
	 *
	 * Conceptually const since it only modifies mutable members.
	 * @param bytes The number of bytes read.
	 */
	void account(S32 bytes) const;

protected:
	/**
	 * @brief boolean to set if byte counts should be checked during parsing.
	 */
	bool mCheckLimits;

	/**
	 * @brief The maximum number of bytes left to be parsed.
	 */
	mutable S32 mMaxBytesLeft;
	
	/**
	 * @brief Use line-based reading to get text
	 */
	bool mParseLines;
};

/** 
 * @class LLSDNotationParser
 * @brief Parser which handles the original notation format for LLSD.
 */
class LL_COMMON_API LLSDNotationParser : public LLSDParser
{
protected:
	/** 
	 * @brief Destructor
	 */
	virtual ~LLSDNotationParser();

public:
	/** 
	 * @brief Constructor
	 */
	LLSDNotationParser();

protected:
	/** 
	 * @brief Call this method to parse a stream for LLSD.
	 *
	 * This method parses the istream for a structured data. This
	 * method assumes that the istream is a complete llsd object --
	 * for example an opened and closed map with an arbitrary nesting
	 * of elements. This method will return after reading one data
	 * object, allowing continued reading from the stream by the
	 * caller.
	 * @param istr The input stream.
	 * @param data[out] The newly parse structured data. Undefined on failure.
	 * @return Returns the number of LLSD objects parsed into
	 * data. Returns PARSE_FAILURE (-1) on parse failure.
	 */
	virtual S32 doParse(std::istream& istr, LLSD& data) const;

private:
	/** 
	 * @brief Parse a map from the istream
	 *
	 * @param istr The input stream.
	 * @param map The map to add the parsed data.
	 * @return Returns The number of LLSD objects parsed into data.
	 */
	S32 parseMap(std::istream& istr, LLSD& map) const;

	/** 
	 * @brief Parse an array from the istream.
	 *
	 * @param istr The input stream.
	 * @param array The array to append the parsed data.
	 * @return Returns The number of LLSD objects parsed into data.
	 */
	S32 parseArray(std::istream& istr, LLSD& array) const;

	/** 
	 * @brief Parse a string from the istream and assign it to data.
	 *
	 * @param istr The input stream.
	 * @param data[out] The data to assign.
	 * @return Retuns true if a complete string was parsed.
	 */
	bool parseString(std::istream& istr, LLSD& data) const;

	/** 
	 * @brief Parse binary data from the stream.
	 *
	 * @param istr The input stream.
	 * @param data[out] The data to assign.
	 * @return Retuns true if a complete blob was parsed.
	 */
	bool parseBinary(std::istream& istr, LLSD& data) const;
};

/** 
 * @class LLSDXMLParser
 * @brief Parser which handles XML format LLSD.
 */
class LL_COMMON_API LLSDXMLParser : public LLSDParser
{
protected:
	/** 
	 * @brief Destructor
	 */
	virtual ~LLSDXMLParser();

public:
	/** 
	 * @brief Constructor
	 */
	LLSDXMLParser();

protected:
	/** 
	 * @brief Call this method to parse a stream for LLSD.
	 *
	 * This method parses the istream for a structured data. This
	 * method assumes that the istream is a complete llsd object --
	 * for example an opened and closed map with an arbitrary nesting
	 * of elements. This method will return after reading one data
	 * object, allowing continued reading from the stream by the
	 * caller.
	 * @param istr The input stream.
	 * @param data[out] The newly parse structured data.
	 * @return Returns the number of LLSD objects parsed into
	 * data. Returns PARSE_FAILURE (-1) on parse failure.
	 */
	virtual S32 doParse(std::istream& istr, LLSD& data) const;

	/** 
	 * @brief Virtual default function for resetting the parser
	 */
	virtual void doReset();

private:
	class Impl;
	Impl& impl;

	void parsePart(const char* buf, int len);
	friend class LLSDSerialize;
};

/** 
 * @class LLSDBinaryParser
 * @brief Parser which handles binary formatted LLSD.
 */
class LL_COMMON_API LLSDBinaryParser : public LLSDParser
{
protected:
	/** 
	 * @brief Destructor
	 */
	virtual ~LLSDBinaryParser();

public:
	/** 
	 * @brief Constructor
	 */
	LLSDBinaryParser();

protected:
	/** 
	 * @brief Call this method to parse a stream for LLSD.
	 *
	 * This method parses the istream for a structured data. This
	 * method assumes that the istream is a complete llsd object --
	 * for example an opened and closed map with an arbitrary nesting
	 * of elements. This method will return after reading one data
	 * object, allowing continued reading from the stream by the
	 * caller.
	 * @param istr The input stream.
	 * @param data[out] The newly parse structured data.
	 * @return Returns the number of LLSD objects parsed into
	 * data. Returns -1 on parse failure.
	 */
	virtual S32 doParse(std::istream& istr, LLSD& data) const;

private:
	/** 
	 * @brief Parse a map from the istream
	 *
	 * @param istr The input stream.
	 * @param map The map to add the parsed data.
	 * @return Returns The number of LLSD objects parsed into data.
	 */
	S32 parseMap(std::istream& istr, LLSD& map) const;

	/** 
	 * @brief Parse an array from the istream.
	 *
	 * @param istr The input stream.
	 * @param array The array to append the parsed data.
	 * @return Returns The number of LLSD objects parsed into data.
	 */
	S32 parseArray(std::istream& istr, LLSD& array) const;

	/** 
	 * @brief Parse a string from the istream and assign it to data.
	 *
	 * @param istr The input stream.
	 * @param value[out] The string to assign.
	 * @return Retuns true if a complete string was parsed.
	 */
	bool parseString(std::istream& istr, std::string& value) const;
};


/** 
 * @class LLSDFormatter
 * @brief Abstract base class for formatting LLSD.
 */
class LL_COMMON_API LLSDFormatter : public LLRefCount
{
protected:
	/** 
	 * @brief Destructor
	 */
	virtual ~LLSDFormatter();

public:
	/**
	 * Options for output
	 */
	typedef enum e_formatter_options_type
	{
		OPTIONS_NONE = 0,
		OPTIONS_PRETTY = 1
	} EFormatterOptions;

	/** 
	 * @brief Constructor
	 */
	LLSDFormatter();

	/** 
	 * @brief Set the boolean serialization format.
	 *
	 * @param alpha Serializes boolean as alpha if true.
	 */
	void boolalpha(bool alpha);

	/** 
	 * @brief Set the real format
	 *
	 * By default, the formatter will use default double serialization
	 * which is frequently frustrating for many applications. You can
	 * set the precision on the stream independently, but that still
	 * might not work depending on the value.
	 * EXAMPLES:<br>
	 * %.2f<br>
	 * @param format A format string which follows the printf format
	 * rules. Specify an empty string to return to default formatting.
	 */
	void realFormat(const std::string& format);

	/** 
	 * @brief Call this method to format an LLSD to a stream.
	 *
	 * @param data The data to write.
	 * @param ostr The destination stream for the data.
	 * @return Returns The number of LLSD objects fomatted out
	 */
	virtual S32 format(const LLSD& data, std::ostream& ostr, U32 options = LLSDFormatter::OPTIONS_NONE) const = 0;

protected:
	/** 
	 * @brief Helper method which appropriately obeys the real format.
	 *
	 * @param real The real value to format.
	 * @param ostr The destination stream for the data.
	 */
	void formatReal(LLSD::Real real, std::ostream& ostr) const;

protected:
	bool mBoolAlpha;
	std::string mRealFormat;
};


/** 
 * @class LLSDNotationFormatter
 * @brief Formatter which outputs the original notation format for LLSD.
 */
class LL_COMMON_API LLSDNotationFormatter : public LLSDFormatter
{
protected:
	/** 
	 * @brief Destructor
	 */
	virtual ~LLSDNotationFormatter();

public:
	/** 
	 * @brief Constructor
	 */
	LLSDNotationFormatter();

	/** 
	 * @brief Helper static method to return a notation escaped string
	 *
	 * This method will return the notation escaped string, but not
	 * the surrounding serialization identifiers such as a double or
	 * single quote. It will be up to the caller to embed those as
	 * appropriate.
	 * @param in The raw, unescaped string.
	 * @return Returns an escaped string appropriate for serialization.
	 */
	static std::string escapeString(const std::string& in);

	/** 
	 * @brief Call this method to format an LLSD to a stream.
	 *
	 * @param data The data to write.
	 * @param ostr The destination stream for the data.
	 * @return Returns The number of LLSD objects fomatted out
	 */
	virtual S32 format(const LLSD& data, std::ostream& ostr, U32 options = LLSDFormatter::OPTIONS_NONE) const;
};


/** 
 * @class LLSDXMLFormatter
 * @brief Formatter which outputs the LLSD as XML.
 */
class LL_COMMON_API LLSDXMLFormatter : public LLSDFormatter
{
protected:
	/** 
	 * @brief Destructor
	 */
	virtual ~LLSDXMLFormatter();

public:
	/** 
	 * @brief Constructor
	 */
	LLSDXMLFormatter();

	/** 
	 * @brief Helper static method to return an xml escaped string
	 *
	 * @param in A valid UTF-8 string.
	 * @return Returns an escaped string appropriate for serialization.
	 */
	static std::string escapeString(const std::string& in);

	/** 
	 * @brief Call this method to format an LLSD to a stream.
	 *
	 * @param data The data to write.
	 * @param ostr The destination stream for the data.
	 * @return Returns The number of LLSD objects fomatted out
	 */
	virtual S32 format(const LLSD& data, std::ostream& ostr, U32 options = LLSDFormatter::OPTIONS_NONE) const;

protected:

	/** 
	 * @brief Implementation to format the data. This is called recursively.
	 *
	 * @param data The data to write.
	 * @param ostr The destination stream for the data.
	 * @return Returns The number of LLSD objects fomatted out
	 */
	S32 format_impl(const LLSD& data, std::ostream& ostr, U32 options, U32 level) const;
};


/** 
 * @class LLSDBinaryFormatter
 * @brief Formatter which outputs the LLSD as a binary notation format.
 *
 * The binary format is a compact and efficient representation of
 * structured data useful for when transmitting over a small data pipe
 * or when transmission frequency is very high.<br>
 *
 * The normal boolalpha and real format commands are ignored.<br>
 *
 * All integers are transmitted in network byte order. The format is:<br>
 * Undefined: '!'<br>
 * Boolean: character '1' for true character '0' for false<br>
 * Integer: 'i' + 4 bytes network byte order<br>
 * Real: 'r' + 8 bytes IEEE double<br>
 * UUID: 'u' + 16 byte unsigned integer<br>
 * String: 's' + 4 byte integer size + string<br>
 * Date: 'd' + 8 byte IEEE double for seconds since epoch<br>
 * URI: 'l' + 4 byte integer size + string uri<br>
 * Binary: 'b' + 4 byte integer size + binary data<br>
 * Array: '[' + 4 byte integer size  + all values + ']'<br>
 * Map: '{' + 4 byte integer size  every(key + value) + '}'<br>
 *  map keys are serialized as 'k' + 4 byte integer size + string
 */
class LL_COMMON_API LLSDBinaryFormatter : public LLSDFormatter
{
protected:
	/** 
	 * @brief Destructor
	 */
	virtual ~LLSDBinaryFormatter();

public:
	/** 
	 * @brief Constructor
	 */
	LLSDBinaryFormatter();

	/** 
	 * @brief Call this method to format an LLSD to a stream.
	 *
	 * @param data The data to write.
	 * @param ostr The destination stream for the data.
	 * @return Returns The number of LLSD objects fomatted out
	 */
	virtual S32 format(const LLSD& data, std::ostream& ostr, U32 options = LLSDFormatter::OPTIONS_NONE) const;

protected:
	/** 
	 * @brief Helper method to serialize strings
	 *
	 * This method serializes a network byte order size and the raw
	 * string contents.
	 * @param string The string to write.
	 * @param ostr The destination stream for the data.
	 */
	void formatString(const std::string& string, std::ostream& ostr) const;
};


/** 
 * @class LLSDNotationStreamFormatter
 * @brief Formatter which is specialized for use on streams which
 * outputs the original notation format for LLSD.
 *
 * This class is useful for doing inline stream operations. For example:
 *
 * <code>
 *  LLSD sd;<br>
 *  sd["foo"] = "bar";<br>
 *  std::stringstream params;<br>
 *	params << "[{'version':i1}," << LLSDOStreamer<LLSDNotationFormatter>(sd)
 *    << "]";
 *  </code>
 *
 * *NOTE - formerly this class inherited from its template parameter Formatter,
 * but all insnatiations passed in LLRefCount subclasses.  This conflicted with
 * the auto allocation intended for this class template (demonstrated in the
 * example above).  -brad
 */
template <class Formatter>
class LLSDOStreamer
{
public:
	/** 
	 * @brief Constructor
	 */
	LLSDOStreamer(const LLSD& data, U32 options = LLSDFormatter::OPTIONS_NONE) :
		mSD(data), mOptions(options) {}

	/**
	 * @brief Stream operator.
	 *
	 * Use this inline during construction during a stream operation.
	 * @param str The destination stream for serialized output.
	 * @param The formatter which will output it's LLSD.
	 * @return Returns the stream passed in after streaming mSD.
	 */
	friend std::ostream& operator<<(
		std::ostream& str,
		const LLSDOStreamer<Formatter>& formatter)
	{
		LLPointer<Formatter> f = new Formatter;
		f->format(formatter.mSD, str, formatter.mOptions);
		return str;
	}

protected:
	LLSD mSD;
	U32 mOptions;
};

typedef LLSDOStreamer<LLSDNotationFormatter>	LLSDNotationStreamer;
typedef LLSDOStreamer<LLSDXMLFormatter>			LLSDXMLStreamer;

/** 
 * @class LLSDSerialize
 * @brief Serializer / deserializer for the various LLSD formats
 */
class LL_COMMON_API LLSDSerialize
{
public:
	enum ELLSD_Serialize
	{
		LLSD_BINARY, LLSD_XML
	};

	/**
	 * @brief anonymouse enumeration for useful max_bytes constants.
	 */
	enum
	{
		// Setting an unlimited size is discouraged and should only be
		// used when reading cin or another stream source which does
		// not provide access to size.
		SIZE_UNLIMITED = -1,
	};

	/*
	 * Generic in/outs
	 */
	static void serialize(const LLSD& sd, std::ostream& str, ELLSD_Serialize,
		U32 options = LLSDFormatter::OPTIONS_NONE);

	/**
	 * @brief Examine a stream, and parse 1 sd object out based on contents.
	 *
	 * @param sd [out] The data found on the stream
	 * @param str The incoming stream
	 * @param max_bytes the maximum number of bytes to parse
	 * @return Returns true if the stream appears to contain valid data
	 */
	static bool deserialize(LLSD& sd, std::istream& str, S32 max_bytes);

	/*
	 * Notation Methods
	 */
	static S32 toNotation(const LLSD& sd, std::ostream& str)
	{
		LLPointer<LLSDNotationFormatter> f = new LLSDNotationFormatter;
		return f->format(sd, str, LLSDFormatter::OPTIONS_NONE);
	}
	static S32 fromNotation(LLSD& sd, std::istream& str, S32 max_bytes)
	{
		LLPointer<LLSDNotationParser> p = new LLSDNotationParser;
		return p->parse(str, sd, max_bytes);
	}
	static LLSD fromNotation(std::istream& str, S32 max_bytes)
	{
		LLPointer<LLSDNotationParser> p = new LLSDNotationParser;
		LLSD sd;
		(void)p->parse(str, sd, max_bytes);
		return sd;
	}
	
	/*
	 * XML Methods
	 */
	static S32 toXML(const LLSD& sd, std::ostream& str)
	{
		LLPointer<LLSDXMLFormatter> f = new LLSDXMLFormatter;
		return f->format(sd, str, LLSDFormatter::OPTIONS_NONE);
	}
	static S32 toPrettyXML(const LLSD& sd, std::ostream& str)
	{
		LLPointer<LLSDXMLFormatter> f = new LLSDXMLFormatter;
		return f->format(sd, str, LLSDFormatter::OPTIONS_PRETTY);
	}

	static S32 fromXMLEmbedded(LLSD& sd, std::istream& str)
	{
		// no need for max_bytes since xml formatting is not
		// subvertable by bad sizes.
		LLPointer<LLSDXMLParser> p = new LLSDXMLParser;
		return p->parse(str, sd, LLSDSerialize::SIZE_UNLIMITED);
	}
	// Line oriented parser, 30% faster than fromXML(), but can
	// only be used when you know you have the complete XML
	// document available in the stream.
	static S32 fromXMLDocument(LLSD& sd, std::istream& str)
	{
		LLPointer<LLSDXMLParser> p = new LLSDXMLParser();
		return p->parseLines(str, sd);
	}
	static S32 fromXML(LLSD& sd, std::istream& str)
	{
		return fromXMLEmbedded(sd, str);
//		return fromXMLDocument(sd, str);
	}

	/*
	 * Binary Methods
	 */
	static S32 toBinary(const LLSD& sd, std::ostream& str)
	{
		LLPointer<LLSDBinaryFormatter> f = new LLSDBinaryFormatter;
		return f->format(sd, str, LLSDFormatter::OPTIONS_NONE);
	}
	static S32 fromBinary(LLSD& sd, std::istream& str, S32 max_bytes)
	{
		LLPointer<LLSDBinaryParser> p = new LLSDBinaryParser;
		return p->parse(str, sd, max_bytes);
	}
	static LLSD fromBinary(std::istream& str, S32 max_bytes)
	{
		LLPointer<LLSDBinaryParser> p = new LLSDBinaryParser;
		LLSD sd;
		(void)p->parse(str, sd, max_bytes);
		return sd;
	}
};

//dirty little zip functions -- yell at davep
LL_COMMON_API std::string zip_llsd(LLSD& data);
LL_COMMON_API bool unzip_llsd(LLSD& data, std::istream& is, S32 size);
LL_COMMON_API U8* unzip_llsdNavMesh( bool& valid, unsigned int& outsize,std::istream& is, S32 size);
#endif // LL_LLSDSERIALIZE_H
