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
#include "llsd.h"
#include "llsdserialize.h"
#include "stringize.h"

#include <boost/foreach.hpp>
#include <iostream>
#include <stdexcept>

/// Need an exception to avoid constructing an invalid LLProcess object, but
/// internal use only
struct LLProcessError: public std::runtime_error
{
	LLProcessError(const std::string& msg): std::runtime_error(msg) {}
};

LLProcessPtr LLProcess::create(const LLSD& params)
{
	try
	{
		return LLProcessPtr(new LLProcess(params));
	}
	catch (const LLProcessError& e)
	{
		LL_WARNS("LLProcess") << e.what() << LL_ENDL;
		return LLProcessPtr();
	}
}

LLProcess::LLProcess(const LLSD& params):
	mProcessID(0),
	mAutokill(params["autokill"].asBoolean())
{
	// nonstandard default bool value
	if (! params.has("autokill"))
		mAutokill = true;
	if (! params.has("executable"))
	{
		throw LLProcessError(STRINGIZE("not launched: missing 'executable'\n"
									   << LLSDNotationStreamer(params)));
	}

	launch(params);
}

LLProcess::~LLProcess()
{
	if (mAutokill)
	{
		kill();
	}
}

bool LLProcess::isRunning(void)
{
	mProcessID = isRunning(mProcessID);
	return (mProcessID != 0);
}

#if LL_WINDOWS

static std::string quote(const std::string& str)
{
	std::string::size_type len(str.length());
	// If the string is already quoted, assume user knows what s/he's doing.
	if (len >= 2 && str[0] == '"' && str[len-1] == '"')
	{
		return str;
	}

	// Not already quoted: do it.
	std::string result("\"");
	for (std::string::const_iterator ci(str.begin()), cend(str.end()); ci != cend; ++ci)
	{
		if (*ci == '"')
		{
			result.append("\\");
		}
		result.push_back(*ci);
	}
	return result + "\"";
}

void LLProcess::launch(const LLSD& params)
{
	PROCESS_INFORMATION pinfo;
	STARTUPINFOA sinfo;
	memset(&sinfo, 0, sizeof(sinfo));
	
	std::string args = quote(params["executable"]);
	BOOST_FOREACH(const std::string& arg, llsd::inArray(params["args"]))
	{
		args += " ";
		args += quote(arg);
	}
	
	// So retarded.  Windows requires that the second parameter to
	// CreateProcessA be a writable (non-const) string...
	std::vector<char> args2(args.begin(), args.end());
	args2.push_back('\0');

	// Convert wrapper to a real std::string so we can use c_str(); but use a
	// named variable instead of a temporary so c_str() pointer remains valid.
	std::string cwd(params["cwd"]);
	const char * working_directory = 0;
	if (! cwd.empty())
		working_directory = cwd.c_str();
	if( ! CreateProcessA( NULL, &args2[0], NULL, NULL, FALSE, 0, NULL, working_directory, &sinfo, &pinfo ) )
	{
		int result = GetLastError();

		LPTSTR error_str = 0;
		if(
			FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				result,
				0,
				(LPTSTR)&error_str,
				0,
				NULL) 
			!= 0) 
		{
			char message[256];
			wcstombs(message, error_str, sizeof(message));
			message[sizeof(message)-1] = 0;
			LocalFree(error_str);
			throw LLProcessError(STRINGIZE("CreateProcessA failed (" << result << "): "
										   << message));
		}
		throw LLProcessError(STRINGIZE("CreateProcessA failed (" << result
									   << "), but FormatMessage() did not explain"));
	}

	// foo = pinfo.dwProcessId; // get your pid here if you want to use it later on
	// CloseHandle(pinfo.hProcess); // stops leaks - nothing else
	mProcessID = pinfo.hProcess;
	CloseHandle(pinfo.hThread); // stops leaks - nothing else
}

LLProcess::id LLProcess::isRunning(id handle)
{
	if (! handle)
		return 0;

	DWORD waitresult = WaitForSingleObject(handle, 0);
	if(waitresult == WAIT_OBJECT_0)
	{
		// the process has completed.
		return 0;
	}

	return handle;
}

bool LLProcess::kill(void)
{
	if (! mProcessID)
		return false;

	TerminateProcess(mProcessID, 0);
	return ! isRunning();
}

#else // Mac and linux

#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

// Attempt to reap a process ID -- returns true if the process has exited and been reaped, false otherwise.
static bool reap_pid(pid_t pid)
{
	pid_t wait_result = ::waitpid(pid, NULL, WNOHANG);
	if (wait_result == pid)
	{
		return true;
	}
	if (wait_result == -1 && errno == ECHILD)
	{
		// No such process -- this may mean we're ignoring SIGCHILD.
		return true;
	}
	
	return false;
}

void LLProcess::launch(const LLSD& params)
{
	// flush all buffers before the child inherits them
	::fflush(NULL);

	pid_t child = vfork();
	if (child == 0)
	{
		// child process

		std::string cwd(params["cwd"]);
		if (! cwd.empty())
		{
			// change to the desired child working directory
			if (::chdir(cwd.c_str()))
			{
				// chdir failed
				LL_WARNS("LLProcess") << "could not chdir(\"" << cwd << "\")" << LL_ENDL;
				// pointless to throw; this is child process...
				_exit(248);
			}
		}

		// create an argv vector for the child process
		std::vector<const char*> fake_argv;

		// add the executable path
		std::string executable(params["executable"]);
		fake_argv.push_back(executable.c_str());

		// and any arguments
		const LLSD& params_args(params["args"]);
		std::vector<std::string> args(params_args.beginArray(), params_args.endArray());
		BOOST_FOREACH(const std::string& arg, args)
		{
			fake_argv.push_back(arg.c_str());
		}

		// terminate with a null pointer
		fake_argv.push_back(NULL);

		::execv(executable.c_str(), const_cast<char* const*>(&fake_argv[0]));

		// If we reach this point, the exec failed.
		LL_WARNS("LLProcess") << "failed to launch: ";
		BOOST_FOREACH(const char* arg, fake_argv)
		{
			LL_CONT << arg << ' ';
		}
		LL_CONT << LL_ENDL;
		// Use _exit() instead of exit() per the vfork man page. Exit with a
		// distinctive rc: someday soon we'll be able to retrieve it, and it
		// would be nice to be able to tell that the child process failed!
		_exit(249);
	}

	// parent process
	mProcessID = child;
}

LLProcess::id LLProcess::isRunning(id pid)
{
	if (! pid)
		return 0;

	// Check whether the process has exited, and reap it if it has.
	if(reap_pid(pid))
	{
		// the process has exited.
		return 0;
	}

	return pid;
}

bool LLProcess::kill(void)
{
	if (! mProcessID)
		return false;

	// Try to kill the process. We'll do approximately the same thing whether
	// the kill returns an error or not, so we ignore the result.
	(void)::kill(mProcessID, SIGTERM);

	// This will have the side-effect of reaping the zombie if the process has exited.
	return ! isRunning();
}

/*==========================================================================*|
static std::list<pid_t> sZombies;

void LLProcess::orphan(void)
{
	// Disassociate the process from this object
	if(mProcessID != 0)
	{	
		// We may still need to reap the process's zombie eventually
		sZombies.push_back(mProcessID);
	
		mProcessID = 0;
	}
}

// static 
void LLProcess::reap(void)
{
	// Attempt to real all saved process ID's.
	
	std::list<pid_t>::iterator iter = sZombies.begin();
	while(iter != sZombies.end())
	{
		if(reap_pid(*iter))
		{
			iter = sZombies.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}
|*==========================================================================*/

#endif
