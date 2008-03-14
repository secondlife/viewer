/** 
 * @file llviewerwindow.cpp
 * @brief Implementation of the LLViewerWindow class.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llpanellogin.h"
#include "llviewerwindow.h"

// system library includes
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "llviewquery.h"
#include "llxmltree.h"
//#include "llviewercamera.h"
#include "llglimmediate.h"

#include "llvoiceclient.h"	// for push-to-talk button handling

#ifdef SABINRIG
#include "cbw.h"
#endif //SABINRIG

//
// TODO: Many of these includes are unnecessary.  Remove them.
//

// linden library includes
#include "audioengine.h"		// mute on minimize
#include "indra_constants.h"
#include "llassetstorage.h"
#include "llfontgl.h"
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

// newview includes
#include "llagent.h"
#include "llalertdialog.h"
#include "llbox.h"
#include "llcameraview.h"
#include "llchatbar.h"
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
#include "llfloateractivespeakers.h"
#include "llfloaterbuildoptions.h"
#include "llfloaterbuyland.h"
#include "llfloaterchat.h"
#include "llfloaterchatterbox.h"
#include "llfloatercustomize.h"
#include "llfloatereditui.h" // HACK JAMESDEBUG for ui editor
#include "llfloaterland.h"
#include "llfloaterinspect.h"
#include "llfloatermap.h"
#include "llfloatermute.h"
#include "llfloaternamedesc.h"
#include "llfloaterpreference.h"
#include "llfloatersnapshot.h"
#include "llfloatertools.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "llframestatview.h"
#include "llgesturemgr.h"
#include "llglheaders.h"
#include "llhippo.h"
#include "llhoverview.h"
#include "llhudmanager.h"
#include "llhudview.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llinventoryview.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llmodaldialog.h"
#include "llmorphview.h"
#include "llmoveview.h"
#include "llnotify.h"
#include "lloverlaybar.h"
#include "llpreviewtexture.h"
#include "llprogressview.h"
#include "llresmgr.h"
#include "llrootview.h"
#include "llselectmgr.h"
#include "llsphere.h"
#include "llstartup.h"
#include "llstatusbar.h"
#include "llstatview.h"
#include "llsurface.h"
#include "llsurfacepatch.h"
#include "llimview.h"
#include "lltexlayer.h"
#include "lltextbox.h"
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "lltextureview.h"
#include "lltool.h"
#include "lltoolbar.h"
#include "lltoolcomp.h"
#include "lltooldraganddrop.h"
#include "lltoolface.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lltoolmorph.h"
#include "lltoolpie.h"
#include "lltoolplacer.h"
#include "lltoolselect.h"
#include "lltoolselectland.h"
#include "lltoolview.h"
#include "llvieweruictrlfactory.h"
#include "lluploaddialog.h"
#include "llurldispatcher.h"		// SLURL from other app instance
#include "llvieweraudio.h"
#include "llviewercamera.h"
#include "llviewergesture.h"
#include "llviewerimagelist.h"
#include "llviewerinventory.h"
#include "llviewerkeyboard.h"
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llvoavatar.h"
#include "llvovolume.h"
#include "llworld.h"
#include "llworldmapview.h"
#include "pipeline.h"
#include "llappviewer.h"
#include "llurlsimstring.h"
#include "llviewerdisplay.h"
#include "llspatialpartition.h"

#if LL_WINDOWS
#include "llwindebug.h"
#include <tchar.h> // For Unicode conversion methods
#endif

//
// Globals
//
void render_ui_and_swap_if_needed();
void render_ui_and_swap();
LLBottomPanel* gBottomPanel = NULL;

extern BOOL gDebugClicks;
extern BOOL gDisplaySwapBuffers;
extern BOOL gResizeScreenTexture;
extern S32 gJamesInt;

LLViewerWindow	*gViewerWindow = NULL;
LLVelocityBar	*gVelocityBar = NULL;

LLVector3d		gLastHitPosGlobal;
LLVector3d		gLastHitObjectOffset;
LLUUID			gLastHitObjectID;
S32				gLastHitObjectFace = -1;
BOOL			gLastHitLand = FALSE;
F32				gLastHitUCoord;
F32				gLastHitVCoord;


LLVector3d		gLastHitNonFloraPosGlobal;
LLVector3d		gLastHitNonFloraObjectOffset;
LLUUID			gLastHitNonFloraObjectID;
S32				gLastHitNonFloraObjectFace = -1;
BOOL			gLastHitParcelWall = FALSE;

S32				gLastHitUIElement = 0;
LLHUDIcon*		gLastHitHUDIcon = NULL;

BOOL			gDebugSelect = FALSE;
U8				gLastPickAlpha = 255;
BOOL			gUseGLPick = FALSE;

// On the next pick pass (whenever that happens)
// should we try to pick individual faces?
// Cleared to FALSE every time a pick happens.
BOOL			gPickFaces = FALSE;

LLFrameTimer	gMouseIdleTimer;
LLFrameTimer	gAwayTimer;
LLFrameTimer	gAwayTriggerTimer;
LLFrameTimer	gAlphaFadeTimer;

BOOL			gShowOverlayTitle = FALSE;
BOOL			gPickTransparent = TRUE;

BOOL			gDebugFastUIRender = FALSE;

// HUD display lines in lower right
BOOL				gDisplayWindInfo = FALSE;
BOOL				gDisplayCameraPos = FALSE;
BOOL				gDisplayNearestWater = FALSE;
BOOL				gDisplayFOV = FALSE;

S32 CHAT_BAR_HEIGHT = 28; 
S32 OVERLAY_BAR_HEIGHT = 20;

const U8 NO_FACE = 255;
BOOL gQuietSnapshot = FALSE;

const F32 MIN_AFK_TIME = 2.f; // minimum time after setting away state before coming back
const F32 MAX_FAST_FRAME_TIME = 0.5f;
const F32 FAST_FRAME_INCREMENT = 0.1f;

const S32 PICK_HALF_WIDTH = 5;
const S32 PICK_DIAMETER = 2 * PICK_HALF_WIDTH+1;

const F32 MIN_DISPLAY_SCALE = 0.85f;

const S32 CONSOLE_BOTTOM_PAD = 40;

#ifdef SABINRIG
/// ALL RIG STUFF
bool rigControl = false;
bool voltDisplay = true;
bool nominalX = false;
bool nominalY = false;
static F32 nomerX = 0.0f;
static F32 nomerY = 0.0f;
const BOARD_NUM = 0; // rig stuff!
const ADRANGE = BIP10VOLTS; // rig stuff!
static unsigned short DataVal; // rig stuff!
static F32 oldValueX = 0;
static F32 newValueX = 50;
static F32 oldValueY = 0;
static F32 newValueY = 50;
static S32 mouseX = 50;
static S32 mouseY = 50;
static float VoltageX = 50; // rig stuff!
static float VoltageY = 50; // rig stuff!
static float nVoltX = 0;
static float nVoltY = 0;
static F32 temp1 = 50.f;
static F32 temp2 = 20.f;
LLCoordGL new_gl;
#endif //SABINRIG

char	LLViewerWindow::sSnapshotBaseName[LL_MAX_PATH];
char	LLViewerWindow::sSnapshotDir[LL_MAX_PATH];

char	LLViewerWindow::sMovieBaseName[LL_MAX_PATH];

extern void toggle_debug_menus(void*);

#ifdef SABINRIG
// static
void LLViewerWindow::printFeedback()
{
	if(rigControl == true)
	{
		cbAIn (BOARD_NUM, 0, ADRANGE, &DataVal);
		cbToEngUnits (BOARD_NUM,ADRANGE,DataVal,&VoltageX); //Convert raw to voltage for X-axis
		cbAIn (BOARD_NUM, 1, ADRANGE, &DataVal);
		cbToEngUnits (BOARD_NUM,ADRANGE,DataVal,&VoltageY); //Convert raw to voltage for Y-axis
		if(voltDisplay == true)
		{
			llinfos <<  "Current Voltages - X:" << VoltageX << " Y:" << VoltageY << llendl; //Display voltage
		}

		if(nVoltX == 0)
		{
			nVoltX = VoltageX;
			nVoltY = VoltageY; //First time setup of nominal values.
		}

		newValueX = VoltageX;
		newValueY = VoltageY; //Take in current voltage and set to a separate value for adjustment.

		mouseX = mCurrentMousePoint.mX;
		mouseY = mCurrentMousePoint.mY; //Take in current cursor position and set to separate value for adjustment.

		if( abs(newValueX - nVoltX) > nomerX )
		{
			if( (newValueX - oldValueX) < 0)
			{
				mouseX += (S32)( ((newValueX - oldValueX)*.5)) * -temp;
			}
			else
			{
				mouseX += (S32)( ((newValueX - oldValueX)*.5) * temp1);
			}
		}
		else
		{
			mouseX = getWindowWidth() / 2;
		}
		if( abs(newValueY - nVoltY) > nomerY )
		{
			if( (newValueY - oldValueY) < 0)
			{
				mouseY += (S32)( ((newValueY - oldValueY)*(newValueY - oldValueY)) * -temp2);
			}
			else
			{
				mouseY += (S32)( ((newValueY - oldValueY)*(newValueY - oldValueY)) * temp2);
			}
		}
		else
		{
			mouseY = getWindowHeight() / 2;
		}
		//mouseX += (S32)( (newValueX - nVoltX) * temp1 + 0.5 );
		// (newValueX - oldValueX) = difference between current position and nominal position
		// * temp1 = the amplification of the number that sets sensitivity
		// + 0.5 = fixes rounding errors
		

		//mouseY += (S32)( (newValueY - nVoltY) * temp2 + 0.5 ); //Algorithm to adjust voltage for mouse adjustment.

		oldValueX = newValueX;
		oldValueY = newValueY;

		new_gl.mX = mouseX;
		new_gl.mY = mouseY; //Setup final coordinate to move mouse to.

		setCursorPosition(new_gl); //Set final cursor position
	}
}
#endif //SABINRIG

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
		U32 xpos = mWindow->getWindowWidth() - 350;
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
				addText(xpos, ypos, llformat(" Debug %d: %d:%02d:%02d", idx, hours,mins,secs)); ypos += y_inc2;
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

			tvector = LLVector4(gCamera->getAtAxis());
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
			addText(xpos, ypos, llformat("FOV: %2.1f deg", RAD_TO_DEG * gCamera->getView()));
			ypos += y_inc;
		}
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
					gPipeline.mMeanBatchSize));

				gPipeline.mMinBatchSize = gPipeline.mMaxBatchSize;
				gPipeline.mMaxBatchSize = 0;
				gPipeline.mBatchCount = 0;
			}
            ypos += y_inc;

			addText(xpos,ypos, llformat("%d/%d Nodes visible", gPipeline.mNumVisibleNodes, LLSpatialGroup::sNodeCount));
			
			ypos += y_inc;


			addText(xpos,ypos, llformat("%d Avatars visible", LLVOAvatar::sNumVisibleAvatars));
			
			ypos += y_inc;

			LLVertexBuffer::sBindCount = LLImageGL::sBindCount = 
				LLVertexBuffer::sSetCount = LLImageGL::sUniqueCount = 
				gPipeline.mNumVisibleNodes = 0;
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
		if (LLPipeline::getRenderBeacons(NULL) && LLPipeline::getProcessBeacons(NULL))
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
			LLFontGL::sMonospace->renderUTF8(line.text, 0, (F32)line.x, (F32)line.y, mTextColor,
											 LLFontGL::LEFT, LLFontGL::TOP,
											 LLFontGL::NORMAL, S32_MAX, S32_MAX, NULL, FALSE);
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

BOOL LLViewerWindow::handleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	S32 x = pos.mX;
	S32 y = pos.mY;
	x = llround((F32)x / mDisplayScale.mV[VX]);
	y = llround((F32)y / mDisplayScale.mV[VY]);

	LLView::sMouseHandlerMessage = "";

	if (gDebugClicks)
	{
		llinfos << "ViewerWindow left mouse down at " << x << "," << y << llendl;
	}

	if (gMenuBarView)
	{
		// stop ALT-key access to menu
		gMenuBarView->resetMenuTrigger();
	}

	mLeftMouseDown = TRUE;

	// Make sure we get a coresponding mouseup event, even if the mouse leaves the window
	mWindow->captureMouse();

	// Indicate mouse was active
	gMouseIdleTimer.reset();

	// Hide tooltips on mousedown
	if( mToolTip )
	{
		mToolTipBlocked = TRUE;
		mToolTip->setVisible( FALSE );
	}

	// Also hide hover info on mousedown
	if (gHoverView)
	{
		gHoverView->cancelHover();
	}

	if (gToolMgr)
	{
		// Don't let the user move the mouse out of the window until mouse up.
		if( gToolMgr->getCurrentTool()->clipMouseWhenDown() )
		{
			mWindow->setMouseClipping(TRUE);
		}
	}

	LLMouseHandler* mouse_captor = gFocusMgr.getMouseCapture();
	if( mouse_captor )
	{
		S32 local_x;
		S32 local_y;
		mouse_captor->screenPointToLocal( x, y, &local_x, &local_y );
		if (LLView::sDebugMouseHandling)
		{
			llinfos << "Left Mouse Down handled by captor " << mouse_captor->getName() << llendl;
		}

		return mouse_captor->handleMouseDown(local_x, local_y, mask);
	}

	// Topmost view gets a chance before the hierarchy
	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	if (top_ctrl)
	{
		S32 local_x, local_y;
		top_ctrl->screenPointToLocal( x, y, &local_x, &local_y );
		if (top_ctrl->pointInView(local_x, local_y))
		{
			return top_ctrl->handleMouseDown(local_x, local_y, mask);
		}
		else
		{
			setTopCtrl(NULL);
		}
	}

	// Give the UI views a chance to process the click
	if( mRootView->handleMouseDown(x, y, mask) )
	{
		if (LLView::sDebugMouseHandling)
		{
			llinfos << "Left Mouse Down" << LLView::sMouseHandlerMessage << llendl;
		}
		return TRUE;
	}
	else if (LLView::sDebugMouseHandling)
	{
		llinfos << "Left Mouse Down not handled by view" << llendl;
	}

	if (gDisconnected)
	{
		return FALSE;
	}

	if (gToolMgr)
	{
		if(gToolMgr->getCurrentTool()->handleMouseDown( x, y, mask ) )
		{
			// This is necessary to force clicks in the world to cause edit
			// boxes that might have keyboard focus to relinquish it, and hence
			// cause a commit to update their value.  JC
			gFocusMgr.setKeyboardFocus(NULL);
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLViewerWindow::handleDoubleClick(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	S32 x = pos.mX;
	S32 y = pos.mY;
	x = llround((F32)x / mDisplayScale.mV[VX]);
	y = llround((F32)y / mDisplayScale.mV[VY]);

	LLView::sMouseHandlerMessage = "";

	if (gDebugClicks)
	{
		llinfos << "ViewerWindow left mouse double-click at " << x << "," << y << llendl;
	}

	mLeftMouseDown = TRUE;

	// Hide tooltips
	if( mToolTip )
	{
		mToolTip->setVisible( FALSE );
	}

	LLMouseHandler* mouse_captor = gFocusMgr.getMouseCapture();
	if( mouse_captor )
	{
		S32 local_x;
		S32 local_y;
		mouse_captor->screenPointToLocal( x, y, &local_x, &local_y );
		if (LLView::sDebugMouseHandling)
		{
			llinfos << "Left Mouse Down handled by captor " << mouse_captor->getName() << llendl;
		}

		return mouse_captor->handleDoubleClick(local_x, local_y, mask);
	}

	// Check for hit on UI.
	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	if (top_ctrl)
	{
		S32 local_x, local_y;
		top_ctrl->screenPointToLocal( x, y, &local_x, &local_y );
		if (top_ctrl->pointInView(local_x, local_y))
		{
			return top_ctrl->handleDoubleClick(local_x, local_y, mask);
		}
		else
		{
			setTopCtrl(NULL);
		}
	}

	if (mRootView->handleDoubleClick(x, y, mask)) 
	{
		if (LLView::sDebugMouseHandling)
		{
			llinfos << "Left Mouse Down" << LLView::sMouseHandlerMessage << llendl;
		}
		return TRUE;
	}
	else if (LLView::sDebugMouseHandling)
	{
		llinfos << "Left Mouse Down not handled by view" << llendl;
	}

		// Why is this here?  JC 9/3/2002
	if (gNoRender) 
	{
		return TRUE;
	}

	if (gToolMgr)
	{
		if(gToolMgr->getCurrentTool()->handleDoubleClick( x, y, mask ) )
		{
			return TRUE;
		}
	}

	// if we got this far and nothing handled a double click, pass a normal mouse down
	return handleMouseDown(window, pos, mask);
}

BOOL LLViewerWindow::handleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	S32 x = pos.mX;
	S32 y = pos.mY;
	x = llround((F32)x / mDisplayScale.mV[VX]);
	y = llround((F32)y / mDisplayScale.mV[VY]);

	LLView::sMouseHandlerMessage = "";

	if (gDebugClicks)
	{
		llinfos << "ViewerWindow left mouse up" << llendl;
	}

	mLeftMouseDown = FALSE;

	// Indicate mouse was active
	gMouseIdleTimer.reset();

	// Hide tooltips on mouseup
	if( mToolTip )
	{
		mToolTip->setVisible( FALSE );
	}

	// Also hide hover info on mouseup
	if (gHoverView)	gHoverView->cancelHover();

	BOOL handled = FALSE;

	mWindow->releaseMouse();

	LLTool *tool = NULL;
	if (gToolMgr)
	{
		tool = gToolMgr->getCurrentTool();

		if( tool->clipMouseWhenDown() )
		{
			mWindow->setMouseClipping(FALSE);
		}
	}

	LLMouseHandler* mouse_captor = gFocusMgr.getMouseCapture();
	if( mouse_captor )
	{
		S32 local_x;
		S32 local_y;
		mouse_captor->screenPointToLocal( x, y, &local_x, &local_y );
		if (LLView::sDebugMouseHandling)
		{
			llinfos << "Left Mouse Up handled by captor " << mouse_captor->getName() << llendl;
		}

		return mouse_captor->handleMouseUp(local_x, local_y, mask);
	}

	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	if (top_ctrl)
	{
		S32 local_x, local_y;
		top_ctrl->screenPointToLocal( x, y, &local_x, &local_y );
		handled = top_ctrl->pointInView(local_x, local_y) && top_ctrl->handleMouseUp(local_x, local_y, mask);
	}

	if( !handled )
	{
		handled = mRootView->handleMouseUp(x, y, mask);
	}

	if (LLView::sDebugMouseHandling)
	{
		if (handled)
		{
			llinfos << "Left Mouse Up" << LLView::sMouseHandlerMessage << llendl;
		}
		else 
		{
			llinfos << "Left Mouse Up not handled by view" << llendl;
		}
	}

	if( !handled )
	{
		if (tool)
		{
			handled = tool->handleMouseUp(x, y, mask);
		}
	}

	// Always handled as far as the OS is concerned.
	return TRUE;
}


BOOL LLViewerWindow::handleRightMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	S32 x = pos.mX;
	S32 y = pos.mY;
	x = llround((F32)x / mDisplayScale.mV[VX]);
	y = llround((F32)y / mDisplayScale.mV[VY]);

	LLView::sMouseHandlerMessage = "";

	if (gDebugClicks)
	{
		llinfos << "ViewerWindow right mouse down at " << x << "," << y << llendl;
	}

	if (gMenuBarView)
	{
		// stop ALT-key access to menu
		gMenuBarView->resetMenuTrigger();
	}

	mRightMouseDown = TRUE;

	// Make sure we get a coresponding mouseup event, even if the mouse leaves the window
	mWindow->captureMouse();

	// Hide tooltips
	if( mToolTip )
	{
		mToolTip->setVisible( FALSE );
	}

	// Also hide hover info on mousedown
	if (gHoverView)
	{
		gHoverView->cancelHover();
	}

	if (gToolMgr)
	{
		// Don't let the user move the mouse out of the window until mouse up.
		if( gToolMgr->getCurrentTool()->clipMouseWhenDown() )
		{
			mWindow->setMouseClipping(TRUE);
		}
	}

	LLMouseHandler* mouse_captor = gFocusMgr.getMouseCapture();
	if( mouse_captor )
	{
		S32 local_x;
		S32 local_y;
		mouse_captor->screenPointToLocal( x, y, &local_x, &local_y );
		if (LLView::sDebugMouseHandling)
		{
			llinfos << "Right Mouse Down handled by captor " << mouse_captor->getName() << llendl;
		}
		return mouse_captor->handleRightMouseDown(local_x, local_y, mask);
	}

	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	if (top_ctrl)
	{
		S32 local_x, local_y;
		top_ctrl->screenPointToLocal( x, y, &local_x, &local_y );
		if (top_ctrl->pointInView(local_x, local_y))
		{
			return top_ctrl->handleRightMouseDown(local_x, local_y, mask);
		}
		else
		{
			setTopCtrl(NULL);
		}
	}

	if( mRootView->handleRightMouseDown(x, y, mask) )
	{
		if (LLView::sDebugMouseHandling)
		{
			llinfos << "Right Mouse Down" << LLView::sMouseHandlerMessage << llendl;
		}
		return TRUE;
	}
	else if (LLView::sDebugMouseHandling)
	{
		llinfos << "Right Mouse Down not handled by view" << llendl;
	}

	if (gToolMgr)
	{
		if(gToolMgr->getCurrentTool()->handleRightMouseDown( x, y, mask ) )
		{
			// This is necessary to force clicks in the world to cause edit
			// boxes that might have keyboard focus to relinquish it, and hence
			// cause a commit to update their value.  JC
			gFocusMgr.setKeyboardFocus(NULL);
			return TRUE;
		}
	}

	// *HACK: this should be rolled into the composite tool logic, not
	// hardcoded at the top level.
	if (gToolPie && (CAMERA_MODE_CUSTOMIZE_AVATAR != gAgent.getCameraMode()) )
	{
		// If the current tool didn't process the click, we should show
		// the pie menu.  This can be done by passing the event to the pie
		// menu tool.
		gToolPie->handleRightMouseDown(x, y, mask);
		// show_context_menu( x, y, mask );
	}

	return TRUE;
}

BOOL LLViewerWindow::handleRightMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	S32 x = pos.mX;
	S32 y = pos.mY;
	x = llround((F32)x / mDisplayScale.mV[VX]);
	y = llround((F32)y / mDisplayScale.mV[VY]);

	LLView::sMouseHandlerMessage = "";

	// Don't care about caps lock for mouse events.
	if (gDebugClicks)
	{
		llinfos << "ViewerWindow right mouse up" << llendl;
	}

	mRightMouseDown = FALSE;

	// Indicate mouse was active
	gMouseIdleTimer.reset();

	// Hide tooltips on mouseup
	if( mToolTip )
	{
		mToolTip->setVisible( FALSE );
	}

	// Also hide hover info on mouseup
	if (gHoverView)	gHoverView->cancelHover();

	BOOL handled = FALSE;

	mWindow->releaseMouse();

	LLTool *tool = NULL;
	if (gToolMgr)
	{
		tool = gToolMgr->getCurrentTool();

		if( tool->clipMouseWhenDown() )
		{
			mWindow->setMouseClipping(FALSE);
		}
	}

	LLMouseHandler* mouse_captor = gFocusMgr.getMouseCapture();
	if( mouse_captor )
	{
		S32 local_x;
		S32 local_y;
		mouse_captor->screenPointToLocal( x, y, &local_x, &local_y );
		if (LLView::sDebugMouseHandling)
		{
			llinfos << "Right Mouse Up handled by captor " << mouse_captor->getName() << llendl;
		}
		return mouse_captor->handleRightMouseUp(local_x, local_y, mask);
	}

	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	if (top_ctrl)
	{
		S32 local_x, local_y;
		top_ctrl->screenPointToLocal( x, y, &local_x, &local_y );
		handled = top_ctrl->pointInView(local_x, local_y) && top_ctrl->handleRightMouseUp(local_x, local_y, mask);
	}

	if( !handled )
	{
		handled = mRootView->handleRightMouseUp(x, y, mask);
	}

	if (LLView::sDebugMouseHandling)
	{
		if (handled)
		{
			llinfos << "Right Mouse Up" << LLView::sMouseHandlerMessage << llendl;
		}
		else 
		{
			llinfos << "Right Mouse Up not handled by view" << llendl;
		}
	}

	if( !handled )
	{
		if (tool)
		{
			handled = tool->handleRightMouseUp(x, y, mask);
		}
	}

	// Always handled as far as the OS is concerned.
	return TRUE;
}

BOOL LLViewerWindow::handleMiddleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	gVoiceClient->middleMouseState(true);

	// Always handled as far as the OS is concerned.
	return TRUE;
}

BOOL LLViewerWindow::handleMiddleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	gVoiceClient->middleMouseState(false);

	// Always handled as far as the OS is concerned.
	return TRUE;
}

void LLViewerWindow::handleMouseMove(LLWindow *window,  LLCoordGL pos, MASK mask)
{
	S32 x = pos.mX;
	S32 y = pos.mY;

	x = llround((F32)x / mDisplayScale.mV[VX]);
	y = llround((F32)y / mDisplayScale.mV[VY]);

	mMouseInWindow = TRUE;

	// Save mouse point for access during idle() and display()

	LLCoordGL prev_saved_mouse_point = mCurrentMousePoint;
	LLCoordGL mouse_point(x, y);
	saveLastMouse(mouse_point);
	BOOL mouse_actually_moved = !gFocusMgr.getMouseCapture() &&  // mouse is not currenty captured
			((prev_saved_mouse_point.mX != mCurrentMousePoint.mX) || (prev_saved_mouse_point.mY != mCurrentMousePoint.mY)); // mouse moved from last recorded position

	gMouseIdleTimer.reset();

	mWindow->showCursorFromMouseMove();

	if (gAwayTimer.getElapsedTimeF32() > MIN_AFK_TIME)
	{
		gAgent.clearAFK();
	}

	if(mToolTip && mouse_actually_moved)
	{
		mToolTipBlocked = FALSE;  // Blocking starts on keyboard events and (only) ends here.
		if( mToolTip->getVisible() && !mToolTipStickyRect.pointInRect( x, y ) )
		{
			mToolTip->setVisible( FALSE );
		}
	}

	// Activate the hover picker on mouse move.
	if (gHoverView)
	{
		gHoverView->setTyping(FALSE);
	}
}

void LLViewerWindow::handleMouseLeave(LLWindow *window)
{
	// Note: we won't get this if we have captured the mouse.
	llassert( gFocusMgr.getMouseCapture() == NULL );
	mMouseInWindow = FALSE;
	if (mToolTip)
	{
		mToolTip->setVisible( FALSE );
	}
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
}

// The top-level window has gained focus (e.g. via ALT-TAB)
void LLViewerWindow::handleFocus(LLWindow *window)
{
	gFocusMgr.setAppHasFocus(TRUE);
	LLModalDialog::onAppFocusGained();

	gAgent.onAppFocusGained();
	if (gToolMgr)
	{
		gToolMgr->onAppFocusGained();
	}

	gShowTextEditCursor = TRUE;

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
	if( gToolMgr )
	{
		gToolMgr->onAppFocusLost();
	}
	gFocusMgr.setMouseCapture( NULL );

	if (gMenuBarView)
	{
		// stop ALT-key access to menu
		gMenuBarView->resetMenuTrigger();
	}

	// restore mouse cursor
	gViewerWindow->showCursor();
	gViewerWindow->getWindow()->setMouseClipping(FALSE);

	// JC - Leave keyboard focus, so if you're popping in and out editing
	// a script, you don't have to click in the editor again and again.
	// gFocusMgr.setKeyboardFocus( NULL );
	gShowTextEditCursor = FALSE;

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
		if (gAllowIdleAFK)
		{
			gAgent.setAFK();
		}
		
		// SL-53351: Make sure we're not in mouselook when minimised, to prevent control issues
		gAgent.changeCameraToDefault();
		
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

		LLString name_str;
		gAgent.getName(name_str);

		S32 len;
		char temp_str[255];		/* Flawfinder: ignore */
		snprintf(temp_str, sizeof(temp_str), "%s FPS %3.1f Phy FPS %2.1f Time Dil %1.3f",		/* Flawfinder: ignore */
				name_str.c_str(),
				gViewerStats->mFPSStat.getMeanPerSec(),
				gViewerStats->mSimPhysicsFPS.getPrev(0),
				gViewerStats->mSimTimeDilation.getPrev(0));
		len = strlen(temp_str);		/* Flawfinder: ignore */
		TextOutA(hdc, 0, 0, temp_str, len); 


		LLVector3d pos_global = gAgent.getPositionGlobal();
		snprintf(temp_str, sizeof(temp_str), "Avatar pos %6.1lf %6.1lf %6.1lf", pos_global.mdV[0], pos_global.mdV[1], pos_global.mdV[2]);		/* Flawfinder: ignore */
		len = strlen(temp_str);		/* Flawfinder: ignore */
		TextOutA(hdc, 0, 25, temp_str, len); 

		TextOutA(hdc, 0, 50, "Set \"DisableRendering FALSE\" in settings.ini file to reenable", 61);
		EndPaint(window_handle, &ps); 
		return TRUE;
	}
#endif
	return FALSE;
}


void LLViewerWindow::handleScrollWheel(LLWindow *window,  S32 clicks)
{
	gViewerWindow->handleScrollWheel( clicks );
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
		const bool from_external_browser = true;
		if (LLURLDispatcher::dispatch(url, from_external_browser))
		{
			// bring window to foreground, as it has just been "launched" from a URL
			mWindow->bringToFront();
		}
		break;
	}
}


//
// Classes
//
LLViewerWindow::LLViewerWindow(
	char* title, char* name,
	S32 x, S32 y,
	S32 width, S32 height,
	BOOL fullscreen, BOOL ignore_pixel_depth)
	:
	mActive(TRUE),
	mWantFullscreen(fullscreen),
	mShowFullscreenProgress(FALSE),
	mWindowRect(0, height, width, 0),
	mVirtualWindowRect(0, height, width, 0),
	mLeftMouseDown(FALSE),
	mRightMouseDown(FALSE),
	mToolTip(NULL),
	mToolTipBlocked(FALSE),
	mMouseInWindow( FALSE ),
	mLastMask( MASK_NONE ),
	mToolStored( NULL ),
	mSuppressToolbox( FALSE ),
	mHideCursorPermanent( FALSE ),
	mPickPending(FALSE),
	mIgnoreActivate( FALSE )
{
	// Default to application directory.
	strcpy(LLViewerWindow::sSnapshotBaseName, "Snapshot");	/* Flawfinder: ignore */
	strcpy(LLViewerWindow::sMovieBaseName, "SLmovie");	/* Flawfinder: ignore */
	LLViewerWindow::sSnapshotDir[0] = '\0';


	// create window
	mWindow = LLWindowManager::createWindow(
		title, name, x, y, width, height, 0,
		fullscreen, 
		gNoRender,
		gSavedSettings.getBOOL("DisableVerticalSync"),
		!gNoRender,
		ignore_pixel_depth);
#if LL_WINDOWS
	if (!LLWinDebug::setupExceptionHandler())
	{
		llwarns << " Someone took over my exception handler (post createWindow)!" << llendl;
	}
#endif

	if (NULL == mWindow)
	{
		LLSplashScreen::update("Shutting down...");
#if LL_LINUX || LL_SOLARIS
		llwarns << "Unable to create window, be sure screen is set at 32-bit color and your graphics driver is configured correctly.  See README-linux.txt or README-solaris.txt for further information."
				<< llendl;
#else
		llwarns << "Unable to create window, be sure screen is set at 32-bit color in Control Panels->Display->Settings"
				<< llendl;
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
		mWindowRect.set(0, size.mY, size.mX, 0);
		mVirtualWindowRect.set(0, llround((F32)size.mY / mDisplayScale.mV[VY]), llround((F32)size.mX / mDisplayScale.mV[VX]), 0);
	}
	
	LLFontManager::initClass();

	//
	// We want to set this stuff up BEFORE we initialize the pipeline, so we can turn off
	// stuff like AGP if we think that it'll crash the viewer.
	//
	llinfos << "Loading feature tables." << llendl;

	gFeatureManagerp->init();

	// Initialize OpenGL Renderer
	if (!gFeatureManagerp->isFeatureAvailable("RenderVBOEnable") ||
		!gGLManager.mHasVertexBufferObject)
	{
		gSavedSettings.setBOOL("RenderVBOEnable", FALSE);
	}
	LLVertexBuffer::initClass(gSavedSettings.getBOOL("RenderVBOEnable"));

	if (gFeatureManagerp->isSafe()
		|| (gSavedSettings.getS32("LastFeatureVersion") != gFeatureManagerp->getVersion())
		|| (gSavedSettings.getBOOL("ProbeHardwareOnStartup")))
	{
		gFeatureManagerp->applyRecommendedSettings();
		gSavedSettings.setBOOL("ProbeHardwareOnStartup", FALSE);
	}

	// If we crashed while initializng GL stuff last time, disable certain features
	if (gSavedSettings.getBOOL("RenderInitError"))
	{
		mInitAlert = "DisplaySettingsNoShaders";
		gFeatureManagerp->setGraphicsLevel(0, false);
		gSavedSettings.setU32("RenderQualityPerformance", 0);		
		
	}
		
	// set callbacks
	mWindow->setCallbacks(this);

	// Init the image list.  Must happen after GL is initialized and before the images that
	// LLViewerWindow needs are requested.
	gImageList.init();
	LLViewerImage::initClass();
	gBumpImageList.init();

	// Create container for all sub-views
	mRootView = new LLRootView("root", mVirtualWindowRect, FALSE);

	if (!gNoRender)
	{
		// Init default fonts
		initFonts();
	}

	// Init Resource Manager
	gResMgr = new LLResMgr();

	// Make avatar head look forward at start
	mCurrentMousePoint.mX = getWindowWidth() / 2;
	mCurrentMousePoint.mY = getWindowHeight() / 2;

	mPickBuffer = new U8[PICK_DIAMETER * PICK_DIAMETER * 4];

	gShowOverlayTitle = gSavedSettings.getBOOL("ShowOverlayTitle");
	mOverlayTitle = gSavedSettings.getString("OverlayTitle");
	// Can't have spaces in settings.ini strings, so use underscores instead and convert them.
	LLString::replaceChar(mOverlayTitle, '_', ' ');

	LLAlertDialog::setDisplayCallback(alertCallback); // call this before calling any modal dialogs

	// sync the keyboard's setting with the saved setting
	gSavedSettings.getControl("NumpadControl")->firePropertyChanged();

	mDebugText = new LLDebugText(this);

}

void LLViewerWindow::initGLDefaults()
{
	gGL.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

	F32 ambient[4] = {0.f,0.f,0.f,0.f };
	F32 diffuse[4] = {1.f,1.f,1.f,1.f };
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,ambient);
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,diffuse);
	
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);

	glEnable(GL_TEXTURE_2D);

	// lights for objects
	glShadeModel( GL_SMOOTH );

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
	
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glCullFace(GL_BACK);

	// RN: Need this for translation and stretch manip.
	gCone.prerender();
	gBox.prerender();
	gSphere.prerender();
	gCylinder.prerender();
}

void LLViewerWindow::initBase()
{
	S32 height = getWindowHeight();
	S32 width = getWindowWidth();

	LLRect full_window(0, height, width, 0);

	adjustRectanglesForFirstUse(full_window);

	////////////////////
	//
	// Set the gamma
	//

	F32 gamma = gSavedSettings.getF32("RenderGamma");
	if (gamma != 0.0f)
	{
		gViewerWindow->getWindow()->setGamma(gamma);
	}

	// Create global views

	// Create the floater view at the start so that other views can add children to it. 
	// (But wait to add it as a child of the root view so that it will be in front of the 
	// other views.)

	// Constrain floaters to inside the menu and status bar regions.
	LLRect floater_view_rect = full_window;
	// make space for menu bar if we have one
	floater_view_rect.mTop -= MENU_BAR_HEIGHT;
	floater_view_rect.mBottom += STATUS_BAR_HEIGHT + 12 + 16 + 2;

	// Check for non-first startup
	S32 floater_view_bottom = gSavedSettings.getS32("FloaterViewBottom");
	if (floater_view_bottom >= 0)
	{
		floater_view_rect.mBottom = floater_view_bottom;
	}
	gFloaterView = new LLFloaterView("Floater View", floater_view_rect );
	gFloaterView->setVisible(TRUE);

	gSnapshotFloaterView = new LLSnapshotFloaterView("Snapshot Floater View", full_window);
	gSnapshotFloaterView->setVisible(TRUE);

	// Console
	llassert( !gConsole );
	LLRect console_rect = full_window;
	console_rect.mTop    -= 24;
	console_rect.mBottom += STATUS_BAR_HEIGHT + 12 + 16 + 12;
	console_rect.mLeft   += 24; //gSavedSettings.getS32("StatusBarButtonWidth") + gSavedSettings.getS32("StatusBarPad");

	if (gSavedSettings.getBOOL("ChatFullWidth"))
	{
		console_rect.mRight -= 10;
	}
	else
	{
		// Make console rect somewhat narrow so having inventory open is
		// less of a problem.
		console_rect.mRight  = console_rect.mLeft + 2 * width / 3;
	}

	gConsole = new LLConsole(
		"console",
		gSavedSettings.getS32("ConsoleBufferSize"),
		console_rect,
		gSavedSettings.getS32("ChatFontSize"),
		gSavedSettings.getF32("ChatPersistTime") );
	gConsole->setFollows(FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_BOTTOM);
	mRootView->addChild(gConsole);

	// Debug view over the console
	gDebugView = new LLDebugView("gDebugView", full_window);
	gDebugView->setFollowsAll();
	gDebugView->setVisible(TRUE);
	mRootView->addChild(gDebugView);

	// Add floater view at the end so it will be on top, and give it tab priority over others
	mRootView->addChild(gFloaterView, -1);
	mRootView->addChild(gSnapshotFloaterView);

	// notify above floaters!
	LLRect notify_rect = full_window;
	//notify_rect.mTop -= 24;
	notify_rect.mBottom += STATUS_BAR_HEIGHT;
	gNotifyBoxView = new LLNotifyBoxView("notify_container", notify_rect, FALSE, FOLLOWS_ALL);
	mRootView->addChild(gNotifyBoxView, -2);

	// Tooltips go above floaters
	mToolTip = new LLTextBox( "tool tip", LLRect(0, 1, 1, 0 ) );
	mToolTip->setHPad( 4 );
	mToolTip->setVPad( 2 );
	mToolTip->setColor( gColors.getColor( "ToolTipTextColor" ) );
	mToolTip->setBorderColor( gColors.getColor( "ToolTipBorderColor" ) );
	mToolTip->setBorderVisible( FALSE );
	mToolTip->setBackgroundColor( gColors.getColor( "ToolTipBgColor" ) );
	mToolTip->setBackgroundVisible( TRUE );
	mToolTip->setFontStyle(LLFontGL::NORMAL);
	mToolTip->setBorderDropshadowVisible( TRUE );
	mToolTip->setVisible( FALSE );

	// Add the progress bar view (startup view), which overrides everything
	mProgressView = new LLProgressView("ProgressView", full_window);
	mRootView->addChild(mProgressView);
	setShowProgress(FALSE);
	setProgressCancelButtonVisible(FALSE, "");
}


void adjust_rect_top_left(const LLString& control, const LLRect& window)
{
	LLRect r = gSavedSettings.getRect(control);
	if (r.mLeft == 0 && r.mBottom == 0)
	{
		r.setLeftTopAndSize(0, window.getHeight(), r.getWidth(), r.getHeight());
		gSavedSettings.setRect(control, r);
	}
}

void adjust_rect_top_right(const LLString& control, const LLRect& window)
{
	LLRect r = gSavedSettings.getRect(control);
	if (r.mLeft == 0 && r.mBottom == 0)
	{
		r.setLeftTopAndSize(window.getWidth() - r.getWidth(),
			window.getHeight(),
			r.getWidth(), 
			r.getHeight());
		gSavedSettings.setRect(control, r);
	}
}

void adjust_rect_bottom_center(const LLString& control, const LLRect& window)
{
	LLRect r = gSavedSettings.getRect(control);
	if (r.mLeft == 0 && r.mBottom == 0)
	{
		// *TODO: Adjust based on XUI XML
		const S32 TOOLBAR_HEIGHT = 64;
		r.setOriginAndSize(
			window.getWidth()/2 - r.getWidth()/2,
			TOOLBAR_HEIGHT,
			r.getWidth(),
			r.getHeight());
		gSavedSettings.setRect(control, r);
	}
}

void adjust_rect_centered_partial_zoom(const LLString& control,
									   const LLRect& window)
{
	LLRect rect = gSavedSettings.getRect(control);
	// Only adjust on first use
	if (rect.mLeft == 0 && rect.mBottom == 0)
	{
		S32 width = window.getWidth();
		S32 height = window.getHeight();
		rect.set(0, height-STATUS_BAR_HEIGHT, width, TOOL_BAR_HEIGHT);
		// Make floater fill 80% of window, leaving 20% padding on
		// the sides.
		const F32 ZOOM_FRACTION = 0.8f;
		S32 dx = (S32)(width * (1.f - ZOOM_FRACTION));
		S32 dy = (S32)(height * (1.f - ZOOM_FRACTION));
		rect.stretch(-dx/2, -dy/2);
		gSavedSettings.setRect(control, rect);
	}
}


// Many rectangles can't be placed until we know the screen size.
// These rectangles have their bottom-left corner as 0,0
void LLViewerWindow::adjustRectanglesForFirstUse(const LLRect& window)
{
	LLRect r;

	adjust_rect_bottom_center("FloaterMoveRect2", window);

	adjust_rect_bottom_center("FloaterCameraRect2", window);

	adjust_rect_top_left("FloaterCustomizeAppearanceRect", window);

	adjust_rect_top_left("FloaterLandRect5", window);

	adjust_rect_top_left("FloaterHUDRect", window);

	adjust_rect_top_left("FloaterFindRect2", window);

	adjust_rect_top_left("FloaterGestureRect2", window);

	adjust_rect_top_right("FloaterMiniMapRect", window);
	
	adjust_rect_top_right("FloaterLagMeter", window);

	adjust_rect_top_left("FloaterBuildOptionsRect", window);

	// bottom-right
	r = gSavedSettings.getRect("FloaterInventoryRect");
	if (r.mLeft == 0 && r.mBottom == 0)
	{
		r.setOriginAndSize(
			window.getWidth() - r.getWidth(),
			0,
			r.getWidth(),
			r.getHeight());
		gSavedSettings.setRect("FloaterInventoryRect", r);
	}
}


void LLViewerWindow::initWorldUI()
{
	pre_init_menus();

	S32 height = mRootView->getRect().getHeight();
	S32 width = mRootView->getRect().getWidth();
	LLRect full_window(0, height, width, 0);

	if ( gBottomPanel == NULL )			// Don't re-enter if objects are alreay created
	{
		// panel containing chatbar, toolbar, and overlay, over floaters
		gBottomPanel = new LLBottomPanel(mRootView->getRect());
		mRootView->addChild(gBottomPanel);

		// View for hover information
		gHoverView = new LLHoverView("gHoverView", full_window);
		gHoverView->setVisible(TRUE);
		mRootView->addChild(gHoverView);

		//
		// Map
		//
		// TODO: Move instance management into class
		gFloaterMap = new LLFloaterMap("Map");
		gFloaterMap->setFollows(FOLLOWS_TOP|FOLLOWS_RIGHT);

		// keep onscreen
		gFloaterView->adjustToFitScreen(gFloaterMap, FALSE);
		
		gIMMgr = LLIMMgr::getInstance();

		if ( gSavedPerAccountSettings.getBOOL("LogShowHistory") )
		{
			LLFloaterChat::getInstance(LLSD())->loadHistory();
		}

		LLRect morph_view_rect = full_window;
		morph_view_rect.stretch( -STATUS_BAR_HEIGHT );
		morph_view_rect.mTop = full_window.mTop - 32;
		gMorphView = new LLMorphView("gMorphView", morph_view_rect );
		mRootView->addChild(gMorphView);
		gMorphView->setVisible(FALSE);

		gFloaterMute = LLFloaterMute::getInstance();

		LLWorldMapView::initClass();

		adjust_rect_centered_partial_zoom("FloaterWorldMapRect2", full_window);

		gFloaterWorldMap = new LLFloaterWorldMap();
		gFloaterWorldMap->setVisible(FALSE);

		//
		// Tools for building
		//

		// Toolbox floater
		init_menus();

		gFloaterTools = new LLFloaterTools();
		gFloaterTools->setVisible(FALSE);

		// Status bar
		S32 menu_bar_height = gMenuBarView->getRect().getHeight();
		LLRect root_rect = gViewerWindow->getRootView()->getRect();
		LLRect status_rect(0, root_rect.getHeight(), root_rect.getWidth(), root_rect.getHeight() - menu_bar_height);
		gStatusBar = new LLStatusBar("status", status_rect);
		gStatusBar->setFollows(FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_TOP);

		gStatusBar->reshape(root_rect.getWidth(), gStatusBar->getRect().getHeight(), TRUE);
		gStatusBar->translate(0, root_rect.getHeight() - gStatusBar->getRect().getHeight());
		// sync bg color with menu bar
		gStatusBar->setBackgroundColor( gMenuBarView->getBackgroundColor() );

		LLFloaterChatterBox::createInstance(LLSD());

		gViewerWindow->getRootView()->addChild(gStatusBar);

		// menu holder appears on top to get first pass at all mouse events
		gViewerWindow->getRootView()->sendChildToFront(gMenuHolder);
	}
}


LLViewerWindow::~LLViewerWindow()
{
	delete mDebugText;
	
	gSavedSettings.setS32("FloaterViewBottom", gFloaterView->getRect().mBottom);

	// Cleanup global views
	if (gMorphView)
	{
		gMorphView->setVisible(FALSE);
	}
	
	// Delete all child views.
	delete mRootView;
	mRootView = NULL;

	// Automatically deleted as children of mRootView.  Fix the globals.
	gFloaterTools = NULL;
	gStatusBar = NULL;
	gFloaterCamera = NULL;
	gIMMgr = NULL;
	gHoverView = NULL;

	gFloaterView		= NULL;
	gMorphView			= NULL;

	gFloaterMute = NULL;

	gFloaterMap	= NULL;
	gHUDView = NULL;

	gNotifyBoxView = NULL;

	delete mToolTip;
	mToolTip = NULL;

	delete gResMgr;
	gResMgr = NULL;
	
	//--------------------------------------------------------
	// Shutdown GL cleanly.  Order is very important here.
	//--------------------------------------------------------
	LLFontGL::destroyDefaultFonts();
	LLFontManager::cleanupClass();
	stop_glerror();

	gSky.cleanup();
	stop_glerror();

	gImageList.shutdown();
	stop_glerror();

	gBumpImageList.shutdown();
	stop_glerror();

	LLWorldMapView::cleanupTextures();

	llinfos << "Cleaning up pipeline" << llendl;
	gPipeline.cleanup();
	stop_glerror();

	LLViewerImage::cleanupClass();
	
	delete[] mPickBuffer;
	mPickBuffer = NULL;

	if (gSelectMgr)
	{
		llinfos << "Cleaning up select manager" << llendl;
		gSelectMgr->cleanup();
	}

	LLVertexBuffer::cleanupClass();

	llinfos << "Stopping GL during shutdown" << llendl;
	if (!gNoRender)
	{
		stopGL(FALSE);
		stop_glerror();
	}


	llinfos << "Destroying Window" << llendl;
	destroyWindow();
}


void LLViewerWindow::setCursor( ECursorType c )
{
	mWindow->setCursor( c );
}

void LLViewerWindow::showCursor()
{
	mWindow->showCursor();
}

void LLViewerWindow::hideCursor()
{
	// Hide tooltips
	if(mToolTip ) mToolTip->setVisible( FALSE );

	// Also hide hover info
	if (gHoverView)	gHoverView->cancelHover();

	// And hide the cursor
	mWindow->hideCursor();
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
	U16 height16 = (U16) mWindowRect.getHeight();
	U16 width16 = (U16) mWindowRect.getWidth();
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

		glViewport(0, 0, width, height );

		if (height > 0 && gCamera)
		{ 
			gCamera->setViewHeightInPixels( height );
			if (mWindow->getFullscreen())
			{
				// force to 4:3 aspect for odd resolutions
				gCamera->setAspect( getDisplayAspectRatio() );
			}
			else
			{
				gCamera->setAspect( width / (F32) height);
			}
		}

		// update our window rectangle
		mWindowRect.mRight = mWindowRect.mLeft + width;
		mWindowRect.mTop = mWindowRect.mBottom + height;
		calcDisplayScale();
	
		BOOL display_scale_changed = mDisplayScale != LLUI::sGLScaleFactor;
		LLUI::setScaleFactor(mDisplayScale);

		// update our window rectangle
		mVirtualWindowRect.mRight = mVirtualWindowRect.mLeft + llround((F32)width / mDisplayScale.mV[VX]);
		mVirtualWindowRect.mTop = mVirtualWindowRect.mBottom + llround((F32)height / mDisplayScale.mV[VY]);

		setupViewport();

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
		gSavedSettings.setBOOL("FullScreen", mWantFullscreen);

		// store new settings for the mode we are in, regardless
		if (mWindow->getFullscreen())
		{
			gSavedSettings.setS32("FullScreenWidth", width);
			gSavedSettings.setS32("FullScreenHeight", height);
		}
		else
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

		gViewerStats->setStat(LLViewerStats::ST_WINDOW_WIDTH, (F64)width);
		gViewerStats->setStat(LLViewerStats::ST_WINDOW_HEIGHT, (F64)height);
		gResizeScreenTexture = TRUE;
	}
}


// Hide normal UI when a logon fails
void LLViewerWindow::setNormalControlsVisible( BOOL visible )
{
	if ( gBottomPanel )
	{
		gBottomPanel->setVisible( visible );
		gBottomPanel->setEnabled( visible );
	}

	if ( gMenuBarView )
	{
		gMenuBarView->setVisible( visible );
		gMenuBarView->setEnabled( visible );

		// ...and set the menu color appropriately.
		setMenuBackgroundColor(gAgent.getGodLevel() > GOD_NOT, 
			LLAppViewer::instance()->isInProductionGrid());
	}
        
	if ( gStatusBar )
	{
		gStatusBar->setVisible( visible );	
		gStatusBar->setEnabled( visible );	
	}
}

void LLViewerWindow::setMenuBackgroundColor(bool god_mode, bool dev_grid)
{
   	LLString::format_map_t args;
    LLColor4 new_bg_color;

    if(god_mode && LLAppViewer::instance()->isInProductionGrid())
    {
        new_bg_color = gColors.getColor( "MenuBarGodBgColor" );
    }
    else if(god_mode && !LLAppViewer::instance()->isInProductionGrid())
    {
        new_bg_color = gColors.getColor( "MenuNonProductionGodBgColor" );
    }
    else if(!god_mode && !LLAppViewer::instance()->isInProductionGrid())
    {
        new_bg_color = gColors.getColor( "MenuNonProductionBgColor" );
    }
    else 
    {
        new_bg_color = gColors.getColor( "MenuBarBgColor" );
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
	gGL.start();
	gGL.pushMatrix();
	{
		// scale view by UI global scale factor and aspect ratio correction factor
		glScalef(mDisplayScale.mV[VX], mDisplayScale.mV[VY], 1.f);
		mDebugText->draw();
	}
	gGL.popMatrix();
	gGL.stop();
}

void LLViewerWindow::draw()
{
	
#if LL_DEBUG
	LLView::sIsDrawing = TRUE;
#endif
	stop_glerror();
	
	LLUI::setLineWidth(1.f);
	//popup alerts from the UI
	LLAlertInfo alert;
	while (LLPanel::nextAlert(alert))
	{
		alertXml(alert.mLabel, alert.mArgs);
	}

	LLUI::setLineWidth(1.f);
	// Reset any left-over transforms
	glMatrixMode(GL_MODELVIEW);
	
	glLoadIdentity();

	//S32 screen_x, screen_y;

	// HACK for timecode debugging
	if (gSavedSettings.getBOOL("DisplayTimecode"))
	{
		// draw timecode block
		char text[256];		/* Flawfinder: ignore */

		glLoadIdentity();

		microsecondsToTimecodeString(gFrameTime,text);
		const LLFontGL* font = gResMgr->getRes( LLFONT_SANSSERIF );
		font->renderUTF8(text, 0,
						llround((getWindowWidth()/2)-100.f),
						llround((getWindowHeight()-60.f)),
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
		if (gCamera)
		{
			// apply camera zoom transform (for high res screenshots)
			F32 zoom_factor = gCamera->getZoomFactor();
			S16 sub_region = gCamera->getZoomSubRegion();
			if (zoom_factor > 1.f)
			{
				//decompose subregion number to x and y values
				int pos_y = sub_region / llceil(zoom_factor);
				int pos_x = sub_region - (pos_y*llceil(zoom_factor));
				// offset for this tile
				glTranslatef((F32)gViewerWindow->getWindowWidth() * -(F32)pos_x, 
							(F32)gViewerWindow->getWindowHeight() * -(F32)pos_y, 
							0.f);
				glScalef(zoom_factor, zoom_factor, 1.f);
				LLUI::sGLScaleFactor *= zoom_factor;
			}
		}

		if (gToolMgr)
		{
			// Draw tool specific overlay on world
			gToolMgr->getCurrentTool()->draw();
		}

		if( gAgent.cameraMouselook() )
		{
			drawMouselookInstructions();
			stop_glerror();
		}

		// Draw all nested UI views.
		// No translation needed, this view is glued to 0,0
		mRootView->draw();

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

		// Draw tooltips
		// Adjust their rectangle so they don't go off the top or bottom
		// of the screen.
		if( mToolTip && mToolTip->getVisible() )
		{
			glMatrixMode(GL_MODELVIEW);
			LLUI::pushMatrix();
			{
				S32 tip_height = mToolTip->getRect().getHeight();

				S32 screen_x, screen_y;
				mToolTip->localPointToScreen(0, -24 - tip_height, 
											 &screen_x, &screen_y);

				// If tooltip would draw off the bottom of the screen,
				// show it from the cursor tip position.
				if (screen_y < tip_height) 
				{
					mToolTip->localPointToScreen(0, 0, &screen_x, &screen_y);
				}
				LLUI::translate( (F32) screen_x, (F32) screen_y, 0);
				mToolTip->draw();
			}
			LLUI::popMatrix();
		}

		if( gShowOverlayTitle && !mOverlayTitle.empty() )
		{
			// Used for special titles such as "Second Life - Special E3 2003 Beta"
			const S32 DIST_FROM_TOP = 20;
			LLFontGL::sSansSerifBig->renderUTF8(
				mOverlayTitle, 0,
				llround( gViewerWindow->getWindowWidth() * 0.5f),
				gViewerWindow->getWindowHeight() - DIST_FROM_TOP,
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
	if (gFocusMgr.getKeyboardFocus() && !(mask &	 (MASK_CONTROL | MASK_ALT)))
	{
		// We have keyboard focus, and it's not an accelerator

		if (key < 0x80)
		{
			// Not a special key, so likely (we hope) to generate a character.  Let it fall through to character handler first.
			return gFocusMgr.childHasKeyboardFocus(mRootView);
		}
	}

	// HACK look for UI editing keys
	if (LLView::sEditingUI)
	{
		if (LLFloaterEditUI::handleKey(key, mask))
		{
			return TRUE;
		}
	}

	// Hide tooltips on keypress
	if(mToolTip )
	{
		mToolTipBlocked = TRUE; // block until next time mouse is moved
		mToolTip->setVisible( FALSE );
	}

	// Also hide hover info on keypress
	if (gHoverView)
	{
		gHoverView->cancelHover();

		gHoverView->setTyping(TRUE);
	}

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

	// Example "bug" for bug reporter web page
	if ((MASK_SHIFT & mask) 
		&& (MASK_ALT & mask)
		&& (MASK_CONTROL & mask)
		&& ('H' == key || 'h' == key))
	{
		trigger_hippo_bug(NULL);
	}

	// handle escape key
	if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		if (gMenuHolder && gMenuHolder->hideMenus())
		{
			return TRUE;
		}

		//if quit from menu, turn off the Keyboardmode for the menu.
		if(LLMenuGL::getKeyboardMode())
			LLMenuGL::setKeyboardMode(FALSE);

		if (gFocusMgr.getTopCtrl())
		{
			gFocusMgr.setTopCtrl(NULL);
			return TRUE;
		}

		// *TODO: get this to play well with mouselook and hidden
		// cursor modes, etc, and re-enable.
		//if (gFocusMgr.getMouseCapture())
		//{
		//	gFocusMgr.setMouseCapture(NULL);
		//	return TRUE;
		//}
	}

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

	// Traverses up the hierarchy
	LLUICtrl* keyboard_focus = gFocusMgr.getKeyboardFocus();
	if( keyboard_focus )
	{
		// arrow keys move avatar while chatting hack
		if (gChatBar && gChatBar->inputEditorHasFocus())
		{
			if (gChatBar->getCurrentChat().empty() || gSavedSettings.getBOOL("ArrowKeysMoveAvatar"))
			{
				switch(key)
				{
				case KEY_LEFT:
				case KEY_RIGHT:
				case KEY_UP:
					// let CTRL UP through for chat line history
					if( MASK_CONTROL == mask )
					{
						break;
					}
				case KEY_DOWN:
					// let CTRL DOWN through for chat line history
					if( MASK_CONTROL == mask )
					{
						break;
					}
				case KEY_PAGE_UP:
				case KEY_PAGE_DOWN:
				case KEY_HOME:
					// when chatbar is empty or ArrowKeysMoveAvatar set, pass arrow keys on to avatar...
					return FALSE;
				default:
					break;
				}
			}
		}

		if (keyboard_focus->handleKey(key, mask, FALSE))
		{
			return TRUE;
		}
	}

	if (gToolMgr)
	{
		if( gToolMgr->getCurrentTool()->handleKey(key, mask) )
		{
			return TRUE;
		}
	}

	// Try for a new-format gesture
	if (gGestureManager.triggerGesture(key, mask))
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
	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
	if (top_ctrl)
	{
		if( top_ctrl->handleKey( key, mask, TRUE ) )
		{
			return TRUE;
		}
	}

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
	LLView* keyboard_focus = gFocusMgr.getKeyboardFocus();
	if( keyboard_focus )
	{
		if (keyboard_focus->handleUnicodeChar(uni_char, FALSE))
		{
			return TRUE;
		}

		// Topmost view gets a chance before the hierarchy
		LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();
		if (top_ctrl && top_ctrl->handleUnicodeChar( uni_char, FALSE ) )
		{
			return TRUE;
		}

		return TRUE;
	}

	return FALSE;
}


void LLViewerWindow::handleScrollWheel(S32 clicks)
{
	LLView::sMouseHandlerMessage = "";

	gMouseIdleTimer.reset();

	// Hide tooltips
	if( mToolTip )
	{
		mToolTip->setVisible( FALSE );
	}

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

	if (gWorldPointer)
	{
		// Zoom the camera in and out behavior
		gAgent.handleScrollWheel(clicks);
	}

	return;
}

void LLViewerWindow::moveCursorToCenter()
{
	S32 x = mVirtualWindowRect.getWidth() / 2;
	S32 y = mVirtualWindowRect.getHeight() / 2;
	
	//on a forced move, all deltas get zeroed out to prevent jumping
	mCurrentMousePoint.set(x,y);
	mLastMousePoint.set(x,y);
	mCurrentMouseDelta.set(0,0);	

	LLUI::setCursorPositionScreen(x, y);	
}

//////////////////////////////////////////////////////////////////////
//
// Hover handlers
//

// Update UI based on stored mouse position from mouse-move
// event processing.
BOOL LLViewerWindow::handlePerFrameHover()
{
	static std::string last_handle_msg;

	LLView::sMouseHandlerMessage = "";

	//RN: fix for asynchronous notification of mouse leaving window not working
	LLCoordWindow mouse_pos;
	mWindow->getCursorPosition(&mouse_pos);
	if (mouse_pos.mX < 0 || 
		mouse_pos.mY < 0 ||
		mouse_pos.mX > mWindowRect.getWidth() ||
		mouse_pos.mY > mWindowRect.getHeight())
	{
		mMouseInWindow = FALSE;
	}
	else
	{
		mMouseInWindow = TRUE;
	}

	S32 dx = lltrunc((F32) (mCurrentMousePoint.mX - mLastMousePoint.mX) * LLUI::sGLScaleFactor.mV[VX]);
	S32 dy = lltrunc((F32) (mCurrentMousePoint.mY - mLastMousePoint.mY) * LLUI::sGLScaleFactor.mV[VY]);

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

	if (gNoRender)
	{
		return TRUE;
	}

	S32 x = mCurrentMousePoint.mX;
	S32 y = mCurrentMousePoint.mY;
	MASK mask = gKeyboard->currentMask(TRUE);

	// clean up current focus
	LLUICtrl* cur_focus = gFocusMgr.getKeyboardFocus();
	if (cur_focus)
	{
		if (!cur_focus->isInVisibleChain() || !cur_focus->isInEnabledChain())
		{
			gFocusMgr.releaseFocusIfNeeded(cur_focus);

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
		}
		else if (cur_focus->isFocusRoot())
		{
			// focus roots keep trying to delegate focus to their first valid descendant
			// this assumes that focus roots are not valid focus holders on their own
			cur_focus->focusFirstItem();
		}
	}

	gPipeline.sRenderProcessBeacons = FALSE;
	KEY key = gKeyboard->currentKey();
	if (((mask & MASK_CONTROL) && ('N' == key || 'n' == key)) || gSavedSettings.getBOOL("BeaconAlwaysOn"))
	{
		gPipeline.sRenderProcessBeacons = TRUE;
	}

	BOOL handled = FALSE;

	BOOL handled_by_top_ctrl = FALSE;
	LLUICtrl* top_ctrl = gFocusMgr.getTopCtrl();

	LLMouseHandler* mouse_captor = gFocusMgr.getMouseCapture();
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
				if (last_handle_msg != "")
				{
					last_handle_msg = "";
					llinfos << "Hover not handled by view" << llendl;
				}
			}
		}

		if( !handled )
		{
			lldebugst(LLERR_USER_INPUT) << "hover not handled by top view or root" << llendl;		
		}
	}

	// *NOTE: sometimes tools handle the mouse as a captor, so this
	// logic is a little confusing
	LLTool *tool = NULL;
	if (gToolMgr && gHoverView && gCamera)
	{
		tool = gToolMgr->getCurrentTool();

		if(!handled && tool)
		{
			handled = tool->handleHover(x, y, mask);

			if (!mWindow->isCursorHidden())
			{
				gHoverView->updateHover(tool);
			}
		}
		else
		{
			// Cancel hovering if any UI element handled the event.
			gHoverView->cancelHover();
		}

		// Suppress the toolbox view if our source tool was the pie tool,
		// and we've overridden to something else.
		mSuppressToolbox = 
			(gToolMgr->getBaseTool() == gToolPie) &&
			(gToolMgr->getCurrentTool() != gToolPie);

	}

	//llinfos << (mToolTipBlocked ? "BLOCKED" : "NOT BLOCKED") << llendl;
	// Show a new tool tip (or update one that is alrady shown)
	BOOL tool_tip_handled = FALSE;
	LLString tool_tip_msg;
	F32 tooltip_delay = gSavedSettings.getF32( "ToolTipDelay" );
	//HACK: hack for tool-based tooltips which need to pop up more quickly
	//Also for show xui names as tooltips debug mode
	if ((mouse_captor && !mouse_captor->isView()) || LLUI::sShowXUINames)
	{
		tooltip_delay = gSavedSettings.getF32( "DragAndDropToolTipDelay" );
	}
	if( handled && 
		!mToolTipBlocked &&
		(gMouseIdleTimer.getElapsedTimeF32() > tooltip_delay) &&
		!mWindow->isCursorHidden() )
	{
		LLRect screen_sticky_rect;

		if (mouse_captor)
		{
			S32 local_x, local_y;
			mouse_captor->screenPointToLocal( x, y, &local_x, &local_y );
			tool_tip_handled = mouse_captor->handleToolTip( local_x, local_y, tool_tip_msg, &screen_sticky_rect );
		}
		else if (handled_by_top_ctrl)
		{
			S32 local_x, local_y;
			top_ctrl->screenPointToLocal( x, y, &local_x, &local_y );
			tool_tip_handled = top_ctrl->handleToolTip( local_x, local_y, tool_tip_msg, &screen_sticky_rect );
		}
		else
		{
			tool_tip_handled = mRootView->handleToolTip(x, y, tool_tip_msg, &screen_sticky_rect );
		}

		if( tool_tip_handled && !tool_tip_msg.empty() )
		{
			mToolTipStickyRect = screen_sticky_rect;
			mToolTip->setWrappedText( tool_tip_msg, 200 );
			mToolTip->reshapeToFitText();
			mToolTip->setOrigin( x, y );
			LLRect virtual_window_rect(0, getWindowHeight(), getWindowWidth(), 0);
			mToolTip->translateIntoRect( virtual_window_rect, FALSE );
			mToolTip->setVisible( TRUE );
		}
	}		
	
	if (tool && tool != gToolNull  && tool != gToolInspect && tool != gToolDragAndDrop && !gSavedSettings.getBOOL("FreezeTime"))
	{ 
		LLMouseHandler *captor = gFocusMgr.getMouseCapture();
		// With the null, inspect, or drag and drop tool, don't muck
		// with visibility.

		if (gFloaterTools->isMinimized() ||
			(tool != gToolPie						// not default tool
			&& tool != gToolGun						// not coming out of mouselook
			&& !mSuppressToolbox					// not override in third person
			&& gToolMgr->getCurrentToolset() != gFaceEditToolset	// not special mode
			&& gToolMgr->getCurrentToolset() != gMouselookToolset
			&& (!captor || captor->isView())) // not dragging
			)
		{
			// Force floater tools to be visible (unless minimized)
			if (!gFloaterTools->getVisible())
			{
				gFloaterTools->open();		/* Flawfinder: ignore */
			}
			// Update the location of the blue box tool popup
			LLCoordGL select_center_screen;
			gFloaterTools->updatePopup( select_center_screen, mask );
		}
		else
		{
			gFloaterTools->setVisible(FALSE);
		}
		// In the future we may wish to hide the tools menu unless you
		// are building. JC
		//gMenuBarView->setItemVisible("Tools", gFloaterTools->getVisible());
		//gMenuBarView->arrange();
	}
	if (gToolBar)
	{
		gToolBar->refresh();
	}

	if (gChatBar)
	{
		gChatBar->refresh();
	}

	if (gOverlayBar)
	{
		gOverlayBar->refresh();
	}

	// Update rectangles for the various toolbars
	if (gOverlayBar && gNotifyBoxView && gConsole && gToolBar)
	{
		LLRect bar_rect(-1, STATUS_BAR_HEIGHT, getWindowWidth()+1, -1);

		LLRect notify_box_rect = gNotifyBoxView->getRect();
		notify_box_rect.mBottom = bar_rect.mBottom;
		gNotifyBoxView->reshape(notify_box_rect.getWidth(), notify_box_rect.getHeight());
		gNotifyBoxView->setRect(notify_box_rect);

		// make sure floaters snap to visible rect by adjusting floater view rect
		LLRect floater_rect = gFloaterView->getRect();
		if (floater_rect.mBottom != bar_rect.mBottom+1)
		{
			floater_rect.mBottom = bar_rect.mBottom+1;
			// Don't bounce the floaters up and down.
			gFloaterView->reshape(floater_rect.getWidth(), floater_rect.getHeight(), 
									TRUE, ADJUST_VERTICAL_NO);
			gFloaterView->setRect(floater_rect);
		}

		// snap floaters to top of chat bar/button strip
		LLView* chatbar_and_buttons = gOverlayBar->getChild<LLView>("chatbar_and_buttons", TRUE);
		// find top of chatbar and strate buttons, if either are visible
		if (chatbar_and_buttons && !chatbar_and_buttons->getLocalBoundingRect().isNull())
		{
			// convert top/left corner of chatbar/buttons container to gFloaterView-relative coordinates
			S32 top, left;
			chatbar_and_buttons->localPointToOtherView(
												chatbar_and_buttons->getLocalBoundingRect().mLeft, 
												chatbar_and_buttons->getLocalBoundingRect().mTop,
												&left,
												&top,
												gFloaterView);
			gFloaterView->setSnapOffsetBottom(top);
		}
		else if (gToolBar->getVisible())
		{
			S32 top, left;
			gToolBar->localPointToOtherView(
											gToolBar->getLocalBoundingRect().mLeft,
											gToolBar->getLocalBoundingRect().mTop,
											&left,
											&top,
											gFloaterView);
			gFloaterView->setSnapOffsetBottom(top);
		}
		else
		{
			gFloaterView->setSnapOffsetBottom(0);
		}

		// Always update console
		LLRect console_rect = gConsole->getRect();
		console_rect.mBottom = gHUDView->getRect().mBottom + CONSOLE_BOTTOM_PAD;
		gConsole->reshape(console_rect.getWidth(), console_rect.getHeight());
		gConsole->setRect(console_rect);
	}

	mLastMousePoint = mCurrentMousePoint;

	// last ditch force of edit menu to selection manager
	if (LLEditMenuHandler::gEditMenuHandler == NULL && gSelectMgr && gSelectMgr->getSelection()->getObjectCount())
	{
		LLEditMenuHandler::gEditMenuHandler = gSelectMgr;
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

	if (gSavedSettings.getBOOL("ChatBarStealsFocus") && gChatBar && gFocusMgr.getKeyboardFocus() == NULL && gChatBar->getVisible())
	{
		gChatBar->startChat(NULL);
	}

	// cleanup unused selections when no modal dialogs are open
	if (gParcelMgr && LLModalDialog::activeCount() == 0)
	{
		gParcelMgr->deselectUnused();
	}

	if (gSelectMgr && LLModalDialog::activeCount() == 0)
	{
		gSelectMgr->deselectUnused();
	}

	return handled;
}


void LLViewerWindow::saveLastMouse(const LLCoordGL &point)
{
	// Store last mouse location.
	// If mouse leaves window, pretend last point was on edge of window
	if (point.mX < 0)
	{
		mCurrentMousePoint.mX = 0;
	}
	else if (point.mX > getWindowWidth())
	{
		mCurrentMousePoint.mX = getWindowWidth();
	}
	else
	{
		mCurrentMousePoint.mX = point.mX;
	}

	if (point.mY < 0)
	{
		mCurrentMousePoint.mY = 0;
	}
	else if (point.mY > getWindowHeight() )
	{
		mCurrentMousePoint.mY = getWindowHeight();
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
	LLObjectSelectionHandle selection = gSelectMgr->getSelection();

	if (!for_hud && !for_gl_pick)
	{
		// Call this once and only once
		gSelectMgr->updateSilhouettes();
	}
	
	// Draw fence around land selections
	if (for_gl_pick)
	{
		if (pick_parcel_walls)
		{
			gParcelMgr->renderParcelCollision();
		}
	}
	else if (( for_hud && selection->getSelectType() == SELECT_TYPE_HUD) ||
			 (!for_hud && selection->getSelectType() != SELECT_TYPE_HUD))
	{		
		gSelectMgr->renderSilhouettes(for_hud);
		
		stop_glerror();

		// setup HUD render
		if (selection->getSelectType() == SELECT_TYPE_HUD && gSelectMgr->getSelection()->getObjectCount())
		{
			LLBBox hud_bbox = gAgent.getAvatarObject()->getHUDBBox();

			// set up transform to encompass bounding box of HUD
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			F32 depth = llmax(1.f, hud_bbox.getExtentLocal().mV[VX] * 1.1f);
			glOrtho(-0.5f * gCamera->getAspect(), 0.5f * gCamera->getAspect(), -0.5f, 0.5f, 0.f, depth);
			
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glLoadMatrixf(OGL_TO_CFR_ROTATION);		// Load Cory's favorite reference frame
			glTranslatef(-hud_bbox.getCenterLocal().mV[VX] + (depth *0.5f), 0.f, 0.f);
		}

		// Render light for editing
		if (LLSelectMgr::sRenderLightRadius && gToolMgr->inEdit())
		{
			LLImageGL::unbindTexture(0);
			LLGLEnable gls_blend(GL_BLEND);
			LLGLEnable gls_cull(GL_CULL_FACE);
			LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			if (selection->getSelectType() == SELECT_TYPE_HUD)
			{
				F32 zoom = gAgent.getAvatarObject()->mHUDCurZoom;
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
			gSelectMgr->getSelection()->applyToObjects(&func);
			
			glPopMatrix();
		}				
		
		// NOTE: The average position for the axis arrows of the selected objects should
		// not be recalculated at this time.  If they are, then group rotations will break.

		// Draw arrows at average center of all selected objects
		LLTool* tool = gToolMgr->getCurrentTool();
		if (tool)
		{
			if(tool->isAlwaysRendered())
			{
				tool->render();
			}
			else
			{
				if( !gSelectMgr->getSelection()->isEmpty() )
				{
					BOOL moveable_object_selected = FALSE;
					BOOL all_selected_objects_move = TRUE;
					BOOL all_selected_objects_modify = TRUE;
					BOOL selecting_linked_set = !gSavedSettings.getBOOL("EditLinkedParts");

					for (LLObjectSelection::iterator iter = gSelectMgr->getSelection()->begin();
						 iter != gSelectMgr->getSelection()->end(); iter++)
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

					if (tool == gToolTranslate && (!moveable_object_selected || !all_selected_objects_move))
					{
						draw_handles = FALSE;
					}

					if (tool == gToolRotate && (!moveable_object_selected || !all_selected_objects_move))
					{
						draw_handles = FALSE;
					}

					if ( !all_selected_objects_modify && tool == gToolStretch )
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

void LLViewerWindow::hitObjectOrLandGlobalAsync(S32 x, S32 y_from_bot, MASK mask, void (*callback)(S32 x, S32 y, MASK mask), BOOL pick_transparent, BOOL pick_parcel_walls)
{
	if (gNoRender)
	{
		return;
	}
	
	render_ui_and_swap_if_needed();
	glClear(GL_DEPTH_BUFFER_BIT);
	gDisplaySwapBuffers = FALSE;

	S32 scaled_x = llround((F32)x * mDisplayScale.mV[VX]);
	S32 scaled_y = llround((F32)y_from_bot * mDisplayScale.mV[VY]);

	BOOL in_build_mode = gFloaterTools && gFloaterTools->getVisible();
	if (in_build_mode || LLDrawPoolAlpha::sShowDebugAlpha)
	{
		// build mode allows interaction with all transparent objects
		// "Show Debug Alpha" means no object actually transparent
		pick_transparent = TRUE;
	}
	gPickTransparent = pick_transparent;

	gUseGLPick = FALSE;
	mPickCallback = callback;

	// Default to not hitting anything
	gLastHitPosGlobal.zeroVec();
	gLastHitObjectOffset.zeroVec();
	gLastHitObjectID.setNull();
	gLastHitObjectFace = -1;

	gLastHitNonFloraPosGlobal.zeroVec();
	gLastHitNonFloraObjectOffset.zeroVec();
	gLastHitNonFloraObjectID.setNull();
	gLastHitNonFloraObjectFace = -1;

	gLastHitParcelWall = FALSE;

	LLCamera pick_camera;
	pick_camera.setOrigin(gCamera->getOrigin());
	pick_camera.setOriginAndLookAt(gCamera->getOrigin(),
								   gCamera->getUpAxis(),
								   gCamera->getOrigin() + mouseDirectionGlobal(x, y_from_bot));
	pick_camera.setView(0.5f*DEG_TO_RAD);
	pick_camera.setNear(gCamera->getNear());
	pick_camera.setFar(gCamera->getFar());
	pick_camera.setAspect(1.f);

	// save our drawing state
	// *TODO: should we be saving using the new method here using
	// glh_get_current_projection/glh_set_current_projection? -brad
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	// build perspective transform and picking viewport
	// Perform pick on a PICK_DIAMETER x PICK_DIAMETER pixel region around cursor point.
	// Don't limit the select distance for this pick.
	// make viewport big enough to handle antialiased frame buffers
	gCamera->setPerspective(FOR_SELECTION, scaled_x - (PICK_HALF_WIDTH + 2), scaled_y - (PICK_HALF_WIDTH + 2), PICK_DIAMETER + 4, PICK_DIAMETER + 4, FALSE);
	// make viewport big enough to handle antialiased frame buffers
	gGLViewport[0] = scaled_x - (PICK_HALF_WIDTH + 2);
	gGLViewport[1] = scaled_y - (PICK_HALF_WIDTH + 2);
	gGLViewport[2] = PICK_DIAMETER + 4;
	gGLViewport[3] = PICK_DIAMETER + 4;
	glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
	LLViewerCamera::updateFrustumPlanes(pick_camera);
	stop_glerror();

	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	//glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Draw the objects so the user can select them.
	// The starting ID is 1, since land is zero.
	gObjectList.renderObjectsForSelect(pick_camera, pick_parcel_walls);

	stop_glerror();

	// restore drawing state
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	setupViewport();

	mPickPoint.set(x, y_from_bot);
	mPickOffset.set(0, 0);
	mPickMask = mask;
	mPickPending = TRUE;

	// delay further event processing until we receive results of pick
	mWindow->delayInputProcessing();
}

void LLViewerWindow::hitUIElementImmediate(S32 x, S32 y, void (*callback)(S32 x, S32 y, MASK mask))
{
	// Performs the GL UI pick.
	// Stores its results in global, gLastHitUIElement
	if (gNoRender)
	{
		return;
	}
	
	hitUIElementAsync(x, y, gKeyboard->currentMask(TRUE), NULL);
	performPick();
	if (callback)
	{
		callback(x, y, gKeyboard->currentMask(TRUE));
	}
}

//RN: this currently doesn't do anything
void LLViewerWindow::hitUIElementAsync(S32 x, S32 y_from_bot, MASK mask, void (*callback)(S32 x, S32 y, MASK mask))
{
	if (gNoRender)
	{
		return;
	}

// 	F32 delta_time = gAlphaFadeTimer.getElapsedTimeAndResetF32();

	gUseGLPick = FALSE;
	mPickCallback = callback;

	// Default to not hitting anything
	gLastHitUIElement = 0;

	LLCamera pick_camera;
	pick_camera.setOrigin(gCamera->getOrigin());
	pick_camera.setOriginAndLookAt(gCamera->getOrigin(),
								   gCamera->getUpAxis(),
								   gCamera->getOrigin() + mouseDirectionGlobal(x, y_from_bot));
	pick_camera.setView(0.5f*DEG_TO_RAD);
	pick_camera.setNear(gCamera->getNear());
	pick_camera.setFar(gCamera->getFar());
	pick_camera.setAspect(1.f);

	// save our drawing state
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	// build orthogonal transform and picking viewport
	// Perform pick on a PICK_DIAMETER x PICK_DIAMETER pixel region around cursor point.
	// Don't limit the select distance for this pick.
	gViewerWindow->setup2DRender();
	const LLVector2& display_scale = gViewerWindow->getDisplayScale();
	glScalef(display_scale.mV[VX], display_scale.mV[VY], 1.f);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// make viewport big enough to handle antialiased frame buffers
	glViewport(x - (PICK_HALF_WIDTH + 2), y_from_bot - (PICK_HALF_WIDTH + 2), PICK_DIAMETER + 4, PICK_DIAMETER + 4);
	stop_glerror();

	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	// Draw the objects so the user can select them.
	// The starting ID is 1, since land is zero.
	//gViewerWindow->drawForSelect();

	stop_glerror();

	// restore drawing state
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	setupViewport();

	mPickPoint.set(x, y_from_bot);
	mPickOffset.set(0, 0);
	mPickMask = mask;
	mPickPending = TRUE;
}

void LLViewerWindow::performPick()
{
	if (gNoRender || !mPickPending)
	{
		return;
	}

	mPickPending = FALSE;
	U32	te_offset = NO_FACE;
	
	// find pick region that is fully onscreen
	LLCoordGL scaled_pick_point = mPickPoint;
	scaled_pick_point.mX = llclamp(llround((F32)mPickPoint.mX * mDisplayScale.mV[VX]), PICK_HALF_WIDTH, gViewerWindow->getWindowDisplayWidth() - PICK_HALF_WIDTH);
	scaled_pick_point.mY = llclamp(llround((F32)mPickPoint.mY * mDisplayScale.mV[VY]), PICK_HALF_WIDTH, gViewerWindow->getWindowDisplayHeight() - PICK_HALF_WIDTH);

	glReadPixels(scaled_pick_point.mX - PICK_HALF_WIDTH, scaled_pick_point.mY - PICK_HALF_WIDTH, PICK_DIAMETER, PICK_DIAMETER, GL_RGBA, GL_UNSIGNED_BYTE, mPickBuffer);

	S32 pixel_index = PICK_HALF_WIDTH * PICK_DIAMETER + PICK_HALF_WIDTH;
	S32 name = (U32)mPickBuffer[(pixel_index * 4) + 0] << 16 | (U32)mPickBuffer[(pixel_index * 4) + 1] << 8 | (U32)mPickBuffer[(pixel_index * 4) + 2];
	gLastPickAlpha = mPickBuffer[(pixel_index * 4) + 3];

	if (name >= (S32)GL_NAME_UI_RESERVED && name < (S32)GL_NAME_INDEX_OFFSET)
	{
		// hit a UI element
		gLastHitUIElement = name;
		if (mPickCallback)
		{
			mPickCallback(mPickPoint.mX, mPickPoint.mY, mPickMask);
		}
	}

	//imdebug("rgba rbga=bbba b=8 w=%d h=%d %p", PICK_DIAMETER, PICK_DIAMETER, mPickBuffer);

	S32 x_offset = mPickPoint.mX - llround((F32)scaled_pick_point.mX / mDisplayScale.mV[VX]);
	S32 y_offset = mPickPoint.mY - llround((F32)scaled_pick_point.mY / mDisplayScale.mV[VY]);

	
	// we hit nothing, scan surrounding pixels for something useful
	if (!name)
	{
		S32 closest_distance = 10000;
		//S32 closest_pick_name = 0;
		for (S32 col = 0; col < PICK_DIAMETER; col++)
		{
			for (S32 row = 0; row < PICK_DIAMETER; row++)
			{
				S32 distance_squared = (llabs(col - x_offset - PICK_HALF_WIDTH) * llabs(col - x_offset - PICK_HALF_WIDTH)) + (llabs(row - y_offset - PICK_HALF_WIDTH) * llabs(row - y_offset - PICK_HALF_WIDTH));
				pixel_index = row * PICK_DIAMETER + col;
				S32 test_name = (U32)mPickBuffer[(pixel_index * 4) + 0] << 16 | (U32)mPickBuffer[(pixel_index * 4) + 1] << 8 | (U32)mPickBuffer[(pixel_index * 4) + 2];
				gLastPickAlpha = mPickBuffer[(pixel_index * 4) + 3];
				if (test_name && distance_squared < closest_distance)
				{
					closest_distance = distance_squared;
					name = test_name;
					gLastPickAlpha = mPickBuffer[(pixel_index * 4) + 3];
					mPickOffset.mX = col - PICK_HALF_WIDTH;
					mPickOffset.mY = row - PICK_HALF_WIDTH;
				}
			}
		}
	}

	if (name)
	{
		mPickPoint.mX += llround((F32)mPickOffset.mX * mDisplayScale.mV[VX]);
		mPickPoint.mY += llround((F32)mPickOffset.mY * mDisplayScale.mV[VY]);
	}

	if (gPickFaces)
	{
		te_offset = ((U32)name >> 20);
		name &= 0x000fffff;
		// don't clear gPickFaces, as we still need to check for UV coordinates
	}

	LLViewerObject	*objectp = NULL;

	// Frontmost non-foreground object that isn't trees or grass
	LLViewerObject* nonflora_objectp = NULL;
	S32 nonflora_name = -1;
	S32 nonflora_te_offset = NO_FACE;

	if (name == (S32)GL_NAME_PARCEL_WALL)
	{
		gLastHitParcelWall = TRUE;
	}

	gLastHitHUDIcon = NULL;

	objectp = gObjectList.getSelectedObject(name);
	if (objectp)
	{
		LLViewerObject* parent = (LLViewerObject*)(objectp->getParent());
		if (NULL == parent) {
			// if you are the parent
			parent = objectp;
		}
		if (objectp->mbCanSelect)
		{
			te_offset = (te_offset == 16) ? NO_FACE : te_offset;

			// If the hit object isn't a plant, store it as the frontmost non-flora object.
			LLPCode pcode = objectp->getPCode();
			if( (LL_PCODE_LEGACY_GRASS != pcode) &&
				(LL_PCODE_LEGACY_TREE != pcode) &&
				(LL_PCODE_TREE_NEW != pcode))
			{
				nonflora_objectp = objectp;
				nonflora_name = name;
				nonflora_te_offset = te_offset;
			}
		}
		else
		{
			//llinfos << "Hit object you can't select" << llendl;
		}
	}
	else
	{
		// was this name referring to a hud icon?
		gLastHitHUDIcon = LLHUDIcon::handlePick(name);
	}

	analyzeHit( 
		mPickPoint.mX, mPickPoint.mY, objectp, te_offset,
		&gLastHitObjectID, &gLastHitObjectFace, &gLastHitPosGlobal, &gLastHitLand, &gLastHitUCoord, &gLastHitVCoord );

	if (objectp && !gLastHitObjectID.isNull())
	{
		gLastHitObjectOffset = gAgent.calcFocusOffset(objectp, mPickPoint.mX, mPickPoint.mY);
	}

	if( objectp == nonflora_objectp )
	{
		gLastHitNonFloraObjectID	= gLastHitObjectID;
		gLastHitNonFloraObjectFace	= gLastHitObjectFace;
		gLastHitNonFloraPosGlobal	= gLastHitPosGlobal;
		gLastHitNonFloraObjectOffset= gLastHitObjectOffset;
	}
	else
	{
		analyzeHit(  mPickPoint.mX, mPickPoint.mY, nonflora_objectp, nonflora_te_offset,
			&gLastHitNonFloraObjectID, &gLastHitNonFloraObjectFace, &gLastHitNonFloraPosGlobal,
			&gLastHitLand, &gLastHitUCoord, &gLastHitVCoord);

		if( nonflora_objectp )
		{
			gLastHitNonFloraObjectOffset = gAgent.calcFocusOffset(nonflora_objectp, mPickPoint.mX, mPickPoint.mY);
		}
	}

	if (mPickCallback)
	{
		mPickCallback(mPickPoint.mX, mPickPoint.mY, mPickMask);
	}

	gPickFaces = FALSE;
}

// Performs the GL object/land pick.
// Stores its results in globals, gHit*
void LLViewerWindow::hitObjectOrLandGlobalImmediate(S32 x, S32 y_from_bot, void (*callback)(S32 x, S32 y, MASK mask), BOOL pick_transparent)
{
	if (gNoRender)
	{
		return;
	}
	
	hitObjectOrLandGlobalAsync(x, y_from_bot, gKeyboard->currentMask(TRUE), NULL, pick_transparent);
	performPick();
	if (callback)
	{
		callback(x, y_from_bot, gKeyboard->currentMask(TRUE));
	}
}

LLViewerObject* LLViewerWindow::getObjectUnderCursor(const F32 depth)
{
	S32 x = getCurrentMouseX();
	S32 y = getCurrentMouseY();
	
	LLVector3		mouse_direction_global = mouseDirectionGlobal(x,y);
	LLVector3		camera_pos_global = gCamera->getOrigin();
	LLVector3		pick_end = camera_pos_global + mouse_direction_global * depth;
	LLVector3		collision_point;
	return gPipeline.pickObject(camera_pos_global, pick_end, collision_point);
}

void LLViewerWindow::analyzeHit(
	S32				x,				// input
	S32				y_from_bot,		// input
	LLViewerObject* objectp,		// input
	U32				te_offset,		// input
	LLUUID*			hit_object_id_p,// output
	S32*			hit_face_p,		// output
	LLVector3d*		hit_pos_p,		// output
	BOOL*			hit_land,		// output
	F32*			hit_u_coord,	// output
	F32*			hit_v_coord)	// output
{
	// Clean up inputs
	S32 face = -1;
	
	if (te_offset != NO_FACE ) 
	{
		face = te_offset;
	}

	*hit_land = FALSE;

	if (objectp)
	{
		if( objectp->getPCode() == LLViewerObject::LL_VO_SURFACE_PATCH )
		{
			// Hit land
			*hit_land = TRUE;

			// put global position into land_pos
			LLVector3d land_pos;
			if (mousePointOnLandGlobal(x, y_from_bot, &land_pos))
			{
				*hit_object_id_p = LLUUID::null;
				*hit_face_p = -1;

				// Fudge the land focus a little bit above ground.
				*hit_pos_p = land_pos + LLVector3d(0.f, 0.f, 0.1f);
				//llinfos << "DEBUG Hit Land " << *hit_pos_p  << llendl;
				return;
			}
			else
			{
				//llinfos << "Hit land but couldn't find position" << llendl;
				// Fall through to "Didn't hit anything"
			}
		}
		else
		{
			*hit_object_id_p = objectp->mID;
			*hit_face_p = face;

			// Hit an object
			if (objectp->isAvatar())
			{
				*hit_pos_p = gAgent.getPosGlobalFromAgent(((LLVOAvatar*)objectp)->mPelvisp->getWorldPosition());
			}
			else if (objectp->mDrawable.notNull())
			{
				*hit_pos_p = gAgent.getPosGlobalFromAgent(objectp->getRenderPosition());
			}
			else
			{
				// regular object
				*hit_pos_p = objectp->getPositionGlobal();
			}

			if (gPickFaces && face > -1 &&
				objectp->mDrawable.notNull() && objectp->getPCode() == LL_PCODE_VOLUME &&
				face < objectp->mDrawable->getNumFaces())
			{
				// render red-blue gradient to get 1/256 precision
				// then render green grid to get final 1/4096 precision
				S32 scaled_x = llround((F32)x * mDisplayScale.mV[VX]);
				S32 scaled_y = llround((F32)y_from_bot * mDisplayScale.mV[VY]);
				const S32 UV_PICK_WIDTH = 41;
				const S32 UV_PICK_HALF_WIDTH = (UV_PICK_WIDTH - 1) / 2;
				U8 uv_pick_buffer[UV_PICK_WIDTH * UV_PICK_WIDTH * 4];
				S32 pick_face = face;
				LLFace* facep = objectp->mDrawable->getFace(pick_face);
				gCamera->setPerspective(FOR_SELECTION, scaled_x - UV_PICK_HALF_WIDTH, scaled_y - UV_PICK_HALF_WIDTH, UV_PICK_WIDTH, UV_PICK_WIDTH, FALSE);
				glViewport(scaled_x - UV_PICK_HALF_WIDTH, scaled_y - UV_PICK_HALF_WIDTH, UV_PICK_WIDTH, UV_PICK_WIDTH);
				gPipeline.renderFaceForUVSelect(facep);

				glReadPixels(scaled_x - UV_PICK_HALF_WIDTH, scaled_y - UV_PICK_HALF_WIDTH, UV_PICK_WIDTH, UV_PICK_WIDTH, GL_RGBA, GL_UNSIGNED_BYTE, uv_pick_buffer);
				U8* center_pixel = &uv_pick_buffer[4 * ((UV_PICK_WIDTH * UV_PICK_HALF_WIDTH) + UV_PICK_HALF_WIDTH + 1)];
				*hit_u_coord = (F32)((center_pixel[VGREEN] & 0xf) + (16.f * center_pixel[VRED])) / 4095.f;
				*hit_v_coord = (F32)((center_pixel[VGREEN] >> 4) + (16.f * center_pixel[VBLUE])) / 4095.f;
			}
			else
			{
				*hit_u_coord = 0.f;
				*hit_v_coord = 0.f;
			}

			//llinfos << "DEBUG Hit Object " << *hit_pos_p << llendl;
			return;
		}
	}

	// Didn't hit anything.
	*hit_object_id_p = LLUUID::null;
	*hit_face_p = -1;
	*hit_pos_p = LLVector3d::zero;
	*hit_u_coord = 0.f;
	*hit_v_coord = 0.f;
	//llinfos << "DEBUG Hit Nothing " << llendl;
}

// Returns unit vector relative to camera
// indicating direction of point on screen x,y
LLVector3 LLViewerWindow::mouseDirectionGlobal(const S32 x, const S32 y) const
{
	// find vertical field of view
	F32			fov = gCamera->getView();

	// find screen resolution
	S32			height = getWindowHeight();
	S32			width = getWindowWidth();

	// calculate pixel distance to screen
	F32			distance = (height / 2.f) / (tan(fov / 2.f));

	// calculate click point relative to middle of screen
	F32			click_x = x - width / 2.f;
	F32			click_y = y - height / 2.f;

	// compute mouse vector
	LLVector3	mouse_vector =	distance * gCamera->getAtAxis()
								- click_x * gCamera->getLeftAxis()
								+ click_y * gCamera->getUpAxis();

	mouse_vector.normVec();

	return mouse_vector;
}


// Returns unit vector relative to camera in camera space
// indicating direction of point on screen x,y
LLVector3 LLViewerWindow::mouseDirectionCamera(const S32 x, const S32 y) const
{
	// find vertical field of view
	F32			fov_height = gCamera->getView();
	F32			fov_width = fov_height * gCamera->getAspect();

	// find screen resolution
	S32			height = getWindowHeight();
	S32			width = getWindowWidth();

	// calculate click point relative to middle of screen
	F32			click_x = (((F32)x / (F32)width) - 0.5f) * fov_width * -1.f;
	F32			click_y = (((F32)y / (F32)height) - 0.5f) * fov_height;

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

		regionp = gWorldPointer->resolveRegionGlobal(probe_point_region, probe_point_global);

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

			regionp = gWorldPointer->resolveRegionGlobal(probe_point_region, probe_point_global);

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
BOOL LLViewerWindow::saveImageNumbered(LLImageRaw *raw, const LLString& extension_in)
{
	if (! raw)
	{
		return FALSE;
	}

	LLString extension(extension_in);
	if (extension.empty())
	{
		extension = (gSavedSettings.getBOOL("CompressSnapshotsToDisk")) ? ".j2c" : ".bmp";
	}

	LLFilePicker::ESaveFilter pick_type;
	if (extension == ".j2c")
		pick_type = LLFilePicker::FFSAVE_J2C;
	else if (extension == ".bmp")
		pick_type = LLFilePicker::FFSAVE_BMP;
	else if (extension == ".tga")
		pick_type = LLFilePicker::FFSAVE_TGA;
	else
		pick_type = LLFilePicker::FFSAVE_ALL; // ???
	
	// Get a directory if this is the first time.
	if (strlen(sSnapshotDir) == 0)		/* Flawfinder: ignore */
	{
		LLString proposed_name( sSnapshotBaseName );
		proposed_name.append( extension );

		// pick a directory in which to save
		LLFilePicker& picker = LLFilePicker::instance();
		if (!picker.getSaveFile(pick_type, proposed_name.c_str()))
		{
			// Clicked cancel
			return FALSE;
		}

		// Copy the directory + file name
		char directory[LL_MAX_PATH];		/* Flawfinder: ignore */
		strncpy(directory, picker.getFirstFile(), LL_MAX_PATH -1);		/* Flawfinder: ignore */
		directory[LL_MAX_PATH -1] = '\0';

		// Smash the file extension
		S32 length = strlen(directory);		/* Flawfinder: ignore */
		S32 index = length;

		// Back up over extension
		index -= extension.length();
		if (index >= 0 && directory[index] == '.')
		{
			directory[index] = '\0';
		}
		else
		{
			index = length;
		}

		// Find trailing backslash
		while (index >= 0 && directory[index] != gDirUtilp->getDirDelimiter()[0])
		{
			index--;
		}

		// If we found one, truncate the string there
		if (index >= 0)
		{
			if (index + 1 <= length)
			{
				strncpy(LLViewerWindow::sSnapshotBaseName, directory + index + 1, LL_MAX_PATH -1);		/* Flawfinder: ignore */
				LLViewerWindow::sSnapshotBaseName[LL_MAX_PATH -1] = '\0';
			}

			index++;
			directory[index] = '\0';
			strncpy(LLViewerWindow::sSnapshotDir, directory, LL_MAX_PATH -1);		/* Flawfinder: ignore */
			LLViewerWindow::sSnapshotDir[LL_MAX_PATH -1] = '\0';
		}
	}

	// Look for an unused file name
	LLString filepath;
	S32 i = 1;
	S32 err = 0;

	do
	{
		filepath = sSnapshotDir;
		filepath += sSnapshotBaseName;
		filepath += llformat("_%.3d",i);
		filepath += extension;

		struct stat stat_info;
		err = gViewerWindow->mWindow->stat( filepath.c_str(), &stat_info );
		i++;
	}
	while( -1 != err );  // search until the file is not found (i.e., stat() gives an error).

	LLPointer<LLImageFormatted> formatted_image = LLImageFormatted::createFromExtension(extension);
	LLImageBase::setSizeOverride(TRUE);
	BOOL success = formatted_image->encode(raw);
	if( success )
	{
		success = formatted_image->save(filepath);
	}
	else
	{
		llwarns << "Unable to encode bmp snapshot" << llendl;
	}
	LLImageBase::setSizeOverride(FALSE);

	return success;
}


static S32 BORDERHEIGHT = 0;
static S32 BORDERWIDTH = 0;

void LLViewerWindow::movieSize(S32 new_width, S32 new_height)
{
	LLCoordScreen size;
	gViewerWindow->mWindow->getSize(&size);
	if (  (size.mX != new_width + BORDERWIDTH)
		||(size.mY != new_height + BORDERHEIGHT))
	{
		S32 x = gViewerWindow->getWindowWidth();
		S32 y = gViewerWindow->getWindowHeight();
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

BOOL LLViewerWindow::saveSnapshot( const LLString& filepath, S32 image_width, S32 image_height, BOOL show_ui, BOOL do_rebuild, ESnapshotType type)
{
	llinfos << "Saving snapshot to: " << filepath << llendl;

	LLPointer<LLImageRaw> raw = new LLImageRaw;
	BOOL success = rawSnapshot(raw, image_width, image_height, TRUE, FALSE, show_ui, do_rebuild);

	if (success)
	{
		LLPointer<LLImageBMP> bmp_image = new LLImageBMP;
		success = bmp_image->encode(raw);
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
	gAgent.sendAnimationRequest(ANIM_AGENT_SNAPSHOT, ANIM_REQUEST_START);
	send_sound_trigger(LLUUID(gSavedSettings.getString("UISndSnapshot")), 1.0f);
}


// Saves the image from the screen to the specified filename and path.
BOOL LLViewerWindow::rawSnapshot(LLImageRaw *raw, S32 image_width, S32 image_height, 
								 BOOL keep_window_aspect, BOOL is_texture, BOOL show_ui, BOOL do_rebuild, ESnapshotType type, S32 max_size)
{
	//F32 image_aspect_ratio = ((F32)image_width) / ((F32)image_height);
	//F32 window_aspect_ratio = ((F32)getWindowWidth()) / ((F32)getWindowHeight());

	if ((!gWorldPointer) ||
		(!raw))
	{
		return FALSE;
	}

	// PRE SNAPSHOT
	render_ui_and_swap_if_needed();
	gDisplaySwapBuffers = FALSE;
	
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
	S32 snapshot_width = mWindowRect.getWidth();
	S32 snapshot_height =  mWindowRect.getHeight();
	F32 scale_factor = 1.0f ;
	if (keep_window_aspect || is_texture) //map the entire window to snapshot
	{
	}
	else //scale or crop
	{
		if(snapshot_width > image_width) //crop
		{
			snapshot_width = image_width ;
		}
		if(snapshot_height > image_height)//crop
		{
			snapshot_height = image_height ;
		}

		//if (image_aspect_ratio > window_aspect_ratio)
		//{
		//	snapshot_height  = llround((F32)snapshot_width / image_aspect_ratio);
		//}
		//else if (image_aspect_ratio < window_aspect_ratio)
		//{
		//	snapshot_width = llround((F32)snapshot_height  * image_aspect_ratio);
		//}
	}

	LLRenderTarget target;
	
	scale_factor = llmax(1.f, (F32)image_width / snapshot_width, (F32)image_height / snapshot_height); 
	
	// SNAPSHOT
	S32 window_width = mWindowRect.getWidth();
	S32 window_height = mWindowRect.getHeight();
	
	LLRect window_rect = mWindowRect;

	BOOL use_fbo = FALSE;
	
	if (gGLManager.mHasFramebufferObject && 
		(image_width > window_width ||
		image_height > window_height) &&
		 !show_ui &&
		 keep_window_aspect)
	{
		GLint max_size = 0;
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &max_size);
		
		if (image_width <= max_size && image_height <= max_size)
		{
			use_fbo = TRUE;
			
			snapshot_width = image_width;
			snapshot_height = image_height;
			target.allocate(snapshot_width, snapshot_height, GL_RGBA, TRUE, GL_TEXTURE_RECTANGLE_ARB, TRUE);
			window_width = snapshot_width;
			window_height = snapshot_height;
			scale_factor = 1.f;
			mWindowRect.set(0, 0, snapshot_width, snapshot_height);
			target.bindTarget();

			
		}
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
	raw->resize(image_buffer_x, image_buffer_y, type == SNAPSHOT_TYPE_DEPTH ? 4 : 3);

	BOOL high_res = scale_factor > 1.f;
	if (high_res)
	{
		send_agent_pause();
		//rescale fonts
		initFonts(scale_factor);
		LLHUDText::reshape();
	}

	S32 output_buffer_offset_y = 0;

	F32 depth_conversion_factor_1 = (gCamera->getFar() + gCamera->getNear()) / (2.f * gCamera->getFar() * gCamera->getNear());
	F32 depth_conversion_factor_2 = (gCamera->getFar() - gCamera->getNear()) / (2.f * gCamera->getFar() * gCamera->getNear());

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
			if (type == SNAPSHOT_TYPE_OBJECT_ID)
			{
				glClearColor(0.f, 0.f, 0.f, 0.f);
				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

				gCamera->setZoomParameters(scale_factor, subimage_x+(subimage_y*llceil(scale_factor)));
				setup3DRender();
				setupViewport();
				BOOL first_time_through = (subimage_x + subimage_y == 0);
				gPickTransparent = FALSE;
				gObjectList.renderObjectsForSelect(*gCamera, FALSE, !first_time_through);
			}
			else
			{
				display(do_rebuild, scale_factor, subimage_x+(subimage_y*llceil(scale_factor)), use_fbo);
				render_ui_and_swap();
			}

			S32 subimage_x_offset = llclamp(buffer_x_offset - (subimage_x * window_width), 0, window_width);
			// handle fractional rows
			U32 read_width = llmax(0, (window_width - subimage_x_offset) -
									llmax(0, (window_width * (subimage_x + 1)) - (buffer_x_offset + raw->getWidth())));
			for(U32 out_y = 0; out_y < read_height ; out_y++)
			{
				if (type == SNAPSHOT_TYPE_OBJECT_ID || type == SNAPSHOT_TYPE_COLOR)
				{
					glReadPixels(
						subimage_x_offset, out_y + subimage_y_offset,
						read_width, 1,
						GL_RGB, GL_UNSIGNED_BYTE,
						raw->getData() + // current output pixel is beginning of buffer...
							( 
								(out_y * (raw->getWidth())) // ...plus iterated y...
								+ (window_width * subimage_x) // ...plus subimage start in x...
								+ (raw->getWidth() * window_height * subimage_y) // ...plus subimage start in y...
								- output_buffer_offset_x // ...minus buffer padding x...
								- (output_buffer_offset_y * (raw->getWidth()))  // ...minus buffer padding y...
							) * 3 // times 3 bytes per pixel
					);
				}
				else // SNAPSHOT_TYPE_DEPTH
				{
					S32 output_buffer_offset = ( 
								(out_y * (raw->getWidth())) // ...plus iterated y...
								+ (window_width * subimage_x) // ...plus subimage start in x...
								+ (raw->getWidth() * window_height * subimage_y) // ...plus subimage start in y...
								- output_buffer_offset_x // ...minus buffer padding x...
								- (output_buffer_offset_y * (raw->getWidth()))  // ...minus buffer padding y...
							) * 4; // times 4 bytes per pixel

					glReadPixels(
						subimage_x_offset, out_y + subimage_y_offset,
						read_width, 1,
						GL_DEPTH_COMPONENT, GL_FLOAT,
						raw->getData() + output_buffer_offset// current output pixel is beginning of buffer...
					);

					for (S32 i = output_buffer_offset; i < output_buffer_offset + (S32)read_width * 4; i += 4)
					{
						F32 depth_float = *(F32*)(raw->getData() + i);
					
						F32 linear_depth_float = 1.f / (depth_conversion_factor_1 - (depth_float * depth_conversion_factor_2));
						U8 depth_byte = F32_to_U8(linear_depth_float, gCamera->getNear(), gCamera->getFar());
						*(raw->getData() + i + 0) = depth_byte;
						*(raw->getData() + i + 1) = depth_byte;
						*(raw->getData() + i + 2) = depth_byte;
						*(raw->getData() + i + 3) = 255;
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
		mWindowRect = window_rect;
		target.flush();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	gDisplaySwapBuffers = FALSE;

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
	image_width += (image_width * (type == SNAPSHOT_TYPE_DEPTH ? 4 : 3)) % 4 ;	

	// Resize image
	if(llabs(image_width - image_buffer_x) > 4 || llabs(image_height - image_buffer_y) > 4)
	{
		raw->scale( image_width, image_height );  
	}
	else if(image_width != image_buffer_x || image_height != image_buffer_y)
	{
		raw->scale( image_width, image_height, FALSE );  
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

	return TRUE;
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
	// Draw instructions for mouselook ("Press ESC to leave Mouselook" in a box at the top of the screen.)
	const char* instructions = "Press ESC to leave Mouselook.";
	const LLFontGL* font = gResMgr->getRes( LLFONT_SANSSERIF );

	const S32 INSTRUCTIONS_PAD = 5;
	LLRect instructions_rect;
	instructions_rect.setLeftTopAndSize( 
		INSTRUCTIONS_PAD,
		gViewerWindow->getWindowHeight() - INSTRUCTIONS_PAD,
		font->getWidth( instructions ) + 2 * INSTRUCTIONS_PAD,
		llround(font->getLineHeight() + 2 * INSTRUCTIONS_PAD));

	{
		LLGLSNoTexture gls_no_texture;
		gGL.color4f( 0.9f, 0.9f, 0.9f, 1.0f );
		gl_rect_2d( instructions_rect );
	}
	
	font->renderUTF8( 
		instructions, 0,
		instructions_rect.mLeft + INSTRUCTIONS_PAD,
		instructions_rect.mTop - INSTRUCTIONS_PAD,
		LLColor4( 0.0f, 0.0f, 0.0f, 1.f ),
		LLFontGL::LEFT, LLFontGL::TOP);
}


// These functions are here only because LLViewerWindow used to do the work that gFocusMgr does now.
// They let other objects continue to work without change.

void LLViewerWindow::setKeyboardFocus(LLUICtrl* new_focus)
{
	gFocusMgr.setKeyboardFocus( new_focus );
}

LLUICtrl* LLViewerWindow::getKeyboardFocus()
{
	return gFocusMgr.getKeyboardFocus();
}

BOOL LLViewerWindow::hasKeyboardFocus(const LLUICtrl* possible_focus) const
{
	return possible_focus == gFocusMgr.getKeyboardFocus();
}

BOOL LLViewerWindow::childHasKeyboardFocus(const LLView* parent) const
{
	return gFocusMgr.childHasKeyboardFocus( parent );
}

void LLViewerWindow::setMouseCapture(LLMouseHandler* new_captor)
{
	gFocusMgr.setMouseCapture( new_captor );
}

LLMouseHandler* LLViewerWindow::getMouseCaptor() const
{
	return gFocusMgr.getMouseCapture();
}

S32	LLViewerWindow::getWindowHeight()	const 	
{ 
	return mVirtualWindowRect.getHeight(); 
}

S32	LLViewerWindow::getWindowWidth() const 	
{ 
	return mVirtualWindowRect.getWidth(); 
}

S32	LLViewerWindow::getWindowDisplayHeight()	const 	
{ 
	return mWindowRect.getHeight(); 
}

S32	LLViewerWindow::getWindowDisplayWidth() const 	
{ 
	return mWindowRect.getWidth(); 
}

LLUICtrl* LLViewerWindow::getTopCtrl() const
{
	return gFocusMgr.getTopCtrl();
}

BOOL LLViewerWindow::hasTopCtrl(LLView* view) const
{
	return view == gFocusMgr.getTopCtrl();
}

void LLViewerWindow::setTopCtrl(LLUICtrl* new_top)
{
	gFocusMgr.setTopCtrl( new_top );
}

void LLViewerWindow::setupViewport(S32 x_offset, S32 y_offset)
{
	gGLViewport[0] = x_offset;
	gGLViewport[1] = y_offset;
	gGLViewport[2] = mWindowRect.getWidth();
	gGLViewport[3] = mWindowRect.getHeight();
	glViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
}

void LLViewerWindow::setup3DRender()
{
	gCamera->setPerspective(NOT_FOR_SELECTION, 0, 0,  mWindowRect.getWidth(), mWindowRect.getHeight(), FALSE, gCamera->getNear(), MAX_FAR_CLIP*2.f);
}

void LLViewerWindow::setup2DRender()
{
	gl_state_for_2d(mWindowRect.getWidth(), mWindowRect.getHeight());
}

// Could cache the pointer from the last hitObjectOrLand here.
LLViewerObject *LLViewerWindow::lastObjectHit()
{
	return gObjectList.findObject( gLastHitObjectID );
}

const LLVector3d& LLViewerWindow::lastObjectHitOffset()
{
	return gLastHitObjectOffset;
}

// Could cache the pointer from the last hitObjectOrLand here.
LLViewerObject *LLViewerWindow::lastNonFloraObjectHit()
{
	return gObjectList.findObject( gLastHitNonFloraObjectID );
}

const LLVector3d& LLViewerWindow::lastNonFloraObjectHitOffset()
{
	return gLastHitNonFloraObjectOffset;
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
		mRootView->removeChild( mProgressView );
		mRootView->addChild( mProgressView );
	}
}

void LLViewerWindow::setProgressString(const LLString& string)
{
	if (mProgressView)
	{
		mProgressView->setText(string);
	}
}

void LLViewerWindow::setProgressMessage(const LLString& msg)
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

void LLViewerWindow::setProgressCancelButtonVisible( BOOL b, const LLString& label )
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
	if (!gGLManager.mIsDisabled)
	{
		llinfos << "Shutting down GL..." << llendl;

		// Pause texture decode threads (will get unpaused during main loop)
		LLAppViewer::getTextureCache()->pause();
		LLAppViewer::getImageDecodeThread()->pause();
		LLAppViewer::getTextureFetch()->pause();
		
		gSky.destroyGL();
		stop_glerror();
	
		gImageList.destroyGL(save_state);
		stop_glerror();

		gBumpImageList.destroyGL();
		stop_glerror();

		LLFontGL::destroyGL();
		stop_glerror();

		LLVOAvatar::destroyGL();
		stop_glerror();

		LLDynamicTexture::destroyGL();
		stop_glerror();

		if (gPipeline.isInit())
		{
			gPipeline.destroyGL();
		}
		
		gCone.cleanupGL();
		gBox.cleanupGL();
		gSphere.cleanupGL();
		gCylinder.cleanupGL();
		
		gGLManager.mIsDisabled = TRUE;
		stop_glerror();
		
		llinfos << "Remaining allocated texture memory: " << LLImageGL::sGlobalTextureMemory << " bytes" << llendl;
	}
}

void LLViewerWindow::restoreGL(const LLString& progress_message)
{
	if (gGLManager.mIsDisabled)
	{
		llinfos << "Restoring GL..." << llendl;
		gGLManager.mIsDisabled = FALSE;

		// for future support of non-square pixels, and fonts that are properly stretched
		//LLFontGL::destroyDefaultFonts();
		initFonts();
		initGLDefaults();
		LLGLState::restoreGL();
		gSky.restoreGL();
		gPipeline.restoreGL();
		LLDrawPoolWater::restoreGL();
		LLManipTranslate::restoreGL();
		gImageList.restoreGL();
		gBumpImageList.restoreGL();
		LLDynamicTexture::restoreGL();
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
#if LL_WINDOWS
		if (SetUnhandledExceptionFilter(LLWinDebug::handleException) != LLWinDebug::handleException)
		{
			llwarns << " Someone took over my exception handler (post restoreGL)!" << llendl;
		}
#endif

	}
}

void LLViewerWindow::initFonts(F32 zoom_factor)
{
	LLFontGL::destroyGL();
	LLFontGL::initDefaultFonts( gSavedSettings.getF32("FontScreenDPI"),
								mDisplayScale.mV[VX] * zoom_factor,
								mDisplayScale.mV[VY] * zoom_factor,
								gSavedSettings.getString("FontMonospace"),
								gSavedSettings.getF32("FontSizeMonospace"),
								gSavedSettings.getString("FontSansSerif"), 
								gSavedSettings.getString("FontSansSerifFallback"),
								gSavedSettings.getF32("FontSansSerifFallbackScale"),
								gSavedSettings.getF32("FontSizeSmall"),	
								gSavedSettings.getF32("FontSizeMedium"), 
								gSavedSettings.getF32("FontSizeLarge"),			 
								gSavedSettings.getF32("FontSizeHuge"),			 
								gSavedSettings.getString("FontSansSerifBold"),
								gSavedSettings.getF32("FontSizeMedium"),
								gDirUtilp->getAppRODataDir()
							);
}
void LLViewerWindow::toggleFullscreen(BOOL show_progress)
{
	if (mWindow)
	{
		mWantFullscreen = mWindow->getFullscreen() ? FALSE : TRUE;
		mShowFullscreenProgress = show_progress;
	}
}

void LLViewerWindow::getTargetWindow(BOOL& fullscreen, S32& width, S32& height) const
{
	fullscreen = mWantFullscreen;
	
	if (gViewerWindow->mWindow
	&&  gViewerWindow->mWindow->getFullscreen() == mWantFullscreen)
	{
		width = gViewerWindow->getWindowDisplayWidth();
		height = gViewerWindow->getWindowDisplayHeight();
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


BOOL LLViewerWindow::checkSettings()
{
	BOOL is_fullscreen = gViewerWindow->mWindow->getFullscreen();
	if (is_fullscreen && !mWantFullscreen)
	{
		gViewerWindow->changeDisplaySettings(FALSE, 
											 LLCoordScreen(gSavedSettings.getS32("WindowWidth"),
														   gSavedSettings.getS32("WindowHeight")),
											 TRUE,
											 mShowFullscreenProgress);
		return TRUE;
	}
	else if (!is_fullscreen && mWantFullscreen)
	{
		if (!LLStartUp::canGoFullscreen())
		{
			return FALSE;
		}
		
#ifndef LL_RELEASE_FOR_DOWNLOAD
		LLGLState::checkStates();
		LLGLState::checkTextureChannels();
#endif
		gViewerWindow->changeDisplaySettings(TRUE, 
											 LLCoordScreen(gSavedSettings.getS32("FullScreenWidth"),
														   gSavedSettings.getS32("FullScreenHeight")),
											 gSavedSettings.getBOOL("DisableVerticalSync"),
											 mShowFullscreenProgress);

#ifndef LL_RELEASE_FOR_DOWNLOAD
		LLGLState::checkStates();
		LLGLState::checkTextureChannels();
#endif
		return TRUE;
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
	gSavedSettings.setBOOL("FullScreen", mWantFullscreen);

	gResizeScreenTexture = TRUE;

	BOOL old_fullscreen = mWindow->getFullscreen();
	if (!old_fullscreen && fullscreen && !LLStartUp::canGoFullscreen())
	{
		// we can't do this now, so do it later
		
		gSavedSettings.setS32("FullScreenWidth", size.mX);
		gSavedSettings.setS32("FullScreenHeight", size.mY);
		//gSavedSettings.setBOOL("DisableVerticalSync", disable_vsync);
		
		return TRUE;	// a lie..., because we'll get to it later
	}

	// going from windowed to windowed
	if (!old_fullscreen && !fullscreen)
	{
		// if not maximized, use the request size
		if (!mWindow->getMaximized())
		{
			mWindow->setSize(size);
		}
		return TRUE;
	}

	// Close floaters that don't handle settings change
	LLFloaterSnapshot::hide(0);
	
	BOOL result_first_try = FALSE;
	BOOL result_second_try = FALSE;

	LLUICtrl* keyboard_focus = gFocusMgr.getKeyboardFocus();
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
	
	result_first_try = mWindow->switchContext(fullscreen, size, disable_vsync);
	if (!result_first_try)
	{
		// try to switch back
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
		LLStringBase<char>::format_map_t args;
		args["[RESX]"] = llformat("%d",size.mX);
		args["[RESY]"] = llformat("%d",size.mY);
		alertXml("ResolutionSwitchFail", args);
		size = old_size; // for reshape below
	}

	BOOL success = result_first_try || result_second_try;
	if (success)
	{
#if LL_WINDOWS
		// Only trigger a reshape after switching to fullscreen; otherwise rely on the windows callback
		// (otherwise size is wrong; this is the entire window size, reshape wants the visible window size)
		if (fullscreen)
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


void LLViewerWindow::drawPickBuffer() const
{
	if (mPickBuffer)
	{
		gGL.start();
		gGL.pushMatrix();
		LLGLDisable no_blend(GL_BLEND);
		LLGLDisable no_alpha_test(GL_ALPHA_TEST);
		LLGLSNoTexture no_texture;
		glPixelZoom(10.f, 10.f);
		glRasterPos2f(((F32)mPickPoint.mX * mDisplayScale.mV[VX] + 10.f), 
			((F32)mPickPoint.mY * mDisplayScale.mV[VY] + 10.f));
		glDrawPixels(PICK_DIAMETER, PICK_DIAMETER, GL_RGBA, GL_UNSIGNED_BYTE, mPickBuffer);
		glPixelZoom(1.f, 1.f);
		gGL.color4fv(LLColor4::white.mV);
		gl_rect_2d(llround((F32)mPickPoint.mX * mDisplayScale.mV[VX] - (F32)(PICK_HALF_WIDTH)), 
			llround((F32)mPickPoint.mY * mDisplayScale.mV[VY] + (F32)(PICK_HALF_WIDTH)),
			llround((F32)mPickPoint.mX * mDisplayScale.mV[VX] + (F32)(PICK_HALF_WIDTH)),
			llround((F32)mPickPoint.mY * mDisplayScale.mV[VY] - (F32)(PICK_HALF_WIDTH)),
			FALSE);
		gl_line_2d(llround((F32)mPickPoint.mX * mDisplayScale.mV[VX] - (F32)(PICK_HALF_WIDTH)), 
			llround((F32)mPickPoint.mY * mDisplayScale.mV[VY] + (F32)(PICK_HALF_WIDTH)),
			llround((F32)mPickPoint.mX * mDisplayScale.mV[VX] + 10.f), 
			llround((F32)mPickPoint.mY * mDisplayScale.mV[VY] + (F32)(PICK_DIAMETER) * 10.f + 10.f));
		gl_line_2d(llround((F32)mPickPoint.mX * mDisplayScale.mV[VX] + (F32)(PICK_HALF_WIDTH)),
			llround((F32)mPickPoint.mY * mDisplayScale.mV[VY] - (F32)(PICK_HALF_WIDTH)),
			llround((F32)mPickPoint.mX * mDisplayScale.mV[VX] + (F32)(PICK_DIAMETER) * 10.f + 10.f), 
			llround((F32)mPickPoint.mY * mDisplayScale.mV[VY] + 10.f));
		gGL.translatef(10.f, 10.f, 0.f);
		gl_rect_2d(llround((F32)mPickPoint.mX * mDisplayScale.mV[VX]), 
			llround((F32)mPickPoint.mY * mDisplayScale.mV[VY] + (F32)(PICK_DIAMETER) * 10.f),
			llround((F32)mPickPoint.mX * mDisplayScale.mV[VX] + (F32)(PICK_DIAMETER) * 10.f),
			llround((F32)mPickPoint.mY * mDisplayScale.mV[VY]),
			FALSE);
		gl_rect_2d(llround((F32)mPickPoint.mX * mDisplayScale.mV[VX] + (F32)(PICK_HALF_WIDTH + mPickOffset.mX)* 10.f), 
			llround((F32)mPickPoint.mY * mDisplayScale.mV[VY] + (F32)(PICK_HALF_WIDTH + mPickOffset.mY + 1) * 10.f),
			llround((F32)mPickPoint.mX * mDisplayScale.mV[VX] + (F32)(PICK_HALF_WIDTH + mPickOffset.mX + 1) * 10.f),
			llround((F32)mPickPoint.mY * mDisplayScale.mV[VY] + (F32)(PICK_HALF_WIDTH  + mPickOffset.mY) * 10.f),
			FALSE);
		gGL.popMatrix();
		gGL.stop();
	}
}

void LLViewerWindow::calcDisplayScale()
{
	F32 ui_scale_factor = gSavedSettings.getF32("UIScaleFactor");
	LLVector2 display_scale;
	display_scale.setVec(llmax(1.f / mWindow->getPixelAspectRatio(), 1.f), llmax(mWindow->getPixelAspectRatio(), 1.f));
	F32 height_normalization = gSavedSettings.getBOOL("UIAutoScale") ? ((F32)mWindowRect.getHeight() / display_scale.mV[VY]) / 768.f : 1.f;
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
		display_scale.mV[0] = llround(display_scale.mV[0], 2.0f/(F32) mWindowRect.getWidth());
		display_scale.mV[1] = llround(display_scale.mV[1], 2.0f/(F32) mWindowRect.getHeight());
	}
	
	if (display_scale != mDisplayScale)
	{
		llinfos << "Setting display scale to " << display_scale << llendl;

		mDisplayScale = display_scale;
		// Init default fonts
		initFonts();
	}
}

//----------------------------------------------------------------------------

// static
bool LLViewerWindow::alertCallback(S32 modal)
{
	if (gNoRender)
	{
		return false;
	}
	else
	{
// 		if (modal) // we really always want to take you out of mouselook
		{
			// If we're in mouselook, the mouse is hidden and so the user can't click 
			// the dialog buttons.  In that case, change to First Person instead.
			if( gAgent.cameraMouselook() )
			{
				gAgent.changeCameraToDefault();
			}
		}
		return true;
	}
}

LLAlertDialog* LLViewerWindow::alertXml(const std::string& xml_filename,
							  LLAlertDialog::alert_callback_t callback, void* user_data)
{
	LLString::format_map_t args;
	return alertXml( xml_filename, args, callback, user_data );
}

LLAlertDialog* LLViewerWindow::alertXml(const std::string& xml_filename, const LLString::format_map_t& args,
							  LLAlertDialog::alert_callback_t callback, void* user_data)
{
	if (gNoRender)
	{
		llinfos << "Alert: " << xml_filename << llendl;
		if (callback)
		{
			callback(-1, user_data);
		}
		return NULL;
	}

	// If we're in mouselook, the mouse is hidden and so the user can't click 
	// the dialog buttons.  In that case, change to First Person instead.
	if( gAgent.cameraMouselook() )
	{
		gAgent.changeCameraToDefault();
	}

	// Note: object adds, removes, and destroys itself.
	return LLAlertDialog::showXml( xml_filename, args, callback, user_data );
}

LLAlertDialog* LLViewerWindow::alertXmlEditText(const std::string& xml_filename, const LLString::format_map_t& args,
									  LLAlertDialog::alert_callback_t callback, void* user_data,
									  LLAlertDialog::alert_text_callback_t text_callback, void *text_data,
									  const LLString::format_map_t& edit_args, BOOL draw_asterixes)
{
	if (gNoRender)
	{
		llinfos << "Alert: " << xml_filename << llendl;
		if (callback)
		{
			callback(-1, user_data);
		}
		return NULL;
	}

	// If we're in mouselook, the mouse is hidden and so the user can't click 
	// the dialog buttons.  In that case, change to First Person instead.
	if( gAgent.cameraMouselook() )
	{
		gAgent.changeCameraToDefault();
	}

	// Note: object adds, removes, and destroys itself.
	LLAlertDialog* alert = LLAlertDialog::createXml( xml_filename, args, callback, user_data );
	if (alert)
	{
		if (text_callback)
		{
			alert->setEditTextCallback(text_callback, text_data);
		}
		alert->setEditTextArgs(edit_args);
		alert->setDrawAsterixes(draw_asterixes);
		alert->show();
	}
	return alert;
}

////////////////////////////////////////////////////////////////////////////

LLBottomPanel::LLBottomPanel(const LLRect &rect) : 
	LLPanel("", rect, FALSE),
	mIndicator(NULL)
{
	// bottom panel is focus root, so Tab moves through the toolbar and button bar, and overlay
	setFocusRoot(TRUE);
	// flag this panel as chrome so buttons don't grab keyboard focus
	setIsChrome(TRUE);

	mFactoryMap["toolbar"] = LLCallbackMap(createToolBar, NULL);
	mFactoryMap["overlay"] = LLCallbackMap(createOverlayBar, NULL);
	mFactoryMap["hud"] = LLCallbackMap(createHUD, NULL);
	gUICtrlFactory->buildPanel(this, "panel_bars.xml", &getFactoryMap());
	
	setOrigin(rect.mLeft, rect.mBottom);
	reshape(rect.getWidth(), rect.getHeight());
}

void LLBottomPanel::setFocusIndicator(LLView * indicator)
{
	mIndicator = indicator;
}

void LLBottomPanel::draw()
{
	if(mIndicator)
	{
		BOOL hasFocus = gFocusMgr.childHasKeyboardFocus(this);
		mIndicator->setVisible(hasFocus);
		mIndicator->setEnabled(hasFocus);
	}
	LLPanel::draw();
}

void* LLBottomPanel::createHUD(void* data)
{
	delete gHUDView;
	gHUDView = new LLHUDView();
	return gHUDView;
}


void* LLBottomPanel::createOverlayBar(void* data)
{
	delete gOverlayBar;
	gOverlayBar = new LLOverlayBar();
	return gOverlayBar;
}

void* LLBottomPanel::createToolBar(void* data)
{
	delete gToolBar;
	gToolBar = new LLToolBar();
	return gToolBar;
}
