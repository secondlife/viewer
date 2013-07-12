/** 
 * @file llagentaccess.cpp
 * @brief LLAgentAccess class implementation - manages maturity and godmode info
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llviewerprecompiledheaders.h"

#include "llagentaccess.h"
#include "indra_constants.h"
#include "llcontrol.h"

LLAgentAccess::LLAgentAccess(LLControlGroup& savedSettings) :
	mSavedSettings(savedSettings),
	mAccess(SIM_ACCESS_PG),
	mAdminOverride(false),
	mGodLevel(GOD_NOT)
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

bool LLAgentAccess::isGodlikeWithoutAdminMenuFakery() const
{
#ifdef HACKED_GODLIKE_VIEWER
	return true;
#else
	return mGodLevel > GOD_NOT;
#endif
}

U8 LLAgentAccess::getGodLevel() const
{
#ifdef HACKED_GODLIKE_VIEWER
	return GOD_MAINTENANCE;
#else
	if(mAdminOverride) return GOD_FULL; // :(
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
	U32 preferred_access = mSavedSettings.getU32("PreferredMaturity");
	while (!canSetMaturity(preferred_access))
	{
		if (preferred_access == SIM_ACCESS_ADULT)
		{
			preferred_access = SIM_ACCESS_MATURE;
		}
		else
		{
			// Mature or invalid access gets set to PG
			preferred_access = SIM_ACCESS_PG;
		}
	}
	mSavedSettings.setU32("PreferredMaturity", preferred_access);
}

bool LLAgentAccess::canSetMaturity(S32 maturity)
{
	if (isGodlike()) // Gods can always set their Maturity level
		return true;
	if (isAdult()) // Adults can always set their Maturity level
		return true;
	if (maturity == SIM_ACCESS_PG || (maturity == SIM_ACCESS_MATURE && isMature()))
		return true;
	else
		return false;
}
