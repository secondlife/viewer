/**
* @file   llservicebuilder_tut.cpp
* @brief  LLServiceBuilder unit tests
* @date   March 2007
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

#include <tut/tut.hpp>

#include "linden_common.h"
#include "lltut.h"
#include "llsd.h"
#include "llservicebuilder.h"

namespace tut
{

    struct ServiceBuilderTestData {
        LLServiceBuilder mServiceBuilder;
    };

    typedef test_group<ServiceBuilderTestData>  ServiceBuilderTestGroup;
    typedef ServiceBuilderTestGroup::object ServiceBuilderTestObject;

    ServiceBuilderTestGroup serviceBuilderTestGroup("ServiceBuilder");

    template<> template<>
    void ServiceBuilderTestObject::test<1>()
    {
        //Simple service build and reply with no mapping
        LLSD test_block;
        test_block["service-builder"] = "/agent/name";
        mServiceBuilder.createServiceDefinition("ServiceBuilderTest", test_block["service-builder"]);
        std::string test_url = mServiceBuilder.buildServiceURI("ServiceBuilderTest");
        ensure_equals("Basic URL Creation", test_url , "/agent/name");
    }   

    template<> template<>
    void ServiceBuilderTestObject::test<2>()
    {
        //Simple replace test
        LLSD test_block;
        test_block["service-builder"] = "/agent/{$agent-id}/name";
        mServiceBuilder.createServiceDefinition("ServiceBuilderTest", test_block["service-builder"]);   
        LLSD data_map;
        data_map["agent-id"] = "257c631f-a0c5-4f29-8a9f-9031feaae6c6";
        std::string test_url = mServiceBuilder.buildServiceURI("ServiceBuilderTest", data_map);
        ensure_equals("Replacement URL Creation", test_url , "/agent/257c631f-a0c5-4f29-8a9f-9031feaae6c6/name");
    }   

    template<> template<>
    void ServiceBuilderTestObject::test<3>()
    {
        //Incorrect service test
        LLSD test_block;
        test_block["service-builder"] = "/agent/{$agent-id}/name";
        mServiceBuilder.createServiceDefinition("ServiceBuilderTest", test_block["service-builder"]);   
        std::string test_url = mServiceBuilder.buildServiceURI("ServiceBuilder");
        ensure_equals("Replacement URL Creation for Non-existant Service", test_url , "");
    }

    template<> template<>
    void ServiceBuilderTestObject::test<4>()
    {
        //Incorrect service test
        LLSD test_block;
        test_block["service-builder"] = "/agent/{$agent-id}/name";
        mServiceBuilder.createServiceDefinition("ServiceBuilderTest", test_block["service-builder"]);
        LLSD data_map;
        data_map["agent_id"] = "257c631f-a0c5-4f29-8a9f-9031feaae6c6";
        std::string test_url = mServiceBuilder.buildServiceURI("ServiceBuilderTest", data_map);
        ensure_equals("Replacement URL Creation for Non-existant Service", test_url , "/agent/{$agent-id}/name");
    }

    template<> template<>
    void ServiceBuilderTestObject::test<5>()
    {
        LLSD test_block;
        test_block["service-builder"] = "/proc/{$proc}{%params}";
        mServiceBuilder.createServiceDefinition("ServiceBuilderTest", test_block["service-builder"]);   
        LLSD data_map;
        data_map["proc"] = "do/something/useful";
        data_map["params"]["estate_id"] = 1;
        data_map["params"]["query"] = "public";
        std::string test_url = mServiceBuilder.buildServiceURI("ServiceBuilderTest", data_map);
        ensure_equals(
            "two part URL Creation",
            test_url ,
            "/proc/do/something/useful?estate_id=1&query=public");
    }

    template<> template<>
    void ServiceBuilderTestObject::test<6>()
    {
        LLSD test_block;
        test_block["service-builder"] = "Which way to the {${$baz}}?";
        mServiceBuilder.createServiceDefinition(
            "ServiceBuilderTest",
            test_block["service-builder"]); 

        LLSD data_map;
        data_map["foo"] = "bar";
        data_map["baz"] = "foo";
        std::string test_url = mServiceBuilder.buildServiceURI(
            "ServiceBuilderTest",
            data_map);
        ensure_equals(
            "recursive url creation",
            test_url ,
            "Which way to the bar?");
    }

    template<> template<>
    void ServiceBuilderTestObject::test<7>()
    {
        LLSD test_block;
        test_block["service-builder"] = "Which way to the {$foo}?";
        mServiceBuilder.createServiceDefinition(
            "ServiceBuilderTest",
            test_block["service-builder"]); 

        LLSD data_map;
        data_map["baz"] = "foo";
        std::string test_url = mServiceBuilder.buildServiceURI(
            "ServiceBuilderTest",
            data_map);
        ensure_equals(
            "fails to do replacement",
            test_url ,
            "Which way to the {$foo}?");
    }

    template<> template<>
    void ServiceBuilderTestObject::test<8>()
    {
        LLSD test_block;
        test_block["service-builder"] = "/proc/{$proc}{%params}";
        mServiceBuilder.createServiceDefinition(
            "ServiceBuilderTest",
            test_block["service-builder"]); 
        LLSD data_map;
        data_map["proc"] = "do/something/useful";
        data_map["params"] = LLSD();
        std::string test_url = mServiceBuilder.buildServiceURI(
            "ServiceBuilderTest",
            data_map);
        ensure_equals(
            "strip params",
            test_url ,
            "/proc/do/something/useful");
    }
}

