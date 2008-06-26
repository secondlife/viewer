/** 
 * @file llliveappconfig.cpp
 * @brief Configuration information for an LLApp that overrides indra.xml
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

LLLiveAppConfig::LLLiveAppConfig(LLApp* app, const std::string& filename, F32 refresh_period)
:	LLLiveFile(filename, refresh_period),
	mApp(app)
{ }


LLLiveAppConfig::~LLLiveAppConfig()
{ }

// virtual 
void LLLiveAppConfig::loadFile()
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
			llinfos << "LLDataserverConfig::loadFile(): not an map!"
				<< " Ignoring the data." << llendl;
			return;
		}
		file.close();
    }
	mApp->setOptionData(
		LLApp::PRIORITY_SPECIFIC_CONFIGURATION, config);
}
