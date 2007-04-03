/** 
 * @file lleventpoll.cpp
 * @brief Implementation of the LLEventPoll class.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lleventpoll.h"

#include "llhttpclient.h"
#include "llhttpnode.h"
#include "llsdserialize.h"



class LLEventPoll::Impl : LLHTTPClient::Responder
{
public:
	static Impl& start(
		const std::string& pollURL, const LLHTTPNode& treeRoot)
	{
		Impl* i = new Impl(pollURL, treeRoot);
		llinfos << "LLEventPoll::Impl::start <" << i->mCount << "> "
			<< pollURL << llendl;
		return *i;
	}
	
	void stop()
	{
		lldebugs << "LLEventPoll::Impl::stop <" << mCount << "> "
				 << mPollURL << llendl;
		// there should be a way to stop a LLHTTPClient request in progress
		mDone = true;
		mPtr = NULL;
	}
	
private:
	Impl(const std::string& pollURL, const LLHTTPNode& treeRoot)
		: mPtr(NULL), mDone(false),
		  mPollURL(pollURL), mTreeRoot(treeRoot),
		  mCount(++sCount)
	{
		mPtr = this;
		makeRequest();
	}
		
	~Impl()
	{
		lldebugs << "LLEventPoll::Impl::~Impl <" << mCount << "> "
				 << mPollURL << llendl;
	}


	void makeRequest()
	{
		LLSD request;
		request["ack"] = mAcknowledge;
		request["done"] = mDone;
		
		lldebugs << "LLEventPoll::Impl::makeRequest <" << mCount << "> ack = "
			<< LLSDXMLStreamer(mAcknowledge) << llendl;
		LLHTTPClient::post(mPollURL, request, mPtr);
	}
	
	void handleMessage(const LLSD& content)
	{
		std::string message = content["message"];
		if (message.empty())
		{
			llwarns << "LLEventPoll::Impl::handleMessage <" << mCount
					<< "> empty message name" << llendl;
			return;
		}
		
		std::string path = "/message/" + message;
		
		LLSD context;
		const LLHTTPNode* handler = mTreeRoot.traverse(path, context);
		if (!handler)
		{
			llwarns << "LLEventPoll::Impl::handleMessage <" << mCount
					<< "> no handler for " << path << llendl;
			return;
		}
		LLPointer<LLSimpleResponse> responsep = LLSimpleResponse::create();
		handler->post((LLHTTPNode::ResponsePtr)responsep, context, content["body"]);
		
		lldebugs << "LLEventPoll::Impl::handleMessage handled <" << mCount << "> "
			<< message << ": " << *responsep << llendl;
	}

	virtual void error(U32 status, const std::string& reason)
	{
		lldebugs << "LLEventPoll::Impl::error <" << mCount << "> got "
			<< status << ": " << reason
			<< (mDone ? " -- done" : "") << llendl;

		if (mDone) return;
		
		if (status == 404)
		{
			// the capability has been revoked
			stop();
			return;
		}
		
		makeRequest();
	}
	
	
	virtual void result(const LLSD& content)
	{
		lldebugs << "LLEventPoll::Impl::result <" << mCount << ">"
			<< (mDone ? " -- done" : "") << llendl;

		if (mDone) return;

		mAcknowledge = content["id"];
		LLSD events = content["events"];
		
		lldebugs << "LLEventPoll::Impl::completed <" << mCount << "> ack =  "
			<< LLSDXMLStreamer(mAcknowledge) << llendl;

		LLSD::array_const_iterator i = events.beginArray();
		LLSD::array_const_iterator end = events.endArray();
		for (; i != end; ++i)
		{
			if (i->has("message"))
			{
				handleMessage(*i);
			}
		}
		
		makeRequest();
	}

private:
	typedef LLHTTPClient::ResponderPtr Ptr;

	Ptr		mPtr;
	bool	mDone;

	std::string			mPollURL;
	const LLHTTPNode&	mTreeRoot;
	
	LLSD	mAcknowledge;
	
	// these are only here for debugging so we can see which poller is which
	static int sCount;
	int mCount;
};

int LLEventPoll::Impl::sCount = 0;


LLEventPoll::LLEventPoll(const std::string& pollURL, const LLHTTPNode& treeRoot)
	: impl(Impl::start(pollURL, treeRoot))
	{ }

LLEventPoll::~LLEventPoll()
{
	impl.stop();
}
