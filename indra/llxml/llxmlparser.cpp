/** 
 * @file llxmlparser.cpp
 * @brief LLXmlParser implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

// llxmlparser.cpp
//              
// copyright 2002, linden research inc


#include "linden_common.h"

#include "llxmlparser.h"
#include "llerror.h"


LLXmlParser::LLXmlParser()
	:
	mParser( NULL ),
	mDepth( 0 )
{
	mAuxErrorString = "no error";

	// Override the document's declared encoding.
	mParser = XML_ParserCreate(NULL);

	XML_SetUserData(mParser, this);
	XML_SetElementHandler(					mParser,	startElementHandler, endElementHandler);
	XML_SetCharacterDataHandler(			mParser,	characterDataHandler);
	XML_SetProcessingInstructionHandler(	mParser,	processingInstructionHandler);
	XML_SetCommentHandler(					mParser,	commentHandler);

	XML_SetCdataSectionHandler(				mParser,	startCdataSectionHandler, endCdataSectionHandler);

	// This sets the default handler but does not inhibit expansion of internal entities.
	// The entity reference will not be passed to the default handler.
	XML_SetDefaultHandlerExpand(			mParser,	defaultDataHandler);
	
	XML_SetUnparsedEntityDeclHandler(		mParser,	unparsedEntityDeclHandler);
}

LLXmlParser::~LLXmlParser()
{
	XML_ParserFree( mParser );
}


BOOL LLXmlParser::parseFile(const std::string &path)
{
	llassert( !mDepth );
	
	BOOL success = TRUE;

	LLFILE* file = LLFile::fopen(path, "rb");		/* Flawfinder: ignore */
	if( !file )
	{
		mAuxErrorString = llformat( "Couldn't open file %s", path.c_str());
		success = FALSE;
	}
	else
	{
		S32 bytes_read = 0;
		
		fseek(file, 0L, SEEK_END);
		S32 buffer_size = ftell(file);
		fseek(file, 0L, SEEK_SET);

		void* buffer = XML_GetBuffer(mParser, buffer_size);
		if( !buffer ) 
		{
			mAuxErrorString = llformat( "Unable to allocate XML buffer while reading file %s", path.c_str() );
			success = FALSE;
			goto exit_label;
		}
		
		bytes_read = (S32)fread(buffer, 1, buffer_size, file);
		if( bytes_read <= 0 )
		{
			mAuxErrorString = llformat( "Error while reading file  %s", path.c_str() );
			success = FALSE;
			goto exit_label;
		}
		
		if( !XML_ParseBuffer(mParser, bytes_read, TRUE ) )
		{
			mAuxErrorString = llformat( "Error while parsing file  %s", path.c_str() );
			success = FALSE;
		}

exit_label: 
		fclose( file );
	}


	if( success )
	{
		llassert( !mDepth );
	}
	mDepth = 0;

	if( !success )
	{
		llwarns << mAuxErrorString << llendl;
	}

	return success;
}


//	Parses some input. Returns 0 if a fatal error is detected.
//	The last call must have isFinal true;
//	len may be zero for this call (or any other).
S32 LLXmlParser::parse( const char* buf, int len, int isFinal )
{
	return XML_Parse(mParser, buf, len, isFinal);
}

const char* LLXmlParser::getErrorString()
{
	const char* error_string = XML_ErrorString(XML_GetErrorCode( mParser ));
	if( !error_string )
	{
		error_string = mAuxErrorString.c_str();
	}
	return error_string;
}

S32 LLXmlParser::getCurrentLineNumber()
{
	return XML_GetCurrentLineNumber( mParser );
}

S32 LLXmlParser::getCurrentColumnNumber()
{
	return XML_GetCurrentColumnNumber(mParser);
}

///////////////////////////////////////////////////////////////////////////////
// Pseudo-private methods.  These are only used by internal callbacks.

// static 
void LLXmlParser::startElementHandler(
	 void *userData,
	 const XML_Char *name,
	 const XML_Char **atts)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->startElement( name, atts );
	self->mDepth++;
}

// static 
void LLXmlParser::endElementHandler(
	void *userData,
	const XML_Char *name)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->mDepth--;
	self->endElement( name );
}

// s is not 0 terminated.
// static 
void LLXmlParser::characterDataHandler(
	void *userData,
	const XML_Char *s,
	int len)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->characterData( s, len );
}

// target and data are 0 terminated
// static 
void LLXmlParser::processingInstructionHandler(
	void *userData,
	const XML_Char *target,
	const XML_Char *data)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->processingInstruction( target, data );
}

// data is 0 terminated 
// static
void LLXmlParser::commentHandler(void *userData, const XML_Char *data)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->comment( data );
}

// static
void LLXmlParser::startCdataSectionHandler(void *userData)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->mDepth++;
	self->startCdataSection();
}

// static
void LLXmlParser::endCdataSectionHandler(void *userData)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->endCdataSection();
	self->mDepth++;
}

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

// static 
void LLXmlParser::defaultDataHandler(
	void *userData,
	const XML_Char *s,
	int len)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->defaultData( s, len );
}

//	This is called for a declaration of an unparsed (NDATA)
//	entity.  The base argument is whatever was set by XML_SetBase.
//	The entityName, systemId and notationName arguments will never be null.
//	The other arguments may be.
// static 
void LLXmlParser::unparsedEntityDeclHandler(
	void *userData,
	const XML_Char *entityName,
	const XML_Char *base,
	const XML_Char *systemId,
	const XML_Char *publicId,
	const XML_Char *notationName)
{
	LLXmlParser* self = (LLXmlParser*) userData;
	self->unparsedEntityDecl( entityName, base, systemId, publicId, notationName );
}




////////////////////////////////////////////////////////////////////
// Test code.

/*
class LLXmlDOMParser : public LLXmlParser
{
public:

	LLXmlDOMParser() {}
	virtual ~LLXmlDOMParser() {}

	void tabs()
	{
		for ( int i = 0; i < getDepth(); i++)
		{
			putchar(' ');
		}
	}

	virtual void startElement(const char *name, const char **atts) 
	{
		tabs();
		printf("startElement %s\n", name);
		
		S32 i = 0;
		while( atts[i] && atts[i+1] )
		{
			tabs();
			printf( "\t%s=%s\n", atts[i], atts[i+1] );
			i += 2;
		}

		if( atts[i] )
		{
			tabs();
			printf( "\ttrailing attribute: %s\n", atts[i] );
		}
	}

	virtual void endElement(const char *name) 
	{
		tabs();
		printf("endElement %s\n", name);
	}

	virtual void characterData(const char *s, int len) 
	{
		tabs();

		char* str = new char[len+1];
		strncpy( str, s, len );
		str[len] = '\0';
		printf("CharacterData %s\n", str);
		delete str;
	}

	virtual void processingInstruction(const char *target, const char *data)
	{
		tabs();
		printf("processingInstruction %s\n", data);
	}
	virtual void comment(const char *data)
	{
		tabs();
		printf("comment %s\n", data);
	}

	virtual void startCdataSection()
	{
		tabs();
		printf("startCdataSection\n");
	}

	virtual void endCdataSection()
	{
		tabs();
		printf("endCdataSection\n");
	}

	virtual void defaultData(const char *s, int len)
	{
		tabs();

		char* str = new char[len+1];
		strncpy( str, s, len );
		str[len] = '\0';
		printf("defaultData %s\n", str);
		delete str;
	}

	virtual void unparsedEntityDecl(
		const char *entityName,
		const char *base,
		const char *systemId,
		const char *publicId,
		const char *notationName)
	{
		tabs();

		printf(
			"unparsed entity:\n"
				"\tentityName %s\n"
				"\tbase %s\n"
				"\tsystemId %s\n"
				"\tpublicId %s\n"
				"\tnotationName %s\n",
			entityName,
			base,
			systemId,
			publicId,
			notationName );
	}
};


int main()
{
  char buf[1024];

  LLFILE* file = LLFile::fopen("test.xml", "rb");
  if( !file )
  {
	  return 1;
  }

  LLXmlDOMParser parser;
  int done;
  do {
    size_t len = fread(buf, 1, sizeof(buf), file);
    done = len < sizeof(buf);
    if( 0 == parser.parse( buf, len, done) )
	{
      fprintf(stderr,
	      "%s at line %d\n",
	      parser.getErrorString(),
	      parser.getCurrentLineNumber() );
      return 1;
    }
  } while (!done);

  fclose( file );
  return 0;
}
*/

