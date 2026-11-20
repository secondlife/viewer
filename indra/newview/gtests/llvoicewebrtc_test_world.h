/**
 * @file llvoicewebrtc_test_world.h
 * @brief The viewer state seen by llvoicewebrtc.cpp under test.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

// llvoicewebrtc.cpp is built into the tests unchanged.  Everything it uses
// from the rest of the viewer -- the agent and its region, LLWorld, region
// capabilities, the parcel, the mute list, stored speaker volumes, the
// avatar name cache and the HTTP coroutine adapter used for the provisioning
// requests -- comes from gtests/llvoicewebrtc_stubs.cpp, which reads and
// records through the TestWorld below.  Tests set the world up and then
// inspect what the voice client did to it.

#ifndef LLVOICEWEBRTC_TEST_WORLD_H
#define LLVOICEWEBRTC_TEST_WORLD_H

#include "llsd.h"
#include "lluuid.h"
#include "v3dmath.h"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

class LLParcel;
class LLViewerRegion;

namespace llvoicewebrtc_test
{

// A POST made through LLCoreHttpUtil::HttpCoroutineAdapter.  The coroutine
// that made it stays suspended until the post is completed.
struct HttpPost
{
    std::string mUrl;
    LLSD        mBody;
    bool        mCompleted{ false };
    LLSD        mResult;
};
using HttpPostPtr = std::shared_ptr<HttpPost>;

// Results in the form HttpCoroutineAdapter returns them.
LLSD httpSuccess(const LLSD& body = LLSD::emptyMap());
LLSD httpFailure(int http_status);

struct FakeRegion
{
    LLUUID                             mID;
    LLVector3d                         mOrigin; // south-west corner, global
    bool                               mCapabilitiesReceived{ true };
    std::map<std::string, std::string> mCapabilities;
    LLSD                               mFeatures;
    LLViewerRegion*                    mRegion{ nullptr };
};

class TestWorld
{
public:
    static TestWorld& get();

    // Back to an empty world: no regions, agent nowhere, nothing recorded.
    void reset();

    //------------------------------------------------------------------
    // Regions.  Each region is 256m square; addRegion() gives it the
    // voice capabilities and a 'webrtc' VoiceServerType unless told not to.
    LLViewerRegion* addRegion(const LLUUID& id, const LLVector3d& origin = LLVector3d(), bool webrtc = true);
    FakeRegion*     findRegion(const LLUUID& id);
    FakeRegion*     findRegion(const LLViewerRegion* region);
    LLViewerRegion* regionAt(const LLVector3d& pos_global);

    void setAgentRegion(const LLUUID& id); // a null id leaves the agent nowhere
    LLViewerRegion* agentRegion() const { return mAgentRegion; }

    static std::string provisionUrl(const LLUUID& region_id);
    static std::string signalingUrl(const LLUUID& region_id);

    //------------------------------------------------------------------
    // The parcel the agent is on, or null.
    LLParcel* mAgentParcel{ nullptr };

    //------------------------------------------------------------------
    // HTTP.  Every post is recorded.  If mHttpHandler is set and returns a
    // result, the post completes with it; otherwise the post stays pending
    // until completePost().  The posting coroutine always suspends at least
    // once, as it would against a real simulator.
    using HttpHandler = std::function<std::optional<LLSD>(const std::string& url, const LLSD& body)>;
    HttpHandler              mHttpHandler;
    std::vector<HttpPostPtr> mPosts;

    std::vector<HttpPostPtr> postsTo(const std::string& url) const;
    std::vector<HttpPostPtr> pendingPosts() const;
    static void              completePost(const HttpPostPtr& post, const LLSD& result);

    //------------------------------------------------------------------
    // Viewer settings and singletons seen by the voice client.
    bool        mVoiceEnabled{ true };   // LLVoiceClient::voiceEnabled()
    bool        mUserPTTState{ false };  // LLVoiceClient push-to-talk
    std::string mGridLoginID{ "aditi" }; // LLGridManager::getGridLoginID()

    std::set<LLUUID>         mVoiceMuted;     // LLMuteList, voice muted agents
    std::map<LLUUID, F32>    mSpeakerVolumes; // LLSpeakerVolumeStorage
    int                      mMuteListObservers{ 0 };

    int mVoiceChannelActivations{ 0 };   // LLVoiceChannel::activate()
    int mVoiceChannelDeactivations{ 0 }; // LLVoiceChannel::deactivate()

    // LLNearbyVoiceModeration::setMutedInfo() calls.
    std::vector<std::pair<std::string, bool>> mModerationMutedInfo;

    //------------------------------------------------------------------
    // LLAvatarNameCache: name requests stay outstanding until resolved.
    std::set<LLUUID> mNameRequests;
    void             resolveName(const LLUUID& id, const std::string& display_name);

    // Used by the stubs.
    struct NameCallbacks;
    std::map<LLUUID, std::shared_ptr<NameCallbacks>> mNameCallbacks;

private:
    TestWorld() = default;

    std::vector<std::unique_ptr<FakeRegion>> mRegions;
    LLViewerRegion*                          mAgentRegion{ nullptr };
};

} // namespace llvoicewebrtc_test

#endif // LLVOICEWEBRTC_TEST_WORLD_H
