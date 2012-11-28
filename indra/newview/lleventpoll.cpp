/**
 * @file lleventpoll.cpp
 * @brief Implementation of the LLEventPoll class.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "lleventpoll.h"
#include "llappviewer.h"
#include "llagent.h"

#include "llhttpclient.h"
#include "llhttpstatuscodes.h"
#include "llsdserialize.h"
#include "lleventtimer.h"
#include "llviewerregion.h"
#include "message.h"
#include "lltrans.h"

namespace
{
	// We will wait RETRY_SECONDS + (errorCount * RETRY_SECONDS_INC) before retrying after an error.
	// This means we attempt to recover relatively quickly but back off giving more time to recover
	// until we finally give up after MAX_EVENT_POLL_HTTP_ERRORS attempts.
	const F32 EVENT_POLL_ERROR_RETRY_SECONDS = 15.f; // ~ half of a normal timeout.
	const F32 EVENT_POLL_ERROR_RETRY_SECONDS_INC = 5.f; // ~ half of a normal timeout.
	const S32 MAX_EVENT_POLL_HTTP_ERRORS = 10; // ~5 minutes, by the above rules.

	class LLEventPollResponder : public LLHTTPClient::Responder
	{
	public:
		
		static LLHTTPClient::ResponderPtr start(const std::string& pollURL, const LLHost& sender);
		void stop();
		
		void makeRequest();

	private:
		LLEventPollResponder(const std::string&	pollURL, const LLHost& sender);
		~LLEventPollResponder();

		
		void handleMessage(const LLSD& content);
		virtual	void error(U32 status, const std::string& reason);
		virtual	void result(const LLSD&	content);

		virtual void completedRaw(U32 status,
									const std::string& reason,
									const LLChannelDescriptors& channels,
									const LLIOPipe::buffer_ptr_t& buffer);
	private:

		bool	mDone;

		std::string			mPollURL;
		std::string			mSender;
		
		LLSD	mAcknowledge;
		
		// these are only here for debugging so	we can see which poller	is which
		static int sCount;
		int	mCount;
		S32 mErrorCount;
	};

	class LLEventPollEventTimer : public LLEventTimer
	{
		typedef LLPointer<LLEventPollResponder> EventPollResponderPtr;

	public:
		LLEventPollEventTimer(F32 period, EventPollResponderPtr responder)
			: LLEventTimer(period), mResponder(responder)
		{ }

		virtual BOOL tick()
		{
			mResponder->makeRequest();
			return TRUE;	// Causes this instance to be deleted.
		}

	private:
		
		EventPollResponderPtr mResponder;
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
		  mCount(++sCount),
		  mErrorCount(0)
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

	// virtual 
	void LLEventPollResponder::completedRaw(U32 status,
									const std::string& reason,
									const LLChannelDescriptors& channels,
									const LLIOPipe::buffer_ptr_t& buffer)
	{
		if (status == HTTP_BAD_GATEWAY)
		{
			// These errors are not parsable as LLSD, 
			// which LLHTTPClient::Responder::completedRaw will try to do.
			completed(status, reason, LLSD());
		}
		else
		{
			LLHTTPClient::Responder::completedRaw(status,reason,channels,buffer);
		}
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

		// A HTTP_BAD_GATEWAY (502) error is our standard timeout response
		// we get this when there are no events.
		if ( status == HTTP_BAD_GATEWAY )	
		{
			mErrorCount = 0;
			makeRequest();
		}
		else if (mErrorCount < MAX_EVENT_POLL_HTTP_ERRORS)
		{
			++mErrorCount;
			
			// The 'tick' will return TRUE causing the timer to delete this.
			new LLEventPollEventTimer(EVENT_POLL_ERROR_RETRY_SECONDS
										+ mErrorCount * EVENT_POLL_ERROR_RETRY_SECONDS_INC
									, this);

			llwarns << "Unexpected HTTP error.  status: " << status << ", reason: " << reason << llendl;
		}
		else
		{
			llwarns <<	"LLEventPollResponder::error: <" << mCount << "> got "
					<<	status << ": " << reason
					<<	(mDone ? " -- done"	: "") << llendl;
			stop();

			// At this point we have given up and the viewer will not receive HTTP messages from the simulator.
			// IMs, teleports, about land, selecing land, region crossing and more will all fail.
			// They are essentially disconnected from the region even though some things may still work.
			// Since things won't get better until they relog we force a disconnect now.

			// *NOTE:Mani - The following condition check to see if this failing event poll
			// is attached to the Agent's main region. If so we disconnect the viewer.
			// Else... its a child region and we just leave the dead event poll stopped and 
			// continue running.
			if(gAgent.getRegion() && gAgent.getRegion()->getHost().getIPandPort() == mSender)
			{
				llwarns << "Forcing disconnect due to stalled main region event poll."  << llendl;
				LLAppViewer::instance()->forceDisconnect(LLTrans::getString("AgentLostConnection"));
			}
		}
	}

	//virtual
	void LLEventPollResponder::result(const LLSD& content)
	{
		lldebugs <<	"LLEventPollResponder::result <" << mCount	<< ">"
				 <<	(mDone ? " -- done"	: "") << llendl;
		
		if (mDone) return;

		mErrorCount = 0;

		if (!content.get("events") ||
			!content.get("id"))
		{
			llwarns << "received event poll with no events or id key" << llendl;
			makeRequest();
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

LLEventPoll::LLEventPoll(const std::string&	poll_url, const LLHost& sender)
	: mImpl(LLEventPollResponder::start(poll_url, sender))
	{ }

LLEventPoll::~LLEventPoll()
{
	LLHTTPClient::Responder* responderp = mImpl.get();
	LLEventPollResponder* event_poll_responder = dynamic_cast<LLEventPollResponder*>(responderp);
	if (event_poll_responder) event_poll_responder->stop();
}
