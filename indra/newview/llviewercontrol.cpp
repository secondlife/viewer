/** 
 * @file llviewercontrol.cpp
 * @brief Viewer configuration
 * @author Richard Nelson
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "llviewercontrol.h"

// Library includes
#include "llwindow.h"	// getGamma()

// For Listeners
#include "llaudioengine.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llconsole.h"
#include "lldrawpoolterrain.h"
#include "llflexibleobject.h"
#include "llfeaturemanager.h"
#include "llviewershadermgr.h"

#include "llsky.h"
#include "llvieweraudio.h"
#include "llviewermenu.h"
#include "llviewertexturelist.h"
#include "llviewerthrottle.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llvoiceclient.h"
#include "llvosky.h"
#include "llvotree.h"
#include "llvovolume.h"
#include "llworld.h"
#include "pipeline.h"
#include "llviewerjoystick.h"
#include "llviewerparcelmgr.h"
#include "llparcel.h"
#include "llkeyboard.h"
#include "llerrorcontrol.h"
#include "llappviewer.h"
#include "llvosurfacepatch.h"
#include "llvowlsky.h"
#include "llrender.h"
#include "llbottomtray.h"
#include "llnavigationbar.h"
#include "llfloatertools.h"
#include "llpaneloutfitsinventory.h"
#include "llpanellogin.h"

#ifdef TOGGLE_HACKED_GODLIKE_VIEWER
BOOL 				gHackGodmode = FALSE;
#endif


LLControlGroup gSavedSettings("Global");	// saved at end of session
LLControlGroup gSavedPerAccountSettings("PerAccount"); // saved at end of session
LLControlGroup gCrashSettings("CrashSettings");	// saved at end of session
LLControlGroup gWarningSettings("Warnings"); // persists ignored dialogs/warnings

std::string gLastRunVersion;
std::string gCurrentVersion;

extern BOOL gResizeScreenTexture;
extern BOOL gDebugGL;
extern BOOL gAuditTexture;
////////////////////////////////////////////////////////////////////////////
// Listeners

static bool handleRenderAvatarMouselookChanged(const LLSD& newvalue)
{
	LLVOAvatar::sVisibleInFirstPerson = newvalue.asBoolean();
	return true;
}

static bool handleRenderFarClipChanged(const LLSD& newvalue)
{
	F32 draw_distance = (F32) newvalue.asReal();
	gAgentCamera.mDrawDistance = draw_distance;
	LLWorld::getInstance()->setLandFarClip(draw_distance);
	return true;
}

static bool handleTerrainDetailChanged(const LLSD& newvalue)
{
	LLDrawPoolTerrain::sDetailMode = newvalue.asInteger();
	return true;
}


static bool handleSetShaderChanged(const LLSD& newvalue)
{
	LLViewerShaderMgr::instance()->setShaders();
	return true;
}

static bool handleReleaseGLBufferChanged(const LLSD& newvalue)
{
	if (gPipeline.isInit())
	{
		gPipeline.releaseGLBuffers();
		gPipeline.createGLBuffers();
	}
	return true;
}

static bool handleVolumeLODChanged(const LLSD& newvalue)
{
	LLVOVolume::sLODFactor = (F32) newvalue.asReal();
	LLVOVolume::sDistanceFactor = 1.f-LLVOVolume::sLODFactor * 0.1f;
	return true;
}

static bool handleAvatarLODChanged(const LLSD& newvalue)
{
	LLVOAvatar::sLODFactor = (F32) newvalue.asReal();
	return true;
}

static bool handleAvatarMaxVisibleChanged(const LLSD& newvalue)
{
	LLVOAvatar::sMaxVisible = (U32) newvalue.asInteger();
	return true;
}

static bool handleTerrainLODChanged(const LLSD& newvalue)
{
		LLVOSurfacePatch::sLODFactor = (F32)newvalue.asReal();
		//sqaure lod factor to get exponential range of [0,4] and keep
		//a value of 1 in the middle of the detail slider for consistency
		//with other detail sliders (see panel_preferences_graphics1.xml)
		LLVOSurfacePatch::sLODFactor *= LLVOSurfacePatch::sLODFactor;
		return true;
}

static bool handleTreeLODChanged(const LLSD& newvalue)
{
	LLVOTree::sTreeFactor = (F32) newvalue.asReal();
	return true;
}

static bool handleFlexLODChanged(const LLSD& newvalue)
{
	LLVolumeImplFlexible::sUpdateFactor = (F32) newvalue.asReal();
	return true;
}

static bool handleGammaChanged(const LLSD& newvalue)
{
	F32 gamma = (F32) newvalue.asReal();
	if (gamma == 0.0f)
	{
		gamma = 1.0f; // restore normal gamma
	}
	if (gViewerWindow && gViewerWindow->getWindow() && gamma != gViewerWindow->getWindow()->getGamma())
	{
		// Only save it if it's changed
		if (!gViewerWindow->getWindow()->setGamma(gamma))
		{
			llwarns << "setGamma failed!" << llendl;
		}
	}

	return true;
}

const F32 MAX_USER_FOG_RATIO = 10.f;
const F32 MIN_USER_FOG_RATIO = 0.5f;

static bool handleFogRatioChanged(const LLSD& newvalue)
{
	F32 fog_ratio = llmax(MIN_USER_FOG_RATIO, llmin((F32) newvalue.asReal(), MAX_USER_FOG_RATIO));
	gSky.setFogRatio(fog_ratio);
	return true;
}

static bool handleMaxPartCountChanged(const LLSD& newvalue)
{
	LLViewerPartSim::setMaxPartCount(newvalue.asInteger());
	return true;
}

static bool handleVideoMemoryChanged(const LLSD& newvalue)
{
	gTextureList.updateMaxResidentTexMem(newvalue.asInteger());
	return true;
}

static bool handleBandwidthChanged(const LLSD& newvalue)
{
	gViewerThrottle.setMaxBandwidth((F32) newvalue.asReal());
	return true;
}

static bool handleChatFontSizeChanged(const LLSD& newvalue)
{
	if(gConsole)
	{
		gConsole->setFontSize(newvalue.asInteger());
	}
	return true;
}

static bool handleChatPersistTimeChanged(const LLSD& newvalue)
{
	if(gConsole)
	{
		gConsole->setLinePersistTime((F32) newvalue.asReal());
	}
	return true;
}

static bool handleConsoleMaxLinesChanged(const LLSD& newvalue)
{
	if(gConsole)
	{
		gConsole->setMaxLines(newvalue.asInteger());
	}
	return true;
}

static void handleAudioVolumeChanged(const LLSD& newvalue)
{
	audio_update_volume(true);
}

static bool handleJoystickChanged(const LLSD& newvalue)
{
	LLViewerJoystick::getInstance()->setCameraNeedsUpdate(TRUE);
	return true;
}

static bool handleUseOcclusionChanged(const LLSD& newvalue)
{
	LLPipeline::sUseOcclusion = (newvalue.asBoolean() && gGLManager.mHasOcclusionQuery 
		&& LLFeatureManager::getInstance()->isFeatureAvailable("UseOcclusion") && !gUseWireframe) ? 2 : 0;
	return true;
}

static bool handleUploadBakedTexOldChanged(const LLSD& newvalue)
{
	LLPipeline::sForceOldBakedUpload = newvalue.asBoolean();
	return true;
}


static bool handleNumpadControlChanged(const LLSD& newvalue)
{
	if (gKeyboard)
	{
		gKeyboard->setNumpadDistinct(static_cast<LLKeyboard::e_numpad_distinct>(newvalue.asInteger()));
	}
	return true;
}

static bool handleRenderUseVBOChanged(const LLSD& newvalue)
{
	if (gPipeline.isInit())
	{
		gPipeline.setUseVBO(newvalue.asBoolean());
	}
	return true;
}

static bool handleWLSkyDetailChanged(const LLSD&)
{
	if (gSky.mVOWLSkyp.notNull())
	{
		gSky.mVOWLSkyp->updateGeometry(gSky.mVOWLSkyp->mDrawable);
	}
	return true;
}

static bool handleResetVertexBuffersChanged(const LLSD&)
{
	if (gPipeline.isInit())
	{
		gPipeline.resetVertexBuffers();
	}
	return true;
}

static bool handleRenderDynamicLODChanged(const LLSD& newvalue)
{
	LLPipeline::sDynamicLOD = newvalue.asBoolean();
	return true;
}

static bool handleRenderUseFBOChanged(const LLSD& newvalue)
{
	LLRenderTarget::sUseFBO = newvalue.asBoolean();
	if (gPipeline.isInit())
	{
		gPipeline.releaseGLBuffers();
		gPipeline.createGLBuffers();
		if (LLPipeline::sRenderDeferred && LLRenderTarget::sUseFBO)
		{
			LLViewerShaderMgr::instance()->setShaders();
		}
	}
	return true;
}

static bool handleRenderUseImpostorsChanged(const LLSD& newvalue)
{
	LLVOAvatar::sUseImpostors = newvalue.asBoolean();
	return true;
}

static bool handleAuditTextureChanged(const LLSD& newvalue)
{
	gAuditTexture = newvalue.asBoolean();
	return true;
}

static bool handleRenderDebugGLChanged(const LLSD& newvalue)
{
	gDebugGL = newvalue.asBoolean() || gDebugSession;
	gGL.clearErrors();
	return true;
}

static bool handleRenderDebugPipelineChanged(const LLSD& newvalue)
{
	gDebugPipeline = newvalue.asBoolean();
	return true;
}

static bool handleRenderResolutionDivisorChanged(const LLSD&)
{
	gResizeScreenTexture = TRUE;
	return true;
}

static bool handleDebugViewsChanged(const LLSD& newvalue)
{
	LLView::sDebugRects = newvalue.asBoolean();
	return true;
}

static bool handleLogFileChanged(const LLSD& newvalue)
{
	std::string log_filename = newvalue.asString();
	LLFile::remove(log_filename);
	LLError::logToFile(log_filename);
	return true;
}

bool handleHideGroupTitleChanged(const LLSD& newvalue)
{
	gAgent.setHideGroupTitle(newvalue);
	return true;
}

bool handleEffectColorChanged(const LLSD& newvalue)
{
	gAgent.setEffectColor(LLColor4(newvalue));
	return true;
}

bool handleVectorizeChanged(const LLSD& newvalue)
{
	LLViewerJointMesh::updateVectorize();
	return true;
}

bool handleHighResSnapshotChanged(const LLSD& newvalue)
{
	// High Res Snapshot active, must uncheck RenderUIInSnapshot
	if (newvalue.asBoolean())
	{
		gSavedSettings.setBOOL( "RenderUIInSnapshot", FALSE );
	}
	return true;
}

bool handleVoiceClientPrefsChanged(const LLSD& newvalue)
{
	LLVoiceClient::getInstance()->updateSettings();
	return true;
}

bool handleVelocityInterpolate(const LLSD& newvalue)
{
	LLMessageSystem* msg = gMessageSystem;
	if ( newvalue.asBoolean() )
	{
		msg->newMessageFast(_PREHASH_VelocityInterpolateOn);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();
		llinfos << "Velocity Interpolation On" << llendl;
	}
	else
	{
		msg->newMessageFast(_PREHASH_VelocityInterpolateOff);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();
		llinfos << "Velocity Interpolation Off" << llendl;
	}
	return true;
}

bool handleForceShowGrid(const LLSD& newvalue)
{
	LLPanelLogin::refreshLocation( false );
	return true;
}

bool toggle_agent_pause(const LLSD& newvalue)
{
	if ( newvalue.asBoolean() )
	{
		send_agent_pause();
	}
	else
	{
		send_agent_resume();
	}
	return true;
}

bool toggle_show_gesture_button(const LLSD& newvalue)
{
	LLBottomTray::getInstance()->showGestureButton(newvalue.asBoolean());
	return true;
}

bool toggle_show_move_button(const LLSD& newvalue)
{
	LLBottomTray::getInstance()->showMoveButton(newvalue.asBoolean());
	return true;
}

bool toggle_show_camera_button(const LLSD& newvalue)
{
	LLBottomTray::getInstance()->showCameraButton(newvalue.asBoolean());
	return true;
}

bool toggle_show_snapshot_button(const LLSD& newvalue)
{
	LLBottomTray::getInstance()->showSnapshotButton(newvalue.asBoolean());
	return true;
}

bool toggle_show_navigation_panel(const LLSD& newvalue)
{
	LLNavigationBar::getInstance()->showNavigationPanel(newvalue.asBoolean());
	return true;
}

bool toggle_show_favorites_panel(const LLSD& newvalue)
{
	LLNavigationBar::getInstance()->showFavoritesPanel(newvalue.asBoolean());
	return true;
}

bool toggle_show_appearance_editor(const LLSD& newvalue)
{
	LLPanelOutfitsInventory::sShowDebugEditor = newvalue.asBoolean();
	return true;
}

bool toggle_show_object_render_cost(const LLSD& newvalue)
{
	LLFloaterTools::sShowObjectCost = newvalue.asBoolean();
	return true;
}

////////////////////////////////////////////////////////////////////////////

void settings_setup_listeners()
{
	gSavedSettings.getControl("FirstPersonAvatarVisible")->getSignal()->connect(boost::bind(&handleRenderAvatarMouselookChanged, _2));
	gSavedSettings.getControl("RenderFarClip")->getSignal()->connect(boost::bind(&handleRenderFarClipChanged, _2));
	gSavedSettings.getControl("RenderTerrainDetail")->getSignal()->connect(boost::bind(&handleTerrainDetailChanged, _2));
	gSavedSettings.getControl("RenderUseTriStrips")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderAnimateTrees")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderAvatarVP")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("VertexShaderEnable")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderUIBuffer")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderFSAASamples")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderShadowResolutionScale")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderGlow")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderGlow")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("EnableRippleWater")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderGlowResolutionPow")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderAvatarCloth")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("WindLightUseAtmosShaders")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderGammaFull")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderAvatarMaxVisible")->getSignal()->connect(boost::bind(&handleAvatarMaxVisibleChanged, _2));
	gSavedSettings.getControl("RenderVolumeLODFactor")->getSignal()->connect(boost::bind(&handleVolumeLODChanged, _2));
	gSavedSettings.getControl("RenderAvatarLODFactor")->getSignal()->connect(boost::bind(&handleAvatarLODChanged, _2));
	gSavedSettings.getControl("RenderTerrainLODFactor")->getSignal()->connect(boost::bind(&handleTerrainLODChanged, _2));
	gSavedSettings.getControl("RenderTreeLODFactor")->getSignal()->connect(boost::bind(&handleTreeLODChanged, _2));
	gSavedSettings.getControl("RenderFlexTimeFactor")->getSignal()->connect(boost::bind(&handleFlexLODChanged, _2));
	gSavedSettings.getControl("ThrottleBandwidthKBPS")->getSignal()->connect(boost::bind(&handleBandwidthChanged, _2));
	gSavedSettings.getControl("RenderGamma")->getSignal()->connect(boost::bind(&handleGammaChanged, _2));
	gSavedSettings.getControl("RenderFogRatio")->getSignal()->connect(boost::bind(&handleFogRatioChanged, _2));
	gSavedSettings.getControl("RenderMaxPartCount")->getSignal()->connect(boost::bind(&handleMaxPartCountChanged, _2));
	gSavedSettings.getControl("RenderDynamicLOD")->getSignal()->connect(boost::bind(&handleRenderDynamicLODChanged, _2));
	gSavedSettings.getControl("RenderDebugTextureBind")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderAutoMaskAlphaDeferred")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderAutoMaskAlphaNonDeferred")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderObjectBump")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderMaxVBOSize")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderUseFBO")->getSignal()->connect(boost::bind(&handleRenderUseFBOChanged, _2));
	gSavedSettings.getControl("RenderDeferredNoise")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderUseImpostors")->getSignal()->connect(boost::bind(&handleRenderUseImpostorsChanged, _2));
	gSavedSettings.getControl("RenderDebugGL")->getSignal()->connect(boost::bind(&handleRenderDebugGLChanged, _2));
	gSavedSettings.getControl("RenderDebugPipeline")->getSignal()->connect(boost::bind(&handleRenderDebugPipelineChanged, _2));
	gSavedSettings.getControl("RenderResolutionDivisor")->getSignal()->connect(boost::bind(&handleRenderResolutionDivisorChanged, _2));
	gSavedSettings.getControl("RenderDeferred")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderShadowDetail")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderDeferredSSAO")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderDeferredGI")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("TextureMemory")->getSignal()->connect(boost::bind(&handleVideoMemoryChanged, _2));
	gSavedSettings.getControl("AuditTexture")->getSignal()->connect(boost::bind(&handleAuditTextureChanged, _2));
	gSavedSettings.getControl("ChatFontSize")->getSignal()->connect(boost::bind(&handleChatFontSizeChanged, _2));
	gSavedSettings.getControl("ChatPersistTime")->getSignal()->connect(boost::bind(&handleChatPersistTimeChanged, _2));
	gSavedSettings.getControl("ConsoleMaxLines")->getSignal()->connect(boost::bind(&handleConsoleMaxLinesChanged, _2));
	gSavedSettings.getControl("UploadBakedTexOld")->getSignal()->connect(boost::bind(&handleUploadBakedTexOldChanged, _2));
	gSavedSettings.getControl("UseOcclusion")->getSignal()->connect(boost::bind(&handleUseOcclusionChanged, _2));
	gSavedSettings.getControl("AudioLevelMaster")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelSFX")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelUI")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelAmbient")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelMusic")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelMedia")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelVoice")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelDoppler")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelRolloff")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteAudio")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteMusic")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteMedia")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteVoice")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteAmbient")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteUI")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("RenderVBOEnable")->getSignal()->connect(boost::bind(&handleRenderUseVBOChanged, _2));
	gSavedSettings.getControl("RenderUseStreamVBO")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("WLSkyDetail")->getSignal()->connect(boost::bind(&handleWLSkyDetailChanged, _2));
	gSavedSettings.getControl("NumpadControl")->getSignal()->connect(boost::bind(&handleNumpadControlChanged, _2));
	gSavedSettings.getControl("JoystickAxis0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis6")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale6")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone6")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("DebugViews")->getSignal()->connect(boost::bind(&handleDebugViewsChanged, _2));
	gSavedSettings.getControl("UserLogFile")->getSignal()->connect(boost::bind(&handleLogFileChanged, _2));
	gSavedSettings.getControl("RenderHideGroupTitle")->getSignal()->connect(boost::bind(handleHideGroupTitleChanged, _2));
	gSavedSettings.getControl("HighResSnapshot")->getSignal()->connect(boost::bind(handleHighResSnapshotChanged, _2));
	gSavedSettings.getControl("VectorizePerfTest")->getSignal()->connect(boost::bind(&handleVectorizeChanged, _2));
	gSavedSettings.getControl("VectorizeEnable")->getSignal()->connect(boost::bind(&handleVectorizeChanged, _2));
	gSavedSettings.getControl("VectorizeProcessor")->getSignal()->connect(boost::bind(&handleVectorizeChanged, _2));
	gSavedSettings.getControl("VectorizeSkin")->getSignal()->connect(boost::bind(&handleVectorizeChanged, _2));
	gSavedSettings.getControl("EnableVoiceChat")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("PTTCurrentlyEnabled")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("PushToTalkButton")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("PushToTalkToggle")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("VoiceEarLocation")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("VoiceInputAudioDevice")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("VoiceOutputAudioDevice")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("AudioLevelMic")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("LipSyncEnabled")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));	
	gSavedSettings.getControl("VelocityInterpolate")->getSignal()->connect(boost::bind(&handleVelocityInterpolate, _2));
	gSavedSettings.getControl("QAMode")->getSignal()->connect(boost::bind(&show_debug_menus));
	gSavedSettings.getControl("UseDebugMenus")->getSignal()->connect(boost::bind(&show_debug_menus));
	gSavedSettings.getControl("AgentPause")->getSignal()->connect(boost::bind(&toggle_agent_pause, _2));
	gSavedSettings.getControl("ShowGestureButton")->getSignal()->connect(boost::bind(&toggle_show_gesture_button, _2));
	gSavedSettings.getControl("ShowMoveButton")->getSignal()->connect(boost::bind(&toggle_show_move_button, _2));
	gSavedSettings.getControl("ShowCameraButton")->getSignal()->connect(boost::bind(&toggle_show_camera_button, _2));
	gSavedSettings.getControl("ShowSnapshotButton")->getSignal()->connect(boost::bind(&toggle_show_snapshot_button, _2));
	gSavedSettings.getControl("ShowNavbarNavigationPanel")->getSignal()->connect(boost::bind(&toggle_show_navigation_panel, _2));
	gSavedSettings.getControl("ShowNavbarFavoritesPanel")->getSignal()->connect(boost::bind(&toggle_show_favorites_panel, _2));
	gSavedSettings.getControl("ShowDebugAppearanceEditor")->getSignal()->connect(boost::bind(&toggle_show_appearance_editor, _2));
	gSavedSettings.getControl("ShowObjectRenderingCost")->getSignal()->connect(boost::bind(&toggle_show_object_render_cost, _2));
	gSavedSettings.getControl("ForceShowGrid")->getSignal()->connect(boost::bind(&handleForceShowGrid, _2));
}

#if TEST_CACHED_CONTROL

#define DECL_LLCC(T, V) static LLCachedControl<T> mySetting_##T("TestCachedControl"#T, V)
DECL_LLCC(U32, (U32)666);
DECL_LLCC(S32, (S32)-666);
DECL_LLCC(F32, (F32)-666.666);
DECL_LLCC(bool, true);
DECL_LLCC(BOOL, FALSE);
static LLCachedControl<std::string> mySetting_string("TestCachedControlstring", "Default String Value");
DECL_LLCC(LLVector3, LLVector3(1.0f, 2.0f, 3.0f));
DECL_LLCC(LLVector3d, LLVector3d(6.0f, 5.0f, 4.0f));
DECL_LLCC(LLRect, LLRect(0, 0, 100, 500));
DECL_LLCC(LLColor4, LLColor4(0.0f, 0.5f, 1.0f));
DECL_LLCC(LLColor3, LLColor3(1.0f, 0.f, 0.5f));
DECL_LLCC(LLColor4U, LLColor4U(255, 200, 100, 255));

LLSD test_llsd = LLSD()["testing1"] = LLSD()["testing2"];
DECL_LLCC(LLSD, test_llsd);

static LLCachedControl<std::string> test_BrowserHomePage("BrowserHomePage", "hahahahahha", "Not the real comment");

void test_cached_control()
{
#define do { TEST_LLCC(T, V) if((T)mySetting_##T != V) llerrs << "Fail "#T << llendl; } while(0)
	TEST_LLCC(U32, 666);
	TEST_LLCC(S32, (S32)-666);
	TEST_LLCC(F32, (F32)-666.666);
	TEST_LLCC(bool, true);
	TEST_LLCC(BOOL, FALSE);
	if((std::string)mySetting_string != "Default String Value") llerrs << "Fail string" << llendl;
	TEST_LLCC(LLVector3, LLVector3(1.0f, 2.0f, 3.0f));
	TEST_LLCC(LLVector3d, LLVector3d(6.0f, 5.0f, 4.0f));
	TEST_LLCC(LLRect, LLRect(0, 0, 100, 500));
	TEST_LLCC(LLColor4, LLColor4(0.0f, 0.5f, 1.0f));
	TEST_LLCC(LLColor3, LLColor3(1.0f, 0.f, 0.5f));
	TEST_LLCC(LLColor4U, LLColor4U(255, 200, 100, 255));
//There's no LLSD comparsion for LLCC yet. TEST_LLCC(LLSD, test_llsd); 

	if((std::string)test_BrowserHomePage != "http://www.secondlife.com") llerrs << "Fail BrowserHomePage" << llendl;
}
#endif // TEST_CACHED_CONTROL

