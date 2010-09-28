/**
 * @file llsky_stub.cpp
 * @brief  stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

class LLSky
{
public:
	void setOverrideSun(BOOL override);
	void setSunDirection(const LLVector3 &sun_direction, const LLVector3 &sun_ang_velocity);
};

void LLSky::setOverrideSun(BOOL override) {}
void LLSky::setSunDirection(const LLVector3 &sun_direction, const LLVector3 &sun_ang_velocity) {}

LLSky gSky;
