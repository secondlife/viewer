/** 
 * @file llfloaterdaycycle_stub.cpp
 * @brief  stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

class LLFloaterDayCycle
{
public:
	static bool isOpen(void);
	static LLFloaterDayCycle* instance(void);
	static void syncMenu(void);
};

bool LLFloaterDayCycle::isOpen() { return true; }
LLFloaterDayCycle* LLFloaterDayCycle::instance() { return NULL; }
void LLFloaterDayCycle::syncMenu(void) {}
