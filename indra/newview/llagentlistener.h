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
#include "throttle.h"

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
    void getGroups(const LLSD& event) const;
    void getPosition(const LLSD& event_data) const;
    void startAutoPilot(const LLSD& event_data);
    void getAutoPilot(const LLSD& event_data) const;
    void startFollowPilot(const LLSD& event_data);
    void setAutoPilotTarget(const LLSD& event_data) const;
    void stopAutoPilot(const LLSD& event_data) const;
    void lookAt(LLSD const & event_data) const;

    void setFollowCamParams(LLSD const & event_data) const;
    void setFollowCamActive(LLSD const & event_data) const;
    void removeFollowCamParams(LLSD const & event_data) const;

    void playAnimation(LLSD const &event_data);
    void playAnimation_(const LLUUID& asset_id, const bool inworld);
    void stopAnimation(LLSD const &event_data);
    void getAnimationInfo(LLSD const &event_data);

    void getID(LLSD const& event_data);
    void getNearbyAvatarsList(LLSD const& event_data);
    void getNearbyObjectsList(LLSD const& event_data);
    void getAgentScreenPos(LLSD const& event_data);

    LLViewerObject * findObjectClosestTo( const LLVector3 & position, bool sit_target = false ) const;

private:
    LLAgent &   mAgent;
    LLUUID      mFollowTarget;

    LogThrottle<LLError::LEVEL_DEBUG, void(const LLUUID &, const bool)> mPlayAnimThrottle;
};

#endif // LL_LLAGENTLISTENER_H

