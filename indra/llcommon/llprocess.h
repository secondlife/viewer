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
#include "llexception.h"
#include "apr_thread_proc.h"
#include <boost/process.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/args.hpp>
#include <boost/process/v1/start_dir.hpp>
#include <boost/process/v1/search_path.hpp>
#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/optional.hpp>
#include <iosfwd>                   // std::ostream

#if LL_WINDOWS
#include "llwin32headers.h" // for HANDLE
#elif LL_LINUX
#if defined(Status)
#undef Status
#endif
#endif

class LLEventPump;

class LLProcess;
/// LLProcess instances are created on the heap by static factory methods and
/// managed by ref-counted pointers.
typedef std::shared_ptr<LLProcess> LLProcessPtr;

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

class LL_COMMON_API LLProcess
{
    LOG_CLASS(LLProcess);
public:
    /**
     * Specify what to pass for each of child stdin, stdout, stderr.
     */
    struct FileParam : public LLInitParam::Block<FileParam>
    {
        /**
         * type of file handle to pass to child process
         * - "" (default): inherit from parent
         * - "pipe": create a pipe for I/O
         * - "file": open a filesystem file (future enhancement)
         */
        Optional<std::string> type;
        Optional<std::string> name;

        FileParam(const std::string& tp = "", const std::string& nm = "") :
            type("type"),
            name("name")
        {
            if (!tp.empty()) type = tp;
            if (!nm.empty()) name = nm;
        }
    };

    /// Param block definition
    struct Params : public LLInitParam::Block<Params>
    {
        Params() :
            executable("executable"),
            args("args"),
            cwd("cwd"),
            autokill("autokill", true),
            attached("attached", true),
            files("files"),
            postend("postend"),
            desc("desc")
        {
        }

        Mandatory<std::string> executable;
        Multiple<std::string> args;
        Optional<std::string> cwd;
        Optional<bool> autokill;
        Optional<bool> attached;
        Multiple<FileParam, AtMost<3>> files;
        Optional<std::string> postend;
        Optional<std::string> desc;
    };

    typedef LLSDParamAdapter<Params> LLSDOrParams;

    static LLProcessPtr create(const LLSDOrParams& params);
    virtual ~LLProcess();

    /// Is child process still running?
    bool isRunning() const;
    static bool isRunning(const LLProcessPtr&);

    /**
     * State of child process
     */
    enum state
    {
        UNSTARTED,
        RUNNING,
        EXITED,
        KILLED
    };

    /**
     * Status info
     */
    struct Status
    {
        Status() : mState(UNSTARTED), mData(0) {}
        state mState;
        int mData;  // exit code or signal number
    };

    Status getStatus() const;
    static Status getStatus(const LLProcessPtr&);
    std::string getStatusString() const;
    static std::string getStatusString(const std::string& desc, const LLProcessPtr&);
    std::string getStatusString(const Status& status) const;
    static std::string getStatusString(const std::string& desc, const Status& status);

    bool kill(const std::string& who = "");
    static bool kill(const LLProcessPtr& p, const std::string& who = "");

#if LL_WINDOWS
    typedef int id;
    typedef HANDLE handle;
#else
    typedef pid_t id;
    typedef pid_t handle;
#endif

    id getProcessID() const;
    handle getProcessHandle() const;
    static handle isRunning(handle, const std::string& desc = "");

    enum FILESLOT { STDIN = 0, STDOUT = 1, STDERR = 2, NSLOTS = 3 };

    /// Exception thrown by getWritePipe(), getReadPipe() if you didn't ask to
    /// create a pipe at the corresponding FILESLOT.
    struct NoPipe : public LLException
    {
        NoPipe(const std::string& what) : LLException(what) {}
    };

    std::string getPipeName(FILESLOT) const;

    /// Base class for pipes
    class LL_COMMON_API BasePipe
    {
    public:
        virtual ~BasePipe() = default;
        typedef std::size_t size_type;
        static const size_type npos;
        virtual size_type size() const = 0;
    };

    /// Write pipe for stdin
    class WritePipe : public BasePipe
    {
    public:
        virtual std::ostream& get_ostream() = 0;
    };

    /// Read pipe for stdout/stderr
    class ReadPipe : public BasePipe
    {
    public:
        virtual std::istream& get_istream() = 0;
        virtual std::string getline() = 0;
        virtual std::string read(size_type len) = 0;
        virtual std::string peek(size_type offset = 0, size_type len = npos) const = 0;

        template <typename SEEK>
        bool contains(SEEK seek, size_type offset = 0) const
        {
            return find(seek, offset) != npos;
        }

        virtual size_type find(const std::string& seek, size_type offset = 0) const = 0;
        virtual size_type find(char seek, size_type offset = 0) const = 0;

        virtual LLEventPump& getPump() = 0;
        virtual void setLimit(size_type limit) = 0;
        virtual size_type getLimit() const = 0;
    };

    WritePipe& getWritePipe(FILESLOT slot = STDIN);
    ReadPipe& getReadPipe(FILESLOT index);
    WritePipe* getOptWritePipe(FILESLOT slot = STDIN);
    ReadPipe* getOptReadPipe(FILESLOT index);

    static std::string basename(const std::string& path);
    static std::string getline(std::istream& in);

    // Constructor is public for the sake of make_shared
    // but create() should be used instead for proper initialization.
    LLProcess(const Params& params);

private:
    void launch(const Params& params);
    void tick();
    void handleExit(Status exitStatus);

    // Boost.Process components
    boost::asio::io_context mIOContext;
    std::unique_ptr<boost::process::v1::child> mChild;

    // Pipes - using Boost.Process async pipes
    std::unique_ptr<boost::process::v1::async_pipe> mStdinPipe;
    std::unique_ptr<boost::process::v1::async_pipe> mStdoutPipe;
    std::unique_ptr<boost::process::v1::async_pipe> mStderrPipe;

    // Our pipe wrapper implementations
    std::unique_ptr<WritePipe> mWritePipe;
    std::unique_ptr<ReadPipe> mStdoutReadPipe;
    std::unique_ptr<ReadPipe> mStderrReadPipe;

    Status mStatus;
    std::string mDesc;
    std::string mPostend;
    bool mAutokill;
    bool mAttached;

    // For integrating with LLEventPump mainloop
    boost::signals2::scoped_connection mMainloopConnection;
};

/// for logging
LL_COMMON_API std::ostream& operator<<(std::ostream&, const LLProcess::Params&);

#endif // LL_LLPROCESS_H
