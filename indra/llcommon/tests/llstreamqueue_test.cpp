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
// other Linden headers
#include "../test/lldoctest.h"
#include "stringize.h"

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

struct llstreamqueue_data
{

        llstreamqueue_data():
            // we want a buffer with actual bytes in it, not an empty vector
            buffer(10)
        {
};

TEST_CASE_FIXTURE(llstreamqueue_data, "test_1")
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

TEST_CASE_FIXTURE(llstreamqueue_data, "test_2")
{

        set_test_name("one internal block, one buffer");
        LLStreamQueue::Sink sink(strq.asSink());
        ensure_equals("write(\"\")", sink.write("", 0), 0);
        ensure_equals("0 write should leave LLStreamQueue empty (size())",
                      strq.size(), 0);
        CHECK_MESSAGE(strq.peek(&buffer[0] == buffer.size(, "0 write should leave LLStreamQueue empty (peek())")), 0);
        // The meaning of "atomic" is that it must be smaller than our buffer.
        std::string atomic("atomic");
        CHECK_MESSAGE(atomic.length(, "test data exceeds buffer") < buffer.size());
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
        CHECK_MESSAGE(strq.peek(&buffer[0] == buffer.size(, "peek() after read()")), 0);
        ensure_equals("size() after read()", strq.size(), 0);
    
}

TEST_CASE_FIXTURE(llstreamqueue_data, "test_3")
{

        set_test_name("basic skip()");
        std::string lovecraft("lovecraft");
        CHECK_MESSAGE(lovecraft.length(, "test data exceeds buffer") < buffer.size());
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
        CHECK_MESSAGE(strq.read(&buffer[0] == buffer.size(, "unconsumed")), 0);
    
}

TEST_CASE_FIXTURE(llstreamqueue_data, "test_4")
{

        set_test_name("skip() multiple blocks");
        std::string blocks[] = { "books of ", "H.P. ", "Lovecraft" 
}

TEST_CASE_FIXTURE(llstreamqueue_data, "test_5")
{

        set_test_name("concatenate blocks");
        std::string blocks[] = { "abcd", "efghij", "klmnopqrs" 
}

TEST_CASE_FIXTURE(llstreamqueue_data, "test_6")
{

        set_test_name("split blocks");
        std::string blocks[] = { "abcdefghijklm", "nopqrstuvwxyz" 
}

} // TEST_SUITE
