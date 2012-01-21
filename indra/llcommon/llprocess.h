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

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#if LL_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

class LLSD;

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

	/**
	 * Factory accepting LLSD::Map.
	 * MAY RETURN DEFAULT-CONSTRUCTED LLProcessPtr if params invalid!
	 *
	 * executable (required, string):				executable pathname
	 * args		  (optional, string array):			extra command-line arguments
	 * cwd		  (optional, string, dft no chdir): change to this directory before executing
	 * autokill	  (optional, bool, dft true):		implicit kill() on ~LLProcess
	 */
	static LLProcessPtr create(const LLSD& params);
	virtual ~LLProcess();

	// isRunning isn't const because, if child isn't running, it clears stored
	// process ID
	bool isRunning(void);
	
	// Attempt to kill the process -- returns true if the process is no longer running when it returns.
	// Note that even if this returns false, the process may exit some time after it's called.
	bool kill(void);

#if LL_WINDOWS
	typedef HANDLE id;
#else
	typedef pid_t  id;
#endif	
	/// Get platform-specific process ID
	id getProcessID() const { return mProcessID; };

	/**
	 * Test if a process (id obtained from getProcessID()) is still
	 * running. Return is same nonzero id value if still running, else
	 * zero, so you can test it like a bool. But if you want to update a
	 * stored variable as a side effect, you can write code like this:
	 * @code
	 * childpid = LLProcess::isRunning(childpid);
	 * @endcode
	 * @note This method is intended as a unit-test hook, not as the first of
	 * a whole set of operations supported on freestanding @c id values. New
	 * functionality should be added as nonstatic members operating on
	 * mProcessID.
	 */
	static id isRunning(id);
	
private:
	/// constructor is private: use create() instead
	LLProcess(const LLSD& params);
	void launch(const LLSD& params);

	id mProcessID;
	bool mAutokill;
};

#endif // LL_LLPROCESS_H
