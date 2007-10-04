/** 
 * @file llhttpnode.h
 * @brief Declaration of classes for generic HTTP/LSL/REST handling.
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

#ifndef LL_LLHTTPNODE_H
#define LL_LLHTTPNODE_H

#include "llmemory.h"
#include "llsd.h"

class LLChainIOFactory;


/**
 * These classes represent the HTTP framework: The URL tree, and the LLSD
 * REST interface that such nodes implement.
 * 
 * To implement a service, in most cases, subclass LLHTTPNode, implement
 * get() or post(), and create a global instance of LLHTTPRegistration<>.
 * This can all be done in a .cpp file, with no publically declared parts.
 * 
 * To implement a server see lliohttpserver.h
 * @see LLHTTPWireServer
 */

/** 
 * @class LLHTTPNode
 * @brief Base class which handles url traversal, response routing
 * and support for standard LLSD services
 *
 * Users of the HTTP responder will typically derive a class from this
 * one, implement the get(), put() and/or post() methods, and then
 * use LLHTTPRegistration to insert it into the URL tree.
 *
 * The default implementation handles servicing the request and creating
 * the pipe fittings needed to read the headers, manage them, convert
 * to and from LLSD, etc.
 */
class LLHTTPNode
{
public:
	LLHTTPNode();
	virtual ~LLHTTPNode();

	/** @name Responses
		Most subclasses override one or more of these methods to provide
		the service.  By default, the rest of the LLHTTPNode architecture
		will handle requests, create the needed LLIOPump, parse the input
		to LLSD, and format the LLSD result to the output.
		
		The default implementation of each of these is to call
		response->methodNotAllowed();  The "simple" versions can be
		overridden instead in those cases where the service can return
		an immediately computed response.
	*/
	//@{
public:	
		virtual LLSD get() const;
		virtual LLSD put(const LLSD& input) const;
		virtual LLSD post(const LLSD& input) const;

		virtual LLSD del() const;
		virtual LLSD del(const LLSD& context) const;

		class Response : public LLRefCount
		{
		protected:
			virtual ~Response();

		public:
			virtual void result(const LLSD&) = 0;
			virtual void status(S32 code, const std::string& message) = 0;

			void status(S32 code);
			void notFound(const std::string& message);
			void notFound();
			void methodNotAllowed();
		};

		typedef LLPointer<Response> ResponsePtr;

		virtual void get(ResponsePtr, const LLSD& context) const;
		virtual void put(ResponsePtr, const LLSD& context, const LLSD& input) const;
		virtual void post(ResponsePtr, const LLSD& context, const LLSD& input) const;
		virtual void del(ResponsePtr, const LLSD& context) const;
	//@}
	

	/** @name URL traversal
		 The tree is traversed by calling getChild() with successive
		 path components, on successive results.  When getChild() returns
		 null, or there are no more components, the last child responds to
		 the request.
		 
		 The default behavior is generally correct, though wildcard nodes
		 will want to implement validate().
	*/
	//@{
public:
	virtual LLHTTPNode* getChild(const std::string& name, LLSD& context) const;
		/**< returns a child node, if any, at the given name
			 default looks at children and wildcard child (see below)
		*/
		
	virtual bool handles(const LLSD& remainder, LLSD& context) const;
		/**< return true if this node can service the remaining components;
			 default returns true if there are no remaining components
		*/
		
	virtual bool validate(const std::string& name, LLSD& context) const;
		/**< called only on wildcard nodes, to check if they will handle
			 the name;	default is false;  overrides will want to check
			 name, and return true if the name will construct to a valid url.
			 For convenience, the <code>getChild()</code> method above will
			 automatically insert the name in
			 context["request"]["wildcard"][key] if this method returns true.
			 For example, the node "agent/<agent_id>/detail" will set
			 context["request"]["wildcard"]["agent_id"] eqaul to the value 
			 found during traversal.
		*/
		
	const LLHTTPNode* traverse(const std::string& path, LLSD& context) const;
		/**< find a node, if any, that can service this path
			 set up context["request"] information 
 		*/
 	//@}
 
	/** @name Child Nodes
		 The standard node can have any number of child nodes under
		 fixed names, and optionally one "wildcard" node that can
		 handle all other names.
		 
		 Usually, child nodes are add through LLHTTPRegistration, not
		 by calling this interface directly.
		 
		 The added node will be now owned by the parent node.
	*/
	//@{
	
	virtual void addNode(const std::string& path, LLHTTPNode* nodeToAdd);

	LLSD allNodePaths() const;
		///< Returns an arrary of node paths at and under this node

	const LLHTTPNode* rootNode() const;
	const LLHTTPNode* findNode(const std::string& name) const;

	//@}

	/* @name Description system
		The Description object contains information about a service.
		All subclasses of LLHTTPNode should override description() and use
		the methods of the Description class to set the various properties.
	 */
	//@{
		class Description
		{
		public:
			void shortInfo(const std::string& s){ mInfo["description"] = s; }
			void longInfo(const std::string& s)	{ mInfo["details"] = s; }

			// Call this method when the service supports the specified verb.
			void getAPI() { mInfo["api"].append("GET"); }
			void putAPI() { mInfo["api"].append("PUT");  }
			void postAPI() { mInfo["api"].append("POST"); }
			void delAPI() { mInfo["api"].append("DELETE"); }

			void input(const std::string& s)	{ mInfo["input"] = s; }
			void output(const std::string& s)	{ mInfo["output"] = s; }
			void source(const char* f, int l)	{ mInfo["__file__"] = f;
												  mInfo["__line__"] = l; }
			
			LLSD getInfo() const { return mInfo; }
			
		private:
			LLSD mInfo;
		};
	
	virtual void describe(Description&) const;
	
	//@}
	
	
	virtual const LLChainIOFactory* getProtocolHandler() const;
		/**< Return a factory object for handling wire protocols.
		 *   The base class returns NULL, as it doesn't know about
		 *   wire protocols at all.  This is okay for most nodes
		 *   as LLIOHTTPServer is smart enough to use a default
		 *   wire protocol for HTTP for such nodes.  Specialized
		 *   subclasses that handle things like XML-RPC will want
		 *   to implement this.  (See LLXMLSDRPCServerFactory.)
		 */

private:
	class Impl;
	Impl& impl;
};



class LLSimpleResponse : public LLHTTPNode::Response
{
public:
	static LLPointer<LLSimpleResponse> create();
	
	void result(const LLSD& result);
	void status(S32 code, const std::string& message);

	void print(std::ostream& out) const;

	S32 mCode;
	std::string mMessage;

protected:
	~LLSimpleResponse();

private:
	LLSimpleResponse() {;} // Must be accessed through LLPointer.
};

std::ostream& operator<<(std::ostream& out, const LLSimpleResponse& resp);



/** 
 * @name Automatic LLHTTPNode registration 
 *
 * To register a node type at a particular url path, construct a global instance
 * of LLHTTPRegistration:
 *
 *		LLHTTPRegistration<LLMyNodeType> gHTTPServiceAlphaBeta("/alpha/beta");
 *
 * (Note the naming convention carefully.)  This object must be global and not
 * static.  However, it needn't be declared in your .h file.  It can exist
 * solely in the .cpp file.  The same is true of your subclass of LLHTTPNode:
 * it can be declared and defined wholly within the .cpp file.
 *
 * When constructing a web server, use LLHTTPRegistrar to add all the registered
 * nodes to the url tree:
 *
 *		LLHTTPRegistrar::buidlAllServices(mRootNode);
 */
//@{

class LLHTTPRegistrar
{
public:
	class NodeFactory
	{
	public:
		virtual ~NodeFactory();
		virtual LLHTTPNode* build() const = 0;
	};

	static void buildAllServices(LLHTTPNode& root);

	static void registerFactory(const std::string& path, NodeFactory& factory);
		///< construct an LLHTTPRegistration below to call this
};

template < class NodeType >
class LLHTTPRegistration
{
public:
	LLHTTPRegistration(const std::string& path)
	{
		LLHTTPRegistrar::registerFactory(path, mFactory);
	}

private:
	class ThisNodeFactory : public LLHTTPRegistrar::NodeFactory
	{
	public:
		virtual LLHTTPNode* build() const { return new NodeType; }
	};
	
	ThisNodeFactory	mFactory;	
};

template < class NodeType>
class LLHTTPParamRegistration
{
public:
	LLHTTPParamRegistration(const std::string& path, LLSD params) :
		mFactory(params)
	{
		LLHTTPRegistrar::registerFactory(path, mFactory);
	}

private:
	class ThisNodeFactory : public LLHTTPRegistrar::NodeFactory
	{
	public:
		ThisNodeFactory(LLSD params) : mParams(params) {}
		virtual LLHTTPNode* build() const { return new NodeType(mParams); }
	private:
		LLSD mParams;
	};
	
	ThisNodeFactory	mFactory;	
};
	
//@}

#endif // LL_LLHTTPNODE_H
