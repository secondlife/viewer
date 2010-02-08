/** 
 * @file llagentui.h
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

#ifndef LLAGENTUI_H
#define LLAGENTUI_H

class LLAgentUI
{
public:
	enum ELocationFormat
	{
		LOCATION_FORMAT_NORMAL,			// Parcel
		LOCATION_FORMAT_LANDMARK,		// Parcel, Region
		LOCATION_FORMAT_NO_MATURITY,	// Parcel, Region (x, y, z)
		LOCATION_FORMAT_NO_COORDS,		// Parcel, Region - Maturity
		LOCATION_FORMAT_FULL,			// Parcel, Region (x, y, z) - Maturity
	};

	static void buildFullname(std::string &name);

	static std::string buildSLURL(const bool escaped = true);
	//build location string using the current position of gAgent.
	static BOOL buildLocationString(std::string& str, ELocationFormat fmt = LOCATION_FORMAT_LANDMARK);
	//build location string using a region position of the avatar. 
	static BOOL buildLocationString(std::string& str, ELocationFormat fmt,const LLVector3& agent_pos_region);
	/**
	 * @brief Check whether  the agent is in neighborhood of the pole  Within same region
	 * @return true if the agent is in neighborhood.
	 */
	static BOOL checkAgentDistance(const LLVector3& local_pole, F32 radius);
};

#endif //LLAGENTUI_H
