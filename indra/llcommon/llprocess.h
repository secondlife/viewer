/** 
 * @file llprocess.h
 * @brief Utility class for launching, terminating, and tracking child processes.
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
 
#ifndef LL_LLPROCESS_H
#define LL_LLPROCESS_H

#include "llinitparam.h"
#include "llsdparam.h"
#include "apr_thread_proc.h"
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <iosfwd>                   // std::ostream
#include <stdexcept>

#if LL_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>                // HANDLE (eye roll)
#elif LL_LINUX
#if defined(Status)
#undef Status
#endif
#endif

class LLEventPump;

class LLProcess;
/// LLProcess instances are created on the heap by static factory methods and
/// managed by ref-counted pointers.
typedef boost::shared_ptr<LLProcess> LLProcessPtr;

/**
 * LLProcess handles launching an external process with specified command line
 * arguments. It also keeps track of whether the process is still running, and
 * can kill it if required.
 *
 * In discussing LLProcess, we use the term "parent" to refer to this process
 * (the process invoking LLProcess), versus "child" to refer to the process
 * spawned by LLProcess.
 *
 * LLProcess relies on periodic post() calls on the "mainloop" LLEventPump: an
 * LLProcess object's Status won't update until the next "mainloop" tick. For
 * instance, the Second Life viewer's main loop already posts to an
 * LLEventPump by that name once per iteration. See
 * indra/llcommon/tests/llprocess_test.cpp for an example of waiting for
 * child-process termination in a standalone test context.
 */
class LL_COMMON_API LLProcess: public boost::noncopyable
{
	LOG_CLASS(LLProcess);
public:
	/**
	 * Specify what to pass for each of child stdin, stdout, stderr.
	 * @see LLProcess::Params::files.
	 */
	struct FileParam: public LLInitParam::Block<FileParam>
	{
		/**
		 * type of file handle to pass to child process
		 *
		 * - "" (default): let the child inherit the same file handle used by
		 *   this process. For instance, if passed as stdout, child stdout
		 *   will be interleaved with stdout from this process. In this case,
		 *   @a name is moot and should be left "".
		 *
		 * - "file": open an OS filesystem file with the specified @a name.
		 *   <i>Not yet implemented.</i>
		 *
		 * - "pipe" or "tpipe" or "npipe": depends on @a name
		 *
		 *   - @a name.empty(): construct an OS pipe used only for this slot
		 *     of the forthcoming child process.
		 *
		 *   - ! @a name.empty(): in a global registry, find or create (using
		 *     the specified @a name) an OS pipe. The point of the (purely
		 *     internal) @a name is that passing the same @a name in more than
		 *     one slot for a given LLProcess -- or for slots in different
		 *     LLProcess instances -- means the same pipe. For example, you
		 *     might pass the same @a name value as both stdout and stderr to
		 *     make the child process produce both on the same actual pipe. Or
		 *     you might pass the same @a name as the stdout for one LLProcess
		 *     and the stdin for another to connect the two child processes.
		 *     Use LLProcess::getPipeName() to generate a unique name
		 *     guaranteed not to already exist in the registry. <i>Not yet
		 *     implemented.</i>
		 *
		 *   The difference between "pipe", "tpipe" and "npipe" is as follows.
		 *
		 *   - "pipe": direct LLProcess to monitor the parent end of the pipe,
		 *     pumping nonblocking I/O every frame. The expectation (at least
		 *     for stdout or stderr) is that the caller will listen for
		 *     incoming data and consume it as it arrives. It's important not
		 *     to neglect such a pipe, because it's buffered in memory. If you
		 *     suspect the child may produce a great volume of output between
		 *     frames, consider directing the child to write to a filesystem
		 *     file instead, then read the file later.
		 *
		 *   - "tpipe": do not engage LLProcess machinery to monitor the
		 *     parent end of the pipe. A "tpipe" is used only to connect
		 *     different child processes. As such, it makes little sense to
		 *     pass an empty @a name. <i>Not yet implemented.</i>
		 *
		 *   - "npipe": like "tpipe", but use an OS named pipe with a
		 *     generated name. Note that @a name is the @em internal name of
		 *     the pipe in our global registry -- it doesn't necessarily have
		 *     anything to do with the pipe's name in the OS filesystem. Use
		 *     LLProcess::getPipeName() to obtain the named pipe's OS
		 *     filesystem name, e.g. to pass it as the @a name to another
		 *     LLProcess instance using @a type "file". This supports usage
		 *     like bash's &lt;(subcommand...) or &gt;(subcommand...)
		 *     constructs. <i>Not yet implemented.</i>
		 *
		 * In all cases the open mode (read, write) is determined by the child
		 * slot you're filling. Child stdin means select the "read" end of a
		 * pipe, or open a filesystem file for reading; child stdout or stderr
		 * means select the "write" end of a pipe, or open a filesystem file
		 * for writing.
		 *
		 * Confusion such as passing the same pipe as the stdin of two
		 * processes (rather than stdout for one and stdin for the other) is
		 * explicitly permitted: it's up to the caller to construct meaningful
		 * LLProcess pipe graphs.
		 */
		Optional<std::string> type;
		Optional<std::string> name;

		FileParam(const std::string& tp="", const std::string& nm=""):
			type("type"),
			name("name")
		{
			// If caller wants to specify values, use explicit assignment to
			// set them rather than initialization.
			if (! tp.empty()) type = tp;
			if (! nm.empty()) name = nm;
		}
	};

	/// Param block definition
	struct Params: public LLInitParam::Block<Params>
	{
		Params():
			executable("executable"),
			args("args"),
			cwd("cwd"),
			autokill("autokill", true),
			files("files"),
			postend("postend"),
			desc("desc")
		{}

		/// pathname of executable
		Mandatory<std::string> executable;
		/**
		 * zero or more additional command-line arguments. Arguments are
		 * passed through as exactly as we can manage, whitespace and all.
		 * @note On Windows we manage this by implicitly double-quoting each
		 * argument while assembling the command line.
		 */
		Multiple<std::string> args;
		/// current working directory, if need it changed
		Optional<std::string> cwd;
		/// implicitly kill process on destruction of LLProcess object
		/// (default true)
		Optional<bool> autokill;
		/**
		 * Up to three FileParam items: for child stdin, stdout, stderr.
		 * Passing two FileParam entries means default treatment for stderr,
		 * and so forth.
		 *
		 * @note LLInitParam::Block permits usage like this:
		 * @code
		 * LLProcess::Params params;
		 * ...
		 * params.files
		 *     .add(LLProcess::FileParam()) // stdin
		 *     .add(LLProcess::FileParam().type("pipe") // stdout
		 *     .add(LLProcess::FileParam().type("file").name("error.log"));
		 * @endcode
		 *
		 * @note While it's theoretically plausible to pass additional open
		 * file handles to a child specifically written to expect them, our
		 * underlying implementation doesn't yet support that.
		 */
		Multiple<FileParam, AtMost<3> > files;
		/**
		 * On child-process termination, if this LLProcess object still
		 * exists, post LLSD event to LLEventPump with specified name (default
		 * no event). Event contains at least:
		 *
		 * - "id" as obtained from getProcessID()
		 * - "desc" short string description of child (executable + pid)
		 * - "state" @c state enum value, from Status.mState
		 * - "data"	 if "state" is EXITED, exit code; if KILLED, on Posix,
		 *   signal number
		 * - "string" English text describing "state" and "data" (e.g. "exited
		 *   with code 0")
		 */
		Optional<std::string> postend;
		/**
		 * Description of child process for logging purposes. It need not be
		 * unique; the logged description string will contain the PID as well.
		 * If this is omitted, a description will be derived from the
		 * executable name.
		 */
		Optional<std::string> desc;
	};
	typedef LLSDParamAdapter<Params> LLSDOrParams;

	/**
	 * Factory accepting either plain LLSD::Map or Params block.
	 * MAY RETURN DEFAULT-CONSTRUCTED LLProcessPtr if params invalid!
	 */
	static LLProcessPtr create(const LLSDOrParams& params);
	virtual ~LLProcess();

	/// Is child process still running?
	bool isRunning() const;
	// static isRunning(LLProcessPtr), getStatus(LLProcessPtr),
	// getStatusString(LLProcessPtr), kill(LLProcessPtr) handle the case in
	// which the passed LLProcessPtr might be NULL (default-constructed).
	static bool isRunning(const LLProcessPtr&);

	/**
	 * State of child process
	 */
	enum state
	{
		UNSTARTED,					///< initial value, invisible to consumer
		RUNNING,					///< child process launched
		EXITED,						///< child process terminated voluntarily
		KILLED						///< child process terminated involuntarily
	};

	/**
	 * Status info
	 */
	struct Status
	{
		Status():
			mState(UNSTARTED),
			mData(0)
		{}

		state mState;				///< @see state
		/**
		 * - for mState == EXITED: mData is exit() code
		 * - for mState == KILLED: mData is signal number (Posix)
		 * - otherwise: mData is undefined
		 */
		int mData;
	};

	/// Status query
	Status getStatus() const;
	static Status getStatus(const LLProcessPtr&);
	/// English Status string query, for logging etc.
	std::string getStatusString() const;
	static std::string getStatusString(const std::string& desc, const LLProcessPtr&);
	/// English Status string query for previously-captured Status
	std::string getStatusString(const Status& status) const;
	/// static English Status string query
	static std::string getStatusString(const std::string& desc, const Status& status);

	// Attempt to kill the process -- returns true if the process is no longer running when it returns.
	// Note that even if this returns false, the process may exit some time after it's called.
	bool kill(const std::string& who="");
	static bool kill(const LLProcessPtr& p, const std::string& who="");

#if LL_WINDOWS
	typedef int id;                 ///< as returned by getProcessID()
	typedef HANDLE handle;          ///< as returned by getProcessHandle()
#else
	typedef pid_t id;
	typedef pid_t handle;
#endif
	/**
	 * Get an int-like id value. This is primarily intended for a human reader
	 * to differentiate processes.
	 */
	id getProcessID() const;
	/**
	 * Get a "handle" of a kind that you might pass to platform-specific API
	 * functions to engage features not directly supported by LLProcess.
	 */
	handle getProcessHandle() const;

	/**
	 * Test if a process (@c handle obtained from getProcessHandle()) is still
	 * running. Return same nonzero @c handle value if still running, else
	 * zero, so you can test it like a bool. But if you want to update a
	 * stored variable as a side effect, you can write code like this:
	 * @code
	 * hchild = LLProcess::isRunning(hchild);
	 * @endcode
	 * @note This method is intended as a unit-test hook, not as the first of
	 * a whole set of operations supported on freestanding @c handle values.
	 * New functionality should be added as nonstatic members operating on
	 * the same data as getProcessHandle().
	 *
	 * In particular, if child termination is detected by this static isRunning()
	 * rather than by nonstatic isRunning(), the LLProcess object won't be
	 * aware of the child's changed status and may encounter OS errors trying
	 * to obtain it. This static isRunning() is only intended for after the
	 * launching LLProcess object has been destroyed.
	 */
	static handle isRunning(handle, const std::string& desc="");

	/// Provide symbolic access to child's file slots
	enum FILESLOT { STDIN=0, STDOUT=1, STDERR=2, NSLOTS=3 };

	/**
	 * For a pipe constructed with @a type "npipe", obtain the generated OS
	 * filesystem name for the specified pipe. Otherwise returns the empty
	 * string. @see LLProcess::FileParam::type
	 */
	std::string getPipeName(FILESLOT) const;

	/// base of ReadPipe, WritePipe
	class LL_COMMON_API BasePipe
	{
	public:
		virtual ~BasePipe() = 0;

		typedef std::size_t size_type;
		static const size_type npos;

		/**
		 * Get accumulated buffer length.
		 *
		 * For WritePipe, is there still pending data to send to child?
		 *
		 * For ReadPipe, we often need to refrain from actually reading the
		 * std::istream returned by get_istream() until we've accumulated
		 * enough data to make it worthwhile. For instance, if we're expecting
		 * a number from the child, but the child happens to flush "12" before
		 * emitting "3\n", get_istream() >> myint could return 12 rather than
		 * 123!
		 */
		virtual size_type size() const = 0;
	};

	/// As returned by getWritePipe() or getOptWritePipe()
	class WritePipe: public BasePipe
	{
	public:
		/**
		 * Get ostream& on which to write to child's stdin.
		 *
		 * @usage
		 * @code
		 * myProcess->getWritePipe().get_ostream() << "Hello, child!" << std::endl;
		 * @endcode
		 */
		virtual std::ostream& get_ostream() = 0;
	};

	/// As returned by getReadPipe() or getOptReadPipe()
	class ReadPipe: public BasePipe
	{
	public:
		/**
		 * Get istream& on which to read from child's stdout or stderr.
		 *
		 * @usage
		 * @code
		 * std::string stuff;
		 * myProcess->getReadPipe().get_istream() >> stuff;
		 * @endcode
		 *
		 * You should be sure in advance that the ReadPipe in question can
		 * fill the request. @see getPump()
		 */
		virtual std::istream& get_istream() = 0;

		/**
		 * Like std::getline(get_istream(), line), but trims off trailing '\r'
		 * to make calling code less platform-sensitive.
		 */
		virtual std::string getline() = 0;

		/**
		 * Like get_istream().read(buffer, n), but returns std::string rather
		 * than requiring caller to construct a buffer, etc.
		 */
		virtual std::string read(size_type len) = 0;

		/**
		 * Peek at accumulated buffer data without consuming it. Optional
		 * parameters give you substr() functionality.
		 *
		 * @note You can discard buffer data using get_istream().ignore(n).
		 */
		virtual std::string peek(size_type offset=0, size_type len=npos) const = 0;

		/**
		 * Detect presence of a substring (or char) in accumulated buffer data
		 * without retrieving it. Optional offset allows you to search from
		 * specified position.
		 */
		template <typename SEEK>
		bool contains(SEEK seek, size_type offset=0) const
		{ return find(seek, offset) != npos; }

		/**
		 * Search for a substring in accumulated buffer data without
		 * retrieving it. Returns size_type position at which found, or npos
		 * meaning not found. Optional offset allows you to search from
		 * specified position.
		 */
		virtual size_type find(const std::string& seek, size_type offset=0) const = 0;

		/**
		 * Search for a char in accumulated buffer data without retrieving it.
		 * Returns size_type position at which found, or npos meaning not
		 * found. Optional offset allows you to search from specified
		 * position.
		 */
		virtual size_type find(char seek, size_type offset=0) const = 0;

		/**
		 * Get LLEventPump& on which to listen for incoming data. The posted
		 * LLSD::Map event will contain:
		 *
		 * - "data" part of pending data; see setLimit()
		 * - "len" entire length of pending data, regardless of setLimit()
		 * - "slot" this ReadPipe's FILESLOT, e.g. LLProcess::STDOUT
		 * - "name" e.g. "stdout"
		 * - "desc" e.g. "SLPlugin (pid) stdout"
		 * - "eof" @c true means there no more data will arrive on this pipe,
		 *   therefore no more events on this pump
		 *
		 * If the child sends "abc", and this ReadPipe posts "data"="abc", but
		 * you don't consume it by reading the std::istream returned by
		 * get_istream(), and the child next sends "def", ReadPipe will post
		 * "data"="abcdef".
		 */
		virtual LLEventPump& getPump() = 0;

		/**
		 * Set maximum length of buffer data that will be posted in the LLSD
		 * announcing arrival of new data from the child. If you call
		 * setLimit(5), and the child sends "abcdef", the LLSD event will
		 * contain "data"="abcde". However, you may still read the entire
		 * "abcdef" from get_istream(): this limit affects only the size of
		 * the data posted with the LLSD event. If you don't call this method,
		 * @em no data will be posted: the default is 0 bytes.
		 */
		virtual void setLimit(size_type limit) = 0;

		/**
		 * Query the current setLimit() limit.
		 */
		virtual size_type getLimit() const = 0;
	};

	/// Exception thrown by getWritePipe(), getReadPipe() if you didn't ask to
	/// create a pipe at the corresponding FILESLOT.
	struct NoPipe: public std::runtime_error
	{
		NoPipe(const std::string& what): std::runtime_error(what) {}
	};

	/**
	 * Get a reference to the (only) WritePipe for this LLProcess. @a slot, if
	 * specified, must be STDIN. Throws NoPipe if you did not request a "pipe"
	 * for child stdin. Use this method when you know how you created the
	 * LLProcess in hand.
	 */
	WritePipe& getWritePipe(FILESLOT slot=STDIN);

	/**
	 * Get a boost::optional<WritePipe&> to the (only) WritePipe for this
	 * LLProcess. @a slot, if specified, must be STDIN. The return value is
	 * empty if you did not request a "pipe" for child stdin. Use this method
	 * for inspecting an LLProcess you did not create.
	 */
	boost::optional<WritePipe&> getOptWritePipe(FILESLOT slot=STDIN);

	/**
	 * Get a reference to one of the ReadPipes for this LLProcess. @a slot, if
	 * specified, must be STDOUT or STDERR. Throws NoPipe if you did not
	 * request a "pipe" for child stdout or stderr. Use this method when you
	 * know how you created the LLProcess in hand.
	 */
	ReadPipe& getReadPipe(FILESLOT slot);

	/**
	 * Get a boost::optional<ReadPipe&> to one of the ReadPipes for this
	 * LLProcess. @a slot, if specified, must be STDOUT or STDERR. The return
	 * value is empty if you did not request a "pipe" for child stdout or
	 * stderr. Use this method for inspecting an LLProcess you did not create.
	 */
	boost::optional<ReadPipe&> getOptReadPipe(FILESLOT slot);

	/// little utilities that really should already be somewhere else in the
	/// code base
	static std::string basename(const std::string& path);
	static std::string getline(std::istream&);

private:
	/// constructor is private: use create() instead
	LLProcess(const LLSDOrParams& params);
	void autokill();
	// Classic-C-style APR callback
	static void status_callback(int reason, void* data, int status);
	// Object-oriented callback
	void handle_status(int reason, int status);
	// implementation for get[Opt][Read|Write]Pipe()
	template <class PIPETYPE>
	PIPETYPE& getPipe(FILESLOT slot);
	template <class PIPETYPE>
	boost::optional<PIPETYPE&> getOptPipe(FILESLOT slot);
	template <class PIPETYPE>
	PIPETYPE* getPipePtr(std::string& error, FILESLOT slot);

	std::string mDesc;
	std::string mPostend;
	apr_proc_t mProcess;
	bool mAutokill;
	Status mStatus;
	// explicitly want this ptr_vector to be able to store NULLs
	typedef boost::ptr_vector< boost::nullable<BasePipe> > PipeVector;
	PipeVector mPipes;
};

/// for logging
LL_COMMON_API std::ostream& operator<<(std::ostream&, const LLProcess::Params&);

#endif // LL_LLPROCESS_H
