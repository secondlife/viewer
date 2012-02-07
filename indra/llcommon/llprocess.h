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
#include <boost/noncopyable.hpp>
#include <iosfwd>                   // std::ostream

#if LL_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>                // HANDLE (eye roll)
#elif LL_LINUX
#if defined(Status)
#undef Status
#endif
#endif

class LLProcess;
/// LLProcess instances are created on the heap by static factory methods and
/// managed by ref-counted pointers.
typedef boost::shared_ptr<LLProcess> LLProcessPtr;

/**
 *	LLProcess handles launching external processes with specified command line arguments.
 *	It also keeps track of whether the process is still running, and can kill it if required.
*/
class LL_COMMON_API LLProcess: public boost::noncopyable
{
	LOG_CLASS(LLProcess);
public:
	/// Param block definition
	struct Params: public LLInitParam::Block<Params>
	{
		Params():
			executable("executable"),
			args("args"),
			cwd("cwd"),
			autokill("autokill", true)
		{}

		/// pathname of executable
		Mandatory<std::string> executable;
		/**
		 * zero or more additional command-line arguments. Arguments are
		 * passed through as exactly as we can manage, whitespace and all.
		 * @note On Windows we manage this by implicitly double-quoting each
		 * argument while assembling the command line. BUT if a given argument
		 * is already double-quoted, we don't double-quote it again. Try to
		 * avoid making use of this, though, as on Mac and Linux explicitly
		 * double-quoted args will be passed to the child process including
		 * the double quotes.
		 */
		Multiple<std::string> args;
		/// current working directory, if need it changed
		Optional<std::string> cwd;
		/// implicitly kill process on destruction of LLProcess object
		Optional<bool> autokill;
	};
	typedef LLSDParamAdapter<Params> LLSDOrParams;

	/**
	 * Factory accepting either plain LLSD::Map or Params block.
	 * MAY RETURN DEFAULT-CONSTRUCTED LLProcessPtr if params invalid!
	 *
	 * Redundant with Params definition above?
	 *
	 * executable (required, string):				executable pathname
	 * args		  (optional, string array):			extra command-line arguments
	 * cwd		  (optional, string, dft no chdir): change to this directory before executing
	 * autokill	  (optional, bool, dft true):		implicit kill() on ~LLProcess
	 */
	static LLProcessPtr create(const LLSDOrParams& params);
	virtual ~LLProcess();

	// isRunning() isn't const because, when child terminates, it sets stored
	// Status
	bool isRunning(void);

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
	Status getStatus();
	/// English Status string query, for logging etc.
	std::string getStatusString();
	/// English Status string query for previously-captured Status
	std::string getStatusString(const Status& status);
	/// static English Status string query
	static std::string getStatusString(const std::string& desc, const Status& status);

	// Attempt to kill the process -- returns true if the process is no longer running when it returns.
	// Note that even if this returns false, the process may exit some time after it's called.
	bool kill(const std::string& who="");

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
	 * In particular, if child termination is detected by static isRunning()
	 * rather than by nonstatic isRunning(), the LLProcess object won't be
	 * aware of the child's changed status and may encounter OS errors trying
	 * to obtain it. static isRunning() is only intended for after the
	 * launching LLProcess object has been destroyed.
	 */
	static handle isRunning(handle, const std::string& desc="");

private:
	/// constructor is private: use create() instead
	LLProcess(const LLSDOrParams& params);
	void autokill();
	// Classic-C-style APR callback
	static void status_callback(int reason, void* data, int status);
	// Object-oriented callback
	void handle_status(int reason, int status);

	std::string mDesc;
	apr_proc_t mProcess;
	bool mAutokill;
	Status mStatus;
};

/// for logging
LL_COMMON_API std::ostream& operator<<(std::ostream&, const LLProcess::Params&);

#endif // LL_LLPROCESS_H
