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

#include "../test/lltut.h"

namespace tut
{
	struct host_data
	{
	};
	typedef test_group<host_data> host_test;
	typedef host_test::object host_object;
	tut::host_test host_testcase("LLHost");


	template<> template<>
	void host_object::test<1>()
	{
		LLHost host;
		ensure("IP address is not NULL", (0 == host.getAddress()) && (0 == host.getPort()) && !host.isOk());
	}
	template<> template<>
	void host_object::test<2>()
	{
		U32 ip_addr = 0xc098017d;
		U32 port = 8080;
		LLHost host(ip_addr, port);
		ensure("IP address is invalid", ip_addr == host.getAddress());
		ensure("Port Number is invalid", port == host.getPort());
		ensure("IP address and port number both should be ok", host.isOk());
	}

	template<> template<>
	void host_object::test<3>()
	{
		const char* str = "192.168.1.1";
		U32 port = 8080;
		LLHost host(str, port);
		ensure("IP address could not be processed", (host.getAddress() == ip_string_to_u32(str)));
		ensure("Port Number is invalid", (port == host.getPort()));
	}

	template<> template<>
	void host_object::test<4>()
	{
		U32 ip = ip_string_to_u32("192.168.1.1");
		U32 port = 22;
		U64 ip_port = (((U64) ip) << 32) | port;
		LLHost host(ip_port);
		ensure("IP address is invalid", ip == host.getAddress());
		ensure("Port Number is invalid", port == host.getPort());
	}

	template<> template<>
	void host_object::test<5>()
	{
		std::string ip_port_string = "192.168.1.1:8080";
		U32 ip = ip_string_to_u32("192.168.1.1");
		U32 port = 8080;

		LLHost host(ip_port_string);
		ensure("IP address from IP:port is invalid", ip == host.getAddress());
		ensure("Port Number from from IP:port is invalid", port == host.getPort());
	}

	template<> template<>
	void host_object::test<6>()
	{
		U32 ip = 0xc098017d, port = 8080;
		LLHost host;
		host.set(ip,port);
		ensure("IP address is invalid", (ip == host.getAddress()));
		ensure("Port Number is invalid", (port == host.getPort()));
	}

	template<> template<>
	void host_object::test<7>()
	{
		const char* str = "192.168.1.1";
		U32 port = 8080, ip;
		LLHost host;
		host.set(str,port);
		ip = ip_string_to_u32(str);
		ensure("IP address is invalid", (ip == host.getAddress()));
		ensure("Port Number is invalid", (port == host.getPort()));
		
		str = "64.233.187.99";
		ip = ip_string_to_u32(str);
		host.setAddress(str);
		ensure("IP address is invalid", (ip == host.getAddress()));

		ip = 0xc098017b;
		host.setAddress(ip);
		ensure("IP address is invalid", (ip == host.getAddress()));
		// should still use the old port
		ensure("Port Number is invalid", (port == host.getPort()));

		port = 8084;
		host.setPort(port);
		ensure("Port Number is invalid", (port == host.getPort()));
		// should still use the old address
		ensure("IP address is invalid", (ip == host.getAddress()));
	}

	template<> template<>
	void host_object::test<8>()
	{
		const std::string str("192.168.1.1");
		U32 port = 8080;
		LLHost host;
		host.set(str,port);

		std::string ip_string = host.getIPString();
		ensure("Function Failed", (ip_string == str));

		std::string ip_string_port = host.getIPandPort();
		ensure("Function Failed", (ip_string_port == "192.168.1.1:8080"));
	}


//	getHostName()  and setHostByName
	template<> template<>
	void host_object::test<9>()
	{
		skip("this test is flaky, but we should figure out why...");
//		skip("setHostByName(\"google.com\"); getHostName() -> (e.g.) \"yx-in-f100.1e100.net\"");
		std::string hostStr = "lindenlab.com";		
		LLHost host;
		host.setHostByName(hostStr);	

		// reverse DNS will likely result in appending of some
		// sub-domain to the main hostname. so look for
		// the main domain name and not do the exact compare
		
		std::string hostname = host.getHostName();
		try
		{
			ensure("getHostName failed", hostname.find(hostStr) != std::string::npos);
		}
		catch (const std::exception&)
		{
			std::cerr << "set '" << hostStr << "'; reported '" << hostname << "'" << std::endl;
			throw;
		}
	}

//	setHostByName for dotted IP
	template<> template<>
	void host_object::test<10>()
	{
		std::string hostStr = "64.233.167.99";		
		LLHost host;
		host.setHostByName(hostStr);	
		ensure("SetHostByName for dotted IP Address failed", host.getAddress() == ip_string_to_u32(hostStr.c_str()));
	}

	template<> template<>
	void host_object::test<11>()
	{
		LLHost host1(0xc098017d, 8080);
		LLHost host2 = host1;
		ensure("Both IP addresses are not same", (host1.getAddress() == host2.getAddress()));
		ensure("Both port numbers are not same", (host1.getPort() == host2.getPort()));
	}

	template<> template<>
	void host_object::test<12>()
	{
		LLHost host1("192.168.1.1", 8080);
		std::string str1 = "192.168.1.1:8080";
		std::ostringstream stream;
		stream << host1;
		ensure("Operator << failed", ( stream.str()== str1));

		// There is no istream >> llhost operator.
		//std::istringstream is(stream.str());
		//LLHost host2;
		//is >> host2;
		//ensure("Operator >> failed. Not compatible with <<", host1 == host2);
	}

	// operators ==, !=, <
	template<> template<>
	void host_object::test<13>()
	{
		U32 ip_addr = 0xc098017d;
		U32 port = 8080;
		LLHost host1(ip_addr, port);
		LLHost host2(ip_addr, port);
		ensure("operator== failed", host1 == host2);

		// change port
		host2.setPort(7070);
		ensure("operator!= failed", host1 != host2);

		// set port back to 8080 and change IP address now
		host2.setPort(8080);
		host2.setAddress(ip_addr+10);
		ensure("operator!= failed", host1 != host2);

		ensure("operator<  failed", host1 < host2);

		// set IP address back to same value and change port
		host2.setAddress(ip_addr);
		host2.setPort(host1.getPort() + 10);
		ensure("operator<  failed", host1 < host2);
	}

	// invalid ip address string
	template<> template<>
	void host_object::test<14>()
	{
		LLHost host1("10.0.1.2", 6143);
		ensure("10.0.1.2 should be a valid address", host1.isOk());

		LLHost host2("booger-brains", 6143);
		ensure("booger-brains should be an invalid ip addess", !host2.isOk());

		LLHost host3("255.255.255.255", 6143);
		ensure("255.255.255.255 should be valid broadcast address", host3.isOk());
	}
}
