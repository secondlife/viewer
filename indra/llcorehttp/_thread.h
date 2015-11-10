/** 
 * @file _thread.h
 * @brief thread type abstraction
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

#ifndef LLCOREINT_THREAD_H_
#define LLCOREINT_THREAD_H_

#include "linden_common.h"

#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "_refcounted.h"

namespace LLCoreInt
{

class HttpThread : public RefCounted
{
private:
	HttpThread();							// Not defined
	void operator=(const HttpThread &);		// Not defined

	void at_exit()
		{
			// the thread function has exited so we need to release our reference
			// to ourself so that we will be automagically cleaned up.
			release();
		}

	void run()
		{ // THREAD CONTEXT

			// Take out additional reference for the at_exit handler
			addRef();
			boost::this_thread::at_thread_exit(boost::bind(&HttpThread::at_exit, this));

			// run the thread function
			mThreadFunc(this);

		} // THREAD CONTEXT

protected:
	virtual ~HttpThread()
		{
			delete mThread;
		}

public:
	/// Constructs a thread object for concurrent execution but does
	/// not start running.  Caller receives on refcount on the thread
	/// instance.  If the thread is started, another will be taken
	/// out for the exit handler.
	explicit HttpThread(boost::function<void (HttpThread *)> threadFunc)
		: RefCounted(true), // implicit reference
		  mThreadFunc(threadFunc)
		{
			// this creates a boost thread that will call HttpThread::run on this instance
			// and pass it the threadfunc callable...
			boost::function<void()> f = boost::bind(&HttpThread::run, this);

			mThread = new boost::thread(f);
		}

	inline void join()
		{
			mThread->join();
		}

	inline bool timedJoin(S32 millis)
		{
			return mThread->timed_join(boost::posix_time::milliseconds(millis));
		}

	inline bool joinable() const
		{
			return mThread->joinable();
		}

	// A very hostile method to force a thread to quit
	inline void cancel()
		{
			boost::thread::native_handle_type thread(mThread->native_handle());
#if		LL_WINDOWS
			TerminateThread(thread, 0);
#else
			pthread_cancel(thread);
#endif
		}
	
private:
	boost::function<void(HttpThread *)> mThreadFunc;
	boost::thread * mThread;
}; // end class HttpThread

} // end namespace LLCoreInt

#endif // LLCOREINT_THREAD_H_


