/**
 * @file llcontrol_tut.cpp
 * @date   February 2008
 * @brief control group unit tests
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
#include "llsdserialize.h"
#include "llfile.h"
#include "stringize.h"

#include "../llcontrol.h"

#include "../test/lldoctest.h"
#include <memory>
#include <vector>

TEST_SUITE("UnknownSuite") {

struct control_group
{

        std::unique_ptr<LLControlGroup> mCG;
        std::string mTestConfigDir;
        std::string mTestConfigFile;
        std::vector<std::string> mCleanups;
        static bool mListenerFired;
        control_group()
        {
            mCG.reset(new LLControlGroup("foo"));
            LLUUID random;
            random.generate();
            // generate temp dir
            mTestConfigDir = STRINGIZE(LLFile::tmpdir() << "llcontrol-test-" << random << "/");
            mTestConfigFile = mTestConfigDir + "settings.xml";
            LLFile::mkdir(mTestConfigDir);
            LLSD config;
            config["TestSetting"]["Comment"] = "Dummy setting used for testing";
            config["TestSetting"]["Persist"] = 1;
            config["TestSetting"]["Type"] = "U32";
            config["TestSetting"]["Value"] = 12;
            writeSettingsFile(config);
        
};

TEST_CASE_FIXTURE(control_group, "test_1")
{

        int results = mCG->loadFromFile(mTestConfigFile.c_str());
        CHECK_MESSAGE((results == 1, "number of settings"));
        CHECK_MESSAGE((mCG->getU32("TestSetting", "value of setting") == 12));
    
}

TEST_CASE_FIXTURE(control_group, "test_2")
{

        int results = mCG->loadFromFile(mTestConfigFile.c_str());
        mCG->setU32("TestSetting", 13);
        ensure_equals("value of changed setting", mCG->getU32("TestSetting"), 13);
        LLControlGroup test_cg("foo2");
        std::string temp_test_file = (mTestConfigDir + "setting_llsd_temp.xml");
        mCleanups.push_back(temp_test_file);
        mCG->saveToFile(temp_test_file.c_str(), true);
        results = test_cg.loadFromFile(temp_test_file.c_str());
        CHECK_MESSAGE((results == 1, "number of changed settings loaded"));
        CHECK_MESSAGE((test_cg.getU32("TestSetting", "value of changed settings loaded") == 13));
    
}

TEST_CASE_FIXTURE(control_group, "test_3")
{

        // Pass default_values = true. This tells loadFromFile() we're loading
        // a default settings file that declares variables, rather than a user
        // settings file. When loadFromFile() encounters an unrecognized user
        // settings variable, it forcibly preserves it (CHOP-962).
        int results = mCG->loadFromFile(mTestConfigFile.c_str(), true);
        LLControlVariable* control = mCG->getControl("TestSetting");
        LLSD new_value = 13;
        control->setValue(new_value, false);
        ensure_equals("value of changed setting", mCG->getU32("TestSetting"), 13);
        LLControlGroup test_cg("foo3");
        std::string temp_test_file = (mTestConfigDir + "setting_llsd_persist_temp.xml");
        mCleanups.push_back(temp_test_file);
        mCG->saveToFile(temp_test_file.c_str(), true);
        results = test_cg.loadFromFile(temp_test_file.c_str());
        //If we haven't changed any settings, then we shouldn't have any settings to load
        CHECK_MESSAGE((results == 0, "number of non-persisted changed settings loaded"));
    
}

TEST_CASE_FIXTURE(control_group, "test_4")
{

        int results = mCG->loadFromFile(mTestConfigFile.c_str());
        CHECK_MESSAGE((results == 1, "number of settings"));
        mCG->getControl("TestSetting")->getSignal()->connect(boost::bind(&this->handleListenerTest));
        mCG->setU32("TestSetting", 13);
        CHECK_MESSAGE(mListenerFired, "listener fired on changed setting");
    
}

} // TEST_SUITE

