/**
 * @file   llleap.cpp
 * @author Nat Goodspeed
 * @date   2012-02-20
 * @brief  Implementation for llleap.
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llleap.h"
// STL headers
#include <sstream>
#include <algorithm>
// std headers
// external library headers
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/tokenizer.hpp>
// other Linden headers
#include "llerror.h"
#include "llstring.h"
#include "llprocess.h"
#include "llevents.h"
#include "stringize.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "llerrorcontrol.h"
#include "lltimer.h"
#include "lluuid.h"
#include "llleaplistener.h"

#if LL_MSVC
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

LLLeap::LLLeap() {}
LLLeap::~LLLeap() {}

class LLLeapImpl: public LLLeap
{
    LOG_CLASS(LLLeap);
public:
    // Called only by LLLeap::create()
    LLLeapImpl(const std::string& desc, const std::vector<std::string>& plugin):
        // We might reassign mDesc in the constructor body if it's empty here.
        mDesc(desc),
        // We expect multiple LLLeapImpl instances. Definitely tweak
        // mDonePump's name for uniqueness.
        mDonePump("LLLeap", true),
        // Troubling thought: what if one plugin intentionally messes with
        // another plugin? LLEventPump names are in a single global namespace.
        // Try to make that more difficult by generating a UUID for the reply-
        // pump name -- so it should NOT need tweaking for uniqueness.
        mReplyPump(LLUUID::generateNewID().asString()),
        mExpect(0),
        mPrevFatalFunction(LLError::getFatalFunction()),
        // Instantiate a distinct LLLeapListener for this plugin. (Every
        // plugin will want its own collection of managed listeners, etc.)
        // Pass it a callback to our connect() method, so it can send events
        // from a particular LLEventPump to the plugin without having to know
        // this class or method name.
        mListener(new LLLeapListener(boost::bind(&LLLeapImpl::connect, this, _1, _2)))
    {
        // Rule out empty vector
        if (plugin.empty())
        {
            throw Error("no plugin command");
        }

        // Don't leave desc empty either, but in this case, if we weren't
        // given one, we'll fake one.
        if (desc.empty())
        {
            mDesc = LLProcess::basename(plugin[0]);
            // how about a toLower() variant that returns the transformed string?!
            std::string desclower(mDesc);
            LLStringUtil::toLower(desclower);
            // If we're running a Python script, use the script name for the
            // desc instead of just 'python'. Arguably we should check for
            // more different interpreters as well, but there's a reason to
            // notice Python specially: we provide Python LLSD serialization
            // support, so there's a pretty good reason to implement plugins
            // in that language.
            if (plugin.size() >= 2 && (desclower == "python" || desclower == "python.exe"))
            {
                mDesc = LLProcess::basename(plugin[1]);
            }
        }

        // Listen for child "termination" right away to catch launch errors.
        mDonePump.listen("LLLeap", boost::bind(&LLLeapImpl::bad_launch, this, _1));

        // Okay, launch child.
        LLProcess::Params params;
        params.desc = mDesc;
        std::vector<std::string>::const_iterator pi(plugin.begin()), pend(plugin.end());
        params.executable = *pi++;
        for ( ; pi != pend; ++pi)
        {
            params.args.add(*pi);
        }
        params.files.add(LLProcess::FileParam("pipe")); // stdin
        params.files.add(LLProcess::FileParam("pipe")); // stdout
        params.files.add(LLProcess::FileParam("pipe")); // stderr
        params.postend = mDonePump.getName();
        mChild = LLProcess::create(params);
        // If that didn't work, no point in keeping this LLLeap object.
        if (! mChild)
        {
            throw Error(STRINGIZE("failed to run " << mDesc));
        }

        // Okay, launch apparently worked. Change our mDonePump listener.
        mDonePump.stopListening("LLLeap");
        mDonePump.listen("LLLeap", boost::bind(&LLLeapImpl::done, this, _1));

        // Child might pump large volumes of data through either stdout or
        // stderr. Don't bother copying all that data into notification event.
        LLProcess::ReadPipe
            &childout(mChild->getReadPipe(LLProcess::STDOUT)),
            &childerr(mChild->getReadPipe(LLProcess::STDERR));
        childout.setLimit(20);
        childerr.setLimit(20);

        // Serialize any event received on mReplyPump to our child's stdin.
        mStdinConnection = connect(mReplyPump, "LLLeap");

        // Listening on stdout is stateful. In general, we're either waiting
        // for the length prefix or waiting for the specified length of data.
        // We address that with two different listener methods -- one of which
        // is blocked at any given time.
        mStdoutConnection = childout.getPump()
            .listen("prefix", boost::bind(&LLLeapImpl::rstdout, this, _1));
        mStdoutDataConnection = childout.getPump()
            .listen("data",   boost::bind(&LLLeapImpl::rstdoutData, this, _1));
        mBlocker.reset(new LLEventPump::Blocker(mStdoutDataConnection));

        // Log anything sent up through stderr. When a typical program
        // encounters an error, it writes its error message to stderr and
        // terminates with nonzero exit code. In particular, the Python
        // interpreter behaves that way. More generally, though, a plugin
        // author can log whatever s/he wants to the viewer log using stderr.
        mStderrConnection = childerr.getPump()
            .listen("LLLeap", boost::bind(&LLLeapImpl::rstderr, this, _1));

        // For our lifespan, intercept any LL_ERRS so we can notify plugin
        LLError::setFatalFunction(boost::bind(&LLLeapImpl::fatalFunction, this, _1));

        // Send child a preliminary event reporting our own reply-pump name --
        // which would otherwise be pretty tricky to guess!
        wstdin(mReplyPump.getName(),
               LLSDMap
               ("command", mListener->getName())
               // Include LLLeap features -- this may be important for child to
               // construct (or recognize) current protocol.
               ("features", LLLeapListener::getFeatures()));
    }

    // Normally we'd expect to arrive here only via done()
    virtual ~LLLeapImpl()
    {
        LL_DEBUGS("LLLeap") << "destroying LLLeap(\"" << mDesc << "\")" << LL_ENDL;
        // Restore original FatalFunction
        LLError::setFatalFunction(mPrevFatalFunction);
    }

    // Listener for failed launch attempt
    bool bad_launch(const LLSD& data)
    {
        LL_WARNS("LLLeap") << data["string"].asString() << LL_ENDL;
        return false;
    }

    // Listener for child-process termination
    bool done(const LLSD& data)
    {
        // Log the termination
        LL_INFOS("LLLeap") << data["string"].asString() << LL_ENDL;

        // Any leftover data at this moment are because protocol was not
        // satisfied. Possibly the child was interrupted in the middle of
        // sending a message, possibly the child didn't flush stdout before
        // terminating, possibly it's just garbage. Log its existence but
        // discard it.
        LLProcess::ReadPipe& childout(mChild->getReadPipe(LLProcess::STDOUT));
        if (childout.size())
        {
            LLProcess::ReadPipe::size_type
                peeklen((std::min)(LLProcess::ReadPipe::size_type(50), childout.size()));
            LL_WARNS("LLLeap") << "Discarding final " << childout.size() << " bytes: "
                               << childout.peek(0, peeklen) << "..." << LL_ENDL;
        }

        // Kill this instance. MUST BE LAST before return!
        delete this;
        return false;
    }

    // Listener for events on mReplyPump: send to child stdin
    bool wstdin(const std::string& pump, const LLSD& data)
    {
        LLSD packet(LLSDMap("pump", pump)("data", data));

        std::ostringstream buffer;
        buffer << LLSDNotationStreamer(packet);

/*==========================================================================*|
        // DEBUGGING ONLY: don't copy str() if we can avoid it.
        std::string strdata(buffer.str());
        if (std::size_t(buffer.tellp()) != strdata.length())
        {
            LL_ERRS("LLLeap") << "tellp() -> " << buffer.tellp() << " != "
                              << "str().length() -> " << strdata.length() << LL_ENDL;
        }
        // DEBUGGING ONLY: reading back is terribly inefficient.
        std::istringstream readback(strdata);
        LLSD echo;
        LLPointer<LLSDParser> parser(new LLSDNotationParser());
        S32 parse_status(parser->parse(readback, echo, strdata.length()));
        if (parse_status == LLSDParser::PARSE_FAILURE)
        {
            LL_ERRS("LLLeap") << "LLSDNotationParser() cannot parse output of "
                              << "LLSDNotationStreamer()" << LL_ENDL;
        }
        if (! llsd_equals(echo, packet))
        {
            LL_ERRS("LLLeap") << "LLSDNotationParser() produced different LLSD "
                              << "than passed to LLSDNotationStreamer()" << LL_ENDL;
        }
|*==========================================================================*/

        LL_DEBUGS("EventHost") << "Sending: " << buffer.tellp() << ':';
        std::string::size_type truncate(80);
        if (buffer.tellp() <= truncate)
        {
            LL_CONT << buffer.str();
        }
        else
        {
            LL_CONT << buffer.str().substr(0, truncate) << "...";
        }
        LL_CONT << LL_ENDL;

        LLProcess::WritePipe& childin(mChild->getWritePipe(LLProcess::STDIN));
        childin.get_ostream() << buffer.tellp() << ':' << buffer.str() << std::flush;
        return false;
    }

    // Initial state of stateful listening on child stdout: wait for a length
    // prefix, followed by ':'.
    bool rstdout(const LLSD& data)
    {
        LLProcess::ReadPipe& childout(mChild->getReadPipe(LLProcess::STDOUT));
        // It's possible we got notified of a couple digit characters without
        // seeing the ':' -- unlikely, but still. Until we see ':', keep
        // waiting.
        if (childout.contains(':'))
        {
            std::istream& childstream(childout.get_istream());
            // Saw ':', read length prefix and store in mExpect.
            size_t expect;
            childstream >> expect;
            int colon(childstream.get());
            if (colon != ':')
            {
                // Protocol failure. Clear out the rest of the pending data in
                // childout (well, up to a max length) to log what was wrong.
                LLProcess::ReadPipe::size_type
                    readlen((std::min)(childout.size(), LLProcess::ReadPipe::size_type(80)));
                bad_protocol(STRINGIZE(expect << char(colon) << childout.read(readlen)));
            }
            else
            {
                // Saw length prefix, saw colon, life is good. Now wait for
                // that length of data to arrive.
                mExpect = expect;
                LL_DEBUGS("LLLeap") << "got length, waiting for "
                                    << mExpect << " bytes of data" << LL_ENDL;
                // Block calls to this method; resetting mBlocker unblocks
                // calls to the other method.
                mBlocker.reset(new LLEventPump::Blocker(mStdoutConnection));
                // Go check if we've already received all the advertised data.
                if (childout.size())
                {
                    LLSD updata(data);
                    updata["len"] = LLSD::Integer(childout.size());
                    rstdoutData(updata);
                }
            }
        }
        else if (childout.contains('\n'))
        {
            // Since this is the initial listening state, this is where we'd
            // arrive if the child isn't following protocol at all -- say
            // because the user specified 'ls' or some darn thing.
            bad_protocol(childout.getline());
        }
        return false;
    }

    // State in which we listen on stdout for the specified length of data to
    // arrive.
    bool rstdoutData(const LLSD& data)
    {
        LLProcess::ReadPipe& childout(mChild->getReadPipe(LLProcess::STDOUT));
        // Until we've accumulated the promised length of data, keep waiting.
        if (childout.size() >= mExpect)
        {
            // Ready to rock and roll.
            LL_DEBUGS("LLLeap") << "needed " << mExpect << " bytes, got "
                                << childout.size() << ", parsing LLSD" << LL_ENDL;
            LLSD data;
            LLPointer<LLSDParser> parser(new LLSDNotationParser());
            S32 parse_status(parser->parse(childout.get_istream(), data, mExpect));
            if (parse_status == LLSDParser::PARSE_FAILURE)
            {
                bad_protocol("unparseable LLSD data");
            }
            else if (! (data.isMap() && data["pump"].isString() && data.has("data")))
            {
                // we got an LLSD object, but it lacks required keys
                bad_protocol("missing 'pump' or 'data'");
            }
            else
            {
                // The LLSD object we got from our stream contains the keys we
                // need.
                LLEventPumps::instance().obtain(data["pump"]).post(data["data"]);
                // Block calls to this method; resetting mBlocker unblocks calls
                // to the other method.
                mBlocker.reset(new LLEventPump::Blocker(mStdoutDataConnection));
                // Go check for any more pending events in the buffer.
                if (childout.size())
                {
                    LLSD updata(data);
                    data["len"] = LLSD::Integer(childout.size());
                    rstdout(updata);
                }
            }
        }
        return false;
    }

    void bad_protocol(const std::string& data)
    {
        LL_WARNS("LLLeap") << mDesc << ": invalid protocol: " << data << LL_ENDL;
        // No point in continuing to run this child.
        mChild->kill();
    }

    // Listen on child stderr and log everything that arrives
    bool rstderr(const LLSD& data)
    {
        LLProcess::ReadPipe& childerr(mChild->getReadPipe(LLProcess::STDERR));
        // We might have gotten a notification involving only a partial line
        // -- or multiple lines. Read all complete lines; stop when there's
        // only a partial line left.
        while (childerr.contains('\n'))
        {
            // DO NOT make calls with side effects in a logging statement! If
            // that log level is suppressed, your side effects WON'T HAPPEN.
            std::string line(childerr.getline());
            // Log the received line. Prefix it with the desc so we know which
            // plugin it's from. This method name rstderr() is intentionally
            // chosen to further qualify the log output.
            LL_INFOS("LLLeap") << mDesc << ": " << line << LL_ENDL;
        }
        // What if child writes a final partial line to stderr?
        if (data["eof"].asBoolean() && childerr.size())
        {
            std::string rest(childerr.read(childerr.size()));
            // Read all remaining bytes and log.
            LL_INFOS("LLLeap") << mDesc << ": " << rest << LL_ENDL;
        }
        return false;
    }

    void fatalFunction(const std::string& error)
    {
        // Notify plugin
        LLSD event;
        event["type"] = "error";
        event["error"] = error;
        mReplyPump.post(event);

        // All the above really accomplished was to buffer the serialized
        // event in our WritePipe. Have to pump mainloop a couple times to
        // really write it out there... but time out in case we can't write.
        LLProcess::WritePipe& childin(mChild->getWritePipe(LLProcess::STDIN));
        LLEventPump& mainloop(LLEventPumps::instance().obtain("mainloop"));
        LLSD nop;
        F64 until(LLTimer::getElapsedSeconds() + 2);
        while (childin.size() && LLTimer::getElapsedSeconds() < until)
        {
            mainloop.post(nop);
        }

        // forward the call to the previous FatalFunction
        mPrevFatalFunction(error);
    }

private:
    /// We always want to listen on mReplyPump with wstdin(); under some
    /// circumstances we'll also echo other LLEventPumps to the plugin.
    LLBoundListener connect(LLEventPump& pump, const std::string& listener)
    {
        // Serialize any event received on the specified LLEventPump to our
        // child's stdin, suitably enriched with the pump name on which it was
        // received.
        return pump.listen(listener,
                           boost::bind(&LLLeapImpl::wstdin, this, pump.getName(), _1));
    }

    std::string mDesc;
    LLEventStream mDonePump;
    LLEventStream mReplyPump;
    LLProcessPtr mChild;
    LLTempBoundListener
        mStdinConnection, mStdoutConnection, mStdoutDataConnection, mStderrConnection;
    boost::scoped_ptr<LLEventPump::Blocker> mBlocker;
    LLProcess::ReadPipe::size_type mExpect;
    LLError::FatalFunction mPrevFatalFunction;
    boost::scoped_ptr<LLLeapListener> mListener;
};

// This must follow the declaration of LLLeapImpl, so it may as well be last.
LLLeap* LLLeap::create(const std::string& desc, const std::vector<std::string>& plugin, bool exc)
{
    // If caller is willing to permit exceptions, just instantiate.
    if (exc)
        return new LLLeapImpl(desc, plugin);

    // Caller insists on suppressing LLLeap::Error. Very well, catch it.
    try
    {
        return new LLLeapImpl(desc, plugin);
    }
    catch (const LLLeap::Error&)
    {
        return NULL;
    }
}

LLLeap* LLLeap::create(const std::string& desc, const std::string& plugin, bool exc)
{
    // Use LLStringUtil::getTokens() to parse the command line
    return create(desc,
                  LLStringUtil::getTokens(plugin,
                                          " \t\r\n", // drop_delims
                                          "",        // no keep_delims
                                          "\"'",     // either kind of quotes
                                          "\\"),     // backslash escape
                  exc);
}
