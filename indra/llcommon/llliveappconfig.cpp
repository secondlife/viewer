/** 
 * @file llliveappconfig.cpp
 * @brief Configuration information for an LLApp that overrides indra.xml
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
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
    llifstream file(filename().c_str());
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
