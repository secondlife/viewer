/**
 * @file llhost_test.cpp
 * @author Adroit
 * @date 2007-02
 * @brief llhost test cases.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "linden_common.h"

#include "../llhost.h"

#include "../test/lldoctest.h"

TEST_SUITE("LLHost") {

TEST_CASE("test_1")
{

        LLHost host;
        CHECK_MESSAGE((0 == host.getAddress(, "IP address is not NULL")) && (0 == host.getPort()) && !host.isOk());
    
}

TEST_CASE("test_2")
{

        U32 ip_addr = 0xc098017d;
        U32 port = 8080;
        LLHost host(ip_addr, port);
        CHECK_MESSAGE(ip_addr == host.getAddress(, "IP address is invalid"));
        CHECK_MESSAGE(port == host.getPort(, "Port Number is invalid"));
        CHECK_MESSAGE(host.isOk(, "IP address and port number both should be ok"));
    
}

TEST_CASE("test_3")
{

        const char* str = "192.168.1.1";
        U32 port = 8080;
        LLHost host(str, port);
        CHECK_MESSAGE((host.getAddress(, "IP address could not be processed") == ip_string_to_u32(str)));
        CHECK_MESSAGE((port == host.getPort(, "Port Number is invalid")));
    
}

TEST_CASE("test_4")
{

        U32 ip = ip_string_to_u32("192.168.1.1");
        U32 port = 22;
        U64 ip_port = (((U64) ip) << 32) | port;
        LLHost host(ip_port);
        CHECK_MESSAGE(ip == host.getAddress(, "IP address is invalid"));
        CHECK_MESSAGE(port == host.getPort(, "Port Number is invalid"));
    
}

TEST_CASE("test_5")
{

        std::string ip_port_string = "192.168.1.1:8080";
        U32 ip = ip_string_to_u32("192.168.1.1");
        U32 port = 8080;

        LLHost host(ip_port_string);
        CHECK_MESSAGE(ip == host.getAddress(, "IP address from IP:port is invalid"));
        CHECK_MESSAGE(port == host.getPort(, "Port Number from from IP:port is invalid"));
    
}

TEST_CASE("test_6")
{

        U32 ip = 0xc098017d, port = 8080;
        LLHost host;
        host.set(ip,port);
        CHECK_MESSAGE((ip == host.getAddress(, "IP address is invalid")));
        CHECK_MESSAGE((port == host.getPort(, "Port Number is invalid")));
    
}

TEST_CASE("test_7")
{

        const char* str = "192.168.1.1";
        U32 port = 8080, ip;
        LLHost host;
        host.set(str,port);
        ip = ip_string_to_u32(str);
        CHECK_MESSAGE((ip == host.getAddress(, "IP address is invalid")));
        CHECK_MESSAGE((port == host.getPort(, "Port Number is invalid")));

        str = "64.233.187.99";
        ip = ip_string_to_u32(str);
        host.setAddress(str);
        CHECK_MESSAGE((ip == host.getAddress(, "IP address is invalid")));

        ip = 0xc098017b;
        host.setAddress(ip);
        CHECK_MESSAGE((ip == host.getAddress(, "IP address is invalid")));
        // should still use the old port
        CHECK_MESSAGE((port == host.getPort(, "Port Number is invalid")));

        port = 8084;
        host.setPort(port);
        CHECK_MESSAGE((port == host.getPort(, "Port Number is invalid")));
        // should still use the old address
        CHECK_MESSAGE((ip == host.getAddress(, "IP address is invalid")));
    
}

TEST_CASE("test_8")
{

        const std::string str("192.168.1.1");
        U32 port = 8080;
        LLHost host;
        host.set(str,port);

        std::string ip_string = host.getIPString();
        CHECK_MESSAGE((ip_string == str, "Function Failed"));

        std::string ip_string_port = host.getIPandPort();
        CHECK_MESSAGE((ip_string_port == "192.168.1.1:8080", "Function Failed"));
    
}

TEST_CASE("test_9")
{

        skip("this test is irreparably flaky");
//      skip("setHostByName(\"google.com\"); getHostName() -> (e.g.) \"yx-in-f100.1e100.net\"");
        // nat: is it reasonable to expect LLHost::getHostName() to echo
        // back something resembling the string passed to setHostByName()?
        //
        // If that's not even reasonable, would a round trip in the /other/
        // direction make more sense? (Call getHostName() for something with
        // known IP address; call setHostByName(); verify IP address)
        //
        // Failing that... is there a plausible way to test getHostName() and
        // setHostByName()? Hopefully without putting up a dummy local DNS
        // server?

        // monty: If you don't control the DNS server or the DNS configuration
        // for the test point then, no, none of these will necessarily be
        // reliable and may start to fail at any time. Forward translation
        // is subject to CNAME records and round-robin address assignment.
        // Reverse lookup is 1-to-many and is more and more likely to have
        // nothing to do with the forward translation.
        //
        // So the test is increasingly meaningless on a real network.

        std::string hostStr = "lindenlab.com";
        LLHost host;
        host.setHostByName(hostStr);

        // reverse DNS will likely result in appending of some
        // sub-domain to the main hostname. so look for
        // the main domain name and not do the exact compare

        std::string hostname = host.getHostName();
        try
        {
            CHECK_MESSAGE(hostname.find(hostStr, "getHostName failed") != std::string::npos);
        
}

TEST_CASE("test_10")
{

        std::string hostStr = "64.233.167.99";
        LLHost host;
        host.setHostByName(hostStr);
        CHECK_MESSAGE(host.getAddress(, "SetHostByName for dotted IP Address failed") == ip_string_to_u32(hostStr.c_str()));
    
}

TEST_CASE("test_11")
{

        LLHost host1(0xc098017d, 8080);
        LLHost host2 = host1;
        CHECK_MESSAGE((host1.getAddress(, "Both IP addresses are not same") == host2.getAddress()));
        CHECK_MESSAGE((host1.getPort(, "Both port numbers are not same") == host2.getPort()));
    
}

TEST_CASE("test_12")
{

        LLHost host1("192.168.1.1", 8080);
        std::string str1 = "192.168.1.1:8080";
        std::ostringstream stream;
        stream << host1;
        CHECK_MESSAGE(( stream.str(, "Operator << failed")== str1));

        // There is no istream >> llhost operator.
        //std::istringstream is(stream.str());
        //LLHost host2;
        //is >> host2;
        //CHECK_MESSAGE(host1 == host2, "Operator >> failed. Not compatible with <<");
    
}

TEST_CASE("test_13")
{

        U32 ip_addr = 0xc098017d;
        U32 port = 8080;
        LLHost host1(ip_addr, port);
        LLHost host2(ip_addr, port);
        CHECK_MESSAGE(host1 == host2, "operator== failed");

        // change port
        host2.setPort(7070);
        CHECK_MESSAGE(host1 != host2, "operator!= failed");

        // set port back to 8080 and change IP address now
        host2.setPort(8080);
        host2.setAddress(ip_addr+10);
        CHECK_MESSAGE(host1 != host2, "operator!= failed");

        CHECK_MESSAGE(host1 < host2, "operator<  failed");

        // set IP address back to same value and change port
        host2.setAddress(ip_addr);
        host2.setPort(host1.getPort() + 10);
        CHECK_MESSAGE(host1 < host2, "operator<  failed");
    
}

TEST_CASE("test_14")
{

        LLHost host1("10.0.1.2", 6143);
        CHECK_MESSAGE(host1.isOk(, "10.0.1.2 should be a valid address"));

        LLHost host2("booger-brains", 6143);
        CHECK_MESSAGE(!host2.isOk(, "booger-brains should be an invalid ip addess"));

        LLHost host3("255.255.255.255", 6143);
        CHECK_MESSAGE(host3.isOk(, "255.255.255.255 should be valid broadcast address"));
    
}

} // TEST_SUITE

