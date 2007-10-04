/** 
 * @file lliohttpserver.h
 * @brief Declaration of function for creating an HTTP wire server
 * @see LLIOServerSocket, LLPumpIO
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
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

#ifndef LL_LLIOHTTPSERVER_H
#define LL_LLIOHTTPSERVER_H

#include "llapr.h"
#include "llchainio.h"
#include "llhttpnode.h"

class LLPumpIO;

class LLIOHTTPServer
{
public:
	typedef void (*timing_callback_t)(const char* hashed_name, F32 time, void* data);

	static LLHTTPNode& create(apr_pool_t* pool, LLPumpIO& pump, U16 port);
	/**< Creates an HTTP wire server on the pump for the given TCP port.
	 *
	 *   Returns the root node of the new server.  Add LLHTTPNode instances
	 *   to this root.
	 *
	 *   Nodes that return NULL for getProtocolHandler(), will use the
	 *   default handler that interprets HTTP on the wire and converts
	 *   it into calls to get(), put(), post(), del() with appropriate
	 *   LLSD arguments and results.
	 *
	 *   To have nodes that implement some other wire protocol (XML-RPC
	 *   for example), use the helper templates below.
	 */
 
	static void createPipe(LLPumpIO::chain_t& chain, 
            const LLHTTPNode& root, const LLSD& ctx);
	/**< Create a pipe on the chain that handles HTTP requests.
	 *   The requests are served by the node tree given at root.
	 *
	 *   This is primarily useful for unit testing.
	 */

	static void setTimingCallback(timing_callback_t callback, void* data);
	/**< Register a callback function that will be called every time
	*    a GET, PUT, POST, or DELETE is handled.
	*
	* This is used to time the LLHTTPNode handler code, which often hits
	* the database or does other, slow operations. JC
	*/
};

/* @name Helper Templates
 *
 * These templates make it easy to create nodes that use thier own protocol
 * handlers rather than the default.  Typically, you subclass LLIOPipe to
 * implement the protocol, and then add a node using the templates:
 *
 * rootNode->addNode("thing", new LLHTTPNodeForPipe<LLThingPipe>);
 *
 * The templates are:
 *
 * 	LLChainIOFactoryForPipe
 * 		- a simple factory that builds instances of a pipe
 *
 * 	LLHTTPNodeForFacotry
 * 		- a HTTP node that uses a factory as the protocol handler
 *
 * 	LLHTTPNodeForPipe
 * 		- a HTTP node that uses a simple factory based on a pipe
 */
//@{

template<class Pipe>
class LLChainIOFactoryForPipe : public LLChainIOFactory
{
public:
	virtual bool build(LLPumpIO::chain_t& chain, LLSD context) const
	{   
		chain.push_back(LLIOPipe::ptr_t(new Pipe));
		return true;
	}
};

template<class Factory>
class LLHTTPNodeForFactory : public LLHTTPNode
{
public:
	const LLChainIOFactory* getProtocolHandler() const
		{ return &mProtocolHandler; }

private:
	Factory mProtocolHandler;
};

//@}


template<class Pipe>
class LLHTTPNodeForPipe : public LLHTTPNodeForFactory<
						  			LLChainIOFactoryForPipe<Pipe> >
{
};


#endif // LL_LLIOHTTPSERVER_H

