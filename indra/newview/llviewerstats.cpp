/** 
 * @file llviewerstats.cpp
 * @brief LLViewerStats class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llviewerstats.h"
#include "llviewerthrottle.h"

#include "message.h"
#include "llfloaterreg.h"
#include "llmemory.h"
#include "lltimer.h"
#include "llvfile.h"

#include "llappviewer.h"

#include "pipeline.h" 
#include "lltexturefetch.h" 
#include "llviewerobjectlist.h" 
#include "llviewertexturelist.h" 
#include "lltexlayer.h"
#include "lltexlayerparams.h"
#include "llsurface.h"
#include "llvlmanager.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llviewercontrol.h"
#include "llversioninfo.h"
#include "llfloatertools.h"
#include "lldebugview.h"
#include "llfasttimerview.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llviewerwindow.h"		// *TODO: remove, only used for width/height
#include "llworld.h"
#include "llfeaturemanager.h"
#include "llviewernetwork.h"
#include "llmeshrepository.h" //for LLMeshRepository::sBytesReceived

namespace LLStatViewer
{

LLTrace::Count<>	FPS("fpsstat"),
					PACKETS_IN("packetsinstat"),
					PACKETS_LOST("packetsloststat"),
					PACKETS_OUT("packetsoutstat"),
					TEXTURE_PACKETS("texturepacketsstat"),
					TRIANGLES_DRAWN("trianglesdrawnstat"),
					CHAT_COUNT("chatcount", "Chat messages sent"),
					IM_COUNT("imcount", "IMs sent"),
					OBJECT_CREATE("objectcreate"),
					OBJECT_REZ("objectrez", "Object rez count"),
					LOADING_WEARABLES_LONG_DELAY("loadingwearableslongdelay", "Wearables took too long to load"),
					LOGIN_TIMEOUTS("logintimeouts", "Number of login attempts that timed out"),
					LSL_SAVES("lslsaves", "Number of times user has saved a script"),
					ANIMATION_UPLOADS("animationuploads", "Animations uploaded"),
					FLY("fly", "Fly count"),
					TELEPORT("teleport", "Teleport count"),
					DELETE_OBJECT("deleteobject", "Objects deleted"),
					SNAPSHOT("snapshot", "Snapshots taken"),
					UPLOAD_SOUND("uploadsound", "Sounds uploaded"),
					UPLOAD_TEXTURE("uploadtexture", "Textures uploaded"),
					EDIT_TEXTURE("edittexture", "Changes to textures on objects"),
					KILLED("killed", "Number of times killed"),
					FRAMETIME_DOUBLED("frametimedoubled", "Ratio of frames 2x longer than previous"),
					TEX_BAKES("texbakes"),
					TEX_REBAKES("texrebakes");
LLTrace::Count<LLTrace::Kilobits>	KBIT("kbitstat"),
									LAYERS_KBIT("layerskbitstat"),
									OBJECT_KBIT("objectkbitstat"),
									ASSET_KBIT("assetkbitstat"),
									TEXTURE_KBIT("texturekbitstat"),
									ACTUAL_IN_KBIT("actualinkbit"),
									ACTUAL_OUT_KBIT("actualoutkbit");

LLTrace::Count<LLTrace::Seconds> AVATAR_EDIT_TIME("avataredittime", "Seconds in Edit Appearence"),
								TOOLBOX_TIME("toolboxtime", "Seconds using Toolbox"),
								MOUSELOOK_TIME("mouselooktime", "Seconds in Mouselook"),
								FPS_10_TIME("fps10time", "Seconds below 10 FPS"),
								FPS_8_TIME("fps8time", "Seconds below 8 FPS"),
								FPS_2_TIME("fps2time", "Seconds below 2 FPS"),
								SIM_20_FPS_TIME("sim20fpstime", "Seconds with sim FPS below 20"),
								SIM_PHYSICS_20_FPS_TIME("simphysics20fpstime", "Seconds with physics FPS below 20"),
								LOSS_5_PERCENT_TIME("loss5percenttime", "Seconds with packet loss > 5%");

SimMeasurement<>			SIM_TIME_DILATION("simtimedilation", "", LL_SIM_STAT_TIME_DILATION),
							SIM_FPS("simfps", "", LL_SIM_STAT_FPS),
							SIM_PHYSICS_FPS("simphysicsfps", "", LL_SIM_STAT_PHYSFPS),
							SIM_AGENT_UPS("simagentups", "", LL_SIM_STAT_AGENTUPS),
							SIM_SCRIPT_EPS("simscripteps", "", LL_SIM_STAT_SCRIPT_EPS),
							SIM_SKIPPED_SILHOUETTE("simsimskippedsilhouettesteps", "", LL_SIM_STAT_SKIPPEDAISILSTEPS_PS),
							SIM_SKIPPED_CHARACTERS_PERCENTAGE("simsimpctsteppedcharacters", "", LL_SIM_STAT_PCTSTEPPEDCHARACTERS),
							SIM_MAIN_AGENTS("simmainagents", "", LL_SIM_STAT_NUMAGENTMAIN),
							SIM_CHILD_AGENTS("simchildagents", "", LL_SIM_STAT_NUMAGENTCHILD),
							SIM_OBJECTS("simobjects", "", LL_SIM_STAT_NUMTASKS),
							SIM_ACTIVE_OBJECTS("simactiveobjects", "", LL_SIM_STAT_NUMTASKSACTIVE),
							SIM_ACTIVE_SCRIPTS("simactivescripts", "", LL_SIM_STAT_NUMSCRIPTSACTIVE),
							SIM_PERCENTAGE_SCRIPTS_RUN("simpctscriptsrun", "", LL_SIM_STAT_PCTSCRIPTSRUN),
							SIM_IN_PACKETS_PER_SEC("siminpps", "", LL_SIM_STAT_INPPS),
							SIM_OUT_PACKETS_PER_SEC("simoutpps", "", LL_SIM_STAT_OUTPPS),
							SIM_PENDING_DOWNLOADS("simpendingdownloads", "", LL_SIM_STAT_PENDING_DOWNLOADS),
							SIM_PENDING_UPLOADS("simpendinguploads", "", LL_SIM_STAT_PENDING_UPLOADS),
							SIM_PENDING_LOCAL_UPLOADS("simpendinglocaluploads", "", LL_SIM_STAT_PENDING_LOCAL_UPLOADS),
							SIM_PHYSICS_PINNED_TASKS("physicspinnedtasks", "", LL_SIM_STAT_PHYSICS_PINNED_TASKS),
							SIM_PHYSICS_LOD_TASKS("physicslodtasks", "", LL_SIM_STAT_PHYSICS_LOD_TASKS);

LLTrace::Measurement<>		NUM_IMAGES("numimagesstat"),
							NUM_RAW_IMAGES("numrawimagesstat"),
							NUM_OBJECTS("numobjectsstat"),
							NUM_ACTIVE_OBJECTS("numactiveobjectsstat"),
							NUM_NEW_OBJECTS("numnewobjectsstat"),
							NUM_SIZE_CULLED("numsizeculledstat"),
							NUM_VIS_CULLED("numvisculledstat"),
							ENABLE_VBO("enablevbo", "Vertex Buffers Enabled"),
							LIGHTING_DETAIL("lightingdetail", "Lighting Detail"),
							VISIBLE_AVATARS("visibleavatars", "Visible Avatars"),
							SHADER_OBJECTS("shaderobjects", "Object Shaders"),
							DRAW_DISTANCE("drawdistance", "Draw Distance"),
							CHAT_BUBBLES("chatbubbles", "Chat Bubbles Enabled"),
							PENDING_VFS_OPERATIONS("vfspendingoperations"), 
							PACKETS_LOST_PERCENT("packetslostpercentstat"),
							WINDOW_WIDTH("windowwidth", "Window width"),
							WINDOW_HEIGHT("windowheight", "Window height");

LLTrace::Measurement<LLTrace::Meters> AGENT_POSITION_SNAP("agentpositionsnap", "agent position corrections");


LLTrace::Measurement<LLTrace::Bytes>	SIM_UNACKED_BYTES("simtotalunackedbytes"),
										SIM_PHYSICS_MEM("physicsmemoryallocated"),
										GL_TEX_MEM("gltexmemstat"),
										GL_BOUND_MEM("glboundmemstat"),
										RAW_MEM("rawmemstat"),
										FORMATTED_MEM("formattedmemstat"),
										DELTA_BANDWIDTH("deltabandwidth", "Increase/Decrease in bandwidth based on packet loss"),
										MAX_BANDWIDTH("maxbandwidth", "Max bandwidth setting");


SimMeasurement<LLTrace::Milliseconds> SIM_FRAME_TIME("simframemsec", "", LL_SIM_STAT_FRAMEMS),
										SIM_NET_TIME("simnetmsec", "", LL_SIM_STAT_NETMS),
										SIM_OTHER_TIME("simsimothermsec", "", LL_SIM_STAT_SIMOTHERMS),
										SIM_PHYSICS_TIME("simsimphysicsmsec", "", LL_SIM_STAT_SIMPHYSICSMS),
										SIM_PHYSICS_STEP_TIME("simsimphysicsstepmsec", "", LL_SIM_STAT_SIMPHYSICSSTEPMS),
										SIM_PHYSICS_SHAPE_UPDATE_TIME("simsimphysicsshapeupdatemsec", "", LL_SIM_STAT_SIMPHYSICSSHAPEMS),
										SIM_PHYSICS_OTHER_TIME("simsimphysicsothermsec", "", LL_SIM_STAT_SIMPHYSICSOTHERMS),
										SIM_AI_TIME("simsimaistepmsec", "", LL_SIM_STAT_SIMAISTEPTIMEMS),
										SIM_AGENTS_TIME("simagentmsec", "", LL_SIM_STAT_AGENTMS),
										SIM_IMAGES_TIME("simimagesmsec", "", LL_SIM_STAT_IMAGESMS),
										SIM_SCRIPTS_TIME("simscriptmsec", "", LL_SIM_STAT_SCRIPTMS),
										SIM_SPARE_TIME("simsparemsec", "", LL_SIM_STAT_SIMSPARETIME),
										SIM_SLEEP_TIME("simsleepmsec", "", LL_SIM_STAT_SIMSLEEPTIME),
										SIM_PUMP_IO_TIME("simpumpiomsec", "", LL_SIM_STAT_IOPUMPTIME);

LLTrace::Measurement<LLTrace::Milliseconds>	FRAMETIME_JITTER("frametimejitter", "Average delta between successive frame times"),
											FRAMETIME_SLEW("frametimeslew", "Average delta between frame time and mean"),
											LOGIN_SECONDS("loginseconds", "Time between LoginRequest and LoginReply"),
											REGION_CROSSING_TIME("regioncrossingtime", "CROSSING_AVG"),
											FRAME_STACKTIME("framestacktime", "FRAME_SECS"),
											UPDATE_STACKTIME("updatestacktime", "UPDATE_SECS"),
											NETWORK_STACKTIME("networkstacktime", "NETWORK_SECS"),
											IMAGE_STACKTIME("imagestacktime", "IMAGE_SECS"),
											REBUILD_STACKTIME("rebuildstacktime", "REBUILD_SECS"),
											RENDER_STACKTIME("renderstacktime", "RENDER_SECS"),
											SIM_PING("simpingstat");

}

LLViewerStats::LLViewerStats() 
:	mLastTimeDiff(0.0)
{
	mRecording.start();
	LLTrace::get_frame_recording().start();
}

LLViewerStats::~LLViewerStats()
{
}

void LLViewerStats::resetStats()
{
	LLViewerStats::instance().mRecording.reset();
}

void LLViewerStats::updateFrameStats(const F64 time_diff)
{
	if (getRecording().getLastValue(LLStatViewer::PACKETS_LOST_PERCENT) > 5.0)
	{
		LLStatViewer::LOSS_5_PERCENT_TIME.add(time_diff);
		//incStat(ST_LOSS_05_SECONDS, time_diff);
	}
	
	F32 sim_fps = getRecording().getLastValue(LLStatViewer::SIM_FPS);
	if (0.f < sim_fps && sim_fps < 20.f)
	{
		LLStatViewer::SIM_20_FPS_TIME.add(time_diff);
		//incStat(ST_SIM_FPS_20_SECONDS, time_diff);
	}
	
	F32 sim_physics_fps = getRecording().getLastValue(LLStatViewer::SIM_PHYSICS_FPS);

	if (0.f < sim_physics_fps && sim_physics_fps < 20.f)
	{
		LLStatViewer::SIM_PHYSICS_20_FPS_TIME.add(time_diff);
		//incStat(ST_PHYS_FPS_20_SECONDS, time_diff);
	}
		
	if (time_diff >= 0.5)
	{
		LLStatViewer::FPS_2_TIME.add(time_diff);
		//incStat(ST_FPS_2_SECONDS, time_diff);
	}
	if (time_diff >= 0.125)
	{
		LLStatViewer::FPS_8_TIME.add(time_diff);
		//incStat(ST_FPS_8_SECONDS, time_diff);
	}
	if (time_diff >= 0.1)
	{
		LLStatViewer::FPS_10_TIME.add(time_diff);
		//incStat(ST_FPS_10_SECONDS, time_diff);
	}

	if (gFrameCount && mLastTimeDiff > 0.0)
	{
		// new "stutter" meter
		LLStatViewer::FRAMETIME_DOUBLED.add(time_diff >= 2.0 * mLastTimeDiff ? 1 : 0);
		//setStat(ST_FPS_DROP_50_RATIO,
		//		(getStat(ST_FPS_DROP_50_RATIO) * (F64)(gFrameCount - 1) + 
		//		 (time_diff >= 2.0 * mLastTimeDiff ? 1.0 : 0.0)) / gFrameCount);
			

		// old stats that were never really used
		LLStatViewer::FRAMETIME_JITTER.sample(mLastTimeDiff - time_diff);
		//setStat(ST_FRAMETIME_JITTER,
		//		(getStat(ST_FRAMETIME_JITTER) * (gFrameCount - 1) + 
		//		 fabs(mLastTimeDiff - time_diff) / mLastTimeDiff) / gFrameCount);
			
		F32 average_frametime = gRenderStartTime.getElapsedTimeF32() / (F32)gFrameCount;
		LLStatViewer::FRAMETIME_SLEW.sample(average_frametime - time_diff);
		//setStat(ST_FRAMETIME_SLEW,
		//		(getStat(ST_FRAMETIME_SLEW) * (gFrameCount - 1) + 
		//		 fabs(average_frametime - time_diff) / average_frametime) / gFrameCount);

		F32 max_bandwidth = gViewerThrottle.getMaxBandwidth();
		F32 delta_bandwidth = gViewerThrottle.getCurrentBandwidth() - max_bandwidth;
		LLStatViewer::DELTA_BANDWIDTH.sample<LLTrace::Bits>(delta_bandwidth);
		//setStat(ST_DELTA_BANDWIDTH, delta_bandwidth / 1024.f);

		LLStatViewer::MAX_BANDWIDTH.sample<LLTrace::Bits>(max_bandwidth);
		//setStat(ST_MAX_BANDWIDTH, max_bandwidth / 1024.f);
		
	}
	
	mLastTimeDiff = time_diff;
}

void LLViewerStats::addToMessage(LLSD &body) const
{
	LLSD &misc = body["misc"];
	
	misc["Version"] = TRUE;
	//TODO RN: get last value, not mean
	misc["Vertex Buffers Enabled"] = getRecording().getMean(LLStatViewer::ENABLE_VBO);
	
	body["AgentPositionSnaps"] = getRecording().getSum(LLStatViewer::AGENT_POSITION_SNAP).value(); //mAgentPositionSnaps.asLLSD();
	llinfos << "STAT: AgentPositionSnaps: Mean = " << getRecording().getMean(LLStatViewer::AGENT_POSITION_SNAP).value() << "; StdDev = " << getRecording().getStandardDeviation(LLStatViewer::AGENT_POSITION_SNAP).value() 
			<< "; Count = " << getRecording().getSampleCount(LLStatViewer::AGENT_POSITION_SNAP) << llendl;
}

// *NOTE:Mani The following methods used to exist in viewer.cpp
// Moving them here, but not merging them into LLViewerStats yet.
U32		gTotalLandIn = 0, gTotalLandOut = 0;
U32		gTotalWaterIn = 0, gTotalWaterOut = 0;

F32		gAveLandCompression = 0.f, gAveWaterCompression = 0.f;
F32		gBestLandCompression = 1.f, gBestWaterCompression = 1.f;
F32		gWorstLandCompression = 0.f, gWorstWaterCompression = 0.f;

U32		gTotalWorldBytes = 0, gTotalObjectBytes = 0, gTotalTextureBytes = 0, gSimPingCount = 0;
U32		gObjectBits = 0;
F32		gAvgSimPing = 0.f;
U32     gTotalTextureBytesPerBoostLevel[LLViewerTexture::MAX_GL_IMAGE_CATEGORY] = {0};

extern U32  gVisCompared;
extern U32  gVisTested;

LLFrameTimer gTextureTimer;

void update_statistics()
{
	gTotalWorldBytes += gVLManager.getTotalBytes();
	gTotalObjectBytes += gObjectBits / 8;

	// make sure we have a valid time delta for this frame
	if (gFrameIntervalSeconds > 0.f)
	{
		if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK)
		{
			LLStatViewer::MOUSELOOK_TIME.add(gFrameIntervalSeconds);
			//LLViewerStats::getInstance()->incStat(LLViewerStats::ST_MOUSELOOK_SECONDS, gFrameIntervalSeconds);
		}
		else if (gAgentCamera.getCameraMode() == CAMERA_MODE_CUSTOMIZE_AVATAR)
		{
			LLStatViewer::AVATAR_EDIT_TIME.add(gFrameIntervalSeconds);
			//LLViewerStats::getInstance()->incStat(LLViewerStats::ST_AVATAR_EDIT_SECONDS, gFrameIntervalSeconds);
		}
		else if (LLFloaterReg::instanceVisible("build"))
		{
			LLStatViewer::TOOLBOX_TIME.add(gFrameIntervalSeconds);
			//LLViewerStats::getInstance()->incStat(LLViewerStats::ST_TOOLBOX_SECONDS, gFrameIntervalSeconds);
		}
	}
	LLStatViewer::ENABLE_VBO.sample((F64)gSavedSettings.getBOOL("RenderVBOEnable"));
	//stats.setStat(LLViewerStats::ST_ENABLE_VBO, (F64)gSavedSettings.getBOOL("RenderVBOEnable"));
	LLStatViewer::LIGHTING_DETAIL.sample((F64)gPipeline.getLightingDetail());
	//stats.setStat(LLViewerStats::ST_LIGHTING_DETAIL, (F64)gPipeline.getLightingDetail());
	LLStatViewer::DRAW_DISTANCE.sample((F64)gSavedSettings.getF32("RenderFarClip"));
	//stats.setStat(LLViewerStats::ST_DRAW_DIST, (F64)gSavedSettings.getF32("RenderFarClip"));
	LLStatViewer::CHAT_BUBBLES.sample((F64)gSavedSettings.getBOOL("UseChatBubbles"));
	//stats.setStat(LLViewerStats::ST_CHAT_BUBBLES, (F64)gSavedSettings.getBOOL("UseChatBubbles"));

	LLStatViewer::FRAME_STACKTIME.sample(gDebugView->mFastTimerView->getTime("Frame"));
	//stats.setStat(LLViewerStats::ST_FRAME_SECS, gDebugView->mFastTimerView->getTime("Frame"));
	F64 idle_secs = gDebugView->mFastTimerView->getTime("Idle");
	F64 network_secs = gDebugView->mFastTimerView->getTime("Network");
	LLStatViewer::UPDATE_STACKTIME.sample(idle_secs - network_secs);
	//stats.setStat(LLViewerStats::ST_UPDATE_SECS, idle_secs - network_secs);
	LLStatViewer::NETWORK_STACKTIME.sample(network_secs);
	//stats.setStat(LLViewerStats::ST_NETWORK_SECS, network_secs);
	LLStatViewer::IMAGE_STACKTIME.sample(gDebugView->mFastTimerView->getTime("Update Images"));
	//stats.setStat(LLViewerStats::ST_IMAGE_SECS, gDebugView->mFastTimerView->getTime("Update Images"));
	LLStatViewer::REBUILD_STACKTIME.sample(gDebugView->mFastTimerView->getTime("Sort Draw State"));
	//stats.setStat(LLViewerStats::ST_REBUILD_SECS, gDebugView->mFastTimerView->getTime("Sort Draw State"));
	LLStatViewer::RENDER_STACKTIME.sample(gDebugView->mFastTimerView->getTime("Geometry"));
	//stats.setStat(LLViewerStats::ST_RENDER_SECS, gDebugView->mFastTimerView->getTime("Geometry"));
		
	LLCircuitData *cdp = gMessageSystem->mCircuitInfo.findCircuit(gAgent.getRegion()->getHost());
	if (cdp)
	{
		LLStatViewer::SIM_PING.sample<LLTrace::Seconds>(cdp->getPingDelay());
		//stats.mSimPingStat.addValue(cdp->getPingDelay());
		gAvgSimPing = ((gAvgSimPing * (F32)gSimPingCount) + (F32)(cdp->getPingDelay())) / ((F32)gSimPingCount + 1);
		gSimPingCount++;
	}
	else
	{
		LLStatViewer::SIM_PING.sample<LLTrace::Seconds>(10000);
		//stats.mSimPingStat.addValue(10000);
	}

	//stats.mFPSStat.addValue(1);
	LLStatViewer::FPS.add(1);
	F32 layer_bits = (F32)(gVLManager.getLandBits() + gVLManager.getWindBits() + gVLManager.getCloudBits());
	LLStatViewer::LAYERS_KBIT.add<LLTrace::Bits>(layer_bits);
	//stats.mLayersKBitStat.addValue(layer_bits/1024.f);
	LLStatViewer::OBJECT_KBIT.add<LLTrace::Bits>(gObjectBits);
	//stats.mObjectKBitStat.addValue(gObjectBits/1024.f);
	//stats.mVFSPendingOperations.addValue(LLVFile::getVFSThread()->getPending());
	LLStatViewer::PENDING_VFS_OPERATIONS.sample(LLVFile::getVFSThread()->getPending());
	LLStatViewer::ASSET_KBIT.add<LLTrace::Bits>(gTransferManager.getTransferBitsIn(LLTCT_ASSET));
	//stats.mAssetKBitStat.addValue(gTransferManager.getTransferBitsIn(LLTCT_ASSET)/1024.f);
	gTransferManager.resetTransferBitsIn(LLTCT_ASSET);

	if (LLAppViewer::getTextureFetch()->getNumRequests() == 0)
	{
		gTextureTimer.pause();
	}
	else
	{
		gTextureTimer.unpause();
	}
	
	{
		static F32 visible_avatar_frames = 0.f;
		static F32 avg_visible_avatars = 0;
		F32 visible_avatars = (F32)LLVOAvatar::sNumVisibleAvatars;
		if (visible_avatars > 0.f)
		{
			visible_avatar_frames = 1.f;
			avg_visible_avatars = (avg_visible_avatars * (F32)(visible_avatar_frames - 1.f) + visible_avatars) / visible_avatar_frames;
		}
		LLStatViewer::VISIBLE_AVATARS.sample((F64)avg_visible_avatars);
		//stats.setStat(LLViewerStats::ST_VISIBLE_AVATARS, (F64)avg_visible_avatars);
	}
	LLWorld::getInstance()->updateNetStats();
	LLWorld::getInstance()->requestCacheMisses();
	
	// Reset all of these values.
	gVLManager.resetBitCounts();
	gObjectBits = 0;
//	gDecodedBits = 0;

	// Only update texture stats periodically so that they are less noisy
	{
		static const F32 texture_stats_freq = 10.f;
		static LLFrameTimer texture_stats_timer;
		if (texture_stats_timer.getElapsedTimeF32() >= texture_stats_freq)
		{
			gTotalTextureBytes = LLTrace::Bytes(LLViewerStats::instance().getRecording().getSum(LLStatViewer::TEXTURE_KBIT)).value();
			texture_stats_timer.reset();
		}
	}

	LLTrace::get_frame_recording().nextPeriod();
}

class ViewerStatsResponder : public LLHTTPClient::Responder
{
public:
    ViewerStatsResponder() { }

    void error(U32 statusNum, const std::string& reason)
    {
		llinfos << "ViewerStatsResponder::error " << statusNum << " "
				<< reason << llendl;
    }

    void result(const LLSD& content)
    {
		llinfos << "ViewerStatsResponder::result" << llendl;
	}
};

/*
 * The sim-side LLSD is in newsim/llagentinfo.cpp:forwardViewerStats.
 *
 * There's also a compatibility shim for the old fixed-format sim
 * stats in newsim/llagentinfo.cpp:processViewerStats.
 *
 * If you move stats around here, make the corresponding changes in
 * those locations, too.
 */
void send_stats()
{
	// IW 9/23/02 I elected not to move this into LLViewerStats
	// because it depends on too many viewer.cpp globals.
	// Someday we may want to merge all our stats into a central place
	// but that day is not today.

	// Only send stats if the agent is connected to a region.
	if (!gAgent.getRegion())
	{
		return;
	}

	LLSD body;
	std::string url = gAgent.getRegion()->getCapability("ViewerStats");

	if (url.empty()) {
		llwarns << "Could not get ViewerStats capability" << llendl;
		return;
	}
	
	body["session_id"] = gAgentSessionID;
	
	LLSD &agent = body["agent"];
	
	time_t ltime;
	time(&ltime);
	F32 run_time = F32(LLFrameTimer::getElapsedSeconds());

	agent["start_time"] = S32(ltime - S32(run_time));

	// The first stat set must have a 0 run time if it doesn't actually
	// contain useful data in terms of FPS, etc.  We use half the
	// SEND_STATS_PERIOD seconds as the point at which these statistics become
	// valid.  Data warehouse uses a 0 value here to easily discard these
	// records with non-useful FPS values etc.
	if (run_time < (SEND_STATS_PERIOD / 2))
	{
		agent["run_time"] = 0.0f;
	}
	else
	{
		agent["run_time"] = run_time;
	}

	// send fps only for time app spends in foreground
	agent["fps"] = (F32)gForegroundFrameCount / gForegroundTime.getElapsedTimeF32();
	agent["version"] = LLVersionInfo::getChannelAndVersion();
	std::string language = LLUI::getLanguage();
	agent["language"] = language;
	
	agent["sim_fps"] = ((F32) gFrameCount - gSimFrames) /
		(F32) (gRenderStartTime.getElapsedTimeF32() - gSimLastTime);

	gSimLastTime = gRenderStartTime.getElapsedTimeF32();
	gSimFrames   = (F32) gFrameCount;

	agent["agents_in_view"] = LLVOAvatar::sNumVisibleAvatars;
	agent["ping"] = gAvgSimPing;
	agent["meters_traveled"] = gAgent.getDistanceTraveled();
	agent["regions_visited"] = gAgent.getRegionsVisited();
	agent["mem_use"] = LLMemory::getCurrentRSS() / 1024.0;

	LLSD &system = body["system"];
	
	system["ram"] = (S32) gSysMemory.getPhysicalMemoryKB();
	system["os"] = LLAppViewer::instance()->getOSInfo().getOSStringSimple();
	system["cpu"] = gSysCPU.getCPUString();
	unsigned char MACAddress[MAC_ADDRESS_BYTES];
	LLUUID::getNodeID(MACAddress);
	std::string macAddressString = llformat("%02x-%02x-%02x-%02x-%02x-%02x",
											MACAddress[0],MACAddress[1],MACAddress[2],
											MACAddress[3],MACAddress[4],MACAddress[5]);
	system["mac_address"] = macAddressString;
	system["serial_number"] = LLAppViewer::instance()->getSerialNumber();
	std::string gpu_desc = llformat(
		"%-6s Class %d ",
		gGLManager.mGLVendorShort.substr(0,6).c_str(),
		(S32)LLFeatureManager::getInstance()->getGPUClass())
		+ LLFeatureManager::getInstance()->getGPUString();

	system["gpu"] = gpu_desc;
	system["gpu_class"] = (S32)LLFeatureManager::getInstance()->getGPUClass();
	system["gpu_vendor"] = gGLManager.mGLVendorShort;
	system["gpu_version"] = gGLManager.mDriverVersionVendorString;
	system["opengl_version"] = gGLManager.mGLVersionString;

	S32 shader_level = 0;
	if (LLPipeline::sRenderDeferred)
	{
		shader_level = 3;
	}
	else if (gPipeline.canUseWindLightShadersOnObjects())
	{
		shader_level = 2;
	}
	else if (gPipeline.canUseVertexShaders())
	{
		shader_level = 1;
	}


	system["shader_level"] = shader_level;

	LLSD &download = body["downloads"];

	download["world_kbytes"] = gTotalWorldBytes / 1024.0;
	download["object_kbytes"] = gTotalObjectBytes / 1024.0;
	download["texture_kbytes"] = gTotalTextureBytes / 1024.0;
	download["mesh_kbytes"] = LLMeshRepository::sBytesReceived/1024.0;

	LLSD &in = body["stats"]["net"]["in"];

	in["kbytes"] = gMessageSystem->mTotalBytesIn / 1024.0;
	in["packets"] = (S32) gMessageSystem->mPacketsIn;
	in["compressed_packets"] = (S32) gMessageSystem->mCompressedPacketsIn;
	in["savings"] = (gMessageSystem->mUncompressedBytesIn -
					 gMessageSystem->mCompressedBytesIn) / 1024.0;
	
	LLSD &out = body["stats"]["net"]["out"];
	
	out["kbytes"] = gMessageSystem->mTotalBytesOut / 1024.0;
	out["packets"] = (S32) gMessageSystem->mPacketsOut;
	out["compressed_packets"] = (S32) gMessageSystem->mCompressedPacketsOut;
	out["savings"] = (gMessageSystem->mUncompressedBytesOut -
					  gMessageSystem->mCompressedBytesOut) / 1024.0;

	LLSD &fail = body["stats"]["failures"];

	fail["send_packet"] = (S32) gMessageSystem->mSendPacketFailureCount;
	fail["dropped"] = (S32) gMessageSystem->mDroppedPackets;
	fail["resent"] = (S32) gMessageSystem->mResentPackets;
	fail["failed_resends"] = (S32) gMessageSystem->mFailedResendPackets;
	fail["off_circuit"] = (S32) gMessageSystem->mOffCircuitPackets;
	fail["invalid"] = (S32) gMessageSystem->mInvalidOnCircuitPackets;

	// Misc stats, two strings and two ints
	// These are not expecticed to persist across multiple releases
	// Comment any changes with your name and the expected release revision
	// If the current revision is recent, ping the previous author before overriding
	LLSD &misc = body["stats"]["misc"];

	// Screen size so the UI team can figure out how big the widgets
	// appear and use a "typical" size for end user tests.

	S32 window_width = gViewerWindow->getWindowWidthRaw();
	S32 window_height = gViewerWindow->getWindowHeightRaw();
	S32 window_size = (window_width * window_height) / 1024;
	misc["string_1"] = llformat("%d", window_size);
	misc["string_2"] = llformat("Texture Time: %.2f, Total Time: %.2f", gTextureTimer.getElapsedTimeF32(), gFrameTimeSeconds);

// 	misc["int_1"] = LLSD::Integer(gSavedSettings.getU32("RenderQualityPerformance")); // Steve: 1.21
// 	misc["int_2"] = LLSD::Integer(gFrameStalls); // Steve: 1.21

	F32 unbaked_time = LLVOAvatar::sUnbakedTime * 1000.f / gFrameTimeSeconds;
	misc["int_1"] = LLSD::Integer(unbaked_time); // Steve: 1.22
	F32 grey_time = LLVOAvatar::sGreyTime * 1000.f / gFrameTimeSeconds;
	misc["int_2"] = LLSD::Integer(grey_time); // Steve: 1.22

	llinfos << "Misc Stats: int_1: " << misc["int_1"] << " int_2: " << misc["int_2"] << llendl;
	llinfos << "Misc Stats: string_1: " << misc["string_1"] << " string_2: " << misc["string_2"] << llendl;

	body["DisplayNamesEnabled"] = gSavedSettings.getBOOL("UseDisplayNames");
	body["DisplayNamesShowUsername"] = gSavedSettings.getBOOL("NameTagShowUsernames");
	
	body["MinimalSkin"] = false;
	
	LLViewerStats::getInstance()->addToMessage(body);
	LLHTTPClient::post(url, body, new ViewerStatsResponder());
}

LLFrameTimer& LLViewerStats::PhaseMap::getPhaseTimer(const std::string& phase_name)
{
	phase_map_t::iterator iter = mPhaseMap.find(phase_name);
	if (iter == mPhaseMap.end())
	{
		LLFrameTimer timer;
		mPhaseMap[phase_name] = timer;
	}
	LLFrameTimer& timer = mPhaseMap[phase_name];
	return timer;
}

void LLViewerStats::PhaseMap::startPhase(const std::string& phase_name)
{
	LLFrameTimer& timer = getPhaseTimer(phase_name);
	lldebugs << "startPhase " << phase_name << llendl;
	timer.unpause();
}

void LLViewerStats::PhaseMap::stopPhase(const std::string& phase_name)
{
	phase_map_t::iterator iter = mPhaseMap.find(phase_name);
	if (iter != mPhaseMap.end())
	{
		if (iter->second.getStarted())
		{
			// Going from started to paused state - record stats.
			recordPhaseStat(phase_name,iter->second.getElapsedTimeF32());
		}
		lldebugs << "stopPhase " << phase_name << llendl;
		iter->second.pause();
	}
	else
	{
		lldebugs << "stopPhase " << phase_name << " is not started, no-op" << llendl;
	}
}

void LLViewerStats::PhaseMap::stopAllPhases()
{
	for (phase_map_t::iterator iter = mPhaseMap.begin();
		 iter != mPhaseMap.end(); ++iter)
	{
		const std::string& phase_name = iter->first;
		if (iter->second.getStarted())
		{
			// Going from started to paused state - record stats.
			recordPhaseStat(phase_name,iter->second.getElapsedTimeF32());
		}
		lldebugs << "stopPhase (all) " << phase_name << llendl;
		iter->second.pause();
	}
}

void LLViewerStats::PhaseMap::clearPhases()
{
	lldebugs << "clearPhases" << llendl;

	mPhaseMap.clear();
}

LLSD LLViewerStats::PhaseMap::asLLSD()
{
	LLSD result;
	for (phase_map_t::iterator iter = mPhaseMap.begin(); iter != mPhaseMap.end(); ++iter)
	{
		const std::string& phase_name = iter->first;
		result[phase_name]["completed"] = LLSD::Integer(!(iter->second.getStarted()));
		result[phase_name]["elapsed"] = iter->second.getElapsedTimeF32();
	}
	return result;
}

// static initializer
//static
LLViewerStats::phase_stats_t LLViewerStats::PhaseMap::sStats;

LLViewerStats::PhaseMap::PhaseMap()
{
}

// static
LLViewerStats::StatsAccumulator& LLViewerStats::PhaseMap::getPhaseStats(const std::string& phase_name)
{
	phase_stats_t::iterator it = sStats.find(phase_name);
	if (it == sStats.end())
	{
		LLViewerStats::StatsAccumulator new_stats;
		sStats[phase_name] = new_stats;
	}
	return sStats[phase_name];
}

// static
void LLViewerStats::PhaseMap::recordPhaseStat(const std::string& phase_name, F32 value)
{
	LLViewerStats::StatsAccumulator& stats = getPhaseStats(phase_name);
	stats.push(value);
}

