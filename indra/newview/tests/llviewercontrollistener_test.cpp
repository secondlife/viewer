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
#include "../test/lltut.h"
#include "../test/catch_and_store_what_in.h" // catch_what()
#include "commoncontrol.h"
#include "llcontrol.h"              // LLControlGroup

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    void ensure_contains(const std::string& msg, const std::string& substr)
    {
        ensure_contains("Exception does not contain " + substr, msg, substr);
    }

    struct llviewercontrollistener_data
    {
        LLControlGroup Global{"FakeGlobal"};

        llviewercontrollistener_data()
        {
            Global.declareString("strvar", "woof", "string variable");
            // together we will stroll the boolvar, ma cherie
            Global.declareBOOL("boolvar",  true, "bool variable");
        }
    };
    typedef test_group<llviewercontrollistener_data> llviewercontrollistener_group;
    typedef llviewercontrollistener_group::object object;
    llviewercontrollistener_group llviewercontrollistenergrp("llviewercontrollistener");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("CommonControl no listener");
        // Not implemented: the linker drags in LLViewerControlListener when
        // we bring in LLViewerControl.
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("CommonControl bad group");
        std::string threw{ catch_what<LL::CommonControl::ParamError>(
                [](){ LL::CommonControl::get("Nonexistent", "Variable"); }) };
        ensure_contains(threw, "group");
        ensure_contains(threw, "Nonexistent");
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("CommonControl bad variable");
        std::string threw{ catch_what<LL::CommonControl::ParamError>(
                [](){ LL::CommonControl::get("FakeGlobal", "Nonexistent"); }) };
        ensure_contains(threw, "key");
        ensure_contains(threw, "Nonexistent");
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("CommonControl toggle string");
        std::string threw{ catch_what<LL::CommonControl::ParamError>(
                [](){ LL::CommonControl::toggle("FakeGlobal", "strvar"); }) };
        ensure_contains(threw, "non-boolean");
        ensure_contains(threw, "strvar");
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("CommonControl list bad group");
        std::string threw{ catch_what<LL::CommonControl::ParamError>(
                [](){ LL::CommonControl::get_vars("Nonexistent"); }) };
        ensure_contains(threw, "group");
        ensure_contains(threw, "Nonexistent");
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("CommonControl get");
        auto strvar{ LL::CommonControl::get("FakeGlobal", "strvar") };
        ensure_equals(strvar, "woof");
        auto boolvar{ LL::CommonControl::get("FakeGlobal", "boolvar") };
        ensure(boolvar);
    }

    template<> template<>
    void object::test<7>()
    {
        set_test_name("CommonControl set, set_default, toggle");

        std::string newstr{ LL::CommonControl::set("FakeGlobal", "strvar", "mouse").asString() };
        ensure_equals(newstr, "mouse");
        ensure_equals(LL::CommonControl::get("FakeGlobal", "strvar").asString(), "mouse");
        ensure_equals(LL::CommonControl::set_default("FakeGlobal", "strvar").asString(), "woof");

        bool newbool{ LL::CommonControl::set("FakeGlobal", "boolvar", false) };
        ensure(! newbool);
        ensure(! LL::CommonControl::get("FakeGlobal", "boolvar").asBoolean());
        ensure(LL::CommonControl::set_default("FakeGlobal", "boolvar").asBoolean());
        ensure(! LL::CommonControl::toggle("FakeGlobal", "boolvar").asBoolean());
    }

    template<> template<>
    void object::test<8>()
    {
        set_test_name("CommonControl get_def");
        LLSD def{ LL::CommonControl::get_def("FakeGlobal", "strvar") };
        ensure_equals(
            def,
            llsd::map("name", "strvar",
                      "type", "String",
                      "value", "woof",
                      "comment", "string variable"));
    }

    template<> template<>
    void object::test<9>()
    {
        set_test_name("CommonControl get_groups");
        std::vector<std::string> groups{ LL::CommonControl::get_groups() };
        ensure_equals(groups.size(), 1);
        ensure_equals(groups[0], "FakeGlobal");
    }

    template<> template<>
    void object::test<10>()
    {
        set_test_name("CommonControl get_vars");
        LLSD vars{ LL::CommonControl::get_vars("FakeGlobal") };
        // convert from array (unpredictable order) to map
        LLSD varsmap{ LLSD::emptyMap() };
        for (auto& var : llsd::inArray(vars))
        {
            varsmap[var["name"].asString()] = var;
        }
        // comparing maps is order-insensitive
        ensure_equals(
            varsmap,
            llsd::map(
                "strvar",
                llsd::map("name", "strvar",
                          "type", "String",
                          "value", "woof",
                          "comment", "string variable"),
                "boolvar",
                llsd::map("name", "boolvar",
                          "type", "Boolean",
                          "value", true,
                          "comment", "bool variable")));
    }
} // namespace tut
