/**
 * @file llviewerdisplay.cpp
 * @brief LLViewerDisplay class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "llviewerdisplay.h"

#include "fsyspath.h"
#include "hexdump.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h"
#include "llcoord.h"
#include "llcriticaldamp.h"
#include "llcubemap.h"
#include "lldir.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolwater.h"
#include "lldynamictexture.h"
#include "llenvironment.h"
#include "llfasttimer.h"
#include "llfeaturemanager.h"
#include "llfloatertools.h"
#include "llfocusmgr.h"
#include "llgl.h"
#include "llglheaders.h"
#include "llgltfmateriallist.h"
#include "llhudmanager.h"
#include "llimagepng.h"
#include "llmachineid.h"
#include "llmemory.h"
#include "llparcel.h"
#include "llperfstats.h"
#include "llrender.h"
#include "llscenemonitor.h"
#include "llsdjson.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llspatialpartition.h"
#include "llstartup.h"
#include "llstartup.h"
#include "lltooldraganddrop.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "lltracker.h"
#include "lltrans.h"
#include "llui.h"
#include "lluuid.h"
#include "llversioninfo.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewershadermgr.h"
#include "llviewerstats.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llvograss.h"
#include "llworld.h"
#include "pipeline.h"

#include <boost/json.hpp>

#include <filesystem>
#include <iomanip>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

extern LLPointer<LLViewerTexture> gStartTexture;
extern bool gShiftFrame;

LLPointer<LLViewerTexture> gDisconnectedImagep = nullptr;

// used to toggle renderer back on after teleport
bool         gTeleportDisplay = false;
LLFrameTimer gTeleportDisplayTimer;
LLFrameTimer gTeleportArrivalTimer;
constexpr F32 RESTORE_GL_TIME = 5.f;  // Wait this long while reloading textures before we raise the curtain

bool gForceRenderLandFence = false;
bool gDisplaySwapBuffers = false;
bool gDepthDirty = false;
bool gResizeScreenTexture = false;
bool gResizeShadowTexture = false;
bool gWindowResized = false;
bool gSnapshot = false;
bool gCubeSnapshot = false;
bool gSnapshotNoPost = false;
bool gShaderProfileFrame = false;

// This is how long the sim will try to teleport you before giving up.
constexpr F32 TELEPORT_EXPIRY = 15.0f;
// Additional time (in seconds) to wait per attachment
constexpr F32 TELEPORT_EXPIRY_PER_ATTACHMENT = 3.f;

U32 gRecentFrameCount = 0; // number of 'recent' frames
LLFrameTimer gRecentFPSTime;
LLFrameTimer gRecentMemoryTime;
LLFrameTimer gAssetStorageLogTime;

// Rendering stuff
void render_ui(F32 zoom_factor = 1.f, int subfield = 0);
void swap();
void render_hud_attachments();
void render_ui_3d();
void render_ui_2d();
void render_disconnected_background();

void getProfileStatsContext(boost::json::object& stats);
std::string getProfileStatsFilename();

void display_startup()
{
    if (   !gViewerWindow
        || !gViewerWindow->getActive()
        || !gViewerWindow->getWindow()->getVisible()
        || gViewerWindow->getWindow()->getMinimized()
        || gNonInteractive)
    {
        return;
    }

    gPipeline.updateGL();

    // Written as branch to appease GCC which doesn't like different
    // pointer types across ternary ops
    //
    if (!LLViewerFetchedTexture::sWhiteImagep.isNull())
    {
    LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();
    }

    LLGLSDefault gls_default;

    // Required for HTML update in login screen
    static S32 frame_count = 0;

    LLGLState::checkStates();

    if (frame_count++ > 1) // make sure we have rendered a frame first
    {
        LLViewerDynamicTexture::updateAllInstances();
    }
    else
    {
        LL_DEBUGS("Window") << "First display_startup frame" << LL_ENDL;
    }

    LLGLState::checkStates();

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT); // | GL_STENCIL_BUFFER_BIT);
    LLGLSUIDefault gls_ui;
    gPipeline.disableLights();

    if (gViewerWindow)
    gViewerWindow->setup2DRender();
    if (gViewerWindow)
    gViewerWindow->draw();
    gGL.flush();

    LLVertexBuffer::unbind();

    LLGLState::checkStates();

    if (gViewerWindow && gViewerWindow->getWindow())
    gViewerWindow->getWindow()->swapBuffers();

    glClear(GL_DEPTH_BUFFER_BIT);
}

void display_update_camera()
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Update Camera");
    // TODO: cut draw distance down if customizing avatar?
    // TODO: cut draw distance on per-parcel basis?

    // Cut draw distance in half when customizing avatar,
    // but on the viewer only.
    F32 final_far = gAgentCamera.mDrawDistance;
    if (gCubeSnapshot)
    {
        static LLCachedControl<F32> reflection_probe_draw_distance(gSavedSettings, "RenderReflectionProbeDrawDistance", 64.f);
        final_far = reflection_probe_draw_distance();
    }
    else if (CAMERA_MODE_CUSTOMIZE_AVATAR == gAgentCamera.getCameraMode())

    {
        final_far *= 0.5f;
    }
    LLViewerCamera::getInstance()->setFar(final_far);
    gViewerWindow->setup3DRender();

    if (!gCubeSnapshot)
    {
        // Update land visibility too
        LLWorld::getInstance()->setLandFarClip(final_far);
    }
}

// Write some stats to LL_INFOS()
void display_stats()
{
    LL_PROFILE_ZONE_SCOPED;
    constexpr F32 FPS_LOG_FREQUENCY = 10.f;
    if (gRecentFPSTime.getElapsedTimeF32() >= FPS_LOG_FREQUENCY)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("DS - FPS");
        F32 fps = gRecentFrameCount / FPS_LOG_FREQUENCY;
        LL_INFOS() << llformat("FPS: %.02f", fps) << LL_ENDL;
        gRecentFrameCount = 0;
        gRecentFPSTime.reset();
    }
    static LLCachedControl<F32> mem_log_freq(gSavedSettings, "MemoryLogFrequency", 600.f);
    if (mem_log_freq > 0.f && gRecentMemoryTime.getElapsedTimeF32() >= mem_log_freq)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("DS - Memory");
        gMemoryAllocated = U64Bytes(LLMemory::getCurrentRSS());
        U32Megabytes memory = gMemoryAllocated;
        LL_INFOS() << "MEMORY: " << memory << LL_ENDL;
        LLMemory::logMemoryInfo(true) ;
        gRecentMemoryTime.reset();
    }
    constexpr F32 ASSET_STORAGE_LOG_FREQUENCY = 60.f;
    if (gAssetStorageLogTime.getElapsedTimeF32() >= ASSET_STORAGE_LOG_FREQUENCY)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("DS - Asset Storage");
        gAssetStorageLogTime.reset();
        gAssetStorage->logAssetStorageInfo();
    }
}

static void update_tp_display(bool minimized)
{
    static LLCachedControl<F32> teleport_arrival_delay(gSavedSettings, "TeleportArrivalDelay");
    static LLCachedControl<F32> teleport_local_delay(gSavedSettings, "TeleportLocalDelay");

    S32 attach_count = 0;
    if (isAgentAvatarValid())
    {
        attach_count = gAgentAvatarp->getAttachmentCount();
    }
    F32 teleport_save_time = TELEPORT_EXPIRY + TELEPORT_EXPIRY_PER_ATTACHMENT * attach_count;
    F32 teleport_elapsed = gTeleportDisplayTimer.getElapsedTimeF32();
    F32 teleport_percent = teleport_elapsed * (100.f / teleport_save_time);
    if (gAgent.getTeleportState() != LLAgent::TELEPORT_START && teleport_percent > 100.f)
    {
        // Give up.  Don't keep the UI locked forever.
        LL_WARNS("Teleport") << "Giving up on teleport. elapsed time " << teleport_elapsed << " exceeds max time " << teleport_save_time << LL_ENDL;
        gAgent.setTeleportState(LLAgent::TELEPORT_NONE);
        gAgent.setTeleportMessage(std::string());
    }

    // Make sure the TP progress panel gets hidden in case the viewer window
    // is minimized *during* a TP. HB
    if (minimized)
    {
        gViewerWindow->setShowProgress(false);
    }

    const std::string& message = gAgent.getTeleportMessage();
    switch (gAgent.getTeleportState())
    {
        case LLAgent::TELEPORT_PENDING:
        {
            gTeleportDisplayTimer.reset();
            const std::string& msg = LLAgent::sTeleportProgressMessages["pending"];
            if (!minimized)
            {
                gViewerWindow->setShowProgress(true);
                gViewerWindow->setProgressPercent(llmin(teleport_percent, 0.0f));
                gViewerWindow->setProgressString(msg);
            }
            gAgent.setTeleportMessage(msg);
            break;
        }

        case LLAgent::TELEPORT_START:
        {
            // Transition to REQUESTED.  Viewer has sent some kind
            // of TeleportRequest to the source simulator
            gTeleportDisplayTimer.reset();
            const std::string& msg = LLAgent::sTeleportProgressMessages["requesting"];
            LL_INFOS("Teleport") << "A teleport request has been sent, setting state to TELEPORT_REQUESTED" << LL_ENDL;
            gAgent.setTeleportState(LLAgent::TELEPORT_REQUESTED);
            gAgent.setTeleportMessage(msg);
            if (!minimized)
            {
                gViewerWindow->setShowProgress(true);
                gViewerWindow->setProgressPercent(llmin(teleport_percent, 0.0f));
                gViewerWindow->setProgressString(msg);
                gViewerWindow->setProgressMessage(gAgent.mMOTD);
            }
            break;
        }

        case LLAgent::TELEPORT_REQUESTED:
            // Waiting for source simulator to respond
            if (!minimized)
            {
                gViewerWindow->setProgressPercent(llmin(teleport_percent, 37.5f));
                gViewerWindow->setProgressString(message);
            }
            break;

        case LLAgent::TELEPORT_MOVING:
            // Viewer has received destination location from source simulator
            if (!minimized)
            {
                gViewerWindow->setProgressPercent(llmin(teleport_percent, 75.f));
                gViewerWindow->setProgressString(message);
            }
            break;

        case LLAgent::TELEPORT_START_ARRIVAL:
            // Transition to ARRIVING.  Viewer has received avatar update, etc.,
            // from destination simulator
            gTeleportArrivalTimer.reset();
            LL_INFOS("Teleport") << "Changing state to TELEPORT_ARRIVING" << LL_ENDL;
            gAgent.setTeleportState(LLAgent::TELEPORT_ARRIVING);
            gAgent.setTeleportMessage(LLAgent::sTeleportProgressMessages["arriving"]);
            gAgent.sheduleTeleportIM();
            gTextureList.mForceResetTextureStats = true;
            gAgentCamera.resetView(true, true);
            if (!minimized)
            {
                gViewerWindow->setProgressCancelButtonVisible(false, LLTrans::getString("Cancel"));
                gViewerWindow->setProgressPercent(75.f);
            }
            break;

        case LLAgent::TELEPORT_ARRIVING:
        // Make the user wait while content "pre-caches"
        {
            F32 arrival_fraction = (gTeleportArrivalTimer.getElapsedTimeF32() / teleport_arrival_delay());
            if (arrival_fraction > 1.f)
            {
                arrival_fraction = 1.f;
                //LLFirstUse::useTeleport();
                LL_INFOS("Teleport") << "arrival_fraction is " << arrival_fraction << " changing state to TELEPORT_NONE" << LL_ENDL;
                gAgent.setTeleportState(LLAgent::TELEPORT_NONE);
            }
            if (!minimized)
            {
                gViewerWindow->setProgressCancelButtonVisible(false, LLTrans::getString("Cancel"));
                gViewerWindow->setProgressPercent(arrival_fraction * 25.f + 75.f);
                gViewerWindow->setProgressString(message);
            }
            break;
        }

        case LLAgent::TELEPORT_LOCAL:
        // Short delay when teleporting in the same sim (progress screen active but not shown - did not
        // fall-through from TELEPORT_START)
        {
            if (gTeleportDisplayTimer.getElapsedTimeF32() > teleport_local_delay())
            {
                //LLFirstUse::useTeleport();
                LL_INFOS("Teleport") << "State is local and gTeleportDisplayTimer " << gTeleportDisplayTimer.getElapsedTimeF32()
                                     << " exceeds teleport_local_delete " << teleport_local_delay
                                     << "; setting state to TELEPORT_NONE"
                                     << LL_ENDL;
                gAgent.setTeleportState(LLAgent::TELEPORT_NONE);
            }
            break;
        }

        case LLAgent::TELEPORT_NONE:
            // No teleport in progress
            gViewerWindow->setShowProgress(false);
            gTeleportDisplay = false;
    }
}

// Paint the display!
void display(bool rebuild, F32 zoom_factor, int subfield, bool for_snapshot)
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Render");
    LL_PROFILE_GPU_ZONE("Render");

    LLPerfStats::RecordSceneTime T (LLPerfStats::StatType_t::RENDER_DISPLAY); // render time capture - This is the main stat for overall rendering.

    if (gWindowResized)
    { //skip render on frames where window has been resized
        LL_DEBUGS("Window") << "Resizing window" << LL_ENDL;
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Resize Window");
        gGL.flush();
        glClear(GL_COLOR_BUFFER_BIT);
        gViewerWindow->getWindow()->swapBuffers();
        LLPipeline::refreshCachedSettings();
        gPipeline.resizeScreenTexture();
        gResizeScreenTexture = false;
        gWindowResized = false;
        return;
    }

    if (gResizeShadowTexture)
    { //skip render on frames where window has been resized
        gPipeline.resizeShadowTexture();
        gResizeShadowTexture = false;
    }

    gSnapshot = for_snapshot;

    if (LLPipeline::sRenderDeferred)
    { //hack to make sky show up in deferred snapshots
        for_snapshot = false;
    }

    LLGLSDefault gls_default;
    LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE, GL_LEQUAL);

    LLVertexBuffer::unbind();

    LLGLState::checkStates();

    gPipeline.disableLights();

    // Don't draw if the window is hidden or minimized.
    // In fact, must explicitly check the minimized state before drawing.
    // Attempting to draw into a minimized window causes a GL error. JC
    if (   !gViewerWindow->getActive()
        || !gViewerWindow->getWindow()->getVisible()
        || gViewerWindow->getWindow()->getMinimized()
        || gNonInteractive)
    {
        // Clean up memory the pools may have allocated
        if (rebuild)
        {
            stop_glerror();
            gPipeline.rebuildPools();
            stop_glerror();
        }

        stop_glerror();
        gViewerWindow->returnEmptyPicks();
        stop_glerror();

        // We still need to update the teleport progress (to get changes done
        // in TP states, else the sim does not get the messages signaling the
        // agent's arrival). This fixes BUG-230616. HB
        if (gTeleportDisplay)
        {
            // true = minimized, do not show/update the TP screen. HB
            update_tp_display(true);
        }

        // Run texture subsystem to discard memory while backgrounded
        if (!gNonInteractive)
        {
            LL_PROFILE_ZONE_NAMED("Update Images");

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Class");
                LLViewerTexture::updateClass();
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Image Update Bump");
                gBumpImageList.updateImages();  // must be called before gTextureList version so that it's textures are thrown out first.
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("List");
                F32 max_image_decode_time = 0.050f * gFrameIntervalSeconds.value();          // 50 ms/second decode time
                max_image_decode_time     = llclamp(max_image_decode_time, 0.002f, 0.005f);  // min 2ms/frame, max 5ms/frame)
                gTextureList.updateImages(max_image_decode_time);
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("GLTF Materials Cleanup");
                // remove dead gltf materials
                gGLTFMaterialList.flushMaterials();
            }
        }
        return;
    }

    gViewerWindow->checkSettings();

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Picking");
        gViewerWindow->performPick();
    }

    LLAppViewer::instance()->pingMainloopTimeout("Display:CheckStates");
    LLGLState::checkStates();

    //////////////////////////////////////////////////////////
    //
    // Logic for forcing window updates if we're in drone mode.
    //

    // *TODO: Investigate running display() during gHeadlessClient.  See if this early exit is needed DK 2011-02-18
    if (gHeadlessClient)
    {
#if LL_WINDOWS
        static F32 last_update_time = 0.f;
        if ((gFrameTimeSeconds - last_update_time) > 1.f)
        {
            InvalidateRect((HWND)gViewerWindow->getPlatformWindow(), NULL, false);
            last_update_time = gFrameTimeSeconds;
        }
#elif LL_DARWIN
        // MBW -- Do something clever here.
#endif
        // Not actually rendering, don't bother.
        return;
    }


    //
    // Bail out if we're in the startup state and don't want to try to
    // render the world.
    //
    if (LLStartUp::getStartupState() < STATE_PRECACHE)
    {
        LLAppViewer::instance()->pingMainloopTimeout("Display:Startup");
        display_startup();
        return;
    }


    if (gShaderProfileFrame)
    {
        LLGLSLShader::initProfile();
    }

    //LLGLState::verify(false);

    /////////////////////////////////////////////////
    //
    // Update GL Texture statistics (used for discard logic?)
    //

    LLAppViewer::instance()->pingMainloopTimeout("Display:TextureStats");
    stop_glerror();

    LLImageGL::updateStats(gFrameTimeSeconds);

    static LLCachedControl<S32> avatar_name_tag_mode(gSavedSettings, "AvatarNameTagMode", 1);
    static LLCachedControl<bool> name_tag_show_group_titles(gSavedSettings, "NameTagShowGroupTitles", true);
    LLVOAvatar::sRenderName = avatar_name_tag_mode;
    LLVOAvatar::sRenderGroupTitles = name_tag_show_group_titles && avatar_name_tag_mode > 0;

    gPipeline.mBackfaceCull = true;
    gFrameCount++;
    gRecentFrameCount++;
    if (gFocusMgr.getAppHasFocus())
    {
        gForegroundFrameCount++;
    }

    //////////////////////////////////////////////////////////
    //
    // Display start screen if we're teleporting, and skip render
    //

    if (gTeleportDisplay)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Teleport Display");
        LLAppViewer::instance()->pingMainloopTimeout("Display:Teleport");
        // Note: false = not minimized, do update the TP screen. HB
        update_tp_display(false);
    }
    else if(LLAppViewer::instance()->logoutRequestSent())
    {
        LLAppViewer::instance()->pingMainloopTimeout("Display:Logout");
        F32 percent_done = gLogoutTimer.getElapsedTimeF32() * 100.f / gLogoutMaxTime;
        if (percent_done > 100.f)
        {
            percent_done = 100.f;
        }

        if( LLApp::isExiting() )
        {
            percent_done = 100.f;
        }

        gViewerWindow->setProgressPercent( percent_done );
        gViewerWindow->setProgressMessage(std::string());
    }
    else
    if (gRestoreGL)
    {
        LLAppViewer::instance()->pingMainloopTimeout("Display:RestoreGL");
        F32 percent_done = gRestoreGLTimer.getElapsedTimeF32() * 100.f / RESTORE_GL_TIME;
        if( percent_done > 100.f )
        {
            gViewerWindow->setShowProgress(false);
            gRestoreGL = false;
        }
        else
        {

            if( LLApp::isExiting() )
            {
                percent_done = 100.f;
            }

            gViewerWindow->setProgressPercent( percent_done );
        }
        gViewerWindow->setProgressMessage(std::string());
    }

    //////////////////////////
    //
    // Prepare for the next frame
    //

    /////////////////////////////
    //
    // Update the camera
    //
    //

    LLAppViewer::instance()->pingMainloopTimeout("Display:Camera");
    if (LLViewerCamera::instanceExists())
    {
        LLViewerCamera::getInstance()->setZoomParameters(zoom_factor, subfield);
        LLViewerCamera::getInstance()->setNear(MIN_NEAR_PLANE);
    }

    //////////////////////////
    //
    // clear the next buffer
    // (must follow dynamic texture writing since that uses the frame buffer)
    //

    if (gDisconnected)
    {
        LLAppViewer::instance()->pingMainloopTimeout("Display:Disconnected");
        render_ui();
        swap();
    }

    //////////////////////////
    //
    // Set rendering options
    //
    //
    LLAppViewer::instance()->pingMainloopTimeout("Display:RenderSetup");
    stop_glerror();

    ///////////////////////////////////////
    //
    // Slam lighting parameters back to our defaults.
    // Note that these are not the same as GL defaults...

    stop_glerror();
    gGL.setAmbientLightColor(LLColor4::white);
    stop_glerror();

    /////////////////////////////////////
    //
    // Render
    //
    // Actually push all of our triangles to the screen.
    //

    // do render-to-texture stuff here
    if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES))
    {
        LLAppViewer::instance()->pingMainloopTimeout("Display:DynamicTextures");
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Update Dynamic Textures");
        if (LLViewerDynamicTexture::updateAllInstances())
        {
            gGL.setColorMask(true, true);
            glClear(GL_DEPTH_BUFFER_BIT);
        }
    }

    gViewerWindow->setup3DViewport();

    gPipeline.resetFrameStats();    // Reset per-frame statistics.

    if (!gDisconnected && !LLApp::isExiting())
    {
        // Render mirrors and associated hero probes before we render the rest of the scene.
        // This ensures the scene state in the hero probes are exactly the same as the rest of the scene before we render it.
        if (gPipeline.RenderMirrors && !gSnapshot)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Update hero probes");
            LL_PROFILE_GPU_ZONE("hero manager")
            gPipeline.mHeroProbeManager.update();
            gPipeline.mHeroProbeManager.renderProbes();
        }

        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("display - 1");
        LLAppViewer::instance()->pingMainloopTimeout("Display:Update");
        if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
        { //don't draw hud objects in this frame
            gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
        }

        if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES))
        { //don't draw hud particles in this frame
            gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
        }

        stop_glerror();
        display_update_camera();
        stop_glerror();

        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Env Update");
            // update all the sky/atmospheric/water settings
            LLEnvironment::instance().update(LLViewerCamera::getInstance());
        }

        // *TODO: merge these two methods
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("HUD Update");
            LLHUDManager::getInstance()->updateEffects();
            LLHUDObject::updateAll();
            stop_glerror();
        }

        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Update Geom");
            const F32 max_geom_update_time = 0.005f*10.f*gFrameIntervalSeconds.value(); // 50 ms/second update time
            gPipeline.createObjects(max_geom_update_time);
            gPipeline.processPartitionQ();
            gPipeline.updateGeom(max_geom_update_time);
            stop_glerror();
        }

        gPipeline.updateGL();

        stop_glerror();

        LLAppViewer::instance()->pingMainloopTimeout("Display:Cull");

        //Increment drawable frame counter
        LLDrawable::incrementVisible();

        LLSpatialGroup::sNoDelete = true;
        LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();

        S32 occlusion = LLPipeline::sUseOcclusion;
        if (gDepthDirty)
        { //depth buffer is invalid, don't overwrite occlusion state
            LLPipeline::sUseOcclusion = llmin(occlusion, 1);
        }
        gDepthDirty = false;

        LLGLState::checkStates();

        static LLCullResult result;
        LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
        LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater();
        gPipeline.updateCull(*LLViewerCamera::getInstance(), result);
        stop_glerror();

        LLGLState::checkStates();

        LLAppViewer::instance()->pingMainloopTimeout("Display:Swap");

        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("display - 2")
            if (gResizeScreenTexture)
            {
                gPipeline.resizeScreenTexture();
                gResizeScreenTexture = false;
            }

            gGL.setColorMask(true, true);
            glClearColor(0.f, 0.f, 0.f, 0.f);

            LLGLState::checkStates();

            if (!for_snapshot)
            {
                if (gFrameCount > 1 && !for_snapshot)
                { //for some reason, ATI 4800 series will error out if you
                  //try to generate a shadow before the first frame is through
                    gPipeline.generateSunShadow(*LLViewerCamera::getInstance());
                }

                LLVertexBuffer::unbind();

                LLGLState::checkStates();

                glm::mat4 proj = get_current_projection();
                glm::mat4 mod = get_current_modelview();
                glViewport(0,0,512,512);

                LLVOAvatar::updateImpostors();

                set_current_projection(proj);
                set_current_modelview(mod);
                gGL.matrixMode(LLRender::MM_PROJECTION);
                gGL.loadMatrix(glm::value_ptr(proj));
                gGL.matrixMode(LLRender::MM_MODELVIEW);
                gGL.loadMatrix(glm::value_ptr(mod));
                gViewerWindow->setup3DViewport();

                LLGLState::checkStates();
            }
            glClear(GL_DEPTH_BUFFER_BIT);
        }

        //////////////////////////////////////
        //
        // Update images, using the image stats generated during object update/culling
        //
        // Can put objects onto the retextured list.
        //
        // Doing this here gives hardware occlusion queries extra time to complete
        LLAppViewer::instance()->pingMainloopTimeout("Display:UpdateImages");

        {
            LL_PROFILE_ZONE_NAMED("Update Images");

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Class");
                LLViewerTexture::updateClass();
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Image Update Bump");
                gBumpImageList.updateImages();  // must be called before gTextureList version so that it's textures are thrown out first.
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("List");
                F32 max_image_decode_time = 0.050f*gFrameIntervalSeconds.value(); // 50 ms/second decode time
                max_image_decode_time = llclamp(max_image_decode_time, 0.002f, 0.005f ); // min 2ms/frame, max 5ms/frame)
                gTextureList.updateImages(max_image_decode_time);
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("GLTF Materials Cleanup");
                //remove dead gltf materials
                gGLTFMaterialList.flushMaterials();
            }
        }

        LLGLState::checkStates();

        ///////////////////////////////////
        //
        // StateSort
        //
        // Responsible for taking visible objects, and adding them to the appropriate draw orders.
        // In the case of alpha objects, z-sorts them first.
        // Also creates special lists for outlines and selected face rendering.
        //
        LLAppViewer::instance()->pingMainloopTimeout("Display:StateSort");
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("display - 4")
            LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
            gPipeline.stateSort(*LLViewerCamera::getInstance(), result);
            stop_glerror();

            if (rebuild)
            {
                //////////////////////////////////////
                //
                // rebuildPools
                //
                //
                gPipeline.rebuildPools();
                stop_glerror();
            }
        }

        LLSceneMonitor::getInstance()->fetchQueryResult();

        LLGLState::checkStates();

        LLPipeline::sUseOcclusion = occlusion;

        {
            LLAppViewer::instance()->pingMainloopTimeout("Display:Sky");
            LL_PROFILE_ZONE_NAMED_CATEGORY_ENVIRONMENT("update sky"); //LL_RECORD_BLOCK_TIME(FTM_UPDATE_SKY);
            gSky.updateSky();
        }

        if(gUseWireframe)
        {
            glClearColor(0.5f, 0.5f, 0.5f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        LLAppViewer::instance()->pingMainloopTimeout("Display:RenderStart");

        //// render frontmost floater opaque for occlusion culling purposes
        //LLFloater* frontmost_floaterp = gFloaterView->getFrontmost();
        //// assumes frontmost floater with focus is opaque
        //if (frontmost_floaterp && gFocusMgr.childHasKeyboardFocus(frontmost_floaterp))
        //{
        //  gGL.matrixMode(LLRender::MM_MODELVIEW);
        //  gGL.pushMatrix();
        //  {
        //      gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

        //      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
        //      gGL.loadIdentity();

        //      LLRect floater_rect = frontmost_floaterp->calcScreenRect();
        //      // deflate by one pixel so rounding errors don't occlude outside of floater extents
        //      floater_rect.stretch(-1);
        //      LLRectf floater_3d_rect((F32)floater_rect.mLeft / (F32)gViewerWindow->getWindowWidthScaled(),
        //                              (F32)floater_rect.mTop / (F32)gViewerWindow->getWindowHeightScaled(),
        //                              (F32)floater_rect.mRight / (F32)gViewerWindow->getWindowWidthScaled(),
        //                              (F32)floater_rect.mBottom / (F32)gViewerWindow->getWindowHeightScaled());
        //      floater_3d_rect.translate(-0.5f, -0.5f);
        //      gGL.translatef(0.f, 0.f, -LLViewerCamera::getInstance()->getNear());
        //      gGL.scalef(LLViewerCamera::getInstance()->getNear() * LLViewerCamera::getInstance()->getAspect() / sinf(LLViewerCamera::getInstance()->getView()), LLViewerCamera::getInstance()->getNear() / sinf(LLViewerCamera::getInstance()->getView()), 1.f);
        //      gGL.color4fv(LLColor4::white.mV);
        //      gGL.begin(LLVertexBuffer::QUADS);
        //      {
        //          gGL.vertex3f(floater_3d_rect.mLeft, floater_3d_rect.mBottom, 0.f);
        //          gGL.vertex3f(floater_3d_rect.mLeft, floater_3d_rect.mTop, 0.f);
        //          gGL.vertex3f(floater_3d_rect.mRight, floater_3d_rect.mTop, 0.f);
        //          gGL.vertex3f(floater_3d_rect.mRight, floater_3d_rect.mBottom, 0.f);
        //      }
        //      gGL.end();
        //      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        //  }
        //  gGL.popMatrix();
        //}

        LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater();

        LLGLState::checkStates();

        stop_glerror();

        gGL.setColorMask(true, true);

        if (LLPipeline::sRenderDeferred)
        {
            gPipeline.mRT->deferredScreen.bindTarget();
            if (gUseWireframe)
            {
                constexpr F32 g = 0.5f;
                glClearColor(g, g, g, 1.f);
            }
            else
            {
                glClearColor(1, 0, 1, 1);
            }
            gPipeline.mRT->deferredScreen.clear();
        }
        else
        {
            gPipeline.mRT->screen.bindTarget();
            if (LLPipeline::sUnderWaterRender && !gPipeline.canUseWindLightShaders())
            {
                const LLColor4& col = LLEnvironment::instance().getCurrentWater()->getWaterFogColor();
                glClearColor(col.mV[VRED], col.mV[VGREEN], col.mV[VBLUE], 0.f);
            }
            gPipeline.mRT->screen.clear();
        }

        gGL.setColorMask(true, false);

        LLAppViewer::instance()->pingMainloopTimeout("Display:RenderGeom");

        if (!(LLAppViewer::instance()->logoutRequestSent() && LLAppViewer::instance()->hasSavedFinalSnapshot())
                && !gRestoreGL)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("display - 5")
            LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;

            static LLCachedControl<bool> render_depth_pre_pass(gSavedSettings, "RenderDepthPrePass", false);
            if (render_depth_pre_pass)
            {
                gGL.setColorMask(false, false);

                constexpr U32 types[] = {
                    LLRenderPass::PASS_SIMPLE,
                    LLRenderPass::PASS_FULLBRIGHT,
                    LLRenderPass::PASS_SHINY
                };

                U32 num_types = LL_ARRAY_SIZE(types);
                gOcclusionProgram.bind();
                for (U32 i = 0; i < num_types; i++)
                {
                    gPipeline.renderObjects(types[i], LLVertexBuffer::MAP_VERTEX, false);
                }

                gOcclusionProgram.unbind();

            }

            gGL.setColorMask(true, true);
            gPipeline.renderGeomDeferred(*LLViewerCamera::getInstance(), true);
        }

        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Texture Unbind");
            for (S32 i = 0; i < gGLManager.mNumTextureImageUnits; i++)
            { //dummy cleanup of any currently bound textures
                if (gGL.getTexUnit(i)->getCurrType() != LLTexUnit::TT_NONE)
                {
                    gGL.getTexUnit(i)->unbind(gGL.getTexUnit(i)->getCurrType());
                    gGL.getTexUnit(i)->disable();
                }
            }
        }

        LLAppViewer::instance()->pingMainloopTimeout("Display:RenderFlush");

        LLRenderTarget &rt = (gPipeline.sRenderDeferred ? gPipeline.mRT->deferredScreen : gPipeline.mRT->screen);
        rt.flush();

        if (LLPipeline::sRenderDeferred)
        {
            gPipeline.renderDeferredLighting();
        }

        LLPipeline::sUnderWaterRender = false;

        {
            //capture the frame buffer.
            LLSceneMonitor::getInstance()->capture();
        }

        LLAppViewer::instance()->pingMainloopTimeout("Display:RenderUI");
        if (!for_snapshot)
        {
            render_ui();
            swap();
        }


        LLSpatialGroup::sNoDelete = false;
        gPipeline.clearReferences();
    }

    LLAppViewer::instance()->pingMainloopTimeout("Display:FrameStats");

    stop_glerror();

    display_stats();

    LLAppViewer::instance()->pingMainloopTimeout("Display:Done");

    gShiftFrame = false;

    if (gShaderProfileFrame)
    {
        gShaderProfileFrame = false;
        boost::json::value stats{ boost::json::object_kind };
        getProfileStatsContext(stats.as_object());
        LLGLSLShader::finishProfile(stats);

        auto report_name = getProfileStatsFilename();
        std::ofstream outf(report_name);
        if (! outf)
        {
            LL_WARNS() << "Couldn't write to " << std::quoted(report_name) << LL_ENDL;
        }
        else
        {
            outf << stats;
            LL_INFOS() << "(also dumped to " << std::quoted(report_name) << ")" << LL_ENDL;
        }
    }
}

void getProfileStatsContext(boost::json::object& stats)
{
    // populate the context with info from LLFloaterAbout
    auto contextit = stats.emplace("context",
                                   LlsdToJson(LLAppViewer::instance()->getViewerInfo())).first;
    auto& context = contextit->value().as_object();

    // then add a few more things
    unsigned char unique_id[MAC_ADDRESS_BYTES]{};
    LLMachineID::getUniqueID(unique_id, sizeof(unique_id));
    context.emplace("machine", stringize(LL::hexdump(unique_id, sizeof(unique_id))));
    context.emplace("grid", LLGridManager::instance().getGrid());
    LLViewerRegion* region = gAgent.getRegion();
    if (region)
    {
        context.emplace("regionid", stringize(region->getRegionID()));
    }
    LLParcel* parcel = LLViewerParcelMgr::instance().getAgentParcel();
    if (parcel)
    {
        context.emplace("parcel", parcel->getName());
        context.emplace("parcelid", parcel->getLocalID());
    }
    context.emplace("time", LLDate::now().toHTTPDateString("%Y-%m-%dT%H:%M:%S"));

    // supplement with stats packet
    stats.emplace("stats", LlsdToJson(capture_viewer_stats(true)));
}

std::string getProfileStatsFilename()
{
    std::ostringstream basebuff;
    // viewer build
    basebuff << "profile.v" << LLVersionInfo::instance().getBuild();
    // machine ID: zero-initialize unique_id in case LLMachineID fails
    unsigned char unique_id[MAC_ADDRESS_BYTES]{};
    LLMachineID::getUniqueID(unique_id, sizeof(unique_id));
    basebuff << ".m" << LL::hexdump(unique_id, sizeof(unique_id));
    // region ID
    LLViewerRegion *region = gAgent.getRegion();
    basebuff << ".r" << (region? region->getRegionID() : LLUUID());
    // local parcel ID
    LLParcel* parcel = LLViewerParcelMgr::instance().getAgentParcel();
    basebuff << ".p" << (parcel? parcel->getLocalID() : 0);
    // date/time -- omit seconds for now
    auto now = LLDate::now();
    basebuff << ".t" << LLDate::now().toHTTPDateString("%Y-%m-%dT%H-%M-");
    // put this candidate file in our logs directory
    auto base = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, basebuff.str());
    S32 sec;
    now.split(nullptr, nullptr, nullptr, nullptr, nullptr, &sec);
    // Loop over finished filename, incrementing sec until we find one that
    // doesn't yet exist. Should rarely loop (only if successive calls within
    // same second), may produce (e.g.) sec==61, but avoids collisions and
    // preserves chronological filename sort order.
    std::string name;
    std::error_code ec;
    do
    {
        // base + missing 2-digit seconds, append ".json"
        // post-increment sec in case we have to try again
        name = stringize(base, std::setw(2), std::setfill('0'), sec++, ".json");
    } while (std::filesystem::exists(fsyspath(name), ec));
    // Ignoring ec means we might potentially return a name that does already
    // exist -- but if we can't check its existence, what more can we do?
    return name;
}

// WIP simplified copy of display() that does minimal work
void display_cube_face()
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Render Cube Face");
    LL_PROFILE_GPU_ZONE("display cube face");

    llassert(!gSnapshot);
    llassert(!gTeleportDisplay);
    llassert(LLStartUp::getStartupState() >= STATE_PRECACHE);
    llassert(!LLAppViewer::instance()->logoutRequestSent());
    llassert(!gRestoreGL);

    bool rebuild = false;

    LLGLSDefault gls_default;
    LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE, GL_LEQUAL);

    LLVertexBuffer::unbind();

    gPipeline.disableLights();

    gPipeline.mBackfaceCull = true;

    gViewerWindow->setup3DViewport();

    if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
    { //don't draw hud objects in this frame
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
    }

    if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES))
    { //don't draw hud particles in this frame
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
    }

    display_update_camera();

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Env Update");
        // update all the sky/atmospheric/water settings
        LLEnvironment::instance().update(LLViewerCamera::getInstance());
    }

    LLSpatialGroup::sNoDelete = true;

    S32 occlusion = LLPipeline::sUseOcclusion;
    LLPipeline::sUseOcclusion = 0; // occlusion data is from main camera point of view, don't read or write it during cube snapshots
    //gDepthDirty = true; //let "real" render pipe know it can't trust the depth buffer for occlusion data

    static LLCullResult result;
    LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
    LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater();
    gPipeline.updateCull(*LLViewerCamera::getInstance(), result);

    gGL.setColorMask(true, true);

    glClearColor(0.f, 0.f, 0.f, 0.f);
    gPipeline.generateSunShadow(*LLViewerCamera::getInstance());

    glClear(GL_DEPTH_BUFFER_BIT); // | GL_STENCIL_BUFFER_BIT);

    {
        LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
        gPipeline.stateSort(*LLViewerCamera::getInstance(), result);

        if (rebuild)
        {
            //////////////////////////////////////
            //
            // rebuildPools
            //
            //
            gPipeline.rebuildPools();
            stop_glerror();
        }
    }

    LLPipeline::sUseOcclusion = occlusion;

    LLAppViewer::instance()->pingMainloopTimeout("Display:RenderStart");

    LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater();

    gGL.setColorMask(true, true);

    gPipeline.mRT->deferredScreen.bindTarget();
    if (gUseWireframe)
    {
        glClearColor(0.5f, 0.5f, 0.5f, 1.f);
    }
    else
    {
        glClearColor(1.f, 0.f, 1.f, 1.f);
    }
    gPipeline.mRT->deferredScreen.clear();

    LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;

    gPipeline.renderGeomDeferred(*LLViewerCamera::getInstance());

    gPipeline.mRT->deferredScreen.flush();

    gPipeline.renderDeferredLighting();

    LLPipeline::sUnderWaterRender = false;

    // Finalize scene
    //gPipeline.renderFinalize();

    LLSpatialGroup::sNoDelete = false;
    gPipeline.clearReferences();
}

void render_hud_attachments()
{
    LLPerfStats::RecordSceneTime T ( LLPerfStats::StatType_t::RENDER_HUDS); // render time capture - Primary contributor to HUDs (though these end up in render batches)
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();

    glm::mat4 current_proj = get_current_projection();
    glm::mat4 current_mod = get_current_modelview();

    // clamp target zoom level to reasonable values
    gAgentCamera.mHUDTargetZoom = llclamp(gAgentCamera.mHUDTargetZoom, 0.1f, 1.f);
    // smoothly interpolate current zoom level
    gAgentCamera.mHUDCurZoom = lerp(gAgentCamera.mHUDCurZoom, gAgentCamera.getAgentHUDTargetZoom(), LLSmoothInterpolation::getInterpolant(0.03f));

    if (LLPipeline::sShowHUDAttachments && !gDisconnected && setup_hud_matrices())
    {
        LLPipeline::sRenderingHUDs = true;
        LLCamera hud_cam = *LLViewerCamera::getInstance();
        hud_cam.setOrigin(-1.f, 0.f, 0.f);
        hud_cam.setAxes(LLVector3(1.f, 0.f, 0.f), LLVector3(0.f, 1.f, 0.f), LLVector3(0.f, 0.f, 1.f));
        LLViewerCamera::updateFrustumPlanes(hud_cam, true);

        static LLCachedControl<bool> render_hud_particles(gSavedSettings, "RenderHUDParticles", false);
        bool render_particles = gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES) && render_hud_particles;

        //only render hud objects
        gPipeline.pushRenderTypeMask();

        // turn off everything
        gPipeline.andRenderTypeMask(LLPipeline::END_RENDER_TYPES);
        // turn on HUD
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
        // turn on HUD particles
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);

        // if particles are off, turn off hud-particles as well
        if (!render_particles)
        {
            // turn back off HUD particles
            gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
        }

        bool has_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);
        if (has_ui)
        {
            gPipeline.toggleRenderDebugFeature(LLPipeline::RENDER_DEBUG_FEATURE_UI);
        }

        S32 use_occlusion = LLPipeline::sUseOcclusion;
        LLPipeline::sUseOcclusion = 0;

        //cull, sort, and render hud objects
        static LLCullResult result;
        LLSpatialGroup::sNoDelete = true;

        LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
        gPipeline.updateCull(hud_cam, result);

        // Toggle render types
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_BUMP);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_SIMPLE);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_VOLUME);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA_PRE_WATER);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA_MASK);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_FULLBRIGHT_ALPHA_MASK);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_FULLBRIGHT);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_GLTF_PBR);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_GLTF_PBR_ALPHA_MASK);

        // Toggle render passes
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_ALPHA);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_ALPHA_MASK);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_BUMP);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_MATERIAL);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_ALPHA_MASK);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_SHINY);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_SHINY);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_INVISIBLE);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_INVISI_SHINY);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_GLTF_PBR);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK);

        gPipeline.stateSort(hud_cam, result);

        gPipeline.renderGeomPostDeferred(hud_cam);

        LLSpatialGroup::sNoDelete = false;
        //gPipeline.clearReferences();

        render_hud_elements();

        //restore type mask
        gPipeline.popRenderTypeMask();

        if (has_ui)
        {
            gPipeline.toggleRenderDebugFeature(LLPipeline::RENDER_DEBUG_FEATURE_UI);
        }
        LLPipeline::sUseOcclusion = use_occlusion;
        LLPipeline::sRenderingHUDs = false;
    }
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.popMatrix();

    set_current_projection(current_proj);
    set_current_modelview(current_mod);
}

LLRect get_whole_screen_region()
{
    LLRect whole_screen = gViewerWindow->getWorldViewRectScaled();

    // apply camera zoom transform (for high res screenshots)
    F32 zoom_factor = LLViewerCamera::getInstance()->getZoomFactor();
    S16 sub_region = LLViewerCamera::getInstance()->getZoomSubRegion();
    if (zoom_factor > 1.f)
    {
        S32 num_horizontal_tiles = llceil(zoom_factor);
        S32 tile_width = ll_round((F32)gViewerWindow->getWorldViewWidthScaled() / zoom_factor);
        S32 tile_height = ll_round((F32)gViewerWindow->getWorldViewHeightScaled() / zoom_factor);
        int tile_y = sub_region / num_horizontal_tiles;
        int tile_x = sub_region - (tile_y * num_horizontal_tiles);

        whole_screen.setLeftTopAndSize(tile_x * tile_width, gViewerWindow->getWorldViewHeightScaled() - (tile_y * tile_height), tile_width, tile_height);
    }
    return whole_screen;
}

bool get_hud_matrices(const LLRect& screen_region, glm::mat4 &proj, glm::mat4&model)
{
    if (isAgentAvatarValid() && gAgentAvatarp->hasHUDAttachment())
    {
        F32 zoom_level = gAgentCamera.mHUDCurZoom;
        LLBBox hud_bbox = gAgentAvatarp->getHUDBBox();

        F32 hud_depth = llmax(1.f, hud_bbox.getExtentLocal().mV[VX] * 1.1f);
        proj = glm::ortho(-0.5f * LLViewerCamera::getInstance()->getAspect(), 0.5f * LLViewerCamera::getInstance()->getAspect(), -0.5f, 0.5f, 0.f, hud_depth);
        proj[2][2] = -0.01f;

        F32 aspect_ratio = LLViewerCamera::getInstance()->getAspect();

        F32 scale_x = (F32)gViewerWindow->getWorldViewWidthScaled() / (F32)screen_region.getWidth();
        F32 scale_y = (F32)gViewerWindow->getWorldViewHeightScaled() / (F32)screen_region.getHeight();

        glm::mat4 mat = glm::identity<glm::mat4>();
        mat = glm::translate(mat,
            glm::vec3(clamp_rescale((F32)(screen_region.getCenterX() - screen_region.mLeft), 0.f, (F32)gViewerWindow->getWorldViewWidthScaled(), 0.5f * scale_x * aspect_ratio, -0.5f * scale_x * aspect_ratio),
                clamp_rescale((F32)(screen_region.getCenterY() - screen_region.mBottom), 0.f, (F32)gViewerWindow->getWorldViewHeightScaled(), 0.5f * scale_y, -0.5f * scale_y),
                0.f));
        mat = glm::scale(mat, glm::vec3(scale_x, scale_y, 1.f));
        proj *= mat;

        glm::mat4 tmp_model = glm::make_mat4(OGL_TO_CFR_ROTATION);
        mat = glm::identity<glm::mat4>();
        mat = glm::translate(mat, glm::vec3(-hud_bbox.getCenterLocal().mV[VX] + (hud_depth * 0.5f), 0.f, 0.f));
        mat = glm::scale(mat, glm::vec3(zoom_level));
        tmp_model *= mat;
        model = tmp_model;

        return true;
    }
    else
    {
        return false;
    }
}

bool get_hud_matrices(glm::mat4 &proj, glm::mat4&model)
{
    LLRect whole_screen = get_whole_screen_region();
    return get_hud_matrices(whole_screen, proj, model);
}

bool setup_hud_matrices()
{
    LLRect whole_screen = get_whole_screen_region();
    return setup_hud_matrices(whole_screen);
}

bool setup_hud_matrices(const LLRect& screen_region)
{
    glm::mat4 proj, model;
    bool result = get_hud_matrices(screen_region, proj, model);
    if (!result) return result;

    // set up transform to keep HUD objects in front of camera
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.loadMatrix(glm::value_ptr(proj));
    set_current_projection(proj);

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.loadMatrix(glm::value_ptr(model));
    set_current_modelview(model);
    return true;
}

void render_ui(F32 zoom_factor, int subfield)
{
    LLPerfStats::RecordSceneTime T ( LLPerfStats::StatType_t::RENDER_UI ); // render time capture - Primary UI stat can have HUD time overlap (TODO)
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI; //LL_RECORD_BLOCK_TIME(FTM_RENDER_UI);
    LL_PROFILE_GPU_ZONE("ui");
    LLGLState::checkStates();

    glm::mat4 saved_view = get_current_modelview();

    if (!gSnapshot)
    {
        gGL.pushMatrix();
        gGL.loadMatrix(gGLLastModelView);
        set_current_modelview(glm::make_mat4(gGLLastModelView));
    }

    if(LLSceneMonitor::getInstance()->needsUpdate())
    {
        gGL.pushMatrix();
        gViewerWindow->setup2DRender();
        LLSceneMonitor::getInstance()->compare();
        gViewerWindow->setup3DRender();
        gGL.popMatrix();
    }

    // apply gamma correction and post effects
    gPipeline.renderFinalize();

    {
        LLGLState::checkStates();


        LL_PROFILE_ZONE_NAMED_CATEGORY_UI("HUD");
        render_hud_elements();
        LLGLState::checkStates();
        render_hud_attachments();

        LLGLState::checkStates();

        LLGLSDefault gls_default;
        LLGLSUIDefault gls_ui;
        {
            gPipeline.disableLights();
        }

        bool render_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);
        if (render_ui)
        {
            if (!gDisconnected)
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_UI("UI 3D"); //LL_RECORD_BLOCK_TIME(FTM_RENDER_UI_3D);
                LLGLState::checkStates();
                render_ui_3d();
                LLGLState::checkStates();
            }
            else
            {
                render_disconnected_background();
            }
        }

        if (render_ui)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_UI("UI 2D"); //LL_RECORD_BLOCK_TIME(FTM_RENDER_UI_2D);
            LLHUDObject::renderAll();
            render_ui_2d();
        }

        gViewerWindow->setup2DRender();
        gViewerWindow->updateDebugText();
        gViewerWindow->drawDebugText();
    }

    if (!gSnapshot)
    {
        set_current_modelview(saved_view);
        gGL.popMatrix();
    }
}

void swap()
{
    LLPerfStats::RecordSceneTime T ( LLPerfStats::StatType_t::RENDER_SWAP ); // render time capture - Swap buffer time - can signify excessive data transfer to/from GPU
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Swap");
    LL_PROFILE_GPU_ZONE("swap");
    if (gDisplaySwapBuffers)
    {
        gViewerWindow->getWindow()->swapBuffers();
    }
    gDisplaySwapBuffers = true;
}

void renderCoordinateAxes()
{
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.begin(LLRender::LINES);
        gGL.color3f(1.0f, 0.0f, 0.0f);   // i direction = X-Axis = red
        gGL.vertex3f(0.0f, 0.0f, 0.0f);
        gGL.vertex3f(2.0f, 0.0f, 0.0f);
        gGL.vertex3f(3.0f, 0.0f, 0.0f);
        gGL.vertex3f(5.0f, 0.0f, 0.0f);
        gGL.vertex3f(6.0f, 0.0f, 0.0f);
        gGL.vertex3f(8.0f, 0.0f, 0.0f);
        // Make an X
        gGL.vertex3f(11.0f, 1.0f, 1.0f);
        gGL.vertex3f(11.0f, -1.0f, -1.0f);
        gGL.vertex3f(11.0f, 1.0f, -1.0f);
        gGL.vertex3f(11.0f, -1.0f, 1.0f);

        gGL.color3f(0.0f, 1.0f, 0.0f);   // j direction = Y-Axis = green
        gGL.vertex3f(0.0f, 0.0f, 0.0f);
        gGL.vertex3f(0.0f, 2.0f, 0.0f);
        gGL.vertex3f(0.0f, 3.0f, 0.0f);
        gGL.vertex3f(0.0f, 5.0f, 0.0f);
        gGL.vertex3f(0.0f, 6.0f, 0.0f);
        gGL.vertex3f(0.0f, 8.0f, 0.0f);
        // Make a Y
        gGL.vertex3f(1.0f, 11.0f, 1.0f);
        gGL.vertex3f(0.0f, 11.0f, 0.0f);
        gGL.vertex3f(-1.0f, 11.0f, 1.0f);
        gGL.vertex3f(0.0f, 11.0f, 0.0f);
        gGL.vertex3f(0.0f, 11.0f, 0.0f);
        gGL.vertex3f(0.0f, 11.0f, -1.0f);

        gGL.color3f(0.0f, 0.0f, 1.0f);   // Z-Axis = blue
        gGL.vertex3f(0.0f, 0.0f, 0.0f);
        gGL.vertex3f(0.0f, 0.0f, 2.0f);
        gGL.vertex3f(0.0f, 0.0f, 3.0f);
        gGL.vertex3f(0.0f, 0.0f, 5.0f);
        gGL.vertex3f(0.0f, 0.0f, 6.0f);
        gGL.vertex3f(0.0f, 0.0f, 8.0f);
        // Make a Z
        gGL.vertex3f(-1.0f, 1.0f, 11.0f);
        gGL.vertex3f(1.0f, 1.0f, 11.0f);
        gGL.vertex3f(1.0f, 1.0f, 11.0f);
        gGL.vertex3f(-1.0f, -1.0f, 11.0f);
        gGL.vertex3f(-1.0f, -1.0f, 11.0f);
        gGL.vertex3f(1.0f, -1.0f, 11.0f);
    gGL.end();
}


void draw_axes()
{
    LLGLSUIDefault gls_ui;
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    // A vertical white line at origin
    LLVector3 v = gAgent.getPositionAgent();
    gGL.begin(LLRender::LINES);
        gGL.color3f(1.0f, 1.0f, 1.0f);
        gGL.vertex3f(0.0f, 0.0f, 0.0f);
        gGL.vertex3f(0.0f, 0.0f, 40.0f);
    gGL.end();
    // Some coordinate axes
    gGL.pushMatrix();
        gGL.translatef( v.mV[VX], v.mV[VY], v.mV[VZ] );
        renderCoordinateAxes();
    gGL.popMatrix();
}

void render_ui_3d()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    LLGLSPipeline gls_pipeline;

    //////////////////////////////////////
    //
    // Render 3D UI elements
    // NOTE: zbuffer is cleared before we get here by LLDrawPoolHUD,
    //       so 3d elements requiring Z buffer are moved to LLDrawPoolHUD
    //

    /////////////////////////////////////////////////////////////
    //
    // Render 2.5D elements (2D elements in the world)
    // Stuff without z writes
    //

    // Debugging stuff goes before the UI.

    stop_glerror();

    gUIProgram.bind();
    gGL.color4f(1.f, 1.f, 1.f, 1.f);

    // Coordinate axes
    static LLCachedControl<bool> show_axes(gSavedSettings, "ShowAxes");
    if (show_axes())
    {
        draw_axes();
    }

    gViewerWindow->renderSelections(false, false, true); // Non HUD call in render_hud_elements

    if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
    {
        // Render debugging beacons.
        gObjectList.renderObjectBeacons();
        gObjectList.resetObjectBeacons();
        gSky.addSunMoonBeacons();
    }
    else
    {
        // Make sure particle effects disappear
        LLHUDObject::renderAllForTimer();
    }

    stop_glerror();
}

void render_ui_2d()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    LLGLSUIDefault gls_ui;

    /////////////////////////////////////////////////////////////
    //
    // Render 2D UI elements that overlay the world (no z compare)

    //  Disable wireframe mode below here, as this is HUD/menus
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //  Menu overlays, HUD, etc
    gViewerWindow->setup2DRender();

    F32 zoom_factor = LLViewerCamera::getInstance()->getZoomFactor();
    S16 sub_region = LLViewerCamera::getInstance()->getZoomSubRegion();

    if (zoom_factor > 1.f)
    {
        //decompose subregion number to x and y values
        int pos_y = sub_region / llceil(zoom_factor);
        int pos_x = sub_region - (pos_y*llceil(zoom_factor));
        // offset for this tile
        LLFontGL::sCurOrigin.mX -= ll_round((F32)gViewerWindow->getWindowWidthScaled() * (F32)pos_x / zoom_factor);
        LLFontGL::sCurOrigin.mY -= ll_round((F32)gViewerWindow->getWindowHeightScaled() * (F32)pos_y / zoom_factor);
    }

    stop_glerror();

    // render outline for HUD
    if (isAgentAvatarValid() && gAgentCamera.mHUDCurZoom < 0.98f)
    {
        gUIProgram.bind();
        gGL.pushMatrix();
        S32 half_width = (gViewerWindow->getWorldViewWidthScaled() / 2);
        S32 half_height = (gViewerWindow->getWorldViewHeightScaled() / 2);
        gGL.scalef(LLUI::getScaleFactor().mV[VX], LLUI::getScaleFactor().mV[VY], 1.f);
        gGL.translatef((F32)half_width, (F32)half_height, 0.f);
        F32 zoom = gAgentCamera.mHUDCurZoom;
        gGL.scalef(zoom,zoom,1.f);
        gGL.color4fv(LLColor4::white.mV);
        gl_rect_2d(-half_width, half_height, half_width, -half_height, false);
        gGL.popMatrix();
        gUIProgram.unbind();
        stop_glerror();
    }


    if (LLPipeline::RenderUIBuffer)
    {
        if (LLView::sIsRectDirty)
        {
            LLView::sIsRectDirty = false;
            LLRect t_rect;

            gPipeline.mUIScreen.bindTarget();
            gGL.setColorMask(true, true);
            {
                constexpr S32 pad = 8;

                LLView::sDirtyRect.mLeft -= pad;
                LLView::sDirtyRect.mRight += pad;
                LLView::sDirtyRect.mBottom -= pad;
                LLView::sDirtyRect.mTop += pad;

                LLGLEnable scissor(GL_SCISSOR_TEST);
                static LLRect last_rect = LLView::sDirtyRect;

                //union with last rect to avoid mouse poop
                last_rect.unionWith(LLView::sDirtyRect);

                t_rect = LLView::sDirtyRect;
                LLView::sDirtyRect = last_rect;
                last_rect = t_rect;

                last_rect.mLeft = LLRect::tCoordType(last_rect.mLeft / LLUI::getScaleFactor().mV[0]);
                last_rect.mRight = LLRect::tCoordType(last_rect.mRight / LLUI::getScaleFactor().mV[0]);
                last_rect.mTop = LLRect::tCoordType(last_rect.mTop / LLUI::getScaleFactor().mV[1]);
                last_rect.mBottom = LLRect::tCoordType(last_rect.mBottom / LLUI::getScaleFactor().mV[1]);

                LLRect clip_rect(last_rect);

                glClear(GL_COLOR_BUFFER_BIT);

                gViewerWindow->draw();
            }

            gPipeline.mUIScreen.flush();
            gGL.setColorMask(true, false);

            LLView::sDirtyRect = t_rect;
        }

        LLGLDisable cull(GL_CULL_FACE);
        LLGLDisable blend(GL_BLEND);
        S32 width = gViewerWindow->getWindowWidthScaled();
        S32 height = gViewerWindow->getWindowHeightScaled();
        gGL.getTexUnit(0)->bind(&gPipeline.mUIScreen);
        gGL.begin(LLRender::TRIANGLE_STRIP);
        gGL.color4f(1.f,1.f,1.f,1.f);
        gGL.texCoord2f(0.f, 0.f);                 gGL.vertex2i(0, 0);
        gGL.texCoord2f((F32)width, 0.f);          gGL.vertex2i(width, 0);
        gGL.texCoord2f(0.f, (F32)height);         gGL.vertex2i(0, height);
        gGL.texCoord2f((F32)width, (F32)height);  gGL.vertex2i(width, height);
        gGL.end();
    }
    else
    {
        gViewerWindow->draw();
    }

    // reset current origin for font rendering, in case of tiling render
    LLFontGL::sCurOrigin.set(0, 0);
}

void render_disconnected_background()
{
    gUIProgram.bind();

    gGL.color4f(1.f, 1.f, 1.f, 1.f);
    if (!gDisconnectedImagep && gDisconnected)
    {
        LL_INFOS() << "Loading last bitmap..." << LL_ENDL;

        std::string temp_str;
        temp_str = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + LLStartUp::getScreenLastFilename();

        LLPointer<LLImagePNG> image_png = new LLImagePNG;
        if( !image_png->load(temp_str) )
        {
            //LL_INFOS() << "Bitmap load failed" << LL_ENDL;
            return;
        }

        LLPointer<LLImageRaw> raw = new LLImageRaw;
        if (!image_png->decode(raw, 0.0f))
        {
            LL_INFOS() << "Bitmap decode failed" << LL_ENDL;
            gDisconnectedImagep = NULL;
            return;
        }

        U8 *rawp = raw->getData();
        S32 npixels = (S32)image_png->getWidth()*(S32)image_png->getHeight();
        for (S32 i = 0; i < npixels; i++)
        {
            S32 sum = 0;
            sum = *rawp + *(rawp+1) + *(rawp+2);
            sum /= 3;
            *rawp = ((S32)sum*6 + *rawp)/7;
            rawp++;
            *rawp = ((S32)sum*6 + *rawp)/7;
            rawp++;
            *rawp = ((S32)sum*6 + *rawp)/7;
            rawp++;
        }


        raw->expandToPowerOfTwo();
        gDisconnectedImagep = LLViewerTextureManager::getLocalTexture(raw.get(), false);
        gStartTexture = gDisconnectedImagep;
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    }

    // Make sure the progress view always fills the entire window.
    S32 width = gViewerWindow->getWindowWidthScaled();
    S32 height = gViewerWindow->getWindowHeightScaled();

    if (gDisconnectedImagep)
    {
        LLGLSUIDefault gls_ui;
        gViewerWindow->setup2DRender();
        gGL.pushMatrix();
        {
            // scale ui to reflect UIScaleFactor
            // this can't be done in setup2DRender because it requires a
            // pushMatrix/popMatrix pair
            const LLVector2& display_scale = gViewerWindow->getDisplayScale();
            gGL.scalef(display_scale.mV[VX], display_scale.mV[VY], 1.f);

            gGL.getTexUnit(0)->bind(gDisconnectedImagep);
            gGL.color4f(1.f, 1.f, 1.f, 1.f);
            gl_rect_2d_simple_tex(width, height);
            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        }
        gGL.popMatrix();
    }
    gGL.flush();

    gUIProgram.unbind();
}

void display_cleanup()
{
    gDisconnectedImagep = nullptr;
}
