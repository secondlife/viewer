/** 
 * @file llfiltersd2xmlrpc.cpp
 * @author Phoenix
 * @date 2005-04-26
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

/** 
 * xml rpc request: 
 * <code>
 * <?xml version="1.0"?>
 * <methodCall><methodName>examples.getStateName</methodName>
 * <params><param><value><i4>41</i4></value></param></params>
 * </methodCall>
 * </code>
 *
 * xml rpc response:
 * <code>
 * <?xml version="1.0"?>
 * <methodResponse>
 * <params><param><value><string>South Dakota</string></value></param></params>
 * </methodResponse>
 * </code>
 *
 * xml rpc fault:
 * </code>
 * <?xml version="1.0"?>
 * <methodResponse>
 * <fault><value><struct>
 * <member><name>faultCode</name><value><int>4</int></value></member>
 * <member><name>faultString</name><value><string>...</string></value></member>
 * </struct></value></fault>
 * </methodResponse>
 * </code>
 *
 * llsd rpc request:
 * <code>
 * { 'method':'...', 'parameter':...]}
 * </code>
 * 
 * llsd rpc response:
 * <code>
 * { 'response':... }
 * </code>
 * 
 * llsd rpc fault: 
 * <code>
 * { 'fault': {'code':i..., 'description':'...'} }
 * </code>
 * 
 */

#include "linden_common.h"
#include "llfiltersd2xmlrpc.h"

#include <sstream>
#include <iterator>
#include <xmlrpc-epi/xmlrpc.h>
#include "apr_base64.h"

#include "llbuffer.h"
#include "llbufferstream.h"
#include "llmemorystream.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "lluuid.h"

// spammy mode
//#define LL_SPEW_STREAM_OUT_DEBUGGING 1

/**
 * String constants
 */
static const char XML_HEADER[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
static const char XMLRPC_REQUEST_HEADER_1[] = "<methodCall><methodName>";
static const char XMLRPC_REQUEST_HEADER_2[] = "</methodName><params>";
static const char XMLRPC_REQUEST_FOOTER[] = "</params></methodCall>";
static const char XMLRPC_METHOD_RESPONSE_HEADER[] = "<methodResponse>";
static const char XMLRPC_METHOD_RESPONSE_FOOTER[] = "</methodResponse>";
static const char XMLRPC_RESPONSE_HEADER[] = "<params><param>";
static const char XMLRPC_RESPONSE_FOOTER[] = "</param></params>";
static const char XMLRPC_FAULT_1[] = "<fault><value><struct><member><name>faultCode</name><value><int>";
static const char XMLRPC_FAULT_2[] = "</int></value></member><member><name>faultString</name><value><string>";
static const char XMLRPC_FAULT_3[] = "</string></value></member></struct></value></fault>";
static const char LLSDRPC_RESPONSE_HEADER[] = "{'response':";
static const char LLSDRPC_RESPONSE_FOOTER[] = "}";
const char LLSDRPC_REQUEST_HEADER_1[] = "{'method':'";
const char LLSDRPC_REQUEST_HEADER_2[] = "', 'parameter': ";
const char LLSDRPC_REQUEST_FOOTER[] = "}";
static const char LLSDRPC_FAULT_HADER_1[] = "{ 'fault': {'code':i";
static const char LLSDRPC_FAULT_HADER_2[] = ", 'description':";
static const char LLSDRPC_FAULT_FOOTER[] = "} }";
static const S32 DEFAULT_PRECISION = 20;

/**
 * LLFilterSD2XMLRPC
 */
LLFilterSD2XMLRPC::LLFilterSD2XMLRPC()
{
}

LLFilterSD2XMLRPC::~LLFilterSD2XMLRPC()
{
}

std::string xml_escape_string(const std::string& in)
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

void LLFilterSD2XMLRPC::streamOut(std::ostream& ostr, const LLSD& sd)
{
	ostr << "<value>";
	switch(sd.type())
	{
	case LLSD::TypeMap:
	{
#if LL_SPEW_STREAM_OUT_DEBUGGING
		llinfos << "streamOut(map) BEGIN" << llendl;
#endif
		ostr << "<struct>";
		if(ostr.fail())
		{
			llinfos << "STREAM FAILURE writing struct" << llendl;
		}
		LLSD::map_const_iterator it = sd.beginMap();
		LLSD::map_const_iterator end = sd.endMap();
		for(; it != end; ++it)
		{
			ostr << "<member><name>" << xml_escape_string((*it).first)
				<< "</name>";
			streamOut(ostr, (*it).second);
			if(ostr.fail())
			{
				llinfos << "STREAM FAILURE writing '" << (*it).first
						<< "' with sd type " << (*it).second.type() << llendl;
			}
			ostr << "</member>";
		}
		ostr << "</struct>";
#if LL_SPEW_STREAM_OUT_DEBUGGING
		llinfos << "streamOut(map) END" << llendl;
#endif
		break;
	}
	case LLSD::TypeArray:
	{
#if LL_SPEW_STREAM_OUT_DEBUGGING
		llinfos << "streamOut(array) BEGIN" << llendl;
#endif
		ostr << "<array><data>";
		LLSD::array_const_iterator it = sd.beginArray();
		LLSD::array_const_iterator end = sd.endArray();
		for(; it != end; ++it)
		{
			streamOut(ostr, *it);
			if(ostr.fail())
			{
				llinfos << "STREAM FAILURE writing array element sd type "
						<< (*it).type() << llendl;
			}
		}
#if LL_SPEW_STREAM_OUT_DEBUGGING
		llinfos << "streamOut(array) END" << llendl;
#endif
		ostr << "</data></array>";
		break;
	}
	case LLSD::TypeUndefined:
		// treat undefined as a bool with a false value.
	case LLSD::TypeBoolean:
#if LL_SPEW_STREAM_OUT_DEBUGGING
		llinfos << "streamOut(bool)" << llendl;
#endif
		ostr << "<boolean>" << (sd.asBoolean() ? "1" : "0") << "</boolean>";
		break;
	case LLSD::TypeInteger:
#if LL_SPEW_STREAM_OUT_DEBUGGING
		llinfos << "streamOut(int)" << llendl;
#endif
		ostr << "<i4>" << sd.asInteger() << "</i4>";
		break;
	case LLSD::TypeReal:
#if LL_SPEW_STREAM_OUT_DEBUGGING
		llinfos << "streamOut(real)" << llendl;
#endif
		ostr << "<double>" << sd.asReal() << "</double>";
		break;
	case LLSD::TypeString:
#if LL_SPEW_STREAM_OUT_DEBUGGING
		llinfos << "streamOut(string)" << llendl;
#endif
		ostr << "<string>" << xml_escape_string(sd.asString()) << "</string>";
		break;
	case LLSD::TypeUUID:
#if LL_SPEW_STREAM_OUT_DEBUGGING
		llinfos << "streamOut(uuid)" << llendl;
#endif
		// serialize it as a string
		ostr << "<string>" << sd.asString() << "</string>";
		break;
	case LLSD::TypeURI:
	{
#if LL_SPEW_STREAM_OUT_DEBUGGING
		llinfos << "streamOut(uri)" << llendl;
#endif
		// serialize it as a string
		ostr << "<string>" << xml_escape_string(sd.asString()) << "</string>";
		break;
	}
	case LLSD::TypeBinary:
	{
#if LL_SPEW_STREAM_OUT_DEBUGGING
		llinfos << "streamOut(binary)" << llendl;
#endif
		// this is pretty inefficient, but we'll deal with that
		// problem when it becomes one.
		ostr << "<base64>";
		LLSD::Binary buffer = sd.asBinary();
		if(!buffer.empty())
		{
			// *TODO: convert to LLBase64
			int b64_buffer_length = apr_base64_encode_len(buffer.size());
			char* b64_buffer = new char[b64_buffer_length];
			b64_buffer_length = apr_base64_encode_binary(
				b64_buffer,
				&buffer[0],
				buffer.size());
			ostr.write(b64_buffer, b64_buffer_length - 1);
			delete[] b64_buffer;
		}
		ostr << "</base64>";
		break;
	}
	case LLSD::TypeDate:
#if LL_SPEW_STREAM_OUT_DEBUGGING
		llinfos << "streamOut(date)" << llendl;
#endif
		// no need to escape this since it will be alpha-numeric.
		ostr << "<dateTime.iso8601>" << sd.asString() << "</dateTime.iso8601>";
		break;
	default:
		// unhandled type
		llwarns << "Unhandled structured data type: " << sd.type()
			<< llendl;
		break;
	}
	ostr << "</value>";
}

/**
 * LLFilterSD2XMLRPCResponse
 */

LLFilterSD2XMLRPCResponse::LLFilterSD2XMLRPCResponse()
{
}

LLFilterSD2XMLRPCResponse::~LLFilterSD2XMLRPCResponse()
{
}


// virtual
LLIOPipe::EStatus LLFilterSD2XMLRPCResponse::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	PUMP_DEBUG;
	// This pipe does not work if it does not have everyting. This
	// could be addressed by making a stream parser for llsd which
	// handled partial information.
	if(!eos)
	{
		return STATUS_BREAK;
	}

	PUMP_DEBUG;
	// we have everyting in the buffer, so turn the structure data rpc
	// response into an xml rpc response.
	LLBufferStream stream(channels, buffer.get());
	stream << XML_HEADER << XMLRPC_METHOD_RESPONSE_HEADER;
	LLSD sd;
	LLSDSerialize::fromNotation(sd, stream, buffer->count(channels.in()));

	PUMP_DEBUG;
	LLIOPipe::EStatus rv = STATUS_ERROR;
	if(sd.has("response"))
	{
		PUMP_DEBUG;
		// it is a normal response. pack it up and ship it out.
		stream.precision(DEFAULT_PRECISION);
		stream << XMLRPC_RESPONSE_HEADER;
		streamOut(stream, sd["response"]);
		stream << XMLRPC_RESPONSE_FOOTER << XMLRPC_METHOD_RESPONSE_FOOTER;
		rv = STATUS_DONE;
	}
	else if(sd.has("fault"))
	{
		PUMP_DEBUG;
		// it is a fault.
		stream << XMLRPC_FAULT_1 << sd["fault"]["code"].asInteger()
			<< XMLRPC_FAULT_2
			<< xml_escape_string(sd["fault"]["description"].asString())
			<< XMLRPC_FAULT_3 << XMLRPC_METHOD_RESPONSE_FOOTER;
		rv = STATUS_DONE;
	}
	else
	{
		llwarns << "Unable to determine the type of LLSD response." << llendl;
	}
	PUMP_DEBUG;
	return rv;
}

/**
 * LLFilterSD2XMLRPCRequest
 */
LLFilterSD2XMLRPCRequest::LLFilterSD2XMLRPCRequest()
{
}

LLFilterSD2XMLRPCRequest::LLFilterSD2XMLRPCRequest(const char* method)
{
	if(method)
	{
		mMethod.assign(method);
	}
}

LLFilterSD2XMLRPCRequest::~LLFilterSD2XMLRPCRequest()
{
}

// virtual
LLIOPipe::EStatus LLFilterSD2XMLRPCRequest::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	// This pipe does not work if it does not have everyting. This
	// could be addressed by making a stream parser for llsd which
	// handled partial information.
	PUMP_DEBUG;
	if(!eos)
	{
		llinfos << "!eos" << llendl;
		return STATUS_BREAK;
	}

	// See if we can parse it
	LLBufferStream stream(channels, buffer.get());
	LLSD sd;
	LLSDSerialize::fromNotation(sd, stream, buffer->count(channels.in()));
	if(stream.fail())
	{
		llinfos << "STREAM FAILURE reading structure data." << llendl;
	}

	PUMP_DEBUG;
	// We can get the method and parameters from either the member
	// function or passed in via the buffer. We prefer the buffer if
	// we found a parameter and a method, or fall back to using
	// mMethod and putting everyting in the buffer into the parameter.
	std::string method;
	LLSD param_sd;
	if(sd.has("method") && sd.has("parameter"))
	{
		method = sd["method"].asString();
		param_sd = sd["parameter"];
	}
	else
	{
		method = mMethod;
		param_sd = sd;
	}
	if(method.empty())
	{
		llwarns << "SD -> XML Request no method found." << llendl;
		return STATUS_ERROR;
	}

	PUMP_DEBUG;
	// We have a method, and some kind of parameter, so package it up
	// and send it out.
	LLBufferStream ostream(channels, buffer.get());
	ostream.precision(DEFAULT_PRECISION);
	if(ostream.fail())
	{
		llinfos << "STREAM FAILURE setting precision" << llendl;
	}
	ostream << XML_HEADER << XMLRPC_REQUEST_HEADER_1
		<< xml_escape_string(method) << XMLRPC_REQUEST_HEADER_2;
	if(ostream.fail())
	{
		llinfos << "STREAM FAILURE writing method headers" << llendl;
	}
	switch(param_sd.type())
	{
	case LLSD::TypeMap:
		// If the params are a map, then we do not want to iterate
		// through them since the iterators returned will be map
		// ordered un-named values, which will lose the names, and
		// only stream the values, turning it into an array.
		ostream << "<param>";
		streamOut(ostream, param_sd);
		ostream << "</param>";
		break;
	case LLSD::TypeArray:
	{

		LLSD::array_iterator it = param_sd.beginArray();
		LLSD::array_iterator end = param_sd.endArray();
		for(; it != end; ++it)
		{
			ostream << "<param>";
			streamOut(ostream, *it);
			ostream << "</param>";
		}
		break;
	}
	default:
		ostream << "<param>";
		streamOut(ostream, param_sd);
		ostream << "</param>";
		break;
	}

	stream << XMLRPC_REQUEST_FOOTER;
	return STATUS_DONE;
}

/**
 * LLFilterXMLRPCResponse2LLSD
 */
// this is a c function here since it's really an implementation
// detail that requires a header file just get the definition of the
// parameters.
LLIOPipe::EStatus stream_out(std::ostream& ostr, XMLRPC_VALUE value)
{
	XMLRPC_VALUE_TYPE_EASY type = XMLRPC_GetValueTypeEasy(value);
	LLIOPipe::EStatus status = LLIOPipe::STATUS_OK;
	switch(type)
	{
	case xmlrpc_type_base64:
	{
		S32 len = XMLRPC_GetValueStringLen(value);
		const char* buf = XMLRPC_GetValueBase64(value);
		ostr << " b(";
		if((len > 0) && buf)
		{
			ostr << len << ")\"";
			ostr.write(buf, len);
			ostr << "\"";
		}
		else
		{
			ostr << "0)\"\"";
		}
		break;
	}
	case xmlrpc_type_boolean:
		//lldebugs << "stream_out() bool" << llendl;
		ostr << " " << (XMLRPC_GetValueBoolean(value) ? "true" : "false");
		break;
	case xmlrpc_type_datetime:
		ostr << " d\"" << XMLRPC_GetValueDateTime_ISO8601(value) << "\"";
		break;
	case xmlrpc_type_double:
		ostr << " r" << XMLRPC_GetValueDouble(value);
		//lldebugs << "stream_out() double" << XMLRPC_GetValueDouble(value)
		//		 << llendl;
		break;
	case xmlrpc_type_int:
		ostr << " i" << XMLRPC_GetValueInt(value);
		//lldebugs << "stream_out() integer:" << XMLRPC_GetValueInt(value)
		//		 << llendl;
		break;
	case xmlrpc_type_string:
		//lldebugs << "stream_out() string: " << str << llendl;
		ostr << " s(" << XMLRPC_GetValueStringLen(value) << ")'"
			<< XMLRPC_GetValueString(value) << "'";
		break;
	case xmlrpc_type_array: // vector
	case xmlrpc_type_mixed: // vector
	{
		//lldebugs << "stream_out() array" << llendl;
		ostr << " [";
		U32 needs_comma = 0;
		XMLRPC_VALUE current = XMLRPC_VectorRewind(value);
		while(current && (LLIOPipe::STATUS_OK == status))
		{
			if(needs_comma++) ostr << ",";
			status = stream_out(ostr, current);
			current = XMLRPC_VectorNext(value);
		}
		ostr << "]";
		break;
	}
	case xmlrpc_type_struct: // still vector
	{
		//lldebugs << "stream_out() struct" << llendl;
		ostr << " {";
		std::string name;
		U32 needs_comma = 0;
		XMLRPC_VALUE current = XMLRPC_VectorRewind(value);
		while(current && (LLIOPipe::STATUS_OK == status))
		{
			if(needs_comma++) ostr << ",";
			name.assign(XMLRPC_GetValueID(current));
			ostr << "'" << LLSDNotationFormatter::escapeString(name) << "':";
			status = stream_out(ostr, current);
			current = XMLRPC_VectorNext(value);
		}
		ostr << "}";
		break;
	}
	case xmlrpc_type_empty:
	case xmlrpc_type_none:
	default:
		status = LLIOPipe::STATUS_ERROR;
		llwarns << "Found an empty xmlrpc type.." << llendl;
		// not much we can do here...
		break;
	};
	return status;
}

LLFilterXMLRPCResponse2LLSD::LLFilterXMLRPCResponse2LLSD()
{
}

LLFilterXMLRPCResponse2LLSD::~LLFilterXMLRPCResponse2LLSD()
{
}

LLIOPipe::EStatus LLFilterXMLRPCResponse2LLSD::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	PUMP_DEBUG;
	if(!eos) return STATUS_BREAK;
	if(!buffer) return STATUS_ERROR;

	PUMP_DEBUG;
	// *FIX: This technique for reading data is far from optimal. We
	// need to have some kind of istream interface into the xml
	// parser...
	S32 bytes = buffer->countAfter(channels.in(), NULL);
	if(!bytes) return STATUS_ERROR;
	char* buf = new char[bytes + 1];
	buf[bytes] = '\0';
	buffer->readAfter(channels.in(), NULL, (U8*)buf, bytes);

	//lldebugs << "xmlrpc response: " << buf << llendl;

	PUMP_DEBUG;
	XMLRPC_REQUEST response = XMLRPC_REQUEST_FromXML(
		buf,
		bytes,
		NULL);
	if(!response)
	{
		llwarns << "XML -> SD Response unable to parse xml." << llendl;
		delete[] buf;
		return STATUS_ERROR;
	}

	PUMP_DEBUG;
	LLBufferStream stream(channels, buffer.get());
	stream.precision(DEFAULT_PRECISION);
	if(XMLRPC_ResponseIsFault(response))
	{
		PUMP_DEBUG;
		stream << LLSDRPC_FAULT_HADER_1
			   << XMLRPC_GetResponseFaultCode(response)
			   << LLSDRPC_FAULT_HADER_2;
		const char* fault_str = XMLRPC_GetResponseFaultString(response);
		std::string fault_string;
		if(fault_str)
		{
			fault_string.assign(fault_str);
		}
		stream << "'" << LLSDNotationFormatter::escapeString(fault_string)
		   << "'" <<LLSDRPC_FAULT_FOOTER;
	}
	else
	{
		PUMP_DEBUG;
		stream << LLSDRPC_RESPONSE_HEADER;
		XMLRPC_VALUE param = XMLRPC_RequestGetData(response);
		if(param)
		{
			stream_out(stream, param);
		}
		stream << LLSDRPC_RESPONSE_FOOTER;
	}
	PUMP_DEBUG;
	XMLRPC_RequestFree(response, 1);
	delete[] buf;
	PUMP_DEBUG;
	return STATUS_DONE;
}

/**
 * LLFilterXMLRPCRequest2LLSD
 */
LLFilterXMLRPCRequest2LLSD::LLFilterXMLRPCRequest2LLSD()
{
}

LLFilterXMLRPCRequest2LLSD::~LLFilterXMLRPCRequest2LLSD()
{
}

LLIOPipe::EStatus LLFilterXMLRPCRequest2LLSD::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	PUMP_DEBUG;
	if(!eos) return STATUS_BREAK;
	if(!buffer) return STATUS_ERROR;

	PUMP_DEBUG;
	// *FIX: This technique for reading data is far from optimal. We
	// need to have some kind of istream interface into the xml
	// parser...
	S32 bytes = buffer->countAfter(channels.in(), NULL);
	if(!bytes) return STATUS_ERROR;
	char* buf = new char[bytes + 1];
	buf[bytes] = '\0';
	buffer->readAfter(channels.in(), NULL, (U8*)buf, bytes);

	//lldebugs << "xmlrpc request: " << buf << llendl;
	
	// Check the value in the buffer. XMLRPC_REQUEST_FromXML will report a error code 4 if 
	// values that are less than 0x20 are passed to it, except
	// 0x09: Horizontal tab; 0x0a: New Line; 0x0d: Carriage
	U8* cur_pBuf = (U8*)buf;
    U8 cur_char;
	for (S32 i=0; i<bytes; i++) 
	{
        cur_char = *cur_pBuf;
		if (   cur_char < 0x20
            && 0x09 != cur_char
            && 0x0a != cur_char
            && 0x0d != cur_char )
        {
			*cur_pBuf = '?';
        }
		++cur_pBuf;
	}

	PUMP_DEBUG;
	XMLRPC_REQUEST request = XMLRPC_REQUEST_FromXML(
		buf,
		bytes,
		NULL);
	if(!request)
	{
		llwarns << "XML -> SD Request process parse error." << llendl;
		delete[] buf;
		return STATUS_ERROR;
	}

	PUMP_DEBUG;
	LLBufferStream stream(channels, buffer.get());
	stream.precision(DEFAULT_PRECISION);
	const char* name = XMLRPC_RequestGetMethodName(request);
	stream << LLSDRPC_REQUEST_HEADER_1 << (name ? name : "")
		   << LLSDRPC_REQUEST_HEADER_2;
	XMLRPC_VALUE param = XMLRPC_RequestGetData(request);
	if(param)
	{
		PUMP_DEBUG;
		S32 size = XMLRPC_VectorSize(param);
		if(size > 1)
		{
			// if there are multiple parameters, stuff the values into
			// an array so that the next step in the chain can read them.
			stream << "[";
		}
 		XMLRPC_VALUE current = XMLRPC_VectorRewind(param);
		bool needs_comma = false;
 		while(current)
 		{
			if(needs_comma)
			{
				stream << ",";
			}
			needs_comma = true;
 			stream_out(stream, current);
 			current = XMLRPC_VectorNext(param);
 		}
		if(size > 1)
		{
			// close the array
			stream << "]";
		}
	}
	stream << LLSDRPC_REQUEST_FOOTER;
	XMLRPC_RequestFree(request, 1);
	delete[] buf;
	PUMP_DEBUG;
	return STATUS_DONE;
}

