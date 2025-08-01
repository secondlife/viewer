/**
 * @file llerror_test.cpp
 * @date   December 2006
 * @brief error unit tests
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include <vector>
#include <stdexcept>

#include "linden_common.h"

#include "../llerror.h"

#include "../llerrorcontrol.h"
#include "../llsd.h"

#include "../test/lldoctest.h"

enum LogFieldIndex
{
    TIME_FIELD,
    LEVEL_FIELD,
    TAGS_FIELD,
    LOCATION_FIELD,
    FUNCTION_FIELD,
    MSG_FIELD
};

static const char* FieldName[] =
{
    "TIME",
    "LEVEL",
    "TAGS",
    "LOCATION",
    "FUNCTION",
    "MSG"
};

namespace
{
#ifdef __clang__
#   pragma clang diagnostic ignored "-Wunused-function"
#endif
    void test_that_error_h_includes_enough_things_to_compile_a_message()
    {
        LL_INFOS() << "!" << LL_ENDL;
    }
}

namespace
{
    static bool fatalWasCalled = false;
    struct FatalWasCalled: public std::runtime_error
    {
        FatalWasCalled(const std::string& what): std::runtime_error(what) {}
    };
    void fatalCall(const std::string& msg) { throw FatalWasCalled(msg); }
}

// Because we use LLError::setFatalFunction(fatalCall), any LL_ERRS call we
// issue will throw FatalWasCalled. But we want the test program to continue.
// So instead of writing:
// LL_ERRS("tag") << "some message" << LL_ENDL;
// write:
// CATCH(LL_ERRS("tag"), "some message");
#define CATCH(logcall, expr)                    \
    try                                         \
    {                                           \
        logcall << expr << LL_ENDL;             \
    }                                           \
    catch (const FatalWasCalled&)               \
    {                                           \
        fatalWasCalled = true;                  \
    }

TEST_SUITE("UnknownSuite") {

struct ErrorTestData
{

        LLError::RecorderPtr mRecorder;
        LLError::SettingsStoragePtr mPriorErrorSettings;

        ErrorTestData():
            mRecorder(new TestRecorder())
        {
            fatalWasCalled = false;

            mPriorErrorSettings = LLError::saveAndResetSettings();
            LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
            LLError::setFatalFunction(fatalCall);
            LLError::addRecorder(mRecorder);
        
};

} // TEST_SUITE
