/** 
 * @file llfloaterwindlight_stub.cpp
 * @brief  stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

class LLFloaterWindLight
{
public:
	static bool isOpen(void);
	static LLFloaterWindLight* instance(void);
	void syncMenu(void);
};

bool LLFloaterWindLight::isOpen() { return true; }
LLFloaterWindLight* LLFloaterWindLight::instance() { return NULL; }
void LLFloaterWindLight::syncMenu(void) {}
