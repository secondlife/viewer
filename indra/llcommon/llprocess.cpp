/** 
 * @file llprocess.cpp
 * @brief Utility class for launching, terminating, and tracking the state of processes.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "linden_common.h"
#include "llprocess.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "llsingleton.h"
#include "llstring.h"
#include "stringize.h"
#include "llapr.h"
#include "apr_signal.h"
#include "llevents.h"

#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <vector>
#include <typeinfo>
#include <utility>

/*****************************************************************************
*   Helpers
*****************************************************************************/
static const char* whichfile_[] = { "stdin", "stdout", "stderr" };
static std::string empty;
static LLProcess::Status interpret_status(int status);
static std::string getDesc(const LLProcess::Params& params);

static std::string whichfile(LLProcess::FILESLOT index)
{
	if (index < LL_ARRAY_SIZE(whichfile_))
		return whichfile_[index];
	return STRINGIZE("file slot " << index);
}

/**
 * Ref-counted "mainloop" listener. As long as there are still outstanding
 * LLProcess objects, keep listening on "mainloop" so we can keep polling APR
 * for process status.
 */
class LLProcessListener
{
	LOG_CLASS(LLProcessListener);
public:
	LLProcessListener():
		mCount(0)
	{}

	void addPoll(const LLProcess&)
	{
		// Unconditionally increment mCount. If it was zero before
		// incrementing, listen on "mainloop".
		if (mCount++ == 0)
		{
			LL_DEBUGS("LLProcess") << "listening on \"mainloop\"" << LL_ENDL;
			mConnection = LLEventPumps::instance().obtain("mainloop")
				.listen("LLProcessListener", boost::bind(&LLProcessListener::tick, this, _1));
		}
	}

	void dropPoll(const LLProcess&)
	{
		// Unconditionally decrement mCount. If it's zero after decrementing,
		// stop listening on "mainloop".
		if (--mCount == 0)
		{
			LL_DEBUGS("LLProcess") << "disconnecting from \"mainloop\"" << LL_ENDL;
			mConnection.disconnect();
		}
	}

private:
	/// called once per frame by the "mainloop" LLEventPump
	bool tick(const LLSD&)
	{
		// Tell APR to sense whether each registered LLProcess is still
		// running and call handle_status() appropriately. We should be able
		// to get the same info from an apr_proc_wait(APR_NOWAIT) call; but at
		// least in APR 1.4.2, testing suggests that even with APR_NOWAIT,
		// apr_proc_wait() blocks the caller. We can't have that in the
		// viewer. Hence the callback rigmarole. (Once we update APR, it's
		// probably worth testing again.) Also -- although there's an
		// apr_proc_other_child_refresh() call, i.e. get that information for
		// one specific child, it accepts an 'apr_other_child_rec_t*' that's
		// mentioned NOWHERE else in the documentation or header files! I
		// would use the specific call in LLProcess::getStatus() if I knew
		// how. As it is, each call to apr_proc_other_child_refresh_all() will
		// call callbacks for ALL still-running child processes. That's why we
		// centralize such calls, using "mainloop" to ensure it happens once
		// per frame, and refcounting running LLProcess objects to remain
		// registered only while needed.
		LL_DEBUGS("LLProcess") << "calling apr_proc_other_child_refresh_all()" << LL_ENDL;
		apr_proc_other_child_refresh_all(APR_OC_REASON_RUNNING);
		return false;
	}

	/// If this object is destroyed before mCount goes to zero, stop
	/// listening on "mainloop" anyway.
	LLTempBoundListener mConnection;
	unsigned mCount;
};
static LLProcessListener sProcessListener;

/*****************************************************************************
*   WritePipe and ReadPipe
*****************************************************************************/
LLProcess::BasePipe::~BasePipe() {}
const LLProcess::BasePipe::size_type
	  // use funky syntax to call max() to avoid blighted max() macros
	  LLProcess::BasePipe::npos((std::numeric_limits<LLProcess::BasePipe::size_type>::max)());

class WritePipeImpl: public LLProcess::WritePipe
{
	LOG_CLASS(WritePipeImpl);
public:
	WritePipeImpl(const std::string& desc, apr_file_t* pipe):
		mDesc(desc),
		mPipe(pipe),
		// Essential to initialize our std::ostream with our special streambuf!
		mStream(&mStreambuf)
	{
		mConnection = LLEventPumps::instance().obtain("mainloop")
			.listen(LLEventPump::inventName("WritePipe"),
					boost::bind(&WritePipeImpl::tick, this, _1));

#if ! LL_WINDOWS
		// We can't count on every child process reading everything we try to
		// write to it. And if the child terminates with WritePipe data still
		// pending, unless we explicitly suppress it, Posix will hit us with
		// SIGPIPE. That would terminate the viewer, boom. "Ignoring" it means
		// APR gets the correct errno, passes it back to us, we log it, etc.
		signal(SIGPIPE, SIG_IGN);
#endif
	}

	virtual std::ostream& get_ostream() { return mStream; }
	virtual size_type size() const { return mStreambuf.size(); }

	bool tick(const LLSD&)
	{
		typedef boost::asio::streambuf::const_buffers_type const_buffer_sequence;
		// If there's anything to send, try to send it.
		std::size_t total(mStreambuf.size()), consumed(0);
		if (total)
		{
			const_buffer_sequence bufs = mStreambuf.data();
			// In general, our streambuf might contain a number of different
			// physical buffers; iterate over those.
			bool keepwriting = true;
			for (const_buffer_sequence::const_iterator bufi(bufs.begin()), bufend(bufs.end());
				 bufi != bufend && keepwriting; ++bufi)
			{
				// http://www.boost.org/doc/libs/1_49_0_beta1/doc/html/boost_asio/reference/buffer.html#boost_asio.reference.buffer.accessing_buffer_contents
				// Although apr_file_write() accepts const void*, we
				// manipulate const char* so we can increment the pointer.
				const char* remainptr = boost::asio::buffer_cast<const char*>(*bufi);
				std::size_t remainlen = boost::asio::buffer_size(*bufi);
				while (remainlen)
				{
					// Tackle the current buffer in discrete chunks. On
					// Windows, we've observed strange failures when trying to
					// write big lengths (~1 MB) in a single operation. Even a
					// 32K chunk seems too large. At some point along the way
					// apr_file_write() returns 11 (Resource temporarily
					// unavailable, i.e. EAGAIN) and says it wrote 0 bytes --
					// even though it did write the chunk! Our next write
					// attempt retries with the same chunk, resulting in the
					// chunk being duplicated at the child end. Using smaller
					// chunks is empirically more reliable.
					std::size_t towrite((std::min)(remainlen, std::size_t(4*1024)));
					apr_size_t written(towrite);
					apr_status_t err = apr_file_write(mPipe, remainptr, &written);
					// EAGAIN is exactly what we want from a nonblocking pipe.
					// Rather than waiting for data, it should return immediately.
					if (! (err == APR_SUCCESS || APR_STATUS_IS_EAGAIN(err)))
					{
						LL_WARNS("LLProcess") << "apr_file_write(" << towrite << ") on " << mDesc
											  << " got " << err << ":" << LL_ENDL;
						ll_apr_warn_status(err);
					}

					// 'written' is modified to reflect the number of bytes actually
					// written. Make sure we consume those later. (Don't consume them
					// now, that would invalidate the buffer iterator sequence!)
					consumed += written;
					// don't forget to advance to next chunk of current buffer
					remainptr += written;
					remainlen -= written;

					char msgbuf[512];
					LL_DEBUGS("LLProcess") << "wrote " << written << " of " << towrite
										   << " bytes to " << mDesc
										   << " (original " << total << "),"
										   << " code " << err << ": "
										   << apr_strerror(err, msgbuf, sizeof(msgbuf))
										   << LL_ENDL;

					// The parent end of this pipe is nonblocking. If we weren't able
					// to write everything we wanted, don't keep banging on it -- that
					// won't change until the child reads some. Wait for next tick().
					if (written < towrite)
					{
						keepwriting = false; // break outer loop over buffers too
						break;
					}
				} // next chunk of current buffer
			}     // next buffer
			// In all, we managed to write 'consumed' bytes. Remove them from the
			// streambuf so we don't keep trying to send them. This could be
			// anywhere from 0 up to mStreambuf.size(); anything we haven't yet
			// sent, we'll try again later.
			mStreambuf.consume(consumed);
		}

		return false;
	}

private:
	std::string mDesc;
	apr_file_t* mPipe;
	LLTempBoundListener mConnection;
	boost::asio::streambuf mStreambuf;
	std::ostream mStream;
};

class ReadPipeImpl: public LLProcess::ReadPipe
{
	LOG_CLASS(ReadPipeImpl);
public:
	ReadPipeImpl(const std::string& desc, apr_file_t* pipe, LLProcess::FILESLOT index):
		mDesc(desc),
		mPipe(pipe),
		mIndex(index),
		// Essential to initialize our std::istream with our special streambuf!
		mStream(&mStreambuf),
		mPump("ReadPipe", true),    // tweak name as needed to avoid collisions
		mLimit(0),
		mEOF(false)
	{
		mConnection = LLEventPumps::instance().obtain("mainloop")
			.listen(LLEventPump::inventName("ReadPipe"),
					boost::bind(&ReadPipeImpl::tick, this, _1));
	}

	// Much of the implementation is simply connecting the abstract virtual
	// methods with implementation data concealed from the base class.
	virtual std::istream& get_istream() { return mStream; }
	virtual std::string getline() { return LLProcess::getline(mStream); }
	virtual LLEventPump& getPump() { return mPump; }
	virtual void setLimit(size_type limit) { mLimit = limit; }
	virtual size_type getLimit() const { return mLimit; }
	virtual size_type size() const { return mStreambuf.size(); }

	virtual std::string read(size_type len)
	{
		// Read specified number of bytes into a buffer.
		size_type readlen((std::min)(size(), len));
		// Formally, &buffer[0] is invalid for a vector of size() 0. Exit
		// early in that situation.
		if (! readlen)
			return "";
		// Make a buffer big enough.
		std::vector<char> buffer(readlen);
		mStream.read(&buffer[0], readlen);
		// Since we've already clamped 'readlen', we can think of no reason
		// why mStream.read() should read fewer than 'readlen' bytes.
		// Nonetheless, use the actual retrieved length.
		return std::string(&buffer[0], mStream.gcount());
	}

	virtual std::string peek(size_type offset=0, size_type len=npos) const
	{
		// Constrain caller's offset and len to overlap actual buffer content.
		std::size_t real_offset = (std::min)(mStreambuf.size(), std::size_t(offset));
		size_type	want_end	= (len == npos)? npos : (real_offset + len);
		std::size_t real_end	= (std::min)(mStreambuf.size(), std::size_t(want_end));
		boost::asio::streambuf::const_buffers_type cbufs = mStreambuf.data();
		return std::string(boost::asio::buffers_begin(cbufs) + real_offset,
						   boost::asio::buffers_begin(cbufs) + real_end);
	}

	virtual size_type find(const std::string& seek, size_type offset=0) const
	{
		// If we're passing a string of length 1, use find(char), which can
		// use an O(n) std::find() rather than the O(n^2) std::search().
		if (seek.length() == 1)
		{
			return find(seek[0], offset);
		}

		// If offset is beyond the whole buffer, can't even construct a valid
		// iterator range; can't possibly find the string we seek.
		if (offset > mStreambuf.size())
		{
			return npos;
		}

		boost::asio::streambuf::const_buffers_type cbufs = mStreambuf.data();
		boost::asio::buffers_iterator<boost::asio::streambuf::const_buffers_type>
			begin(boost::asio::buffers_begin(cbufs)),
			end	 (boost::asio::buffers_end(cbufs)),
			found(std::search(begin + offset, end, seek.begin(), seek.end()));
		return (found == end)? npos : (found - begin);
	}

	virtual size_type find(char seek, size_type offset=0) const
	{
		// If offset is beyond the whole buffer, can't even construct a valid
		// iterator range; can't possibly find the char we seek.
		if (offset > mStreambuf.size())
		{
			return npos;
		}

		boost::asio::streambuf::const_buffers_type cbufs = mStreambuf.data();
		boost::asio::buffers_iterator<boost::asio::streambuf::const_buffers_type>
			begin(boost::asio::buffers_begin(cbufs)),
			end	 (boost::asio::buffers_end(cbufs)),
			found(std::find(begin + offset, end, seek));
		return (found == end)? npos : (found - begin);
	}

	bool tick(const LLSD&)
	{
		// Once we've hit EOF, skip all the rest of this.
		if (mEOF)
			return false;

		typedef boost::asio::streambuf::mutable_buffers_type mutable_buffer_sequence;
		// Try, every time, to read into our streambuf. In fact, we have no
		// idea how much data the child might be trying to send: keep trying
		// until we're convinced we've temporarily exhausted the pipe.
		enum PipeState { RETRY, EXHAUSTED, CLOSED };
		PipeState state = RETRY;
		std::size_t committed(0);
		do
		{
			// attempt to read an arbitrary size
			mutable_buffer_sequence bufs = mStreambuf.prepare(4096);
			// In general, the mutable_buffer_sequence returned by prepare() might
			// contain a number of different physical buffers; iterate over those.
			std::size_t tocommit(0);
			for (mutable_buffer_sequence::const_iterator bufi(bufs.begin()), bufend(bufs.end());
				 bufi != bufend; ++bufi)
			{
				// http://www.boost.org/doc/libs/1_49_0_beta1/doc/html/boost_asio/reference/buffer.html#boost_asio.reference.buffer.accessing_buffer_contents
				std::size_t toread(boost::asio::buffer_size(*bufi));
				apr_size_t gotten(toread);
				apr_status_t err = apr_file_read(mPipe,
												 boost::asio::buffer_cast<void*>(*bufi),
												 &gotten);
				// EAGAIN is exactly what we want from a nonblocking pipe.
				// Rather than waiting for data, it should return immediately.
				if (! (err == APR_SUCCESS || APR_STATUS_IS_EAGAIN(err)))
				{
					// Handle EOF specially: it's part of normal-case processing.
					if (err == APR_EOF)
					{
						LL_DEBUGS("LLProcess") << "EOF on " << mDesc << LL_ENDL;
					}
					else
					{
						LL_WARNS("LLProcess") << "apr_file_read(" << toread << ") on " << mDesc
											  << " got " << err << ":" << LL_ENDL;
						ll_apr_warn_status(err);
					}
					// Either way, though, we won't need any more tick() calls.
					mConnection.disconnect();
					// Ignore any subsequent calls we might get anyway.
					mEOF = true;
					state = CLOSED; // also break outer retry loop
					break;
				}

				// 'gotten' was modified to reflect the number of bytes actually
				// received. Make sure we commit those later. (Don't commit them
				// now, that would invalidate the buffer iterator sequence!)
				tocommit += gotten;
				LL_DEBUGS("LLProcess") << "filled " << gotten << " of " << toread
									   << " bytes from " << mDesc << LL_ENDL;

				// The parent end of this pipe is nonblocking. If we weren't even
				// able to fill this buffer, don't loop to try to fill the next --
				// that won't change until the child writes more. Wait for next
				// tick().
				if (gotten < toread)
				{
					// break outer retry loop too
					state = EXHAUSTED;
					break;
				}
			}

			// Don't forget to "commit" the data!
			mStreambuf.commit(tocommit);
			committed += tocommit;

			// state is changed from RETRY when we can't fill any one buffer
			// of the mutable_buffer_sequence established by the current
			// prepare() call -- whether due to error or not enough bytes.
			// That is, if state is still RETRY, we've filled every physical
			// buffer in the mutable_buffer_sequence. In that case, for all we
			// know, the child might have still more data pending -- go for it!
		} while (state == RETRY);

		// Once we recognize that the pipe is closed, make one more call to
		// listener. The listener might be waiting for a particular substring
		// to arrive, or a particular length of data or something. The event
		// with "eof" == true announces that nothing further will arrive, so
		// use it or lose it.
		if (committed || state == CLOSED)
		{
			// If we actually received new data, publish it on our LLEventPump
			// as advertised. Constrain it by mLimit. But show listener the
			// actual accumulated buffer size, regardless of mLimit.
			size_type datasize((std::min)(mLimit, size_type(mStreambuf.size())));
			mPump.post(LLSDMap
					   ("data", peek(0, datasize))
					   ("len", LLSD::Integer(mStreambuf.size()))
					   ("slot", LLSD::Integer(mIndex))
					   ("name", whichfile(mIndex))
					   ("desc", mDesc)
					   ("eof", state == CLOSED));
		}

		return false;
	}

private:
	std::string mDesc;
	apr_file_t* mPipe;
	LLProcess::FILESLOT mIndex;
	LLTempBoundListener mConnection;
	boost::asio::streambuf mStreambuf;
	std::istream mStream;
	LLEventStream mPump;
	size_type mLimit;
	bool mEOF;
};

/*****************************************************************************
*   LLProcess itself
*****************************************************************************/
/// Need an exception to avoid constructing an invalid LLProcess object, but
/// internal use only
struct LLProcessError: public std::runtime_error
{
	LLProcessError(const std::string& msg): std::runtime_error(msg) {}
};

LLProcessPtr LLProcess::create(const LLSDOrParams& params)
{
	try
	{
		return LLProcessPtr(new LLProcess(params));
	}
	catch (const LLProcessError& e)
	{
		LL_WARNS("LLProcess") << e.what() << LL_ENDL;

		// If caller is requesting an event on process termination, send one
		// indicating bad launch. This may prevent someone waiting forever for
		// a termination post that can't arrive because the child never
		// started.
		if (params.postend.isProvided())
		{
			LLEventPumps::instance().obtain(params.postend)
				.post(LLSDMap
					  // no "id"
					  ("desc", getDesc(params))
					  ("state", LLProcess::UNSTARTED)
					  // no "data"
					  ("string", e.what())
					 );
		}

		return LLProcessPtr();
	}
}

/// Call an apr function returning apr_status_t. On failure, log warning and
/// throw LLProcessError mentioning the function call that produced that
/// result.
#define chkapr(func)                            \
	if (ll_apr_warn_status(func))				\
		throw LLProcessError(#func " failed")

LLProcess::LLProcess(const LLSDOrParams& params):
	mAutokill(params.autokill),
	mPipes(NSLOTS)
{
	// Hmm, when you construct a ptr_vector with a size, it merely reserves
	// space, it doesn't actually make it that big. Explicitly make it bigger.
	// Because of ptr_vector's odd semantics, have to push_back(0) the right
	// number of times! resize() wants to default-construct new BasePipe
	// instances, which fails because it's pure virtual. But because of the
	// constructor call, these push_back() calls should require no new
	// allocation.
	for (size_t i = 0; i < mPipes.capacity(); ++i)
		mPipes.push_back(0);

	if (! params.validateBlock(true))
	{
		throw LLProcessError(STRINGIZE("not launched: failed parameter validation\n"
									   << LLSDNotationStreamer(params)));
	}

	mPostend = params.postend;

	apr_procattr_t *procattr = NULL;
	chkapr(apr_procattr_create(&procattr, gAPRPoolp));

	// IQA-490, CHOP-900: On Windows, ask APR to jump through hoops to
	// constrain the set of handles passed to the child process. Before we
	// changed to APR, the Windows implementation of LLProcessLauncher called
	// CreateProcess(bInheritHandles=FALSE), meaning to pass NO open handles
	// to the child process. Now that we support pipes, though, we must allow
	// apr_proc_create() to pass bInheritHandles=TRUE. But without taking
	// special pains, that causes trouble in a number of ways, due to the fact
	// that the viewer is constantly opening and closing files -- most of
	// which CreateProcess() passes to every child process!
#if ! defined(APR_HAS_PROCATTR_CONSTRAIN_HANDLE_SET)
	// Our special preprocessor symbol isn't even defined -- wrong APR
	LL_WARNS("LLProcess") << "This version of APR lacks Linden "
						  << "apr_procattr_constrain_handle_set() extension" << LL_ENDL;
#else
	chkapr(apr_procattr_constrain_handle_set(procattr, 1));
#endif

	// For which of stdin, stdout, stderr should we create a pipe to the
	// child? In the viewer, there are only a couple viable
	// apr_procattr_io_set() alternatives: inherit the viewer's own stdxxx
	// handle (APR_NO_PIPE, e.g. for stdout, stderr), or create a pipe that's
	// blocking on the child end but nonblocking at the viewer end
	// (APR_CHILD_BLOCK).
	// Other major options could include explicitly creating a single APR pipe
	// and passing it as both stdout and stderr (apr_procattr_child_out_set(),
	// apr_procattr_child_err_set()), or accepting a filename, opening it and
	// passing that apr_file_t (simple <, >, 2> redirect emulation).
	std::vector<apr_int32_t> select;
	BOOST_FOREACH(const FileParam& fparam, params.files)
	{
		// Every iteration, we're going to append an item to 'select'. At the
		// top of the loop, its size() is, in effect, an index. Use that to
		// pick a string description for messages.
		std::string which(whichfile(FILESLOT(select.size())));
		if (fparam.type().empty())  // inherit our file descriptor
		{
			select.push_back(APR_NO_PIPE);
		}
		else if (fparam.type() == "pipe") // anonymous pipe
		{
			if (! fparam.name().empty())
			{
				LL_WARNS("LLProcess") << "For " << params.executable()
									  << ": internal names for reusing pipes ('"
									  << fparam.name() << "' for " << which
									  << ") are not yet supported -- creating distinct pipe"
									  << LL_ENDL;
			}
			// The viewer can't block for anything: the parent end MUST be
			// nonblocking. As the APR documentation itself points out, it
			// makes very little sense to set nonblocking I/O for the child
			// end of a pipe: only a specially-written child could deal with
			// that.
			select.push_back(APR_CHILD_BLOCK);
		}
		else
		{
			throw LLProcessError(STRINGIZE("For " << params.executable()
										   << ": unsupported FileParam for " << which
										   << ": type='" << fparam.type()
										   << "', name='" << fparam.name() << "'"));
		}
	}
	// By default, pass APR_NO_PIPE for unspecified slots.
	while (select.size() < NSLOTS)
	{
		select.push_back(APR_NO_PIPE);
	}
	chkapr(apr_procattr_io_set(procattr, select[STDIN], select[STDOUT], select[STDERR]));

	// Thumbs down on implicitly invoking the shell to invoke the child. From
	// our point of view, the other major alternative to APR_PROGRAM_PATH
	// would be APR_PROGRAM_ENV: still copy environment, but require full
	// executable pathname. I don't see a downside to searching the PATH,
	// though: if our caller wants (e.g.) a specific Python interpreter, s/he
	// can still pass the full pathname.
	chkapr(apr_procattr_cmdtype_set(procattr, APR_PROGRAM_PATH));
	// YES, do extra work if necessary to report child exec() failures back to
	// parent process.
	chkapr(apr_procattr_error_check_set(procattr, 1));
	// Do not start a non-autokill child in detached state. On Posix
	// platforms, this setting attempts to daemonize the new child, closing
	// std handles and the like, and that's a bit more detachment than we
	// want. autokill=false just means not to implicitly kill the child when
	// the parent terminates!
//	chkapr(apr_procattr_detach_set(procattr, params.autokill? 0 : 1));

	if (params.autokill)
	{
#if ! defined(APR_HAS_PROCATTR_AUTOKILL_SET)
		// Our special preprocessor symbol isn't even defined -- wrong APR
		LL_WARNS("LLProcess") << "This version of APR lacks Linden apr_procattr_autokill_set() extension" << LL_ENDL;
#elif ! APR_HAS_PROCATTR_AUTOKILL_SET
		// Symbol is defined, but to 0: expect apr_procattr_autokill_set() to
		// return APR_ENOTIMPL.
#else   // APR_HAS_PROCATTR_AUTOKILL_SET nonzero
		ll_apr_warn_status(apr_procattr_autokill_set(procattr, 1));
#endif
	}

	// In preparation for calling apr_proc_create(), we collect a number of
	// const char* pointers obtained from std::string::c_str(). Turns out
	// LLInitParam::Block's helpers Optional, Mandatory, Multiple et al.
	// guarantee that converting to the wrapped type (std::string in our
	// case), e.g. by calling operator(), returns a reference to *the same
	// instance* of the wrapped type that's stored in our Block subclass.
	// That's important! We know 'params' persists throughout this method
	// call; but without that guarantee, we'd have to assume that converting
	// one of its members to std::string might return a different (temp)
	// instance. Capturing the c_str() from a temporary std::string is Bad Bad
	// Bad. But armed with this knowledge, when you see params.cwd().c_str(),
	// grit your teeth and smile and carry on.

	if (params.cwd.isProvided())
	{
		chkapr(apr_procattr_dir_set(procattr, params.cwd().c_str()));
	}

	// create an argv vector for the child process
	std::vector<const char*> argv;

	// Add the executable path. See above remarks about c_str().
	argv.push_back(params.executable().c_str());

	// Add arguments. See above remarks about c_str().
	BOOST_FOREACH(const std::string& arg, params.args)
	{
		argv.push_back(arg.c_str());
	}

	// terminate with a null pointer
	argv.push_back(NULL);

	// Launch! The NULL would be the environment block, if we were passing
	// one. Hand-expand chkapr() macro so we can fill in the actual command
	// string instead of the variable names.
	if (ll_apr_warn_status(apr_proc_create(&mProcess, argv[0], &argv[0], NULL, procattr,
										   gAPRPoolp)))
	{
		throw LLProcessError(STRINGIZE(params << " failed"));
	}

	// arrange to call status_callback()
	apr_proc_other_child_register(&mProcess, &LLProcess::status_callback, this, mProcess.in,
								  gAPRPoolp);
	// and make sure we poll it once per "mainloop" tick
	sProcessListener.addPoll(*this);
	mStatus.mState = RUNNING;

	mDesc = STRINGIZE(getDesc(params) << " (" << mProcess.pid << ')');
	LL_INFOS("LLProcess") << mDesc << ": launched " << params << LL_ENDL;

	// Unless caller explicitly turned off autokill (child should persist),
	// take steps to terminate the child. This is all suspenders-and-belt: in
	// theory our destructor should kill an autokill child, but in practice
	// that doesn't always work (e.g. VWR-21538).
	if (params.autokill)
	{
/*==========================================================================*|
		// NO: There may be an APR bug, not sure -- but at least on Mac, when
		// gAPRPoolp is destroyed, OUR process receives SIGTERM! Apparently
		// either our own PID is getting into the list of processes to kill()
		// (unlikely), or somehow one of those PIDs is getting zeroed first,
		// so that kill() sends SIGTERM to the whole process group -- this
		// process included. I'd have to build and link with a debug version
		// of APR to know for sure. It's too bad: this mechanism would be just
		// right for dealing with static autokill LLProcessPtr variables,
		// which aren't destroyed until after APR is no longer available.

		// Tie the lifespan of this child process to the lifespan of our APR
		// pool: on destruction of the pool, forcibly kill the process. Tell
		// APR to try SIGTERM and wait 3 seconds. If that didn't work, use
		// SIGKILL.
		apr_pool_note_subprocess(gAPRPoolp, &mProcess, APR_KILL_AFTER_TIMEOUT);
|*==========================================================================*/

		// On Windows, associate the new child process with our Job Object.
		autokill();
	}

	// Instantiate the proper pipe I/O machinery
	// want to be able to point to apr_proc_t::in, out, err by index
	typedef apr_file_t* apr_proc_t::*apr_proc_file_ptr;
	static apr_proc_file_ptr members[] =
		{ &apr_proc_t::in, &apr_proc_t::out, &apr_proc_t::err };
	for (size_t i = 0; i < NSLOTS; ++i)
	{
		if (select[i] != APR_CHILD_BLOCK)
			continue;
		std::string desc(STRINGIZE(mDesc << ' ' << whichfile(FILESLOT(i))));
		apr_file_t* pipe(mProcess.*(members[i]));
		if (i == STDIN)
		{
			mPipes.replace(i, new WritePipeImpl(desc, pipe));
		}
		else
		{
			mPipes.replace(i, new ReadPipeImpl(desc, pipe, FILESLOT(i)));
		}
		LL_DEBUGS("LLProcess") << "Instantiating " << typeid(mPipes[i]).name()
							   << "('" << desc << "')" << LL_ENDL;
	}
}

// Helper to obtain a description string, given a Params block
static std::string getDesc(const LLProcess::Params& params)
{
	// If caller specified a description string, by all means use it.
	if (params.desc.isProvided())
		return params.desc;

	// Caller didn't say. Use the executable name -- but use just the filename
	// part. On Mac, for instance, full pathnames get cumbersome.
	return LLProcess::basename(params.executable);
}

//static
std::string LLProcess::basename(const std::string& path)
{
	// If there are Linden utility functions to manipulate pathnames, I
	// haven't found them -- and for this usage, Boost.Filesystem seems kind
	// of heavyweight.
	std::string::size_type delim = path.find_last_of("\\/");
	// If path contains no pathname delimiters, return the whole thing.
	if (delim == std::string::npos)
		return path;

	// Return just the part beyond the last delimiter.
	return path.substr(delim + 1);
}

LLProcess::~LLProcess()
{
	// In the Linden viewer, there's at least one static LLProcessPtr. Its
	// destructor will be called *after* ll_cleanup_apr(). In such a case,
	// unregistering is pointless (and fatal!) -- and kill(), which also
	// relies on APR, is impossible.
	if (! gAPRPoolp)
		return;

	// Only in state RUNNING are we registered for callback. In UNSTARTED we
	// haven't yet registered. And since receiving the callback is the only
	// way we detect child termination, we only change from state RUNNING at
	// the same time we unregister.
	if (mStatus.mState == RUNNING)
	{
		// We're still registered for a callback: unregister. Do it before
		// we even issue the kill(): even if kill() somehow prompted an
		// instantaneous callback (unlikely), this object is going away! Any
		// information updated in this object by such a callback is no longer
		// available to any consumer anyway.
		apr_proc_other_child_unregister(this);
		// One less LLProcess to poll for
		sProcessListener.dropPoll(*this);
	}

	if (mAutokill)
	{
		kill("destructor");
	}
}

bool LLProcess::kill(const std::string& who)
{
	if (isRunning())
	{
		LL_INFOS("LLProcess") << who << " killing " << mDesc << LL_ENDL;

#if LL_WINDOWS
		int sig = -1;
#else  // Posix
		int sig = SIGTERM;
#endif

		ll_apr_warn_status(apr_proc_kill(&mProcess, sig));
	}

	return ! isRunning();
}

//static
bool LLProcess::kill(const LLProcessPtr& p, const std::string& who)
{
	if (! p)
		return true;                // process dead! (was never running)
	return p->kill(who);
}

bool LLProcess::isRunning() const
{
	return getStatus().mState == RUNNING;
}

//static
bool LLProcess::isRunning(const LLProcessPtr& p)
{
	if (! p)
		return false;
	return p->isRunning();
}

LLProcess::Status LLProcess::getStatus() const
{
	return mStatus;
}

//static
LLProcess::Status LLProcess::getStatus(const LLProcessPtr& p)
{
	if (! p)
	{
		// default-constructed Status has mState == UNSTARTED
		return Status();
	}
	return p->getStatus();
}

std::string LLProcess::getStatusString() const
{
	return getStatusString(getStatus());
}

std::string LLProcess::getStatusString(const Status& status) const
{
	return getStatusString(mDesc, status);
}

//static
std::string LLProcess::getStatusString(const std::string& desc, const LLProcessPtr& p)
{
	if (! p)
	{
		// default-constructed Status has mState == UNSTARTED
		return getStatusString(desc, Status());
	}
	return desc + " " + p->getStatusString();
}

//static
std::string LLProcess::getStatusString(const std::string& desc, const Status& status)
{
	if (status.mState == UNSTARTED)
		return desc + " was never launched";

	if (status.mState == RUNNING)
		return desc + " running";

	if (status.mState == EXITED)
		return STRINGIZE(desc << " exited with code " << status.mData);

	if (status.mState == KILLED)
#if LL_WINDOWS
		return STRINGIZE(desc << " killed with exception " << std::hex << status.mData);
#else
		return STRINGIZE(desc << " killed by signal " << status.mData
						 << " (" << apr_signal_description_get(status.mData) << ")");
#endif

	return STRINGIZE(desc << " in unknown state " << status.mState << " (" << status.mData << ")");
}

// Classic-C-style APR callback
void LLProcess::status_callback(int reason, void* data, int status)
{
	// Our only role is to bounce this static method call back into object
	// space.
	static_cast<LLProcess*>(data)->handle_status(reason, status);
}

#define tabent(symbol) { symbol, #symbol }
static struct ReasonCode
{
	int code;
	const char* name;
} reasons[] =
{
	tabent(APR_OC_REASON_DEATH),
	tabent(APR_OC_REASON_UNWRITABLE),
	tabent(APR_OC_REASON_RESTART),
	tabent(APR_OC_REASON_UNREGISTER),
	tabent(APR_OC_REASON_LOST),
	tabent(APR_OC_REASON_RUNNING)
};
#undef tabent

// Object-oriented callback
void LLProcess::handle_status(int reason, int status)
{
	{
		// This odd appearance of LL_DEBUGS is just to bracket a lookup that will
		// only be performed if in fact we're going to produce the log message.
		LL_DEBUGS("LLProcess") << empty;
		std::string reason_str;
		BOOST_FOREACH(const ReasonCode& rcp, reasons)
		{
			if (reason == rcp.code)
			{
				reason_str = rcp.name;
				break;
			}
		}
		if (reason_str.empty())
		{
			reason_str = STRINGIZE("unknown reason " << reason);
		}
		LL_CONT << mDesc << ": handle_status(" << reason_str << ", " << status << ")" << LL_ENDL;
	}

	if (! (reason == APR_OC_REASON_DEATH || reason == APR_OC_REASON_LOST))
	{
		// We're only interested in the call when the child terminates.
		return;
	}

	// Somewhat oddly, APR requires that you explicitly unregister even when
	// it already knows the child has terminated. We must pass the same 'data'
	// pointer as for the register() call, which was our 'this'.
	apr_proc_other_child_unregister(this);
	// don't keep polling for a terminated process
	sProcessListener.dropPoll(*this);
	// We overload mStatus.mState to indicate whether the child is registered
	// for APR callback: only RUNNING means registered. Track that we've
	// unregistered. We know the child has terminated; might be EXITED or
	// KILLED; refine below.
	mStatus.mState = EXITED;

	// Make last-gasp calls for each of the ReadPipes we have on hand. Since
	// they're listening on "mainloop", we can be sure they'll eventually
	// collect all pending data from the child. But we want to be able to
	// guarantee to our consumer that by the time we post on the "postend"
	// LLEventPump, our ReadPipes are already buffering all the data there
	// will ever be from the child. That lets the "postend" listener decide
	// what to do with that final data.
	for (size_t i = 0; i < mPipes.size(); ++i)
	{
		std::string error;
		ReadPipeImpl* ppipe = getPipePtr<ReadPipeImpl>(error, FILESLOT(i));
		if (ppipe)
		{
			static LLSD trivial;
			ppipe->tick(trivial);
		}
	}

//	wi->rv = apr_proc_wait(wi->child, &wi->rc, &wi->why, APR_NOWAIT);
	// It's just wrong to call apr_proc_wait() here. The only way APR knows to
	// call us with APR_OC_REASON_DEATH is that it's already reaped this child
	// process, so calling wait() will only produce "huh?" from the OS. We
	// must rely on the status param passed in, which unfortunately comes
	// straight from the OS wait() call, which means we have to decode it by
	// hand.
	mStatus = interpret_status(status);
	LL_INFOS("LLProcess") << getStatusString() << LL_ENDL;

	// If caller requested notification on child termination, send it.
	if (! mPostend.empty())
	{
		LLEventPumps::instance().obtain(mPostend)
			.post(LLSDMap
				  ("id",     getProcessID())
				  ("desc",   mDesc)
				  ("state",  mStatus.mState)
				  ("data",   mStatus.mData)
				  ("string", getStatusString())
				 );
	}
}

LLProcess::id LLProcess::getProcessID() const
{
	return mProcess.pid;
}

LLProcess::handle LLProcess::getProcessHandle() const
{
#if LL_WINDOWS
	return mProcess.hproc;
#else
	return mProcess.pid;
#endif
}

std::string LLProcess::getPipeName(FILESLOT) const
{
	// LLProcess::FileParam::type "npipe" is not yet implemented
	return "";
}

template<class PIPETYPE>
PIPETYPE* LLProcess::getPipePtr(std::string& error, FILESLOT slot)
{
	if (slot >= NSLOTS)
	{
		error = STRINGIZE(mDesc << " has no slot " << slot);
		return NULL;
	}
	if (mPipes.is_null(slot))
	{
		error = STRINGIZE(mDesc << ' ' << whichfile(slot) << " not a monitored pipe");
		return NULL;
	}
	// Make sure we dynamic_cast in pointer domain so we can test, rather than
	// accepting runtime's exception.
	PIPETYPE* ppipe = dynamic_cast<PIPETYPE*>(&mPipes[slot]);
	if (! ppipe)
	{
		error = STRINGIZE(mDesc << ' ' << whichfile(slot) << " not a " << typeid(PIPETYPE).name());
		return NULL;
	}

	error.clear();
	return ppipe;
}

template <class PIPETYPE>
PIPETYPE& LLProcess::getPipe(FILESLOT slot)
{
	std::string error;
	PIPETYPE* wp = getPipePtr<PIPETYPE>(error, slot);
	if (! wp)
	{
		throw NoPipe(error);
	}
	return *wp;
}

template <class PIPETYPE>
boost::optional<PIPETYPE&> LLProcess::getOptPipe(FILESLOT slot)
{
	std::string error;
	PIPETYPE* wp = getPipePtr<PIPETYPE>(error, slot);
	if (! wp)
	{
		LL_DEBUGS("LLProcess") << error << LL_ENDL;
		return boost::optional<PIPETYPE&>();
	}
	return *wp;
}

LLProcess::WritePipe& LLProcess::getWritePipe(FILESLOT slot)
{
	return getPipe<WritePipe>(slot);
}

boost::optional<LLProcess::WritePipe&> LLProcess::getOptWritePipe(FILESLOT slot)
{
	return getOptPipe<WritePipe>(slot);
}

LLProcess::ReadPipe& LLProcess::getReadPipe(FILESLOT slot)
{
	return getPipe<ReadPipe>(slot);
}

boost::optional<LLProcess::ReadPipe&> LLProcess::getOptReadPipe(FILESLOT slot)
{
	return getOptPipe<ReadPipe>(slot);
}

//static
std::string LLProcess::getline(std::istream& in)
{
	std::string line;
	std::getline(in, line);
	// Blur the distinction between "\r\n" and plain "\n". std::getline() will
	// have eaten the "\n", but we could still end up with a trailing "\r".
	std::string::size_type lastpos = line.find_last_not_of("\r");
	if (lastpos != std::string::npos)
	{
		// Found at least one character that's not a trailing '\r'. SKIP OVER
		// IT and erase the rest of the line.
		line.erase(lastpos+1);
	}
	return line;
}

std::ostream& operator<<(std::ostream& out, const LLProcess::Params& params)
{
	if (params.cwd.isProvided())
	{
		out << "cd " << LLStringUtil::quote(params.cwd) << ": ";
	}
	out << LLStringUtil::quote(params.executable);
	BOOST_FOREACH(const std::string& arg, params.args)
	{
		out << ' ' << LLStringUtil::quote(arg);
	}
	return out;
}

/*****************************************************************************
*   Windows specific
*****************************************************************************/
#if LL_WINDOWS

static std::string WindowsErrorString(const std::string& operation);

void LLProcess::autokill()
{
	// hopefully now handled by apr_procattr_autokill_set()
}

LLProcess::handle LLProcess::isRunning(handle h, const std::string& desc)
{
	// This direct Windows implementation is because we have no access to the
	// apr_proc_t struct: we expect it's been destroyed.
	if (! h)
		return 0;

	DWORD waitresult = WaitForSingleObject(h, 0);
	if(waitresult == WAIT_OBJECT_0)
	{
		// the process has completed.
		if (! desc.empty())
		{
			DWORD status = 0;
			if (! GetExitCodeProcess(h, &status))
			{
				LL_WARNS("LLProcess") << desc << " terminated, but "
									  << WindowsErrorString("GetExitCodeProcess()") << LL_ENDL;
			}
			{
				LL_INFOS("LLProcess") << getStatusString(desc, interpret_status(status))
									  << LL_ENDL;
			}
		}
		CloseHandle(h);
		return 0;
	}

	return h;
}

static LLProcess::Status interpret_status(int status)
{
	LLProcess::Status result;

	// This bit of code is cribbed from apr/threadproc/win32/proc.c, a
	// function (unfortunately static) called why_from_exit_code():
	/* See WinNT.h STATUS_ACCESS_VIOLATION and family for how
	 * this class of failures was determined
	 */
	if ((status & 0xFFFF0000) == 0xC0000000)
	{
		result.mState = LLProcess::KILLED;
	}
	else
	{
		result.mState = LLProcess::EXITED;
	}
	result.mData = status;

	return result;
}

/// GetLastError()/FormatMessage() boilerplate
static std::string WindowsErrorString(const std::string& operation)
{
	int result = GetLastError();

	LPTSTR error_str = 0;
	if (FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					   NULL,
					   result,
					   0,
					   (LPTSTR)&error_str,
					   0,
					   NULL)
		!= 0) 
	{
		// convert from wide-char string to multi-byte string
		char message[256];
		wcstombs(message, error_str, sizeof(message));
		message[sizeof(message)-1] = 0;
		LocalFree(error_str);
		// convert to std::string to trim trailing whitespace
		std::string mbsstr(message);
		mbsstr.erase(mbsstr.find_last_not_of(" \t\r\n"));
		return STRINGIZE(operation << " failed (" << result << "): " << mbsstr);
	}
	return STRINGIZE(operation << " failed (" << result
					 << "), but FormatMessage() did not explain");
}

/*****************************************************************************
*   Posix specific
*****************************************************************************/
#else // Mac and linux

#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

void LLProcess::autokill()
{
	// What we ought to do here is to:
	// 1. create a unique process group and run all autokill children in that
	//    group (see https://jira.secondlife.com/browse/SWAT-563);
	// 2. figure out a way to intercept control when the viewer exits --
	//    gracefully or not; 
	// 3. when the viewer exits, kill off the aforementioned process group.

	// It's point 2 that's troublesome. Although I've seen some signal-
	// handling logic in the Posix viewer code, I haven't yet found any bit of
	// code that's run no matter how the viewer exits (a try/finally for the
	// whole process, as it were).
}

// Attempt to reap a process ID -- returns true if the process has exited and been reaped, false otherwise.
static bool reap_pid(pid_t pid, LLProcess::Status* pstatus=NULL)
{
	LLProcess::Status dummy;
	if (! pstatus)
	{
		// If caller doesn't want to see Status, give us a target anyway so we
		// don't have to have a bunch of conditionals.
		pstatus = &dummy;
	}

	int status = 0;
	pid_t wait_result = ::waitpid(pid, &status, WNOHANG);
	if (wait_result == pid)
	{
		*pstatus = interpret_status(status);
		return true;
	}
	if (wait_result == 0)
	{
		pstatus->mState = LLProcess::RUNNING;
		pstatus->mData	= 0;
		return false;
	}

	// Clear caller's Status block; caller must interpret UNSTARTED to mean
	// "if this PID was ever valid, it no longer is."
	*pstatus = LLProcess::Status();

	// We've dealt with the success cases: we were able to reap the child
	// (wait_result == pid) or it's still running (wait_result == 0). It may
	// be that the child terminated but didn't hang around long enough for us
	// to reap. In that case we still have no Status to report, but we can at
	// least state that it's not running.
	if (wait_result == -1 && errno == ECHILD)
	{
		// No such process -- this may mean we're ignoring SIGCHILD.
		return true;
	}

	// Uh, should never happen?!
	LL_WARNS("LLProcess") << "LLProcess::reap_pid(): waitpid(" << pid << ") returned "
						  << wait_result << "; not meaningful?" << LL_ENDL;
	// If caller is looping until this pid terminates, and if we can't find
	// out, better to break the loop than to claim it's still running.
	return true;
}

LLProcess::id LLProcess::isRunning(id pid, const std::string& desc)
{
	// This direct Posix implementation is because we have no access to the
	// apr_proc_t struct: we expect it's been destroyed.
	if (! pid)
		return 0;

	// Check whether the process has exited, and reap it if it has.
	LLProcess::Status status;
	if(reap_pid(pid, &status))
	{
		// the process has exited.
		if (! desc.empty())
		{
			std::string statstr(desc + " apparently terminated: no status available");
			// We don't just pass UNSTARTED to getStatusString() because, in
			// the context of reap_pid(), that state has special meaning.
			if (status.mState != UNSTARTED)
			{
				statstr = getStatusString(desc, status);
			}
			LL_INFOS("LLProcess") << statstr << LL_ENDL;
		}
		return 0;
	}

	return pid;
}

static LLProcess::Status interpret_status(int status)
{
	LLProcess::Status result;

	if (WIFEXITED(status))
	{
		result.mState = LLProcess::EXITED;
		result.mData  = WEXITSTATUS(status);
	}
	else if (WIFSIGNALED(status))
	{
		result.mState = LLProcess::KILLED;
		result.mData  = WTERMSIG(status);
	}
	else                            // uh, shouldn't happen?
	{
		result.mState = LLProcess::EXITED;
		result.mData  = status;     // someone else will have to decode
	}

	return result;
}

#endif  // Posix
