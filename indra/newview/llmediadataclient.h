/** 
 * @file llmediadataclient.h
 * @brief class for queueing up requests to the media service
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#ifndef LL_LLMEDIADATACLIENT_H
#define LL_LLMEDIADATACLIENT_H

#include "llhttpclient.h"
#include <queue>
#include "llrefcount.h"
#include "llpointer.h"
#include "lleventtimer.h"


// Link seam for LLVOVolume
class LLMediaDataClientObject : public LLRefCount
{
public:
	// Get the number of media data items
	virtual U8 getMediaDataCount() const = 0;
	// Get the media data at index, as an LLSD
	virtual LLSD getMediaDataLLSD(U8 index) const = 0;
	// Get this object's UUID
	virtual LLUUID getID() const = 0;
	// Navigate back to previous URL
	virtual void mediaNavigateBounceBack(U8 index) = 0;
	// Does this object have media?
	virtual bool hasMedia() const = 0;
	// Update the object's media data to the given array
	virtual void updateObjectMediaData(LLSD const &media_data_array, const std::string &version_string) = 0;
	// Return the total "interest" of the media (on-screen area)
	virtual F64 getMediaInterest() const = 0;
	// Return the given cap url
	virtual std::string getCapabilityUrl(const std::string &name) const = 0;
	// Return whether the object has been marked dead
	virtual bool isDead() const = 0;
	// Returns a media version number for the object
	virtual U32 getMediaVersion() const = 0;
	// Returns whether the object is "interesting enough" to fetch
	virtual bool isInterestingEnough() const = 0;
	// Returns whether we've seen this object yet or not
	virtual bool isNew() const = 0;

	// smart pointer
	typedef LLPointer<LLMediaDataClientObject> ptr_t;
};

// This object creates a priority queue for requests.
// Abstracts the Cap URL, the request, and the responder
class LLMediaDataClient : public LLRefCount
{
public:
    LOG_CLASS(LLMediaDataClient);
    
    const static F32 QUEUE_TIMER_DELAY;// = 1.0; // seconds(s)
	const static F32 UNAVAILABLE_RETRY_TIMER_DELAY;// = 5.0; // secs
	const static U32 MAX_RETRIES;// = 4;
	const static U32 MAX_SORTED_QUEUE_SIZE;// = 10000;
	const static U32 MAX_ROUND_ROBIN_QUEUE_SIZE;// = 10000;

	// Constructor
	LLMediaDataClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
					  F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
		              U32 max_retries = MAX_RETRIES,
					  U32 max_sorted_queue_size = MAX_SORTED_QUEUE_SIZE,
					  U32 max_round_robin_queue_size = MAX_ROUND_ROBIN_QUEUE_SIZE);
	
	// Make the request
	void request(const LLMediaDataClientObject::ptr_t &object, const LLSD &payload);

	F32 getRetryTimerDelay() const { return mRetryTimerDelay; }
	
	// Returns true iff the queue is empty
	bool isEmpty() const;
	
	// Returns true iff the given object is in the queue
	bool isInQueue(const LLMediaDataClientObject::ptr_t &object);
	
	// Remove the given object from the queue. Returns true iff the given object is removed.
	bool removeFromQueue(const LLMediaDataClientObject::ptr_t &object);
	
	// Called only by the Queue timer and tests (potentially)
	bool processQueueTimer();
	
protected:
	// Destructor
	virtual ~LLMediaDataClient(); // use unref
    
	// Request
	class Request : public LLRefCount
	{
	public:
        enum Type {
            GET,
            UPDATE,
            NAVIGATE,
			ANY
        };
        
		Request(const char *cap_name, const LLSD& sd_payload, LLMediaDataClientObject *obj, LLMediaDataClient *mdc);
		const char *getCapName() const { return mCapName; }
		const LLSD &getPayload() const { return mPayload; }
		LLMediaDataClientObject *getObject() const { return mObject; }

        U32 getNum() const { return mNum; }

		U32 getRetryCount() const { return mRetryCount; }
		void incRetryCount() { mRetryCount++; }
		
		// Note: may return empty string!
		std::string getCapability() const;
        
        Type getType() const;
		const char *getTypeAsString() const;
		
		// Re-enqueue thyself
		void reEnqueue() const;
		
		F32 getRetryTimerDelay() const;
		U32 getMaxNumRetries() const;
		
		bool isNew() const { return mObject.notNull() ? mObject->isNew() : false; }
		void markSent(bool flag);
		bool isMarkedSent() const { return mMarkedSent; }
		void updateScore();
		F64 getScore() const { return mScore; }
		
	public:
		friend std::ostream& operator<<(std::ostream &s, const Request &q);
		
    protected:
        virtual ~Request(); // use unref();
        
	private:
		const char *mCapName;
		LLSD mPayload;
		LLMediaDataClientObject::ptr_t mObject;
		// Simple tracking
		U32 mNum;
		static U32 sNum;
        U32 mRetryCount;
		F64 mScore;
		bool mMarkedSent;

		// Back pointer to the MDC...not a ref!
		LLMediaDataClient *mMDC;
	};
	typedef LLPointer<Request> request_ptr_t;

	// Responder
	class Responder : public LLHTTPClient::Responder
	{
	public:
		Responder(const request_ptr_t &request);
		//If we get back an error (not found, etc...), handle it here
		virtual void error(U32 status, const std::string& reason);
		//If we get back a normal response, handle it here.	 Default just logs it.
		virtual void result(const LLSD& content);

		const request_ptr_t &getRequest() const { return mRequest; }

    protected:
		virtual ~Responder();
        
	private:

		class RetryTimer : public LLEventTimer
		{
		public:
			RetryTimer(F32 time, Responder *);
			virtual ~RetryTimer();
			virtual BOOL tick();
		private:
			// back-pointer
			boost::intrusive_ptr<Responder> mResponder;
		};
		
		request_ptr_t mRequest;
	};
	
protected:

	// Subclasses must override this factory method to return a new responder
	virtual Responder *createResponder(const request_ptr_t &request) const = 0;
	
	// Subclasses must override to return a cap name
	virtual const char *getCapabilityName() const = 0;
	
	virtual void sortQueue();
	virtual void serviceQueue();
	
private:
	typedef std::list<request_ptr_t> request_queue_t;
		
	void enqueue(const Request*);
	
	// Return whether the given object is/was in the queue
	static LLMediaDataClient::request_ptr_t findOrRemove(request_queue_t &queue, const LLMediaDataClientObject::ptr_t &obj, bool remove, Request::Type type);
	
	// Comparator for sorting
	static bool compareRequests(const request_ptr_t &o1, const request_ptr_t &o2);
	static F64 getObjectScore(const LLMediaDataClientObject::ptr_t &obj);
    
	friend std::ostream& operator<<(std::ostream &s, const Request &q);
	friend std::ostream& operator<<(std::ostream &s, const request_queue_t &q);

	class QueueTimer : public LLEventTimer
	{
	public:
		QueueTimer(F32 time, LLMediaDataClient *mdc);
		virtual BOOL tick();
    protected:
		virtual ~QueueTimer();
	private:
		// back-pointer
		LLPointer<LLMediaDataClient> mMDC;
	};
	
	void startQueueTimer();
	void stopQueueTimer();
	void setIsRunning(bool val) { mQueueTimerIsRunning = val; }
	
	void swapCurrentQueue();
	request_queue_t *getCurrentQueue();
	
	const F32 mQueueTimerDelay;
	const F32 mRetryTimerDelay;
	const U32 mMaxNumRetries;
	const U32 mMaxSortedQueueSize;
	const U32 mMaxRoundRobinQueueSize;
	
	bool mQueueTimerIsRunning;
	
	request_queue_t mSortedQueue;
	request_queue_t mRoundRobinQueue;
	bool mCurrentQueueIsTheSortedQueue;
};


// MediaDataClient specific for the ObjectMedia cap
class LLObjectMediaDataClient : public LLMediaDataClient
{
public:
    LLObjectMediaDataClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
							F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
							U32 max_retries = MAX_RETRIES,
							U32 max_sorted_queue_size = MAX_SORTED_QUEUE_SIZE,
							U32 max_round_robin_queue_size = MAX_ROUND_ROBIN_QUEUE_SIZE)
		: LLMediaDataClient(queue_timer_delay, retry_timer_delay, max_retries)
		{}
    virtual ~LLObjectMediaDataClient() {}
    
	void fetchMedia(LLMediaDataClientObject *object); 
    void updateMedia(LLMediaDataClientObject *object);
    
protected:
	// Subclasses must override this factory method to return a new responder
	virtual Responder *createResponder(const request_ptr_t &request) const;
	
	// Subclasses must override to return a cap name
	virtual const char *getCapabilityName() const;
    
    class Responder : public LLMediaDataClient::Responder
    {
    public:
        Responder(const request_ptr_t &request)
            : LLMediaDataClient::Responder(request) {}
        virtual void result(const LLSD &content);
    };
};


// MediaDataClient specific for the ObjectMediaNavigate cap
class LLObjectMediaNavigateClient : public LLMediaDataClient
{
public:
	// NOTE: from llmediaservice.h
	static const int ERROR_PERMISSION_DENIED_CODE = 8002;
	
    LLObjectMediaNavigateClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
								F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
								U32 max_retries = MAX_RETRIES,
								U32 max_sorted_queue_size = MAX_SORTED_QUEUE_SIZE,
								U32 max_round_robin_queue_size = MAX_ROUND_ROBIN_QUEUE_SIZE)
		: LLMediaDataClient(queue_timer_delay, retry_timer_delay, max_retries)
		{}
    virtual ~LLObjectMediaNavigateClient() {}
    
    void navigate(LLMediaDataClientObject *object, U8 texture_index, const std::string &url);
    
protected:
	// Subclasses must override this factory method to return a new responder
	virtual Responder *createResponder(const request_ptr_t &request) const;
	
	// Subclasses must override to return a cap name
	virtual const char *getCapabilityName() const;

    class Responder : public LLMediaDataClient::Responder
    {
    public:
        Responder(const request_ptr_t &request)
            : LLMediaDataClient::Responder(request) {}
		virtual void error(U32 status, const std::string& reason);
        virtual void result(const LLSD &content);
    private:
        void mediaNavigateBounceBack();
    };

};


#endif // LL_LLMEDIADATACLIENT_H
