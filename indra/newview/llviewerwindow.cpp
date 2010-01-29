/** 
 * @file llviewerwindow.cpp
 * @brief Implementation of the LLViewerWindow class.
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

#include "llviewerwindow.h"

#if LL_WINDOWS
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

// system library includes
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "llfloaterreg.h"
#include "llpanellogin.h"
#include "llviewerkeyboard.h"
#include "llviewermenu.h"

#include "llviewquery.h"
#include "llxmltree.h"
//#include "llviewercamera.h"
#include "llrender.h"

#include "llvoiceclient.h"	// for push-to-talk button handling

//
// TODO: Many of these includes are unnecessary.  Remove them.
//

// linden library includes
#include "llaudioengine.h"		// mute on minimize
#include "indra_constants.h"
#include "llassetstorage.h"
#include "llerrorcontrol.h"
#include "llfontgl.h"
#include "llmousehandler.h"
#include "llrect.h"
#include "llsky.h"
#include "llstring.h"
#include "llui.h"
#include "lluuid.h"
#include "llview.h"
#include "llxfermanager.h"
#include "message.h"
#include "object_flags.h"
#include "lltimer.h"
#include "timing.h"
#include "llviewermenu.h"
#include "lltooltip.h"

// newview includes
#include "llagent.h"
#include "llbox.h"
#include "llconsole.h"
#include "llviewercontrol.h"
#include "llcylinder.h"
#include "lldebugview.h"
#include "lldir.h"
#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolwater.h"
#include "llmaniptranslate.h"
#include "llface.h"
#include "llfeaturemanager.h"
#include "llfilepicker.h"
#include "llfloater.h"
#include "llfloaterbuildoptions.h"
#include "llfloaterbuyland.h"
#include "llfloatercamera.h"
#include "llfloatercustomize.h"
#include "llfloaterland.h"
#include "llfloaterinspect.h"
#include "llfloatermap.h"
#include "llfloaternamedesc.h"
#include "llfloaterpreference.h"
#include "llfloatersnapshot.h"
#include "llfloatertools.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "llfontfreetype.h"
#include "llgesturemgr.h"
#include "llglheaders.h"
#include "lltooltip.h"
#include "llhudmanager.h"
#include "llhudview.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimageworker.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llmodaldialog.h"
#include "llmorphview.h"
#include "llmoveview.h"
#include "llnavigationbar.h"
#include "llpreviewtexture.h"
#include "llprogressview.h"
#include "llresmgr.h"
#include "llsidetray.h"
#include "llselectmgr.h"
#include "llrootview.h"
#include "llrendersphere.h"
#include "llstartup.h"
#include "llstatusbar.h"
#include "llstatview.h"
#include "llsurface.h"
#include "llsurfacepatch.h"
#include "lltexlayer.h"
#include "lltextbox.h"
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "lltextureview.h"
#include "lltool.h"
#include "lltoolcomp.h"
#include "lltooldraganddrop.h"
#include "lltoolface.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lltoolmorph.h"
#include "lltoolpie.h"
#include "lltoolselectland.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llurldispatcher.h"		// SLURL from other app instance
#include "llvieweraudio.h"
#include "llviewercamera.h"
#include "llviewergesture.h"
#include "llviewertexturelist.h"
#include "llviewerinventory.h"
#include "llviewerkeyboard.h"
#include "llviewermedia.h"
#include "llviewermediafocus.h"
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewershadermgr.h"
#include "llviewerstats.h"
#include "llvoavatarself.h"
#include "llvovolume.h"
#include "llworld.h"
#include "llworldmapview.h"
#include "pipeline.h"
#include "llappviewer.h"
#include "llviewerdisplay.h"
#include "llspatialpartition.h"
#include "llviewerjoystick.h"
#include "llviewernetwork.h"
#include "llpostprocess.h"
#include "llbottomtray.h"
#include "llnearbychatbar.h"
#include "llagentui.h"
#include "llwearablelist.h"

#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llnotificationmanager.h"

#include "llfloaternotificationsconsole.h"

#include "llnearbychat.h"
#include "llviewerwindowlistener.h"

#if LL_WINDOWS
#include <tchar.h> // For Unicode conversion methods
#endif

//
// Globals
//
void render_ui(F32 zoom_factor = 1.f, int subfield = 0);

extern BOOL gDebugClicks;
extern BOOL gDisplaySwapBuffers;
extern BOOL gDepthDirty;
extern BOOL gResizeScreenTexture;

LLViewerWindow	*gViewerWindow = NULL;

LLFrameTimer	gAwayTimer;
LLFrameTimer	gAwayTriggerTimer;

BOOL			gShowOverlayTitle = FALSE;

LLViewerObject*  gDebugRaycastObject = NULL;
LLVector3       gDebugRaycastIntersection;
LLVector2       gDebugRaycastTexCoord;
LLVector3       gDebugRaycastNormal;
LLVector3       gDebugRaycastBinormal;
S32				gDebugRaycastFaceHit;

// HUD display lines in lower right
BOOL				gDisplayWindInfo = FALSE;
BOOL				gDisplayCameraPos = FALSE;
BOOL				gDisplayFOV = FALSE;

S32 CHAT_BAR_HEIGHT = 28; 
S32 OVERLAY_BAR_HEIGHT = 20;

const U8 NO_FACE = 255;
BOOL gQuietSnapshot = FALSE;

const F32 MIN_AFK_TIME = 2.f; // minimum time after setting away state before coming back
const F32 MAX_FAST_FRAME_TIME = 0.5f;
const F32 FAST_FRAME_INCREMENT = 0.1f;

const F32 MIN_DISPLAY_SCALE = 0.75f;

std::string	LLViewerWindow::sSnapshotBaseName;
std::string	LLViewerWindow::sSnapshotDir;

std::string	LLViewerWindow::sMovieBaseName;

class RecordToChatConsole : public LLError::Recorder, public LLSingleton<RecordToChatConsole>
{
public:
	virtual void recordMessage(LLError::ELevel level,
								const std::string& message)
	{
		//FIXME: this is NOT thread safe, and will do bad things when a warning is issued from a non-UI thread

		// only log warnings to chat console
		//if (level == LLError::LEVEL_WARN)
		//{
			//LLFloaterChat* chat_floater = LLFloaterReg::findTypedInstance<LLFloaterChat>("chat");
			//if (chat_floater && gSavedSettings.getBOOL("WarningsAsChat"))
			//{
			//	LLChat chat;
			//	chat.mText = message;
			//	chat.mSourceType = CHAT_SOURCE_SYSTEM;

			//	chat_floater->addChat(chat, FALSE, FALSE);
			//}
		//}
	}
};

////////////////////////////////////////////////////////////////////////////
//
// LLDebugText
//

class LLDebugText
{
private:
	struct Line
	{
		Line(const std::string& in_text, S32 in_x, S32 in_y) : text(in_text), x(in_x), y(in_y) {}
		std::string text;
		S32 x,y;
	};

	LLViewerWindow *mWindow;
	
	typedef std::vector<Line> line_list_t;
	line_list_t mLineList;
	LLColor4 mTextColor;
	
public:
	LLDebugText(LLViewerWindow* window) : mWindow(window) {}
	
	void addText(S32 x, S32 y, const std::string &text) 
	{
		mLineList.push_back(Line(text, x, y));
	}

	void update()
	{
		std::string wind_vel_text;
		std::string wind_vector_text;
		std::string rwind_vel_text;
		std::string rwind_vector_text;
		std::string audio_text;

		// Draw the statistics in a light gray
		// and in a thin font
		mTextColor = LLColor4( 0.86f, 0.86f, 0.86f, 1.f );

		// Draw stuff growing up from right lower corner of screen
		U32 xpos = mWindow->getWindowWidthScaled() - 350;
		U32 ypos = 64;
		const U32 y_inc = 20;

		if (gSavedSettings.getBOOL("DebugShowTime"))
		{
			const U32 y_inc2 = 15;
			for (std::map<S32,LLFrameTimer>::reverse_iterator iter = gDebugTimers.rbegin();
				 iter != gDebugTimers.rend(); ++iter)
			{
				S32 idx = iter->first;
				LLFrameTimer& timer = iter->second;
				F32 time = timer.getElapsedTimeF32();
				S32 hours = (S32)(time / (60*60));
				S32 mins = (S32)((time - hours*(60*60)) / 60);
				S32 secs = (S32)((time - hours*(60*60) - mins*60));
				std::string label = gDebugTimerLabel[idx];
				if (label.empty()) label = llformat("Debug: %d", idx);
				addText(xpos, ypos, llformat(" %s: %d:%02d:%02d", label.c_str(), hours,mins,secs)); ypos += y_inc2;
			}
			
			F32 time = gFrameTimeSeconds;
			S32 hours = (S32)(time / (60*60));
			S32 mins = (S32)((time - hours*(60*60)) / 60);
			S32 secs = (S32)((time - hours*(60*60) - mins*60));
			addText(xpos, ypos, llformat("Time: %d:%02d:%02d", hours,mins,secs)); ypos += y_inc;
		}
		
		if (gDisplayCameraPos)
		{
			std::string camera_view_text;
			std::string camera_center_text;
			std::string agent_view_text;
			std::string agent_left_text;
			std::string agent_center_text;
			std::string agent_root_center_text;

			LLVector3d tvector; // Temporary vector to hold data for printing.

			// Update camera center, camera view, wind info every other frame
			tvector = gAgent.getPositionGlobal();
			agent_center_text = llformat("AgentCenter  %f %f %f",
										 (F32)(tvector.mdV[VX]), (F32)(tvector.mdV[VY]), (F32)(tvector.mdV[VZ]));

			if (gAgent.getAvatarObject())
			{
				tvector = gAgent.getPosGlobalFromAgent(gAgent.getAvatarObject()->mRoot.getWorldPosition());
				agent_root_center_text = llformat("AgentRootCenter %f %f %f",
												  (F32)(tvector.mdV[VX]), (F32)(tvector.mdV[VY]), (F32)(tvector.mdV[VZ]));
			}
			else
			{
				agent_root_center_text = "---";
			}


			tvector = LLVector4(gAgent.getFrameAgent().getAtAxis());
			agent_view_text = llformat("AgentAtAxis  %f %f %f",
									   (F32)(tvector.mdV[VX]), (F32)(tvector.mdV[VY]), (F32)(tvector.mdV[VZ]));

			tvector = LLVector4(gAgent.getFrameAgent().getLeftAxis());
			agent_left_text = llformat("AgentLeftAxis  %f %f %f",
									   (F32)(tvector.mdV[VX]), (F32)(tvector.mdV[VY]), (F32)(tvector.mdV[VZ]));

			tvector = gAgent.getCameraPositionGlobal();
			camera_center_text = llformat("CameraCenter %f %f %f",
										  (F32)(tvector.mdV[VX]), (F32)(tvector.mdV[VY]), (F32)(tvector.mdV[VZ]));

			tvector = LLVector4(LLViewerCamera::getInstance()->getAtAxis());
			camera_view_text = llformat("CameraAtAxis    %f %f %f",
										(F32)(tvector.mdV[VX]), (F32)(tvector.mdV[VY]), (F32)(tvector.mdV[VZ]));
		
			addText(xpos, ypos, agent_center_text);  ypos += y_inc;
			addText(xpos, ypos, agent_root_center_text);  ypos += y_inc;
			addText(xpos, ypos, agent_view_text);  ypos += y_inc;
			addText(xpos, ypos, agent_left_text);  ypos += y_inc;
			addText(xpos, ypos, camera_center_text);  ypos += y_inc;
			addText(xpos, ypos, camera_view_text);  ypos += y_inc;
		}

		if (gDisplayWindInfo)
		{
			wind_vel_text = llformat("Wind velocity %.2f m/s", gWindVec.magVec());
			wind_vector_text = llformat("Wind vector   %.2f %.2f %.2f", gWindVec.mV[0], gWindVec.mV[1], gWindVec.mV[2]);
			rwind_vel_text = llformat("RWind vel %.2f m/s", gRelativeWindVec.magVec());
			rwind_vector_text = llformat("RWind vec   %.2f %.2f %.2f", gRelativeWindVec.mV[0], gRelativeWindVec.mV[1], gRelativeWindVec.mV[2]);

			addText(xpos, ypos, wind_vel_text);  ypos += y_inc;
			addText(xpos, ypos, wind_vector_text);  ypos += y_inc;
			addText(xpos, ypos, rwind_vel_text);  ypos += y_inc;
			addText(xpos, ypos, rwind_vector_text);  ypos += y_inc;
		}
		if (gDisplayWindInfo)
		{
			if (gAudiop)
			{
				audio_text= llformat("Audio for wind: %d", gAudiop->isWindEnabled());
			}
			addText(xpos, ypos, audio_text);  ypos += y_inc;
		}
		if (gDisplayFOV)
		{
			addText(xpos, ypos, llformat("FOV: %2.1f deg", RAD_TO_DEG * LLViewerCamera::getInstance()->getView()));
			ypos += y_inc;
		}
		
		/*if (LLViewerJoystick::getInstance()->getOverrideCamera())
		{
			addText(xpos + 200, ypos, llformat("Flycam"));
			ypos += y_inc;
		}*/
		
		if (gSavedSettings.getBOOL("DebugShowRenderInfo"))
		{
			if (gPipeline.getUseVertexShaders() == 0)
			{
				addText(xpos, ypos, "Shaders Disabled");
				ypos += y_inc;
			}
			addText(xpos, ypos, llformat("%d MB Vertex Data", LLVertexBuffer::sAllocatedBytes/(1024*1024)));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%d Vertex Buffers", LLVertexBuffer::sGLCount));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%d Mapped Buffers", LLVertexBuffer::sMappedCount));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%d Vertex Buffer Binds", LLVertexBuffer::sBindCount));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%d Vertex Buffer Sets", LLVertexBuffer::sSetCount));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%d Texture Binds", LLImageGL::sBindCount));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%d Unique Textures", LLImageGL::sUniqueCount));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%d Render Calls", gPipeline.mBatchCount));
            ypos += y_inc;

			addText(xpos, ypos, llformat("%d Matrix Ops", gPipeline.mMatrixOpCount));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%d Texture Matrix Ops", gPipeline.mTextureMatrixOps));
			ypos += y_inc;

			gPipeline.mTextureMatrixOps = 0;
			gPipeline.mMatrixOpCount = 0;

			if (gPipeline.mBatchCount > 0)
			{
				addText(xpos, ypos, llformat("Batch min/max/mean: %d/%d/%d", gPipeline.mMinBatchSize, gPipeline.mMaxBatchSize, 
					gPipeline.mTrianglesDrawn/gPipeline.mBatchCount));

				gPipeline.mMinBatchSize = gPipeline.mMaxBatchSize;
				gPipeline.mMaxBatchSize = 0;
				gPipeline.mBatchCount = 0;
			}
            ypos += y_inc;

			addText(xpos,ypos, llformat("%d/%d Nodes visible", gPipeline.mNumVisibleNodes, LLSpatialGroup::sNodeCount));
			
			ypos += y_inc;


			addText(xpos,ypos, llformat("%d Avatars visible", LLVOAvatar::sNumVisibleAvatars));
			
			ypos += y_inc;

			addText(xpos,ypos, llformat("%d Lights visible", LLPipeline::sVisibleLightCount));
			
			ypos += y_inc;

			LLVertexBuffer::sBindCount = LLImageGL::sBindCount = 
				LLVertexBuffer::sSetCount = LLImageGL::sUniqueCount = 
				gPipeline.mNumVisibleNodes = LLPipeline::sVisibleLightCount = 0;
		}
		if (gSavedSettings.getBOOL("DebugShowRenderMatrices"))
		{
			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", gGLProjection[12], gGLProjection[13], gGLProjection[14], gGLProjection[15]));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", gGLProjection[8], gGLProjection[9], gGLProjection[10], gGLProjection[11]));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", gGLProjection[4], gGLProjection[5], gGLProjection[6], gGLProjection[7]));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", gGLProjection[0], gGLProjection[1], gGLProjection[2], gGLProjection[3]));
			ypos += y_inc;

			addText(xpos, ypos, "Projection Matrix");
			ypos += y_inc;


			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", gGLModelView[12], gGLModelView[13], gGLModelView[14], gGLModelView[15]));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", gGLModelView[8], gGLModelView[9], gGLModelView[10], gGLModelView[11]));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", gGLModelView[4], gGLModelView[5], gGLModelView[6], gGLModelView[7]));
			ypos += y_inc;

			addText(xpos, ypos, llformat("%.4f    .%4f    %.4f    %.4f", gGLModelView[0], gGLModelView[1], gGLModelView[2], gGLModelView[3]));
			ypos += y_inc;

			addText(xpos, ypos, "View Matrix");
			ypos += y_inc;
		}
		if (gSavedSettings.getBOOL("DebugShowColor"))
		{
			U8 color[4];
			LLCoordGL coord = gViewerWindow->getCurrentMouse();
			glReadPixels(coord.mX, coord.mY, 1,1,GL_RGBA, GL_UNSIGNED_BYTE, color);
			addText(xpos, ypos, llformat("%d %d %d %d", color[0], color[1], color[2], color[3]));
			ypos += y_inc;
		}
		// only display these messages if we are actually rendering beacons at this moment
		if (LLPipeline::getRenderBeacons(NULL) && LLFloaterReg::instanceVisible("beacons"))
		{
			if (LLPipeline::getRenderParticleBeacons(NULL))
			{
				addText(xpos, ypos, "Viewing particle beacons (blue)");
				ypos += y_inc;
			}
			if (LLPipeline::toggleRenderTypeControlNegated((void*)LLPipeline::RENDER_TYPE_PARTICLES))
			{
				addText(xpos, ypos, "Hiding particles");
				ypos += y_inc;
			}
			if (LLPipeline::getRenderPhysicalBeacons(NULL))
			{
				addText(xpos, ypos, "Viewing physical object beacons (green)");
				ypos += y_inc;
			}
			if (LLPipeline::getRenderScriptedBeacons(NULL))
			{
				addText(xpos, ypos, "Viewing scripted object beacons (red)");
				ypos += y_inc;
			}
			else
				if (LLPipeline::getRenderScriptedTouchBeacons(NULL))
				{
					addText(xpos, ypos, "Viewing scripted object with touch function beacons (red)");
					ypos += y_inc;
				}
			if (LLPipeline::getRenderSoundBeacons(NULL))
			{
				addText(xpos, ypos, "Viewing sound beacons (yellow)");
				ypos += y_inc;
			}
		}
	}

	void draw()
	{
		for (line_list_t::iterator iter = mLineList.begin();
			 iter != mLineList.end(); ++iter)
		{
			const Line& line = *iter;
			LLFontGL::getFontMonospace()->renderUTF8(line.text, 0, (F32)line.x, (F32)line.y, mTextColor,
											 LLFontGL::LEFT, LLFontGL::TOP,
											 LLFontGL::NORMAL, LLFontGL::NO_SHADOW, S32_MAX, S32_MAX, NULL, FALSE);
		}
		mLineList.clear();
	}

};

void LLViewerWindow::updateDebugText()
{
	mDebugText->update();
}

////////////////////////////////////////////////////////////////////////////
//
// LLViewerWindow
//

BOOL LLViewerWindow::handleAnyMouseClick(LLWindow *window,  LLCoordGL pos, MASK mask, LLMouseHandler::EClickType clicktype, BOOL down)
{
	const char* buttonname = "";
	const char* buttonstatestr = "";
	S32 x = pos.mX;
	S32 y = pos.mY;
	x = llround((F32)x / mDisplayScale.mV[VX]);
	y = llround((F32)y / mDisplayScale.mV[VY]);

	if (down)
	{
		buttonstatestr = "down" ;
	}
	else
	{
		buttonstatestr = "up" ;
	}
	
	switch (clicktype)
	{
	case LLMouseHandler::CLICK_LEFT:
		mLeftMouseDown = down;
		buttonname = "Left";
		break;
	case LLMouseHandler::CLICK_RIGHT:
		mRightMouseDown = down;
		buttonname = "Right";
		break;
	case LLMouseHandler::CLICK_MIDDLE:
		mMiddleMouseDown = down;
		buttonname = "Middle";
		break;
	case LLMouseHandler::CLICK_DOUBLELEFT:
		mLeftMouseDown = down;
		buttonname = "Left Double Click";
		break;
	}
	
	LLView::sMouseHandlerMessage.clear();

	if (gMenuBarView)
	{
		// stop ALT-key access to menu
		gMenuBarView->resetMenuTrigger();
	}

	if (gDebugClicks)
	{	
		llinfos << "ViewerWindow " << buttonname << " mouse " << buttonstatestr << " at " << x << "," << y << llendl;
	}

	// Make sure we get a corresponding mouseup event, even if the mouse leaves the window
	if (down)
		mWindow->captureMouse();
	else
		mWindow->releaseMouse();

	// Indicate mouse was active
	LLUI::resetMouseIdleTimer();

	// Don't let the user move the mouse out of the window until mouse up.
	if( LLToolMgr::getInstance()->getCurrentTool()->clipMouseWhenDown() )
	{
		mWindow->setMouseClipping(down);
	}

	LLMouseHandler* mouse_captor = gFocusMgr.getMouseCapture();
	if( mouse_captor )
	{
		S32 local_x;
		S32 local_y;
		mouse_captor->screenPointToLocal( x, y, &local_x, &local_y );
		if (LLView::sDebugMouseHandling)
		{
			llinfos << buttonname << " Mouse " << buttonstatestr << " handled by captor " << mouse_captor->getName() << llendl;
		}
		return mouse_captor->handleAnyMouseClick(local_x, local_y, mask, clicktype, down);
	}

	// Topmost view gets a chance before the hierarchy
	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	if (top_ctrl)
	{
		S32 local_x, local_y;
		top_ctrl->screenPointToLocal( x, y, &local_x, &local_y );
		if (down)
		{
			if (top_ctrl->pointInView(local_x, local_y))
			{
				return top_ctrl->handleAnyMouseClick(local_x, local_y, mask, clicktype, down)	;
			}
			else
			{
				gFocusMgr.setTopCtrl(NULL);
			}
		}
		else
		{
			if (top_ctrl->pointInView(local_x, local_y) && top_ctrl->handleMouseUp(local_x, local_y, mask))
			{
				return TRUE;
			}
		}
	}

	// Give the UI views a chance to process the click
	if( mRootView->handleAnyMouseClick(x, y, mask, clicktype, down) )
	{
		if (LLView::sDebugMouseHandling)
		{
			llinfos << buttonname << " Mouse " << buttonstatestr << " " << LLView::sMouseHandlerMessage << llendl;
		}
		return TRUE;
	}
	else if (LLView::sDebugMouseHandling)
	{
		llinfos << buttonname << " Mouse " << buttonstatestr << " not handled by view" << llendl;
	}

	// Do not allow tool manager to handle mouseclicks if we have disconnected	
	if(!gDisconnected && LLToolMgr::getInstance()->getCurrentTool()->handleAnyMouseClick( x, y, mask, clicktype, down ) )
	{
		return TRUE;
	}
	

	// If we got this far on a down-click, it wasn't handled.
	// Up-clicks, though, are always handled as far as the OS is concerned.
	BOOL default_rtn = !down;
	return default_rtn;
}

BOOL LLViewerWindow::handleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	BOOL down = TRUE;
	return handleAnyMouseClick(window,pos,mask,LLMouseHandler::CLICK_LEFT,down);
}

BOOL LLViewerWindow::handleDoubleClick(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	// try handling as a double-click first, then a single-click if that
	// wasn't handled.
	BOOL down = TRUE;
	if (handleAnyMouseClick(window, pos, mask,
				LLMouseHandler::CLICK_DOUBLELEFT, down))
	{
		return TRUE;
	}
	return handleMouseDown(window, pos, mask);
}

BOOL LLViewerWindow::handleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	BOOL down = FALSE;
	return handleAnyMouseClick(window,pos,mask,LLMouseHandler::CLICK_LEFT,down);
}


BOOL LLViewerWindow::handleRightMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	S32 x = pos.mX;
	S32 y = pos.mY;
	x = llround((F32)x / mDisplayScale.mV[VX]);
	y = llround((F32)y / mDisplayScale.mV[VY]);

	BOOL down = TRUE;
	BOOL handle = handleAnyMouseClick(window,pos,mask,LLMouseHandler::CLICK_RIGHT,down);
	if (handle)
		return handle;

	// *HACK: this should be rolled into the composite tool logic, not
	// hardcoded at the top level.
	if (CAMERA_MODE_CUSTOMIZE_AVATAR != gAgent.getCameraMode() && LLToolMgr::getInstance()->getCurrentTool() != LLToolPie::getInstance())
	{
		// If the current tool didn't process the click, we should show
		// the pie menu.  This can be done by passing the event to the pie
		// menu tool.
		LLToolPie::getInstance()->handleRightMouseDown(x, y, mask);
		// show_context_menu( x, y, mask );
	}

	return TRUE;
}

BOOL LLViewerWindow::handleRightMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	BOOL down = FALSE;
 	return handleAnyMouseClick(window,pos,mask,LLMouseHandler::CLICK_RIGHT,down);
}

BOOL LLViewerWindow::handleMiddleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	BOOL down = TRUE;
	gVoiceClient->middleMouseState(true);
 	handleAnyMouseClick(window,pos,mask,LLMouseHandler::CLICK_MIDDLE,down);
  
  	// Always handled as far as the OS is concerned.
	return TRUE;
}
  
BOOL LLViewerWindow::handleMiddleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	BOOL down = FALSE;
	gVoiceClient->middleMouseState(false);
 	handleAnyMouseClick(window,pos,mask,LLMouseHandler::CLICK_MIDDLE,down);
  
  	// Always handled as far as the OS is concerned.
	return TRUE;
}

// WARNING: this is potentially called multiple times per frame
void LLViewerWindow::handleMouseMove(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	S32 x = pos.mX;
	S32 y = pos.mY;

	x = llround((F32)x / mDisplayScale.mV[VX]);
	y = llround((F32)y / mDisplayScale.mV[VY]);

	mMouseInWindow = TRUE;

	// Save mouse point for access during idle() and display()

	LLCoordGL mouse_point(x, y);

	if (mouse_point != mCurrentMousePoint)
	{
		LLUI::resetMouseIdleTimer();
	}

	saveLastMouse(mouse_point);

	mWindow->showCursorFromMouseMove();

	if (gAwayTimer.getElapsedTimeF32() > MIN_AFK_TIME)
	{
		gAgent.clearAFK();
	}
}

void LLViewerWindow::handleMouseLeave(LLWindow *window)
{
	// Note: we won't get this if we have captured the mouse.
	llassert( gFocusMgr.getMouseCapture() == NULL );
	mMouseInWindow = FALSE;
}

BOOL LLViewerWindow::handleCloseRequest(LLWindow *window)
{
	// User has indicated they want to close, but we may need to ask
	// about modified documents.
	LLAppViewer::instance()->userQuit();
	// Don't quit immediately
	return FALSE;
}

void LLViewerWindow::handleQuit(LLWindow *window)
{
	LLAppViewer::instance()->forceQuit();
}

void LLViewerWindow::handleResize(LLWindow *window,  S32 width,  S32 height)
{
	reshape(width, height);
	mResDirty = true;
}

// The top-level window has gained focus (e.g. via ALT-TAB)
void LLViewerWindow::handleFocus(LLWindow *window)
{
	gFocusMgr.setAppHasFocus(TRUE);
	LLModalDialog::onAppFocusGained();

	gAgent.onAppFocusGained();
	LLToolMgr::getInstance()->onAppFocusGained();

	// See if we're coming in with modifier keys held down
	if (gKeyboard)
	{
		gKeyboard->resetMaskKeys();
	}

	// resume foreground running timer
	// since we artifically limit framerate when not frontmost
	gForegroundTime.unpause();
}

// The top-level window has lost focus (e.g. via ALT-TAB)
void LLViewerWindow::handleFocusLost(LLWindow *window)
{
	gFocusMgr.setAppHasFocus(FALSE);
	//LLModalDialog::onAppFocusLost();
	LLToolMgr::getInstance()->onAppFocusLost();
	gFocusMgr.setMouseCapture( NULL );

	if (gMenuBarView)
	{
		// stop ALT-key access to menu
		gMenuBarView->resetMenuTrigger();
	}

	// restore mouse cursor
	showCursor();
	getWindow()->setMouseClipping(FALSE);

	// If losing focus while keys are down, reset them.
	if (gKeyboard)
	{
		gKeyboard->resetKeys();
	}

	// pause timer that tracks total foreground running time
	gForegroundTime.pause();
}


BOOL LLViewerWindow::handleTranslatedKeyDown(KEY key,  MASK mask, BOOL repeated)
{
	// Let the voice chat code check for its PTT key.  Note that this never affects event processing.
	gVoiceClient->keyDown(key, mask);
	
	if (gAwayTimer.getElapsedTimeF32() > MIN_AFK_TIME)
	{
		gAgent.clearAFK();
	}

	// *NOTE: We want to interpret KEY_RETURN later when it arrives as
	// a Unicode char, not as a keydown.  Otherwise when client frame
	// rate is really low, hitting return sends your chat text before
	// it's all entered/processed.
	if (key == KEY_RETURN && mask == MASK_NONE)
	{
		return FALSE;
	}

	return gViewerKeyboard.handleKey(key, mask, repeated);
}

BOOL LLViewerWindow::handleTranslatedKeyUp(KEY key,  MASK mask)
{
	// Let the voice chat code check for its PTT key.  Note that this never affects event processing.
	gVoiceClient->keyUp(key, mask);

	return FALSE;
}


void LLViewerWindow::handleScanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level)
{
	LLViewerJoystick::getInstance()->setCameraNeedsUpdate(true);
	return gViewerKeyboard.scanKey(key, key_down, key_up, key_level);
}




BOOL LLViewerWindow::handleActivate(LLWindow *window, BOOL activated)
{
	if (activated)
	{
		mActive = TRUE;
		send_agent_resume();
		gAgent.clearAFK();
		if (mWindow->getFullscreen() && !mIgnoreActivate)
		{
			if (!LLApp::isExiting() )
			{
				if (LLStartUp::getStartupState() >= STATE_STARTED)
				{
					// if we're in world, show a progress bar to hide reloading of textures
					llinfos << "Restoring GL during activate" << llendl;
					restoreGL("Restoring...");
				}
				else
				{
					// otherwise restore immediately
					restoreGL();
				}
			}
			else
			{
				llwarns << "Activating while quitting" << llendl;
			}
		}

		// Unmute audio
		audio_update_volume();
	}
	else
	{
		mActive = FALSE;
				
		if (gSavedSettings.getS32("AFKTimeout"))
		{
			gAgent.setAFK();
		}
		
		// SL-53351: Make sure we're not in mouselook when minimised, to prevent control issues
		if (gAgent.getCameraMode() == CAMERA_MODE_MOUSELOOK)
		{
			gAgent.changeCameraToDefault();
		}
		
		send_agent_pause();
		
		if (mWindow->getFullscreen() && !mIgnoreActivate)
		{
			llinfos << "Stopping GL during deactivation" << llendl;
			stopGL();
		}
		// Mute audio
		audio_update_volume();
	}
	return TRUE;
}

BOOL LLViewerWindow::handleActivateApp(LLWindow *window, BOOL activating)
{
	//if (!activating) gAgent.changeCameraToDefault();

	LLViewerJoystick::getInstance()->setNeedsReset(true);
	return FALSE;
}


void LLViewerWindow::handleMenuSelect(LLWindow *window,  S32 menu_item)
{
}


BOOL LLViewerWindow::handlePaint(LLWindow *window,  S32 x,  S32 y, S32 width,  S32 height)
{
#if LL_WINDOWS
	if (gNoRender)
	{
		HWND window_handle = (HWND)window->getPlatformWindow();
		PAINTSTRUCT ps; 
		HDC hdc; 
 
		RECT wnd_rect;
		wnd_rect.left = 0;
		wnd_rect.top = 0;
		wnd_rect.bottom = 200;
		wnd_rect.right = 500;

		hdc = BeginPaint(window_handle, &ps); 
		//SetBKColor(hdc, RGB(255, 255, 255));
		FillRect(hdc, &wnd_rect, CreateSolidBrush(RGB(255, 255, 255)));

		std::string name_str;
		LLAgentUI::buildName(name_str);

		std::string temp_str;
		temp_str = llformat( "%s FPS %3.1f Phy FPS %2.1f Time Dil %1.3f",		/* Flawfinder: ignore */
				name_str.c_str(),
				LLViewerStats::getInstance()->mFPSStat.getMeanPerSec(),
				LLViewerStats::getInstance()->mSimPhysicsFPS.getPrev(0),
				LLViewerStats::getInstance()->mSimTimeDilation.getPrev(0));
		S32 len = temp_str.length();
		TextOutA(hdc, 0, 0, temp_str.c_str(), len); 


		LLVector3d pos_global = gAgent.getPositionGlobal();
		temp_str = llformat( "Avatar pos %6.1lf %6.1lf %6.1lf", pos_global.mdV[0], pos_global.mdV[1], pos_global.mdV[2]);
		len = temp_str.length();
		TextOutA(hdc, 0, 25, temp_str.c_str(), len); 

		TextOutA(hdc, 0, 50, "Set \"DisableRendering FALSE\" in settings.ini file to reenable", 61);
		EndPaint(window_handle, &ps); 
		return TRUE;
	}
#endif
	return FALSE;
}


void LLViewerWindow::handleScrollWheel(LLWindow *window,  S32 clicks)
{
	handleScrollWheel( clicks );
}

void LLViewerWindow::handleWindowBlock(LLWindow *window)
{
	send_agent_pause();
}

void LLViewerWindow::handleWindowUnblock(LLWindow *window)
{
	send_agent_resume();
}

void LLViewerWindow::handleDataCopy(LLWindow *window, S32 data_type, void *data)
{
	const S32 SLURL_MESSAGE_TYPE = 0;
	switch (data_type)
	{
	case SLURL_MESSAGE_TYPE:
		// received URL
		std::string url = (const char*)data;
		LLMediaCtrl* web = NULL;
		const bool trusted_browser = false;
		if (LLURLDispatcher::dispatch(url, web, trusted_browser))
		{
			// bring window to foreground, as it has just been "launched" from a URL
			mWindow->bringToFront();
		}
		break;
	}
}

BOOL LLViewerWindow::handleTimerEvent(LLWindow *window)
{
	if (LLViewerJoystick::getInstance()->getOverrideCamera())
	{
		LLViewerJoystick::getInstance()->updateStatus();
		return TRUE;
	}
	return FALSE;
}

BOOL LLViewerWindow::handleDeviceChange(LLWindow *window)
{
	// give a chance to use a joystick after startup (hot-plugging)
	if (!LLViewerJoystick::getInstance()->isJoystickInitialized() )
	{
		LLViewerJoystick::getInstance()->init(true);
		return TRUE;
	}
	return FALSE;
}

void LLViewerWindow::handlePingWatchdog(LLWindow *window, const char * msg)
{
	LLAppViewer::instance()->pingMainloopTimeout(msg);
}


void LLViewerWindow::handleResumeWatchdog(LLWindow *window)
{
	LLAppViewer::instance()->resumeMainloopTimeout();
}

void LLViewerWindow::handlePauseWatchdog(LLWindow *window)
{
	LLAppViewer::instance()->pauseMainloopTimeout();
}

//virtual
std::string LLViewerWindow::translateString(const char* tag)
{
	return LLTrans::getString( std::string(tag) );
}

//virtual
std::string LLViewerWindow::translateString(const char* tag,
		const std::map<std::string, std::string>& args)
{
	// LLTrans uses a special subclass of std::string for format maps,
	// but we must use std::map<> in these callbacks, otherwise we create
	// a dependency between LLWindow and LLFormatMapString.  So copy the data.
	LLStringUtil::format_map_t args_copy;
	std::map<std::string,std::string>::const_iterator it = args.begin();
	for ( ; it != args.end(); ++it)
	{
		args_copy[it->first] = it->second;
	}
	return LLTrans::getString( std::string(tag), args_copy);
}

//
// Classes
//
LLViewerWindow::LLViewerWindow(
	const std::string& title, const std::string& name,
	S32 x, S32 y,
	S32 width, S32 height,
	BOOL fullscreen, BOOL ignore_pixel_depth)
	:
	mWindow(NULL),
	mActive(TRUE),
	mWantFullscreen(fullscreen),
	mShowFullscreenProgress(FALSE),
	mWindowRectRaw(0, height, width, 0),
	mWindowRectScaled(0, height, width, 0),
	mWorldViewRectRaw(0, height, width, 0),
	mLeftMouseDown(FALSE),
	mMiddleMouseDown(FALSE),
	mRightMouseDown(FALSE),
	mMouseInWindow( FALSE ),
	mLastMask( MASK_NONE ),
	mToolStored( NULL ),
	mHideCursorPermanent( FALSE ),
	mCursorHidden(FALSE),
	mIgnoreActivate( FALSE ),
	mResDirty(false),
	mStatesDirty(false),
	mIsFullscreenChecked(false),
	mCurrResolutionIndex(0),
    mViewerWindowListener(new LLViewerWindowListener(this))
{
	LLNotificationChannel::buildChannel("VW_alerts", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "alert"));
	LLNotificationChannel::buildChannel("VW_alertmodal", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "alertmodal"));

	LLNotifications::instance().getChannel("VW_alerts")->connectChanged(&LLViewerWindow::onAlert);
	LLNotifications::instance().getChannel("VW_alertmodal")->connectChanged(&LLViewerWindow::onAlert);
	LLNotifications::instance().setIgnoreAllNotifications(gSavedSettings.getBOOL("IgnoreAllNotifications"));
	llinfos << "NOTE: ALL NOTIFICATIONS THAT OCCUR WILL GET ADDED TO IGNORE LIST FOR LATER RUNS." << llendl;

	// Default to application directory.
	LLViewerWindow::sSnapshotBaseName = "Snapshot";
	LLViewerWindow::sMovieBaseName = "SLmovie";
	resetSnapshotLoc();

	// create window
	mWindow = LLWindowManager::createWindow(this,
		title, name, x, y, width, height, 0,
		fullscreen, 
		gNoRender,
		gSavedSettings.getBOOL("DisableVerticalSync"),
		!gNoRender,
		ignore_pixel_depth,
		gSavedSettings.getU32("RenderFSAASamples"));

	if (!LLAppViewer::instance()->restoreErrorTrap())
	{
		LL_WARNS("Window") << " Someone took over my signal/exception handler (post createWindow)!" << LL_ENDL;
	}


	if (NULL == mWindow)
	{
		LLSplashScreen::update("Shutting down...");
#if LL_LINUX || LL_SOLARIS
		llwarns << "Unable to create window, be sure screen is set at 32-bit color and your graphics driver is configured correctly.  See README-linux.txt or README-solaris.txt for further information."
				<< llendl;
#else
		LL_WARNS("Window") << "Unable to create window, be sure screen is set at 32-bit color in Control Panels->Display->Settings"
				<< LL_ENDL;
#endif
        LLAppViewer::instance()->forceExit(1);
	}
	
	// Get the real window rect the window was created with (since there are various OS-dependent reasons why
	// the size of a window or fullscreen context may have been adjusted slightly...)
	F32 ui_scale_factor = gSavedSettings.getF32("UIScaleFactor");
	
	mDisplayScale.setVec(llmax(1.f / mWindow->getPixelAspectRatio(), 1.f), llmax(mWindow->getPixelAspectRatio(), 1.f));
	mDisplayScale *= ui_scale_factor;
	LLUI::setScaleFactor(mDisplayScale);

	{
		LLCoordWindow size;
		mWindow->getSize(&size);
		mWindowRectRaw.set(0, size.mY, size.mX, 0);
		mWindowRectScaled.set(0, llround((F32)size.mY / mDisplayScale.mV[VY]), llround((F32)size.mX / mDisplayScale.mV[VX]), 0);
	}
	
	LLFontManager::initClass();

	//
	// We want to set this stuff up BEFORE we initialize the pipeline, so we can turn off
	// stuff like AGP if we think that it'll crash the viewer.
	//
	LL_DEBUGS("Window") << "Loading feature tables." << LL_ENDL;

	LLFeatureManager::getInstance()->init();

	// Initialize OpenGL Renderer
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderVBOEnable") ||
		!gGLManager.mHasVertexBufferObject)
	{
		gSavedSettings.setBOOL("RenderVBOEnable", FALSE);
	}
	LLVertexBuffer::initClass(gSavedSettings.getBOOL("RenderVBOEnable"));

	if (LLFeatureManager::getInstance()->isSafe()
		|| (gSavedSettings.getS32("LastFeatureVersion") != LLFeatureManager::getInstance()->getVersion())
		|| (gSavedSettings.getBOOL("ProbeHardwareOnStartup")))
	{
		LLFeatureManager::getInstance()->applyRecommendedSettings();
		gSavedSettings.setBOOL("ProbeHardwareOnStartup", FALSE);
	}

	// If we crashed while initializng GL stuff last time, disable certain features
	if (gSavedSettings.getBOOL("RenderInitError"))
	{
		mInitAlert = "DisplaySettingsNoShaders";
		LLFeatureManager::getInstance()->setGraphicsLevel(0, false);
		gSavedSettings.setU32("RenderQualityPerformance", 0);		
	}
		
	// Init the image list.  Must happen after GL is initialized and before the images that
	// LLViewerWindow needs are requested.
	LLImageGL::initClass(LLViewerTexture::MAX_GL_IMAGE_CATEGORY) ;
	gTextureList.init();
	LLViewerTextureManager::init() ;
	gBumpImageList.init();
	
	// Init font system, but don't actually load the fonts yet
	// because our window isn't onscreen and they take several
	// seconds to parse.
	LLFontGL::initClass( gSavedSettings.getF32("FontScreenDPI"),
								mDisplayScale.mV[VX],
								mDisplayScale.mV[VY],
								gDirUtilp->getAppRODataDir(),
								LLUI::getXUIPaths());
	
	// Create container for all sub-views
	LLView::Params rvp;
	rvp.name("root");
	rvp.rect(mWindowRectScaled);
	rvp.mouse_opaque(false);
	rvp.follows.flags(FOLLOWS_NONE);
	mRootView = LLUICtrlFactory::create<LLRootView>(rvp);
	LLUI::setRootView(mRootView);

	// Make avatar head look forward at start
	mCurrentMousePoint.mX = getWindowWidthScaled() / 2;
	mCurrentMousePoint.mY = getWindowHeightScaled() / 2;

	gShowOverlayTitle = gSavedSettings.getBOOL("ShowOverlayTitle");
	mOverlayTitle = gSavedSettings.getString("OverlayTitle");
	// Can't have spaces in settings.ini strings, so use underscores instead and convert them.
	LLStringUtil::replaceChar(mOverlayTitle, '_', ' ');

	// sync the keyboard's setting with the saved setting
	gSavedSettings.getControl("NumpadControl")->firePropertyChanged();

	mDebugText = new LLDebugText(this);

	mWorldViewRectScaled = calcScaledRect(mWorldViewRectRaw, mDisplayScale);
}

void LLViewerWindow::initGLDefaults()
{
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

	F32 ambient[4] = {0.f,0.f,0.f,0.f };
	F32 diffuse[4] = {1.f,1.f,1.f,1.f };
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,ambient);
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,diffuse);
	
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);

	gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);

	// lights for objects
	glShadeModel( GL_SMOOTH );

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
	
	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);

	glCullFace(GL_BACK);

	// RN: Need this for translation and stretch manip.
	gCone.prerender();
	gBox.prerender();
	gSphere.prerender();
	gCylinder.prerender();
}

struct MainPanel : public LLPanel
{
};

void LLViewerWindow::initBase()
{
	S32 height = getWindowHeightScaled();
	S32 width = getWindowWidthScaled();

	LLRect full_window(0, height, width, 0);

	////////////////////
	//
	// Set the gamma
	//

	F32 gamma = gSavedSettings.getF32("RenderGamma");
	if (gamma != 0.0f)
	{
		getWindow()->setGamma(gamma);
	}

	// Create global views

	// Create the floater view at the start so that other views can add children to it. 
	// (But wait to add it as a child of the root view so that it will be in front of the 
	// other views.)
	MainPanel* main_view = new MainPanel();
	LLUICtrlFactory::instance().buildPanel(main_view, "main_view.xml");
	main_view->setShape(full_window);
	getRootView()->addChild(main_view);

	// placeholder widget that controls where "world" is rendered
	mWorldViewPlaceholder = main_view->getChildView("world_view_rect")->getHandle();
	mNonSideTrayView = main_view->getChildView("non_side_tray_view")->getHandle();
	mFloaterViewHolder = main_view->getChildView("floater_view_holder")->getHandle();

	// Constrain floaters to inside the menu and status bar regions.
	gFloaterView = main_view->getChild<LLFloaterView>("Floater View");
	gSnapshotFloaterView = main_view->getChild<LLSnapshotFloaterView>("Snapshot Floater View");
	
	// Console
	llassert( !gConsole );
	LLConsole::Params cp;
	cp.name("console");
	cp.max_lines(gSavedSettings.getS32("ConsoleBufferSize"));
	cp.rect(getChatConsoleRect());
	cp.persist_time(gSavedSettings.getF32("ChatPersistTime"));
	cp.font_size_index(gSavedSettings.getS32("ChatFontSize"));
	cp.follows.flags(FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_BOTTOM);
	gConsole = LLUICtrlFactory::create<LLConsole>(cp);
	getRootView()->addChild(gConsole);

	// optionally forward warnings to chat console/chat floater
	// for qa runs and dev builds
#if  !LL_RELEASE_FOR_DOWNLOAD
	LLError::addRecorder(RecordToChatConsole::getInstance());
#else
	if(gSavedSettings.getBOOL("QAMode"))
	{
		LLError::addRecorder(RecordToChatConsole::getInstance());
	}
#endif

	gDebugView = getRootView()->getChild<LLDebugView>("DebugView");
	gDebugView->init();
	gToolTipView = getRootView()->getChild<LLToolTipView>("tooltip view");

	// Add the progress bar view (startup view), which overrides everything
	mProgressView = new LLProgressView(full_window);
	getRootView()->addChild(mProgressView);
	setShowProgress(FALSE);
	setProgressCancelButtonVisible(FALSE);

	gMenuHolder = getRootView()->getChild<LLViewerMenuHolderGL>("Menu Holder");

	LLMenuGL::sMenuContainer = gMenuHolder;

}

void LLViewerWindow::initWorldUI()
{
	S32 height = mRootView->getRect().getHeight();
	S32 width = mRootView->getRect().getWidth();
	LLRect full_window(0, height, width, 0);


	gIMMgr = LLIMMgr::getInstance();

	getRootView()->sendChildToFront(gFloaterView);
	getRootView()->sendChildToFront(gSnapshotFloaterView);

	// new bottom panel
	LLPanel* bottom_tray_container = getRootView()->getChild<LLPanel>("bottom_tray_container");
	LLBottomTray* bottom_tray = LLBottomTray::getInstance();
	bottom_tray->setShape(bottom_tray_container->getLocalRect());
	bottom_tray->setFollowsAll();
	bottom_tray_container->addChild(bottom_tray);
	bottom_tray_container->setVisible(TRUE);

	LLRect morph_view_rect = full_window;
	morph_view_rect.stretch( -STATUS_BAR_HEIGHT );
	morph_view_rect.mTop = full_window.mTop - 32;
	LLMorphView::Params mvp;
	mvp.name("MorphView");
	mvp.rect(morph_view_rect);
	mvp.visible(false);
	gMorphView = LLUICtrlFactory::create<LLMorphView>(mvp);
	getRootView()->addChild(gMorphView);

	LLWorldMapView::initClass();
	
	// Force gFloaterWorldMap to initialize
	LLFloaterReg::getInstance("world_map");

	// Force gFloaterTools to initialize
	LLFloaterReg::getInstance("build");
	LLFloaterReg::hideInstance("build");

	// Status bar
	LLPanel* status_bar_container = getRootView()->getChild<LLPanel>("status_bar_container");
	gStatusBar = new LLStatusBar(status_bar_container->getLocalRect());
	gStatusBar->setFollowsAll();
	gStatusBar->setShape(status_bar_container->getLocalRect());
	// sync bg color with menu bar
	gStatusBar->setBackgroundColor( gMenuBarView->getBackgroundColor().get() );
	status_bar_container->addChild(gStatusBar);
	status_bar_container->setVisible(TRUE);

	// Navigation bar
	LLPanel* nav_bar_container = getRootView()->getChild<LLPanel>("nav_bar_container");

	LLNavigationBar* navbar = LLNavigationBar::getInstance();
	navbar->setShape(nav_bar_container->getLocalRect());
	navbar->setBackgroundColor(gMenuBarView->getBackgroundColor().get());
	nav_bar_container->addChild(navbar);
	nav_bar_container->setVisible(TRUE);
	
	if (!gSavedSettings.getBOOL("ShowNavbarNavigationPanel"))
	{
		navbar->showNavigationPanel(FALSE);
	}

	if (!gSavedSettings.getBOOL("ShowNavbarFavoritesPanel"))
	{
		navbar->showFavoritesPanel(FALSE);
	}

	if (!gSavedSettings.getBOOL("ShowCameraButton"))
	{
		LLBottomTray::getInstance()->showCameraButton(FALSE);
	}

	if (!gSavedSettings.getBOOL("ShowSnapshotButton"))
	{
		LLBottomTray::getInstance()->showSnapshotButton(FALSE);
	}

	if (!gSavedSettings.getBOOL("ShowMoveButton"))
	{
		LLBottomTray::getInstance()->showMoveButton(FALSE);
	}

	if (!gSavedSettings.getBOOL("ShowGestureButton"))
	{
		LLBottomTray::getInstance()->showGestureButton(FALSE);
	}

	if ( gHUDView == NULL )
	{
		LLRect hud_rect = full_window;
		hud_rect.mBottom += 50;
		if (gMenuBarView)
		{
			hud_rect.mTop -= gMenuBarView->getRect().getHeight();
		}
		gHUDView = new LLHUDView(hud_rect);
		// put behind everything else in the UI
		getRootView()->addChildInBack(gHUDView);
	}

	LLPanel* panel_ssf_container = getRootView()->getChild<LLPanel>("stand_stop_flying_container");
	LLPanelStandStopFlying* panel_stand_stop_flying	= LLPanelStandStopFlying::getInstance();
	panel_ssf_container->addChild(panel_stand_stop_flying);
	panel_ssf_container->setVisible(TRUE);

	// put sidetray in container
	LLPanel* side_tray_container = getRootView()->getChild<LLPanel>("side_tray_container");
	LLSideTray* sidetrayp = LLSideTray::getInstance();
	sidetrayp->setShape(side_tray_container->getLocalRect());
	// don't follow right edge to avoid spurious resizes, since we are using a fixed width layout
	sidetrayp->setFollows(FOLLOWS_LEFT|FOLLOWS_TOP|FOLLOWS_BOTTOM);
	side_tray_container->addChild(sidetrayp);
	side_tray_container->setVisible(FALSE);
	
	// put sidetray buttons in their own panel
	LLPanel* buttons_panel = sidetrayp->getButtonsPanel();
	LLPanel* buttons_panel_container = getRootView()->getChild<LLPanel>("side_bar_tabs");
	buttons_panel->setShape(buttons_panel_container->getLocalRect());
	buttons_panel->setFollowsAll();
	buttons_panel_container->addChild(buttons_panel);
}

// Destroy the UI
void LLViewerWindow::shutdownViews()
{
	// clean up warning logger
	LLError::removeRecorder(RecordToChatConsole::getInstance());

	delete mDebugText;
	mDebugText = NULL;
	
	// Cleanup global views
	if (gMorphView)
	{
		gMorphView->setVisible(FALSE);
	}

	// DEV-40930: Clear sModalStack. Otherwise, any LLModalDialog left open
	// will crump with LL_ERRS.
	LLModalDialog::shutdownModals();
	
	// destroy the nav bar, not currently part of gViewerWindow
	// *TODO: Make LLNavigationBar part of gViewerWindow
	delete LLNavigationBar::getInstance();
	
	// Delete all child views.
	delete mRootView;
	mRootView = NULL;

	// Automatically deleted as children of mRootView.  Fix the globals.
	gStatusBar = NULL;
	gIMMgr = NULL;
	gToolTipView = NULL;

	gFloaterView = NULL;
	gMorphView = NULL;

	gHUDView = NULL;
}

void LLViewerWindow::shutdownGL()
{
	//--------------------------------------------------------
	// Shutdown GL cleanly.  Order is very important here.
	//--------------------------------------------------------
	LLFontGL::destroyDefaultFonts();
	LLFontManager::cleanupClass();
	stop_glerror();

	gSky.cleanup();
	stop_glerror();

	LLWearableList::instance().cleanup() ;

	gTextureList.shutdown();
	stop_glerror();

	gBumpImageList.shutdown();
	stop_glerror();

	LLWorldMapView::cleanupTextures();

	llinfos << "Cleaning up pipeline" << llendl;
	gPipeline.cleanup();
	stop_glerror();

	LLViewerTextureManager::cleanup() ;
	LLImageGL::cleanupClass() ;

	llinfos << "All textures and llimagegl images are destroyed!" << llendl ;

	llinfos << "Cleaning up select manager" << llendl;
	LLSelectMgr::getInstance()->cleanup();

	LLVertexBuffer::cleanupClass();

	llinfos << "Stopping GL during shutdown" << llendl;
	if (!gNoRender)
	{
		stopGL(FALSE);
		stop_glerror();
	}

	gGL.shutdown();
}

// shutdownViews() and shutdownGL() need to be called first
LLViewerWindow::~LLViewerWindow()
{
	llinfos << "Destroying Window" << llendl;
	destroyWindow();

	delete mDebugText;
	mDebugText = NULL;
}


void LLViewerWindow::setCursor( ECursorType c )
{
	mWindow->setCursor( c );
}

void LLViewerWindow::showCursor()
{
	mWindow->showCursor();
	
	mCursorHidden = FALSE;
}

void LLViewerWindow::hideCursor()
{
	// And hide the cursor
	mWindow->hideCursor();

	mCursorHidden = TRUE;
}

void LLViewerWindow::sendShapeToSim()
{
	LLMessageSystem* msg = gMessageSystem;
	if(!msg) return;
	msg->newMessageFast(_PREHASH_AgentHeightWidth);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addU32Fast(_PREHASH_CircuitCode, gMessageSystem->mOurCircuitCode);
	msg->nextBlockFast(_PREHASH_HeightWidthBlock);
	msg->addU32Fast(_PREHASH_GenCounter, 0);
	U16 height16 = (U16) mWorldViewRectRaw.getHeight();
	U16 width16 = (U16) mWorldViewRectRaw.getWidth();
	msg->addU16Fast(_PREHASH_Height, height16);
	msg->addU16Fast(_PREHASH_Width, width16);
	gAgent.sendReliableMessage();
}

// Must be called after window is created to set up agent
// camera variables and UI variables.
void LLViewerWindow::reshape(S32 width, S32 height)
{
	// Destroying the window at quit time generates spurious
	// reshape messages.  We don't care about these, and we
	// don't want to send messages because the message system
	// may have been destructed.
	if (!LLApp::isExiting())
	{
		if (gNoRender)
		{
			return;
		}

		// update our window rectangle
		mWindowRectRaw.mRight = mWindowRectRaw.mLeft + width;
		mWindowRectRaw.mTop = mWindowRectRaw.mBottom + height;

		//glViewport(0, 0, width, height );

		if (height > 0)
		{ 
			LLViewerCamera::getInstance()->setViewHeightInPixels( mWorldViewRectRaw.getHeight() );
			LLViewerCamera::getInstance()->setAspect( getWorldViewAspectRatio() );
		}

		calcDisplayScale();
	
		BOOL display_scale_changed = mDisplayScale != LLUI::sGLScaleFactor;
		LLUI::setScaleFactor(mDisplayScale);

		// update our window rectangle
		mWindowRectScaled.mRight = mWindowRectScaled.mLeft + llround((F32)width / mDisplayScale.mV[VX]);
		mWindowRectScaled.mTop = mWindowRectScaled.mBottom + llround((F32)height / mDisplayScale.mV[VY]);

		setup2DViewport();

		// Inform lower views of the change
		// round up when converting coordinates to make sure there are no gaps at edge of window
		LLView::sForceReshape = display_scale_changed;
		mRootView->reshape(llceil((F32)width / mDisplayScale.mV[VX]), llceil((F32)height / mDisplayScale.mV[VY]));
		LLView::sForceReshape = FALSE;

		// clear font width caches
		if (display_scale_changed)
		{
			LLHUDText::reshape();
		}

		sendShapeToSim();


		// store the mode the user wants (even if not there yet)
		gSavedSettings.setBOOL("WindowFullScreen", mWantFullscreen);

		// store new settings for the mode we are in, regardless
		if (!mWindow->getFullscreen())
		{
			// Only save size if not maximized
			BOOL maximized = mWindow->getMaximized();
			gSavedSettings.setBOOL("WindowMaximized", maximized);

			LLCoordScreen window_size;
			if (!maximized
				&& mWindow->getSize(&window_size))
			{
				gSavedSettings.setS32("WindowWidth", window_size.mX);
				gSavedSettings.setS32("WindowHeight", window_size.mY);
			}
		}

		LLViewerStats::getInstance()->setStat(LLViewerStats::ST_WINDOW_WIDTH, (F64)width);
		LLViewerStats::getInstance()->setStat(LLViewerStats::ST_WINDOW_HEIGHT, (F64)height);
	}
}


// Hide normal UI when a logon fails
void LLViewerWindow::setNormalControlsVisible( BOOL visible )
{
	if(LLBottomTray::instanceExists())
	{
		LLBottomTray::getInstance()->setVisible(visible);
		LLBottomTray::getInstance()->setEnabled(visible);
	}

	if ( gMenuBarView )
	{
		gMenuBarView->setVisible( visible );
		gMenuBarView->setEnabled( visible );

		// ...and set the menu color appropriately.
		setMenuBackgroundColor(gAgent.getGodLevel() > GOD_NOT, 
			LLViewerLogin::getInstance()->isInProductionGrid());
	}
        
	if ( gStatusBar )
	{
		gStatusBar->setVisible( visible );	
		gStatusBar->setEnabled( visible );	
	}
	
	LLNavigationBar* navbarp = LLUI::getRootView()->findChild<LLNavigationBar>("navigation_bar");
	if (navbarp)
	{
		navbarp->setVisible( visible );
	}
}

void LLViewerWindow::setMenuBackgroundColor(bool god_mode, bool dev_grid)
{
    LLSD args;
    LLColor4 new_bg_color;

    if(god_mode && LLViewerLogin::getInstance()->isInProductionGrid())
    {
        new_bg_color = LLUIColorTable::instance().getColor( "MenuBarGodBgColor" );
    }
    else if(god_mode && !LLViewerLogin::getInstance()->isInProductionGrid())
    {
        new_bg_color = LLUIColorTable::instance().getColor( "MenuNonProductionGodBgColor" );
    }
    else if(!god_mode && !LLViewerLogin::getInstance()->isInProductionGrid())
    {
        new_bg_color = LLUIColorTable::instance().getColor( "MenuNonProductionBgColor" );
    }
    else 
    {
        new_bg_color = LLUIColorTable::instance().getColor( "MenuBarBgColor" );
    }

    if(gMenuBarView)
    {
        gMenuBarView->setBackgroundColor( new_bg_color );
    }

    if(gStatusBar)
    {
        gStatusBar->setBackgroundColor( new_bg_color );
    }
}

void LLViewerWindow::drawDebugText()
{
	gGL.color4f(1,1,1,1);
	gGL.pushMatrix();
	{
		// scale view by UI global scale factor and aspect ratio correction factor
		glScalef(mDisplayScale.mV[VX], mDisplayScale.mV[VY], 1.f);
		mDebugText->draw();
	}
	gGL.popMatrix();
	gGL.flush();
}

void LLViewerWindow::draw()
{
	
#if LL_DEBUG
	LLView::sIsDrawing = TRUE;
#endif
	stop_glerror();
	
	LLUI::setLineWidth(1.f);

	LLUI::setLineWidth(1.f);
	// Reset any left-over transforms
	glMatrixMode(GL_MODELVIEW);
	
	glLoadIdentity();

	//S32 screen_x, screen_y;

	if (!gSavedSettings.getBOOL("RenderUIBuffer"))
	{
		LLUI::sDirtyRect = getWindowRectScaled();
	}

	// HACK for timecode debugging
	if (gSavedSettings.getBOOL("DisplayTimecode"))
	{
		// draw timecode block
		std::string text;

		glLoadIdentity();

		microsecondsToTimecodeString(gFrameTime,text);
		const LLFontGL* font = LLFontGL::getFontSansSerif();
		font->renderUTF8(text, 0,
						llround((getWindowWidthScaled()/2)-100.f),
						llround((getWindowHeightScaled()-60.f)),
			LLColor4( 1.f, 1.f, 1.f, 1.f ),
			LLFontGL::LEFT, LLFontGL::TOP);
	}

	// Draw all nested UI views.
	// No translation needed, this view is glued to 0,0

	gGL.pushMatrix();
	{
		// scale view by UI global scale factor and aspect ratio correction factor
		glScalef(mDisplayScale.mV[VX], mDisplayScale.mV[VY], 1.f);

		LLVector2 old_scale_factor = LLUI::sGLScaleFactor;
		// apply camera zoom transform (for high res screenshots)
		F32 zoom_factor = LLViewerCamera::getInstance()->getZoomFactor();
		S16 sub_region = LLViewerCamera::getInstance()->getZoomSubRegion();
		if (zoom_factor > 1.f)
		{
			//decompose subregion number to x and y values
			int pos_y = sub_region / llceil(zoom_factor);
			int pos_x = sub_region - (pos_y*llceil(zoom_factor));
			// offset for this tile
			glTranslatef((F32)getWindowWidthScaled() * -(F32)pos_x, 
						(F32)getWindowHeightScaled() * -(F32)pos_y, 
						0.f);
			glScalef(zoom_factor, zoom_factor, 1.f);
			LLUI::sGLScaleFactor *= zoom_factor;
		}

		// Draw tool specific overlay on world
		LLToolMgr::getInstance()->getCurrentTool()->draw();

		if( gAgent.cameraMouselook() )
		{
			drawMouselookInstructions();
			stop_glerror();
		}

		// Draw all nested UI views.
		// No translation needed, this view is glued to 0,0
		mRootView->draw();

		if (LLView::sDebugRects)
		{
			gToolTipView->drawStickyRect();
		}

		// Draw optional on-top-of-everyone view
		LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
		if (top_ctrl && top_ctrl->getVisible())
		{
			S32 screen_x, screen_y;
			top_ctrl->localPointToScreen(0, 0, &screen_x, &screen_y);

			glMatrixMode(GL_MODELVIEW);
			LLUI::pushMatrix();
			LLUI::translate( (F32) screen_x, (F32) screen_y, 0.f);
			top_ctrl->draw();	
			LLUI::popMatrix();
		}


		if( gShowOverlayTitle && !mOverlayTitle.empty() )
		{
			// Used for special titles such as "Second Life - Special E3 2003 Beta"
			const S32 DIST_FROM_TOP = 20;
			LLFontGL::getFontSansSerifBig()->renderUTF8(
				mOverlayTitle, 0,
				llround( getWindowWidthScaled() * 0.5f),
				getWindowHeightScaled() - DIST_FROM_TOP,
				LLColor4(1, 1, 1, 0.4f),
				LLFontGL::HCENTER, LLFontGL::TOP);
		}

		LLUI::sGLScaleFactor = old_scale_factor;
	}
	gGL.popMatrix();

#if LL_DEBUG
	LLView::sIsDrawing = FALSE;
#endif
}

// Takes a single keydown event, usually when UI is visible
BOOL LLViewerWindow::handleKey(KEY key, MASK mask)
{
	if (gFocusMgr.getKeyboardFocus() 
		&& !(mask & (MASK_CONTROL | MASK_ALT))
		&& !gFocusMgr.getKeystrokesOnly())
	{
		// We have keyboard focus, and it's not an accelerator

		if (key < 0x80)
		{
			// Not a special key, so likely (we hope) to generate a character.  Let it fall through to character handler first.
			return (gFocusMgr.getKeyboardFocus() != NULL);
		}
	}

	// hide tooltips on keypress
	LLToolTipMgr::instance().blockToolTips();
	
	// Explicit hack for debug menu.
	if ((MASK_ALT & mask) &&
		(MASK_CONTROL & mask) &&
		('D' == key || 'd' == key))
	{
		toggle_debug_menus(NULL);
	}

		// Explicit hack for debug menu.
	if ((mask == (MASK_SHIFT | MASK_CONTROL)) &&
		('G' == key || 'g' == key))
	{
		if  (LLStartUp::getStartupState() < STATE_LOGIN_CLEANUP)  //on splash page
		{
			BOOL visible = ! gSavedSettings.getBOOL("ForceShowGrid");
			gSavedSettings.setBOOL("ForceShowGrid", visible);

			// Initialize visibility (and don't force visibility - use prefs)
			LLPanelLogin::refreshLocation( false );
		}
	}

	// Debugging view for unified notifications: CTRL-SHIFT-5
	// *FIXME: Having this special-cased right here (just so this can be invoked from the login screen) sucks.
	if ((MASK_SHIFT & mask) 
	    && (!(MASK_ALT & mask))
	    && (MASK_CONTROL & mask)
	    && ('5' == key))
	{
		//LLFloaterNotificationConsole::showInstance();
		LLFloaterReg::showInstance("notifications_console");
		return TRUE;
	}

	// handle escape key
	//if (key == KEY_ESCAPE && mask == MASK_NONE)
	//{

		// *TODO: get this to play well with mouselook and hidden
		// cursor modes, etc, and re-enable.
		//if (gFocusMgr.getMouseCapture())
		//{
		//	gFocusMgr.setMouseCapture(NULL);
		//	return TRUE;
		//}
	//}

	// let menus handle navigation keys
	if (gMenuBarView && gMenuBarView->handleKey(key, mask, TRUE))
	{
		return TRUE;
	}
	// let menus handle navigation keys
	if (gLoginMenuBarView && gLoginMenuBarView->handleKey(key, mask, TRUE))
	{
		return TRUE;
	}
	//some of context menus use this container, let context menu handle navigation keys
	if(gMenuHolder && gMenuHolder->handleKey(key, mask, TRUE))
	{
		return TRUE;
	}

	// Traverses up the hierarchy
	LLFocusableElement* keyboard_focus = gFocusMgr.getKeyboardFocus();
	if( keyboard_focus )
	{
		LLLineEditor* chat_editor = LLBottomTray::instanceExists() ? LLBottomTray::getInstance()->getNearbyChatBar()->getChatBox() : NULL;
		// arrow keys move avatar while chatting hack
		if (chat_editor && chat_editor->hasFocus())
		{
			// If text field is empty, there's no point in trying to move
			// cursor with arrow keys, so allow movement
			if (chat_editor->getText().empty() 
				|| gSavedSettings.getBOOL("ArrowKeysAlwaysMove"))
			{
				// let Control-Up and Control-Down through for chat line history,
				if (!(key == KEY_UP && mask == MASK_CONTROL)
					&& !(key == KEY_DOWN && mask == MASK_CONTROL))
				{
					switch(key)
					{
					case KEY_LEFT:
					case KEY_RIGHT:
					case KEY_UP:
					case KEY_DOWN:
					case KEY_PAGE_UP:
					case KEY_PAGE_DOWN:
					case KEY_HOME:
						// when chatbar is empty or ArrowKeysAlwaysMove set,
						// pass arrow keys on to avatar...
						return FALSE;
					default:
						break;
					}
				}
			}
		}

		if (keyboard_focus->handleKey(key, mask, FALSE))
		{
			return TRUE;
		}
	}

	if( LLToolMgr::getInstance()->getCurrentTool()->handleKey(key, mask) )
	{
		return TRUE;
	}

	// Try for a new-format gesture
	if (LLGestureManager::instance().triggerGesture(key, mask))
	{
		return TRUE;
	}

	// See if this is a gesture trigger.  If so, eat the key and
	// don't pass it down to the menus.
	if (gGestureList.trigger(key, mask))
	{
		return TRUE;
	}

	// Topmost view gets a chance before the hierarchy
	// *FIX: get rid of this?
	//LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	//if (top_ctrl)
	//{
	//	if( top_ctrl->handleKey( key, mask, TRUE ) )
	//	{
	//		return TRUE;
	//	}
	//}

	// give floaters first chance to handle TAB key
	// so frontmost floater gets focus
	if (key == KEY_TAB)
	{
		// if nothing has focus, go to first or last UI element as appropriate
		if (mask & MASK_CONTROL || gFocusMgr.getKeyboardFocus() == NULL)
		{
			if (gMenuHolder) gMenuHolder->hideMenus();

			// if CTRL-tabbing (and not just TAB with no focus), go into window cycle mode
			gFloaterView->setCycleMode((mask & MASK_CONTROL) != 0);

			// do CTRL-TAB and CTRL-SHIFT-TAB logic
			if (mask & MASK_SHIFT)
			{
				mRootView->focusPrevRoot();
			}
			else
			{
				mRootView->focusNextRoot();
			}
			return TRUE;
		}
	}
	
	// give menus a chance to handle keys
	if (gMenuBarView && gMenuBarView->handleAcceleratorKey(key, mask))
	{
		return TRUE;
	}
	
	// give menus a chance to handle keys
	if (gLoginMenuBarView && gLoginMenuBarView->handleAcceleratorKey(key, mask))
	{
		return TRUE;
	}

	// don't pass keys on to world when something in ui has focus
	return gFocusMgr.childHasKeyboardFocus(mRootView) 
		|| LLMenuGL::getKeyboardMode() 
		|| (gMenuBarView && gMenuBarView->getHighlightedItem() && gMenuBarView->getHighlightedItem()->isActive());
}


BOOL LLViewerWindow::handleUnicodeChar(llwchar uni_char, MASK mask)
{
	// HACK:  We delay processing of return keys until they arrive as a Unicode char,
	// so that if you're typing chat text at low frame rate, we don't send the chat
	// until all keystrokes have been entered. JC
	// HACK: Numeric keypad <enter> on Mac is Unicode 3
	// HACK: Control-M on Windows is Unicode 13
	if ((uni_char == 13 && mask != MASK_CONTROL)
		|| (uni_char == 3 && mask == MASK_NONE))
	{
		return gViewerKeyboard.handleKey(KEY_RETURN, mask, gKeyboard->getKeyRepeated(KEY_RETURN));
	}

	// let menus handle navigation (jump) keys
	if (gMenuBarView && gMenuBarView->handleUnicodeChar(uni_char, TRUE))
	{
		return TRUE;
	}

	// Traverses up the hierarchy
	LLFocusableElement* keyboard_focus = gFocusMgr.getKeyboardFocus();
	if( keyboard_focus )
	{
		if (keyboard_focus->handleUnicodeChar(uni_char, FALSE))
		{
			return TRUE;
		}

		//// Topmost view gets a chance before the hierarchy
		//LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
		//if (top_ctrl && top_ctrl->handleUnicodeChar( uni_char, FALSE ) )
		//{
		//	return TRUE;
		//}

		return TRUE;
	}

	return FALSE;
}


void LLViewerWindow::handleScrollWheel(S32 clicks)
{
	LLView::sMouseHandlerMessage.clear();

	LLUI::resetMouseIdleTimer();
	
	LLMouseHandler* mouse_captor = gFocusMgr.getMouseCapture();
	if( mouse_captor )
	{
		S32 local_x;
		S32 local_y;
		mouse_captor->screenPointToLocal( mCurrentMousePoint.mX, mCurrentMousePoint.mY, &local_x, &local_y );
		mouse_captor->handleScrollWheel(local_x, local_y, clicks);
		if (LLView::sDebugMouseHandling)
		{
			llinfos << "Scroll Wheel handled by captor " << mouse_captor->getName() << llendl;
		}
		return;
	}

	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	if (top_ctrl)
	{
		S32 local_x;
		S32 local_y;
		top_ctrl->screenPointToLocal( mCurrentMousePoint.mX, mCurrentMousePoint.mY, &local_x, &local_y );
		if (top_ctrl->handleScrollWheel(local_x, local_y, clicks)) return;
	}

	if (mRootView->handleScrollWheel(mCurrentMousePoint.mX, mCurrentMousePoint.mY, clicks) )
	{
		if (LLView::sDebugMouseHandling)
		{
			llinfos << "Scroll Wheel" << LLView::sMouseHandlerMessage << llendl;
		}
		return;
	}
	else if (LLView::sDebugMouseHandling)
	{
		llinfos << "Scroll Wheel not handled by view" << llendl;
	}

	// Zoom the camera in and out behavior

	if(top_ctrl == 0 && getWorldViewRectScaled().pointInRect(mCurrentMousePoint.mX, mCurrentMousePoint.mY) )
		gAgent.handleScrollWheel(clicks);

	return;
}

void LLViewerWindow::moveCursorToCenter()
{
	S32 x = getWorldViewWidthScaled() / 2;
	S32 y = getWorldViewHeightScaled() / 2;
	
	//on a forced move, all deltas get zeroed out to prevent jumping
	mCurrentMousePoint.set(x,y);
	mLastMousePoint.set(x,y);
	mCurrentMouseDelta.set(0,0);	

	LLUI::setMousePositionScreen(x, y);	
}


//////////////////////////////////////////////////////////////////////
//
// Hover handlers
//

void append_xui_tooltip(LLView* viewp, LLToolTip::Params& params)
{
	if (viewp) 
	{
		if (!params.styled_message().empty())
		{
			params.styled_message.add().text("\n---------\n"); 
		}
		LLView::root_to_view_iterator_t end_tooltip_it = viewp->endRootToView();
		// NOTE: we skip "root" since it is assumed
		for (LLView::root_to_view_iterator_t tooltip_it = ++viewp->beginRootToView();
			tooltip_it != end_tooltip_it;
			++tooltip_it)
		{
			LLView* viewp = *tooltip_it;
		
			params.styled_message.add().text(viewp->getName());

			LLPanel* panelp = dynamic_cast<LLPanel*>(viewp);
			if (panelp && !panelp->getXMLFilename().empty())
			{
				params.styled_message.add()
					.text("(" + panelp->getXMLFilename() + ")")
					.style.color(LLColor4(0.7f, 0.7f, 1.f, 1.f));
			}
			params.styled_message.add().text("/");
		}
	}
}

// Update UI based on stored mouse position from mouse-move
// event processing.
void LLViewerWindow::updateUI()
{
	static std::string last_handle_msg;

	// animate layout stacks so we have up to date rect for world view
	LLLayoutStack::updateClass();

	updateWorldViewRect();

	LLView::sMouseHandlerMessage.clear();

	S32 x = mCurrentMousePoint.mX;
	S32 y = mCurrentMousePoint.mY;
	MASK mask = gKeyboard->currentMask(TRUE);

	if (gNoRender)
	{
		return;
	}

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_RAYCAST))
	{
		gDebugRaycastFaceHit = -1;
		gDebugRaycastObject = cursorIntersect(-1, -1, 512.f, NULL, -1, FALSE,
											  &gDebugRaycastFaceHit,
											  &gDebugRaycastIntersection,
											  &gDebugRaycastTexCoord,
											  &gDebugRaycastNormal,
											  &gDebugRaycastBinormal);
	}

	updateMouseDelta();
	updateKeyboardFocus();

	BOOL handled = FALSE;

	BOOL handled_by_top_ctrl = FALSE;
	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	LLMouseHandler* mouse_captor = gFocusMgr.getMouseCapture();
	LLView* captor_view = dynamic_cast<LLView*>(mouse_captor);

	//FIXME: only include captor and captor's ancestors if mouse is truly over them --RN

	//build set of views containing mouse cursor by traversing UI hierarchy and testing 
	//screen rect against mouse cursor
	view_handle_set_t mouse_hover_set;

	// constraint mouse enter events to children of mouse captor
	LLView* root_view = captor_view;

	// if mouse captor doesn't exist or isn't a LLView
	// then allow mouse enter events on entire UI hierarchy
	if (!root_view)
	{
		root_view = mRootView;
	}

	// include all ancestors of captor_view as automatically having mouse
	if (captor_view)
	{
		LLView* captor_parent_view = captor_view->getParent();
		while(captor_parent_view)
		{
			mouse_hover_set.insert(captor_parent_view->getHandle());
			captor_parent_view = captor_parent_view->getParent();
		}
	}

	// aggregate visible views that contain mouse cursor in display order

	// while the top_ctrl contains the mouse cursor, only it and its descendants will receive onMouseEnter events
	if (top_ctrl && top_ctrl->calcScreenBoundingRect().pointInRect(x, y))
	{
		// iterator over contents of top_ctrl, and throw into mouse_hover_set
		for (LLView::tree_iterator_t it = top_ctrl->beginTreeDFS();
			it != top_ctrl->endTreeDFS();
			++it)
		{
			LLView* viewp = *it;
			if (viewp->getVisible()
				&& viewp->calcScreenBoundingRect().pointInRect(x, y))
			{
				// we have a view that contains the mouse, add it to the set
				mouse_hover_set.insert(viewp->getHandle());
			}
			else
			{
				// skip this view and all of its children
				it.skipDescendants();
			}
		}
	}
	else
	{
		// walk UI tree in depth-first order
		for (LLView::tree_iterator_t it = root_view->beginTreeDFS();
			it != root_view->endTreeDFS();
			++it)
		{
			LLView* viewp = *it;
			// calculating the screen rect involves traversing the parent, so this is less than optimal
			if (viewp->getVisible()
				&& viewp->calcScreenBoundingRect().pointInRect(x, y))
			{

				// if this view is mouse opaque, nothing behind it should be in mouse_hover_set
				if (viewp->getMouseOpaque())
				{
					// constrain further iteration to children of this widget
					it = viewp->beginTreeDFS();
				}
	
				// we have a view that contains the mouse, add it to the set
				mouse_hover_set.insert(viewp->getHandle());
			}
			else
			{
				// skip this view and all of its children
				it.skipDescendants();
			}
		}
	}

	typedef std::vector<LLHandle<LLView> > view_handle_list_t;

	// call onMouseEnter() on all views which contain the mouse cursor but did not before
	view_handle_list_t mouse_enter_views;
	std::set_difference(mouse_hover_set.begin(), mouse_hover_set.end(),
						mMouseHoverViews.begin(), mMouseHoverViews.end(),
						std::back_inserter(mouse_enter_views));
	for (view_handle_list_t::iterator it = mouse_enter_views.begin();
		it != mouse_enter_views.end();
		++it)
	{
		LLView* viewp = it->get();
		if (viewp)
		{
			LLRect view_screen_rect = viewp->calcScreenRect();
			viewp->onMouseEnter(x - view_screen_rect.mLeft, y - view_screen_rect.mBottom, mask);
		}
	}

	// call onMouseLeave() on all views which no longer contain the mouse cursor
	view_handle_list_t mouse_leave_views;
	std::set_difference(mMouseHoverViews.begin(), mMouseHoverViews.end(),
						mouse_hover_set.begin(), mouse_hover_set.end(),
						std::back_inserter(mouse_leave_views));
	for (view_handle_list_t::iterator it = mouse_leave_views.begin();
		it != mouse_leave_views.end();
		++it)
	{
		LLView* viewp = it->get();
		if (viewp)
		{
			LLRect view_screen_rect = viewp->calcScreenRect();
			viewp->onMouseLeave(x - view_screen_rect.mLeft, y - view_screen_rect.mBottom, mask);
		}
	}

	// store resulting hover set for next frame
	swap(mMouseHoverViews, mouse_hover_set);

	if( mouse_captor )
	{
		// Pass hover events to object capturing mouse events.
		S32 local_x;
		S32 local_y; 
		mouse_captor->screenPointToLocal( x, y, &local_x, &local_y );
		handled = mouse_captor->handleHover(local_x, local_y, mask);
		if (LLView::sDebugMouseHandling)
		{
			llinfos << "Hover handled by captor " << mouse_captor->getName() << llendl;
		}

		if( !handled )
		{
			lldebugst(LLERR_USER_INPUT) << "hover not handled by mouse captor" << llendl;
		}
	}
	else
	{
		if (top_ctrl)
		{
			S32 local_x, local_y;
			top_ctrl->screenPointToLocal( x, y, &local_x, &local_y );
			handled = top_ctrl->pointInView(local_x, local_y) && top_ctrl->handleHover(local_x, local_y, mask);
			handled_by_top_ctrl = TRUE;
		}

		if ( !handled )
		{
			// x and y are from last time mouse was in window
			// mMouseInWindow tracks *actual* mouse location
			if (mMouseInWindow && mRootView->handleHover(x, y, mask) )
			{
				if (LLView::sDebugMouseHandling && LLView::sMouseHandlerMessage != last_handle_msg)
				{
					last_handle_msg = LLView::sMouseHandlerMessage;
					llinfos << "Hover" << LLView::sMouseHandlerMessage << llendl;
				}
				handled = TRUE;
			}
			else if (LLView::sDebugMouseHandling)
			{
				if (last_handle_msg != LLStringUtil::null)
				{
					last_handle_msg.clear();
					llinfos << "Hover not handled by view" << llendl;
				}
			}
		}

		if (!handled)
		{
			LLTool *tool = LLToolMgr::getInstance()->getCurrentTool();

			if(mMouseInWindow && tool)
			{
				handled = tool->handleHover(x, y, mask);
			}
		}
	}

	// Show a new tool tip (or update one that is already shown)
	BOOL tool_tip_handled = FALSE;
	std::string tool_tip_msg;
	if( handled 
		&& !mWindow->isCursorHidden())
	{
		LLRect screen_sticky_rect = mRootView->getLocalRect();
		S32 local_x, local_y;

		if (gSavedSettings.getBOOL("DebugShowXUINames"))
		{
			LLToolTip::Params params;

			LLView* tooltip_view = mRootView;
			LLView::tree_iterator_t end_it = mRootView->endTreeDFS();
			for (LLView::tree_iterator_t it = mRootView->beginTreeDFS(); it != end_it; ++it)
			{
				LLView* viewp = *it;
				LLRect screen_rect;
				viewp->localRectToScreen(viewp->getLocalRect(), &screen_rect);
				if (!(viewp->getVisible()
					 && screen_rect.pointInRect(x, y)))
				{
					it.skipDescendants();
				}
				// only report xui names for LLUICtrls, 
				// and blacklist the various containers we don't care about
				else if (dynamic_cast<LLUICtrl*>(viewp) 
						&& viewp != gMenuHolder
						&& viewp != gFloaterView
						&& viewp != gConsole) 
				{
					if (dynamic_cast<LLFloater*>(viewp))
					{
						// constrain search to descendants of this (frontmost) floater
						// by resetting iterator
						it = viewp->beginTreeDFS();
					}

					// if we are in a new part of the tree (not a descendent of current tooltip_view)
					// then push the results for tooltip_view and start with a new potential view
					// NOTE: this emulates visiting only the leaf nodes that meet our criteria
					if (!viewp->hasAncestor(tooltip_view))
					{
						append_xui_tooltip(tooltip_view, params);
						screen_sticky_rect.intersectWith(tooltip_view->calcScreenRect());
					}
					tooltip_view = viewp;
				}
			}

			append_xui_tooltip(tooltip_view, params);
			screen_sticky_rect.intersectWith(tooltip_view->calcScreenRect());
			
			params.sticky_rect = screen_sticky_rect;
			params.max_width = 400;

			LLToolTipMgr::instance().show(params);
		}
		// if there is a mouse captor, nothing else gets a tooltip
		else if (mouse_captor)
		{
			mouse_captor->screenPointToLocal(x, y, &local_x, &local_y);
			tool_tip_handled = mouse_captor->handleToolTip(local_x, local_y, mask);
		}
		else 
		{
			// next is top_ctrl
			if (!tool_tip_handled && top_ctrl)
			{
				top_ctrl->screenPointToLocal(x, y, &local_x, &local_y);
				tool_tip_handled = top_ctrl->handleToolTip(local_x, local_y, mask );
			}
			
			if (!tool_tip_handled)
			{
				local_x = x; local_y = y;
				tool_tip_handled = mRootView->handleToolTip(local_x, local_y, mask );
			}

			LLTool* current_tool = LLToolMgr::getInstance()->getCurrentTool();
			if (!tool_tip_handled && current_tool)
			{
				current_tool->screenPointToLocal(x, y, &local_x, &local_y);
				tool_tip_handled = current_tool->handleToolTip(local_x, local_y, mask );
			}
		}
	}		
	
	updateLayout();

	mLastMousePoint = mCurrentMousePoint;

	// cleanup unused selections when no modal dialogs are open
	if (LLModalDialog::activeCount() == 0)
	{
		LLViewerParcelMgr::getInstance()->deselectUnused();
	}

	if (LLModalDialog::activeCount() == 0)
	{
		LLSelectMgr::getInstance()->deselectUnused();
	}
}


void LLViewerWindow::updateLayout()
{
	LLTool* tool = LLToolMgr::getInstance()->getCurrentTool();
	if (gFloaterTools != NULL
		&& tool != NULL
		&& tool != gToolNull  
		&& tool != LLToolCompInspect::getInstance() 
		&& tool != LLToolDragAndDrop::getInstance() 
		&& !gSavedSettings.getBOOL("FreezeTime"))
	{ 
		// Suppress the toolbox view if our source tool was the pie tool,
		// and we've overridden to something else.
		bool suppress_toolbox = 
			(LLToolMgr::getInstance()->getBaseTool() == LLToolPie::getInstance()) &&
			(LLToolMgr::getInstance()->getCurrentTool() != LLToolPie::getInstance());

		LLMouseHandler *captor = gFocusMgr.getMouseCapture();
		// With the null, inspect, or drag and drop tool, don't muck
		// with visibility.

		if (gFloaterTools->isMinimized()
			||	(tool != LLToolPie::getInstance()						// not default tool
				&& tool != LLToolCompGun::getInstance()					// not coming out of mouselook
				&& !suppress_toolbox									// not override in third person
				&& LLToolMgr::getInstance()->getCurrentToolset() != gFaceEditToolset	// not special mode
				&& LLToolMgr::getInstance()->getCurrentToolset() != gMouselookToolset
				&& (!captor || dynamic_cast<LLView*>(captor) != NULL)))						// not dragging
		{
			// Force floater tools to be visible (unless minimized)
			if (!gFloaterTools->getVisible())
			{
				gFloaterTools->openFloater();
			}
			// Update the location of the blue box tool popup
			LLCoordGL select_center_screen;
			gFloaterTools->updatePopup( select_center_screen, gKeyboard->currentMask(TRUE) );
		}
		else
		{
			gFloaterTools->setVisible(FALSE);
		}
		//gMenuBarView->setItemVisible("BuildTools", gFloaterTools->getVisible());
	}

	// Always update console
	if(gConsole)
	{
		LLRect console_rect = getChatConsoleRect();
		gConsole->reshape(console_rect.getWidth(), console_rect.getHeight());
		gConsole->setRect(console_rect);
	}
}

void LLViewerWindow::updateMouseDelta()
{
	S32 dx = lltrunc((F32) (mCurrentMousePoint.mX - mLastMousePoint.mX) * LLUI::sGLScaleFactor.mV[VX]);
	S32 dy = lltrunc((F32) (mCurrentMousePoint.mY - mLastMousePoint.mY) * LLUI::sGLScaleFactor.mV[VY]);

	//RN: fix for asynchronous notification of mouse leaving window not working
	LLCoordWindow mouse_pos;
	mWindow->getCursorPosition(&mouse_pos);
	if (mouse_pos.mX < 0 || 
		mouse_pos.mY < 0 ||
		mouse_pos.mX > mWindowRectRaw.getWidth() ||
		mouse_pos.mY > mWindowRectRaw.getHeight())
	{
		mMouseInWindow = FALSE;
	}
	else
	{
		mMouseInWindow = TRUE;
	}

	LLVector2 mouse_vel; 

	if (gSavedSettings.getBOOL("MouseSmooth"))
	{
		static F32 fdx = 0.f;
		static F32 fdy = 0.f;

		F32 amount = 16.f;
		fdx = fdx + ((F32) dx - fdx) * llmin(gFrameIntervalSeconds*amount,1.f);
		fdy = fdy + ((F32) dy - fdy) * llmin(gFrameIntervalSeconds*amount,1.f);

		mCurrentMouseDelta.set(llround(fdx), llround(fdy));
		mouse_vel.setVec(fdx,fdy);
	}
	else
	{
		mCurrentMouseDelta.set(dx, dy);
		mouse_vel.setVec((F32) dx, (F32) dy);
	}
    
	mMouseVelocityStat.addValue(mouse_vel.magVec());
}

void LLViewerWindow::updateKeyboardFocus()
{
	// clean up current focus
	LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
	if (cur_focus)
	{
		if (!cur_focus->isInVisibleChain() || !cur_focus->isInEnabledChain())
		{
            // don't release focus, just reassign so that if being given
            // to a sibling won't call onFocusLost on all the ancestors
			// gFocusMgr.releaseFocusIfNeeded(cur_focus);

			LLUICtrl* parent = cur_focus->getParentUICtrl();
			const LLUICtrl* focus_root = cur_focus->findRootMostFocusRoot();
			while(parent)
			{
				if (parent->isCtrl() && 
					(parent->hasTabStop() || parent == focus_root) && 
					!parent->getIsChrome() && 
					parent->isInVisibleChain() && 
					parent->isInEnabledChain())
				{
					if (!parent->focusFirstItem())
					{
						parent->setFocus(TRUE);
					}
					break;
				}
				parent = parent->getParentUICtrl();
			}

			// if we didn't find a better place to put focus, just release it
			// hasFocus() will return true if and only if we didn't touch focus since we
			// are only moving focus higher in the hierarchy
			if (cur_focus->hasFocus())
			{
				cur_focus->setFocus(FALSE);
			}
		}
		else if (cur_focus->isFocusRoot())
		{
			// focus roots keep trying to delegate focus to their first valid descendant
			// this assumes that focus roots are not valid focus holders on their own
			cur_focus->focusFirstItem();
		}
	}

	// last ditch force of edit menu to selection manager
	if (LLEditMenuHandler::gEditMenuHandler == NULL && LLSelectMgr::getInstance()->getSelection()->getObjectCount())
	{
		LLEditMenuHandler::gEditMenuHandler = LLSelectMgr::getInstance();
	}

	if (gFloaterView->getCycleMode())
	{
		// sync all floaters with their focus state
		gFloaterView->highlightFocusedFloater();
		gSnapshotFloaterView->highlightFocusedFloater();
		if ((gKeyboard->currentMask(TRUE) & MASK_CONTROL) == 0)
		{
			// control key no longer held down, finish cycle mode
			gFloaterView->setCycleMode(FALSE);

			gFloaterView->syncFloaterTabOrder();
		}
		else
		{
			// user holding down CTRL, don't update tab order of floaters
		}
	}
	else
	{
		// update focused floater
		gFloaterView->highlightFocusedFloater();
		gSnapshotFloaterView->highlightFocusedFloater();
		// make sure floater visible order is in sync with tab order
		gFloaterView->syncFloaterTabOrder();
	}

	if(LLSideTray::instanceCreated())//just getInstance will create sidetray. we don't want this
		LLSideTray::getInstance()->highlightFocused();
}

static LLFastTimer::DeclareTimer FTM_UPDATE_WORLD_VIEW("Update World View");
void LLViewerWindow::updateWorldViewRect(bool use_full_window)
{
	LLFastTimer ft(FTM_UPDATE_WORLD_VIEW);

	// start off using whole window to render world
	LLRect new_world_rect = mWindowRectRaw;

	if (use_full_window == false && mWorldViewPlaceholder.get())
	{
		new_world_rect = mWorldViewPlaceholder.get()->calcScreenRect();
		// clamp to at least a 1x1 rect so we don't try to allocate zero width gl buffers
		new_world_rect.mTop = llmax(new_world_rect.mTop, new_world_rect.mBottom + 1);
		new_world_rect.mRight = llmax(new_world_rect.mRight, new_world_rect.mLeft + 1);

		new_world_rect.mLeft = llround((F32)new_world_rect.mLeft * mDisplayScale.mV[VX]);
		new_world_rect.mRight = llround((F32)new_world_rect.mRight * mDisplayScale.mV[VX]);
		new_world_rect.mBottom = llround((F32)new_world_rect.mBottom * mDisplayScale.mV[VY]);
		new_world_rect.mTop = llround((F32)new_world_rect.mTop * mDisplayScale.mV[VY]);
	}

	if (mWorldViewRectRaw != new_world_rect)
	{
		LLRect old_world_rect = mWorldViewRectRaw;
		mWorldViewRectRaw = new_world_rect;
		gResizeScreenTexture = TRUE;
		LLViewerCamera::getInstance()->setViewHeightInPixels( mWorldViewRectRaw.getHeight() );
		LLViewerCamera::getInstance()->setAspect( getWorldViewAspectRatio() );

		mWorldViewRectScaled = calcScaledRect(mWorldViewRectRaw, mDisplayScale);

		// sending a signal with a new WorldView rect
		old_world_rect = calcScaledRect(old_world_rect, mDisplayScale);
		mOnWorldViewRectUpdated(old_world_rect, mWorldViewRectScaled);
	}
}

void LLViewerWindow::saveLastMouse(const LLCoordGL &point)
{
	// Store last mouse location.
	// If mouse leaves window, pretend last point was on edge of window
	if (point.mX < 0)
	{
		mCurrentMousePoint.mX = 0;
	}
	else if (point.mX > getWindowWidthScaled())
	{
		mCurrentMousePoint.mX = getWindowWidthScaled();
	}
	else
	{
		mCurrentMousePoint.mX = point.mX;
	}

	if (point.mY < 0)
	{
		mCurrentMousePoint.mY = 0;
	}
	else if (point.mY > getWindowHeightScaled() )
	{
		mCurrentMousePoint.mY = getWindowHeightScaled();
	}
	else
	{
		mCurrentMousePoint.mY = point.mY;
	}
}


// Draws the selection outlines for the currently selected objects
// Must be called after displayObjects is called, which sets the mGLName parameter
// NOTE: This function gets called 3 times:
//  render_ui_3d: 			FALSE, FALSE, TRUE
//  renderObjectsForSelect:	TRUE, pick_parcel_wall, FALSE
//  render_hud_elements:	FALSE, FALSE, FALSE
void LLViewerWindow::renderSelections( BOOL for_gl_pick, BOOL pick_parcel_walls, BOOL for_hud )
{
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();

	if (!for_hud && !for_gl_pick)
	{
		// Call this once and only once
		LLSelectMgr::getInstance()->updateSilhouettes();
	}
	
	// Draw fence around land selections
	if (for_gl_pick)
	{
		if (pick_parcel_walls)
		{
			LLViewerParcelMgr::getInstance()->renderParcelCollision();
		}
	}
	else if (( for_hud && selection->getSelectType() == SELECT_TYPE_HUD) ||
			 (!for_hud && selection->getSelectType() != SELECT_TYPE_HUD))
	{		
		LLSelectMgr::getInstance()->renderSilhouettes(for_hud);
		
		stop_glerror();

		// setup HUD render
		if (selection->getSelectType() == SELECT_TYPE_HUD && LLSelectMgr::getInstance()->getSelection()->getObjectCount())
		{
			LLBBox hud_bbox = gAgent.getAvatarObject()->getHUDBBox();

			// set up transform to encompass bounding box of HUD
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			F32 depth = llmax(1.f, hud_bbox.getExtentLocal().mV[VX] * 1.1f);
			glOrtho(-0.5f * LLViewerCamera::getInstance()->getAspect(), 0.5f * LLViewerCamera::getInstance()->getAspect(), -0.5f, 0.5f, 0.f, depth);
			
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glLoadMatrixf(OGL_TO_CFR_ROTATION);		// Load Cory's favorite reference frame
			glTranslatef(-hud_bbox.getCenterLocal().mV[VX] + (depth *0.5f), 0.f, 0.f);
		}

		// Render light for editing
		if (LLSelectMgr::sRenderLightRadius && LLToolMgr::getInstance()->inEdit())
		{
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			LLGLEnable gls_blend(GL_BLEND);
			LLGLEnable gls_cull(GL_CULL_FACE);
			LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			if (selection->getSelectType() == SELECT_TYPE_HUD)
			{
				F32 zoom = gAgent.mHUDCurZoom;
				glScalef(zoom, zoom, zoom);
			}

			struct f : public LLSelectedObjectFunctor
			{
				virtual bool apply(LLViewerObject* object)
				{
					LLDrawable* drawable = object->mDrawable;
					if (drawable && drawable->isLight())
					{
						LLVOVolume* vovolume = drawable->getVOVolume();
						glPushMatrix();

						LLVector3 center = drawable->getPositionAgent();
						glTranslatef(center[0], center[1], center[2]);
						F32 scale = vovolume->getLightRadius();
						glScalef(scale, scale, scale);

						LLColor4 color(vovolume->getLightColor(), .5f);
						glColor4fv(color.mV);
					
						F32 pixel_area = 100000.f;
						// Render Outside
						gSphere.render(pixel_area);

						// Render Inside
						glCullFace(GL_FRONT);
						gSphere.render(pixel_area);
						glCullFace(GL_BACK);
					
						glPopMatrix();
					}
					return true;
				}
			} func;
			LLSelectMgr::getInstance()->getSelection()->applyToObjects(&func);
			
			glPopMatrix();
		}				
		
		// NOTE: The average position for the axis arrows of the selected objects should
		// not be recalculated at this time.  If they are, then group rotations will break.

		// Draw arrows at average center of all selected objects
		LLTool* tool = LLToolMgr::getInstance()->getCurrentTool();
		if (tool)
		{
			if(tool->isAlwaysRendered())
			{
				tool->render();
			}
			else
			{
				if( !LLSelectMgr::getInstance()->getSelection()->isEmpty() )
				{
					BOOL moveable_object_selected = FALSE;
					BOOL all_selected_objects_move = TRUE;
					BOOL all_selected_objects_modify = TRUE;
					BOOL selecting_linked_set = !gSavedSettings.getBOOL("EditLinkedParts");

					for (LLObjectSelection::iterator iter = LLSelectMgr::getInstance()->getSelection()->begin();
						 iter != LLSelectMgr::getInstance()->getSelection()->end(); iter++)
					{
						LLSelectNode* nodep = *iter;
						LLViewerObject* object = nodep->getObject();
						BOOL this_object_movable = FALSE;
						if (object->permMove() && (object->permModify() || selecting_linked_set))
						{
							moveable_object_selected = TRUE;
							this_object_movable = TRUE;
						}
						all_selected_objects_move = all_selected_objects_move && this_object_movable;
						all_selected_objects_modify = all_selected_objects_modify && object->permModify();
					}

					BOOL draw_handles = TRUE;

					if (tool == LLToolCompTranslate::getInstance() && (!moveable_object_selected || !all_selected_objects_move))
					{
						draw_handles = FALSE;
					}

					if (tool == LLToolCompRotate::getInstance() && (!moveable_object_selected || !all_selected_objects_move))
					{
						draw_handles = FALSE;
					}

					if ( !all_selected_objects_modify && tool == LLToolCompScale::getInstance() )
					{
						draw_handles = FALSE;
					}
				
					if( draw_handles )
					{
						tool->render();
					}
				}
			}
			if (selection->getSelectType() == SELECT_TYPE_HUD && selection->getObjectCount())
			{
				glMatrixMode(GL_PROJECTION);
				glPopMatrix();

				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
				stop_glerror();
			}
		}
	}
}

// Return a point near the clicked object representative of the place the object was clicked.
LLVector3d LLViewerWindow::clickPointInWorldGlobal(S32 x, S32 y_from_bot, LLViewerObject* clicked_object) const
{
	// create a normalized vector pointing from the camera center into the 
	// world at the location of the mouse click
	LLVector3 mouse_direction_global = mouseDirectionGlobal( x, y_from_bot );

	LLVector3d relative_object = clicked_object->getPositionGlobal() - gAgent.getCameraPositionGlobal();

	// make mouse vector as long as object vector, so it touchs a point near
	// where the user clicked on the object
	mouse_direction_global *= (F32) relative_object.magVec();

	LLVector3d new_pos;
	new_pos.setVec(mouse_direction_global);
	// transform mouse vector back to world coords
	new_pos += gAgent.getCameraPositionGlobal();

	return new_pos;
}


BOOL LLViewerWindow::clickPointOnSurfaceGlobal(const S32 x, const S32 y, LLViewerObject *objectp, LLVector3d &point_global) const
{
	BOOL intersect = FALSE;

//	U8 shape = objectp->mPrimitiveCode & LL_PCODE_BASE_MASK;
	if (!intersect)
	{
		point_global = clickPointInWorldGlobal(x, y, objectp);
		llinfos << "approx intersection at " <<  (objectp->getPositionGlobal() - point_global) << llendl;
	}
	else
	{
		llinfos << "good intersection at " <<  (objectp->getPositionGlobal() - point_global) << llendl;
	}

	return intersect;
}

void LLViewerWindow::pickAsync(S32 x, S32 y_from_bot, MASK mask, void (*callback)(const LLPickInfo& info), BOOL pick_transparent)
{
	if (gNoRender)
	{
		return;
	}
	
	BOOL in_build_mode = LLFloaterReg::instanceVisible("build");
	if (in_build_mode || LLDrawPoolAlpha::sShowDebugAlpha)
	{
		// build mode allows interaction with all transparent objects
		// "Show Debug Alpha" means no object actually transparent
		pick_transparent = TRUE;
	}

	LLPickInfo pick_info(LLCoordGL(x, y_from_bot), mask, pick_transparent, TRUE, callback);
	schedulePick(pick_info);
}

void LLViewerWindow::schedulePick(LLPickInfo& pick_info)
{
	if (mPicks.size() >= 1024 || mWindow->getMinimized())
	{ //something went wrong, picks are being scheduled but not processed
		
		if (pick_info.mPickCallback)
		{
			pick_info.mPickCallback(pick_info);
		}
	
		return;
	}
	mPicks.push_back(pick_info);
	
	// delay further event processing until we receive results of pick
	// only do this for async picks so that handleMouseUp won't be called
	// until the pick triggered in handleMouseDown has been processed, for example
	mWindow->delayInputProcessing();
}


void LLViewerWindow::performPick()
{
	if (gNoRender)
	{
		return;
	}

	if (!mPicks.empty())
	{
		std::vector<LLPickInfo>::iterator pick_it;
		for (pick_it = mPicks.begin(); pick_it != mPicks.end(); ++pick_it)
		{
			pick_it->fetchResults();
		}

		mLastPick = mPicks.back();
		mPicks.clear();
	}
}

void LLViewerWindow::returnEmptyPicks()
{
	std::vector<LLPickInfo>::iterator pick_it;
	for (pick_it = mPicks.begin(); pick_it != mPicks.end(); ++pick_it)
	{
		mLastPick = *pick_it;
		// just trigger callback with empty results
		if (pick_it->mPickCallback)
		{
			pick_it->mPickCallback(*pick_it);
		}
	}
	mPicks.clear();
}

// Performs the GL object/land pick.
LLPickInfo LLViewerWindow::pickImmediate(S32 x, S32 y_from_bot,  BOOL pick_transparent)
{
	if (gNoRender)
	{
		return LLPickInfo();
	}

	BOOL in_build_mode = LLFloaterReg::instanceVisible("build");
	if (in_build_mode || LLDrawPoolAlpha::sShowDebugAlpha)
	{
		// build mode allows interaction with all transparent objects
		// "Show Debug Alpha" means no object actually transparent
		pick_transparent = TRUE;
	}

	// shortcut queueing in mPicks and just update mLastPick in place
	mLastPick = LLPickInfo(LLCoordGL(x, y_from_bot), gKeyboard->currentMask(TRUE), pick_transparent, TRUE, NULL);
	mLastPick.fetchResults();

	return mLastPick;
}

LLHUDIcon* LLViewerWindow::cursorIntersectIcon(S32 mouse_x, S32 mouse_y, F32 depth,
										   LLVector3* intersection)
{
	S32 x = mouse_x;
	S32 y = mouse_y;

	if ((mouse_x == -1) && (mouse_y == -1)) // use current mouse position
	{
		x = getCurrentMouseX();
		y = getCurrentMouseY();
	}

	// world coordinates of mouse
	LLVector3 mouse_direction_global = mouseDirectionGlobal(x,y);
	LLVector3 mouse_point_global = LLViewerCamera::getInstance()->getOrigin();
	LLVector3 mouse_world_start = mouse_point_global;
	LLVector3 mouse_world_end   = mouse_point_global + mouse_direction_global * depth;

	return LLHUDIcon::lineSegmentIntersectAll(mouse_world_start, mouse_world_end, intersection);

	
}

LLViewerObject* LLViewerWindow::cursorIntersect(S32 mouse_x, S32 mouse_y, F32 depth,
												LLViewerObject *this_object,
												S32 this_face,
												BOOL pick_transparent,
												S32* face_hit,
												LLVector3 *intersection,
												LLVector2 *uv,
												LLVector3 *normal,
												LLVector3 *binormal)
{
	S32 x = mouse_x;
	S32 y = mouse_y;

	if ((mouse_x == -1) && (mouse_y == -1)) // use current mouse position
	{
		x = getCurrentMouseX();
		y = getCurrentMouseY();
	}

	// HUD coordinates of mouse
	LLVector3 mouse_point_hud = mousePointHUD(x, y);
	LLVector3 mouse_hud_start = mouse_point_hud - LLVector3(depth, 0, 0);
	LLVector3 mouse_hud_end   = mouse_point_hud + LLVector3(depth, 0, 0);
	
	// world coordinates of mouse
	LLVector3 mouse_direction_global = mouseDirectionGlobal(x,y);
	LLVector3 mouse_point_global = LLViewerCamera::getInstance()->getOrigin();
	
	//get near clip plane
	LLVector3 n = LLViewerCamera::getInstance()->getAtAxis();
	LLVector3 p = mouse_point_global + n * LLViewerCamera::getInstance()->getNear();

	//project mouse point onto plane
	LLVector3 pos;
	line_plane(mouse_point_global, mouse_direction_global, p, n, pos);
	mouse_point_global = pos;

	LLVector3 mouse_world_start = mouse_point_global;
	LLVector3 mouse_world_end   = mouse_point_global + mouse_direction_global * depth;

	
	LLViewerObject* found = NULL;

	if (this_object)  // check only this object
	{
		if (this_object->isHUDAttachment()) // is a HUD object?
		{
			if (this_object->lineSegmentIntersect(mouse_hud_start, mouse_hud_end, this_face, pick_transparent,
												  face_hit, intersection, uv, normal, binormal))
			{
				found = this_object;
			}
			}
		
		else // is a world object
		{
			if (this_object->lineSegmentIntersect(mouse_world_start, mouse_world_end, this_face, pick_transparent,
												  face_hit, intersection, uv, normal, binormal))
			{
				found = this_object;
			}
			}
			}

	else // check ALL objects
			{
		found = gPipeline.lineSegmentIntersectInHUD(mouse_hud_start, mouse_hud_end, pick_transparent,
													face_hit, intersection, uv, normal, binormal);

		if (!found) // if not found in HUD, look in world:

			{
			found = gPipeline.lineSegmentIntersectInWorld(mouse_world_start, mouse_world_end, pick_transparent,
														  face_hit, intersection, uv, normal, binormal);
			}

	}

	return found;
}

// Returns unit vector relative to camera
// indicating direction of point on screen x,y
LLVector3 LLViewerWindow::mouseDirectionGlobal(const S32 x, const S32 y) const
{
	// find vertical field of view
	F32			fov = LLViewerCamera::getInstance()->getView();

	// find world view center in scaled ui coordinates
	F32			center_x = getWorldViewRectScaled().getCenterX();
	F32			center_y = getWorldViewRectScaled().getCenterY();

	// calculate pixel distance to screen
	F32			distance = ((F32)getWorldViewHeightScaled() * 0.5f) / (tan(fov / 2.f));

	// calculate click point relative to middle of screen
	F32			click_x = x - center_x;
	F32			click_y = y - center_y;

	// compute mouse vector
	LLVector3	mouse_vector =	distance * LLViewerCamera::getInstance()->getAtAxis()
								- click_x * LLViewerCamera::getInstance()->getLeftAxis()
								+ click_y * LLViewerCamera::getInstance()->getUpAxis();

	mouse_vector.normVec();

	return mouse_vector;
}

LLVector3 LLViewerWindow::mousePointHUD(const S32 x, const S32 y) const
{
	// find screen resolution
	S32			height = getWorldViewHeightScaled();

	// find world view center
	F32			center_x = getWorldViewRectScaled().getCenterX();
	F32			center_y = getWorldViewRectScaled().getCenterY();

	// remap with uniform scale (1/height) so that top is -0.5, bottom is +0.5
	F32 hud_x = -((F32)x - center_x)  / height;
	F32 hud_y = ((F32)y - center_y) / height;

	return LLVector3(0.f, hud_x/gAgent.mHUDCurZoom, hud_y/gAgent.mHUDCurZoom);
}

// Returns unit vector relative to camera in camera space
// indicating direction of point on screen x,y
LLVector3 LLViewerWindow::mouseDirectionCamera(const S32 x, const S32 y) const
{
	// find vertical field of view
	F32			fov_height = LLViewerCamera::getInstance()->getView();
	F32			fov_width = fov_height * LLViewerCamera::getInstance()->getAspect();

	// find screen resolution
	S32			height = getWorldViewHeightScaled();
	S32			width = getWorldViewWidthScaled();

	// find world view center
	F32			center_x = getWorldViewRectScaled().getCenterX();
	F32			center_y = getWorldViewRectScaled().getCenterY();

	// calculate click point relative to middle of screen
	F32			click_x = (((F32)x - center_x) / (F32)width) * fov_width * -1.f;
	F32			click_y = (((F32)y - center_y) / (F32)height) * fov_height;

	// compute mouse vector
	LLVector3	mouse_vector =	LLVector3(0.f, 0.f, -1.f);
	LLQuaternion mouse_rotate;
	mouse_rotate.setQuat(click_y, click_x, 0.f);

	mouse_vector = mouse_vector * mouse_rotate;
	// project to z = -1 plane;
	mouse_vector = mouse_vector * (-1.f / mouse_vector.mV[VZ]);

	return mouse_vector;
}



BOOL LLViewerWindow::mousePointOnPlaneGlobal(LLVector3d& point, const S32 x, const S32 y, 
										const LLVector3d &plane_point_global, 
										const LLVector3 &plane_normal_global)
{
	LLVector3d	mouse_direction_global_d;

	mouse_direction_global_d.setVec(mouseDirectionGlobal(x,y));
	LLVector3d	plane_normal_global_d;
	plane_normal_global_d.setVec(plane_normal_global);
	F64 plane_mouse_dot = (plane_normal_global_d * mouse_direction_global_d);
	LLVector3d plane_origin_camera_rel = plane_point_global - gAgent.getCameraPositionGlobal();
	F64	mouse_look_at_scale = (plane_normal_global_d * plane_origin_camera_rel)
								/ plane_mouse_dot;
	if (llabs(plane_mouse_dot) < 0.00001)
	{
		// if mouse is parallel to plane, return closest point on line through plane origin
		// that is parallel to camera plane by scaling mouse direction vector
		// by distance to plane origin, modulated by deviation of mouse direction from plane origin
		LLVector3d plane_origin_dir = plane_origin_camera_rel;
		plane_origin_dir.normVec();
		
		mouse_look_at_scale = plane_origin_camera_rel.magVec() / (plane_origin_dir * mouse_direction_global_d);
	}

	point = gAgent.getCameraPositionGlobal() + mouse_look_at_scale * mouse_direction_global_d;

	return mouse_look_at_scale > 0.0;
}


// Returns global position
BOOL LLViewerWindow::mousePointOnLandGlobal(const S32 x, const S32 y, LLVector3d *land_position_global)
{
	LLVector3		mouse_direction_global = mouseDirectionGlobal(x,y);
	F32				mouse_dir_scale;
	BOOL			hit_land = FALSE;
	LLViewerRegion	*regionp;
	F32			land_z;
	const F32	FIRST_PASS_STEP = 1.0f;		// meters
	const F32	SECOND_PASS_STEP = 0.1f;	// meters
	LLVector3d	camera_pos_global;

	camera_pos_global = gAgent.getCameraPositionGlobal();
	LLVector3d		probe_point_global;
	LLVector3		probe_point_region;

	// walk forwards to find the point
	for (mouse_dir_scale = FIRST_PASS_STEP; mouse_dir_scale < gAgent.mDrawDistance; mouse_dir_scale += FIRST_PASS_STEP)
	{
		LLVector3d mouse_direction_global_d;
		mouse_direction_global_d.setVec(mouse_direction_global * mouse_dir_scale);
		probe_point_global = camera_pos_global + mouse_direction_global_d;

		regionp = LLWorld::getInstance()->resolveRegionGlobal(probe_point_region, probe_point_global);

		if (!regionp)
		{
			// ...we're outside the world somehow
			continue;
		}

		S32 i = (S32) (probe_point_region.mV[VX]/regionp->getLand().getMetersPerGrid());
		S32 j = (S32) (probe_point_region.mV[VY]/regionp->getLand().getMetersPerGrid());
		S32 grids_per_edge = (S32) regionp->getLand().mGridsPerEdge;
		if ((i >= grids_per_edge) || (j >= grids_per_edge))
		{
			//llinfos << "LLViewerWindow::mousePointOnLand probe_point is out of region" << llendl;
			continue;
		}

		land_z = regionp->getLand().resolveHeightRegion(probe_point_region);

		//llinfos << "mousePointOnLand initial z " << land_z << llendl;

		if (probe_point_region.mV[VZ] < land_z)
		{
			// ...just went under land

			// cout << "under land at " << probe_point << " scale " << mouse_vec_scale << endl;

			hit_land = TRUE;
			break;
		}
	}


	if (hit_land)
	{
		// Don't go more than one step beyond where we stopped above.
		// This can't just be "mouse_vec_scale" because floating point error
		// will stop the loop before the last increment.... X - 1.0 + 0.1 + 0.1 + ... + 0.1 != X
		F32 stop_mouse_dir_scale = mouse_dir_scale + FIRST_PASS_STEP;

		// take a step backwards, then walk forwards again to refine position
		for ( mouse_dir_scale -= FIRST_PASS_STEP; mouse_dir_scale <= stop_mouse_dir_scale; mouse_dir_scale += SECOND_PASS_STEP)
		{
			LLVector3d mouse_direction_global_d;
			mouse_direction_global_d.setVec(mouse_direction_global * mouse_dir_scale);
			probe_point_global = camera_pos_global + mouse_direction_global_d;

			regionp = LLWorld::getInstance()->resolveRegionGlobal(probe_point_region, probe_point_global);

			if (!regionp)
			{
				// ...we're outside the world somehow
				continue;
			}

			/*
			i = (S32) (local_probe_point.mV[VX]/regionp->getLand().getMetersPerGrid());
			j = (S32) (local_probe_point.mV[VY]/regionp->getLand().getMetersPerGrid());
			if ((i >= regionp->getLand().mGridsPerEdge) || (j >= regionp->getLand().mGridsPerEdge))
			{
				// llinfos << "LLViewerWindow::mousePointOnLand probe_point is out of region" << llendl;
				continue;
			}
			land_z = regionp->getLand().mSurfaceZ[ i + j * (regionp->getLand().mGridsPerEdge) ];
			*/

			land_z = regionp->getLand().resolveHeightRegion(probe_point_region);

			//llinfos << "mousePointOnLand refine z " << land_z << llendl;

			if (probe_point_region.mV[VZ] < land_z)
			{
				// ...just went under land again

				*land_position_global = probe_point_global;
				return TRUE;
			}
		}
	}

	return FALSE;
}

// Saves an image to the harddrive as "SnapshotX" where X >= 1.
BOOL LLViewerWindow::saveImageNumbered(LLImageFormatted *image)
{
	if (!image)
	{
		return FALSE;
	}

	LLFilePicker::ESaveFilter pick_type;
	std::string extension("." + image->getExtension());
	if (extension == ".j2c")
		pick_type = LLFilePicker::FFSAVE_J2C;
	else if (extension == ".bmp")
		pick_type = LLFilePicker::FFSAVE_BMP;
	else if (extension == ".jpg")
		pick_type = LLFilePicker::FFSAVE_JPEG;
	else if (extension == ".png")
		pick_type = LLFilePicker::FFSAVE_PNG;
	else if (extension == ".tga")
		pick_type = LLFilePicker::FFSAVE_TGA;
	else
		pick_type = LLFilePicker::FFSAVE_ALL; // ???
	
	// Get a base file location if needed.
	if ( ! isSnapshotLocSet())		
	{
		std::string proposed_name( sSnapshotBaseName );

		// getSaveFile will append an appropriate extension to the proposed name, based on the ESaveFilter constant passed in.

		// pick a directory in which to save
		LLFilePicker& picker = LLFilePicker::instance();
		if (!picker.getSaveFile(pick_type, proposed_name))
		{
			// Clicked cancel
			return FALSE;
		}

		// Copy the directory + file name
		std::string filepath = picker.getFirstFile();

		LLViewerWindow::sSnapshotBaseName = gDirUtilp->getBaseFileName(filepath, true);
		LLViewerWindow::sSnapshotDir = gDirUtilp->getDirName(filepath);
	}

	// Look for an unused file name
	std::string filepath;
	S32 i = 1;
	S32 err = 0;

	do
	{
		filepath = sSnapshotDir;
		filepath += gDirUtilp->getDirDelimiter();
		filepath += sSnapshotBaseName;
		filepath += llformat("_%.3d",i);
		filepath += extension;

		llstat stat_info;
		err = LLFile::stat( filepath, &stat_info );
		i++;
	}
	while( -1 != err );  // search until the file is not found (i.e., stat() gives an error).

	return image->save(filepath);
}

void LLViewerWindow::resetSnapshotLoc()
{
	sSnapshotDir.clear();
}

static S32 BORDERHEIGHT = 0;
static S32 BORDERWIDTH = 0;

// static
void LLViewerWindow::movieSize(S32 new_width, S32 new_height)
{
	LLCoordScreen size;
	gViewerWindow->mWindow->getSize(&size);
	if (  (size.mX != new_width + BORDERWIDTH)
		||(size.mY != new_height + BORDERHEIGHT))
	{
		// use actual display dimensions, not virtual UI dimensions
		S32 x = gViewerWindow->getWindowWidthRaw();
		S32 y = gViewerWindow->getWindowHeightRaw();
		BORDERWIDTH = size.mX - x;
		BORDERHEIGHT = size.mY- y;
		LLCoordScreen new_size(new_width + BORDERWIDTH, 
							   new_height + BORDERHEIGHT);
		BOOL disable_sync = gSavedSettings.getBOOL("DisableVerticalSync");
		if (gViewerWindow->mWindow->getFullscreen())
		{
			gViewerWindow->changeDisplaySettings(FALSE, 
												new_size, 
												disable_sync, 
												TRUE);
		}
		else
		{
			gViewerWindow->mWindow->setSize(new_size);
		}
	}
}

BOOL LLViewerWindow::saveSnapshot( const std::string& filepath, S32 image_width, S32 image_height, BOOL show_ui, BOOL do_rebuild, ESnapshotType type)
{
	llinfos << "Saving snapshot to: " << filepath << llendl;

	LLPointer<LLImageRaw> raw = new LLImageRaw;
	BOOL success = rawSnapshot(raw, image_width, image_height, TRUE, FALSE, show_ui, do_rebuild);

	if (success)
	{
		LLPointer<LLImageBMP> bmp_image = new LLImageBMP;
		success = bmp_image->encode(raw, 0.0f);
		if( success )
		{
			success = bmp_image->save(filepath);
		}
		else
		{
			llwarns << "Unable to encode bmp snapshot" << llendl;
		}
	}
	else
	{
		llwarns << "Unable to capture raw snapshot" << llendl;
	}

	return success;
}


void LLViewerWindow::playSnapshotAnimAndSound()
{
	if (gSavedSettings.getBOOL("QuietSnapshotsToDisk"))
	{
		return;
	}
	gAgent.sendAnimationRequest(ANIM_AGENT_SNAPSHOT, ANIM_REQUEST_START);
	send_sound_trigger(LLUUID(gSavedSettings.getString("UISndSnapshot")), 1.0f);
}

BOOL LLViewerWindow::thumbnailSnapshot(LLImageRaw *raw, S32 preview_width, S32 preview_height, BOOL show_ui, BOOL do_rebuild, ESnapshotType type)
{
	return rawSnapshot(raw, preview_width, preview_height, FALSE, FALSE, show_ui, do_rebuild, type);
	
	// *TODO below code was broken in deferred pipeline
	/*
	if ((!raw) || preview_width < 10 || preview_height < 10)
	{
		return FALSE;
	}

	if(gResizeScreenTexture) //the window is resizing
	{
		return FALSE ;
	}

	setCursor(UI_CURSOR_WAIT);

	// Hide all the UI widgets first and draw a frame
	BOOL prev_draw_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);

	if ( prev_draw_ui != show_ui)
	{
		LLPipeline::toggleRenderDebugFeature((void*)LLPipeline::RENDER_DEBUG_FEATURE_UI);
	}

	BOOL hide_hud = !gSavedSettings.getBOOL("RenderHUDInSnapshot") && LLPipeline::sShowHUDAttachments;
	if (hide_hud)
	{
		LLPipeline::sShowHUDAttachments = FALSE;
	}

	S32 render_name = gSavedSettings.getS32("RenderName");
	gSavedSettings.setS32("RenderName", 0);
	LLVOAvatar::updateFreezeCounter(1) ; //pause avatar updating for one frame
	
	S32 w = preview_width ;
	S32 h = preview_height ;
	LLVector2 display_scale = mDisplayScale ;
	mDisplayScale.setVec((F32)w / mWindowRectRaw.getWidth(), (F32)h / mWindowRectRaw.getHeight()) ;
	LLRect window_rect = mWindowRectRaw;
	mWindowRectRaw.set(0, h, w, 0);
	
	gDisplaySwapBuffers = FALSE;
	gDepthDirty = TRUE;
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	setup3DRender();

	LLFontGL::setFontDisplay(FALSE) ;
	LLHUDText::setDisplayText(FALSE) ;
	if (type == SNAPSHOT_TYPE_OBJECT_ID)
	{
		gObjectList.renderPickList(gViewerWindow->getWindowRectScaled(), FALSE, FALSE);
	}
	else
	{
		display(do_rebuild, 1.0f, 0, TRUE);
		render_ui();
	}

	S32 glformat, gltype, glpixel_length ;
	if(SNAPSHOT_TYPE_DEPTH == type)
	{
		glpixel_length = 4 ;
		glformat = GL_DEPTH_COMPONENT ; 
		gltype = GL_FLOAT ;
	}
	else
	{
		glpixel_length = 3 ;
		glformat = GL_RGB ;
		gltype = GL_UNSIGNED_BYTE ;
	}

	raw->resize(w, h, glpixel_length);
	glReadPixels(0, 0, w, h, glformat, gltype, raw->getData());

	if(SNAPSHOT_TYPE_DEPTH == type)
	{
		LLViewerCamera* camerap = LLViewerCamera::getInstance();
		F32 depth_conversion_factor_1 = (camerap->getFar() + camerap->getNear()) / (2.f * camerap->getFar() * camerap->getNear());
		F32 depth_conversion_factor_2 = (camerap->getFar() - camerap->getNear()) / (2.f * camerap->getFar() * camerap->getNear());

		//calculate the depth 
		for (S32 y = 0 ; y < h ; y++)
		{
			for(S32 x = 0 ; x < w ; x++)
			{
				S32 i = (w * y + x) << 2 ;
				
				F32 depth_float_i = *(F32*)(raw->getData() + i);
				
				F32 linear_depth_float = 1.f / (depth_conversion_factor_1 - (depth_float_i * depth_conversion_factor_2));
				U8 depth_byte = F32_to_U8(linear_depth_float, camerap->getNear(), camerap->getFar());
				*(raw->getData() + i + 0) = depth_byte;
				*(raw->getData() + i + 1) = depth_byte;
				*(raw->getData() + i + 2) = depth_byte;
				*(raw->getData() + i + 3) = 255;
			}
		}		
	}

	LLFontGL::setFontDisplay(TRUE) ;
	LLHUDText::setDisplayText(TRUE) ;
	mDisplayScale.setVec(display_scale) ;
	mWindowRectRaw = window_rect;	
	setup3DRender();
	gDisplaySwapBuffers = FALSE;
	gDepthDirty = TRUE;

	// POST SNAPSHOT
	if (!gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
	{
		LLPipeline::toggleRenderDebugFeature((void*)LLPipeline::RENDER_DEBUG_FEATURE_UI);
	}

	if (hide_hud)
	{
		LLPipeline::sShowHUDAttachments = TRUE;
	}

	setCursor(UI_CURSOR_ARROW);

	if (do_rebuild)
	{
		// If we had to do a rebuild, that means that the lists of drawables to be rendered
		// was empty before we started.
		// Need to reset these, otherwise we call state sort on it again when render gets called the next time
		// and we stand a good chance of crashing on rebuild because the render drawable arrays have multiple copies of
		// objects on them.
		gPipeline.resetDrawOrders();
	}
	
	gSavedSettings.setS32("RenderName", render_name);	
	
	return TRUE;*/
}

// Saves the image from the screen to the specified filename and path.
BOOL LLViewerWindow::rawSnapshot(LLImageRaw *raw, S32 image_width, S32 image_height, 
								 BOOL keep_window_aspect, BOOL is_texture, BOOL show_ui, BOOL do_rebuild, ESnapshotType type, S32 max_size)
{
	if (!raw)
	{
		return FALSE;
	}

	// PRE SNAPSHOT
	gDisplaySwapBuffers = FALSE;
	
	// if not showing ui, use full window to render world view
	updateWorldViewRect(!show_ui);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	setCursor(UI_CURSOR_WAIT);

	// Hide all the UI widgets first and draw a frame
	BOOL prev_draw_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);

	if ( prev_draw_ui != show_ui)
	{
		LLPipeline::toggleRenderDebugFeature((void*)LLPipeline::RENDER_DEBUG_FEATURE_UI);
	}

	BOOL hide_hud = !gSavedSettings.getBOOL("RenderHUDInSnapshot") && LLPipeline::sShowHUDAttachments;
	if (hide_hud)
	{
		LLPipeline::sShowHUDAttachments = FALSE;
	}

	// Copy screen to a buffer
	// crop sides or top and bottom, if taking a snapshot of different aspect ratio
	// from window
	S32 snapshot_width = mWindowRectRaw.getWidth();
	S32 snapshot_height =  mWindowRectRaw.getHeight();
	// SNAPSHOT
	S32 window_width = mWindowRectRaw.getWidth();
	S32 window_height = mWindowRectRaw.getHeight();	
	LLRect window_rect = mWindowRectRaw;
	BOOL use_fbo = FALSE;

	LLRenderTarget target;
	F32 scale_factor = 1.0f ;
	if(!keep_window_aspect) //image cropping
	{		
		F32 ratio = llmin( (F32)window_width / image_width , (F32)window_height / image_height) ;
		snapshot_width = (S32)(ratio * image_width) ;
		snapshot_height = (S32)(ratio * image_height) ;
		scale_factor = llmax(1.0f, 1.0f / ratio) ;
	}
	else //the scene(window) proportion needs to be maintained.
	{
		if(image_width > window_width || image_height > window_height) //need to enlarge the scene
		{
			if (!LLPipeline::sRenderDeferred && gGLManager.mHasFramebufferObject && !show_ui)
			{
				GLint max_size = 0;
				glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &max_size);
		
				if (image_width <= max_size && image_height <= max_size) //re-project the scene
				{
					use_fbo = TRUE;
					
					snapshot_width = image_width;
					snapshot_height = image_height;
					target.allocate(snapshot_width, snapshot_height, GL_RGBA, TRUE, TRUE, LLTexUnit::TT_RECT_TEXTURE, TRUE);
					window_width = snapshot_width;
					window_height = snapshot_height;
					scale_factor = 1.f;
					mWindowRectRaw.set(0, snapshot_height, snapshot_width, 0);
					target.bindTarget();			
				}
			}

			if(!use_fbo) //no re-projection, so tiling the scene
			{
				F32 ratio = llmin( (F32)window_width / image_width , (F32)window_height / image_height) ;
				snapshot_width = (S32)(ratio * image_width) ;
				snapshot_height = (S32)(ratio * image_height) ;
				scale_factor = llmax(1.0f, 1.0f / ratio) ;	
			}
		}
		//else: keep the current scene scale, re-scale it if necessary after reading out.
	}
	
	S32 buffer_x_offset = llfloor(((window_width - snapshot_width) * scale_factor) / 2.f);
	S32 buffer_y_offset = llfloor(((window_height - snapshot_height) * scale_factor) / 2.f);

	S32 image_buffer_x = llfloor(snapshot_width*scale_factor) ;
	S32 image_buffer_y = llfloor(snapshot_height *scale_factor) ;
	if(image_buffer_x > max_size || image_buffer_y > max_size) //boundary check to avoid memory overflow
	{
		scale_factor *= llmin((F32)max_size / image_buffer_x, (F32)max_size / image_buffer_y) ;
		image_buffer_x = llfloor(snapshot_width*scale_factor) ;
		image_buffer_y = llfloor(snapshot_height *scale_factor) ;
	}
	raw->resize(image_buffer_x, image_buffer_y, 3);
	if(raw->isBufferInvalid())
	{
		return FALSE ;
	}

	BOOL high_res = scale_factor >= 2.f; // Font scaling is slow, only do so if rez is much higher
	if (high_res)
	{
		send_agent_pause();
		//rescale fonts
		initFonts(scale_factor);
		LLHUDText::reshape();
	}

	S32 output_buffer_offset_y = 0;

	F32 depth_conversion_factor_1 = (LLViewerCamera::getInstance()->getFar() + LLViewerCamera::getInstance()->getNear()) / (2.f * LLViewerCamera::getInstance()->getFar() * LLViewerCamera::getInstance()->getNear());
	F32 depth_conversion_factor_2 = (LLViewerCamera::getInstance()->getFar() - LLViewerCamera::getInstance()->getNear()) / (2.f * LLViewerCamera::getInstance()->getFar() * LLViewerCamera::getInstance()->getNear());

	gObjectList.generatePickList(*LLViewerCamera::getInstance());

	for (int subimage_y = 0; subimage_y < scale_factor; ++subimage_y)
	{
		S32 subimage_y_offset = llclamp(buffer_y_offset - (subimage_y * window_height), 0, window_height);;
		// handle fractional columns
		U32 read_height = llmax(0, (window_height - subimage_y_offset) -
			llmax(0, (window_height * (subimage_y + 1)) - (buffer_y_offset + raw->getHeight())));

		S32 output_buffer_offset_x = 0;
		for (int subimage_x = 0; subimage_x < scale_factor; ++subimage_x)
		{
			gDisplaySwapBuffers = FALSE;
			gDepthDirty = TRUE;
			if (type == SNAPSHOT_TYPE_OBJECT_ID)
			{
				glClearColor(0.f, 0.f, 0.f, 0.f);
				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

				LLViewerCamera::getInstance()->setZoomParameters(scale_factor, subimage_x+(subimage_y*llceil(scale_factor)));
				setup3DRender();
				gObjectList.renderPickList(gViewerWindow->getWindowRectScaled(), FALSE, FALSE);
			}
			else
			{
				const U32 subfield = subimage_x+(subimage_y*llceil(scale_factor));

				if (LLPipeline::sRenderDeferred)
				{
					display(do_rebuild, scale_factor, subfield, FALSE);
				}
				else
				{
					display(do_rebuild, scale_factor, subfield, TRUE);
					// Required for showing the GUI in snapshots?  See DEV-16350 for details. JC
					render_ui(scale_factor, subfield);
				}
			}

			S32 subimage_x_offset = llclamp(buffer_x_offset - (subimage_x * window_width), 0, window_width);
			// handle fractional rows
			U32 read_width = llmax(0, (window_width - subimage_x_offset) -
									llmax(0, (window_width * (subimage_x + 1)) - (buffer_x_offset + raw->getWidth())));
			for(U32 out_y = 0; out_y < read_height ; out_y++)
			{
				S32 output_buffer_offset = ( 
							(out_y * (raw->getWidth())) // ...plus iterated y...
							+ (window_width * subimage_x) // ...plus subimage start in x...
							+ (raw->getWidth() * window_height * subimage_y) // ...plus subimage start in y...
							- output_buffer_offset_x // ...minus buffer padding x...
							- (output_buffer_offset_y * (raw->getWidth()))  // ...minus buffer padding y...
						) * raw->getComponents();
				
				// Ping the wathdog thread every 100 lines to keep us alive (arbitrary number, feel free to change)
				if (out_y % 100 == 0)
				{
					LLAppViewer::instance()->pingMainloopTimeout("LLViewerWindow::rawSnapshot");
				}
				
				if (type == SNAPSHOT_TYPE_OBJECT_ID || type == SNAPSHOT_TYPE_COLOR)
				{
					glReadPixels(
						subimage_x_offset, out_y + subimage_y_offset,
						read_width, 1,
						GL_RGB, GL_UNSIGNED_BYTE,
						raw->getData() + output_buffer_offset
					);
				}
				else // SNAPSHOT_TYPE_DEPTH
				{
					LLPointer<LLImageRaw> depth_line_buffer = new LLImageRaw(read_width, 1, sizeof(GL_FLOAT)); // need to store floating point values
					glReadPixels(
						subimage_x_offset, out_y + subimage_y_offset,
						read_width, 1,
						GL_DEPTH_COMPONENT, GL_FLOAT,
						depth_line_buffer->getData()// current output pixel is beginning of buffer...
					);

					for (S32 i = 0; i < (S32)read_width; i++)
					{
						F32 depth_float = *(F32*)(depth_line_buffer->getData() + (i * sizeof(F32)));
					
						F32 linear_depth_float = 1.f / (depth_conversion_factor_1 - (depth_float * depth_conversion_factor_2));
						U8 depth_byte = F32_to_U8(linear_depth_float, LLViewerCamera::getInstance()->getNear(), LLViewerCamera::getInstance()->getFar());
						//write converted scanline out to result image
						for(S32 j = 0; j < raw->getComponents(); j++)
						{
							*(raw->getData() + output_buffer_offset + (i * raw->getComponents()) + j) = depth_byte;
						}
					}
				}
			}
			output_buffer_offset_x += subimage_x_offset;
			stop_glerror();
		}
		output_buffer_offset_y += subimage_y_offset;
	}

	if (use_fbo)
	{
		mWindowRectRaw = window_rect;
		target.flush();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	gDisplaySwapBuffers = FALSE;
	gDepthDirty = TRUE;

	// POST SNAPSHOT
	if (!gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
	{
		LLPipeline::toggleRenderDebugFeature((void*)LLPipeline::RENDER_DEBUG_FEATURE_UI);
	}

	if (hide_hud)
	{
		LLPipeline::sShowHUDAttachments = TRUE;
	}

	if (high_res)
	{
		initFonts(1.f);
		LLHUDText::reshape();
	}

	// Pre-pad image to number of pixels such that the line length is a multiple of 4 bytes (for BMP encoding)
	// Note: this formula depends on the number of components being 3.  Not obvious, but it's correct.	
	image_width += (image_width * 3) % 4;

	BOOL ret = TRUE ;
	// Resize image
	if(llabs(image_width - image_buffer_x) > 4 || llabs(image_height - image_buffer_y) > 4)
	{
		ret = raw->scale( image_width, image_height );  
	}
	else if(image_width != image_buffer_x || image_height != image_buffer_y)
	{
		ret = raw->scale( image_width, image_height, FALSE );  
	}
	

	setCursor(UI_CURSOR_ARROW);

	if (do_rebuild)
	{
		// If we had to do a rebuild, that means that the lists of drawables to be rendered
		// was empty before we started.
		// Need to reset these, otherwise we call state sort on it again when render gets called the next time
		// and we stand a good chance of crashing on rebuild because the render drawable arrays have multiple copies of
		// objects on them.
		gPipeline.resetDrawOrders();
	}

	if (high_res)
	{
		send_agent_resume();
	}

	return ret;
}

void LLViewerWindow::destroyWindow()
{
	if (mWindow)
	{
		LLWindowManager::destroyWindow(mWindow);
	}
	mWindow = NULL;
}


void LLViewerWindow::drawMouselookInstructions()
{
	// Draw instructions for mouselook ("Press ESC to return to World View" partially transparent at the bottom of the screen.)
	const std::string instructions = LLTrans::getString("LeaveMouselook");
	const LLFontGL* font = LLFontGL::getFont(LLFontDescriptor("SansSerif", "Large", LLFontGL::BOLD));
	
	//to be on top of Bottom bar when it is opened
	const S32 INSTRUCTIONS_PAD = 50;

	font->renderUTF8( 
		instructions, 0,
		getWorldViewRectScaled().getCenterX(),
		getWorldViewRectScaled().mBottom + INSTRUCTIONS_PAD,
		LLColor4( 1.0f, 1.0f, 1.0f, 0.5f ),
		LLFontGL::HCENTER, LLFontGL::TOP,
		LLFontGL::NORMAL,LLFontGL::DROP_SHADOW);
}

void* LLViewerWindow::getPlatformWindow() const
{
	return mWindow->getPlatformWindow();
}

void* LLViewerWindow::getMediaWindow() 	const
{
	return mWindow->getMediaWindow();
}

void LLViewerWindow::focusClient()		const
{
	return mWindow->focusClient();
}

LLRootView*	LLViewerWindow::getRootView() const
{
	return mRootView;
}

LLRect LLViewerWindow::getWorldViewRectScaled() const
{
	return mWorldViewRectScaled;
}

S32 LLViewerWindow::getWorldViewHeightScaled() const
{
	return mWorldViewRectScaled.getHeight();
}

S32 LLViewerWindow::getWorldViewWidthScaled() const
{
	return mWorldViewRectScaled.getWidth();
}


S32 LLViewerWindow::getWorldViewHeightRaw() const
{
	return mWorldViewRectRaw.getHeight(); 
}

S32 LLViewerWindow::getWorldViewWidthRaw() const
{
	return mWorldViewRectRaw.getWidth(); 
}

S32	LLViewerWindow::getWindowHeightScaled()	const 	
{ 
	return mWindowRectScaled.getHeight(); 
}

S32	LLViewerWindow::getWindowWidthScaled() const 	
{ 
	return mWindowRectScaled.getWidth(); 
}

S32	LLViewerWindow::getWindowHeightRaw()	const 	
{ 
	return mWindowRectRaw.getHeight(); 
}

S32	LLViewerWindow::getWindowWidthRaw() const 	
{ 
	return mWindowRectRaw.getWidth(); 
}

void LLViewerWindow::setup2DRender()
{
	// setup ortho camera
	gl_state_for_2d(mWindowRectRaw.getWidth(), mWindowRectRaw.getHeight());
	setup2DViewport();
}

void LLViewerWindow::setup2DViewport(S32 x_offset, S32 y_offset)
{
	gGLViewport[0] = mWindowRectRaw.mLeft + x_offset;
	gGLViewport[1] = mWindowRectRaw.mBottom + y_offset;
	gGLViewport[2] = mWindowRectRaw.getWidth();
	gGLViewport[3] = mWindowRectRaw.getHeight();
	glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
}


void LLViewerWindow::setup3DRender()
{
	// setup perspective camera
	LLViewerCamera::getInstance()->setPerspective(NOT_FOR_SELECTION, mWorldViewRectRaw.mLeft, mWorldViewRectRaw.mBottom,  mWorldViewRectRaw.getWidth(), mWorldViewRectRaw.getHeight(), FALSE, LLViewerCamera::getInstance()->getNear(), MAX_FAR_CLIP*2.f);
	setup3DViewport();
}

void LLViewerWindow::setup3DViewport(S32 x_offset, S32 y_offset)
{
	if (LLRenderTarget::getCurrentBoundTarget() != NULL)
	{
		// don't use translation component of mWorldViewRectRaw, as we are already in a properly sized render target
		gGLViewport[0] = x_offset;
		gGLViewport[1] = y_offset;
	}
	else
	{
		gGLViewport[0] = mWorldViewRectRaw.mLeft + x_offset;
		gGLViewport[1] = mWorldViewRectRaw.mBottom + y_offset;
	}
	gGLViewport[2] = mWorldViewRectRaw.getWidth();
	gGLViewport[3] = mWorldViewRectRaw.getHeight();
	glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
}

void LLViewerWindow::setShowProgress(const BOOL show)
{
	if (mProgressView)
	{
		mProgressView->setVisible(show);
	}
}

BOOL LLViewerWindow::getShowProgress() const
{
	return (mProgressView && mProgressView->getVisible());
}

void LLViewerWindow::moveProgressViewToFront()
{
	if( mProgressView && mRootView )
	{
		mRootView->sendChildToFront(mProgressView);
	}
}

void LLViewerWindow::setProgressString(const std::string& string)
{
	if (mProgressView)
	{
		mProgressView->setText(string);
	}
}

void LLViewerWindow::setProgressMessage(const std::string& msg)
{
	if(mProgressView)
	{
		mProgressView->setMessage(msg);
	}
}

void LLViewerWindow::setProgressPercent(const F32 percent)
{
	if (mProgressView)
	{
		mProgressView->setPercent(percent);
	}
}

void LLViewerWindow::setProgressCancelButtonVisible( BOOL b, const std::string& label )
{
	if (mProgressView)
	{
		mProgressView->setCancelButtonVisible( b, label );
	}
}


LLProgressView *LLViewerWindow::getProgressView() const
{
	return mProgressView;
}

void LLViewerWindow::dumpState()
{
	llinfos << "LLViewerWindow Active " << S32(mActive) << llendl;
	llinfos << "mWindow visible " << S32(mWindow->getVisible())
		<< " minimized " << S32(mWindow->getMinimized())
		<< llendl;
}

void LLViewerWindow::stopGL(BOOL save_state)
{
	//Note: --bao
	//if not necessary, do not change the order of the function calls in this function.
	//if change something, make sure it will not break anything.
	//especially be careful to put anything behind gTextureList.destroyGL(save_state);
	if (!gGLManager.mIsDisabled)
	{
		llinfos << "Shutting down GL..." << llendl;

		// Pause texture decode threads (will get unpaused during main loop)
		LLAppViewer::getTextureCache()->pause();
		LLAppViewer::getImageDecodeThread()->pause();
		LLAppViewer::getTextureFetch()->pause();
				
		gSky.destroyGL();
		stop_glerror();		

		LLManipTranslate::destroyGL() ;
		stop_glerror();		

		gBumpImageList.destroyGL();
		stop_glerror();

		LLFontGL::destroyAllGL();
		stop_glerror();

		LLVOAvatar::destroyGL();
		stop_glerror();

		LLViewerDynamicTexture::destroyGL();
		stop_glerror();

		if (gPipeline.isInit())
		{
			gPipeline.destroyGL();
		}
		
		gCone.cleanupGL();
		gBox.cleanupGL();
		gSphere.cleanupGL();
		gCylinder.cleanupGL();
		
		if(gPostProcess)
		{
			gPostProcess->invalidate();
		}

		gTextureList.destroyGL(save_state);
		stop_glerror();
		
		gGLManager.mIsDisabled = TRUE;
		stop_glerror();
		
		llinfos << "Remaining allocated texture memory: " << LLImageGL::sGlobalTextureMemoryInBytes << " bytes" << llendl;
	}
}

void LLViewerWindow::restoreGL(const std::string& progress_message)
{
	//Note: --bao
	//if not necessary, do not change the order of the function calls in this function.
	//if change something, make sure it will not break anything. 
	//especially, be careful to put something before gTextureList.restoreGL();
	if (gGLManager.mIsDisabled)
	{
		llinfos << "Restoring GL..." << llendl;
		gGLManager.mIsDisabled = FALSE;
		
		initGLDefaults();
		LLGLState::restoreGL();
		
		gTextureList.restoreGL();
		
		// for future support of non-square pixels, and fonts that are properly stretched
		//LLFontGL::destroyDefaultFonts();
		initFonts();
				
		gSky.restoreGL();
		gPipeline.restoreGL();
		LLDrawPoolWater::restoreGL();
		LLManipTranslate::restoreGL();
		
		gBumpImageList.restoreGL();
		LLViewerDynamicTexture::restoreGL();
		LLVOAvatar::restoreGL();
		
		gResizeScreenTexture = TRUE;

		if (gFloaterCustomize && gFloaterCustomize->getVisible())
		{
			LLVisualParamHint::requestHintUpdates();
		}

		if (!progress_message.empty())
		{
			gRestoreGLTimer.reset();
			gRestoreGL = TRUE;
			setShowProgress(TRUE);
			setProgressString(progress_message);
		}
		llinfos << "...Restoring GL done" << llendl;
		if(!LLAppViewer::instance()->restoreErrorTrap())
		{
			llwarns << " Someone took over my signal/exception handler (post restoreGL)!" << llendl;
		}

	}
}

void LLViewerWindow::initFonts(F32 zoom_factor)
{
	LLFontGL::destroyAllGL();
	// Initialize with possibly different zoom factor
	LLFontGL::initClass( gSavedSettings.getF32("FontScreenDPI"),
								mDisplayScale.mV[VX] * zoom_factor,
								mDisplayScale.mV[VY] * zoom_factor,
								gDirUtilp->getAppRODataDir(),
								LLUI::getXUIPaths());
	// Force font reloads, which can be very slow
	LLFontGL::loadDefaultFonts();
}

void LLViewerWindow::toggleFullscreen(BOOL show_progress)
{
	if (mWindow)
	{
		mWantFullscreen = mWindow->getFullscreen() ? FALSE : TRUE;
		mIsFullscreenChecked =  mWindow->getFullscreen() ? FALSE : TRUE;
		mShowFullscreenProgress = show_progress;
	}
}

void LLViewerWindow::getTargetWindow(BOOL& fullscreen, S32& width, S32& height) const
{
	fullscreen = mWantFullscreen;
	
	if (mWindow
	&&  mWindow->getFullscreen() == mWantFullscreen)
	{
		width = getWindowWidthRaw();
		height = getWindowHeightRaw();
	}
	else if (mWantFullscreen)
	{
		width = gSavedSettings.getS32("FullScreenWidth");
		height = gSavedSettings.getS32("FullScreenHeight");
	}
	else
	{
		width = gSavedSettings.getS32("WindowWidth");
		height = gSavedSettings.getS32("WindowHeight");
	}
}

void LLViewerWindow::requestResolutionUpdate()
{
	mResDirty = true;
}

void LLViewerWindow::requestResolutionUpdate(bool fullscreen_checked)
{
	mResDirty = true;
	mWantFullscreen = fullscreen_checked;
	mIsFullscreenChecked = fullscreen_checked;
}

BOOL LLViewerWindow::checkSettings()
{
	if (mStatesDirty)
	{
		gGL.refreshState();
		LLViewerShaderMgr::instance()->setShaders();
		mStatesDirty = false;
	}
	
	// We want to update the resolution AFTER the states getting refreshed not before.
	if (mResDirty)
	{
		if (gSavedSettings.getBOOL("FullScreenAutoDetectAspectRatio"))
		{
			getWindow()->setNativeAspectRatio(0.f);
		}
		else
		{
			getWindow()->setNativeAspectRatio(gSavedSettings.getF32("FullScreenAspectRatio"));
		}
		
		reshape(getWindowWidthRaw(), getWindowHeightRaw());

		// force aspect ratio
		if (mIsFullscreenChecked)
		{
			LLViewerCamera::getInstance()->setAspect( getWorldViewAspectRatio() );
		}

		mResDirty = false;
	}
		
	BOOL is_fullscreen = mWindow->getFullscreen();
	if(mWantFullscreen)
	{
		LLCoordScreen screen_size;
		LLCoordScreen desired_screen_size(gSavedSettings.getS32("FullScreenWidth"),
								   gSavedSettings.getS32("FullScreenHeight"));
		getWindow()->getSize(&screen_size);
		if(!is_fullscreen || 
		    screen_size.mX != desired_screen_size.mX 
			|| screen_size.mY != desired_screen_size.mY)
		{
			if (!LLStartUp::canGoFullscreen())
			{
				return FALSE;
			}
			
			LLGLState::checkStates();
			LLGLState::checkTextureChannels();
			changeDisplaySettings(TRUE, 
								  desired_screen_size,
								  gSavedSettings.getBOOL("DisableVerticalSync"),
								  mShowFullscreenProgress);

			LLGLState::checkStates();
			LLGLState::checkTextureChannels();
			mStatesDirty = true;
			return TRUE;
		}
	}
	else
	{
		if(is_fullscreen)
		{
			// Changing to windowed mode.
			changeDisplaySettings(FALSE, 
								  LLCoordScreen(gSavedSettings.getS32("WindowWidth"),
												gSavedSettings.getS32("WindowHeight")),
								  TRUE,
								  mShowFullscreenProgress);
			mStatesDirty = true;
			return TRUE;
		}
	}
	return FALSE;
}

void LLViewerWindow::restartDisplay(BOOL show_progress_bar)
{
	llinfos << "Restaring GL" << llendl;
	stopGL();
	if (show_progress_bar)
	{
		restoreGL("Changing Resolution...");
	}
	else
	{
		restoreGL();
	}
}

BOOL LLViewerWindow::changeDisplaySettings(BOOL fullscreen, LLCoordScreen size, BOOL disable_vsync, BOOL show_progress_bar)
{
	BOOL was_maximized = gSavedSettings.getBOOL("WindowMaximized");
	mWantFullscreen = fullscreen;
	mShowFullscreenProgress = show_progress_bar;
	gSavedSettings.setBOOL("WindowFullScreen", mWantFullscreen);

	//gResizeScreenTexture = TRUE;

	BOOL old_fullscreen = mWindow->getFullscreen();
	if (!old_fullscreen && fullscreen && !LLStartUp::canGoFullscreen())
	{
		// Not allowed to switch to fullscreen now, so exit early.
		// *NOTE: This case should never be reached, but just-in-case.
		return TRUE;
	}

	U32 fsaa = gSavedSettings.getU32("RenderFSAASamples");
	U32 old_fsaa = mWindow->getFSAASamples();
	// going from windowed to windowed
	if (!old_fullscreen && !fullscreen)
	{
		// if not maximized, use the request size
		if (!mWindow->getMaximized())
		{
			mWindow->setSize(size);
		}

		if (fsaa == old_fsaa)
		{
			return TRUE;
		}
	}

	// Close floaters that don't handle settings change
	LLFloaterReg::hideInstance("snapshot");
	
	BOOL result_first_try = FALSE;
	BOOL result_second_try = FALSE;

	LLFocusableElement* keyboard_focus = gFocusMgr.getKeyboardFocus();
	send_agent_pause();
	llinfos << "Stopping GL during changeDisplaySettings" << llendl;
	stopGL();
	mIgnoreActivate = TRUE;
	LLCoordScreen old_size;
	LLCoordScreen old_pos;
	mWindow->getSize(&old_size);
	BOOL got_position = mWindow->getPosition(&old_pos);

	if (!old_fullscreen && fullscreen && got_position)
	{
		// switching from windowed to fullscreen, so save window position
		gSavedSettings.setS32("WindowX", old_pos.mX);
		gSavedSettings.setS32("WindowY", old_pos.mY);
	}
	
	mWindow->setFSAASamples(fsaa);

	result_first_try = mWindow->switchContext(fullscreen, size, disable_vsync);
	if (!result_first_try)
	{
		// try to switch back
		mWindow->setFSAASamples(old_fsaa);
		result_second_try = mWindow->switchContext(old_fullscreen, old_size, disable_vsync);

		if (!result_second_try)
		{
			// we are stuck...try once again with a minimal resolution?
			send_agent_resume();
			mIgnoreActivate = FALSE;
			return FALSE;
		}
	}
	send_agent_resume();

	llinfos << "Restoring GL during resolution change" << llendl;
	if (show_progress_bar)
	{
		restoreGL("Changing Resolution...");
	}
	else
	{
		restoreGL();
	}

	if (!result_first_try)
	{
		LLSD args;
		args["RESX"] = llformat("%d",size.mX);
		args["RESY"] = llformat("%d",size.mY);
		LLNotificationsUtil::add("ResolutionSwitchFail", args);
		size = old_size; // for reshape below
	}

	BOOL success = result_first_try || result_second_try;
	if (success)
	{
#if LL_WINDOWS
		// Only trigger a reshape after switching to fullscreen; otherwise rely on the windows callback
		// (otherwise size is wrong; this is the entire window size, reshape wants the visible window size)
		if (fullscreen && result_first_try)
#endif
		{
			reshape(size.mX, size.mY);
		}
	}

	if (!mWindow->getFullscreen() && success)
	{
		// maximize window if was maximized, else reposition
		if (was_maximized)
		{
			mWindow->maximize();
		}
		else
		{
			S32 windowX = gSavedSettings.getS32("WindowX");
			S32 windowY = gSavedSettings.getS32("WindowY");

			mWindow->setPosition(LLCoordScreen ( windowX, windowY ) );
		}
	}

	mIgnoreActivate = FALSE;
	gFocusMgr.setKeyboardFocus(keyboard_focus);
	mWantFullscreen = mWindow->getFullscreen();
	mShowFullscreenProgress = FALSE;
	
	return success;
}


F32 LLViewerWindow::getDisplayAspectRatio() const
{
	if (mWindow->getFullscreen())
	{
		if (gSavedSettings.getBOOL("FullScreenAutoDetectAspectRatio"))
		{
			return mWindow->getNativeAspectRatio();	
		}
		else
		{
			return gSavedSettings.getF32("FullScreenAspectRatio");
		}
	}
	else
	{
		return mWindow->getNativeAspectRatio();
	}
}


F32	LLViewerWindow::getWorldViewAspectRatio() const
{
	F32 world_aspect = (F32)mWorldViewRectRaw.getWidth() / (F32)mWorldViewRectRaw.getHeight();
	//F32 window_aspect = (F32)mWindowRectRaw.getWidth() / (F32)mWindowRectRaw.getHeight();
	if (mWindow->getFullscreen())
	{
		return world_aspect * mWindow->getPixelAspectRatio();
	}
	else
	{
		return world_aspect;
	}
}

void LLViewerWindow::calcDisplayScale()
{
	F32 ui_scale_factor = gSavedSettings.getF32("UIScaleFactor");
	LLVector2 display_scale;
	display_scale.setVec(llmax(1.f / mWindow->getPixelAspectRatio(), 1.f), llmax(mWindow->getPixelAspectRatio(), 1.f));
	F32 height_normalization = gSavedSettings.getBOOL("UIAutoScale") ? ((F32)mWindowRectRaw.getHeight() / display_scale.mV[VY]) / 768.f : 1.f;
	if(mWindow->getFullscreen())
	{
		display_scale *= (ui_scale_factor * height_normalization);
	}
	else
	{
		display_scale *= ui_scale_factor;
	}

	// limit minimum display scale
	if (display_scale.mV[VX] < MIN_DISPLAY_SCALE || display_scale.mV[VY] < MIN_DISPLAY_SCALE)
	{
		display_scale *= MIN_DISPLAY_SCALE / llmin(display_scale.mV[VX], display_scale.mV[VY]);
	}

	if (mWindow->getFullscreen())
	{
		display_scale.mV[0] = llround(display_scale.mV[0], 2.0f/(F32) mWindowRectRaw.getWidth());
		display_scale.mV[1] = llround(display_scale.mV[1], 2.0f/(F32) mWindowRectRaw.getHeight());
	}
	
	if (display_scale != mDisplayScale)
	{
		llinfos << "Setting display scale to " << display_scale << llendl;

		mDisplayScale = display_scale;
		// Init default fonts
		initFonts();
	}
}

//static
LLRect 	LLViewerWindow::calcScaledRect(const LLRect & rect, const LLVector2& display_scale)
{
	LLRect res = rect;
	res.mLeft = llround((F32)res.mLeft / display_scale.mV[VX]);
	res.mRight = llround((F32)res.mRight / display_scale.mV[VX]);
	res.mBottom = llround((F32)res.mBottom / display_scale.mV[VY]);
	res.mTop = llround((F32)res.mTop / display_scale.mV[VY]);

	return res;
}

S32 LLViewerWindow::getChatConsoleBottomPad()
{
	S32 offset = 0;

	if(LLBottomTray::instanceExists())
		offset += LLBottomTray::getInstance()->getRect().getHeight();

	return offset;
}

LLRect LLViewerWindow::getChatConsoleRect()
{
	LLRect full_window(0, getWindowHeightScaled(), getWindowWidthScaled(), 0);
	LLRect console_rect = full_window;

	const S32 CONSOLE_PADDING_TOP = 24;
	const S32 CONSOLE_PADDING_LEFT = 24;
	const S32 CONSOLE_PADDING_RIGHT = 10;

	console_rect.mTop    -= CONSOLE_PADDING_TOP;
	console_rect.mBottom += getChatConsoleBottomPad();

	console_rect.mLeft   += CONSOLE_PADDING_LEFT; 

	static const BOOL CHAT_FULL_WIDTH = gSavedSettings.getBOOL("ChatFullWidth");

	if (CHAT_FULL_WIDTH)
	{
		console_rect.mRight -= CONSOLE_PADDING_RIGHT;
	}
	else
	{
		// Make console rect somewhat narrow so having inventory open is
		// less of a problem.
		console_rect.mRight  = console_rect.mLeft + 2 * getWindowWidthScaled() / 3;
	}

	return console_rect;
}
//----------------------------------------------------------------------------


//static 
bool LLViewerWindow::onAlert(const LLSD& notify)
{
	LLNotificationPtr notification = LLNotifications::instance().find(notify["id"].asUUID());

	if (gNoRender)
	{
		llinfos << "Alert: " << notification->getName() << llendl;
		notification->respond(LLSD::emptyMap());
		LLNotifications::instance().cancel(notification);
		return false;
	}

	// If we're in mouselook, the mouse is hidden and so the user can't click 
	// the dialog buttons.  In that case, change to First Person instead.
	if( gAgent.cameraMouselook() )
	{
		gAgent.changeCameraToDefault();
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////
//
// LLPickInfo
//
LLPickInfo::LLPickInfo()
	: mKeyMask(MASK_NONE),
	  mPickCallback(NULL),
	  mPickType(PICK_INVALID),
	  mWantSurfaceInfo(FALSE),
	  mObjectFace(-1),
	  mUVCoords(-1.f, -1.f),
	  mSTCoords(-1.f, -1.f),
	  mXYCoords(-1, -1),
	  mIntersection(),
	  mNormal(),
	  mBinormal(),
	  mHUDIcon(NULL),
	  mPickTransparent(FALSE)
{
}

LLPickInfo::LLPickInfo(const LLCoordGL& mouse_pos, 
		       MASK keyboard_mask, 
		       BOOL pick_transparent,
		       BOOL pick_uv_coords,
		       void (*pick_callback)(const LLPickInfo& pick_info))
	: mMousePt(mouse_pos),
	  mKeyMask(keyboard_mask),
	  mPickCallback(pick_callback),
	  mPickType(PICK_INVALID),
	  mWantSurfaceInfo(pick_uv_coords),
	  mObjectFace(-1),
	  mUVCoords(-1.f, -1.f),
	  mSTCoords(-1.f, -1.f),
	  mXYCoords(-1, -1),
	  mNormal(),
	  mBinormal(),
	  mHUDIcon(NULL),
	  mPickTransparent(pick_transparent)
{
}

void LLPickInfo::fetchResults()
{

	S32 face_hit = -1;
	LLVector3 intersection, normal, binormal;
	LLVector2 uv;

	LLHUDIcon* hit_icon = gViewerWindow->cursorIntersectIcon(mMousePt.mX, mMousePt.mY, 512.f, &intersection);
	
	F32 icon_dist = 0.f;
	if (hit_icon)
	{
		icon_dist = (LLViewerCamera::getInstance()->getOrigin()-intersection).magVec();
	}
	LLViewerObject* hit_object = gViewerWindow->cursorIntersect(mMousePt.mX, mMousePt.mY, 512.f,
									NULL, -1, mPickTransparent, &face_hit,
									&intersection, &uv, &normal, &binormal);
	
	mPickPt = mMousePt;

	U32 te_offset = face_hit > -1 ? face_hit : 0;

	//unproject relative clicked coordinate from window coordinate using GL
	
	LLViewerObject* objectp = hit_object;

	if (hit_icon && 
		(!objectp || 
		icon_dist < (LLViewerCamera::getInstance()->getOrigin()-intersection).magVec()))
	{
		// was this name referring to a hud icon?
		mHUDIcon = hit_icon;
		mPickType = PICK_ICON;
		mPosGlobal = mHUDIcon->getPositionGlobal();
	}
	else if (objectp)
	{
		if( objectp->getPCode() == LLViewerObject::LL_VO_SURFACE_PATCH )
		{
			// Hit land
			mPickType = PICK_LAND;
			mObjectID.setNull(); // land has no id

			// put global position into land_pos
			LLVector3d land_pos;
			if (!gViewerWindow->mousePointOnLandGlobal(mPickPt.mX, mPickPt.mY, &land_pos))
			{
				// The selected point is beyond the draw distance or is otherwise 
				// not selectable. Return before calling mPickCallback().
				return;
			}

			// Fudge the land focus a little bit above ground.
			mPosGlobal = land_pos + LLVector3d::z_axis * 0.1f;
		}
		else
		{
			if(isFlora(objectp))
			{
				mPickType = PICK_FLORA;
			}
			else
			{
				mPickType = PICK_OBJECT;
			}
			mObjectOffset = gAgent.calcFocusOffset(objectp, intersection, mPickPt.mX, mPickPt.mY);
			mObjectID = objectp->mID;
			mObjectFace = (te_offset == NO_FACE) ? -1 : (S32)te_offset;

			mPosGlobal = gAgent.getPosGlobalFromAgent(intersection);
			
			if (mWantSurfaceInfo)
			{
				getSurfaceInfo();
			}
		}
	}
	
	if (mPickCallback)
	{
		mPickCallback(*this);
	}
}

LLPointer<LLViewerObject> LLPickInfo::getObject() const
{
	return gObjectList.findObject( mObjectID );
}

void LLPickInfo::updateXYCoords()
{
	if (mObjectFace > -1)
	{
		const LLTextureEntry* tep = getObject()->getTE(mObjectFace);
		LLPointer<LLViewerTexture> imagep = LLViewerTextureManager::getFetchedTexture(tep->getID());
		if(mUVCoords.mV[VX] >= 0.f && mUVCoords.mV[VY] >= 0.f && imagep.notNull())
		{
			mXYCoords.mX = llround(mUVCoords.mV[VX] * (F32)imagep->getWidth());
			mXYCoords.mY = llround((1.f - mUVCoords.mV[VY]) * (F32)imagep->getHeight());
		}
	}
}

void LLPickInfo::getSurfaceInfo()
{
	// set values to uninitialized - this is what we return if no intersection is found
	mObjectFace   = -1;
	mUVCoords     = LLVector2(-1, -1);
	mSTCoords     = LLVector2(-1, -1);
	mXYCoords	  = LLCoordScreen(-1, -1);
	mIntersection = LLVector3(0,0,0);
	mNormal       = LLVector3(0,0,0);
	mBinormal     = LLVector3(0,0,0);
	
	LLViewerObject* objectp = getObject();

	if (objectp)
	{
		if (gViewerWindow->cursorIntersect(llround((F32)mMousePt.mX), llround((F32)mMousePt.mY), 1024.f,
										   objectp, -1, mPickTransparent,
										   &mObjectFace,
										   &mIntersection,
										   &mSTCoords,
										   &mNormal,
										   &mBinormal))
		{
			// if we succeeded with the intersect above, compute the texture coordinates:

			if (objectp->mDrawable.notNull() && mObjectFace > -1)
			{
				LLFace* facep = objectp->mDrawable->getFace(mObjectFace);

				mUVCoords = facep->surfaceToTexture(mSTCoords, mIntersection, mNormal);
			}

			// and XY coords:
			updateXYCoords();
			
		}
	}
}


/* code to get UV via a special UV render - removed in lieu of raycast method
LLVector2 LLPickInfo::pickUV()
{
	LLVector2 result(-1.f, -1.f);

	LLViewerObject* objectp = getObject();
	if (!objectp)
	{
		return result;
	}

	if (mObjectFace > -1 &&
		objectp->mDrawable.notNull() && objectp->getPCode() == LL_PCODE_VOLUME &&
		mObjectFace < objectp->mDrawable->getNumFaces())
	{
		S32 scaled_x = llround((F32)mPickPt.mX * gViewerWindow->getDisplayScale().mV[VX]);
		S32 scaled_y = llround((F32)mPickPt.mY * gViewerWindow->getDisplayScale().mV[VY]);
		const S32 UV_PICK_WIDTH = 5;
		const S32 UV_PICK_HALF_WIDTH = (UV_PICK_WIDTH - 1) / 2;
		U8 uv_pick_buffer[UV_PICK_WIDTH * UV_PICK_WIDTH * 4];
		LLFace* facep = objectp->mDrawable->getFace(mObjectFace);
		if (facep)
		{
			LLGLState scissor_state(GL_SCISSOR_TEST);
			scissor_state.enable();
			LLViewerCamera::getInstance()->setPerspective(FOR_SELECTION, scaled_x - UV_PICK_HALF_WIDTH, scaled_y - UV_PICK_HALF_WIDTH, UV_PICK_WIDTH, UV_PICK_WIDTH, FALSE);
			//glViewport(scaled_x - UV_PICK_HALF_WIDTH, scaled_y - UV_PICK_HALF_WIDTH, UV_PICK_WIDTH, UV_PICK_WIDTH);
			glScissor(scaled_x - UV_PICK_HALF_WIDTH, scaled_y - UV_PICK_HALF_WIDTH, UV_PICK_WIDTH, UV_PICK_WIDTH);

			glClear(GL_DEPTH_BUFFER_BIT);

			facep->renderSelectedUV();

			glReadPixels(scaled_x - UV_PICK_HALF_WIDTH, scaled_y - UV_PICK_HALF_WIDTH, UV_PICK_WIDTH, UV_PICK_WIDTH, GL_RGBA, GL_UNSIGNED_BYTE, uv_pick_buffer);
			U8* center_pixel = &uv_pick_buffer[4 * ((UV_PICK_WIDTH * UV_PICK_HALF_WIDTH) + UV_PICK_HALF_WIDTH + 1)];

			result.mV[VX] = (F32)((center_pixel[VGREEN] & 0xf) + (16.f * center_pixel[VRED])) / 4095.f;
			result.mV[VY] = (F32)((center_pixel[VGREEN] >> 4) + (16.f * center_pixel[VBLUE])) / 4095.f;
		}
	}

	return result;
} */


//static 
bool LLPickInfo::isFlora(LLViewerObject* object)
{
	if (!object) return false;

	LLPCode pcode = object->getPCode();

	if( (LL_PCODE_LEGACY_GRASS == pcode) 
		|| (LL_PCODE_LEGACY_TREE == pcode) 
		|| (LL_PCODE_TREE_NEW == pcode))
	{
		return true;
	}
	return false;
}
