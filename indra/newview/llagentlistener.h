/**
 * @file   llagentlistener.h
 * @author Brad Kittenbrink
 * @date   2009-07-09
 * @brief  Event API for subset of LLViewerControl methods
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */


#ifndef LL_LLAGENTLISTENER_H
#define LL_LLAGENTLISTENER_H

#include "lleventapi.h"

class LLAgent;
class LLSD;

class LLAgentListener : public LLEventAPI
{
public:
	LLAgentListener(LLAgent &agent);

private:
	void requestTeleport(LLSD const & event_data) const;
	void requestSit(LLSD const & event_data) const;
	void requestStand(LLSD const & event_data) const;

private:
	LLAgent & mAgent;
};

#endif // LL_LLAGENTLISTENER_H

