/**
 * @file lleventpoll.cpp
 * @brief Implementation of the LLEventPoll class.
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

#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "lleventpoll.h"

#include "llhttpclient.h"
#include "llsdserialize.h"
#include "llviewerregion.h"
#include "message.h"

namespace
{
	class LLEventPollResponder : public LLHTTPClient::Responder
	{
	public:
		
		static LLHTTPClient::ResponderPtr start(const std::string& pollURL, const LLHost& sender);
		void stop();
		
	private:
		LLEventPollResponder(const std::string&	pollURL, const LLHost& sender);
		~LLEventPollResponder();

		void makeRequest();
		void handleMessage(const LLSD& content);
		virtual	void error(U32 status, const std::string& reason);
		virtual	void result(const LLSD&	content);

	private:

		bool	mDone;

		std::string			mPollURL;
		std::string			mSender;
		
		LLSD	mAcknowledge;
		
		// these are only here for debugging so	we can see which poller	is which
		static int sCount;
		int	mCount;
	};

	//static
	LLHTTPClient::ResponderPtr LLEventPollResponder::start(
		const std::string& pollURL, const LLHost& sender)
	{
		LLHTTPClient::ResponderPtr result = new LLEventPollResponder(pollURL, sender);
		llinfos	<< "LLEventPollResponder::start <" << sCount << "> "
				<< pollURL << llendl;
		return result;
	}

	void LLEventPollResponder::stop()
	{
		llinfos	<< "LLEventPollResponder::stop	<" << mCount <<	"> "
				<< mPollURL	<< llendl;
		// there should	be a way to	stop a LLHTTPClient	request	in progress
		mDone =	true;
	}

	int	LLEventPollResponder::sCount =	0;

	LLEventPollResponder::LLEventPollResponder(const std::string& pollURL, const LLHost& sender)
		: mDone(false),
		  mPollURL(pollURL),
		  mCount(++sCount)
	{
		//extract host and port of simulator to set as sender
		LLViewerRegion *regionp = gAgent.getRegion();
		if (!regionp)
		{
			llerrs << "LLEventPoll initialized before region is added." << llendl;
		}
		mSender = sender.getIPandPort();
		llinfos << "LLEventPoll initialized with sender " << mSender << llendl;
		makeRequest();
	}

	LLEventPollResponder::~LLEventPollResponder()
	{
		stop();
		lldebugs <<	"LLEventPollResponder::~Impl <" <<	mCount << "> "
				 <<	mPollURL <<	llendl;
	}

	void LLEventPollResponder::makeRequest()
	{
		LLSD request;
		request["ack"] = mAcknowledge;
		request["done"]	= mDone;
		
		lldebugs <<	"LLEventPollResponder::makeRequest	<" << mCount <<	"> ack = "
				 <<	LLSDXMLStreamer(mAcknowledge) << llendl;
		LLHTTPClient::post(mPollURL, request, this);
	}

	void LLEventPollResponder::handleMessage(const	LLSD& content)
	{
		std::string	msg_name	= content["message"];
		LLSD message;
		message["sender"] = mSender;
		message["body"] = content["body"];
		LLMessageSystem::dispatch(msg_name, message);
	}

	//virtual
	void LLEventPollResponder::error(U32 status, const	std::string& reason)
	{
		if (mDone) return;

		if(status != 499)
		{
			llwarns <<	"LLEventPollResponder::error: <" << mCount << "> got "
					<<	status << ": " << reason
					<<	(mDone ? " -- done"	: "") << llendl;
			stop();
			return;
		}

		makeRequest();
	}

	//virtual
	void LLEventPollResponder::result(const LLSD& content)
	{
		lldebugs <<	"LLEventPollResponder::result <" << mCount	<< ">"
				 <<	(mDone ? " -- done"	: "") << llendl;
		
		if (mDone) return;

		if (!content.get("events") ||
			!content.get("id"))
		{
			llwarns << "received event poll with no events or id key" << llendl;
			return;
		}
		
		mAcknowledge = content["id"];
		LLSD events	= content["events"];

		if(mAcknowledge.isUndefined())
		{
			llwarns << "LLEventPollResponder: id undefined" << llendl;
		}
		
		// was llinfos but now that CoarseRegionUpdate is TCP @ 1/second, it'd be too verbose for viewer logs. -MG
		lldebugs  << "LLEventPollResponder::completed <" <<	mCount << "> " << events.size() << "events (id "
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
}

LLEventPoll::LLEventPoll(const std::string&	pollURL, const LLHost& sender)
	: mImpl(LLEventPollResponder::start(pollURL, sender))
	{ }

LLEventPoll::~LLEventPoll()
{
}
