/**
 * @file lleventpoll.cpp
 * @brief Implementation of the LLEventPoll class.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "lleventpoll.h"

#include "llhttpclient.h"
#include "llsdserialize.h"
#include "llviewerregion.h"
#include "message.h"

class LLEventPoll::Impl	: LLHTTPClient::Responder
{
public:
	static Impl& start(const std::string& pollURL);
	void stop();
	
private:
	Impl(const std::string&	pollURL);
	~Impl();

	void makeRequest();
	void handleMessage(const LLSD& content);
	virtual	void error(U32 status, const std::string& reason);
	virtual	void result(const LLSD&	content);

private:
	typedef	LLHTTPClient::ResponderPtr Ptr;

	Ptr		mPtr;
	bool	mDone;

	std::string			mPollURL;
	std::string			mSender;
	
	LLSD	mAcknowledge;
	
	// these are only here for debugging so	we can see which poller	is which
	static int sCount;
	int	mCount;
};

//static
LLEventPoll::Impl& LLEventPoll::Impl::start(
	const std::string& pollURL)
{
	Impl* i	= new Impl(pollURL);
	llinfos	<< "LLEventPoll::Impl::start <"	<< i->mCount <<	"> "
			<< pollURL << llendl;
	return *i;
}

void LLEventPoll::Impl::stop()
{
	lldebugs	<< "LLEventPoll::Impl::stop	<" << mCount <<	"> "
			<< mPollURL	<< llendl;
	// there should	be a way to	stop a LLHTTPClient	request	in progress
	mDone =	true;
	mPtr = NULL;
}

int	LLEventPoll::Impl::sCount =	0;

LLEventPoll::Impl::Impl(const std::string& pollURL)
	: mPtr(NULL), mDone(false),
	  mPollURL(pollURL),
	  mCount(++sCount)
{
	mPtr = this;
	//extract host and port of simulator to set as sender
	LLViewerRegion *regionp = gAgent.getRegion();
	if (!regionp)
	{
		llerrs << "LLEventPoll initialized before region is added." << llendl;
	}
	mSender = regionp->getHost().getIPandPort();
	llinfos << "LLEventPoll initialized with sender " << mSender << llendl;
	makeRequest();
}

LLEventPoll::Impl::~Impl()
{
	lldebugs <<	"LLEventPoll::Impl::~Impl <" <<	mCount << "> "
			 <<	mPollURL <<	llendl;
}

void LLEventPoll::Impl::makeRequest()
{
	LLSD request;
	request["ack"] = mAcknowledge;
	request["done"]	= mDone;
	
	lldebugs <<	"LLEventPoll::Impl::makeRequest	<" << mCount <<	"> ack = "
			 <<	LLSDXMLStreamer(mAcknowledge) << llendl;
	LLHTTPClient::post(mPollURL, request, mPtr);
}

void LLEventPoll::Impl::handleMessage(const	LLSD& content)
{
	std::string	msg_name	= content["message"];
	LLSD message;
	message["sender"] = mSender;
	message["body"] = content["body"];
	LLMessageSystem::dispatch(msg_name, message);
}

//virtual
void LLEventPoll::Impl::error(U32 status, const	std::string& reason)
{
	if (mDone) return;

	if(status != 499)
	{
		llwarns <<	"LLEventPoll::Impl::error: <" << mCount << "> got "
				<<	status << ": " << reason
				<<	(mDone ? " -- done"	: "") << llendl;
		stop();
		return;
	}

	makeRequest();
}

//virtual
void LLEventPoll::Impl::result(const LLSD& content)
{
	lldebugs <<	"LLEventPoll::Impl::result <" << mCount	<< ">"
			 <<	(mDone ? " -- done"	: "") << llendl;
	
	if (mDone) return;
	
	mAcknowledge = content["id"];
	LLSD events	= content["events"];

	if(mAcknowledge.isUndefined())
	{
		llwarns << "LLEventPoll::Impl: id undefined" << llendl;
	}
	
	llinfos  << "LLEventPoll::Impl::completed <" <<	mCount << "> " << events.size() << "events (id "
			 <<	LLSDXMLStreamer(mAcknowledge) << ")" << llendl;
	
	LLSD::array_const_iterator i = events.beginArray();
	LLSD::array_const_iterator end = events.endArray();
	for	(; i !=	end; ++i)
	{
		if (i->has("message"))
		{
			handleMessage(*i);
		}
	}
	
	makeRequest();
}

LLEventPoll::LLEventPoll(const std::string&	pollURL)
	: impl(Impl::start(pollURL))
	{ }

LLEventPoll::~LLEventPoll()
{
	impl.stop();
}
