/** 
 * @file llsdrpcserver.h
 * @author Phoenix
 * @date 2005-10-11
 * @brief Declaration of the structured data remote procedure call server.
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

#ifndef LL_LLSDRPCSERVER_H
#define LL_LLSDRPCSERVER_H

/** 
 * I've set this up to be pretty easy to use when you want to make a
 * structured data rpc server which responds to methods by
 * name. Derive a class from the LLSDRPCServer, and during
 * construction (or initialization if you have the luxury) map method
 * names to pointers to member functions. This will look a lot like:
 *
 * <code>
 *  class LLMessageAgents : public LLSDRPCServer {<br>
 *  public:<br>
 *    typedef LLSDRPCServer<LLUsher> mem_fn_t;<br>
 *    LLMessageAgents() {<br>
 *      mMethods["message"] = new mem_fn_t(this, &LLMessageAgents::rpc_IM);<br>
 *      mMethods["alert"] = new mem_fn_t(this, &LLMessageAgents::rpc_Alrt);<br>
 *    }<br>
 *  protected:<br>
 *    rpc_IM(const LLSD& params,
 *		const LLChannelDescriptors& channels,
 *		LLBufferArray* data)
 *		{...}<br>
 *    rpc_Alert(const LLSD& params,
 *		const LLChannelDescriptors& channels,
 *		LLBufferArray* data)
 *		{...}<br>
 *  };<br>
 * </code>
 *
 * The params are an array where each element in the array is a single
 * parameter in the call.
 *
 * It is up to you to pack a valid serialized llsd response into the
 * data object passed into the method, but you can use the helper
 * methods below to help.
 */

#include <map>
#include "lliopipe.h"
#include "lliohttpserver.h"
#include "llfiltersd2xmlrpc.h"

class LLSD;

/** 
 * @brief Enumeration for specifying server method call status. This
 * enumeration controls how the server class will manage the pump
 * process/callback mechanism.
 */
enum ESDRPCSStatus
{
 	// The call went ok, but the response is not yet ready. The
 	// method will arrange for the clearLock() call to be made at
 	// a later date, after which, once the chain is being pumped
	// again, deferredResponse() will be called to gather the result
 	ESDRPCS_DEFERRED,

	// The LLSDRPCServer would like to handle the method on the
	// callback queue of the pump.
	ESDRPCS_CALLBACK,

	// The method call finished and generated output.
	ESDRPCS_DONE,

	// Method failed for some unspecified reason - you should avoid
	// this. A generic fault will be sent to the output.
	ESDRPCS_ERROR,

	ESDRPCS_COUNT,
};

/** 
 * @class LLSDRPCMethodCallBase
 * @brief Base class for calling a member function in an sd rpcserver
 * implementation.
 */
class LLSDRPCMethodCallBase
{
public:
	LLSDRPCMethodCallBase() {}
	virtual ~LLSDRPCMethodCallBase() {}

	virtual ESDRPCSStatus call(
		const LLSD& params,
		const LLChannelDescriptors& channels,
		LLBufferArray* response) = 0;
protected:
};

/** 
 * @class LLSDRPCMethodCall
 * @brief Class which implements member function calls.
 */
template<class Server>
class LLSDRPCMethodCall : public LLSDRPCMethodCallBase
{
public:
	typedef ESDRPCSStatus (Server::*mem_fn)(
		const LLSD& params,
		const LLChannelDescriptors& channels,
		LLBufferArray* data);
	LLSDRPCMethodCall(Server* s, mem_fn fn) :
		mServer(s),
		mMemFn(fn)
	{
	}
	virtual ~LLSDRPCMethodCall() {}
	virtual ESDRPCSStatus call(
		const LLSD& params,
		const LLChannelDescriptors& channels,
		LLBufferArray* data)
	{
		return (*mServer.*mMemFn)(params, channels, data);
	}

protected:
	Server* mServer;
	mem_fn mMemFn;
	//bool (Server::*mMemFn)(const LLSD& params, LLBufferArray& data);
};


/** 
 * @class LLSDRPCServer
 * @brief Basic implementation of a structure data rpc server
 *
 * The rpc server is also designed to appropriately straddle the pump
 * <code>process()</code> and <code>callback()</code> to specify which
 * thread you want to work on when handling a method call. The
 * <code>mMethods</code> methods are called from
 * <code>process()</code>, while the <code>mCallbackMethods</code> are
 * called when a pump is in a <code>callback()</code> cycle.
 */
class LLSDRPCServer : public LLIOPipe
{
public:
	LLSDRPCServer();
	virtual ~LLSDRPCServer();

	/**
	 * enumeration for generic fault codes
	 */
	enum
	{
		FAULT_BAD_REQUEST = 2000,
		FAULT_NO_RESPONSE = 2001,
	};

	/** 
	 * @brief Call this method to return an rpc fault.
	 *
	 * @param channel The channel for output on the data buffer
	 * @param data buffer which will recieve the final output 
	 * @param code The fault code 
	 * @param msg The fault message 
	 */
	static void buildFault(
		const LLChannelDescriptors& channels,
		LLBufferArray* data,
		S32 code,
		const std::string& msg);

	/** 
	 * @brief Call this method to build an rpc response.
	 *
	 * @param channel The channel for output on the data buffer
	 * @param data buffer which will recieve the final output 
	 * @param response The return value from the method call
	 */
	static void buildResponse(
		const LLChannelDescriptors& channels,
		LLBufferArray* data,
		const LLSD& response);

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data in buffer
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:

	/** 
	 * @brief Enumeration to track the state of the rpc server instance
	 */
	enum EState
	{
		STATE_NONE,
		STATE_CALLBACK,
		STATE_DEFERRED,
		STATE_DONE
	};

	/** 
	 * @brief This method is called when an http post comes in.
	 *
	 * The default behavior is to look at the method name, look up the
	 * method in the method table, and call it. If the method is not
	 * found, this function will build a fault response.  You can
	 * implement your own version of this function if you want to hard
	 * wire some behavior or optimize things a bit.
	 * @param method The method name being called
	 * @param params The parameters
	 * @param channel The channel for output on the data buffer
	 * @param data The http data
	 * @return Returns the status of the method call, done/deferred/etc
	 */
	virtual ESDRPCSStatus callMethod(
		const std::string& method,
		const LLSD& params,
		const LLChannelDescriptors& channels,
		LLBufferArray* data);

	/** 
	 * @brief This method is called when a pump callback is processed.
	 *
	 * The default behavior is to look at the method name, look up the
	 * method in the callback method table, and call it. If the method
	 * is not found, this function will build a fault response.  You
	 * can implement your own version of this function if you want to
	 * hard wire some behavior or optimize things a bit.
	 * @param method The method name being called
	 * @param params The parameters
	 * @param channel The channel for output on the data buffer
	 * @param data The http data
	 * @return Returns the status of the method call, done/deferred/etc
	 */
	virtual ESDRPCSStatus callbackMethod(
		const std::string& method,
		const LLSD& params,
		const LLChannelDescriptors& channels,
		LLBufferArray* data);

	/**
	 * @brief Called after a deferred service is unlocked
	 *
	 * If a method returns ESDRPCS_DEFERRED, then the service chain
	 * will be locked and not processed until some other system calls
	 * clearLock() on the service instance again.  At that point,
	 * once the pump starts processing the chain again, this method
	 * will be called so the service can output the final result
	 * into the buffers.
	 */
	virtual ESDRPCSStatus deferredResponse(
		const LLChannelDescriptors& channels,
		LLBufferArray* data);

	// donovan put this public here 7/27/06
public:
	/**
	 * @brief unlock a service that as ESDRPCS_DEFERRED
	 */
	void clearLock();

protected:
	EState mState;
	LLSD mRequest;
	LLPumpIO* mPump;
	S32 mLock;
	typedef std::map<std::string, LLSDRPCMethodCallBase*> method_map_t;
	method_map_t mMethods;
	method_map_t mCallbackMethods;
};

/** 
 * @name Helper Templates for making LLHTTPNodes
 *
 * These templates help in creating nodes for handing a service from
 * either SDRPC or XMLRPC, given a single implementation of LLSDRPCServer.
 *
 * To use it:
 * \code
 *  class LLUsefulServer : public LLSDRPCServer { ... }
 *
 *  LLHTTPNode& root = LLCreateHTTPWireServer(...);
 *  root.addNode("llsdrpc/useful", new LLSDRPCNode<LLUsefulServer>);
 *  root.addNode("xmlrpc/useful", new LLXMLRPCNode<LLUsefulServer>);
 * \endcode
 */
//@{

template<class Server>
class LLSDRPCServerFactory : public LLChainIOFactory
{
public:
	virtual bool build(LLPumpIO::chain_t& chain, LLSD context) const
	{
		lldebugs << "LLXMLSDRPCServerFactory::build" << llendl;
		chain.push_back(LLIOPipe::ptr_t(new Server));
		return true;
	}
};

template<class Server>
class LLSDRPCNode : public LLHTTPNodeForFactory<
								LLSDRPCServerFactory<Server> >
{
};

template<class Server>
class LLXMLRPCServerFactory : public LLChainIOFactory
{
public:
	virtual bool build(LLPumpIO::chain_t& chain, LLSD context) const
	{
		lldebugs << "LLXMLSDRPCServerFactory::build" << llendl;
		chain.push_back(LLIOPipe::ptr_t(new LLFilterXMLRPCRequest2LLSD));
		chain.push_back(LLIOPipe::ptr_t(new Server));
		chain.push_back(LLIOPipe::ptr_t(new LLFilterSD2XMLRPCResponse));
		return true;
	}
};

template<class Server>
class LLXMLRPCNode : public LLHTTPNodeForFactory<
					 			LLXMLRPCServerFactory<Server> >
{
};

//@}

#endif // LL_LLSDRPCSERVER_H
