/**
 * @file llsingleton_test.cpp
 * @date 2011-08-11
 * @brief Unit test for the LLSingleton class
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llsingleton.h"
#include "../test/lldoctest.h"
#include "wrapllerrs.h"
#include "llsd.h"

// Capture execution sequence by appending to log string.
std::string sLog;

#define DECLARE_CLASS(CLS)                          \
struct CLS: public LLSingleton<CLS>                 \
{                                                   \
    LLSINGLETON(CLS);                               \
    ~CLS();                                         \
public:                                             \
    static enum dep_flag {                          \
        DEP_NONE, /* no dependency */               \
        DEP_CTOR, /* dependency in ctor */          \
        DEP_INIT  /* dependency in initSingleton */ \
    } sDepFlag;                                     \
                                                    \
    void initSingleton() override;                  \
    void cleanupSingleton() override;               \
};                                                  \
                                                    \
CLS::dep_flag CLS::sDepFlag = DEP_NONE

DECLARE_CLASS(A);
DECLARE_CLASS(B);

#define DEFINE_MEMBERS(CLS, OTHER)              \
CLS::CLS()                                      \
{                                               \
    sLog.append(#CLS);                          \
    if (sDepFlag == DEP_CTOR)                   \
    {                                           \
        (void)OTHER::instance();                \
    }                                           \
}                                               \
                                                \
void CLS::initSingleton()                       \
{                                               \
    sLog.append("i" #CLS);                      \
    if (sDepFlag == DEP_INIT)                   \
    {                                           \
        (void)OTHER::instance();                \
    }                                           \
}                                               \
                                                \
void CLS::cleanupSingleton()                    \
{                                               \
    sLog.append("x" #CLS);                      \
}                                               \
                                                \
CLS::~CLS()                                     \
{                                               \
    sLog.append("~" #CLS);                      \
}

DEFINE_MEMBERS(A, B)
DEFINE_MEMBERS(B, A)

TEST_SUITE("LLSingleton") {

struct singleton
{

        // We need a class created with the LLSingleton template to test with.
        class LLSingletonTest: public LLSingleton<LLSingletonTest>
        {
            LLSINGLETON_EMPTY_CTOR(LLSingletonTest);
        
};

TEST_CASE_FIXTURE(singleton, "test_1")
{


    
}

TEST_CASE_FIXTURE(singleton, "test_2")
{

        LLSingletonTest* singleton_test = LLSingletonTest::getInstance();
        ensure(singleton_test);
    
}

TEST_CASE_FIXTURE(singleton, "test_3")
{

        //Construct the instance
        LLSingletonTest::getInstance();
        ensure(LLSingletonTest::instanceExists());

        //Delete the instance
        LLSingletonTest::deleteSingleton();
        ensure(!LLSingletonTest::instanceExists());

        //Construct it again.
        LLSingletonTest* singleton_test = LLSingletonTest::getInstance();
        ensure(singleton_test);
        ensure(LLSingletonTest::instanceExists());
    
}

TEST_CASE_FIXTURE(singleton, "test_12")
{

        set_test_name("LLParamSingleton");

        WrapLLErrs catcherr;
        // query methods
        CHECK_MESSAGE(! PSing1::instanceExists(, "false positive on instanceExists()"));
        CHECK_MESSAGE(! PSing1::wasDeleted(, "false positive on wasDeleted()"));
        // try to reference before initializing
        std::string threw = catcherr.catch_llerrs([](){
                (void)PSing1::instance();
            
}

TEST_CASE_FIXTURE(singleton, "test_13")
{

        set_test_name("LLParamSingleton alternate ctor");

        WrapLLErrs catcherr;
        // We don't have to restate all the tests for PSing1. Only test validly
        // using the other constructor.
        PSing2::initParamSingleton(17);
        ensure_equals(PSing2::instance().desc(), "17");
        // can't do it twice
        std::string threw = catcherr.catch_llerrs([](){
                PSing2::initParamSingleton(34);
            
}

TEST_CASE_FIXTURE(singleton, "test_14")
{

        set_test_name("Circular LLParamSingleton constructor");
        WrapLLErrs catcherr;
        std::string threw = catcherr.catch_llerrs([](){
                CircularPCtor::initParamSingleton();
            
}

TEST_CASE_FIXTURE(singleton, "test_15")
{

        set_test_name("Circular LLParamSingleton initSingleton()");
        WrapLLErrs catcherr;
        std::string threw = catcherr.catch_llerrs([](){
                CircularPInit::initParamSingleton();
            
}

} // TEST_SUITE

