/** 
 * @file llpumpio.h
 * @author Phoenix
 * @date 2004-11-19
 * @brief Declaration of pump class which manages io chains.
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

#ifndef LL_LLPUMPIO_H
#define LL_LLPUMPIO_H

#include <set>
#if LL_LINUX  // needed for PATH_MAX in APR.
#include <sys/param.h>
#endif

#include "apr_pools.h"
#include "llbuffer.h"
#include "llframetimer.h"
#include "lliopipe.h"
#include "llrun.h"

// Define this to enable use with the APR thread library.
//#define LL_THREADS_APR 1

// some simple constants to help with timeouts
extern const F32 DEFAULT_CHAIN_EXPIRY_SECS;
extern const F32 SHORT_CHAIN_EXPIRY_SECS;
extern const F32 NEVER_CHAIN_EXPIRY_SECS;

/** 
 * @class LLPumpIO
 * @brief Class to manage sets of io chains.
 *
 * The pump class provides a thread abstraction for doing IO based
 * communication between two threads in a structured and optimized for
 * processor time. The primary usage is to create a pump, and call
 * <code>pump()</code> on a thread used for IO and call
 * <code>respond()</code> on a thread that is expected to do higher
 * level processing. You can call almost any other method from any
 * thread - see notes for each method for details. In order for the
 * threading abstraction to work, you need to call <code>prime()</code>
 * with a valid apr pool.
 * A pump instance manages much of the state for the pipe, including
 * the list of pipes in the chain, the channel for each element in the
 * chain, the buffer, and if any pipe has marked the stream or process
 * as done. Pipes can also set file descriptor based conditional
 * statements so that calls to process do not happen until data is
 * ready to be read or written.  Pipes control execution of calls to
 * process by returning a status code such as STATUS_OK or
 * STATUS_BREAK.
 * One way to conceptualize the way IO will work is that a pump
 * combines the unit processing of pipes to behave like file pipes on
 * the unix command line.
 */
class LLPumpIO
{
public:
	/**
	 * @brief Constructor.
	 */
	LLPumpIO(apr_pool_t* pool);

	/**
	 * @brief Destructor.
	 */
	~LLPumpIO();

	/**
	 * @brief Prepare this pump for usage.
	 *
	 * If you fail to call this method prior to use, the pump will
	 * try to work, but will not come with any thread locking
	 * mechanisms.
	 * @param pool The apr pool to use.
	 * @return Returns true if the pump is primed.
	 */
	bool prime(apr_pool_t* pool);

	/**
	 * @brief Typedef for having a chain of pipes.
	 */
	typedef std::vector<LLIOPipe::ptr_t> chain_t;

	/** 
	 * @brief Add a chain to this pump and process in the next cycle.
	 *
	 * This method will automatically generate a buffer and assign
	 * each link in the chain as if it were the consumer to the
	 * previous.
	 * @param chain The pipes for the chain
	 * @param timeout The number of seconds in the future to
	 * expire. Pass in 0.0f to never expire.
	 * @param has_curl_request The chain contains LLURLRequest if true.
	 * @return Returns true if anything was added to the pump.
	 */
	bool addChain(const chain_t& chain, F32 timeout, bool has_curl_request = false);
	
	/** 
	 * @brief Struct to associate a pipe with it's buffer io indexes.
	 */
	struct LLLinkInfo
	{
		LLIOPipe::ptr_t mPipe;
		LLChannelDescriptors mChannels;
	};

	/** 
	 * @brief Typedef for having a chain of <code>LLLinkInfo</code>
	 * instances.
	 */
	typedef std::vector<LLLinkInfo> links_t;

	/** 
	 * @brief Add a chain to this pump and process in the next cycle.
	 *
	 * This method provides a slightly more sophisticated method for
	 * adding a chain where the caller can specify which link elements
	 * are on what channels. This method will fail if no buffer is
	 * provided since any calls to generate new channels for the
	 * buffers will cause unpredictable interleaving of data.
	 * @param links The pipes and io indexes for the chain
	 * @param data Shared pointer to data buffer
	 * @param context Potentially undefined context meta-data for chain.
	 * @param timeout The number of seconds in the future to
	 * expire. Pass in 0.0f to never expire.
	 * @return Returns true if anything was added to the pump.
	 */
	bool addChain(
		const links_t& links,
		LLIOPipe::buffer_ptr_t data,
		LLSD context,
		F32 timeout);

	/** 
	 * @brief Set or clear a timeout for the running chain
	 *
	 * @param timeout The number of seconds in the future to
	 * expire. Pass in 0.0f to never expire.
	 * @return Returns true if the timer was set.
	 */
	bool setTimeoutSeconds(F32 timeout);

	/** 
	 * @brief Adjust the timeout of the running chain.
	 *
	 * This method has no effect if there is no timeout on the chain.
	 * @param delta The number of seconds to add to/remove from the timeout.
	 */
	void adjustTimeoutSeconds(F32 delta);

	/** 
	 * @brief Set up file descriptors for for the running chain.
	 * @see rebuildPollset()
	 *
	 * There is currently a limit of one conditional per pipe.
	 * *NOTE: The internal mechanism for building a pollset based on
	 * pipe/pollfd/chain generates an epoll error on linux (and
	 * probably behaves similarly on other platforms) because the
	 * pollset rebuilder will add each apr_pollfd_t serially. This
	 * does not matter for pipes on the same chain, since any
	 * signalled pipe will eventually invoke a call to process(), but
	 * is a problem if the same apr_pollfd_t is on different
	 * chains. Once we have more than just network i/o on the pump,
	 * this might matter.
	 * *FIX: Given the structure of the pump and pipe relationship,
	 * this should probably go through a different mechanism than the
	 * pump. I think it would be best if the pipe had some kind of
	 * controller which was passed into <code>process()</code> rather
	 * than the pump which exposed this interface.
	 * @param pipe The pipe which is setting a conditional
	 * @param poll The entire socket and read/write condition - null to remove
	 * @return Returns true if the poll state was set.
	 */
	bool setConditional(LLIOPipe* pipe, const apr_pollfd_t* poll);

	/** 
	 * @brief Lock the current chain.
	 * @see sleepChain() since it relies on the implementation of this method.
	 *
	 * This locks the currently running chain so that no more calls to
	 * <code>process()</code> until you call <code>clearLock()</code>
	 * with the lock identifier.
	 * *FIX: Given the structure of the pump and pipe relationship,
	 * this should probably go through a different mechanism than the
	 * pump. I think it would be best if the pipe had some kind of
	 * controller which was passed into <code>process()</code> rather
	 * than the pump which exposed this interface.
	 * @return Returns the lock identifer to be used in
	 * <code>clearLock()</code> or 0 on failure.
	 */
	S32 setLock();

	/** 
	 * @brief Clears the identified lock.
	 *
	 * @param links A container for the links which will be appended
	 */
	void clearLock(S32 key);

	/**
	 * @brief Stop processing a chain for a while.
	 * @see setLock()
	 *
	 * This method will <em>not</em> update the timeout for this
	 * chain, so it is possible to sleep the chain until it is
	 * collected by the pump during a timeout cleanup.
	 * @param seconds The number of seconds in the future to
	 * resume processing.
	 * @return Returns true if the 
	 */
	bool sleepChain(F64 seconds);

	/** 
	 * @brief Copy the currently running chain link info
	 *
	 * *FIX: Given the structure of the pump and pipe relationship,
	 * this should probably go through a different mechanism than the
	 * pump. I think it would be best if the pipe had some kind of
	 * controller which was passed into <code>process()</code> rather
	 * than the pump which exposed this interface.
	 * @param links A container for the links which will be appended
	 * @return Returns true if the currently running chain was copied.
	 */
	bool copyCurrentLinkInfo(links_t& links) const;

	/** 
	 * @brief Call this method to call process on all running chains.
	 *
	 * This method iterates through the running chains, and if all
	 * pipe on a chain are unconditionally ready or if any pipe has
	 * any conditional processiong condition then process will be
	 * called on every chain which has requested processing.  that
	 * chain has a file descriptor ready, <code>process()</code> will
	 * be called for all pipes which have requested it.
	 */
	void pump(const S32& poll_timeout);
	void pump();

	/** 
	 * @brief Add a chain to a special queue which will be called
	 * during the next call to <code>callback()</code> and then
	 * dropped from the queue.
	 *
	 * @param chain The IO chain that will get one <code>process()</code>.
	 */
	//void respond(const chain_t& pipes);

	/** 
	 * @brief Add pipe to a special queue which will be called
	 * during the next call to <code>callback()</code> and then dropped
	 * from the queue.
	 *
	 * This call will add a single pipe, with no buffer, context, or
	 * channel information to the callback queue. It will be called
	 * once, and then dropped.
	 * @param pipe A single io pipe which will be called
	 * @return Returns true if anything was added to the pump.
	 */
	bool respond(LLIOPipe* pipe);

	/** 
	 * @brief Add a chain to a special queue which will be called
	 * during the next call to <code>callback()</code> and then
	 * dropped from the queue.
	 *
	 * It is important to remember that you should not add a data
	 * buffer or context which may still be in another chain - that
	 * will almost certainly lead to a problems. Ensure that you are
	 * done reading and writing to those parameters, have new
	 * generated, or empty pointers. 
	 * @param links The pipes and io indexes for the chain
	 * @param data Shared pointer to data buffer
	 * @param context Potentially undefined context meta-data for chain.
	 * @return Returns true if anything was added to the pump.
	 */
	bool respond(
		const links_t& links,
		LLIOPipe::buffer_ptr_t data,
		LLSD context);

	/** 
	 * @brief Run through the callback queue and call <code>process()</code>.
	 *
	 * This call will process all prending responses and call process
	 * on each. This method will then drop all processed callback
	 * requests which may lead to deleting the referenced objects.
	 */
	void callback();

	/** 
	 * @brief Enumeration to send commands to the pump.
	 */
	enum EControl
	{
		PAUSE,
		RESUME,
	};
	
	/** 
	 * @brief Send a command to the pump.
	 *
	 * @param op What control to send to the pump.
	 */
	void control(EControl op);

protected:
	/** 
	 * @brief State of the pump
	 */
	enum EState
	{
		NORMAL,
		PAUSING,
		PAUSED
	};

	// instance data
	EState mState;
	bool mRebuildPollset;
	apr_pollset_t* mPollset;
	S32 mPollsetClientID;
	S32 mNextLock;
	std::set<S32> mClearLocks;

	// This is the pump's runnable scheduler used for handling
	// expiring locks.
	LLRunner mRunner;

	// This structure is the stuff we track while running chains.
	struct LLChainInfo
	{
		// methods
		LLChainInfo();
		void setTimeoutSeconds(F32 timeout);
		void adjustTimeoutSeconds(F32 delta);

		// basic member data
		bool mInit;
		bool mEOS;
		bool mHasCurlRequest;
		S32 mLock;
		LLFrameTimer mTimer;
		links_t::iterator mHead;
		links_t mChainLinks;
		LLIOPipe::buffer_ptr_t mData;		
		LLSD mContext;

		// tracking inside the pump
		typedef std::pair<LLIOPipe::ptr_t, apr_pollfd_t> pipe_conditional_t;
		typedef std::vector<pipe_conditional_t> conditionals_t;
		conditionals_t mDescriptors;
	};

	// All the running chains & info
 	typedef std::vector<LLChainInfo> pending_chains_t;
	pending_chains_t mPendingChains;
	typedef std::list<LLChainInfo> running_chains_t;
	running_chains_t mRunningChains;

	typedef running_chains_t::iterator current_chain_t;
	current_chain_t mCurrentChain;

	// structures necessary for doing callbacks
	// since the callbacks only get one chance to run, we do not have
	// to maintain a list.
	typedef std::vector<LLChainInfo> callbacks_t;
	callbacks_t mPendingCallbacks;
	callbacks_t mCallbacks;

	// memory allocator for pollsets & mutexes.
	apr_pool_t* mPool;
	apr_pool_t* mCurrentPool;
	S32 mCurrentPoolReallocCount;

#if LL_THREADS_APR
	apr_thread_mutex_t* mChainsMutex;
	apr_thread_mutex_t* mCallbackMutex;
#else
	int* mChainsMutex;
	int* mCallbackMutex;
#endif

protected:
	void initialize(apr_pool_t* pool);
	void cleanup();
	current_chain_t removeRunningChain(current_chain_t& chain) ;
	/** 
	 * @brief Given the internal state of the chains, rebuild the pollset
	 * @see setConditional()
	 */
	void rebuildPollset();

	/** 
	 * @brief Process the chain passed in.
	 *
	 * This method will potentially modify the internals of the
	 * chain. On end, the chain.mHead will equal
	 * chain.mChainLinks.end().
	 * @param chain The LLChainInfo object to work on.
	 */
	void processChain(LLChainInfo& chain);

	/** 
	 * @brief Rewind through the chain to try to recover from an error.
	 *
	 * This method will potentially modify the internals of the
	 * chain.
	 * @param chain The LLChainInfo object to work on.
	 * @return Retuns true if someone handled the error
	 */
	bool handleChainError(LLChainInfo& chain, LLIOPipe::EStatus error);

	//if the chain is expired, remove it
	bool isChainExpired(LLChainInfo& chain) ;

public:
	/** 
	 * @brief Return number of running chains.
	 *
	 * *NOTE: This is only used in debugging and not considered
	 * efficient or safe enough for production use.
	 */
	running_chains_t::size_type runningChains() const
	{
		return mRunningChains.size();
	}


};


#endif // LL_LLPUMPIO_H
