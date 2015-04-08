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
#include "llhttpconstants.h"
#include "llsdserialize.h"
#include "lleventtimer.h"
#include "llviewerregion.h"
#include "message.h"
#include "lltrans.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "llcorehttputil.h"

namespace LLEventPolling
{
namespace Details
{
        // We will wait RETRY_SECONDS + (errorCount * RETRY_SECONDS_INC) before retrying after an error.
    // This means we attempt to recover relatively quickly but back off giving more time to recover
    // until we finally give up after MAX_EVENT_POLL_HTTP_ERRORS attempts.
    const F32 EVENT_POLL_ERROR_RETRY_SECONDS = 15.f; // ~ half of a normal timeout.
    const F32 EVENT_POLL_ERROR_RETRY_SECONDS_INC = 5.f; // ~ half of a normal timeout.
    const S32 MAX_EVENT_POLL_HTTP_ERRORS = 10; // ~5 minutes, by the above rules.

#if 1

    class LLEventPollImpl
    {
    public:
        LLEventPollImpl(const LLHost &sender);

        void start(const std::string &url);
        void stop();
    private:
        void eventPollCoro(LLCoros::self& self, std::string url);

        void                            handleMessage(const LLSD &content);

        bool                            mDone;
        LLCore::HttpRequest::ptr_t      mHttpRequest;
        LLCore::HttpRequest::policy_t   mHttpPolicy;
        std::string                     mSenderIp;
        int                             mCounter;
        LLCoreHttpUtil::HttpCoroutineAdapter::wptr_t mAdapter;

        static int                      sNextCounter;
    };


    int LLEventPollImpl::sNextCounter = 1;


    LLEventPollImpl::LLEventPollImpl(const LLHost &sender) :
        mDone(false),
        mHttpRequest(),
        mHttpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID),
        mSenderIp(),
        mCounter(sNextCounter++)

    {
        mHttpRequest = LLCore::HttpRequest::ptr_t(new LLCore::HttpRequest);
        mSenderIp = sender.getIPandPort();
    }

    void LLEventPollImpl::handleMessage(const LLSD& content)
    {
        std::string	msg_name = content["message"];
        LLSD message;
        message["sender"] = mSenderIp;
        message["body"] = content["body"];
        LLMessageSystem::dispatch(msg_name, message);
    }

    void LLEventPollImpl::start(const std::string &url)
    {
        if (!url.empty())
        {
            std::string coroname =
                LLCoros::instance().launch("LLAccountingCostManager::accountingCostCoro",
                boost::bind(&LLEventPollImpl::eventPollCoro, this, _1, url));
            LL_INFOS() << coroname << " with  url '" << url << LL_ENDL;
        }
    }

    void LLEventPollImpl::stop()
    {
        LL_INFOS() << "requesting stop for event poll coroutine <" << mCounter << ">" << LL_ENDL;
        mDone = true;

        LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter = mAdapter.lock();
        if (adapter)
        {
            // cancel the yielding operation if any.
            adapter->cancelYieldingOperation();
        }
    }

    void LLEventPollImpl::eventPollCoro(LLCoros::self& self, std::string url)
    {
        LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("EventPoller", mHttpPolicy));
        LLSD acknowledge;
        int errorCount = 0;
        int counter = mCounter; // saved on the stack for debugging.

        LL_INFOS("LLEventPollImpl::eventPollCoro") << " <" << counter << "> entering coroutine." << LL_ENDL;

        mAdapter = httpAdapter;
        while (!mDone)
        {
            LLSD request;
            request["ack"] = acknowledge;
            request["done"] = mDone;

//            LL_DEBUGS("LLEventPollImpl::eventPollCoro") << "<" << counter << "> request = "
//                << LLSDXMLStreamer(request) << LL_ENDL;

            LL_DEBUGS("LLEventPollImpl::eventPollCoro") << " <" << counter << "> posting and yielding." << LL_ENDL;
            LLSD result = httpAdapter->postAndYield(self, mHttpRequest, url, request);

//            LL_DEBUGS("LLEventPollImpl::eventPollCoro") << "<" << counter << "> result = "
//                << LLSDXMLStreamer(result) << LL_ENDL;

            LLSD httpResults;
            httpResults = result["http_result"];
            LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroHandler::getStatusFromLLSD(httpResults);

            if (!status)
            {

                if (status == LLCore::HttpStatus(HTTP_BAD_GATEWAY))
                {
                    // A HTTP_BAD_GATEWAY (502) error is our standard timeout response
                    // we get this when there are no events.
                    errorCount = 0;
                    continue;
                }
                    
                if ((status == LLCore::HttpStatus(LLCore::HttpStatus::LLCORE, LLCore::HE_OP_CANCELED)) || 
                        (status == LLCore::HttpStatus(HTTP_NOT_FOUND)))
                {   
                    LL_WARNS() << "Canceling coroutine" << LL_ENDL;
                    break;
                }
                LL_WARNS("LLEventPollImpl::eventPollCoro") << "<" << counter << "> Error result from LLCoreHttpUtil::HttpCoroHandler. Code "
                    << status.toTerseString() << ": '" << httpResults["message"] << "'" << LL_ENDL;

                if (errorCount < MAX_EVENT_POLL_HTTP_ERRORS)
                {
                    ++errorCount;

                    // The 'tick' will return TRUE causing the timer to delete this.
                    int waitToRetry = EVENT_POLL_ERROR_RETRY_SECONDS
                        + errorCount * EVENT_POLL_ERROR_RETRY_SECONDS_INC;

                    LL_WARNS() << "Retrying in " << waitToRetry <<
                        " seconds, error count is now " << errorCount << LL_ENDL;

                    // *TODO: Wait for a timeout here
                    //                     new LLEventPollEventTimer(
                    //                         , this);
                    continue;
                }
                else
                {
                    // At this point we have given up and the viewer will not receive HTTP messages from the simulator.
                    // IMs, teleports, about land, selecting land, region crossing and more will all fail.
                    // They are essentially disconnected from the region even though some things may still work.
                    // Since things won't get better until they relog we force a disconnect now.

                    mDone = true;

                    // *NOTE:Mani - The following condition check to see if this failing event poll
                    // is attached to the Agent's main region. If so we disconnect the viewer.
                    // Else... its a child region and we just leave the dead event poll stopped and 
                    // continue running.
                    if (gAgent.getRegion() && gAgent.getRegion()->getHost().getIPandPort() == mSenderIp)
                    {
                        LL_WARNS("LLEventPollImpl::eventPollCoro") << "< " << counter << "> Forcing disconnect due to stalled main region event poll." << LL_ENDL;
                        LLAppViewer::instance()->forceDisconnect(LLTrans::getString("AgentLostConnection"));
                    }
                    break;
                }
            }

            LL_DEBUGS("LLEventPollImpl::eventPollCoro") << " <" << counter << ">"
                << (mDone ? " -- done" : "") << LL_ENDL;

            errorCount = 0;

            if (!result.isMap() ||
                !result.get("events") ||
                !result.get("id"))
            {
                LL_WARNS("LLEventPollImpl::eventPollCoro") << " <" << counter << "> received event poll with no events or id key: " << LLSDXMLStreamer(result) << LL_ENDL;
                continue;
            }

            acknowledge = result["id"];
            LLSD events = result["events"];

            if (acknowledge.isUndefined())
            {
                LL_WARNS("LLEventPollImpl::eventPollCoro") << " id undefined" << LL_ENDL;
            }

            // was LL_INFOS() but now that CoarseRegionUpdate is TCP @ 1/second, it'd be too verbose for viewer logs. -MG
            LL_DEBUGS() << "LLEventPollResponder::httpSuccess <" << counter << "> " << events.size() << "events (id "
                << LLSDXMLStreamer(acknowledge) << ")" << LL_ENDL;

            LLSD::array_const_iterator i = events.beginArray();
            LLSD::array_const_iterator end = events.endArray();
            for (; i != end; ++i)
            {
                if (i->has("message"))
                {
                    handleMessage(*i);
                }
            }
        }
        LL_INFOS("LLEventPollImpl::eventPollCoro") << " <" << counter << "> Leaving coroutine." << LL_ENDL;

    }

#else
    class LLEventPollResponder : public LLHTTPClient::Responder
    {
        LOG_CLASS(LLEventPollResponder);
    public:

        static LLHTTPClient::ResponderPtr start(const std::string& pollURL, const LLHost& sender);
        void stop();

        void makeRequest();

        /* virtual */ void completedRaw(const LLChannelDescriptors& channels,
            const LLIOPipe::buffer_ptr_t& buffer);

    private:
        LLEventPollResponder(const std::string&	pollURL, const LLHost& sender);
        ~LLEventPollResponder();


        void handleMessage(const LLSD& content);

        /* virtual */ void httpFailure();
        /* virtual */ void httpSuccess();

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
        LL_INFOS()	<< "LLEventPollResponder::start <" << sCount << "> "
            << pollURL << LL_ENDL;
        return result;
    }

    void LLEventPollResponder::stop()
    {
        LL_INFOS()	<< "LLEventPollResponder::stop	<" << mCount <<	"> "
            << mPollURL	<< LL_ENDL;
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
            LL_ERRS() << "LLEventPoll initialized before region is added." << LL_ENDL;
        }
        mSender = sender.getIPandPort();
        LL_INFOS() << "LLEventPoll initialized with sender " << mSender << LL_ENDL;
        makeRequest();
    }

    LLEventPollResponder::~LLEventPollResponder()
    {
        stop();
        LL_DEBUGS() <<	"LLEventPollResponder::~Impl <" <<	mCount << "> "
            <<	mPollURL <<	LL_ENDL;
    }

    // virtual 
    void LLEventPollResponder::completedRaw(const LLChannelDescriptors& channels,
        const LLIOPipe::buffer_ptr_t& buffer)
    {
        if (getStatus() == HTTP_BAD_GATEWAY)
        {
            // These errors are not parsable as LLSD, 
            // which LLHTTPClient::Responder::completedRaw will try to do.
            httpCompleted();
        }
        else
        {
            LLHTTPClient::Responder::completedRaw(channels,buffer);
        }
    }

    void LLEventPollResponder::makeRequest()
    {
        LLSD request;
        request["ack"] = mAcknowledge;
        request["done"]	= mDone;

        LL_DEBUGS() <<	"LLEventPollResponder::makeRequest	<" << mCount <<	"> ack = "
            <<	LLSDXMLStreamer(mAcknowledge) << LL_ENDL;
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
    void LLEventPollResponder::httpFailure()
    {
        if (mDone) return;

        // A HTTP_BAD_GATEWAY (502) error is our standard timeout response
        // we get this when there are no events.
        if ( getStatus() == HTTP_BAD_GATEWAY )
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

            LL_WARNS() << dumpResponse() << LL_ENDL;
        }
        else
        {
            LL_WARNS() << dumpResponse()
                << " [count:" << mCount << "] "
                << (mDone ? " -- done" : "") << LL_ENDL;
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
                LL_WARNS() << "Forcing disconnect due to stalled main region event poll."  << LL_ENDL;
                LLAppViewer::instance()->forceDisconnect(LLTrans::getString("AgentLostConnection"));
            }
        }
    }

    //virtual
    void LLEventPollResponder::httpSuccess()
    {
        LL_DEBUGS() <<	"LLEventPollResponder::result <" << mCount	<< ">"
            <<	(mDone ? " -- done"	: "") << LL_ENDL;

        if (mDone) return;

        mErrorCount = 0;

        const LLSD& content = getContent();
        if (!content.isMap() ||
            !content.get("events") ||
            !content.get("id"))
        {
            LL_WARNS() << "received event poll with no events or id key: " << dumpResponse() << LL_ENDL;
            makeRequest();
            return;
        }

        mAcknowledge = content["id"];
        LLSD events	= content["events"];

        if(mAcknowledge.isUndefined())
        {
            LL_WARNS() << "LLEventPollResponder: id undefined" << LL_ENDL;
        }

        // was LL_INFOS() but now that CoarseRegionUpdate is TCP @ 1/second, it'd be too verbose for viewer logs. -MG
        LL_DEBUGS()  << "LLEventPollResponder::httpSuccess <" <<	mCount << "> " << events.size() << "events (id "
            <<	LLSDXMLStreamer(mAcknowledge) << ")" << LL_ENDL;

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
#endif
}
}

LLEventPoll::LLEventPoll(const std::string&	poll_url, const LLHost& sender):
    mImpl()
{ 
    mImpl = boost::unique_ptr<LLEventPolling::Details::LLEventPollImpl>
            (new LLEventPolling::Details::LLEventPollImpl(sender));
    mImpl->start(poll_url);
}

LLEventPoll::~LLEventPoll()
{
    mImpl->stop();

}
