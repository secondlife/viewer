/** 
 * @file llpipeline_stub.cpp
 * @brief  stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

class LLPipeline
{
public: BOOL canUseWindLightShaders() const;
};
BOOL LLPipeline::canUseWindLightShaders() const {return TRUE;}
LLPipeline gPipeline;
