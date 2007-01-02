/** 
 * @file llxmlparser.h
 * @brief LLXmlParser class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLXMLPARSER_H
#define LL_LLXMLPARSER_H

#define XML_STATIC
#include "expat/expat.h"

class LLXmlParser
{
public:
	LLXmlParser();
	virtual ~LLXmlParser();

	// Parses entire file
	BOOL parseFile(const std::string &path);

	//	Parses some input. Returns 0 if a fatal error is detected.
	//	The last call must have isFinal true;
	//	len may be zero for this call (or any other).
	S32 parse( const char* buf, int len, int isFinal );

	const char* getErrorString();
	
	S32 getCurrentLineNumber();

	S32 getCurrentColumnNumber();

	S32 getDepth() { return mDepth; }

protected:
	// atts is array of name/value pairs, terminated by 0;
	// names and values are 0 terminated.
	virtual void startElement(const char *name, const char **atts) {}

	virtual void endElement(const char *name) {}

	// s is not 0 terminated.
	virtual void characterData(const char *s, int len) {}
	
	// target and data are 0 terminated
	virtual void processingInstruction(const char *target, const char *data) {}

	// data is 0 terminated 
	virtual void comment(const char *data) {}

	virtual void startCdataSection() {}

	virtual void endCdataSection() {}

	//	This is called for any characters in the XML document for
	//	which there is no applicable handler.  This includes both
	//	characters that are part of markup which is of a kind that is
	//	not reported (comments, markup declarations), or characters
	//	that are part of a construct which could be reported but
	//	for which no handler has been supplied. The characters are passed
	//	exactly as they were in the XML document except that
	//	they will be encoded in UTF-8.  Line boundaries are not normalized.
	//	Note that a byte order mark character is not passed to the default handler.
	//	There are no guarantees about how characters are divided between calls
	//	to the default handler: for example, a comment might be split between
	//	multiple calls.
	virtual void defaultData(const char *s, int len) {}

	//	This is called for a declaration of an unparsed (NDATA)
	//	entity.  The base argument is whatever was set by XML_SetBase.
	//	The entityName, systemId and notationName arguments will never be null.
	//	The other arguments may be.
	virtual void unparsedEntityDecl(
		const char *entityName,
		const char *base,
		const char *systemId,
		const char *publicId,
		const char *notationName) {}

public:
	///////////////////////////////////////////////////////////////////////////////
	// Pseudo-private methods.  These are only used by internal callbacks.
	
	static void startElementHandler(void *userData, const XML_Char *name, const XML_Char **atts);
	static void endElementHandler(void *userData, const XML_Char *name);
	static void characterDataHandler(void *userData, const XML_Char *s, int len);
	static void processingInstructionHandler(void *userData, const XML_Char *target, const XML_Char *data);
	static void commentHandler(void *userData, const XML_Char *data);
	static void startCdataSectionHandler(void *userData);
	static void endCdataSectionHandler(void *userData);
	static void defaultDataHandler( void *userData, const XML_Char *s, int len);
	static void unparsedEntityDeclHandler(
		void *userData,
		const XML_Char *entityName,
		const XML_Char *base,
		const XML_Char *systemId,
		const XML_Char *publicId,
		const XML_Char *notationName);


protected:
	XML_Parser		mParser;
	int				mDepth;
	char			mAuxErrorString[1024];
};

#endif  // LL_LLXMLPARSER_H
