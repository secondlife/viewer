/** 
 * @file llliveappconfig.cpp
 * @brief Configuration information for an LLApp that overrides indra.xml
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "llliveappconfig.h"

#include "llapp.h"
#include "llsd.h"
#include "llsdserialize.h"

LLLiveAppConfig::LLLiveAppConfig(
	const std::string& filename,
	F32 refresh_period,
	LLApp::OptionPriority priority) :
	LLLiveFile(filename, refresh_period),
	mPriority(priority)
{ }


LLLiveAppConfig::~LLLiveAppConfig()
{ }

// virtual 
bool LLLiveAppConfig::loadFile()
{
	llinfos << "LLLiveAppConfig::loadFile(): reading from "
		<< filename() << llendl;
    llifstream file(filename());
	LLSD config;
    if (file.is_open())
    {
        LLSDSerialize::fromXML(config, file);
		if(!config.isMap())
		{
			llwarns << "Live app config not an map in " << filename()
				<< " Ignoring the data." << llendl;
			return false;
		}
		file.close();
    }
	else
	{
		llinfos << "Live file " << filename() << " does not exit." << llendl;
	}
	// *NOTE: we do not handle the else case here because we would not
	// have attempted to load the file unless LLLiveFile had
	// determined there was a reason to load it. This only happens
	// when either the file has been updated or it is either suddenly
	// in existence or has passed out of existence. Therefore, we want
	// to set the config to an empty config, and return that it
	// changed.

	LLApp* app = LLApp::instance();
	if(app) app->setOptionData(mPriority, config);
	return true;
}
