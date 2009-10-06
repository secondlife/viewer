/** 
 * @file llmediadataclient.cpp
 * @brief class for queueing up requests for media data
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.	Terms of
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

#include "llviewerprecompiledheaders.h"

#include "llmediadataclient.h"

#include <boost/lexical_cast.hpp>

#include "llhttpstatuscodes.h"
#include "llsdutil.h"
#include "llmediaentry.h"
#include "lltextureentry.h"
#include "llviewerregion.h"
#include "llvovolume.h"

//
// When making a request
// - obtain the "overall interest score" of the object.	 
//	 This would be the sum of the impls' interest scores.
// - put the request onto a queue sorted by this score 
//	 (highest score at the front of the queue)
// - On a timer, once a second, pull off the head of the queue and send 
//	 the request.
// - Any request that gets a 503 still goes through the retry logic
//

// Some helpful logging macros

//////////////////////////////////////////////////////////////////////////////////////
//
// LLMediaDataClient::Request
//
//////////////////////////////////////////////////////////////////////////////////////
/*static*/U32 LLMediaDataClient::Request::sNum = 0;

LLMediaDataClient::Request::Request(const std::string &cap_name, 
									const LLSD& sd_payload,
									LLVOVolume *obj, 
									LLMediaDataClient *mdc)
	: mCapName(cap_name), 
	  mPayload(sd_payload), 
	  mObject(obj),
	  mNum(++sNum), 
	  mRetryCount(0),
	  mMDC(mdc)
{
}

LLMediaDataClient::Request::~Request()
{
	mMDC = NULL;
    mObject = NULL;
}

													  
std::string LLMediaDataClient::Request::getCapability() const
{
	return getObject()->getRegion()->getCapability(getCapName());
}

// Helper function to get the "type" of request, which just pokes around to
// discover it.
LLMediaDataClient::Request::Type LLMediaDataClient::Request::getType() const
{
    if (mCapName == "ObjectMediaNavigate")
    {
        return NAVIGATE;
    }
    else if (mCapName == "ObjectMedia")
    {
        const std::string &verb = mPayload["verb"];
        if (verb == "GET")
        {
            return GET;
        }
        else if (verb == "UPDATE")
        {
            return UPDATE;
        }
    }
    llassert(false);
    return GET;
}

const char *LLMediaDataClient::Request::getTypeAsString() const
{
	Type t = getType();
	switch (t)
	{
		case GET:
			return "GET";
			break;
		case UPDATE:
			return "UPDATE";
			break;
		case NAVIGATE:
			return "NAVIGATE";
			break;
	}
	return "";
}
	

void LLMediaDataClient::Request::reEnqueue() const
{
	// I sure hope this doesn't deref a bad pointer:
	mMDC->enqueue(this);
}

std::ostream& operator<<(std::ostream &s, const LLMediaDataClient::Request &r)
{
	s << "<request>" 
	  << "<num>" << r.getNum() << "</num>"
	  << "<type>" << r.getTypeAsString() << "</type>"
	  << "<object_id>" << r.getObject()->getID() << "</object_id>"
	  << "<num_retries>" << r.getRetryCount() << "</num_retries>"
	  << "</request> ";
	return s;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// LLMediaDataClient::Responder::RetryTimer
//
//////////////////////////////////////////////////////////////////////////////////////

LLMediaDataClient::Responder::RetryTimer::RetryTimer(F32 time, Responder *mdr)
	: LLEventTimer(time), mResponder(mdr)
{
}

// virtual 
LLMediaDataClient::Responder::RetryTimer::~RetryTimer() 
{
	mResponder = NULL;
}

// virtual
BOOL LLMediaDataClient::Responder::RetryTimer::tick()
{
	// Instead of retrying, we just put the request back onto the queue
	LL_INFOS("LLMediaDataClient") << "RetryTimer fired for: " << *(mResponder->getRequest()) << "retrying" << LL_ENDL;
	mResponder->getRequest()->reEnqueue();
	// Don't fire again
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// LLMediaDataClient::Responder
//
//////////////////////////////////////////////////////////////////////////////////////

LLMediaDataClient::Responder::Responder(const request_ptr_t &request)
	: mRequest(request)
{
}

LLMediaDataClient::Responder::~Responder()
{
    mRequest = NULL;
}

/*virtual*/
void LLMediaDataClient::Responder::error(U32 status, const std::string& reason)
{
	extern LLControlGroup gSavedSettings;

	if (status == HTTP_SERVICE_UNAVAILABLE)
	{
		F32 retry_timeout = gSavedSettings.getF32("PrimMediaRetryTimerDelay");
		if (retry_timeout <= 0.0)
		{
			retry_timeout = (F32)UNAVAILABLE_RETRY_TIMER_DELAY;
		}
		LL_INFOS("LLMediaDataClient") << *mRequest << "got SERVICE_UNAVAILABLE...retrying in " << retry_timeout << " seconds" << LL_ENDL;

		mRequest->incRetryCount();
		
		// Start timer (instances are automagically tracked by
		// InstanceTracker<> and LLEventTimer)
		new RetryTimer(F32(retry_timeout/*secs*/), this);
	}
	else {
		std::string msg = boost::lexical_cast<std::string>(status) + ": " + reason;
		LL_WARNS("LLMediaDataClient") << *mRequest << " http error(" << msg << ")" << LL_ENDL;
	}
}


/*virtual*/
void LLMediaDataClient::Responder::result(const LLSD& content)
{
	LL_INFOS("LLMediaDataClient") << *mRequest << "result : " << ll_pretty_print_sd(content) << LL_ENDL;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// LLMediaDataClient::Comparator
//
//////////////////////////////////////////////////////////////////////////////////////

bool LLMediaDataClient::Comparator::operator() (const request_ptr_t &o1, const request_ptr_t &o2) const
{
	if (o2.isNull()) return true;
	if (o1.isNull()) return false;

	// The score is intended to be a measure of how close an object is or
	// how much screen real estate (interest) it takes up
	// Further away = lower score.
	// Lesser interest = lower score
	// For instance, here are some cases:
	// 1: Two items with no impl, closest one wins
	// 2: Two items with an impl: interest should rule, but distance is
	// still taken into account (i.e. something really close might take
	// precedence over a large item far away)
	// 3: One item with an impl, another without: item with impl wins 
	//	  (XXX is that what we want?)		 
	// Calculate the scores for each.  
	F64 o1_score = Comparator::getObjectScore(o1->getObject());
	F64 o2_score = Comparator::getObjectScore(o2->getObject());
		
	return ( o1_score > o2_score );
}
	
// static
F64 LLMediaDataClient::Comparator::getObjectScore(const ll_vo_volume_ptr_t &obj)
{
	// *TODO: make this less expensive?
	F32 dist = obj->getRenderPosition().length() + 0.1;	 // avoids div by 0
	// square the distance so that they are in the same "unit magnitude" as
	// the interest (which is an area) 
	dist *= dist;
	F64 interest = (F64)1;
	int i = 0;
	int end = obj->getNumTEs();
	for ( ; i < end; ++i)
	{
		const viewer_media_t &impl = obj->getMediaImpl(i);
		if (!impl.isNull())
		{
			interest += impl->getInterest();
		}
	}
		
	return interest/dist;	   
}

//////////////////////////////////////////////////////////////////////////////////////
//
// LLMediaDataClient::PriorityQueue
// Queue of LLVOVolume smart pointers to request media for.
//
//////////////////////////////////////////////////////////////////////////////////////

// dump the queue
std::ostream& operator<<(std::ostream &s, const LLMediaDataClient::PriorityQueue &q)
{
	int i = 0;
	std::vector<LLMediaDataClient::request_ptr_t>::const_iterator iter = q.c.begin();
	std::vector<LLMediaDataClient::request_ptr_t>::const_iterator end = q.c.end();
	while (iter < end)
	{
		s << "\t" << i << "]: " << (*iter)->getObject()->getID().asString();
		iter++;
		i++;
	}
	return s;
}

//////////////////////////////////////////////////////////////////////////////////////
//
// LLMediaDataClient::QueueTimer
// Queue of LLVOVolume smart pointers to request media for.
//
//////////////////////////////////////////////////////////////////////////////////////

LLMediaDataClient::QueueTimer::QueueTimer(F32 time, LLMediaDataClient *mdc)
	: LLEventTimer(time), mMDC(mdc)
{
	mMDC->setIsRunning(true);
}

LLMediaDataClient::QueueTimer::~QueueTimer()
{
	mMDC->setIsRunning(false);
	mMDC = NULL;
}

// virtual
BOOL LLMediaDataClient::QueueTimer::tick()
{
	if (NULL == mMDC->pRequestQueue)
	{
		// Shutting down?  stop.
		LL_DEBUGS("LLMediaDataClient") << "queue gone" << LL_ENDL;
		return TRUE;
	}
	
	LLMediaDataClient::PriorityQueue &queue = *(mMDC->pRequestQueue);
	
	if (queue.empty())
	{
		LL_DEBUGS("LLMediaDataClient") << "queue empty: " << queue << LL_ENDL;
		return TRUE;
	}

	LL_DEBUGS("LLMediaDataClient") << "QueueTimer::tick() started, queue is:	  " << queue << LL_ENDL;

	// Peel one off of the items from the queue, and execute request
	request_ptr_t request = queue.top();
	llassert(!request.isNull());
	const ll_vo_volume_ptr_t &object = request->getObject();
	bool performed_request = false;
	llassert(!object.isNull());
	if (!object.isNull() && object->hasMedia())
	{
		std::string url = request->getCapability();
		if (!url.empty())
		{
			const LLSD &sd_payload = request->getPayload();
			LL_INFOS("LLMediaDataClient") << "Sending request for " << *request << LL_ENDL;

			// Call the subclass for creating the responder
			LLHTTPClient::post(url, sd_payload, mMDC->createResponder(request));
			performed_request = true;
		}
		else {
			LL_INFOS("LLMediaDataClient") << "NOT Sending request for " << *request << ": empty cap url!" << LL_ENDL;
		}
	}
	else {
		if (!object->hasMedia())
		{
			LL_INFOS("LLMediaDataClient") << "Not Sending request for " << *request << " hasMedia() is false!" << LL_ENDL;
		}
	}
	bool exceeded_retries = request->getRetryCount() > LLMediaDataClient::MAX_RETRIES;
	if (performed_request || exceeded_retries) // Try N times before giving up 
	{
		if (exceeded_retries)
		{
			LL_WARNS("LLMediaDataClient") << "Could not send request " << *request << " for " << LLMediaDataClient::MAX_RETRIES << " tries...popping object id " << object->getID() << LL_ENDL; 
			// XXX Should we bring up a warning dialog??
		}
		queue.pop();
	}
	else {
		request->incRetryCount();
	}
	LL_DEBUGS("LLMediaDataClient") << "QueueTimer::tick() finished, queue is now: " << (*(mMDC->pRequestQueue)) << LL_ENDL;

	return queue.empty();
}
	
void LLMediaDataClient::startQueueTimer() 
{
	if (! mQueueTimerIsRunning)
	{
		extern LLControlGroup gSavedSettings;
		F32 queue_timer_delay = gSavedSettings.getF32("PrimMediaRequestQueueDelay");
		if (queue_timer_delay <= 0.0f)
		{
			queue_timer_delay = (F32)LLMediaDataClient::QUEUE_TIMER_DELAY;
		}
		LL_INFOS("LLMediaDataClient") << "starting queue timer (delay=" << queue_timer_delay << " seconds)" << LL_ENDL;
		// LLEventTimer automagically takes care of the lifetime of this object
		new QueueTimer(queue_timer_delay, this);
	}
}
	
void LLMediaDataClient::stopQueueTimer()
{
	mQueueTimerIsRunning = false;
}
	
void LLMediaDataClient::request(LLVOVolume *object, const LLSD &payload)
{
	if (NULL == object || ! object->hasMedia()) return; 

	// Push the object on the priority queue
	enqueue(new Request(getCapabilityName(), payload, object, this));
}

void LLMediaDataClient::enqueue(const Request *request)
{
	LL_INFOS("LLMediaDataClient") << "Queuing request for " << *request << LL_ENDL;
    // Push the request on the priority queue
    // Sadly, we have to const-cast because items put into the queue are not const
	pRequestQueue->push(const_cast<LLMediaDataClient::Request*>(request));
	LL_DEBUGS("LLMediaDataClient") << "Queue:" << (*pRequestQueue) << LL_ENDL;
	// Start the timer if not already running
	startQueueTimer();
}

//////////////////////////////////////////////////////////////////////////////////////
//
// LLMediaDataClient
//
//////////////////////////////////////////////////////////////////////////////////////

LLMediaDataClient::LLMediaDataClient()
{
	pRequestQueue = new PriorityQueue();
}


LLMediaDataClient::~LLMediaDataClient()
{
	stopQueueTimer();
	
	// This should clear the queue, and hopefully call all the destructors.
    while (! pRequestQueue->empty()) pRequestQueue->pop();
	
	delete pRequestQueue;
    pRequestQueue = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////
//
// LLObjectMediaDataClient
// Subclass of LLMediaDataClient for the ObjectMedia cap
//
//////////////////////////////////////////////////////////////////////////////////////

LLMediaDataClient::Responder *LLObjectMediaDataClient::createResponder(const request_ptr_t &request) const
{
    return new LLObjectMediaDataClient::Responder(request);
}

const char *LLObjectMediaDataClient::getCapabilityName() const 
{
    return "ObjectMedia";
}

void LLObjectMediaDataClient::fetchMedia(LLVOVolume *object)
{
    LLSD sd_payload;
    sd_payload["verb"] = "GET";
    sd_payload[LLTextureEntry::OBJECT_ID_KEY] = object->getID();
    request(object, sd_payload);
}

void LLObjectMediaDataClient::updateMedia(LLVOVolume *object)
{
    LLSD sd_payload;
    sd_payload["verb"] = "UPDATE";
    sd_payload[LLTextureEntry::OBJECT_ID_KEY] = object->getID();
    LLSD object_media_data;
    for (int i=0; i < object->getNumTEs(); i++) {
        LLTextureEntry *texture_entry = object->getTE(i);
        llassert((texture_entry->getMediaData() != NULL) == texture_entry->hasMedia());
        const LLSD &media_data =  
            (texture_entry->getMediaData() == NULL) ? LLSD() : texture_entry->getMediaData()->asLLSD();
        object_media_data.append(media_data);
    }
    sd_payload[LLTextureEntry::OBJECT_MEDIA_DATA_KEY] = object_media_data;
        
	LL_INFOS("LLMediaDataClient") << "update media data: " << object->getID() << " " << ll_pretty_print_sd(sd_payload) << LL_ENDL;
    
    request(object, sd_payload);
}

/*virtual*/
void LLObjectMediaDataClient::Responder::result(const LLSD& content)
{
    const LLMediaDataClient::Request::Type type = getRequest()->getType();
    llassert(type == LLMediaDataClient::Request::GET || type == LLMediaDataClient::Request::UPDATE)
    if (type == LLMediaDataClient::Request::GET)
    {
        LL_INFOS("LLMediaDataClient") << *(getRequest()) << "GET returned: " << ll_pretty_print_sd(content) << LL_ENDL;
        
        // Look for an error
        if (content.has("error"))
        {
            const LLSD &error = content["error"];
            LL_WARNS("LLMediaDataClient") << *(getRequest()) << " Error getting media data for object: code=" << 
				error["code"].asString() << ": " << error["message"].asString() << LL_ENDL;
            
            // XXX Warn user?
        }
        else {
            // Check the data
            const LLUUID &object_id = content[LLTextureEntry::OBJECT_ID_KEY];
            if (object_id != getRequest()->getObject()->getID()) 
            {
                // NOT good, wrong object id!!
                LL_WARNS("LLMediaDataClient") << *(getRequest()) << "DROPPING response with wrong object id (" << object_id << ")" << LL_ENDL;
                return;
            }
            
            // Otherwise, update with object media data
            getRequest()->getObject()->updateObjectMediaData(content[LLTextureEntry::OBJECT_MEDIA_DATA_KEY]);
        }
    }
    else if (type == LLMediaDataClient::Request::UPDATE)
    {
        // just do what our superclass does
        LLMediaDataClient::Responder::result(content);
    }
}

//////////////////////////////////////////////////////////////////////////////////////
//
// LLObjectMediaNavigateClient
// Subclass of LLMediaDataClient for the ObjectMediaNavigate cap
//
//////////////////////////////////////////////////////////////////////////////////////
LLMediaDataClient::Responder *LLObjectMediaNavigateClient::createResponder(const request_ptr_t &request) const
{
    return new LLObjectMediaNavigateClient::Responder(request);
}

const char *LLObjectMediaNavigateClient::getCapabilityName() const 
{
    return "ObjectMediaNavigate";
}

void LLObjectMediaNavigateClient::navigate(LLVOVolume *object, U8 texture_index, const std::string &url)
{
    LLSD sd_payload;
    sd_payload[LLTextureEntry::OBJECT_ID_KEY] = object->getID();
	sd_payload[LLMediaEntry::CURRENT_URL_KEY] = url;
	sd_payload[LLTextureEntry::TEXTURE_INDEX_KEY] = (LLSD::Integer)texture_index;
    request(object, sd_payload);
}

/*virtual*/
void LLObjectMediaNavigateClient::Responder::error(U32 status, const std::string& reason)
{
	// Bounce back (unless HTTP_SERVICE_UNAVAILABLE, in which case call base
	// class
	if (status == HTTP_SERVICE_UNAVAILABLE)
	{
		LLMediaDataClient::Responder::error(status, reason);
	}
	else {
		// bounce the face back
		bounceBack();
		LL_WARNS("LLMediaDataClient") << *(getRequest()) << " Error navigating: http code=" << status << LL_ENDL;
	}
}

/*virtual*/
void LLObjectMediaNavigateClient::Responder::result(const LLSD& content)
{
    LL_DEBUGS("LLMediaDataClient") << *(getRequest()) << " NAVIGATE returned " << ll_pretty_print_sd(content) << LL_ENDL;
    
	if (content.has("error"))
	{
		const LLSD &error = content["error"];
		int error_code = error["code"];
		
		if (ERROR_PERMISSION_DENIED_CODE == error_code)
		{
            LL_WARNS("LLMediaDataClient") << *(getRequest()) << " Navigation denied: bounce back" << LL_ENDL;
			// bounce the face back
			bounceBack();
		}
		else {
            LL_WARNS("LLMediaDataClient") << *(getRequest()) << " Error navigating: code=" << 
				error["code"].asString() << ": " << error["message"].asString() << LL_ENDL;
		}            
		// XXX Warn user?
	}
    else {
        // just do what our superclass does
        LLMediaDataClient::Responder::result(content);
    }
}


void LLObjectMediaNavigateClient::Responder::bounceBack()
{
	const LLSD &payload = getRequest()->getPayload();
	U8 texture_index = (U8)(LLSD::Integer)payload[LLTextureEntry::TEXTURE_INDEX_KEY];
    viewer_media_t impl = getRequest()->getObject()->getMediaImpl(texture_index);
    // Find the media entry for this navigate
    LLMediaEntry* mep = NULL;
    LLTextureEntry *te = getRequest()->getObject()->getTE(texture_index);
    if(te)
    {
        mep = te->getMediaData();
    }
    
    if (mep && impl)
    {
//        impl->navigateTo(mep->getCurrentURL(), "", false, true);
    }
}
