/**
 * @file   llagentlistener.h
 * @author Brad Kittenbrink
 * @date   2009-07-09
 * @brief  Event API for subset of LLViewerControl methods
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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


#ifndef LL_LLAGENTLISTENER_H
#define LL_LLAGENTLISTENER_H

#include "lleventapi.h"

class LLAgent;
class LLSD;
class LLViewerObject;
class LLVector3d;

class LLAgentListener : public LLEventAPI
{
public:
    LLAgentListener(LLAgent &agent);

private:
    void requestTeleport(LLSD const & event_data) const;
    void requestSit(LLSD const & event_data) const;
    void requestStand(LLSD const & event_data) const;
    void requestTouch(LLSD const & event_data) const;
    void resetAxes(const LLSD& event_data) const;
    void getAxes(const LLSD& event_data) const;
    void getGroups(const LLSD& event) const;
    void getPosition(const LLSD& event_data) const;
    void startAutoPilot(const LLSD& event_data);
    void getAutoPilot(const LLSD& event_data) const;
    void startFollowPilot(const LLSD& event_data);
    void setAutoPilotTarget(const LLSD& event_data) const;
    void stopAutoPilot(const LLSD& event_data) const;
    void lookAt(LLSD const & event_data) const;

    LLViewerObject * findObjectClosestTo( const LLVector3 & position ) const;

private:
    LLAgent &   mAgent;
    LLUUID      mFollowTarget;
};

#endif // LL_LLAGENTLISTENER_H

