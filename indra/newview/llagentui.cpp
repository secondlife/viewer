/** 
 * @file llagentui.cpp
 * @brief Utility methods to process agent's data as slurl's etc. before displaying
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llagentui.h"

// Library includes
#include "llparcel.h"

// Viewer includes
#include "llagent.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewerparcelmgr.h"
#include "llvoavatarself.h"
#include "llslurl.h"

//static
void LLAgentUI::buildName(std::string& name)
{
	name.clear();
	if (isAgentAvatarValid())
	{
		LLNameValue *first_nv = gAgentAvatarp->getNVPair("FirstName");
		LLNameValue *last_nv = gAgentAvatarp->getNVPair("LastName");
		if (first_nv && last_nv)
		{
			name = first_nv->printData() + " " + last_nv->printData();
		}
		else
		{
			llwarns << "Agent is missing FirstName and/or LastName nv pair." << llendl;
		}
	}
	else
	{
		name = gSavedSettings.getString("FirstName") + " " + gSavedSettings.getString("LastName");
	}
}

//static
void LLAgentUI::buildFullname(std::string& name)
{
	if (isAgentAvatarValid())
		name = gAgentAvatarp->getFullname();
}

//static
std::string LLAgentUI::buildSLURL(const bool escaped /*= true*/)
{
	std::string slurl;
	LLViewerRegion *regionp = gAgent.getRegion();
	if (regionp)
	{
		LLVector3d agentPos = gAgent.getPositionGlobal();
		slurl = LLSLURL::buildSLURLfromPosGlobal(regionp->getName(), agentPos, escaped);
	}
	return slurl;
}

//static
BOOL LLAgentUI::checkAgentDistance(const LLVector3& pole, F32 radius)
{
	F32 delta_x = gAgent.getPositionAgent().mV[VX] - pole.mV[VX];
	F32 delta_y = gAgent.getPositionAgent().mV[VY] - pole.mV[VY];
	
	return  sqrt( delta_x* delta_x + delta_y* delta_y ) < radius;
}
BOOL LLAgentUI::buildLocationString(std::string& str, ELocationFormat fmt,const LLVector3& agent_pos_region)
{
	LLViewerRegion* region = gAgent.getRegion();
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

	if (!region || !parcel) return FALSE;

	S32 pos_x = S32(agent_pos_region.mV[VX]);
	S32 pos_y = S32(agent_pos_region.mV[VY]);
	S32 pos_z = S32(agent_pos_region.mV[VZ]);

	// Round the numbers based on the velocity
	F32 velocity_mag_sq = gAgent.getVelocity().magVecSquared();

	const F32 FLY_CUTOFF = 6.f;		// meters/sec
	const F32 FLY_CUTOFF_SQ = FLY_CUTOFF * FLY_CUTOFF;
	const F32 WALK_CUTOFF = 1.5f;	// meters/sec
	const F32 WALK_CUTOFF_SQ = WALK_CUTOFF * WALK_CUTOFF;

	if (velocity_mag_sq > FLY_CUTOFF_SQ)
	{
		pos_x -= pos_x % 4;
		pos_y -= pos_y % 4;
	}
	else if (velocity_mag_sq > WALK_CUTOFF_SQ)
	{
		pos_x -= pos_x % 2;
		pos_y -= pos_y % 2;
	}

	// create a default name and description for the landmark
	std::string parcel_name = LLViewerParcelMgr::getInstance()->getAgentParcelName();
	std::string region_name = region->getName();
	std::string sim_access_string = region->getSimAccessString();
	std::string buffer;
	if( parcel_name.empty() )
	{
		// the parcel doesn't have a name
		switch (fmt)
		{
		case LOCATION_FORMAT_LANDMARK:
			buffer = llformat("%.100s", region_name.c_str());
			break;
		case LOCATION_FORMAT_NORMAL:
			buffer = llformat("%s", region_name.c_str());
			break;
		case LOCATION_FORMAT_NO_COORDS:
			buffer = llformat("%s%s%s",
				region_name.c_str(),
				sim_access_string.empty() ? "" : " - ",
				sim_access_string.c_str());
			break;
		case LOCATION_FORMAT_NO_MATURITY:
			buffer = llformat("%s (%d, %d, %d)",
				region_name.c_str(),
				pos_x, pos_y, pos_z);
			break;
		case LOCATION_FORMAT_FULL:
			buffer = llformat("%s (%d, %d, %d)%s%s",
				region_name.c_str(),
				pos_x, pos_y, pos_z,
				sim_access_string.empty() ? "" : " - ",
				sim_access_string.c_str());
			break;
		}
	}
	else
	{
		// the parcel has a name, so include it in the landmark name
		switch (fmt)
		{
		case LOCATION_FORMAT_LANDMARK:
			buffer = llformat("%.100s", parcel_name.c_str());
			break;
		case LOCATION_FORMAT_NORMAL:
			buffer = llformat("%s, %s", parcel_name.c_str(), region_name.c_str());
			break;
		case LOCATION_FORMAT_NO_MATURITY:
			buffer = llformat("%s, %s (%d, %d, %d)",
				parcel_name.c_str(),
				region_name.c_str(),
				pos_x, pos_y, pos_z);
			break;
		case LOCATION_FORMAT_NO_COORDS:
			buffer = llformat("%s, %s%s%s",
							  parcel_name.c_str(),
							  region_name.c_str(),
							  sim_access_string.empty() ? "" : " - ",
							  sim_access_string.c_str());
				break;
		case LOCATION_FORMAT_FULL:
			buffer = llformat("%s, %s (%d, %d, %d)%s%s",
				parcel_name.c_str(),
				region_name.c_str(),
				pos_x, pos_y, pos_z,
				sim_access_string.empty() ? "" : " - ",
				sim_access_string.c_str());
			break;
		}
	}
	str = buffer;
	return TRUE;
}
BOOL LLAgentUI::buildLocationString(std::string& str, ELocationFormat fmt)
{
	return buildLocationString(str,fmt, gAgent.getPositionAgent());
}
