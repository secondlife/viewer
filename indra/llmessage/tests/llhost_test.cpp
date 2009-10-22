/**
 * @file llhost_test.cpp
 * @author Adroit
 * @date 2007-02
 * @brief llhost test cases.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
	tut::host_test host_testcase("llhost");


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
		std::string hostStr = "linux.org";
		LLHost host;
		host.setHostByName(hostStr);	

		// reverse DNS will likely result in appending of some
		// sub-domain to the main hostname. so look for
		// the main domain name and not do the exact compare
		
		std::string hostname = host.getHostName();
		ensure("getHostName failed", hostname.find(hostStr) != std::string::npos);
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
