/** 
 * @file lliopipe.h
 * @author Phoenix
 * @date 2004-11-18
 * @brief Declaration of base IO class
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLIOPIPE_H
#define LL_LLIOPIPE_H

#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "apr_poll.h"

#include "llsd.h"

class LLIOPipe;
class LLPumpIO;
class LLBufferArray;
class LLChannelDescriptors;

// Debugging schmutz for deadlocks
//#define LL_DEBUG_PUMPS
#ifdef LL_DEBUG_PUMPS
void pump_debug(const char *file, S32 line);
#define PUMP_DEBUG pump_debug(__FILE__, __LINE__);
#define END_PUMP_DEBUG pump_debug("none", 0);
#else /* LL_DEBUG_PUMPS */
#define PUMP_DEBUG
#define END_PUMP_DEBUG
#endif


/**
 * intrusive pointer support
 */
namespace boost
{
	void intrusive_ptr_add_ref(LLIOPipe* p);
	void intrusive_ptr_release(LLIOPipe* p);
};

/** 
 * @class LLIOPipe
 * @brief This class is an abstract base class for data processing units
 * @see LLPumpIO
 *
 * The LLIOPipe is a base class for implementing the basic non-blocking
 * processing of data subsystem in our system.
 *
 * Implementations of this class should behave like a stateful or
 * stateless signal processor. Each call to <code>process()</code>
 * hands the pipe implementation a buffer and a set of channels in the
 * buffer to process, and the pipe returns the status of the
 * operation. This is an abstract base class and developer created
 * concrete implementations provide block or stream based processing
 * of data to implement a particular protocol.
 */
class LLIOPipe
{
public:
	/** 
	 * @brief I have decided that IO objects should have a reference
	 * count. In general, you can pass bald LLIOPipe pointers around
	 * as you need, but if you need to maintain a reference to one,
	 * you need to hold a ptr_t.
	 */
	typedef boost::intrusive_ptr<LLIOPipe> ptr_t;

	/** 
	 * @brief Scattered memory container.
	 */
	typedef boost::shared_ptr<LLBufferArray> buffer_ptr_t;

	/** 
	 * @brief Enumeration for IO return codes
	 *
	 * A status code a positive integer value is considered a success,
	 * but may indicate special handling for future calls, for
	 * example, issuing a STATUS_STOP to an LLIOSocketReader instance
	 * will tell the instance to stop reading the socket. A status
	 * code with a negative value means that a problem has been
	 * encountered which will require further action on the caller or
	 * a developer to correct. Some mechanisms, such as the LLPumpIO
	 * may depend on this definition of success and failure.
	 */
	enum EStatus
	{
		// Processing occurred normally, future calls will be accepted.
		STATUS_OK = 0,

		// Processing occured normally, but stop unsolicited calls to
		// process.
		STATUS_STOP = 1,

		// This pipe is done with the processing. Future calls to
		// process will be accepted as long as new data is available.
		STATUS_DONE = 2,

		// This pipe is requesting that it become the head in a process.
		STATUS_BREAK = 3,

		// This pipe is requesting that it become the head in a process.
		STATUS_NEED_PROCESS = 4,

		// Keep track of the highest number of success codes here.
		STATUS_SUCCESS_COUNT = 5,

		// A generic error code.
		STATUS_ERROR = -1,

		// This method has not yet been implemented. This usually
		// indicates the programmer working on the pipe is not yet
		// done.
		STATUS_NOT_IMPLEMENTED = -2,

		// This indicates that a pipe precondition was not met. For
		// example, many pipes require an element to appear after them
		// in a chain (ie, mNext is not null) and will return this in
		// response to method calls. To recover from this, it will
		// require the caller to adjust the pipe state or may require
		// a dev to adjust the code to satisfy the preconditions.
		STATUS_PRECONDITION_NOT_MET = -3,

		// This means we could not connect to a remote host.
		STATUS_NO_CONNECTION = -4,

		// The connection was lost.
		STATUS_LOST_CONNECTION = -5,

		// The totoal process time has exceeded the timeout.
		STATUS_EXPIRED = -6,

		// Keep track of the count of codes here.
		STATUS_ERROR_COUNT = 6,
	};

	/** 
	 * @brief Helper function to check status.
	 *
	 * When writing code to check status codes, if you do not
	 * specifically check a particular value, use this method for
	 * checking an error condition.
	 * @param status The status to check.
	 * @return Returns true if the code indicates an error occurred.
	 */
	inline static bool isError(EStatus status)
	{
		return ((S32)status < 0);
	}

	/** 
	 * @brief Helper function to check status.
	 *
	 * When writing code to check status codes, if you do not
	 * specifically check a particular value, use this method for
	 * checking an error condition.
	 * @param status The status to check.
	 * @return Returns true if the code indicates no error was generated.
	 */
	inline static bool isSuccess(EStatus status)
	{
		return ((S32)status >= 0);
	}

	/** 
	 * @brief Helper function to turn status into a string.
	 *
	 * @param status The status to check.
	 * @return Returns the name of the status code or empty string on failure.
	 */
	static std::string lookupStatusString(EStatus status);

	/** 
	 * @brief Process the data in buffer.
	 *
	 * @param data The data processed
	 * @param eos True if this function call is the last because end of stream.
	 * @param pump The pump which is calling process. May be NULL.
	 * @param context Shared meta-data for the process.
	 * @return Returns a status code from the operation.
	 */
	EStatus process(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);

	/** 
	 * @brief Give this pipe a chance to handle a generated error
	 *
	 * If this pipe is in a chain being processed by a pump, and one
	 * of the pipes generates an error, the pump will rewind through
	 * the chain to see if any of the links can handle the error. For
	 * example, if a connection is refused in a socket connection, the
	 * socket client can try to find a new destination host. Return an
	 * error code if this pipe does not handle the error passed in.
	 * @param status The status code for the error
	 * @param pump The pump which was calling process before the error
	 * was generated.
	 * @return Returns a status code from the operation. Returns an
	 * error code if the error passed in was not handled. Returns
	 * STATUS_OK to indicate the error has been handled.
	 */
	virtual EStatus handleError(EStatus status, LLPumpIO* pump);

	/**
	 * @brief Base Destructor - do not call <code>delete</code> directly.
	 */
	virtual ~LLIOPipe();

	virtual bool isValid() ;

protected:
	/**
	 * @brief Base Constructor.
	 */
	LLIOPipe();

	/** 
	 * @brief Process the data in buffer
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump) = 0;

private:
	friend void boost::intrusive_ptr_add_ref(LLIOPipe* p);
	friend void boost::intrusive_ptr_release(LLIOPipe* p);
	U32 mReferenceCount;
};

namespace boost
{
	inline void intrusive_ptr_add_ref(LLIOPipe* p)
	{
		++p->mReferenceCount;
	}
	inline void intrusive_ptr_release(LLIOPipe* p)
	{
		if(p && 0 == --p->mReferenceCount)
		{
			delete p;
		}
	}
};


#if 0
/** 
 * @class LLIOBoiler
 * @brief This class helps construct new LLIOPipe specializations
 * @see LLIOPipe
 *
 * THOROUGH_DESCRIPTION
 */
class LLIOBoiler : public LLIOPipe
{
public:
	LLIOBoiler();
	virtual ~LLIOBoiler();

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data in buffer
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}
};

// virtual
LLIOPipe::EStatus process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	return STATUS_NOT_IMPLEMENTED;
}

#endif // #if 0 - use this block as a boilerplate

#endif // LL_LLIOPIPE_H
