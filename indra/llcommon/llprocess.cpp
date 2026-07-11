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
#include "llexception.h"
#include "stringize.h"

#include <boost/process.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/args.hpp>
#include <boost/process/v1/start_dir.hpp>
#include <boost/process/v1/search_path.hpp>
#include <boost/process/v1/async.hpp>
#include <boost/process/v1/async_pipe.hpp>
#include <boost/asio.hpp>
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


namespace bp = boost::process::v1;
namespace asio = boost::asio;

/*****************************************************************************
*   Helpers
*****************************************************************************/
/*
static const char* whichfile_[] = { "stdin", "stdout", "stderr" };

static LLProcess::Status interpret_status(int status);
static std::string getDesc(const LLProcess::Params& params);

static std::string whichfile(LLProcess::FILESLOT index)
{
    if (index < LL_ARRAY_SIZE(whichfile_))
        return whichfile_[index];
    return STRINGIZE("file slot " << index);
}*/

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
        std::shared_ptr<bp::async_pipe> pipe) :
        mDesc(desc),
        mPipe(pipe),
        mStream(&mStreambuf),
        mWritePending(false)
    {
        // Start async write monitoring
        startAsyncWrite();
    }

    virtual ~WritePipeImpl() = default;

    virtual std::ostream& get_ostream() override { return mStream; }

    virtual size_type size() const override
    {
        return mStreambuf.size();
    }

    void tick()
    {
        // Don't start a new write if one is already pending
        if (mWritePending || !mPipe || !mPipe->is_open() || mStreambuf.size() == 0)
            return;

        mWritePending = true;

        // Write buffered data asynchronously
        asio::async_write(*mPipe, mStreambuf.data(),
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

private:
    void startAsyncWrite()
    {
        if (!mPipe || !mPipe->is_open() || mStreambuf.size() == 0)
            return;

        // Write buffered data asynchronously
        asio::async_write(*mPipe, mStreambuf.data(),
            [this](const boost::system::error_code& ec, std::size_t bytes_transferred)
        {
            if (!ec)
            {
                mStreambuf.consume(bytes_transferred);
                LL_DEBUGS("LLProcess") << "Wrote " << bytes_transferred
                    << " bytes to " << mDesc << LL_ENDL;
                // Continue writing if there's more data
                startAsyncWrite();
            }
            else if (ec != asio::error::operation_aborted)
            {
                LL_WARNS("LLProcess") << "Write error on " << mDesc
                    << ": " << ec.message() << LL_ENDL;
            }
        });
    }

    std::string mDesc;
    std::shared_ptr<bp::async_pipe> mPipe;
    asio::streambuf mStreambuf;
    std::ostream mStream;
    bool mWritePending;
};

class ReadPipeImpl : public LLProcess::ReadPipe
{
    LOG_CLASS(ReadPipeImpl);
public:
    ReadPipeImpl(const std::string& desc,
        std::shared_ptr<bp::async_pipe> pipe,
        LLProcess::FILESLOT slot) :
        mDesc(desc),
        mPipe(pipe),
        mSlot(slot),
        mStream(&mStreambuf),
        mPump("ReadPipe", true),   // tweak name as needed to avoid collisions, use LLEventPump::inventName?
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

        size_type to_read = (mLimit > 0 && mStreambuf.size() >= mLimit) ? 0 : 4096;
        if (to_read == 0)
            return;

        auto bufs = mStreambuf.prepare(to_read);

        mPipe->async_read_some(bufs,
            [this](const boost::system::error_code& ec, std::size_t bytes_transferred)
        {
            if (!ec)
            {
                mStreambuf.commit(bytes_transferred);
                LL_DEBUGS("LLProcess") << "Read " << bytes_transferred
                    << " bytes from " << mDesc << LL_ENDL;

                LLSD event;
                event["len"] = LLSD::Integer(mStreambuf.size());
                event["slot"] = LLSD::Integer(mSlot);
                event["desc"] = mDesc;

                if (mLimit > 0)
                {
                    size_type data_len = (std::min)(mStreambuf.size(), mLimit);
                    event["data"] = peek(0, data_len);
                }

                mPump.post(event);
                startAsyncRead();
            }
            else if (ec == asio::error::eof)
            {
                if (bytes_transferred > 0)
                {
                    mStreambuf.commit(bytes_transferred);

                    LLSD event;
                    event["len"] = LLSD::Integer(mStreambuf.size());
                    event["slot"] = LLSD::Integer(mSlot);
                    event["desc"] = mDesc;

                    if (mLimit > 0)
                    {
                        size_type data_len = (std::min)(mStreambuf.size(), mLimit);
                        event["data"] = peek(0, data_len);
                    }

                    mPump.post(event);
                }

                mEOF = true;
                LL_DEBUGS("LLProcess") << "EOF on " << mDesc << LL_ENDL;

                LLSD eof_event;
                eof_event["eof"] = true;
                eof_event["slot"] = LLSD::Integer(mSlot);
                eof_event["desc"] = mDesc;
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
    std::shared_ptr<bp::async_pipe> mPipe;
    LLProcess::FILESLOT mSlot;
    mutable asio::streambuf mStreambuf;
    std::istream mStream;
    LLEventStream mPump; //  pump specific to this pipe
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
    mAttached(params.attached)
{
    launch(params);
}

LLProcess::~LLProcess()
{
    if (mChild && mStatus.mState == RUNNING)
    {
        if (mAttached && mAutokill)
        {
            LL_INFOS("LLProcess") << "Terminating child process " << mDesc << LL_ENDL;
            std::error_code ec;
            mChild->terminate(ec);

#if !LL_WINDOWS
            // On POSIX, terminate() sends SIGTERM which allows graceful shutdown.
            // Poll with waitpid(WNOHANG) rather than mChild->running() to avoid
            // competing with tick()'s own waitpid call.
            pid_t pid = mChild->id();
            for (int i = 0; i < 30; ++i)
            {
                int child_status;
                if (::waitpid(pid, &child_status, WNOHANG) == pid)
                    break; // child exited
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
            // Wait briefly to ensure termination completes
            WaitForSingleObject(mChild->native_handle(), 100);
#endif
        }
        else
        {
            LL_INFOS("LLProcess") << "Not terminating " << mDesc
                << " (attached=" << mAttached
                << ", autokill=" << mAutokill << ")" << LL_ENDL;
            // Detach the bp::child so its destructor does not send SIGKILL
            // to the still-running process (boost::process v1 terminates the
            // child in bp::child::~child() if the handle is still valid).
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
        return std::make_shared<LLProcess>(params);
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

void LLProcess::launch(const Params& params)
{
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

    // Create pipes if needed
    if (use_stdin_pipe)
    {
        mStdinPipe = std::make_unique<bp::async_pipe>(mIOContext);
        LL_DEBUGS("LLProcess") << "Created stdin pipe for " << mDesc << LL_ENDL;
    }
    if (use_stdout_pipe)
    {
        mStdoutPipe = std::make_unique<bp::async_pipe>(mIOContext);
        LL_DEBUGS("LLProcess") << "Created stdout pipe for " << mDesc << LL_ENDL;
    }
    if (use_stderr_pipe)
    {
        mStderrPipe = std::make_unique<bp::async_pipe>(mIOContext);
        LL_DEBUGS("LLProcess") << "Created stderr pipe for " << mDesc << LL_ENDL;
    }

    // Build the process
    try
    {
        std::error_code ec;

#if !LL_WINDOWS
        // Ignore SIGPIPE so that writing to a child's closed stdin doesn't
        // terminate the viewer process. The write will fail with EPIPE instead.
        signal(SIGPIPE, SIG_IGN);
#endif

        // Create child process with appropriate redirections.
        // Do NOT pass mIOContext to bp::child: boost::process v1 would then
        // install a SIGCHLD handler via boost::asio without SA_RESTART, which
        // causes blocking waitpid() calls elsewhere to return EINTR. We detect
        // child exit ourselves via waitpid(WNOHANG) in tick() instead.
        //
        // Use a generic lambda to avoid repeating the executable/args/cwd for
        // each of the 8 pipe-combination branches.
        auto make_child = [&](auto&&... redirects) {
            if (params.cwd.isProvided())
                mChild = std::make_unique<bp::child>(
                    params.executable(),
                    bp::args(args),
                    bp::start_dir(params.cwd()),
                    std::forward<decltype(redirects)>(redirects)...,
                    ec
                );
            else
                mChild = std::make_unique<bp::child>(
                    params.executable(),
                    bp::args(args),
                    std::forward<decltype(redirects)>(redirects)...,
                    ec
                );
        };

        if (use_stdin_pipe && use_stdout_pipe && use_stderr_pipe)
            make_child(bp::std_in  < *mStdinPipe,
                       bp::std_out > *mStdoutPipe,
                       bp::std_err > *mStderrPipe);
        else if (use_stdin_pipe && use_stdout_pipe)
            make_child(bp::std_in  < *mStdinPipe,
                       bp::std_out > *mStdoutPipe);
        else if (use_stdin_pipe && use_stderr_pipe)
            make_child(bp::std_in  < *mStdinPipe,
                       bp::std_err > *mStderrPipe);
        else if (use_stdout_pipe && use_stderr_pipe)
            make_child(bp::std_out > *mStdoutPipe,
                       bp::std_err > *mStderrPipe);
        else if (use_stdin_pipe)
            make_child(bp::std_in  < *mStdinPipe);
        else if (use_stdout_pipe)
            make_child(bp::std_out > *mStdoutPipe);
        else if (use_stderr_pipe)
            make_child(bp::std_err > *mStderrPipe);
        else
            make_child();

        if (ec)
        {
            throw std::runtime_error(STRINGIZE("failed to launch " << params.executable()
                << ": " << ec.message()));
        }

        mStatus.mState = RUNNING;

        // Create pipe wrappers
        if (mStdinPipe)
        {
            mWritePipe = std::make_unique<WritePipeImpl>(
                STRINGIZE(mDesc << " stdin"),
                std::shared_ptr<bp::async_pipe>(mStdinPipe.get(), [](auto*) {})
            );
        }
        if (mStdoutPipe)
        {
            mStdoutReadPipe = std::make_unique<ReadPipeImpl>(
                STRINGIZE(mDesc << " stdout"),
                std::shared_ptr<bp::async_pipe>(mStdoutPipe.get(), [](auto*) {}),
                STDOUT
            );
        }
        if (mStderrPipe)
        {
            mStderrReadPipe = std::make_unique<ReadPipeImpl>(
                STRINGIZE(mDesc << " stderr"),
                std::shared_ptr<bp::async_pipe>(mStderrPipe.get(), [](auto*) {}),
                STDERR
            );
        }

        // Hook into mainloop for I/O processing
        mMainloopConnection = LLEventPumps::instance().obtain("mainloop")
            .listen(LLEventPump::inventName("LLProcess"),
                [this](const LLSD&) { tick(); return false; });

        LL_INFOS("LLProcess") << "Launched " << mDesc
            << " (PID: " << mChild->id() << ")" << LL_ENDL;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(STRINGIZE("failed to create process: " << e.what()));
    }
}

void LLProcess::tick()
{
    // Poll I/O context to process async operations
    while (mIOContext.poll_one() > 0)
    {
        // Keep polling until no more handlers are ready
    }

    if (auto* wp = dynamic_cast<WritePipeImpl*>(mWritePipe.get()))
        wp->tick();

#if LL_WINDOWS
    // Check process status
    if (mChild && mStatus.mState == RUNNING && !mChild->running())
    {
        WaitForSingleObject(mChild->native_handle(), 100);
        Status exitStatus;
        exitStatus.mState = EXITED;
        exitStatus.mData = mChild->exit_code();
        handleExit(exitStatus);
    }
#else
    // Check process status using WNOHANG to avoid blocking or generating
    // signals that interfere with other waitpid() callers.
    if (mChild && mStatus.mState == RUNNING)
    {
        int status = 0;
        pid_t result;
        do {
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
}

void LLProcess::handleExit(Status exitStatus)
{
    if (mStatus.mState != RUNNING)
        return; // Already handled

    mStatus = exitStatus;

    // Drain any remaining data from pipes before notifying callers
    for (int i = 0; i < 10; ++i)
    {
        if (mIOContext.poll_one() == 0)
            break;
    }

    LL_INFOS("LLProcess") << mDesc << " " << getStatusString(mStatus) << LL_ENDL;

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

    // Disconnect from mainloop
    if (mMainloopConnection.connected())
    {
        mMainloopConnection.disconnect();
    }
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
    std::error_code ec;
    mChild->terminate(ec);

    if (ec)
    {
        LL_WARNS("LLProcess") << "Failed to terminate " << mDesc
            << ": " << ec.message() << LL_ENDL;
        return false;
    }
#else
    // Send SIGTERM so the child can clean up gracefully, and so tick()'s
    // waitpid() reports WIFSIGNALED/WTERMSIG == SIGTERM rather than SIGKILL.
    // Do NOT call mChild->terminate(): in boost::process v1, that function
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

    // Don't set status here - let handleExit() do it when the process actually terminates
    return true;
}

//static
bool LLProcess::kill(const LLProcessPtr& ptr, const std::string& who)
{
    return ptr && ptr->kill(who);
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
    return mChild->native_handle();
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
        return (exit_code == STILL_ACTIVE) ? h : 0;
    }
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

/*****************************************************************************
*   Windows specific
*****************************************************************************/
#if LL_WINDOWS
/*
static std::string WindowsErrorString(const std::string& operation);

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
     * /
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
    auto result = GetLastError();
    return STRINGIZE(operation << " failed (" << result << "): "
                     << windows_message<std::string>(result));
}
*/
/*****************************************************************************
*   Posix specific
*****************************************************************************/
#else // Mac and linux
/*
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
        pstatus->mData  = 0;
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
}*/

#endif  // Posix
