/**
 * @file   llviewercontrollistener_test.cpp
 * @author Nat Goodspeed
 * @date   2022-06-09
 * @brief  Test for llviewercontrollistener.
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "../llviewerprecompiledheaders.h"
// associated header
#include "../llviewercontrollistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "../test/lldoctest.h"
#include "../test/catch_and_store_what_in.h" // catch_what()
#include "commoncontrol.h"
#include "llcontrol.h"              // LLControlGroup

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

struct llviewercontrollistener_data
{

        LLControlGroup Global{"FakeGlobal"
};

TEST_CASE_FIXTURE(llviewercontrollistener_data, "test_1")
{

        set_test_name("CommonControl no listener");
        // Not implemented: the linker drags in LLViewerControlListener when
        // we bring in LLViewerControl.
    
}

TEST_CASE_FIXTURE(llviewercontrollistener_data, "test_2")
{

        set_test_name("CommonControl bad group");
        std::string threw{ catch_what<LL::CommonControl::ParamError>(
                [](){ LL::CommonControl::get("Nonexistent", "Variable"); 
}

TEST_CASE_FIXTURE(llviewercontrollistener_data, "test_3")
{

        set_test_name("CommonControl bad variable");
        std::string threw{ catch_what<LL::CommonControl::ParamError>(
                [](){ LL::CommonControl::get("FakeGlobal", "Nonexistent"); 
}

TEST_CASE_FIXTURE(llviewercontrollistener_data, "test_4")
{

        set_test_name("CommonControl toggle string");
        std::string threw{ catch_what<LL::CommonControl::ParamError>(
                [](){ LL::CommonControl::toggle("FakeGlobal", "strvar"); 
}

TEST_CASE_FIXTURE(llviewercontrollistener_data, "test_5")
{

        set_test_name("CommonControl list bad group");
        std::string threw{ catch_what<LL::CommonControl::ParamError>(
                [](){ LL::CommonControl::get_vars("Nonexistent"); 
}

TEST_CASE_FIXTURE(llviewercontrollistener_data, "test_6")
{

        set_test_name("CommonControl get");
        auto strvar{ LL::CommonControl::get("FakeGlobal", "strvar") 
}

TEST_CASE_FIXTURE(llviewercontrollistener_data, "test_7")
{

        set_test_name("CommonControl set, set_default, toggle");

        std::string newstr{ LL::CommonControl::set("FakeGlobal", "strvar", "mouse").asString() 
}

TEST_CASE_FIXTURE(llviewercontrollistener_data, "test_8")
{

        set_test_name("CommonControl get_def");
        LLSD def{ LL::CommonControl::get_def("FakeGlobal", "strvar") 
}

TEST_CASE_FIXTURE(llviewercontrollistener_data, "test_9")
{

        set_test_name("CommonControl get_groups");
        std::vector<std::string> groups{ LL::CommonControl::get_groups() 
}

TEST_CASE_FIXTURE(llviewercontrollistener_data, "test_10")
{

        set_test_name("CommonControl get_vars");
        LLSD vars{ LL::CommonControl::get_vars("FakeGlobal") 
}

} // TEST_SUITE
