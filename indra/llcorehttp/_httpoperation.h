/**
 * @file _httpoperation.h
 * @brief Internal declarations for HttpOperation and sub-classes
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef	_LLCORE_HTTP_OPERATION_H_
#define	_LLCORE_HTTP_OPERATION_H_


#include "httpcommon.h"

#include "_refcounted.h"


namespace LLCore
{

class HttpReplyQueue;
class HttpHandler;
class HttpService;
class HttpRequest;

/// HttpOperation is the base class for all request/reply
/// pairs.
///
/// Operations are expected to be of two types:  immediate
/// and queued.  Immediate requests go to the singleton
/// request queue and when picked up by the worker thread
/// are executed immediately and there results placed on
/// the supplied reply queue.  Queued requests (namely for
/// HTTP operations), go to the request queue, are picked
/// up and moved to a ready queue where they're ordered by
/// priority and managed by the policy component, are
/// then activated issuing HTTP requests and moved to an
/// active list managed by the transport (libcurl) component
/// and eventually finalized when a response is available
/// and status and data return via reply queue.
///
/// To manage these transitions, derived classes implement
/// three methods:  stageFromRequest, stageFromReady and
/// stageFromActive.  Immediate requests will only override
/// stageFromRequest which will perform the operation and
/// return the result by invoking addAsReply() to put the
/// request on a reply queue.  Queued requests will involve
/// all three stage methods.
///
/// Threading:  not thread-safe.  Base and derived classes
/// provide no locking.  Instances move across threads
/// via queue-like interfaces that are thread compatible
/// and those interfaces establish the access rules.

class HttpOperation : public LLCoreInt::RefCounted
{
public:
	HttpOperation();
	virtual ~HttpOperation();

private:
	HttpOperation(const HttpOperation &);				// Not defined
	void operator=(const HttpOperation &);				// Not defined

public:
	void setHandlers(HttpReplyQueue * reply_queue,
					 HttpHandler * lib_handler,
					 HttpHandler * user_handler);

	HttpHandler * getUserHandler() const
		{
			return mUserHandler;
		}
	
	virtual void stageFromRequest(HttpService *);
	virtual void stageFromReady(HttpService *);
	virtual void stageFromActive(HttpService *);

	virtual void visitNotifier(HttpRequest *);
	
	virtual HttpStatus cancel();
	
protected:
	void addAsReply();
	
protected:
	HttpReplyQueue *	mReplyQueue;					// Have refcount
	HttpHandler *		mLibraryHandler;				// Have refcount
	HttpHandler *		mUserHandler;					// Have refcount

public:
	unsigned int		mReqPolicy;
	unsigned int		mReqPriority;
	
};  // end class HttpOperation


/// HttpOpStop requests the servicing thread to shutdown
/// operations, cease pulling requests from the request
/// queue and release shared resources (particularly
/// those shared via reference count).  The servicing
/// thread will then exit.  The underlying thread object
/// remains so that another thread can join on the
/// servicing thread prior to final cleanup.  The
/// request *does* generate a reply on the response
/// queue, if requested.

class HttpOpStop : public HttpOperation
{
public:
	HttpOpStop();
	virtual ~HttpOpStop();

private:
	HttpOpStop(const HttpOpStop &);					// Not defined
	void operator=(const HttpOpStop &);				// Not defined

public:
	virtual void stageFromRequest(HttpService *);

};  // end class HttpOpStop


/// HttpOpNull is a do-nothing operation used for testing via
/// a basic loopback pattern.  It's executed immediately by
/// the servicing thread which bounces a reply back to the
/// caller without any further delay.

class HttpOpNull : public HttpOperation
{
public:
	HttpOpNull();
	virtual ~HttpOpNull();

private:
	HttpOpNull(const HttpOpNull &);					// Not defined
	void operator=(const HttpOpNull &);				// Not defined

public:
	virtual void stageFromRequest(HttpService *);

};  // end class HttpOpNull


/// HttpOpCompare isn't an operation but a uniform comparison
/// functor for STL containers that order by priority.  Mainly
/// used for the ready queue container but defined here.
class HttpOpCompare
{
public:
	bool operator()(const HttpOperation * lhs, const HttpOperation * rhs)
		{
			return lhs->mReqPriority > rhs->mReqPriority;
		}
};  // end class HttpOpCompare

	
}   // end namespace LLCore

#endif	// _LLCORE_HTTP_OPERATION_H_

