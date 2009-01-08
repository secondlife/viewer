/** 
 * @file llfiltersd2xmlrpc.h
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

#ifndef LL_LLFILTERSD2XMLRPC_H
#define LL_LLFILTERSD2XMLRPC_H

/**
 * These classes implement the necessary pipes for translating between
 * xmlrpc and llsd rpc. The llsd rpcs mechanism was developed as an
 * extensible and easy to parse serialization grammer which maintains
 * a time efficient in-memory representation.
 */

#include <iosfwd>
#include "lliopipe.h"

/** 
 * @class LLFilterSD2XMLRPC
 * @brief Filter from serialized LLSD to an XMLRPC method call 
 *
 * This clas provides common functionality for the LLFilterSD2XMLRPRC
 * request and response classes.
 */
class LLFilterSD2XMLRPC : public LLIOPipe
{
public:
	LLFilterSD2XMLRPC();
	virtual ~LLFilterSD2XMLRPC();

protected:
	/** 
	 * @brief helper method
	 */
	void streamOut(std::ostream& ostr, const LLSD& sd);
};

/** 
 * @class LLFilterSD2XMLRPCResponse
 * @brief Filter from serialized LLSD to an XMLRPC response 
 *
 * This class filters a serialized LLSD object to an xmlrpc
 * repsonse. Since resonses are limited to a single param, the xmlrprc
 * response only serializes it as one object.
 * This class correctly handles normal llsd responses as well as llsd
 * rpc faults.
 *
 * For example, if given:
 * <code>{'response':[ i200, r3.4, {"foo":"bar"} ]}</code>
 * Would generate:
 * <code>
 *  <?xml version="1.0"?>
 *  <methodResponse><params><param><array><data>
 *    <value><int>200</int></value>
 *    <value><double>3.4</double></value>
 *    <value><struct><member>
 *      <name>foo</name><value><string>bar</string></value></member>
 *      </struct></value>
 *  </data></array></param></params></methodResponse>
 * </code>
 */
class LLFilterSD2XMLRPCResponse : public LLFilterSD2XMLRPC
{
public:
	// constructor
	LLFilterSD2XMLRPCResponse();

	// destructor
	virtual ~LLFilterSD2XMLRPCResponse();

	/* @name LLIOPipe virtual implementations
	 */
	//@{
protected:
	/** 
	 * @brief Process the data in buffer.
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}
};

/** 
 * @class LLFilterSD2XMLRPCRequest
 * @brief Filter from serialized LLSD to an XMLRPC method call 
 *
 * This class will accept any kind of serialized LLSD object, but you
 * probably want to have an array on the outer boundary since this
 * object will interpret each element in the top level LLSD as a
 * parameter into the xmlrpc spec.
 *
 * For example, you would represent 3 params as:
 * <code>
 *  {'method'='foo', 'parameter':[i200, r3.4, {"foo":"bar"}]}
 * </code>
 * To generate:
 * <code>
 *  <?xml version="1.0"?>
 *  <methodCall><params>
 *  <param><value><int>200</int></value></param>
 *  <param><value><double>3.4</double></value></param>
 *  <param><value><struct><member>
 *    <name>foo</name><value><string>bar</string></value></member>
 *    </struct></value></param>
 *  </params></methodCall>
 *
 * This class will accept 2 different kinds of encodings. The first
 * just an array of params as long as you specify the method in the
 * constructor. It will also accept a structured data in the form:
 * {'method':'$method_name', 'parameter':[...] } In the latter form, the
 * encoded 'method' will be used regardless of the construction of the
 * object, and the 'parameter' will be used as parameter to the call.
 */
class LLFilterSD2XMLRPCRequest : public LLFilterSD2XMLRPC
{
public:
	// constructor
	LLFilterSD2XMLRPCRequest();

	// constructor
	LLFilterSD2XMLRPCRequest(const char* method);

	// destructor
	virtual ~LLFilterSD2XMLRPCRequest();

	/* @name LLIOPipe virtual implementations
	 */
	//@{
protected:
	/** 
	 * @brief Process the data in buffer.
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:
	// The method name of this request.
	std::string mMethod;
};

/** 
 * @class LLFilterXMLRPCResponse2LLSD
 * @brief Filter from serialized XMLRPC method response to LLSD 
 *
 * The xmlrpc spec states that responses can only have one element
 * which can be of any supported type.
 * This takes in  xml of the form:
 * <code>
 * <?xml version=\"1.0\"?><methodResponse><params><param>
 * <value><string>ok</string></value></param></params></methodResponse>
 * </code>
 * And processes it into:
 *  <code>'ok'</code>
 *
 */
class LLFilterXMLRPCResponse2LLSD : public LLIOPipe
{
public:
	// constructor
	LLFilterXMLRPCResponse2LLSD();

	// destructor
	virtual ~LLFilterXMLRPCResponse2LLSD();

	/* @name LLIOPipe virtual implementations
	 */
	//@{
protected:
	/** 
	 * @brief Process the data in buffer.
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:
};

/** 
 * @class LLFilterXMLRPCRequest2LLSD
 * @brief Filter from serialized XMLRPC method call to LLSD 
 *
 * This takes in  xml of the form:
 * <code>
 *  <?xml version=\"1.0\"?><methodCall>
 *  <methodName>repeat</methodName>
 *  <params>
 *  <param><value><i4>4</i4></value></param>
 *  <param><value><string>ok</string></value></param>
 *  </params></methodCall>
 * </code>
 * And processes it into:
 *  <code>{ 'method':'repeat', 'params':[i4, 'ok'] }</code>
 */
class LLFilterXMLRPCRequest2LLSD : public LLIOPipe
{
public:
	// constructor
	LLFilterXMLRPCRequest2LLSD();

	// destructor
	virtual ~LLFilterXMLRPCRequest2LLSD();

	/* @name LLIOPipe virtual implementations
	 */
	//@{
protected:
	/** 
	 * @brief Process the data in buffer.
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:
};

/**
 * @brief This function takes string, and escapes it appropritately
 * for inclusion as xml data.
 */
std::string xml_escape_string(const std::string& in);

/**
 * @brief Externally available constants
 */
extern const char LLSDRPC_REQUEST_HEADER_1[];
extern const char LLSDRPC_REQUEST_HEADER_2[];
extern const char LLSDRPC_REQUEST_FOOTER[];

#endif // LL_LLFILTERSD2XMLRPC_H
