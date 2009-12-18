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

#if LL_MSVC
// disable boost::lexical_cast warning
#pragma warning (disable:4702)
#endif

#include <boost/lexical_cast.hpp>

#include "llhttpstatuscodes.h"
#include "llsdutil.h"
#include "llmediaentry.h"
#include "lltextureentry.h"
#include "llviewerregion.h"

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

//
// Forward decls
//
const F32 LLMediaDataClient::QUEUE_TIMER_DELAY = 1.0; // seconds(s)
const F32 LLMediaDataClient::UNAVAILABLE_RETRY_TIMER_DELAY = 5.0; // secs
const U32 LLMediaDataClient::MAX_RETRIES = 4;
const U32 LLMediaDataClient::MAX_SORTED_QUEUE_SIZE = 10000;
const U32 LLMediaDataClient::MAX_ROUND_ROBIN_QUEUE_SIZE = 10000;

// << operators
std::ostream& operator<<(std::ostream &s, const LLMediaDataClient::request_queue_t &q);
std::ostream& operator<<(std::ostream &s, const LLMediaDataClient::Request &q);

//////////////////////////////////////////////////////////////////////////////////////
//
// LLMediaDataClient
//
//////////////////////////////////////////////////////////////////////////////////////

LLMediaDataClient::LLMediaDataClient(F32 queue_timer_delay,
									 F32 retry_timer_delay,
									 U32 max_retries,
									 U32 max_sorted_queue_size,
									 U32 max_round_robin_queue_size)
	: mQueueTimerDelay(queue_timer_delay),
	  mRetryTimerDelay(retry_timer_delay),
	  mMaxNumRetries(max_retries),
	  mMaxSortedQueueSize(max_sorted_queue_size),
	  mMaxRoundRobinQueueSize(max_round_robin_queue_size),
	  mQueueTimerIsRunning(false),
	  mCurrentQueueIsTheSortedQueue(true)
{
}

LLMediaDataClient::~LLMediaDataClient()
{
	stopQueueTimer();

	// This should clear the queue, and hopefully call all the destructors.
	LL_DEBUGS("LLMediaDataClient") << "~LLMediaDataClient destructor: queue: " << 
		(isEmpty() ? "<empty> " : "<not empty> ") << LL_ENDL;
	
	mSortedQueue.clear();
	mRoundRobinQueue.clear();
}

bool LLMediaDataClient::isEmpty() const
{
	return mSortedQueue.empty() && mRoundRobinQueue.empty();
}

bool LLMediaDataClient::isInQueue(const LLMediaDataClientObject::ptr_t &object)
{
	return (LLMediaDataClient::findOrRemove(mSortedQueue, object, false/*remove*/, LLMediaDataClient::Request::ANY).notNull()
		|| (LLMediaDataClient::findOrRemove(mRoundRobinQueue, object, false/*remove*/, LLMediaDataClient::Request::ANY).notNull()));
}

bool LLMediaDataClient::removeFromQueue(const LLMediaDataClientObject::ptr_t &object)
{
	bool removedFromSortedQueue = LLMediaDataClient::findOrRemove(mSortedQueue, object, true/*remove*/, LLMediaDataClient::Request::ANY).notNull();
	bool removedFromRoundRobinQueue = LLMediaDataClient::findOrRemove(mRoundRobinQueue, object, true/*remove*/, LLMediaDataClient::Request::ANY).notNull();
	return removedFromSortedQueue || removedFromRoundRobinQueue;
}

//static
LLMediaDataClient::request_ptr_t LLMediaDataClient::findOrRemove(request_queue_t &queue, const LLMediaDataClientObject::ptr_t &obj, bool remove, LLMediaDataClient::Request::Type type)
{
	request_ptr_t result;
	request_queue_t::iterator iter = queue.begin();
	request_queue_t::iterator end = queue.end();
	while (iter != end)
	{
		if (obj->getID() == (*iter)->getObject()->getID() && (type == LLMediaDataClient::Request::ANY || type == (*iter)->getType()))
		{
			result = *iter;
			if (remove) queue.erase(iter);
			break;
		}
		iter++;
	}
	return result;
}

void LLMediaDataClient::request(const LLMediaDataClientObject::ptr_t &object, const LLSD &payload)
{
	if (object.isNull() || ! object->hasMedia()) return; 
	
	// Push the object on the queue
	enqueue(new Request(getCapabilityName(), payload, object, this));
}

void LLMediaDataClient::enqueue(const Request *request)
{
	if (request->isNew())
	{		
		// Add to sorted queue
		if (LLMediaDataClient::findOrRemove(mSortedQueue, request->getObject(), true/*remove*/, request->getType()).notNull())
		{
			LL_DEBUGS("LLMediaDataClient") << "REMOVING OLD request for " << *request << " ALREADY THERE!" << LL_ENDL;
		}
		
		LL_DEBUGS("LLMediaDataClient") << "Queuing SORTED request for " << *request << LL_ENDL;
		
		// Sadly, we have to const-cast because items put into the queue are not const
		mSortedQueue.push_back(const_cast<LLMediaDataClient::Request*>(request));
		
		LL_DEBUGS("LLMediaDataClientQueue") << "SORTED queue:" << mSortedQueue << LL_ENDL;
	}
	else {
		if (mRoundRobinQueue.size() > mMaxRoundRobinQueueSize) 
		{
			LL_INFOS_ONCE("LLMediaDataClient") << "RR QUEUE MAXED OUT!!!" << LL_ENDL;
			LL_DEBUGS("LLMediaDataClient") << "Not queuing " << *request << LL_ENDL;
			return;
		}
				
		// ROUND ROBIN: if it is there, and it is a GET request, leave it.  If not, put at front!		
		request_ptr_t existing_request;
		if (request->getType() == Request::GET)
		{
			existing_request = LLMediaDataClient::findOrRemove(mRoundRobinQueue, request->getObject(), false/*remove*/, request->getType());
		}
		if (existing_request.isNull())
		{
			LL_DEBUGS("LLMediaDataClient") << "Queuing RR request for " << *request << LL_ENDL;
			// Push the request on the pending queue
			// Sadly, we have to const-cast because items put into the queue are not const
			mRoundRobinQueue.push_front(const_cast<LLMediaDataClient::Request*>(request));
			
			LL_DEBUGS("LLMediaDataClientQueue") << "RR queue:" << mRoundRobinQueue << LL_ENDL;			
		}
		else
		{
			LL_DEBUGS("LLMediaDataClient") << "ALREADY THERE: NOT Queuing request for " << *request << LL_ENDL;
						
			existing_request->markSent(false);
		}
	}	
	// Start the timer if not already running
	startQueueTimer();
}

void LLMediaDataClient::startQueueTimer() 
{
	if (! mQueueTimerIsRunning)
	{
		LL_DEBUGS("LLMediaDataClient") << "starting queue timer (delay=" << mQueueTimerDelay << " seconds)" << LL_ENDL;
		// LLEventTimer automagically takes care of the lifetime of this object
		new QueueTimer(mQueueTimerDelay, this);
	}
	else { 
		LL_DEBUGS("LLMediaDataClient") << "not starting queue timer (it's already running, right???)" << LL_ENDL;
	}
}

void LLMediaDataClient::stopQueueTimer()
{
	mQueueTimerIsRunning = false;
}

bool LLMediaDataClient::processQueueTimer()
{
	sortQueue();
	
	if(!isEmpty())
	{
		LL_DEBUGS("LLMediaDataClient") << "QueueTimer::tick() started, SORTED queue size is:	  " << mSortedQueue.size() 
			<< ", RR queue size is:	  " << mRoundRobinQueue.size() << LL_ENDL;
		LL_DEBUGS("LLMediaDataClientQueue") << "QueueTimer::tick() started, SORTED queue is:	  " << mSortedQueue << LL_ENDL;
		LL_DEBUGS("LLMediaDataClientQueue") << "QueueTimer::tick() started, RR queue is:	  " << mRoundRobinQueue << LL_ENDL;
	}
	
	serviceQueue();
	
	LL_DEBUGS("LLMediaDataClient") << "QueueTimer::tick() finished, SORTED queue size is:	  " << mSortedQueue.size() 
		<< ", RR queue size is:	  " << mRoundRobinQueue.size() << LL_ENDL;
	LL_DEBUGS("LLMediaDataClientQueue") << "QueueTimer::tick() finished, SORTED queue is:	  " << mSortedQueue << LL_ENDL;
	LL_DEBUGS("LLMediaDataClientQueue") << "QueueTimer::tick() finished, RR queue is:	  " << mRoundRobinQueue << LL_ENDL;
	
	return isEmpty();
}

void LLMediaDataClient::sortQueue()
{
	if(!mSortedQueue.empty())
	{
		// Score all items first
		request_queue_t::iterator iter = mSortedQueue.begin();
		request_queue_t::iterator end = mSortedQueue.end();
		while (iter != end)
		{
			(*iter)->updateScore();
			iter++;
		}
		
		// Re-sort the list...
		// NOTE: should this be a stable_sort?  If so we need to change to using a vector.
		mSortedQueue.sort(LLMediaDataClient::compareRequests);
		
		// ...then cull items over the max
		U32 size = mSortedQueue.size();
		if (size > mMaxSortedQueueSize) 
		{
			U32 num_to_cull = (size - mMaxSortedQueueSize);
			LL_INFOS_ONCE("LLMediaDataClient") << "sorted queue MAXED OUT!  Culling " 
				<< num_to_cull << " items" << LL_ENDL;
			while (num_to_cull-- > 0)
			{
				mSortedQueue.pop_back();
			}
		}
	}
}

// static
bool LLMediaDataClient::compareRequests(const request_ptr_t &o1, const request_ptr_t &o2)
{
	if (o2.isNull()) return true;
	if (o1.isNull()) return false;
	return ( o1->getScore() > o2->getScore() );
}

void LLMediaDataClient::serviceQueue()
{	
	request_queue_t *queue_p = getCurrentQueue();
	
	// quick retry loop for cases where we shouldn't wait for the next timer tick
	while(true)
	{
		if (queue_p->empty())
		{
			LL_DEBUGS("LLMediaDataClient") << "queue empty: " << (*queue_p) << LL_ENDL;
			break;
		}
		
		// Peel one off of the items from the queue, and execute request
		request_ptr_t request = queue_p->front();
		llassert(!request.isNull());
		const LLMediaDataClientObject *object = (request.isNull()) ? NULL : request->getObject();
		llassert(NULL != object);
		
		// Check for conditions that would make us just pop and rapidly loop through
		// the queue.
		if(request.isNull() ||
		   request->isMarkedSent() ||
		   NULL == object ||
		   object->isDead() ||
		   !object->hasMedia())
		{
			if (request.isNull()) 
			{
				LL_WARNS("LLMediaDataClient") << "Skipping NULL request" << LL_ENDL;
			}
			else {
				LL_INFOS("LLMediaDataClient") << "Skipping : " << *request << " " 
				<< ((request->isMarkedSent()) ? " request is marked sent" :
					((NULL == object) ? " object is NULL " :
					 ((object->isDead()) ? "object is dead" : 
					  ((!object->hasMedia()) ? "object has no media!" : "BADNESS!")))) << LL_ENDL;
			}
			queue_p->pop_front();
			continue;	// jump back to the start of the quick retry loop
		}
		
		// Next, ask if this is "interesting enough" to fetch.  If not, just stop
		// and wait for the next timer go-round.  Only do this for the sorted 
		// queue.
		if (mCurrentQueueIsTheSortedQueue && !object->isInterestingEnough())
		{
			LL_DEBUGS("LLMediaDataClient") << "Not fetching " << *request << ": not interesting enough" << LL_ENDL;
			break;
		}
		
		// Finally, try to send the HTTP message to the cap url
		std::string url = request->getCapability();
		bool maybe_retry = false;
		if (!url.empty())
		{
			const LLSD &sd_payload = request->getPayload();
			LL_INFOS("LLMediaDataClient") << "Sending request for " << *request << LL_ENDL;
			
			// Call the subclass for creating the responder
			LLHTTPClient::post(url, sd_payload, createResponder(request));
		}
		else {
			LL_INFOS("LLMediaDataClient") << "NOT Sending request for " << *request << ": empty cap url!" << LL_ENDL;
			maybe_retry = true;
		}

		bool exceeded_retries = request->getRetryCount() > mMaxNumRetries;
		if (maybe_retry && ! exceeded_retries) // Try N times before giving up 
		{
			// We got an empty cap, but in that case we will retry again next
			// timer fire.
			request->incRetryCount();
		}
		else {
			if (exceeded_retries)
			{
				LL_WARNS("LLMediaDataClient") << "Could not send request " << *request << " for " 
					<< mMaxNumRetries << " tries...popping object id " << object->getID() << LL_ENDL; 
				// XXX Should we bring up a warning dialog??
			}
			
			queue_p->pop_front();
			
			if (! mCurrentQueueIsTheSortedQueue) {
				// Round robin
				request->markSent(true);
				mRoundRobinQueue.push_back(request);				
			}
		}
		
 		// end of quick loop -- any cases where we want to loop will use 'continue' to jump back to the start.
 		break;
	}
	
	swapCurrentQueue();
}

void LLMediaDataClient::swapCurrentQueue()
{
	// Swap
	mCurrentQueueIsTheSortedQueue = !mCurrentQueueIsTheSortedQueue;
	// If its empty, swap back
	if (getCurrentQueue()->empty()) 
	{
		mCurrentQueueIsTheSortedQueue = !mCurrentQueueIsTheSortedQueue;
	}
}

LLMediaDataClient::request_queue_t *LLMediaDataClient::getCurrentQueue()
{
	return (mCurrentQueueIsTheSortedQueue) ? &mSortedQueue : &mRoundRobinQueue;
}

// dump the queue
std::ostream& operator<<(std::ostream &s, const LLMediaDataClient::request_queue_t &q)
{
	int i = 0;
	LLMediaDataClient::request_queue_t::const_iterator iter = q.begin();
	LLMediaDataClient::request_queue_t::const_iterator end = q.end();
	while (iter != end)
	{
		s << "\t" << i << "]: " << (*iter)->getObject()->getID().asString() << "(" << (*iter)->getObject()->getMediaInterest() << ")";
		iter++;
		i++;
	}
	return s;
}

//////////////////////////////////////////////////////////////////////////////////////
//
// LLMediaDataClient::QueueTimer
// Queue of LLMediaDataClientObject smart pointers to request media for.
//
//////////////////////////////////////////////////////////////////////////////////////

LLMediaDataClient::QueueTimer::QueueTimer(F32 time, LLMediaDataClient *mdc)
: LLEventTimer(time), mMDC(mdc)
{
	mMDC->setIsRunning(true);
}

LLMediaDataClient::QueueTimer::~QueueTimer()
{
	LL_DEBUGS("LLMediaDataClient") << "~QueueTimer" << LL_ENDL;
	mMDC->setIsRunning(false);
	mMDC = NULL;
}

// virtual
BOOL LLMediaDataClient::QueueTimer::tick()
{
	if (mMDC.isNull()) return TRUE;
	return mMDC->processQueueTimer();
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
	LL_DEBUGS("LLMediaDataClient") << "~RetryTimer" << *(mResponder->getRequest()) << LL_ENDL;
	
	// XXX This is weird: Instead of doing the work in tick()  (which re-schedules
	// a timer, which might be risky), do it here, in the destructor.  Yes, it is very odd.
	// Instead of retrying, we just put the request back onto the queue
	LL_INFOS("LLMediaDataClient") << "RetryTimer fired for: " << *(mResponder->getRequest()) << " retrying" << LL_ENDL;
	mResponder->getRequest()->reEnqueue();
	
	// Release the ref to the responder.
	mResponder = NULL;
}

// virtual
BOOL LLMediaDataClient::Responder::RetryTimer::tick()
{
	// Don't fire again
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// LLMediaDataClient::Request
//
//////////////////////////////////////////////////////////////////////////////////////
/*static*/U32 LLMediaDataClient::Request::sNum = 0;

LLMediaDataClient::Request::Request(const char *cap_name, 
									const LLSD& sd_payload,
									LLMediaDataClientObject *obj, 
									LLMediaDataClient *mdc)
: mCapName(cap_name), 
  mPayload(sd_payload), 
  mObject(obj),
  mNum(++sNum), 
  mRetryCount(0),
  mMDC(mdc),
  mMarkedSent(false),
  mScore((F64)0.0)
{
}

LLMediaDataClient::Request::~Request()
{
	LL_DEBUGS("LLMediaDataClient") << "~Request" << (*this) << LL_ENDL;
	mMDC = NULL;
	mObject = NULL;
}


std::string LLMediaDataClient::Request::getCapability() const
{
	return getObject()->getCapabilityUrl(getCapName());
}

// Helper function to get the "type" of request, which just pokes around to
// discover it.
LLMediaDataClient::Request::Type LLMediaDataClient::Request::getType() const
{
	if (0 == strcmp(mCapName, "ObjectMediaNavigate"))
	{
		return NAVIGATE;
	}
	else if (0 == strcmp(mCapName, "ObjectMedia"))
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
		case ANY:
			return "ANY";
			break;
	}
	return "";
}


void LLMediaDataClient::Request::reEnqueue() const
{
	// I sure hope this doesn't deref a bad pointer:
	mMDC->enqueue(this);
}

F32 LLMediaDataClient::Request::getRetryTimerDelay() const
{
	return (mMDC == NULL) ? LLMediaDataClient::UNAVAILABLE_RETRY_TIMER_DELAY :
	mMDC->mRetryTimerDelay; 
}

U32 LLMediaDataClient::Request::getMaxNumRetries() const
{
	return (mMDC == NULL) ? LLMediaDataClient::MAX_RETRIES : mMDC->mMaxNumRetries;
}

void LLMediaDataClient::Request::markSent(bool flag)
{
	 if (mMarkedSent != flag)
	 {
		 mMarkedSent = flag;
		 if (!mMarkedSent)
		 {
			 mNum = ++sNum;
		 }
	 }
}
		   
void LLMediaDataClient::Request::updateScore()
{				
	F64 tmp = mObject->getMediaInterest();
	if (tmp != mScore)
	{
		LL_DEBUGS("LLMediaDataClient") << "Score for " << mObject->getID() << " changed from " << mScore << " to " << tmp << LL_ENDL; 
		mScore = tmp;
	}
}
		   
std::ostream& operator<<(std::ostream &s, const LLMediaDataClient::Request &r)
{
	s << "request: num=" << r.getNum() 
	<< " type=" << r.getTypeAsString() 
	<< " ID=" << r.getObject()->getID() 
	<< " #retries=" << r.getRetryCount();
	return s;
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
	LL_DEBUGS("LLMediaDataClient") << "~Responder" << *(getRequest()) << LL_ENDL;
	mRequest = NULL;
}

/*virtual*/
void LLMediaDataClient::Responder::error(U32 status, const std::string& reason)
{
	if (status == HTTP_SERVICE_UNAVAILABLE)
	{
		F32 retry_timeout = mRequest->getRetryTimerDelay();
		
		mRequest->incRetryCount();
		
		if (mRequest->getRetryCount() < mRequest->getMaxNumRetries()) 
		{
			LL_INFOS("LLMediaDataClient") << *mRequest << " got SERVICE_UNAVAILABLE...retrying in " << retry_timeout << " seconds" << LL_ENDL;
			
			// Start timer (instances are automagically tracked by
			// InstanceTracker<> and LLEventTimer)
			new RetryTimer(F32(retry_timeout/*secs*/), this);
		}
		else {
			LL_INFOS("LLMediaDataClient") << *mRequest << " got SERVICE_UNAVAILABLE...retry count " 
				<< mRequest->getRetryCount() << " exceeds " << mRequest->getMaxNumRetries() << ", not retrying" << LL_ENDL;
		}
	}
	else {
		std::string msg = boost::lexical_cast<std::string>(status) + ": " + reason;
		LL_WARNS("LLMediaDataClient") << *mRequest << " http error(" << msg << ")" << LL_ENDL;
	}
}

/*virtual*/
void LLMediaDataClient::Responder::result(const LLSD& content)
{
	LL_DEBUGS("LLMediaDataClientResponse") << *mRequest << " result : " << ll_print_sd(content) << LL_ENDL;
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

void LLObjectMediaDataClient::fetchMedia(LLMediaDataClientObject *object)
{
	LLSD sd_payload;
	sd_payload["verb"] = "GET";
	sd_payload[LLTextureEntry::OBJECT_ID_KEY] = object->getID();
	request(object, sd_payload);
}

void LLObjectMediaDataClient::updateMedia(LLMediaDataClientObject *object)
{
	LLSD sd_payload;
	sd_payload["verb"] = "UPDATE";
	sd_payload[LLTextureEntry::OBJECT_ID_KEY] = object->getID();
	LLSD object_media_data;
	int i = 0;
	int end = object->getMediaDataCount();
	for ( ; i < end ; ++i) 
	{
		object_media_data.append(object->getMediaDataLLSD(i));
	}
	sd_payload[LLTextureEntry::OBJECT_MEDIA_DATA_KEY] = object_media_data;
		
	LL_DEBUGS("LLMediaDataClient") << "update media data: " << object->getID() << " " << ll_print_sd(sd_payload) << LL_ENDL;
	
	request(object, sd_payload);
}

/*virtual*/
void LLObjectMediaDataClient::Responder::result(const LLSD& content)
{
	const LLMediaDataClient::Request::Type type = getRequest()->getType();
	llassert(type == LLMediaDataClient::Request::GET || type == LLMediaDataClient::Request::UPDATE)
	if (type == LLMediaDataClient::Request::GET)
	{
		LL_DEBUGS("LLMediaDataClientResponse") << *(getRequest()) << " GET returned: " << ll_print_sd(content) << LL_ENDL;
		
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
				LL_WARNS("LLMediaDataClient") << *(getRequest()) << " DROPPING response with wrong object id (" << object_id << ")" << LL_ENDL;
				return;
			}
			
			// Otherwise, update with object media data
			getRequest()->getObject()->updateObjectMediaData(content[LLTextureEntry::OBJECT_MEDIA_DATA_KEY],
															 content[LLTextureEntry::MEDIA_VERSION_KEY]);
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

void LLObjectMediaNavigateClient::navigate(LLMediaDataClientObject *object, U8 texture_index, const std::string &url)
{
	LLSD sd_payload;
	sd_payload[LLTextureEntry::OBJECT_ID_KEY] = object->getID();
	sd_payload[LLMediaEntry::CURRENT_URL_KEY] = url;
	sd_payload[LLTextureEntry::TEXTURE_INDEX_KEY] = (LLSD::Integer)texture_index;
	
	LL_INFOS("LLMediaDataClient") << "navigate() initiated: " << ll_print_sd(sd_payload) << LL_ENDL;
	
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
		LL_WARNS("LLMediaDataClient") << *(getRequest()) << " Error navigating: http code=" << status << LL_ENDL;
		const LLSD &payload = getRequest()->getPayload();
		// bounce the face back
		getRequest()->getObject()->mediaNavigateBounceBack((LLSD::Integer)payload[LLTextureEntry::TEXTURE_INDEX_KEY]);
	}
}

/*virtual*/
void LLObjectMediaNavigateClient::Responder::result(const LLSD& content)
{
	LL_INFOS("LLMediaDataClient") << *(getRequest()) << " NAVIGATE returned " << ll_print_sd(content) << LL_ENDL;
	
	if (content.has("error"))
	{
		const LLSD &error = content["error"];
		int error_code = error["code"];
		
		if (ERROR_PERMISSION_DENIED_CODE == error_code)
		{
			LL_WARNS("LLMediaDataClient") << *(getRequest()) << " Navigation denied: bounce back" << LL_ENDL;
			const LLSD &payload = getRequest()->getPayload();
			// bounce the face back
			getRequest()->getObject()->mediaNavigateBounceBack((LLSD::Integer)payload[LLTextureEntry::TEXTURE_INDEX_KEY]);
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
