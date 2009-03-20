/** 
 * @file llagentaccess.cpp
 * @brief LLAgentAccess class implementation - manages maturity and godmode info
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * $/LicenseInfo$
 */
#include "llviewerprecompiledheaders.h"

#include "llagentaccess.h"
#include "indra_constants.h"
#include "llcontrolgroupreader.h"

LLAgentAccess::LLAgentAccess(LLControlGroupReader& savedSettings) :
	mSavedSettings(savedSettings),
	mAccess(SIM_ACCESS_PG),
	mAdminOverride(false),
	mGodLevel(GOD_NOT),
	mAOTransition(false)
{
}

bool LLAgentAccess::getAdminOverride() const	
{ 
	return mAdminOverride; 
}

void LLAgentAccess::setAdminOverride(bool b)
{ 
	mAdminOverride = b; 
}

void LLAgentAccess::setGodLevel(U8 god_level)
{ 
	mGodLevel = god_level; 
}

bool LLAgentAccess::isGodlike() const
{
#ifdef HACKED_GODLIKE_VIEWER
	return true;
#else
	if(mAdminOverride) return true;
	return mGodLevel > GOD_NOT;
#endif
}

U8 LLAgentAccess::getGodLevel() const
{
#ifdef HACKED_GODLIKE_VIEWER
	return GOD_MAINTENANCE;
#else
	if(mAdminOverride) return GOD_FULL;
	return mGodLevel;
#endif
}

bool LLAgentAccess::wantsPGOnly() const
{
	return (prefersPG() || isTeen()) && !isGodlike();
}

bool LLAgentAccess::canAccessMature() const
{
	// if you prefer mature, you're either mature or adult, and
	// therefore can access all mature content
	return isGodlike() || (prefersMature() && !isTeen());
}

bool LLAgentAccess::canAccessAdult() const
{
	// if you prefer adult, you must BE adult.
	return isGodlike() || (prefersAdult() && isAdult());
}

bool LLAgentAccess::prefersPG() const
{
	U32 access = mSavedSettings.getU32("PreferredMaturity");
	return access < SIM_ACCESS_MATURE;
}

bool LLAgentAccess::prefersMature() const
{
	U32 access = mSavedSettings.getU32("PreferredMaturity");
	return access >= SIM_ACCESS_MATURE;
}

bool LLAgentAccess::prefersAdult() const
{
	U32 access = mSavedSettings.getU32("PreferredMaturity");
	return access >= SIM_ACCESS_ADULT;
}

bool LLAgentAccess::isTeen() const
{
	return mAccess < SIM_ACCESS_MATURE;
}

bool LLAgentAccess::isMature() const
{
	return mAccess >= SIM_ACCESS_MATURE;
}

bool LLAgentAccess::isAdult() const
{
	return mAccess >= SIM_ACCESS_ADULT;
}

void LLAgentAccess::setTeen(bool teen)
{
	if (teen)
	{
		mAccess = SIM_ACCESS_PG;
	}
	else
	{
		mAccess = SIM_ACCESS_MATURE;
	}
}

//static 
int LLAgentAccess::convertTextToMaturity(char text)
{
	if ('A' == text)
	{
		return SIM_ACCESS_ADULT;
	}
	else if ('M'== text)
	{
		return SIM_ACCESS_MATURE;
	}
	else if ('P'== text)
	{
		return SIM_ACCESS_PG;
	}
	return SIM_ACCESS_MIN;
}

void LLAgentAccess::setMaturity(char text)
{
	mAccess = LLAgentAccess::convertTextToMaturity(text);
}

void LLAgentAccess::setTransition()
{
	mAOTransition = true;
}

bool LLAgentAccess::isInTransition() const
{
	return mAOTransition;
}

