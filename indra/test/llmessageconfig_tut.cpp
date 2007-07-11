/** 
 * @file llmessageconfig_tut.cpp
 * @date   March 2007
 * @brief LLMessageConfig unit tests
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include <tut/tut.h>
#include "lltut.h"
#include "llmessageconfig.h"
#include "llsdserialize.h"
#include "llfile.h"
#include "lldir.h"
#include "lltimer.h"
#include "llframetimer.h"

namespace tut
{
	struct LLMessageConfigTestData {
		std::string mTestConfigDir;

		LLMessageConfigTestData()
		{
			// generate temp dir
			mTestConfigDir = "/tmp/llmessage-config-test";
			LLFile::mkdir(mTestConfigDir.c_str());

			LLMessageConfig::initClass("simulator", mTestConfigDir);
		}

		~LLMessageConfigTestData()
		{
			// rm contents of temp dir
			gDirUtilp->deleteFilesInDir(mTestConfigDir, "*");
			// rm temp dir
			LLFile::rmdir(mTestConfigDir.c_str());
		}

		void reloadConfig(const LLSD& config)
		{
			LLMessageConfig::useConfig(config);
		}
		
		void writeConfigFile(const LLSD& config)
		{
			std::string configFile = mTestConfigDir + "/message.xml";
			llofstream file(configFile.c_str());
			if (file.is_open())
			{
				LLSDSerialize::toPrettyXML(config, file);
			}
			file.close();
		}
	};
	
	typedef test_group<LLMessageConfigTestData> LLMessageConfigTestGroup;
	typedef LLMessageConfigTestGroup::object LLMessageConfigTestObject;
	LLMessageConfigTestGroup llMessageConfigTestGroup("LLMessageConfig");
	
	template<> template<>
	void LLMessageConfigTestObject::test<1>()
		// tests server defaults
	{
		LLSD config;
		config["serverDefaults"]["simulator"] = "template";
		reloadConfig(config);
		ensure_equals("Ensure server default is not template",
					  LLMessageConfig::getServerDefaultFlavor(),
					  LLMessageConfig::TEMPLATE_FLAVOR);
	}

	template<> template<>
	void LLMessageConfigTestObject::test<2>()
		// tests message flavors
	{
		LLSD config;
		config["serverDefaults"]["simulator"] = "template";
		config["messages"]["msg1"]["flavor"] = "template";
		config["messages"]["msg2"]["flavor"] = "llsd";
		reloadConfig(config);
		ensure_equals("Ensure msg template flavor",
					  LLMessageConfig::getMessageFlavor("msg1"),
					  LLMessageConfig::TEMPLATE_FLAVOR);
		ensure_equals("Ensure msg llsd flavor",
					  LLMessageConfig::getMessageFlavor("msg2"),
					  LLMessageConfig::LLSD_FLAVOR);
	}

	template<> template<>
	void LLMessageConfigTestObject::test<4>()
		// tests message flavor defaults
	{
		LLSD config;
		config["serverDefaults"]["simulator"] = "llsd";
		config["messages"]["msg1"]["trusted-sender"] = true;
		reloadConfig(config);
		ensure_equals("Ensure missing message gives no flavor",
					  LLMessageConfig::getMessageFlavor("Test"),
					  LLMessageConfig::NO_FLAVOR);
		ensure_equals("Ensure missing flavor is NO_FLAVOR even with sender trustedness set",
					  LLMessageConfig::getMessageFlavor("msg1"),
					  LLMessageConfig::NO_FLAVOR);
		ensure_equals("Ensure server default is llsd",
					  LLMessageConfig::getServerDefaultFlavor(),
					  LLMessageConfig::LLSD_FLAVOR);
	}

	template<> template<>
	void LLMessageConfigTestObject::test<3>()
		// tests trusted/untrusted senders
	{
		LLSD config;
		config["serverDefaults"]["simulator"] = "template";
		config["messages"]["msg1"]["flavor"] = "llsd";
		config["messages"]["msg1"]["trusted-sender"] = false;
		config["messages"]["msg2"]["flavor"] = "llsd";
		config["messages"]["msg2"]["trusted-sender"] = true;
		reloadConfig(config);
		ensure_equals("Ensure untrusted is untrusted",
					  LLMessageConfig::getSenderTrustedness("msg1"),
					  LLMessageConfig::UNTRUSTED);
		ensure_equals("Ensure trusted is trusted",
					  LLMessageConfig::getSenderTrustedness("msg2"),
					  LLMessageConfig::TRUSTED);
		ensure_equals("Ensure missing trustedness is NOT_SET",
					  LLMessageConfig::getSenderTrustedness("msg3"),
					  LLMessageConfig::NOT_SET);
	}
	
	template<> template<>
	void LLMessageConfigTestObject::test<5>()
		// tests trusted/untrusted without flag, only flavor
	{
		LLSD config;
		config["serverDefaults"]["simulator"] = "template";
		config["messages"]["msg1"]["flavor"] = "llsd";
		reloadConfig(config);
		ensure_equals("Ensure msg1 exists, has llsd flavor",
					  LLMessageConfig::getMessageFlavor("msg1"),
					  LLMessageConfig::LLSD_FLAVOR);
		ensure_equals("Ensure missing trusted is not set",
					  LLMessageConfig::getSenderTrustedness("msg1"),
					  LLMessageConfig::NOT_SET);
	}

	template<> template<>
	void LLMessageConfigTestObject::test<6>()
	{
		LLSD config;
		config["capBans"]["MapLayer"] = true;
		config["capBans"]["MapLayerGod"] = false;
		reloadConfig(config);
		ensure_equals("Ensure cap ban true MapLayer",
					  LLMessageConfig::isCapBanned("MapLayer"),
					  true);
		ensure_equals("Ensure cap ban false",
					  LLMessageConfig::isCapBanned("MapLayerGod"),
					  false);
	}

	template<> template<>
	void LLMessageConfigTestObject::test<7>()
		// tests that config changes are picked up/refreshed periodically
	{
		LLSD config;
		config["serverDefaults"]["simulator"] = "llsd";
		writeConfigFile(config);

		// wait for it to reload after N seconds
		ms_sleep(6*1000);
		LLFrameTimer::updateFrameTime();
		ensure_equals("Ensure reload after 6 seconds",
					  LLMessageConfig::getServerDefaultFlavor(),
					  LLMessageConfig::LLSD_FLAVOR);
	}
}
