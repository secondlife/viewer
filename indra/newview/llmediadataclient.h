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
#include "lltimer.h"

// Forward decls
class LLVOVolume;

typedef LLPointer<LLVOVolume> ll_vo_volume_ptr_t;

// This object creates a priority queue for requests.
// Abstracts the Cap URL, the request, and the responder
class LLMediaDataClient : public LLRefCount
{
public:
    LOG_CLASS(LLMediaDataClient);
    
    const static int QUEUE_TIMER_DELAY = 1; // seconds(s)
	const static int MAX_RETRIES = 4;

	// Constructor
	LLMediaDataClient();
	
	// Make the request
	void request(LLVOVolume *object, const LLSD &payload);
    
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
        
		Request(const std::string &cap_name, const LLSD& sd_payload, LLVOVolume *obj, LLMediaDataClient *mdc);
		const std::string &getCapName() const { return mCapName; }
		const LLSD &getPayload() const { return mPayload; }
		LLVOVolume *getObject() const { return mObject; }

        U32 getNum() const { return mNum; }

		U32 getRetryCount() const { return mRetryCount; }
		void incRetryCount() { mRetryCount++; }
		
		// Note: may return empty string!
		std::string getCapability() const;
        
        Type getType() const;
		const char *getTypeAsString() const;
		
		// Re-enqueue thyself
		void reEnqueue() const;
		
	public:
		friend std::ostream& operator<<(std::ostream &s, const Request &q);
		
    protected:
        virtual ~Request(); // use unref();
        
	private:
		std::string mCapName;
		LLSD mPayload;
		ll_vo_volume_ptr_t mObject;
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
		static const int UNAVAILABLE_RETRY_TIMER_DELAY = 5; // secs

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
		static F64 getObjectScore(const ll_vo_volume_ptr_t &obj);
	};
	
    // PriorityQueue
	class PriorityQueue : public std::priority_queue<
		request_ptr_t, 
		std::vector<request_ptr_t>, 
		Comparator >
	{
	public:
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
	
	bool mQueueTimerIsRunning;
	
	PriorityQueue *pRequestQueue;
};


// MediaDataResponder specific for the ObjectMedia cap
class LLObjectMediaDataClient : public LLMediaDataClient
{
public:
    LLObjectMediaDataClient() {}
    ~LLObjectMediaDataClient() {}
    
	void fetchMedia(LLVOVolume *object); 
    void updateMedia(LLVOVolume *object);
    
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
	// NOTE: from llmediaservice.h
	static const int ERROR_PERMISSION_DENIED_CODE = 8002;
	
public:
    LLObjectMediaNavigateClient() {}
    ~LLObjectMediaNavigateClient() {}
    
    void navigate(LLVOVolume *object, U8 texture_index, const std::string &url);
    
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
