/** 
 * @file llwldaycycle_stub.cpp
 * @brief  stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

LLWLDayCycle::LLWLDayCycle(void)
{
}

LLWLDayCycle::~LLWLDayCycle(void)
{
}

bool LLWLDayCycle::getKeytime(LLWLParamKey keyFrame, F32& keyTime)
{
	keyTime = 0.5;
	return true;
}

bool LLWLDayCycle::removeKeyframe(F32 time)
{
	return true;
}

void LLWLDayCycle::loadDayCycleFromFile(const std::string& fileName)
{
}

void LLWLDayCycle::removeReferencesTo(const LLWLParamKey &keyframe)
{
}
