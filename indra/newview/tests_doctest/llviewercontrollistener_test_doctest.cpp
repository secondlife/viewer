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

#include "../llviewerprecompiledheaders.h"
#include "doctest.h"
#include "indra/test/tut_compat_doctest.h"
#include "../llviewercontrollistener.h"
#include "../test/catch_and_store_what_in.h"
#include "commoncontrol.h"
#include "llcontrol.h"

namespace tut
{
    using tut_compat::ensure;
    using tut_compat::ensure_equals;
    using tut_compat::set_test_name;

    inline void ensure_contains(const std::string& msg, const std::string& actual, const std::string& expectedSubString)
    {
        CHECK_MESSAGE(actual.find(expectedSubString) != std::string::npos, msg.c_str());
    }

    inline void ensure_contains(const std::string& msg, const std::string& substr)
    {
        ensure_contains("Exception does not contain " + substr, msg, substr);
    }

    struct llviewercontrollistener_data
    {
        LLControlGroup Global{"FakeGlobal"};

        llviewercontrollistener_data()
        {
            Global.declareString("strvar", "woof", "string variable");
            Global.declareBOOL("boolvar",  true, "bool variable");
        }
    };
}

TUT_SUITE("llviewercontrollistener")
{
    TUT_CASE("llviewercontrollistener::test_1")
    {
        using namespace tut;
        llviewercontrollistener_data data;
        (void)data;
        set_test_name("CommonControl no listener");
    }

    TUT_CASE("llviewercontrollistener::test_2")
    {
        using namespace tut;
        llviewercontrollistener_data data;
        (void)data;
        set_test_name("CommonControl bad group");
        std::string threw{ catch_what<LL::CommonControl::ParamError>(
                [](){ LL::CommonControl::get("Nonexistent", "Variable"); }) };
        ensure_contains(threw, "group");
        ensure_contains(threw, "Nonexistent");
    }

    TUT_CASE("llviewercontrollistener::test_3")
    {
        using namespace tut;
        llviewercontrollistener_data data;
        (void)data;
        set_test_name("CommonControl bad variable");
        std::string threw{ catch_what<LL::CommonControl::ParamError>(
                [](){ LL::CommonControl::get("FakeGlobal", "Nonexistent"); }) };
        ensure_contains(threw, "key");
        ensure_contains(threw, "Nonexistent");
    }

    TUT_CASE("llviewercontrollistener::test_4")
    {
        using namespace tut;
        llviewercontrollistener_data data;
        (void)data;
        set_test_name("CommonControl toggle string");
        std::string threw{ catch_what<LL::CommonControl::ParamError>(
                [](){ LL::CommonControl::toggle("FakeGlobal", "strvar"); }) };
        ensure_contains(threw, "non-boolean");
        ensure_contains(threw, "strvar");
    }

    TUT_CASE("llviewercontrollistener::test_5")
    {
        using namespace tut;
        llviewercontrollistener_data data;
        (void)data;
        set_test_name("CommonControl list bad group");
        std::string threw{ catch_what<LL::CommonControl::ParamError>(
                [](){ LL::CommonControl::get_vars("Nonexistent"); }) };
        ensure_contains(threw, "group");
        ensure_contains(threw, "Nonexistent");
    }

    TUT_CASE("llviewercontrollistener::test_6")
    {
        using namespace tut;
        llviewercontrollistener_data data;
        (void)data;
        set_test_name("CommonControl get");
        auto strvar{ LL::CommonControl::get("FakeGlobal", "strvar") };
        ensure_equals(strvar, "woof");
        auto boolvar{ LL::CommonControl::get("FakeGlobal", "boolvar") };
        ensure(boolvar);
    }

    TUT_CASE("llviewercontrollistener::test_7")
    {
        using namespace tut;
        llviewercontrollistener_data data;
        (void)data;
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

    TUT_CASE("llviewercontrollistener::test_8")
    {
        using namespace tut;
        llviewercontrollistener_data data;
        (void)data;
        set_test_name("CommonControl get_def");
        LLSD def{ LL::CommonControl::get_def("FakeGlobal", "strvar") };
        ensure_equals(
            def,
            llsd::map("name", "strvar",
                      "type", "String",
                      "value", "woof",
                      "comment", "string variable"));
    }

    TUT_CASE("llviewercontrollistener::test_9")
    {
        using namespace tut;
        llviewercontrollistener_data data;
        (void)data;
        set_test_name("CommonControl get_groups");
        std::vector<std::string> groups{ LL::CommonControl::get_groups() };
        ensure_equals(groups.size(), 1);
        ensure_equals(groups[0], "FakeGlobal");
    }

    TUT_CASE("llviewercontrollistener::test_10")
    {
        using namespace tut;
        llviewercontrollistener_data data;
        (void)data;
        set_test_name("CommonControl get_vars");
        LLSD vars{ LL::CommonControl::get_vars("FakeGlobal") };
        LLSD varsmap{ LLSD::emptyMap() };
        for (auto& var : llsd::inArray(vars))
        {
            varsmap[var["name"].asString()] = var;
        }
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
}
