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
#include "../test/lltut.h"


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
    void initSingleton();                           \
    void cleanupSingleton();                        \
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

namespace tut
{
    struct singleton
    {
        // We need a class created with the LLSingleton template to test with.
        class LLSingletonTest: public LLSingleton<LLSingletonTest>
        {
            LLSINGLETON_EMPTY_CTOR(LLSingletonTest);
        };
    };

    typedef test_group<singleton> singleton_t;
    typedef singleton_t::object singleton_object_t;
    tut::singleton_t tut_singleton("LLSingleton");

    template<> template<>
    void singleton_object_t::test<1>()
    {

    }
    template<> template<>
    void singleton_object_t::test<2>()
    {
        LLSingletonTest* singleton_test = LLSingletonTest::getInstance();
        ensure(singleton_test);
    }

    template<> template<>
    void singleton_object_t::test<3>()
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

#define TESTS(CLS, OTHER, N0, N1, N2, N3)                               \
    template<> template<>                                               \
    void singleton_object_t::test<N0>()                                 \
    {                                                                   \
        set_test_name("just " #CLS);                                    \
        CLS::sDepFlag = CLS::DEP_NONE;                                  \
        OTHER::sDepFlag = OTHER::DEP_NONE;                              \
        sLog.clear();                                                   \
                                                                        \
        (void)CLS::instance();                                          \
        ensure_equals(sLog, #CLS "i" #CLS);                             \
        LLSingletonBase::cleanupAll();                                  \
        ensure_equals(sLog, #CLS "i" #CLS "x" #CLS);                    \
        LLSingletonBase::deleteAll();                                   \
        ensure_equals(sLog, #CLS "i" #CLS "x" #CLS "~" #CLS);           \
    }                                                                   \
                                                                        \
    template<> template<>                                               \
    void singleton_object_t::test<N1>()                                 \
    {                                                                   \
        set_test_name(#CLS " ctor depends " #OTHER);                    \
        CLS::sDepFlag = CLS::DEP_CTOR;                                  \
        OTHER::sDepFlag = OTHER::DEP_NONE;                              \
        sLog.clear();                                                   \
                                                                        \
        (void)CLS::instance();                                          \
        ensure_equals(sLog, #CLS #OTHER "i" #OTHER "i" #CLS);           \
        LLSingletonBase::cleanupAll();                                  \
        ensure_equals(sLog, #CLS #OTHER "i" #OTHER "i" #CLS "x" #CLS "x" #OTHER); \
        LLSingletonBase::deleteAll();                                   \
        ensure_equals(sLog, #CLS #OTHER "i" #OTHER "i" #CLS "x" #CLS "x" #OTHER "~" #CLS "~" #OTHER); \
    }                                                                   \
                                                                        \
    template<> template<>                                               \
    void singleton_object_t::test<N2>()                                 \
    {                                                                   \
        set_test_name(#CLS " init depends " #OTHER);                    \
        CLS::sDepFlag = CLS::DEP_INIT;                                  \
        OTHER::sDepFlag = OTHER::DEP_NONE;                              \
        sLog.clear();                                                   \
                                                                        \
        (void)CLS::instance();                                          \
        ensure_equals(sLog, #CLS "i" #CLS #OTHER "i" #OTHER);           \
        LLSingletonBase::cleanupAll();                                  \
        ensure_equals(sLog, #CLS "i" #CLS #OTHER "i" #OTHER "x" #CLS "x" #OTHER); \
        LLSingletonBase::deleteAll();                                   \
        ensure_equals(sLog, #CLS "i" #CLS #OTHER "i" #OTHER "x" #CLS "x" #OTHER "~" #CLS "~" #OTHER); \
    }                                                                   \
                                                                        \
    template<> template<>                                               \
    void singleton_object_t::test<N3>()                                 \
    {                                                                   \
        set_test_name(#CLS " circular init");                           \
        CLS::sDepFlag = CLS::DEP_INIT;                                  \
        OTHER::sDepFlag = OTHER::DEP_CTOR;                              \
        sLog.clear();                                                   \
                                                                        \
        (void)CLS::instance();                                          \
        ensure_equals(sLog, #CLS "i" #CLS #OTHER "i" #OTHER);           \
        LLSingletonBase::cleanupAll();                                  \
        ensure_equals(sLog, #CLS "i" #CLS #OTHER "i" #OTHER "x" #CLS "x" #OTHER); \
        LLSingletonBase::deleteAll();                                   \
        ensure_equals(sLog, #CLS "i" #CLS #OTHER "i" #OTHER "x" #CLS "x" #OTHER "~" #CLS "~" #OTHER); \
    }

    TESTS(A, B, 4, 5, 6, 7)
    TESTS(B, A, 8, 9, 10, 11)
}
