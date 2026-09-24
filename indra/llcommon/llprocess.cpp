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
#include "llevents.h"
#include "llexception.h"
#include "stringize.h"

#include <boost/process/v2/process.hpp>
#include <boost/process/v2/environment.hpp>
#include <boost/process/v2/start_dir.hpp>
#include <boost/process/v2/stdio.hpp>
#include <boost/asio.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/filesystem/path.hpp>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <errno.h>
#include <thread>
#include <vector>
#include <typeinfo>
#include <signal.h>
#include <utility>

#if !LL_WINDOWS
 // not necessarily available on random SDL platforms
 // for waitpid()
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#if LL_WINDOWS
#include <windows.h>
#include "llwin32headers.h"
#include <mutex>

namespace {
    // Global job object that will kill all child processes when parent terminates
    HANDLE g_jobObject = NULL;
    bool g_jobObjectInitialized = false;
    std::mutex g_jobObjectMutex;

    void InitializeJobObject()
    {
        if (g_jobObjectInitialized)
            return;
        std::lock_guard<std::mutex> lock(g_jobObjectMutex);
        if (g_jobObjectInitialized)
            return;

        g_jobObjectInitialized = true;

        // Create a job object
        g_jobObject = ::CreateJobObjectW(NULL, NULL);
        if (g_jobObject == NULL)
        {
            LL_WARNS("LLProcess") << "Failed to create job object: " << ::GetLastError() << LL_ENDL;
            return;
        }

        // Configure the job to kill all processes when the last handle closes
        // (i.e., when the parent process exits)
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

        // Windows 8+ supports nested jobs (for VS studio)
        jeli.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_BREAKAWAY_OK;

        if (!::SetInformationJobObject(g_jobObject, JobObjectExtendedLimitInformation,
            &jeli, sizeof(jeli)))
        {
            LL_WARNS("LLProcess") << "Failed to set job object limits: " << ::GetLastError() << LL_ENDL;
            ::CloseHandle(g_jobObject);
            g_jobObject = NULL;
            return;
        }

        LL_INFOS("LLProcess") << "Job object created - child processes will terminate with parent" << LL_ENDL;
    }

    void AssignProcessToJob(HANDLE hProcess, const std::string& desc)
    {
        if (!g_jobObjectInitialized)
            InitializeJobObject();

        if (g_jobObject != NULL)
        {
            if (!::AssignProcessToJobObject(g_jobObject, hProcess))
            {
                DWORD error = ::GetLastError();
                // ERROR_ACCESS_DENIED (5) means the process is already in a job
                // This can happen if the parent viewer is itself in a job
                if (error == ERROR_ACCESS_DENIED)
                {
                    LL_WARNS("LLProcess") << "Autokill requested but process " << desc
                        << " is already in a job object (ERROR_ACCESS_DENIED)"
                        << LL_ENDL;
                }
                else
                {
                    LL_WARNS("LLProcess") << "Failed to assign process " << desc
                        << " to job object: error " << error << LL_ENDL;
                }
            }
            else
            {
                LL_DEBUGS("LLProcess") << "Process " << desc << " assigned to job object" << LL_ENDL;
            }
        }
    }
}
#endif

namespace bp = boost::process::v2;
namespace asio = boost::asio;

/*****************************************************************************
*   Helpers
*****************************************************************************/

static const char* whichfile_[] = { "stdin", "stdout", "stderr" };

static std::string whichfile(LLProcess::FILESLOT index)
{
    if (index < LL_ARRAY_SIZE(whichfile_))
        return whichfile_[index];
    return STRINGIZE("file slot " << index);
}

std::ostream& operator<<(std::ostream& out, const LLProcess::Params& params)
{
    if (params.cwd.isProvided())
    {
        out << "cd " << LLStringUtil::quote(params.cwd) << ": ";
    }
    out << LLStringUtil::quote(params.executable);
    for (const std::string& arg : params.args)
    {
        out << ' ' << LLStringUtil::quote(arg);
    }
    return out;
}
/*****************************************************************************
*   Helper classes for pipe I/O
*****************************************************************************/

class WritePipeImpl : public LLProcess::WritePipe
{
    LOG_CLASS(WritePipeImpl);
public:
    WritePipeImpl(const std::string& desc,
        std::shared_ptr<asio::writable_pipe> pipe) :
        mDesc(desc),
        mPipe(pipe),
        mStream(&mStreambuf),
        mWritePending(false)
    {}

    virtual ~WritePipeImpl() = default;

    virtual std::ostream& get_ostream() override { return mStream; }

    virtual size_type size() const override
    {
        return mStreambuf.size();
    }

    // Called from LLProcess::tick() to initiate writing buffered data.
    // Self-chains: each completed write immediately starts the next one if
    // more data is waiting, so large transfers don't stall between ticks.
    void tick() override
    {
        startAsyncWrite();
    }

private:
    void startAsyncWrite()
    {
        if (mWritePending || !mPipe || !mPipe->is_open() || mStreambuf.size() == 0)
            return;

        mWritePending = true;

        // Snapshot the number of bytes currently buffered so the async_write
        // operates on a fixed-size, stable view. Any data written to
        // get_ostream() while this write is in flight lands in the streambuf's
        // put area and is excluded from the current operation; it will be sent
        // on the next tick(). Without this snapshot, a concurrent write to
        // get_ostream() could reallocate/invalidate the buffer sequence.
        std::size_t writeSize = mStreambuf.size();

        // Write all buffered data asynchronously. Do NOT self-chain in the
        // completion handler: sending the next buffer immediately can cause
        // the child to respond within the same tick(), which posts a read
        // event before the test listener has a chance to disconnect -- that
        // was the root cause of test 18 "more than 3 events" and test 9
        // "many small messages" failures. tick() calls mWritePipe->tick() on
        // every mainloop frame, so any newly-queued data will be sent then.
        asio::async_write(*mPipe, asio::buffer(mStreambuf.data(), writeSize),
            [this](const boost::system::error_code& ec, std::size_t bytes_transferred)
        {
            mWritePending = false;

            if (!ec)
            {
                mStreambuf.consume(bytes_transferred);
                LL_DEBUGS("LLProcess") << "Wrote " << bytes_transferred
                    << " bytes to " << mDesc << LL_ENDL;
            }
            else if (ec != asio::error::operation_aborted)
            {
                LL_WARNS("LLProcess") << "Write error on " << mDesc
                    << ": " << ec.message() << LL_ENDL;
            }
        });
    }

    std::string mDesc;
    std::shared_ptr<asio::writable_pipe> mPipe;
    asio::streambuf mStreambuf;
    std::ostream mStream;
    bool mWritePending;
};

class ReadPipeImpl : public LLProcess::ReadPipe
{
    LOG_CLASS(ReadPipeImpl);
public:
    ReadPipeImpl(const std::string& desc,
        std::shared_ptr<asio::readable_pipe> pipe,
        LLProcess::FILESLOT slot) :
        mDesc(desc),
        mPipe(pipe),
        mSlot(slot),
        mStream(&mStreambuf),
        mPump("ReadPipe", true),
        mLimit(0),
        mEOF(false)
    {
        // Start async read
        startAsyncRead();
    }

    virtual ~ReadPipeImpl()
    {
        if (mPipe && mPipe->is_open())
        {
            boost::system::error_code ec;
            mPipe->close(ec);
        }
    }

    virtual std::istream& get_istream() override { return mStream; }

    virtual std::string getline() override
    {
        return LLProcess::getline(mStream);
    }

    virtual LLEventPump& getPump() override { return mPump; }

    virtual void setLimit(size_type limit) override { mLimit = limit; }

    virtual size_type getLimit() const override { return mLimit; }

    virtual bool atEOF() const override { return mEOF; }

    virtual size_type size() const override { return mStreambuf.size(); }

    virtual std::string read(size_type len) override
    {
        size_type readlen = (std::min)(size(), len);
        if (!readlen)
            return "";

        std::vector<char> buffer(readlen);
        mStream.read(&buffer[0], readlen);
        return std::string(&buffer[0], mStream.gcount());
    }

    virtual std::string peek(size_type offset = 0, size_type len = npos) const override
    {
        std::size_t real_offset = (std::min)(mStreambuf.size(), std::size_t(offset));
        size_type want_end = (len == npos) ? npos : (real_offset + len);
        std::size_t real_end = (std::min)(mStreambuf.size(), std::size_t(want_end));

        auto cbufs = mStreambuf.data();
        return std::string(asio::buffers_begin(cbufs) + real_offset,
            asio::buffers_begin(cbufs) + real_end);
    }

    virtual size_type find(const std::string& seek, size_type offset = 0) const override
    {
        if (seek.length() == 1)
            return find(seek[0], offset);

        if (offset > mStreambuf.size())
            return npos;

        auto cbufs = mStreambuf.data();
        auto begin = asio::buffers_begin(cbufs);
        auto end = asio::buffers_end(cbufs);
        auto found = std::search(begin + offset, end, seek.begin(), seek.end());
        return (found == end) ? npos : (found - begin);
    }

    virtual size_type find(char seek, size_type offset = 0) const override
    {
        if (offset > mStreambuf.size())
            return npos;

        auto cbufs = mStreambuf.data();
        auto begin = asio::buffers_begin(cbufs);
        auto end = asio::buffers_end(cbufs);
        auto found = std::find(begin + offset, end, seek);
        return (found == end) ? npos : (found - begin);
    }

private:
    void startAsyncRead()
    {
        if (!mPipe || !mPipe->is_open() || mEOF)
            return;

        // Always read data regardless of mLimit to prevent INTEGRATION_TEST_llleap
        // deadlock: stopping reads fills the OS pipe buffer and blocks the child,
        // which prevents large (~1 MB) messages from being fully received.
        // mLimit only controls how many bytes appear in event notifications.
        auto bufs = mStreambuf.prepare(4096);

        mPipe->async_read_some(bufs,
            [this](const boost::system::error_code& ec, std::size_t bytes_transferred)
        {
            if (!ec)
            {
                mStreambuf.commit(bytes_transferred);
                LL_DEBUGS("LLProcess") << "Read " << bytes_transferred
                    << " bytes from " << mDesc << LL_ENDL;

                // Restore original 7-field event contract:
                // data, len, slot, name, desc, eof, exhst
                // A successful async read corresponds to the original EXHAUSTED
                // state: we got data, pipe is not yet closed.
                size_type data_len = (std::min)(mStreambuf.size(), mLimit);
                LLSD event;
                event["data"]  = peek(0, data_len);
                event["len"]   = LLSD::Integer(mStreambuf.size());
                event["slot"]  = LLSD::Integer(mSlot);
                event["name"]  = whichfile(mSlot);
                event["desc"]  = mDesc;
                event["eof"]   = false;
                event["exhst"] = true;

                // Arm the next read before posting: LLEventStream::post() is synchronous
                // and listeners may destroy this object.
                startAsyncRead();
                mPump.post(event);
            }
            else if (ec == asio::error::eof
#if LL_WINDOWS
                     // On Windows anonymous pipes, the write end closing
                     // delivers ERROR_BROKEN_PIPE (109), not asio::error::eof.
                     || ec.value() == ERROR_BROKEN_PIPE
#endif
                    )
            {
                if (bytes_transferred > 0)
                {
                    mStreambuf.commit(bytes_transferred);
                }

                mEOF = true;
                LL_DEBUGS("LLProcess") << "EOF on " << mDesc << LL_ENDL;

                // Match the original behavior: pack all 7 fields into the
                // single eof event so consumers can "use it or lose it" --
                // this is the last chance to see any remaining buffered data.
                // EOF corresponds to the original CLOSED state.
                size_type data_len = (std::min)(mStreambuf.size(), mLimit);
                LLSD eof_event;
                eof_event["data"]  = peek(0, data_len);
                eof_event["len"]   = LLSD::Integer(mStreambuf.size());
                eof_event["slot"]  = LLSD::Integer(mSlot);
                eof_event["name"]  = whichfile(mSlot);
                eof_event["desc"]  = mDesc;
                eof_event["eof"]   = true;
                eof_event["exhst"] = false;

                mPump.post(eof_event);
            }
            else if (ec != asio::error::operation_aborted)
            {
                LL_WARNS("LLProcess") << "Read error on " << mDesc
                    << ": " << ec.message() << LL_ENDL;
            }
        });
    }

    std::string mDesc;
    std::shared_ptr<asio::readable_pipe> mPipe;
    LLProcess::FILESLOT mSlot;
    mutable asio::streambuf mStreambuf;
    std::istream mStream;
    LLEventStream mPump; // pump specific to this pipe
    size_type mLimit;
    bool mEOF;
};

/*****************************************************************************
*   LLProcess implementation
*****************************************************************************/

const LLProcess::BasePipe::size_type LLProcess::BasePipe::npos =
static_cast<LLProcess::BasePipe::size_type>(-1);

LLProcess::LLProcess(const Params& params) :
    mStatus(),
    mDesc(params.desc.isProvided() ? params.desc() : basename(params.executable())),
    mPostend(params.postend.isProvided() ? params.postend() : ""),
    mAutokill(params.autokill),
    mAttached(params.attached.isProvided() ? params.attached() : bool(params.autokill)),
    mKillCalled(false)
{
    launch(params);
}

LLProcess::~LLProcess()
{
    if (mChild && mStatus.mState == RUNNING)
    {
        if (mAttached && mAutokill && !mKillCalled)
        {
            LL_INFOS("LLProcess") << "Terminating child process " << mDesc << LL_ENDL;
            boost::system::error_code ec;
            mChild->terminate(ec);

#if !LL_WINDOWS
            // On POSIX, terminate() sends SIGTERM which allows graceful shutdown.
            // Poll with waitpid(WNOHANG) rather than mChild->running() to avoid
            // competing with tick()'s own waitpid call.
            pid_t pid = mChild->id();
            for (int i = 0; i < 30; ++i)
            {
                int child_status = 0;
                pid_t w = ::waitpid(pid, &child_status, WNOHANG);
                if (w == pid || (w == -1 && errno == ECHILD))
                    break; // child exited or was already reaped
                if (w == -1 && errno != EINTR)
                {
                    LL_WARNS("LLProcess") << "waitpid(" << pid << ") failed while terminating "
                        << mDesc << ": " << strerror(errno) << LL_ENDL;
                    break;
                }
                // Sleep before next poll: applies to both EINTR and
                // still-running (w == 0) cases.
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // Force kill if still running
            {
                int child_status;
                if (::waitpid(pid, &child_status, WNOHANG) == 0)
                {
                    LL_WARNS("LLProcess") << "Force killing " << mDesc << LL_ENDL;
                    (void)::kill(pid, SIGKILL);
                }
            }
#else
            // On Windows, terminate() already does an immediate hard kill via TerminateProcess()
            // Only wait if terminate succeeded (process was running)

            if (!ec)
            {
                DWORD exit_code = 0;
                bool still_running = (::GetExitCodeProcess(mChild->native_handle(), &exit_code)
                    && exit_code == STILL_ACTIVE);
                if (still_running)
                {
                    // We are likely on the main thread. Don't wait long!
                    // Maybe shouldn't wait at all.
                    WaitForSingleObject(mChild->native_handle(), 10);
                }
            }
            else
            {
                LL_WARNS("LLProcess") << "Process " << mDesc
                    << " terminate failed with error " << ec.value()
                    << " (" << ec.message() << "), skipping wait" << LL_ENDL;
            }
#endif
        }
        else if (!mKillCalled)
        {
            LL_INFOS("LLProcess") << "Not terminating " << mDesc
                << " (attached=" << mAttached
                << ", autokill=" << mAutokill << ")" << LL_ENDL;
            // Detach the bp::process so its destructor does not send SIGKILL
            // to the still-running process (boost::process v2 terminates the
            // child in bp::process::~process() if the handle is still valid).
            mChild->detach();
        }
    }

    if (mMainloopConnection.connected())
    {
        mMainloopConnection.disconnect();
    }
}

//static
LLProcessPtr LLProcess::create(const LLSDOrParams& params)
{
    try
    {
        // Construct and then register the mainloop connection separately.
        // connectMainloop() calls shared_from_this(), which requires the
        // shared_ptr control block to be fully initialized — this is only true
        // after make_shared returns, not during the constructor.
        LLProcessPtr ptr = std::make_shared<LLProcess>(params);
        ptr->connectMainloop();
        return ptr;
    }
    catch (const std::exception& e)
    {
        LL_WARNS("LLProcess") << "Failed to create process: " << e.what() << LL_ENDL;

        // Even on failure, fire the postend event if requested, so callers
        // that listen for it can detect the launch failure.
        if (params.postend.isProvided() && !params.postend().empty())
        {
            std::string desc = params.desc.isProvided() ? params.desc() :
                               (params.executable.isProvided() ? LLProcess::basename(params.executable()) : "");
            LLSD event;
            event["desc"]   = desc;
            event["state"]  = LLProcess::UNSTARTED;
            event["string"] = e.what();
            LLEventPumps::instance().obtain(params.postend()).post(event);
        }

        return LLProcessPtr();
    }
}

void LLProcess::launch(const LLSDOrParams& params)
{
    if (!params.validateBlock(true))
    {
        LL_WARNS("LLProcess") << "Failed parameter validation " << LLSDNotationStreamer(params) << LL_ENDL;
        throw std::runtime_error("not launched: failed parameter validation\n");
    }

    // Validate FileParam types before attempting to launch
    int file_idx = 0;
    for (const auto& fparam : params.files)
    {
        if (fparam.type.isProvided())
        {
            const std::string& type = fparam.type();
            // Only "" (inherit) and "pipe" are supported
            if (!type.empty() && type != "pipe")
            {
                std::string slotname;
                switch (file_idx)
                {
                case STDIN: slotname = "stdin"; break;
                case STDOUT: slotname = "stdout"; break;
                case STDERR: slotname = "stderr"; break;
                default: slotname = STRINGIZE("file slot " << file_idx); break;
                }

                LL_WARNS("LLProcess") << "For " << params.executable()
                    << ": unsupported FileParam for " << slotname
                    << ": type='" << type << "'";

                if (fparam.name.isProvided())
                {
                    LL_CONT << ", name='" << fparam.name() << "'";
                }

                LL_CONT << LL_ENDL;

                throw std::runtime_error(
                    STRINGIZE("unsupported FileParam type '" << type
                        << "' for " << slotname));
            }

            // Warn about internal pipe names (not yet supported)
            if (type == "pipe" && fparam.name.isProvided() && !fparam.name().empty())
            {
                LL_WARNS("LLProcess") << "Internal pipe name '" << fparam.name()
                    << "' not yet supported; ignoring" << LL_ENDL;
            }
        }
        file_idx++;
    }

    // Build arguments vector
    std::vector<std::string> args;
    for (const auto& arg : params.args)
    {
        args.push_back(arg);
    }

    // Determine pipe configuration
    bool use_stdin_pipe = false;
    bool use_stdout_pipe = false;
    bool use_stderr_pipe = false;

    file_idx = 0;
    for (const auto& fparam : params.files)
    {
        if (fparam.type.isProvided() && fparam.type() == "pipe")
        {
            switch (file_idx)
            {
            case STDIN: use_stdin_pipe = true; break;
            case STDOUT: use_stdout_pipe = true; break;
            case STDERR: use_stderr_pipe = true; break;
            }
        }
        file_idx++;
    }

    // Create pipes if needed - v2 uses asio pipes directly
    // NOTE: From parent's perspective: write to stdin, read from stdout/stderr
    if (use_stdin_pipe)
    {
        mStdinPipe = std::make_shared<asio::writable_pipe>(mIOContext);
        LL_DEBUGS("LLProcess") << "Created stdin pipe for " << mDesc << LL_ENDL;
    }
    if (use_stdout_pipe)
    {
        mStdoutPipe = std::make_shared<asio::readable_pipe>(mIOContext);
        LL_DEBUGS("LLProcess") << "Created stdout pipe for " << mDesc << LL_ENDL;
    }
    if (use_stderr_pipe)
    {
        mStderrPipe = std::make_shared<asio::readable_pipe>(mIOContext);
        LL_DEBUGS("LLProcess") << "Created stderr pipe for " << mDesc << LL_ENDL;
    }

    // Build the process
    try
    {
#if !LL_WINDOWS
        // Ignore SIGPIPE so that writing to a child's closed stdin doesn't
        // terminate the viewer process. The write will fail with EPIPE instead.
        signal(SIGPIPE, SIG_IGN);
#endif

        // Create child process with appropriate redirections.
        // v2 uses process_stdio for I/O redirection
        bp::process_stdio stdio;

        if (use_stdin_pipe)
            stdio.in = *mStdinPipe;
        else
            stdio.in = nullptr; // inherit

        if (use_stdout_pipe)
            stdio.out = *mStdoutPipe;
        else
            stdio.out = nullptr; // inherit

        if (use_stderr_pipe)
            stdio.err = *mStderrPipe;
        else
            stdio.err = nullptr; // inherit

        // Build executable path.
        // Resolve bare executable names (i.e. "outleap-agent") through the
        // environment's PATH
        boost::filesystem::path executable_path(params.executable());
        if (!executable_path.has_parent_path())
        {
            boost::filesystem::path resolved =
                bp::environment::find_executable(executable_path);
            if (!resolved.empty())
            {
                executable_path = resolved;
            }
            else
            {
                LL_WARNS("LLProcess") << "Could not locate '" << params.executable()
                    << "' on PATH -- launch will likely fail" << LL_ENDL;
                // Let bp::process try to produce an error.
            }
        }

        // In Boost.Process v2, error_code cannot be passed as an initializer.
        // Use the throwing overload and catch the exception instead.
        try
        {
            if (params.cwd.isProvided())
            {
                mChild = std::make_unique<bp::process>(
                    mIOContext,
                    executable_path,
                    args,
                    bp::process_start_dir(params.cwd()),
                    stdio
                );
            }
            else
            {
                mChild = std::make_unique<bp::process>(
                    mIOContext,
                    executable_path,
                    args,
                    stdio
                );
            }
        }
        catch (const boost::system::system_error& ex)
        {
            throw std::runtime_error(STRINGIZE("failed to launch " << params.executable()
                << ": " << ex.what()));
        }

#if LL_WINDOWS
        // Add the process to the job object so it terminates when parent dies.
        // This is done for all processes with autokill=true (the default).
        // Job objects are the Windows-recommended way to ensure child processes
        // don't become orphaned if the parent crashes or is killed.
        if (mAutokill && mChild)
        {
            AssignProcessToJob(mChild->native_handle(), mDesc);
        }
#else
        // boost::process v2 may install a SIGCHLD handler via boost::asio
        // without SA_RESTART when mIOContext is passed to bp::process.
        // Without SA_RESTART, blocking waitpid() calls elsewhere in the
        // process return EINTR when a child exits. Ensure SA_RESTART is set.
        {
            struct sigaction sa_chld;
            if (sigaction(SIGCHLD, nullptr, &sa_chld) != 0)
            {
                LL_WARNS("LLProcess") << "Failed to read SIGCHLD disposition: "
                    << strerror(errno) << LL_ENDL;
            }
            else if (!(sa_chld.sa_flags & SA_RESTART))
            {
                sa_chld.sa_flags |= SA_RESTART;
                if (sigaction(SIGCHLD, &sa_chld, nullptr) != 0)
                {
                    LL_WARNS("LLProcess") << "Failed to set SA_RESTART on SIGCHLD: "
                        << strerror(errno) << LL_ENDL;
                }
            }
        }
#endif

        mStatus.mState = RUNNING;

        // Create pipe wrappers
        if (mStdinPipe)
        {
            mWritePipe = std::make_unique<WritePipeImpl>(
                STRINGIZE(mDesc << " stdin"),
                mStdinPipe
            );
        }
        if (mStdoutPipe)
        {
            mStdoutReadPipe = std::make_unique<ReadPipeImpl>(
                STRINGIZE(mDesc << " stdout"),
                mStdoutPipe,
                STDOUT
            );
        }
        if (mStderrPipe)
        {
            mStderrReadPipe = std::make_unique<ReadPipeImpl>(
                STRINGIZE(mDesc << " stderr"),
                mStderrPipe,
                STDERR
            );
        }

        LL_INFOS("LLProcess") << "Launched " << mDesc
            << " (PID: " << mChild->id() << ")" << LL_ENDL;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(STRINGIZE("failed to create process: " << e.what()));
    }
}

void LLProcess::connectMainloop()
{
    // Capture a weak_ptr to prevent use-after-free: a listener responding to
    // a synchronous event post inside tick() may drop the last LLProcessPtr,
    // destroying this object while tick() is on the call stack. Locking the
    // weak_ptr before calling tick() keeps *this alive for the duration of
    // the callback.
    // NOTE: shared_from_this() is only valid after the shared_ptr owning this
    // object has been fully constructed, so this must be called from create()
    // rather than from the constructor.
    std::weak_ptr<LLProcess> weak = shared_from_this();
    mMainloopConnection = LLEventPumps::instance().obtain("mainloop")
        .listen(LLEventPump::inventName("LLProcess"),
            [weak](const LLSD&)
            {
                // Lock weak_ptr to keep *this alive during tick(), preventing
                // destruction mid-callback if a listener drops the last LLProcessPtr.
                auto self = weak.lock();
                if (self) self->tick();
                return false;
            });
}

void LLProcess::tick()
{
    // Initiate pending stdin writes before draining the I/O context so that
    // self-chained async writes can keep advancing within the same tick
    // instead of waiting for the next mainloop frame.
    if (mWritePipe)
        mWritePipe->tick();

    mIOContext.restart();
    while (mIOContext.poll_one() > 0)
    {
        // Keep polling until no more handlers are ready
    }

#if LL_WINDOWS
    // Check process status
    if (mChild && mStatus.mState == RUNNING && !mChild->running())
    {
        // Process has exited.
        // We are on the main thread, so get exit code without blocking
        DWORD exit_code = STILL_ACTIVE;
        if (::GetExitCodeProcess(mChild->native_handle(), &exit_code))
        {
            if (exit_code != STILL_ACTIVE)
            {
                // Exit code is available
                Status exitStatus;
                exitStatus.mState = EXITED;
                exitStatus.mData = static_cast<int>(exit_code);
                handleExit(exitStatus);
            }
            else
            {
                // Exit code not ready yet, will retry next tick
                // This shouldn't happen if mChild->running() returned false,
                // but handle it gracefully
                LL_DEBUGS("LLProcess") << "Process " << mDesc
                    << " exited but exit code not ready yet" << LL_ENDL;
            }
        }
        else
        {
            LL_WARNS("LLProcess") << "GetExitCodeProcess failed for " << mDesc
                << ": error " << ::GetLastError() << LL_ENDL;
            // Synthesize exit status
            Status exitStatus;
            exitStatus.mState = EXITED;
            exitStatus.mData = -1;
            handleExit(exitStatus);
        }
    }
#else
    // Check process status using WNOHANG to avoid blocking or generating
    // signals that interfere with other waitpid() callers.
    if (mChild && mStatus.mState == RUNNING)
    {
        int status = 0;
        pid_t result;
        // Retry on EINTR: SA_RESTART only applies to blocking calls, not
        // WNOHANG waitpid(), so manual retry is still needed.
        do { // manual EINTR retry (SA_RESTART does not apply to WNOHANG)
            result = ::waitpid(mChild->id(), &status, WNOHANG);
        } while (result == -1 && errno == EINTR);

        if (result == mChild->id())
        {
            // Child has exited; decode exit status now (before handleExit,
            // since the child has already been reaped by this waitpid call).
            Status exitStatus;
            if (WIFEXITED(status))
            {
                exitStatus.mState = EXITED;
                exitStatus.mData = WEXITSTATUS(status);
            }
            else if (WIFSIGNALED(status))
            {
                exitStatus.mState = KILLED;
                exitStatus.mData = WTERMSIG(status);
            }
            else
            {
                exitStatus.mState = EXITED;
                exitStatus.mData = 0;
            }
            handleExit(exitStatus);
        }
        else if (result == -1 && errno == ECHILD)
        {
            // The zombie was already reaped by someone else (e.g. a SIGCHLD
            // handler from APR or another library). We can't determine the
            // real exit code; synthesize EXITED/0 so the process is no longer
            // considered "running" and waitfor() doesn't spin for 60 seconds.
            Status exitStatus;
            exitStatus.mState = EXITED;
            exitStatus.mData = 0;
            handleExit(exitStatus);
        }
        // result == 0 means still running
    }
#endif

    // Keep pumping after process exit until all ReadPipes report EOF, then
    // disconnect from mainloop to avoid losing trailing EOF notifications.
    if (mStatus.mState != RUNNING && mMainloopConnection.connected())
    {
        bool stdout_eof = (!mStdoutReadPipe || mStdoutReadPipe->atEOF());
        bool stderr_eof = (!mStderrReadPipe || mStderrReadPipe->atEOF());
        if (stdout_eof && stderr_eof)
        {
            mMainloopConnection.disconnect();
        }
    }
}

void LLProcess::handleExit(Status exitStatus)
{
    if (mStatus.mState != RUNNING)
        return; // Already handled

    mStatus = exitStatus;

    // Drain any remaining pipe handlers before notifying callers. LLLeap
    // consumes child stdout/stderr from these callbacks, and some tests write
    // enough data to require far more than a handful of async-read completions
    // after the child has already exited.
    if (mIOContext.stopped())
    {
        // If the io_context has reached the stopped state,
        // the following loop will never run and trailing stdout/stderr
        // handlers (and EOF notifications) may be missed.
        LL_INFOS("LLProcess") << "mIOContext was stopped on exit, restarting" << LL_ENDL;
        mIOContext.restart();
    }
    while (mIOContext.poll_one() > 0)
    {
        // Keep polling until no more handlers are immediately ready.
    }

    LL_INFOS("LLProcess") << getStatusString(mStatus) << LL_ENDL;

    // Post to event pump if configured
    if (!mPostend.empty())
    {
        LLSD event;
        event["id"] = static_cast<int>(getProcessID());
        event["desc"] = mDesc;
        event["state"] = mStatus.mState;
        event["data"] = mStatus.mData;
        event["string"] = getStatusString(mStatus);

        LLEventPumps::instance().obtain(mPostend).post(event);
    }

    // Leave mainloop connected until tick() observes EOF on all ReadPipes.
}

bool LLProcess::isRunning() const
{
    return mStatus.mState == RUNNING;
}

//static
bool LLProcess::isRunning(const LLProcessPtr& ptr)
{
    return ptr && ptr->isRunning();
}

LLProcess::Status LLProcess::getStatus() const
{
    return mStatus;
}

//static
LLProcess::Status LLProcess::getStatus(const LLProcessPtr& ptr)
{
    if (!ptr)
    {
        Status status;
        status.mState = UNSTARTED;
        return status;
    }
    return ptr->getStatus();
}

std::string LLProcess::getStatusString() const
{
    return getStatusString(mDesc, mStatus);
}

//static
std::string LLProcess::getStatusString(const std::string& desc, const LLProcessPtr& ptr)
{
    return getStatusString(desc, getStatus(ptr));
}

std::string LLProcess::getStatusString(const Status& status) const
{
    return getStatusString(mDesc, status);
}

//static
std::string LLProcess::getStatusString(const std::string& desc, const Status& status)
{
    std::string result = desc + ": ";
    switch (status.mState)
    {
    case UNSTARTED: return result + "not started";
    case RUNNING: return result + "running";
    case EXITED: return result + STRINGIZE("exited with code " << status.mData);
    case KILLED: return result + STRINGIZE("killed by signal " << status.mData);
    default: return result + "unknown state";
    }
}

bool LLProcess::kill(const std::string& who)
{
    if (!mChild || mStatus.mState != RUNNING)
        return true;

    LL_INFOS("LLProcess") << who << " killing " << mDesc << LL_ENDL;

#if LL_WINDOWS
    // Call TerminateProcess directly with exit code (UINT)-1 so that
    // tick()'s GetExitCodeProcess reads back -1 as a signed int, matching
    // the original APR-based behavior expected by tests and callers.
    // Do NOT use mChild->terminate(): boost::process v2 may use a different
    // exit code (e.g. EXIT_FAILURE=1) or invalidate the handle internally,
    // which would break the exit-code check in tick().
    if (!::TerminateProcess(mChild->native_handle(), (UINT)-1))
    {
        LL_WARNS("LLProcess") << "Failed to terminate " << mDesc
            << ": error " << ::GetLastError() << LL_ENDL;
        return false;
    }
#else
    // Send SIGTERM so the child can clean up gracefully, and so tick()'s
    // waitpid() reports WIFSIGNALED/WTERMSIG == SIGTERM rather than SIGKILL.
    // Do NOT call mChild->terminate(): in boost::process v2, that function
    // sends SIGKILL and may set the internal pid to -1, which would cause
    // tick()'s waitpid(mChild->id(), ...) to wait for any child (-1) and
    // never match the result against the stored pid.
    pid_t pid = mChild->id();
    if (::kill(pid, SIGTERM) != 0 && errno != ESRCH)
    {
        LL_WARNS("LLProcess") << "Failed to send SIGTERM to " << mDesc
            << " (pid " << pid << "): " << strerror(errno) << LL_ENDL;
        return false;
    }
#endif

    // Mark as killed so the destructor doesn't repeat the termination attempt,
    // but a better idea might be to modify mState.
    mKillCalled = true;
    // Don't set status here - let handleExit() do it when the process actually terminates
    // so that it will be able to post EOF event.
    // At this point in time mStatus.mState is still RUNNING, so this is basically
    // retuning false.
    return !isRunning();
}

//static
bool LLProcess::kill(const LLProcessPtr& ptr, const std::string& who)
{
    return !ptr || ptr->kill(who);
}

void LLProcess::pump()
{
    tick();
}

LLProcess::id LLProcess::getProcessID() const
{
    if (!mChild)
        return 0;

#if LL_WINDOWS
    return static_cast<int>(mChild->id());
#else
    return mChild->id();
#endif
}

LLProcess::handle LLProcess::getProcessHandle() const
{
    if (!mChild)
        return 0;

#if LL_WINDOWS
    // Duplicate the process handle so the caller owns an independent copy.
    // boost::process v2's process destructor always closes the handle
    // (even after process::detach()), so the raw native_handle() becomes invalid
    // once ~process() runs. DuplicateHandle gives the caller a handle whose
    // lifetime is not tied to boost's internal process cleanup.
    HANDLE source = mChild->native_handle();
    if (!source || source == INVALID_HANDLE_VALUE)
        return 0;
    HANDLE dup = nullptr;
    if (!::DuplicateHandle(
            ::GetCurrentProcess(), source,
            ::GetCurrentProcess(), &dup,
            PROCESS_QUERY_INFORMATION | SYNCHRONIZE,
            FALSE, 0))
    {
        LL_WARNS("LLProcess") << "DuplicateHandle failed for " << mDesc
            << ": error " << ::GetLastError() << LL_ENDL;
        return 0;
    }
    return dup;
#else
    return mChild->id();
#endif
}

//static
LLProcess::handle LLProcess::isRunning(handle h, const std::string& desc)
{
#if LL_WINDOWS
    if (h == 0 || h == INVALID_HANDLE_VALUE)
        return 0;

    DWORD exit_code;
    if (GetExitCodeProcess(h, &exit_code))
    {
        if (exit_code == STILL_ACTIVE)
            return h;   // process still running
        // Process has exited: close the duplicated handle (see getProcessHandle()).
    }
    // Either process exited or GetExitCodeProcess failed; either way we're done.
    CloseHandle(h);
    return 0;
#else
    if (h == 0)
        return 0;

    // Use waitpid with WNOHANG to check if process is still running
    // This is more reliable than kill(pid, 0) and properly reaps zombies
    int status;
    pid_t result;

    // Retry on EINTR (interrupted system call)
    do
    {
        result = waitpid(h, &status, WNOHANG);
    } while (result == -1 && errno == EINTR);

    if (result == 0)
    {
        // Process still running
        return h;
    }
    else if (result == h)
    {
        // Process has terminated (and we've reaped it)
        return 0;
    }
    else if (result == -1)
    {
        // Error occurred
        if (errno == ECHILD)
        {
            // Process doesn't exist or was already reaped
            return 0;
        }
        // For other errors, assume process is gone
        LL_WARNS("LLProcess") << "waitpid(" << h << ") failed: "
            << strerror(errno) << LL_ENDL;
        return 0;
    }

    // Shouldn't get here, but if we do, assume process is gone
    return 0;
#endif
}

std::string LLProcess::getPipeName(FILESLOT slot) const
{
    // Named pipes not yet implemented in this PoC
    return "";
}

LLProcess::WritePipe& LLProcess::getWritePipe(FILESLOT slot)
{
    if (slot >= NSLOTS)
        throw NoPipe(STRINGIZE(mDesc << ": no slot " << slot));

    if (slot == STDIN)
    {
        if (!mWritePipe)
            throw NoPipe(STRINGIZE(mDesc << ": stdin is not a monitored pipe"));
        return *mWritePipe;
    }

    // STDOUT or STDERR slots: neither has a WritePipe.
    // Distinguish "not piped at all" from "piped but wrong direction".
    const char* slotname = (slot == STDOUT) ? "stdout" : "stderr";
    bool is_piped = (slot == STDOUT) ? bool(mStdoutReadPipe) : bool(mStderrReadPipe);
    if (!is_piped)
        throw NoPipe(STRINGIZE(mDesc << ": " << slotname << " is not a monitored pipe"));
    else
        throw NoPipe(STRINGIZE(mDesc << ": " << slotname << " is a ReadPipe, not a WritePipe"));
}

LLProcess::ReadPipe& LLProcess::getReadPipe(FILESLOT slot)
{
    if (slot >= NSLOTS)
        throw NoPipe(STRINGIZE(mDesc << ": no slot " << slot));

    if (slot == STDIN)
        throw NoPipe(STRINGIZE(mDesc << ": ReadPipe is invalid for stdin"));

    // At this point, slot must be STDOUT or STDERR
    // Check if a pipe was configured for this slot
    if (slot == STDOUT)
    {
        if (!mStdoutReadPipe)
            throw NoPipe(STRINGIZE(mDesc << ": stdout is not a monitored pipe"));
        return *mStdoutReadPipe;
    }
    else if (slot == STDERR)
    {
        if (!mStderrReadPipe)
            throw NoPipe(STRINGIZE(mDesc << ": stderr is not a monitored pipe"));
        return *mStderrReadPipe;
    }
    else
    {
        // This should never happen given the checks above, but handle it anyway
        throw NoPipe(STRINGIZE(mDesc << ": no slot " << slot));
    }
}

LLProcess::WritePipe* LLProcess::getOptWritePipe(FILESLOT slot)
{
    if (slot >= NSLOTS)
    {
        LL_WARNS("LLProcess") << mDesc << ": no slot " << slot << LL_ENDL;
        return nullptr;
    }

    if (slot == STDIN)
    {
        if (!mWritePipe)
        {
            LL_WARNS("LLProcess") << mDesc << ": stdin is not a monitored pipe" << LL_ENDL;
            return nullptr;
        }
        return mWritePipe.get();
    }

    // STDOUT or STDERR slots: neither has a WritePipe.
    const char* slotname = (slot == STDOUT) ? "stdout" : "stderr";
    bool is_piped = (slot == STDOUT) ? bool(mStdoutReadPipe) : bool(mStderrReadPipe);
    if (!is_piped)
        LL_WARNS("LLProcess") << mDesc << ": " << slotname << " is not a monitored pipe" << LL_ENDL;
    else
        LL_WARNS("LLProcess") << mDesc << ": " << slotname << " is a ReadPipe, not a WritePipe" << LL_ENDL;
    return nullptr;
}

LLProcess::ReadPipe* LLProcess::getOptReadPipe(FILESLOT slot)
{
    if (slot >= NSLOTS)
    {
        LL_WARNS("LLProcess") << mDesc << ": no slot " << slot << LL_ENDL;
        return nullptr;
    }

    if (slot == STDIN)
    {
        LL_WARNS("LLProcess") << mDesc << ": ReadPipe is invalid for stdin" << LL_ENDL;
        return nullptr;
    }

    if (slot == STDOUT)
    {
        if (!mStdoutReadPipe)
        {
            LL_WARNS("LLProcess") << mDesc << ": stdout is not a monitored pipe" << LL_ENDL;
            return nullptr;
        }
        return mStdoutReadPipe.get();
    }
    else if (slot == STDERR)
    {
        if (!mStderrReadPipe)
        {
            LL_WARNS("LLProcess") << mDesc << ": stderr is not a monitored pipe" << LL_ENDL;
            return nullptr;
        }
        return mStderrReadPipe.get();
    }
    else
    {
        LL_WARNS("LLProcess") << mDesc << ": no slot " << slot << LL_ENDL;
        return nullptr;
    }
}

//static
std::string LLProcess::basename(const std::string& path)
{
    std::string::size_type delim = path.find_last_of("\\/");
    if (delim == std::string::npos)
        return path;
    return path.substr(delim + 1);
}

//static
std::string LLProcess::getline(std::istream& in)
{
    std::string line;
    std::getline(in, line);
    // Trim trailing \r for cross-platform compatibility
    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    return line;
}
