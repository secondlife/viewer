/** 
 * @file llmediadataclient.h
 * @brief class for queueing up requests to the media service
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLMEDIADATACLIENT_H
#define LL_LLMEDIADATACLIENT_H

#include <set>
#include "llrefcount.h"
#include "llpointer.h"
#include "lleventtimer.h"
#include "llhttpsdhandler.h"
#include "httpcommon.h"
#include "httprequest.h"
#include "httpoptions.h"
#include "httpheaders.h"

// Link seam for LLVOVolume
class LLMediaDataClientObject : public LLRefCount
{
public:
    // Get the number of media data items
    virtual U8 getMediaDataCount() const = 0;
    // Get the media data at index, as an LLSD
    virtual LLSD getMediaDataLLSD(U8 index) const = 0;
    // Return true if the current URL for the face in the media data matches the specified URL.
    virtual bool isCurrentMediaUrl(U8 index, const std::string &url) const = 0;
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
    friend class PredicateMatchRequest;

protected:
    LOG_CLASS(LLMediaDataClient);
public:
    
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
    
    F32 getRetryTimerDelay() const { return mRetryTimerDelay; }
    
    // Returns true iff the queue is empty
    virtual bool isEmpty() const;
    
    // Returns true iff the given object is in the queue
    virtual bool isInQueue(const LLMediaDataClientObject::ptr_t &object);
    
    // Remove the given object from the queue. Returns true iff the given object is removed.
    virtual void removeFromQueue(const LLMediaDataClientObject::ptr_t &object);
    
    // Called only by the Queue timer and tests (potentially)
    virtual bool processQueueTimer();
    
protected:
    // Destructor
    virtual ~LLMediaDataClient(); // use unref
    
    // Request (pure virtual base class for requests in the queue)
    class Request: 
        public boost::enable_shared_from_this<Request>
    {
    public:
        typedef boost::shared_ptr<Request> ptr_t;

        // Subclasses must implement this to build a payload for their request type.
        virtual LLSD getPayload() const = 0;
        // and must create the correct type of responder.
        virtual LLCore::HttpHandler::ptr_t createHandler() = 0;

        virtual std::string getURL() { return ""; }

        enum Type {
            GET,
            UPDATE,
            NAVIGATE,
            ANY
        };

        virtual ~Request()
        { }

    protected:
        // The only way to create one of these is through a subclass.
        Request(Type in_type, LLMediaDataClientObject *obj, LLMediaDataClient *mdc, S32 face = -1);
    public:
        LLMediaDataClientObject *getObject() const { return mObject; }

        U32 getNum() const { return mNum; }
        U32 getRetryCount() const { return mRetryCount; }
        void incRetryCount() { mRetryCount++; }
        Type getType() const { return mType; }
        F64 getScore() const { return mScore; }
        
        // Note: may return empty string!
        std::string getCapability() const;
        const char *getCapName() const;
        const char *getTypeAsString() const;
        
        // Re-enqueue thyself
        void reEnqueue();
        
        F32 getRetryTimerDelay() const;
        U32 getMaxNumRetries() const;
        
        bool isObjectValid() const { return mObject.notNull() && (!mObject->isDead()); }
        bool isNew() const { return isObjectValid() && mObject->isNew(); }
        void updateScore();
        
        void markDead();
        bool isDead();
        void startTracking();
        void stopTracking();
        
        friend std::ostream& operator<<(std::ostream &s, const Request &q);
        
        const LLUUID &getID() const { return mObjectID; }
        S32 getFace() const { return mFace; }
        
        bool isMatch (const Request::ptr_t &other, Type match_type = ANY) const 
        { 
            return ((match_type == ANY) || (mType == other->mType)) && 
                    (mFace == other->mFace) && 
                    (mObjectID == other->mObjectID); 
        }
    protected:
        LLMediaDataClientObject::ptr_t mObject;
    private:
        Type mType;
        // Simple tracking
        U32 mNum;
        static U32 sNum;
        U32 mRetryCount;
        F64 mScore;
        
        LLUUID mObjectID;
        S32 mFace;

        // Back pointer to the MDC...not a ref!
        LLMediaDataClient *mMDC;
    };
    //typedef LLPointer<Request> request_ptr_t;

    class Handler : public LLHttpSDHandler
    {
        LOG_CLASS(Handler);
    public:
        Handler(const Request::ptr_t &request);
        Request::ptr_t getRequest() const { return mRequest; }

    protected:
        virtual void onSuccess(LLCore::HttpResponse * response, const LLSD &content);
        virtual void onFailure(LLCore::HttpResponse * response, LLCore::HttpStatus status);

    private:
        Request::ptr_t mRequest;
    };


    class RetryTimer : public LLEventTimer
    {
    public:
        RetryTimer(F32 time, Request::ptr_t);
        virtual BOOL tick();
    private:
        // back-pointer
        Request::ptr_t mRequest;
    };
        
    
protected:
    typedef std::list<Request::ptr_t> request_queue_t;
    typedef std::set<Request::ptr_t> request_set_t;

    // Subclasses must override to return a cap name
    virtual const char *getCapabilityName() const = 0;

    // Puts the request into a queue, appropriately handling duplicates, etc.
    virtual void enqueue(Request::ptr_t) = 0;
    
    virtual void serviceQueue();
    virtual void serviceHttp();

    virtual request_queue_t *getQueue() { return &mQueue; };

    // Gets the next request, removing it from the queue
    virtual Request::ptr_t dequeue();
    
    virtual bool canServiceRequest(Request::ptr_t request) { return true; };

    // Returns a request to the head of the queue (should only be used for requests that came from dequeue
    virtual void pushBack(Request::ptr_t request);
    
    void trackRequest(Request::ptr_t request);
    void stopTrackingRequest(Request::ptr_t request);

    bool isDoneProcessing() const;
    
    request_queue_t mQueue;

    const F32 mQueueTimerDelay;
    const F32 mRetryTimerDelay;
    const U32 mMaxNumRetries;
    const U32 mMaxSortedQueueSize;
    const U32 mMaxRoundRobinQueueSize;
    
    // Set for keeping track of requests that aren't in either queue.  This includes:
    //  Requests that have been sent and are awaiting a response (pointer held by the Responder)
    //  Requests that are waiting for their retry timers to fire (pointer held by the retry timer)
    request_set_t mUnQueuedRequests;

    void startQueueTimer();
    void stopQueueTimer();

    LLCore::HttpRequest::ptr_t  mHttpRequest;
    LLCore::HttpHeaders::ptr_t  mHttpHeaders;
    LLCore::HttpOptions::ptr_t  mHttpOpts;
    LLCore::HttpRequest::policy_t mHttpPolicy;

private:
    
    static F64 getObjectScore(const LLMediaDataClientObject::ptr_t &obj);
    
    friend std::ostream& operator<<(std::ostream &s, const Request &q);
    friend std::ostream& operator<<(std::ostream &s, const request_queue_t &q);

    class QueueTimer : public LLEventTimer
    {
    public:
        QueueTimer(F32 time, LLMediaDataClient *mdc);
        virtual BOOL tick();
    private:
        // back-pointer
        LLPointer<LLMediaDataClient> mMDC;
    };
    
    void setIsRunning(bool val) { mQueueTimerIsRunning = val; }
        
    bool mQueueTimerIsRunning;

//  template <typename T> friend typename T::iterator find_matching_request(T &c, const LLMediaDataClient::Request *request, LLMediaDataClient::Request::Type match_type);
//  template <typename T> friend typename T::iterator find_matching_request(T &c, const LLUUID &id, LLMediaDataClient::Request::Type match_type);
//  template <typename T> friend void remove_matching_requests(T &c, const LLUUID &id, LLMediaDataClient::Request::Type match_type);
};

// MediaDataClient specific for the ObjectMedia cap
class LLObjectMediaDataClient : public LLMediaDataClient
{
protected:
    LOG_CLASS(LLObjectMediaDataClient);
public:
    LLObjectMediaDataClient(F32 queue_timer_delay = QUEUE_TIMER_DELAY,
                            F32 retry_timer_delay = UNAVAILABLE_RETRY_TIMER_DELAY,
                            U32 max_retries = MAX_RETRIES,
                            U32 max_sorted_queue_size = MAX_SORTED_QUEUE_SIZE,
                            U32 max_round_robin_queue_size = MAX_ROUND_ROBIN_QUEUE_SIZE)
        : LLMediaDataClient(queue_timer_delay, retry_timer_delay, max_retries),
          mCurrentQueueIsTheSortedQueue(true)
        {}
    
    void fetchMedia(LLMediaDataClientObject *object); 
    void updateMedia(LLMediaDataClientObject *object);

    class RequestGet: public Request
    {
    public:
        RequestGet(LLMediaDataClientObject *obj, LLMediaDataClient *mdc);
        /*virtual*/ LLSD getPayload() const;
        /*virtual*/ LLCore::HttpHandler::ptr_t createHandler();
    };

    class RequestUpdate: public Request
    {
    public:
        RequestUpdate(LLMediaDataClientObject *obj, LLMediaDataClient *mdc);
        /*virtual*/ LLSD getPayload() const;
        /*virtual*/ LLCore::HttpHandler::ptr_t createHandler();
    };

    // Returns true iff the queue is empty
    virtual bool isEmpty() const;
    
    // Returns true iff the given object is in the queue
    virtual bool isInQueue(const LLMediaDataClientObject::ptr_t &object);
        
    // Remove the given object from the queue. Returns true iff the given object is removed.
    virtual void removeFromQueue(const LLMediaDataClientObject::ptr_t &object);

    virtual bool processQueueTimer();

    virtual bool canServiceRequest(Request::ptr_t request);

protected:
    // Subclasses must override to return a cap name
    virtual const char *getCapabilityName() const;
    
    virtual request_queue_t *getQueue();

    // Puts the request into the appropriate queue
    virtual void enqueue(Request::ptr_t);
            
    class Handler: public LLMediaDataClient::Handler
    {
        LOG_CLASS(Handler);
    public:
        Handler(const Request::ptr_t &request):
            LLMediaDataClient::Handler(request)
        {}

    protected:
        virtual void onSuccess(LLCore::HttpResponse * response, const LLSD &content);
    };

private:
    // The Get/Update data client needs a second queue to avoid object updates starving load-ins.
    void swapCurrentQueue();
    
    request_queue_t mRoundRobinQueue;
    bool mCurrentQueueIsTheSortedQueue;

    // Comparator for sorting
    static bool compareRequestScores(const Request::ptr_t &o1, const Request::ptr_t &o2);
    void sortQueue();
};


// MediaDataClient specific for the ObjectMediaNavigate cap
class LLObjectMediaNavigateClient : public LLMediaDataClient
{
protected:
    LOG_CLASS(LLObjectMediaNavigateClient);
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
    
    void navigate(LLMediaDataClientObject *object, U8 texture_index, const std::string &url);

    // Puts the request into the appropriate queue
    virtual void enqueue(Request::ptr_t);

    class RequestNavigate: public Request
    {
    public:
        RequestNavigate(LLMediaDataClientObject *obj, LLMediaDataClient *mdc, U8 texture_index, const std::string &url);
        /*virtual*/ LLSD getPayload() const;
        /*virtual*/ LLCore::HttpHandler::ptr_t createHandler();
        /*virtual*/ std::string getURL() { return mURL; }
    private:
        std::string mURL;
    };
    
protected:
    // Subclasses must override to return a cap name
    virtual const char *getCapabilityName() const;

    class Handler : public LLMediaDataClient::Handler
    {
        LOG_CLASS(Handler);
    public:
        Handler(const Request::ptr_t &request):
            LLMediaDataClient::Handler(request)
        {}

    protected:
        virtual void onSuccess(LLCore::HttpResponse * response, const LLSD &content);
        virtual void onFailure(LLCore::HttpResponse * response, LLCore::HttpStatus status);

    private:
        void mediaNavigateBounceBack();
    };

};


#endif // LL_LLMEDIADATACLIENT_H
