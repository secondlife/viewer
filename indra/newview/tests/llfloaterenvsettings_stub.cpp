/** 
 * @file llfloaterenvsettings_stub.cpp
 * @brief  stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

class LLFloaterEnvSettings
{
public:
	static bool isOpen(void);
	static LLFloaterEnvSettings* instance(void);
	void syncMenu(void);
};

bool LLFloaterEnvSettings::isOpen() { return true; }
LLFloaterEnvSettings* LLFloaterEnvSettings::instance() { return NULL; }
void LLFloaterEnvSettings::syncMenu(void) {}
