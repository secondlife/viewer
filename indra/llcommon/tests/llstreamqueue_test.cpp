/**
 * @file   llstreamqueue_test.cpp
 * @author Nat Goodspeed
 * @date   2012-01-05
 * @brief  Test for llstreamqueue.
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llstreamqueue.h"
// STL headers
#include <vector>
// std headers
// external library headers
#include <boost/foreach.hpp>
// other Linden headers
#include "../test/lltut.h"
#include "stringize.h"

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llstreamqueue_data
    {
        llstreamqueue_data():
            // we want a buffer with actual bytes in it, not an empty vector
            buffer(10)
        {}
        // As LLStreamQueue is merely a typedef for
        // LLGenericStreamQueue<char>, and no logic in LLGenericStreamQueue is
        // specific to the <char> instantiation, we're comfortable for now
        // testing only the narrow-char version.
        LLStreamQueue strq;
        // buffer for use in multiple tests
        std::vector<char> buffer;
    };
    typedef test_group<llstreamqueue_data> llstreamqueue_group;
    typedef llstreamqueue_group::object object;
    llstreamqueue_group llstreamqueuegrp("llstreamqueue");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("empty LLStreamQueue");
        ensure_equals("brand-new LLStreamQueue isn't empty",
                      strq.size(), 0);
        ensure_equals("brand-new LLStreamQueue returns data",
                      strq.asSource().read(&buffer[0], buffer.size()), 0);
        strq.asSink().close();
        ensure_equals("closed empty LLStreamQueue not at EOF",
                      strq.asSource().read(&buffer[0], buffer.size()), -1);
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("one internal block, one buffer");
        LLStreamQueue::Sink sink(strq.asSink());
        ensure_equals("write(\"\")", sink.write("", 0), 0);
        ensure_equals("0 write should leave LLStreamQueue empty (size())",
                      strq.size(), 0);
        ensure_equals("0 write should leave LLStreamQueue empty (peek())",
                      strq.peek(&buffer[0], buffer.size()), 0);
        // The meaning of "atomic" is that it must be smaller than our buffer.
        std::string atomic("atomic");
        ensure("test data exceeds buffer", atomic.length() < buffer.size());
        ensure_equals(STRINGIZE("write(\"" << atomic << "\")"),
                      sink.write(&atomic[0], atomic.length()), atomic.length());
        ensure_equals("size() after write()", strq.size(), atomic.length());
        size_t peeklen(strq.peek(&buffer[0], buffer.size()));
        ensure_equals(STRINGIZE("peek(\"" << atomic << "\")"),
                      peeklen, atomic.length());
        ensure_equals(STRINGIZE("peek(\"" << atomic << "\") result"),
                      std::string(buffer.begin(), buffer.begin() + peeklen), atomic);
        ensure_equals("size() after peek()", strq.size(), atomic.length());
        // peek() should not consume. Use a different buffer to prove it isn't
        // just leftover data from the first peek().
        std::vector<char> again(buffer.size());
        peeklen = size_t(strq.peek(&again[0], again.size()));
        ensure_equals(STRINGIZE("peek(\"" << atomic << "\") again"),
                      peeklen, atomic.length());
        ensure_equals(STRINGIZE("peek(\"" << atomic << "\") again result"),
                      std::string(again.begin(), again.begin() + peeklen), atomic);
        // now consume.
        std::vector<char> third(buffer.size());
        size_t readlen(strq.read(&third[0], third.size()));
        ensure_equals(STRINGIZE("read(\"" << atomic << "\")"),
                      readlen, atomic.length());
        ensure_equals(STRINGIZE("read(\"" << atomic << "\") result"),
                      std::string(third.begin(), third.begin() + readlen), atomic);
        ensure_equals("peek() after read()", strq.peek(&buffer[0], buffer.size()), 0);
        ensure_equals("size() after read()", strq.size(), 0);
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("basic skip()");
        std::string lovecraft("lovecraft");
        ensure("test data exceeds buffer", lovecraft.length() < buffer.size());
        ensure_equals(STRINGIZE("write(\"" << lovecraft << "\")"),
                      strq.write(&lovecraft[0], lovecraft.length()), lovecraft.length());
        size_t peeklen(strq.peek(&buffer[0], buffer.size()));
        ensure_equals(STRINGIZE("peek(\"" << lovecraft << "\")"),
                      peeklen, lovecraft.length());
        ensure_equals(STRINGIZE("peek(\"" << lovecraft << "\") result"),
                      std::string(buffer.begin(), buffer.begin() + peeklen), lovecraft);
        std::streamsize skip1(4);
        ensure_equals(STRINGIZE("skip(" << skip1 << ")"), strq.skip(skip1), skip1);
        ensure_equals("size() after skip()", strq.size(), lovecraft.length() - skip1);
        size_t readlen(strq.read(&buffer[0], buffer.size()));
        ensure_equals(STRINGIZE("read(\"" << lovecraft.substr(skip1) << "\")"),
                      readlen, lovecraft.length() - skip1);
        ensure_equals(STRINGIZE("read(\"" << lovecraft.substr(skip1) << "\") result"),
                      std::string(buffer.begin(), buffer.begin() + readlen),
                      lovecraft.substr(skip1));
        ensure_equals("unconsumed", strq.read(&buffer[0], buffer.size()), 0);
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("skip() multiple blocks");
        std::string blocks[] = { "books of ", "H.P. ", "Lovecraft" };
        std::streamsize total(blocks[0].length() + blocks[1].length() + blocks[2].length());
        std::streamsize leave(5);   // len("craft") above
        std::streamsize skip(total - leave);
        std::streamsize written(0);
        BOOST_FOREACH(const std::string& block, blocks)
        {
            written += strq.write(&block[0], block.length());
            ensure_equals("size() after write()", strq.size(), written);
        }
        std::streamsize skiplen(strq.skip(skip));
        ensure_equals(STRINGIZE("skip(" << skip << ")"), skiplen, skip);
        ensure_equals("size() after skip()", strq.size(), leave);
        size_t readlen(strq.read(&buffer[0], buffer.size()));
        ensure_equals("read(\"craft\")", readlen, leave);
        ensure_equals("read(\"craft\") result",
                      std::string(buffer.begin(), buffer.begin() + readlen), "craft");
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("concatenate blocks");
        std::string blocks[] = { "abcd", "efghij", "klmnopqrs" };
        BOOST_FOREACH(const std::string& block, blocks)
        {
            strq.write(&block[0], block.length());
        }
        std::vector<char> longbuffer(30);
        std::streamsize readlen(strq.read(&longbuffer[0], longbuffer.size()));
        ensure_equals("read() multiple blocks",
                      readlen, blocks[0].length() + blocks[1].length() + blocks[2].length());
        ensure_equals("read() multiple blocks result",
                      std::string(longbuffer.begin(), longbuffer.begin() + readlen),
                      blocks[0] + blocks[1] + blocks[2]);
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("split blocks");
        std::string blocks[] = { "abcdefghijklm", "nopqrstuvwxyz" };
        BOOST_FOREACH(const std::string& block, blocks)
        {
            strq.write(&block[0], block.length());
        }
        strq.close();
        // We've already verified what strq.size() should be at this point;
        // see above test named "skip() multiple blocks"
        std::streamsize chksize(strq.size());
        std::streamsize readlen(strq.read(&buffer[0], buffer.size()));
        ensure_equals("read() 0", readlen, buffer.size());
        ensure_equals("read() 0 result", std::string(buffer.begin(), buffer.end()), "abcdefghij");
        chksize -= readlen;
        ensure_equals("size() after read() 0", strq.size(), chksize);
        readlen = strq.read(&buffer[0], buffer.size());
        ensure_equals("read() 1", readlen, buffer.size());
        ensure_equals("read() 1 result", std::string(buffer.begin(), buffer.end()), "klmnopqrst");
        chksize -= readlen;
        ensure_equals("size() after read() 1", strq.size(), chksize);
        readlen = strq.read(&buffer[0], buffer.size());
        ensure_equals("read() 2", readlen, chksize);
        ensure_equals("read() 2 result",
                      std::string(buffer.begin(), buffer.begin() + readlen), "uvwxyz");
        ensure_equals("read() 3", strq.read(&buffer[0], buffer.size()), -1);
    }
} // namespace tut
