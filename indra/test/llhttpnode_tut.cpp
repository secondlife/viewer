/** 
 * @file lliohttpnode_tut.cpp
 * @date   May 2006
 * @brief HTTP server unit tests
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include <tut/tut.h>
#include "lltut.h"

#include "llhttpnode.h"
#include "llsdhttpserver.h"

namespace tut
{
	struct HTTPNodeTestData
	{
		LLHTTPNode mRoot;
		LLSD mContext;
	
		const LLSD& context() { return mContext; }
		
		std::string remainderPath()
		{
			std::ostringstream pathOutput;
			bool addSlash = false;
			
			LLSD& remainder = mContext["request"]["remainder"];
			for (LLSD::array_const_iterator i = remainder.beginArray();
				i != remainder.endArray();
				++i)
			{
				if (addSlash) { pathOutput << '/'; }
				pathOutput << i->asString();
				addSlash = true;
			}
			
			return pathOutput.str();
		}
		
		void ensureRootTraversal(const std::string& path,
			const LLHTTPNode* expectedNode,
			const char* expectedRemainder)
		{
			mContext.clear();
			
			const LLHTTPNode* actualNode = mRoot.traverse(path, mContext);
			
			ensure_equals("traverse " + path + " node",
				actualNode, expectedNode);
			ensure_equals("traverse " + path + " remainder",
				remainderPath(), expectedRemainder);
		}

		class Response : public LLHTTPNode::Response
		{
		public:
			static LLPointer<Response> create() {return new Response();}

			LLSD mResult;

			void result(const LLSD& result) { mResult = result; }
			void status(S32 code, const std::string& message) { }

		private:
			Response() {;} // Must be accessed through LLPointer.
		};

		typedef LLPointer<Response> ResponsePtr;

		LLSD get(const std::string& path)
		{
			mContext.clear();
			const LLHTTPNode* node = mRoot.traverse(path, mContext);
			ensure(path + " found", node != NULL);

			ResponsePtr response = Response::create();
			node->get(LLHTTPNode::ResponsePtr(response), mContext);
			return response->mResult;
		}
		
		LLSD post(const std::string& path, const LLSD& input)
		{
			mContext.clear();
			const LLHTTPNode* node = mRoot.traverse(path, mContext);
			ensure(path + " found", node != NULL);

			ResponsePtr response = Response::create();
			node->post(LLHTTPNode::ResponsePtr(response), mContext, input);
			return response->mResult;
		}
		
		void ensureMemberString(const std::string& name,
			const LLSD& actualMap, const std::string& member,
			const std::string& expectedValue)
		{
			ensure_equals(name + " " + member,
				actualMap[member].asString(), expectedValue);
		}
		
		
		void ensureInArray(const LLSD& actualArray,
			const std::string& expectedValue)
		{
			LLSD::array_const_iterator i = actualArray.beginArray();
			LLSD::array_const_iterator end = actualArray.endArray();
		
			for (; i != end; ++i)
			{
				std::string path = i->asString();

				if (path == expectedValue)
				{
					return;
				}
			}
			
			fail("didn't find " + expectedValue);
		}
			
	};
	
	typedef test_group<HTTPNodeTestData>	HTTPNodeTestGroup;
	typedef HTTPNodeTestGroup::object		HTTPNodeTestObject;
	HTTPNodeTestGroup httpNodeTestGroup("http node");

	template<> template<>
	void HTTPNodeTestObject::test<1>()
	{
		// traversal of the lone node
		
		ensureRootTraversal("", &mRoot, "");
		ensureRootTraversal("/", &mRoot, "");
		ensureRootTraversal("foo", NULL, "foo");
		ensureRootTraversal("foo/bar", NULL, "foo/bar");
		
		ensure_equals("root of root", mRoot.rootNode(), &mRoot);
	}
	
	template<> template<>
	void HTTPNodeTestObject::test<2>()
	{
		// simple traversal of a single node
		
		LLHTTPNode* helloNode = new LLHTTPNode;
		mRoot.addNode("hello", helloNode);

		ensureRootTraversal("hello", helloNode, "");
		ensureRootTraversal("/hello", helloNode, "");
		ensureRootTraversal("hello/", helloNode, "");
		ensureRootTraversal("/hello/", helloNode, "");

		ensureRootTraversal("hello/there", NULL, "there");
		
		ensure_equals("root of hello", helloNode->rootNode(), &mRoot);
	}

	template<> template<>
	void HTTPNodeTestObject::test<3>()
	{
		// traversal of mutli-branched tree
		
		LLHTTPNode* greekNode = new LLHTTPNode;
		LLHTTPNode* alphaNode = new LLHTTPNode;
		LLHTTPNode* betaNode = new LLHTTPNode;
		LLHTTPNode* gammaNode = new LLHTTPNode;
		
		greekNode->addNode("alpha", alphaNode);
		greekNode->addNode("beta", betaNode);
		greekNode->addNode("gamma", gammaNode);
		mRoot.addNode("greek", greekNode);
		
		LLHTTPNode* hebrewNode = new LLHTTPNode;
		LLHTTPNode* alephNode = new LLHTTPNode;
		
		hebrewNode->addNode("aleph", alephNode);
		mRoot.addNode("hebrew", hebrewNode);

		ensureRootTraversal("greek/alpha", alphaNode, "");
		ensureRootTraversal("greek/beta", betaNode, "");
		ensureRootTraversal("greek/delta", NULL, "delta");
		ensureRootTraversal("greek/gamma", gammaNode, "");
		ensureRootTraversal("hebrew/aleph", alephNode, "");

		ensure_equals("root of greek", greekNode->rootNode(), &mRoot);
		ensure_equals("root of alpha", alphaNode->rootNode(), &mRoot);
		ensure_equals("root of beta", betaNode->rootNode(), &mRoot);
		ensure_equals("root of gamma", gammaNode->rootNode(), &mRoot);
		ensure_equals("root of hebrew", hebrewNode->rootNode(), &mRoot);
		ensure_equals("root of aleph", alephNode->rootNode(), &mRoot);
	}

	template<> template<>
	void HTTPNodeTestObject::test<4>()
	{
		// automatic creation of parent nodes and not overriding existing nodes
		
		LLHTTPNode* alphaNode = new LLHTTPNode;
		LLHTTPNode* betaNode = new LLHTTPNode;
		LLHTTPNode* gammaNode = new LLHTTPNode;
		LLHTTPNode* gamma2Node = new LLHTTPNode;
		
		mRoot.addNode("greek/alpha", alphaNode);
		mRoot.addNode("greek/beta", betaNode);
		
		mRoot.addNode("greek/gamma", gammaNode);
		mRoot.addNode("greek/gamma", gamma2Node);
		
		LLHTTPNode* alephNode = new LLHTTPNode;
		
		mRoot.addNode("hebrew/aleph", alephNode);

		ensureRootTraversal("greek/alpha", alphaNode, "");
		ensureRootTraversal("greek/beta", betaNode, "");
		ensureRootTraversal("greek/delta", NULL, "delta");
		ensureRootTraversal("greek/gamma", gammaNode, "");
		ensureRootTraversal("hebrew/aleph", alephNode, "");

		ensure_equals("root of alpha", alphaNode->rootNode(), &mRoot);
		ensure_equals("root of beta", betaNode->rootNode(), &mRoot);
		ensure_equals("root of gamma", gammaNode->rootNode(), &mRoot);
		ensure_equals("root of aleph", alephNode->rootNode(), &mRoot);
	}
	
	class IntegerNode : public LLHTTPNode
	{
	public:
		virtual void get(ResponsePtr response, const LLSD& context) const
		{
			int n = context["extra"]["value"];
			
			LLSD info;
			info["value"] = n;
			info["positive"] = n > 0;
			info["zero"] = n == 0;
			info["negative"] = n < 0;
		
			response->result(info);
		}
		
		virtual bool validate(const std::string& name, LLSD& context) const
		{
			int n;
			std::istringstream i_stream(name);
			i_stream >> n;

			if (i_stream.fail()  ||  i_stream.get() != EOF)
			{
				return false;
			}

			context["extra"]["value"] = n;
			return true;
		}
	};
	
	class SquareNode : public LLHTTPNode
	{
	public:
		virtual void get(ResponsePtr response, const LLSD& context) const
		{
			int n = context["extra"]["value"];		
			response->result(n*n);
		}
	};
	
	template<> template<>
	void HTTPNodeTestObject::test<5>()
	{
		// wildcard nodes
		
		LLHTTPNode* miscNode = new LLHTTPNode;
		LLHTTPNode* iNode = new IntegerNode;
		LLHTTPNode* sqNode = new SquareNode;
		
		mRoot.addNode("test/misc", miscNode);
		mRoot.addNode("test/<int>", iNode);
		mRoot.addNode("test/<int>/square", sqNode);
		
		ensureRootTraversal("test/42", iNode, "");
		ensure_equals("stored integer",
			context()["extra"]["value"].asInteger(), 42);
		
		ensureRootTraversal("test/bob", NULL, "bob");
		ensure("nothing stored",
			context()["extra"]["value"].isUndefined());
			
		ensureRootTraversal("test/3/square", sqNode, "");
		ResponsePtr response = Response::create();
		sqNode->get(LLHTTPNode::ResponsePtr(response), context());
		ensure_equals("square result", response->mResult.asInteger(), 9);
	}
	
	class AlphaNode : public LLHTTPNode
	{
	public:
		virtual bool handles(const LLSD& remainder, LLSD& context) const
		{
			LLSD::array_const_iterator i = remainder.beginArray();
			LLSD::array_const_iterator end = remainder.endArray();
			
			for (; i != end; ++i)
			{
				std::string s = i->asString();
				if (s.empty() || s[0] != 'a')
				{
					return false;
				}
			}
			
			return true;
		}
	};
	
	template<> template<>
	void HTTPNodeTestObject::test<6>()
	{
		// nodes that handle remainders
		
		LLHTTPNode* miscNode = new LLHTTPNode;
		LLHTTPNode* aNode = new AlphaNode;
		LLHTTPNode* zNode = new LLHTTPNode;
		
		mRoot.addNode("test/misc", miscNode);
		mRoot.addNode("test/alpha", aNode);
		mRoot.addNode("test/alpha/zebra", zNode);
		
		ensureRootTraversal("test/alpha", aNode, "");
		ensureRootTraversal("test/alpha/abe", aNode, "abe");
		ensureRootTraversal("test/alpha/abe/amy", aNode, "abe/amy");
		ensureRootTraversal("test/alpha/abe/bea", NULL, "abe/bea");
		ensureRootTraversal("test/alpha/bob", NULL, "bob");
		ensureRootTraversal("test/alpha/zebra", zNode, "");
	}
	
	template<> template<>
	void HTTPNodeTestObject::test<7>()
	{
		// test auto registration
		
		LLHTTPStandardServices::useServices();
		LLHTTPRegistrar::buildAllServices(mRoot);
		
		{
			LLSD result = get("web/hello");
			ensure_equals("hello result", result.asString(), "hello");
		}
		{
			LLSD stuff = 3.14159;
			LLSD result = post("web/echo", stuff);
			ensure_equals("echo result", result, stuff);
		}
	}

	template<> template<>
	void HTTPNodeTestObject::test<8>()
	{
		// test introspection
		
		LLHTTPRegistrar::buildAllServices(mRoot);
		
		mRoot.addNode("test/misc", new LLHTTPNode);
		mRoot.addNode("test/<int>", new IntegerNode);
		mRoot.addNode("test/<int>/square", new SquareNode);

		const LLSD result = get("web/server/api");
		
		ensure("result is array", result.isArray());
		ensure("result size", result.size() >= 2);
		
		ensureInArray(result, "web/echo");
		ensureInArray(result, "web/hello");
		ensureInArray(result, "test/misc");
		ensureInArray(result, "test/<int>");
		ensureInArray(result, "test/<int>/square");
	}
	
	template<> template<>
	void HTTPNodeTestObject::test<9>()
	{
		// test introspection details

		LLHTTPRegistrar::buildAllServices(mRoot);

		const LLSD helloDetails = get("web/server/api/web/hello");
		
		ensure_contains("hello description",
			helloDetails["description"].asString(), "hello");
		ensure_equals("method name", helloDetails["api"][0].asString(), std::string("GET"));
		ensureMemberString("hello", helloDetails, "output", "\"hello\"");
		ensure_contains("hello __file__",
			helloDetails["__file__"].asString(), "llsdhttpserver.cpp");
		ensure("hello line", helloDetails["__line__"].isInteger());


		const LLSD echoDetails = get("web/server/api/web/echo");
		
		ensure_contains("echo description",
			echoDetails["description"].asString(), "echo");
		ensure_equals("method name", echoDetails["api"][0].asString(), std::string("POST"));
		ensureMemberString("echo", echoDetails, "input", "<any>");
		ensureMemberString("echo", echoDetails, "output", "<the input>");
		ensure_contains("echo __file__",
			echoDetails["__file__"].asString(), "llsdhttpserver.cpp");
		ensure("echo", echoDetails["__line__"].isInteger());
	}
}
