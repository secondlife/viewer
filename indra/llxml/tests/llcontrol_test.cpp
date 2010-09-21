/** 
 * @file llcontrol_tut.cpp
 * @date   February 2008
 * @brief control group unit tests
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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
#include "llsdserialize.h"

#include "../llcontrol.h"

#include "../test/lltut.h"

namespace tut
{

	struct control_group
	{
		LLControlGroup* mCG;
		std::string mTestConfigDir;
		std::string mTestConfigFile;
		static bool mListenerFired;
		control_group()
		{
			mCG = new LLControlGroup("foo");
			LLUUID random;
			random.generate();
			// generate temp dir
			std::ostringstream oStr;

#ifdef LL_WINDOWS
			char* tmp_dir = getenv("TMP");
			if(tmp_dir)
			{
				oStr << tmp_dir << "/llcontrol-test-" << random << "/";
			}
			else
			{
				oStr << "c:/tmp/llcontrol-test-" << random << "/";
			}
#else
			oStr << "/tmp/llcontrol-test-" << random << "/";
#endif

			mTestConfigDir = oStr.str();
			mTestConfigFile = mTestConfigDir + "settings.xml";
			LLFile::mkdir(mTestConfigDir);
			LLSD config;
			config["TestSetting"]["Comment"] = "Dummy setting used for testing";
			config["TestSetting"]["Persist"] = 1;
			config["TestSetting"]["Type"] = "U32";
			config["TestSetting"]["Value"] = 12;
			writeSettingsFile(config);
		}
		~control_group()
		{
			//Remove test files
			delete mCG;
		}
		void writeSettingsFile(const LLSD& config)
		{
			llofstream file(mTestConfigFile);
			if (file.is_open())
			{
				LLSDSerialize::toPrettyXML(config, file);
			}
			file.close();
		}
		static bool handleListenerTest()
		{
			control_group::mListenerFired = true;
			return true;
		}
	};

	bool control_group::mListenerFired = false;

	typedef test_group<control_group> control_group_test;
	typedef control_group_test::object control_group_t;
	control_group_test tut_control_group("control_group");

	//load settings from files - LLSD
	template<> template<>
	void control_group_t::test<1>()
	{
		int results = mCG->loadFromFile(mTestConfigFile.c_str());
		ensure("number of settings", (results == 1));
		ensure("value of setting", (mCG->getU32("TestSetting") == 12));
	}

	//save settings to files
	template<> template<>
	void control_group_t::test<2>()
	{
		int results = mCG->loadFromFile(mTestConfigFile.c_str());
		mCG->setU32("TestSetting", 13);
		ensure_equals("value of changed setting", mCG->getU32("TestSetting"), 13);
		LLControlGroup test_cg("foo2");
		std::string temp_test_file = (mTestConfigDir + "setting_llsd_temp.xml");
		mCG->saveToFile(temp_test_file.c_str(), TRUE);
		results = test_cg.loadFromFile(temp_test_file.c_str());
		ensure("number of changed settings loaded", (results == 1));
		ensure("value of changed settings loaded", (test_cg.getU32("TestSetting") == 13));
	}
   
	//priorities
	template<> template<>
	void control_group_t::test<3>()
	{
		int results = mCG->loadFromFile(mTestConfigFile.c_str());
		LLControlVariable* control = mCG->getControl("TestSetting");
		LLSD new_value = 13;
		control->setValue(new_value, FALSE);
		ensure_equals("value of changed setting", mCG->getU32("TestSetting"), 13);
		LLControlGroup test_cg("foo3");
		std::string temp_test_file = (mTestConfigDir + "setting_llsd_persist_temp.xml");
		mCG->saveToFile(temp_test_file.c_str(), TRUE);
		results = test_cg.loadFromFile(temp_test_file.c_str());
		//If we haven't changed any settings, then we shouldn't have any settings to load
		ensure("number of non-persisted changed settings loaded", (results == 0));
	}

	//listeners
	template<> template<>
	void control_group_t::test<4>()
	{
		int results = mCG->loadFromFile(mTestConfigFile.c_str());
		ensure("number of settings", (results == 1));
		mCG->getControl("TestSetting")->getSignal()->connect(boost::bind(&this->handleListenerTest));
		mCG->setU32("TestSetting", 13);
		ensure("listener fired on changed setting", mListenerFired);	   
	}

}
