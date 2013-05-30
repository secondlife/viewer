/** 
 * @file llapp_tut.cpp
 * @author Phoenix
 * @date 2006-09-12
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2006-2011, Linden Research, Inc.
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
#include "llapp.h"
#include "lltut.h"


namespace tut
{
	struct application
	{
		class LLTestApp : public LLApp
		{
		public:
			virtual bool init() { return true; }
			virtual bool cleanup() { return true; }
			virtual bool mainLoop() { return true; }
		};
		LLTestApp* mApp;
		application()
		{
			mApp = new LLTestApp;
		}
		~application()
		{
			delete mApp;
		}
	};

	typedef test_group<application> application_t;
	typedef application_t::object application_object_t;
	tut::application_t tut_application("application");

	template<> template<>
	void application_object_t::test<1>()
	{
		LLSD defaults;
		defaults["template"] = "../../../scripts/messages/message_template.msg";
		defaults["configdir"] = ".";
		defaults["datadir"] = "data";
		mApp->setOptionData(LLApp::PRIORITY_DEFAULT, defaults);

		LLSD datadir_sd = mApp->getOption("datadir");
		ensure_equals("data type", datadir_sd.type(), LLSD::TypeString);
		ensure_equals(
			"data value", datadir_sd.asString(), std::string("data"));
	}

	template<> template<>
	void application_object_t::test<2>()
	{
		const int ARGC = 13;
		const char* ARGV[ARGC] =
		{
			"", // argv[0] is usually the application name
			"-crashcount",
			"2",
			"-space",
			"spaceserver.grid.lindenlab.com",
			"-db_host",
			"localhost",
			"--allowlslhttprequests",
			"-asset-uri",
			"http://test.lindenlab.com/assets",
			"-data",
			"127.0.0.1",
			"--smtp"
		};
		bool ok = mApp->parseCommandOptions(ARGC, const_cast<char**>(ARGV));
		ensure("command line parsed", ok);
		ensure_equals(
			"crashcount", mApp->getOption("crashcount").asInteger(), 2);
		ensure_equals(
			"space",
			mApp->getOption("space").asString(),
			std::string("spaceserver.grid.lindenlab.com"));
		ensure_equals(
			"db_host",
			mApp->getOption("db_host").asString(),
			std::string("localhost"));
		ensure("allowlshlttprequests", mApp->getOption("smtp"));
		ensure_equals(
			"asset-uri",
			mApp->getOption("asset-uri").asString(),
			std::string("http://test.lindenlab.com/assets"));
		ensure_equals(
			"data",
			mApp->getOption("data").asString(),
			std::string("127.0.0.1"));
		ensure("smtp", mApp->getOption("smtp"));
	}

	template<> template<>
	void application_object_t::test<3>()
	{
		const int ARGC = 4;
		const char* ARGV[ARGC] =
		{
			"", // argv[0] is usually the application name
			"crashcount",
			"2",
			"--space"
		};
		bool ok = mApp->parseCommandOptions(ARGC, const_cast<char**>(ARGV));
		ensure("command line parse failure", !ok);
	}

	template<> template<>
	void application_object_t::test<4>()
	{
		const int ARGC = 4;
		const char* ARGV[ARGC] =
		{
			"", // argv[0] is usually the application name
			"--crashcount",
			"2",
			"space"
		};
		bool ok = mApp->parseCommandOptions(ARGC, const_cast<char**>(ARGV));
		ensure("command line parse failure", !ok);
	}


	template<> template<>
	void application_object_t::test<5>()
	{
		LLSD options;
		options["boolean-test"] = true;
		mApp->setOptionData(LLApp::PRIORITY_GENERAL_CONFIGURATION, options);
		ensure("bool set", mApp->getOption("boolean-test").asBoolean());
		options["boolean-test"] = false;
		mApp->setOptionData(LLApp::PRIORITY_RUNTIME_OVERRIDE, options);
		ensure("bool unset", !mApp->getOption("boolean-test").asBoolean());
	}
}
