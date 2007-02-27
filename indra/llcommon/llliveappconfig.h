/** 
 * @file llliveappconfig.h
 * @brief Configuration information for an LLApp that overrides indra.xml
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LLLIVEAPPCONFIG_H
#define LLLIVEAPPCONFIG_H

#include "lllivefile.h"

class LLApp;

class LLLiveAppConfig : public LLLiveFile
{
public:
	// To use this, instantiate a LLLiveAppConfig object inside your main loop.
	// The traditional name for it is live_config.
	// Be sure to call live_config.checkAndReload() periodically.

	LLLiveAppConfig(LLApp* app, const std::string& filename, F32 refresh_period);
	~LLLiveAppConfig();

protected:
	/*virtual*/ void loadFile();

private:
	LLApp* mApp;
};

#endif
