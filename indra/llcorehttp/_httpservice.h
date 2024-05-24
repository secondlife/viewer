/**
 * @file _httpservice.h
 * @brief Declarations for internal class providing HTTP service.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2013, Linden Research, Inc.
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

#ifndef _LLCORE_HTTP_SERVICE_H_
#define _LLCORE_HTTP_SERVICE_H_


#include <vector>

#include "linden_common.h"
#include "llatomic.h"
#include "httpcommon.h"
#include "httprequest.h"
#include "_httppolicyglobal.h"
#include "_httppolicyclass.h"


namespace LLCoreInt
{

class HttpThread;

}


namespace LLCore
{


class HttpRequestQueue;
class HttpPolicy;
class HttpLibcurl;
class HttpOpSetGet;


/// The HttpService class does the work behind the request queue.  It
/// oversees the HTTP workflow carrying out a number of tasks:
/// - Pulling requests from the global request queue
/// - Executing 'immediate' requests directly
/// - Prioritizing and re-queuing on internal queues the slower requests
/// - Providing cpu cycles to the libcurl plumbing
/// - Overseeing retry operations
///
/// Note that the service object doesn't have a pointer to any
/// reply queue.  These are kept by HttpRequest and HttpOperation
/// only.
///
/// Service, Policy and Transport
///
/// HttpService could have been a monolithic class combining a request
/// queue servicer, request policy manager and network transport.
/// Instead, to prevent monolithic growth and allow for easier
/// replacement, it was developed as three separate classes:  HttpService,
/// HttpPolicy and HttpLibcurl (transport).  These always exist in a
/// 1:1:1 relationship with HttpService managing instances of the other
/// two.  So, these classes do not use reference counting to refer
/// to one another, their lifecycles are always managed together.

class HttpService
{
protected:
    HttpService();
    virtual ~HttpService();

private:
    HttpService(const HttpService &);           // Not defined
    void operator=(const HttpService &);        // Not defined

public:
    enum EState
    {
        NOT_INITIALIZED = -1,
        INITIALIZED,            ///< init() has been called
        RUNNING,                ///< thread created and running
        STOPPED                 ///< thread has committed to exiting
    };

    // Ordered enumeration of idling strategies available to
    // threadRun's loop.  Ordered so that std::min on values
    // produces the most conservative result of multiple
    // requests.
    enum ELoopSpeed
    {
        NORMAL,                 ///< continuous polling of request, ready, active queues
        REQUEST_SLEEP           ///< can sleep indefinitely waiting for request queue write
    };

    static void init(HttpRequestQueue *);
    static void term();

    /// Threading:  callable by any thread once inited.
    inline static HttpService * instanceOf()
        {
            return sInstance;
        }

    /// Return the state of the worker thread.  Note that the
    /// transition from RUNNING to STOPPED is performed by the
    /// worker thread itself.  This has two weaknesses:
    /// - race where the thread hasn't really stopped but will
    /// - data ordering between threads where a non-worker thread
    ///   may see a stale RUNNING status.
    ///
    /// This transition is generally of interest only to unit tests
    /// and these weaknesses shouldn't be any real burden.
    ///
    /// Threading:  callable by any thread with above exceptions.
    static EState getState()
        {
            return sState;
        }

    /// Threading:  callable by any thread but uses @see getState() and
    /// acquires its weaknesses.
    static bool isStopped();

    /// Threading:  callable by init thread *once*.
    void startThread();

    /// Threading:  callable by worker thread.
    void stopRequested();

    /// Threading:  callable by worker thread.
    void shutdown();

    /// Try to find the given request handle on any of the request
    /// queues and cancel the operation.
    ///
    /// @return         True if the request was found and canceled.
    ///
    /// Threading:  callable by worker thread.
    bool cancel(HttpHandle handle);

    /// Threading:  callable by worker thread.
    HttpPolicy & getPolicy()
        {
            return *mPolicy;
        }

    /// Threading:  callable by worker thread.
    HttpLibcurl & getTransport()
        {
            return *mTransport;
        }

    /// Threading:  callable by worker thread.
    HttpRequestQueue & getRequestQueue()
        {
            return *mRequestQueue;
        }

    /// Threading:  callable by consumer thread.
    HttpRequest::policy_t createPolicyClass();

protected:
    void threadRun(LLCoreInt::HttpThread * thread);

    ELoopSpeed processRequestQueue(ELoopSpeed loop);

protected:
    friend class HttpOpSetGet;
    friend class HttpRequest;

    // Used internally to describe what operations are allowed
    // on each policy option.
    struct OptionDescriptor
    {
        bool        mIsLong;
        bool        mIsDynamic;
        bool        mIsGlobal;
        bool        mIsClass;
        bool        mIsCallback;
    };

    HttpStatus getPolicyOption(HttpRequest::EPolicyOption opt, HttpRequest::policy_t,
                               long * ret_value);
    HttpStatus getPolicyOption(HttpRequest::EPolicyOption opt, HttpRequest::policy_t,
                               std::string * ret_value);
    HttpStatus getPolicyOption(HttpRequest::EPolicyOption opt, HttpRequest::policy_t,
                                HttpRequest::policyCallback_t * ret_value);

    HttpStatus setPolicyOption(HttpRequest::EPolicyOption opt, HttpRequest::policy_t,
                               long value, long * ret_value);
    HttpStatus setPolicyOption(HttpRequest::EPolicyOption opt, HttpRequest::policy_t,
                               const std::string & value, std::string * ret_value);
    HttpStatus setPolicyOption(HttpRequest::EPolicyOption opt, HttpRequest::policy_t,
                                HttpRequest::policyCallback_t value,
                                HttpRequest::policyCallback_t * ret_value);

protected:
    static const OptionDescriptor       sOptionDesc[HttpRequest::PO_LAST];
    static HttpService *                sInstance;

    // === shared data ===
    static volatile EState              sState;
    HttpRequestQueue *                  mRequestQueue;  // Refcounted
    LLAtomicU32                         mExitRequested;
    LLCoreInt::HttpThread *             mThread;

    // === working-thread-only data ===
    HttpPolicy *                        mPolicy;        // Simple pointer, has ownership
    HttpLibcurl *                       mTransport;     // Simple pointer, has ownership

    // === main-thread-only data ===
    HttpRequest::policy_t               mLastPolicy;

};  // end class HttpService

}  // end namespace LLCore

#endif // _LLCORE_HTTP_SERVICE_H_
