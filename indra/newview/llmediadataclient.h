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
#include "lltimer.h"


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
	virtual void updateObjectMediaData(LLSD const &media_data_array) = 0;
	// Return the distance from the object to the avatar
	virtual F64 getDistanceFromAvatar() const = 0;
	// Return the total "interest" of the media (on-screen area)
	virtual F64 getTotalMediaInterest() const = 0;
	// Return the given cap url
	virtual std::string getCapabilityUrl(const std::string &name) const = 0;

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

	// Constructor
	LLMediaDataClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
					  F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
		              U32 max_retries = MAX_RETRIES);
	
	// Make the request
	void request(const LLMediaDataClientObject::ptr_t &object, const LLSD &payload);

	F32 getRetryTimerDelay() const { return mRetryTimerDelay; }
	
	// Returns true iff the queue is empty
	bool isEmpty() const;
	
	// Returns true iff the given object is in the queue
	bool isInQueue(const LLMediaDataClientObject::ptr_t &object) const;
	
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
            NAVIGATE
        };
        
		Request(const std::string &cap_name, const LLSD& sd_payload, LLMediaDataClientObject *obj, LLMediaDataClient *mdc);
		const std::string &getCapName() const { return mCapName; }
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
		
	public:
		friend std::ostream& operator<<(std::ostream &s, const Request &q);
		
    protected:
        virtual ~Request(); // use unref();
        
	private:
		std::string mCapName;
		LLSD mPayload;
		LLMediaDataClientObject::ptr_t mObject;
		// Simple tracking
		const U32 mNum;
		static U32 sNum;
        U32 mRetryCount;
		
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
	
	void enqueue(const Request*);
	
	// Subclasses must override this factory method to return a new responder
	virtual Responder *createResponder(const request_ptr_t &request) const = 0;
	
	// Subclasses must override to return a cap name
	virtual const char *getCapabilityName() const = 0;
	
private:
	
	// Comparator for PriorityQueue
	class Comparator
	{
	public:
		bool operator() (const request_ptr_t &o1, const request_ptr_t &o2) const;
	private:
		static F64 getObjectScore(const LLMediaDataClientObject::ptr_t &obj);
	};
	
    // PriorityQueue
	class PriorityQueue : public std::priority_queue<
		request_ptr_t, 
		std::vector<request_ptr_t>, 
		Comparator >
	{
	public:
		// Return whether the given object is in the queue
		bool find(const LLMediaDataClientObject::ptr_t &obj) const;
		
		friend std::ostream& operator<<(std::ostream &s, const PriorityQueue &q);
	};
    
	friend std::ostream& operator<<(std::ostream &s, const Request &q);
    friend std::ostream& operator<<(std::ostream &s, const PriorityQueue &q);

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

	const F32 mQueueTimerDelay;
	const F32 mRetryTimerDelay;
	const U32 mMaxNumRetries;
	
	bool mQueueTimerIsRunning;
	
	PriorityQueue *pRequestQueue;
};


// MediaDataResponder specific for the ObjectMedia cap
class LLObjectMediaDataClient : public LLMediaDataClient
{
public:
    LLObjectMediaDataClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
							F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
							U32 max_retries = MAX_RETRIES)
		: LLMediaDataClient(queue_timer_delay, retry_timer_delay, max_retries)
		{}
    ~LLObjectMediaDataClient() {}
    
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


// MediaDataResponder specific for the ObjectMediaNavigate cap
class LLObjectMediaNavigateClient : public LLMediaDataClient
{
public:
	// NOTE: from llmediaservice.h
	static const int ERROR_PERMISSION_DENIED_CODE = 8002;
	
    LLObjectMediaNavigateClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
								F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
								U32 max_retries = MAX_RETRIES)
		: LLMediaDataClient(queue_timer_delay, retry_timer_delay, max_retries)
		{}
    ~LLObjectMediaNavigateClient() {}
    
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
