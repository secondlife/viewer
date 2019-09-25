/** 
 * @file llviewerkeyboard.cpp
 * @brief LLViewerKeyboard class implementation
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "llappviewer.h"
#include "llfloaterreg.h"
#include "llviewerkeyboard.h"
#include "llmath.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llfloaterimnearbychat.h"
#include "llviewercontrol.h"
#include "llfocusmgr.h"
#include "llmorphview.h"
#include "llmoveview.h"
#include "lltoolfocus.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llfloatercamera.h"
#include "llinitparam.h"

//
// Constants
//

const F32 FLY_TIME = 0.5f;
const F32 FLY_FRAMES = 4;

const F32 NUDGE_TIME = 0.25f;  // in seconds
const S32 NUDGE_FRAMES = 2;
const F32 ORBIT_NUDGE_RATE = 0.05f;  // fraction of normal speed

struct LLKeyboardActionRegistry 
:	public LLRegistrySingleton<std::string, boost::function<void (EKeystate keystate)>, LLKeyboardActionRegistry>
{
	LLSINGLETON_EMPTY_CTOR(LLKeyboardActionRegistry);
};

LLViewerKeyboard gViewerKeyboard;

void agent_jump( EKeystate s )
{
	static BOOL first_fly_attempt(TRUE);
	if (KEYSTATE_UP == s)
	{
		first_fly_attempt = TRUE;
		return;
	}
	F32 time = gKeyboard->getCurKeyElapsedTime();
	S32 frame_count = ll_round(gKeyboard->getCurKeyElapsedFrameCount());

	if( time < FLY_TIME 
		|| frame_count <= FLY_FRAMES 
		|| gAgent.upGrabbed()
		|| !gSavedSettings.getBOOL("AutomaticFly"))
	{
		gAgent.moveUp(1);
	}
	else
	{
		gAgent.setFlying(TRUE, first_fly_attempt);
		first_fly_attempt = FALSE;
		gAgent.moveUp(1);
	}
}

void agent_push_down( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgent.moveUp(-1);
}

static void agent_check_temporary_run(LLAgent::EDoubleTapRunMode mode)
{
	if (gAgent.mDoubleTapRunMode == mode &&
		gAgent.getRunning() &&
		!gAgent.getAlwaysRun())
	{
		// Turn off temporary running.
		gAgent.clearRunning();
		gAgent.sendWalkRun(gAgent.getRunning());
	}
}

static void agent_handle_doubletap_run(EKeystate s, LLAgent::EDoubleTapRunMode mode)
{
	if (KEYSTATE_UP == s)
	{
		// Note: in case shift is already released, slide left/right run
		// will be released in agent_turn_left()/agent_turn_right()
		agent_check_temporary_run(mode);
	}
	else if (gSavedSettings.getBOOL("AllowTapTapHoldRun") &&
		 KEYSTATE_DOWN == s &&
		 !gAgent.getRunning())
	{
		if (gAgent.mDoubleTapRunMode == mode &&
		    gAgent.mDoubleTapRunTimer.getElapsedTimeF32() < NUDGE_TIME)
		{
			// Same walk-key was pushed again quickly; this is a
			// double-tap so engage temporary running.
			gAgent.setRunning();
			gAgent.sendWalkRun(gAgent.getRunning());
		}

		// Pressing any walk-key resets the double-tap timer
		gAgent.mDoubleTapRunTimer.reset();
		gAgent.mDoubleTapRunMode = mode;
	}
}

static void agent_push_forwardbackward( EKeystate s, S32 direction, LLAgent::EDoubleTapRunMode mode )
{
	agent_handle_doubletap_run(s, mode);
	if (KEYSTATE_UP == s) return;

	F32 time = gKeyboard->getCurKeyElapsedTime();
	S32 frame_count = ll_round(gKeyboard->getCurKeyElapsedFrameCount());

	if( time < NUDGE_TIME || frame_count <= NUDGE_FRAMES)
	{
		gAgent.moveAtNudge(direction);
	}
	else
	{
		gAgent.moveAt(direction);
	}
}

void camera_move_forward( EKeystate s );

void agent_push_forward( EKeystate s )
{
	if(gAgent.isMovementLocked()) return;

	//in free camera control mode we need to intercept keyboard events for avatar movements
	if (LLFloaterCamera::inFreeCameraMode())
	{
		camera_move_forward(s);
	}
	else
	{
		agent_push_forwardbackward(s, 1, LLAgent::DOUBLETAP_FORWARD);
	}
}

void camera_move_backward( EKeystate s );

void agent_push_backward( EKeystate s )
{
	if(gAgent.isMovementLocked()) return;

	//in free camera control mode we need to intercept keyboard events for avatar movements
	if (LLFloaterCamera::inFreeCameraMode())
	{
		camera_move_backward(s);
	}
	else if (!gAgent.backwardGrabbed() && gAgentAvatarp->isSitting() && gSavedSettings.getBOOL("LeaveMouselook"))
	{
		gAgentCamera.changeCameraToThirdPerson();
	}
	else
	{
		agent_push_forwardbackward(s, -1, LLAgent::DOUBLETAP_BACKWARD);
	}
}

static void agent_slide_leftright( EKeystate s, S32 direction, LLAgent::EDoubleTapRunMode mode )
{
	agent_handle_doubletap_run(s, mode);
	if( KEYSTATE_UP == s ) return;
	F32 time = gKeyboard->getCurKeyElapsedTime();
	S32 frame_count = ll_round(gKeyboard->getCurKeyElapsedFrameCount());

	if( time < NUDGE_TIME || frame_count <= NUDGE_FRAMES)
	{
		gAgent.moveLeftNudge(direction);
	}
	else
	{
		gAgent.moveLeft(direction);
	}
}


void agent_slide_left( EKeystate s )
{
	if(gAgent.isMovementLocked()) return;
	agent_slide_leftright(s, 1, LLAgent::DOUBLETAP_SLIDELEFT);
}


void agent_slide_right( EKeystate s )
{
	if(gAgent.isMovementLocked()) return;
	agent_slide_leftright(s, -1, LLAgent::DOUBLETAP_SLIDERIGHT);
}

void camera_spin_around_cw( EKeystate s );

void agent_turn_left( EKeystate s )
{
	//in free camera control mode we need to intercept keyboard events for avatar movements
	if (LLFloaterCamera::inFreeCameraMode())
	{
		camera_spin_around_cw(s);
		return;
	}

	if(gAgent.isMovementLocked()) return;

	if (LLToolCamera::getInstance()->mouseSteerMode())
	{
		agent_slide_left(s);
	}
	else
	{
		if (KEYSTATE_UP == s)
		{
			// Check temporary running. In case user released 'left' key with shift already released.
			agent_check_temporary_run(LLAgent::DOUBLETAP_SLIDELEFT);
			return;
		}
		F32 time = gKeyboard->getCurKeyElapsedTime();
		gAgent.moveYaw( LLFloaterMove::getYawRate( time ) );
	}
}

void camera_spin_around_ccw( EKeystate s );

void agent_turn_right( EKeystate s )
{
	//in free camera control mode we need to intercept keyboard events for avatar movements
	if (LLFloaterCamera::inFreeCameraMode())
	{
		camera_spin_around_ccw(s);
		return;
	}

	if(gAgent.isMovementLocked()) return;

	if (LLToolCamera::getInstance()->mouseSteerMode())
	{
		agent_slide_right(s);
	}
	else
	{
		if (KEYSTATE_UP == s)
		{
			// Check temporary running. In case user released 'right' key with shift already released.
			agent_check_temporary_run(LLAgent::DOUBLETAP_SLIDERIGHT);
			return;
		}
		F32 time = gKeyboard->getCurKeyElapsedTime();
		gAgent.moveYaw( -LLFloaterMove::getYawRate( time ) );
	}
}

void agent_look_up( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgent.movePitch(-1);
	//gAgent.rotate(-2.f * DEG_TO_RAD, gAgent.getFrame().getLeftAxis() );
}


void agent_look_down( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgent.movePitch(1);
	//gAgent.rotate(2.f * DEG_TO_RAD, gAgent.getFrame().getLeftAxis() );
}

void agent_toggle_fly( EKeystate s )
{
	// Only catch the edge
	if (KEYSTATE_DOWN == s )
	{
		LLAgent::toggleFlying();
	}
}

F32 get_orbit_rate()
{
	F32 time = gKeyboard->getCurKeyElapsedTime();
	if( time < NUDGE_TIME )
	{
		F32 rate = ORBIT_NUDGE_RATE + time * (1 - ORBIT_NUDGE_RATE)/ NUDGE_TIME;
		//LL_INFOS() << rate << LL_ENDL;
		return rate;
	}
	else
	{
		return 1;
	}
}

void camera_spin_around_ccw( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitLeftKey( get_orbit_rate() );
}


void camera_spin_around_cw( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitRightKey( get_orbit_rate() );
}

void camera_spin_around_ccw_sitting( EKeystate s )
{
	if( KEYSTATE_UP == s && gAgent.mDoubleTapRunMode != LLAgent::DOUBLETAP_SLIDERIGHT ) return;
	if (gAgent.rotateGrabbed() || gAgentCamera.sitCameraEnabled() || gAgent.getRunning())
	{
		//send keystrokes, but do not change camera
		agent_turn_right(s);
	}
	else
	{
		//change camera but do not send keystrokes
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitLeftKey( get_orbit_rate() );
	}
}


void camera_spin_around_cw_sitting( EKeystate s )
{
	if( KEYSTATE_UP == s && gAgent.mDoubleTapRunMode != LLAgent::DOUBLETAP_SLIDELEFT ) return;
	if (gAgent.rotateGrabbed() || gAgentCamera.sitCameraEnabled() || gAgent.getRunning())
	{
		//send keystrokes, but do not change camera
		agent_turn_left(s);
	}
	else
	{
		//change camera but do not send keystrokes
		gAgentCamera.unlockView();
		gAgentCamera.setOrbitRightKey( get_orbit_rate() );
	}
}


void camera_spin_over( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitUpKey( get_orbit_rate() );
}


void camera_spin_under( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitDownKey( get_orbit_rate() );
}

void camera_spin_over_sitting( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	if (gAgent.upGrabbed() || gAgentCamera.sitCameraEnabled())
	{
		//send keystrokes, but do not change camera
		agent_jump(s);
	}
	else
	{
		//change camera but do not send keystrokes
		gAgentCamera.setOrbitUpKey( get_orbit_rate() );
	}
}


void camera_spin_under_sitting( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	if (gAgent.downGrabbed() || gAgentCamera.sitCameraEnabled())
	{
		//send keystrokes, but do not change camera
		agent_push_down(s);
	}
	else
	{
		//change camera but do not send keystrokes
		gAgentCamera.setOrbitDownKey( get_orbit_rate() );
	}
}

void camera_move_forward( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitInKey( get_orbit_rate() );
}


void camera_move_backward( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitOutKey( get_orbit_rate() );
}

void camera_move_forward_sitting( EKeystate s )
{
	if( KEYSTATE_UP == s && gAgent.mDoubleTapRunMode != LLAgent::DOUBLETAP_FORWARD ) return;
	if (gAgent.forwardGrabbed() || gAgentCamera.sitCameraEnabled() || (gAgent.getRunning() && !gAgent.getAlwaysRun()))
	{
		agent_push_forward(s);
	}
	else
	{
		gAgentCamera.setOrbitInKey( get_orbit_rate() );
	}
}


void camera_move_backward_sitting( EKeystate s )
{
	if( KEYSTATE_UP == s && gAgent.mDoubleTapRunMode != LLAgent::DOUBLETAP_BACKWARD ) return;

	if (gAgent.backwardGrabbed() || gAgentCamera.sitCameraEnabled() || (gAgent.getRunning() && !gAgent.getAlwaysRun()))
	{
		agent_push_backward(s);
	}
	else
	{
		gAgentCamera.setOrbitOutKey( get_orbit_rate() );
	}
}

void camera_pan_up( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setPanUpKey( get_orbit_rate() );
}

void camera_pan_down( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setPanDownKey( get_orbit_rate() );
}

void camera_pan_left( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setPanLeftKey( get_orbit_rate() );
}

void camera_pan_right( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setPanRightKey( get_orbit_rate() );
}

void camera_pan_in( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setPanInKey( get_orbit_rate() );
}

void camera_pan_out( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setPanOutKey( get_orbit_rate() );
}

void camera_move_forward_fast( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitInKey(2.5f);
}

void camera_move_backward_fast( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitOutKey(2.5f);
}


void edit_avatar_spin_ccw( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gMorphView->setCameraDrivenByKeys( TRUE );
	gAgentCamera.setOrbitLeftKey( get_orbit_rate() );
	//gMorphView->orbitLeft( get_orbit_rate() );
}


void edit_avatar_spin_cw( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gMorphView->setCameraDrivenByKeys( TRUE );
	gAgentCamera.setOrbitRightKey( get_orbit_rate() );
	//gMorphView->orbitRight( get_orbit_rate() );
}

void edit_avatar_spin_over( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gMorphView->setCameraDrivenByKeys( TRUE );
	gAgentCamera.setOrbitUpKey( get_orbit_rate() );
	//gMorphView->orbitUp( get_orbit_rate() );
}


void edit_avatar_spin_under( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gMorphView->setCameraDrivenByKeys( TRUE );
	gAgentCamera.setOrbitDownKey( get_orbit_rate() );
	//gMorphView->orbitDown( get_orbit_rate() );
}

void edit_avatar_move_forward( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gMorphView->setCameraDrivenByKeys( TRUE );
	gAgentCamera.setOrbitInKey( get_orbit_rate() );
	//gMorphView->orbitIn();
}


void edit_avatar_move_backward( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gMorphView->setCameraDrivenByKeys( TRUE );
	gAgentCamera.setOrbitOutKey( get_orbit_rate() );
	//gMorphView->orbitOut();
}

void stop_moving( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	// stop agent
	gAgent.setControlFlags(AGENT_CONTROL_STOP);

	// cancel autopilot
	gAgent.stopAutoPilot();
}

void start_chat( EKeystate s )
{
    if (LLAppViewer::instance()->quitRequested())
    {
        return; // can't talk, gotta go, kthxbye!
    }
    
	// start chat
	LLFloaterIMNearbyChat::startChat(NULL);
}

void start_gesture( EKeystate s )
{
	LLUICtrl* focus_ctrlp = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
	if (KEYSTATE_UP == s &&
		! (focus_ctrlp && focus_ctrlp->acceptsTextInput()))
	{
 		if ((LLFloaterReg::getTypedInstance<LLFloaterIMNearbyChat>("nearby_chat"))->getCurrentChat().empty())
 		{
 			// No existing chat in chat editor, insert '/'
 			LLFloaterIMNearbyChat::startChat("/");
 		}
 		else
 		{
 			// Don't overwrite existing text in chat editor
 			LLFloaterIMNearbyChat::startChat(NULL);
 		}
	}
}

void toggle_run(EKeystate s)
{
    if (KEYSTATE_DOWN != s) return;
    bool run = gAgent.getAlwaysRun();
    if (run)
    {
        gAgent.clearAlwaysRun();
        gAgent.clearRunning();
    }
    else
    {
        gAgent.setAlwaysRun();
        gAgent.setRunning();
    }
    gAgent.sendWalkRun(!run);
}

void toggle_sit(EKeystate s)
{
    if (KEYSTATE_DOWN != s) return;
    if (gAgent.isSitting())
    {
        gAgent.standUp();
    }
    else
    {
        gAgent.sitDown();
    }
}

void toggle_pause_media(EKeystate s) // analogue of play/pause button in top bar
{
    if (KEYSTATE_DOWN != s) return;
    bool pause = LLViewerMedia::isAnyMediaPlaying();
    LLViewerMedia::setAllMediaPaused(pause);
}

void toggle_enable_media(EKeystate s)
{
    if (KEYSTATE_DOWN != s) return;
    bool pause = LLViewerMedia::isAnyMediaPlaying() || LLViewerMedia::isAnyMediaShowing();
    LLViewerMedia::setAllMediaEnabled(!pause);
}

#define REGISTER_KEYBOARD_ACTION(KEY, ACTION) LLREGISTER_STATIC(LLKeyboardActionRegistry, KEY, ACTION);
REGISTER_KEYBOARD_ACTION("jump", agent_jump);
REGISTER_KEYBOARD_ACTION("push_down", agent_push_down);
REGISTER_KEYBOARD_ACTION("push_forward", agent_push_forward);
REGISTER_KEYBOARD_ACTION("push_backward", agent_push_backward);
REGISTER_KEYBOARD_ACTION("look_up", agent_look_up);
REGISTER_KEYBOARD_ACTION("look_down", agent_look_down);
REGISTER_KEYBOARD_ACTION("toggle_fly", agent_toggle_fly);
REGISTER_KEYBOARD_ACTION("turn_left", agent_turn_left);
REGISTER_KEYBOARD_ACTION("turn_right", agent_turn_right);
REGISTER_KEYBOARD_ACTION("slide_left", agent_slide_left);
REGISTER_KEYBOARD_ACTION("slide_right", agent_slide_right);
REGISTER_KEYBOARD_ACTION("spin_around_ccw", camera_spin_around_ccw);
REGISTER_KEYBOARD_ACTION("spin_around_cw", camera_spin_around_cw);
REGISTER_KEYBOARD_ACTION("spin_around_ccw_sitting", camera_spin_around_ccw_sitting);
REGISTER_KEYBOARD_ACTION("spin_around_cw_sitting", camera_spin_around_cw_sitting);
REGISTER_KEYBOARD_ACTION("spin_over", camera_spin_over);
REGISTER_KEYBOARD_ACTION("spin_under", camera_spin_under);
REGISTER_KEYBOARD_ACTION("spin_over_sitting", camera_spin_over_sitting);
REGISTER_KEYBOARD_ACTION("spin_under_sitting", camera_spin_under_sitting);
REGISTER_KEYBOARD_ACTION("move_forward", camera_move_forward);
REGISTER_KEYBOARD_ACTION("move_backward", camera_move_backward);
REGISTER_KEYBOARD_ACTION("move_forward_sitting", camera_move_forward_sitting);
REGISTER_KEYBOARD_ACTION("move_backward_sitting", camera_move_backward_sitting);
REGISTER_KEYBOARD_ACTION("pan_up", camera_pan_up);
REGISTER_KEYBOARD_ACTION("pan_down", camera_pan_down);
REGISTER_KEYBOARD_ACTION("pan_left", camera_pan_left);
REGISTER_KEYBOARD_ACTION("pan_right", camera_pan_right);
REGISTER_KEYBOARD_ACTION("pan_in", camera_pan_in);
REGISTER_KEYBOARD_ACTION("pan_out", camera_pan_out);
REGISTER_KEYBOARD_ACTION("move_forward_fast", camera_move_forward_fast);
REGISTER_KEYBOARD_ACTION("move_backward_fast", camera_move_backward_fast);
REGISTER_KEYBOARD_ACTION("edit_avatar_spin_ccw", edit_avatar_spin_ccw);
REGISTER_KEYBOARD_ACTION("edit_avatar_spin_cw", edit_avatar_spin_cw);
REGISTER_KEYBOARD_ACTION("edit_avatar_spin_over", edit_avatar_spin_over);
REGISTER_KEYBOARD_ACTION("edit_avatar_spin_under", edit_avatar_spin_under);
REGISTER_KEYBOARD_ACTION("edit_avatar_move_forward", edit_avatar_move_forward);
REGISTER_KEYBOARD_ACTION("edit_avatar_move_backward", edit_avatar_move_backward);
REGISTER_KEYBOARD_ACTION("stop_moving", stop_moving);
REGISTER_KEYBOARD_ACTION("start_chat", start_chat);
REGISTER_KEYBOARD_ACTION("start_gesture", start_gesture);
REGISTER_KEYBOARD_ACTION("toggle_run", toggle_run);
REGISTER_KEYBOARD_ACTION("toggle_sit", toggle_sit);
REGISTER_KEYBOARD_ACTION("toggle_pause_media", toggle_pause_media);
REGISTER_KEYBOARD_ACTION("toggle_enable_media", toggle_enable_media);
#undef REGISTER_KEYBOARD_ACTION

LLViewerKeyboard::LLViewerKeyboard()
{
    resetBindings();

	for (S32 i = 0; i < KEY_COUNT; i++)
	{
		mKeyHandledByUI[i] = FALSE;
    }
    for (S32 i = 0; i < CLICK_COUNT; i++)
    {
        mMouseLevel[i] = MOUSE_STATE_SILENT;
    }
	// we want the UI to never see these keys so that they can always control the avatar/camera
	for(KEY k = KEY_PAD_UP; k <= KEY_PAD_DIVIDE; k++) 
	{
		mKeysSkippedByUI.insert(k);	
	}
}

BOOL LLViewerKeyboard::modeFromString(const std::string& string, S32 *mode) const
{
	if (string == "FIRST_PERSON")
	{
		*mode = MODE_FIRST_PERSON;
		return TRUE;
	}
	else if (string == "THIRD_PERSON")
	{
		*mode = MODE_THIRD_PERSON;
		return TRUE;
	}
	else if (string == "EDIT")
	{
		*mode = MODE_EDIT;
		return TRUE;
	}
	else if (string == "EDIT_AVATAR")
	{
		*mode = MODE_EDIT_AVATAR;
		return TRUE;
	}
	else if (string == "SITTING")
	{
		*mode = MODE_SITTING;
		return TRUE;
	}
	else
	{
		*mode = MODE_THIRD_PERSON;
		return FALSE;
	}
}

BOOL LLViewerKeyboard::mouseFromString(const std::string& string, EMouseClickType *mode) const
{
    if (string == "LMB")
    {
        *mode = CLICK_LEFT;
        return TRUE;
    }
    else if (string == "DLMB")
    {
        *mode = CLICK_DOUBLELEFT;
        return TRUE;
    }
    else if (string == "MMB")
    {
        *mode = CLICK_MIDDLE;
        return TRUE;
    }
    else if (string == "MB4")
    {
        *mode = CLICK_BUTTON4;
        return TRUE;
    }
    else if (string == "MB5")
    {
        *mode = CLICK_BUTTON5;
        return TRUE;
    }
    else
    {
        *mode = CLICK_NONE;
        return FALSE;
    }
}

BOOL LLViewerKeyboard::handleKey(KEY translated_key,  MASK translated_mask, BOOL repeated)
{
	// check for re-map
	EKeyboardMode mode = gViewerKeyboard.getMode();
	U32 keyidx = (translated_mask<<16) | translated_key;
	key_remap_t::iterator iter = mRemapKeys[mode].find(keyidx);
	if (iter != mRemapKeys[mode].end())
	{
		translated_key = (iter->second) & 0xff;
		translated_mask = (iter->second)>>16;
	}

	// No repeats of F-keys
	BOOL repeatable_key = (translated_key < KEY_F1 || translated_key > KEY_F12);
	if (!repeatable_key && repeated)
	{
		return FALSE;
	}

	LL_DEBUGS("UserInput") << "keydown -" << translated_key << "-" << LL_ENDL;
	// skip skipped keys
	if(mKeysSkippedByUI.find(translated_key) != mKeysSkippedByUI.end()) 
	{
		mKeyHandledByUI[translated_key] = FALSE;
		LL_INFOS("KeyboardHandling") << "Key wasn't handled by UI!" << LL_ENDL;
	}
	else
	{
		// it is sufficient to set this value once per call to handlekey
		// without clearing it, as it is only used in the subsequent call to scanKey
		mKeyHandledByUI[translated_key] = gViewerWindow->handleKey(translated_key, translated_mask); 
		// mKeyHandledByUI is not what you think ... this indicates whether the UI has handled this keypress yet (any keypress)
		// NOT whether some UI shortcut wishes to handle the keypress
	  
	}
	return mKeyHandledByUI[translated_key];
}

BOOL LLViewerKeyboard::handleKeyUp(KEY translated_key, MASK translated_mask)
{
	return gViewerWindow->handleKeyUp(translated_key, translated_mask);
}

BOOL LLViewerKeyboard::bindKey(const S32 mode, const KEY key, const MASK mask, const bool ignore, const std::string& function_name)
{
	S32 index;
	typedef boost::function<void(EKeystate)> function_t;
	function_t function = NULL;
	std::string name;

	// Allow remapping of F2-F12
	if (function_name[0] == 'F')
	{
		int c1 = function_name[1] - '0';
		int c2 = function_name[2] ? function_name[2] - '0' : -1;
		if (c1 >= 0 && c1 <= 9 && c2 >= -1 && c2 <= 9)
		{
			int idx = c1;
			if (c2 >= 0)
				idx = idx*10 + c2;
			if (idx >=2 && idx <= 12)
			{
				U32 keyidx = ((mask<<16)|key);
				(mRemapKeys[mode])[keyidx] = ((0<<16)|(KEY_F1+(idx-1)));
				return TRUE;
			}
		}
	}

	// Not remapped, look for a function
	
	function_t* result = LLKeyboardActionRegistry::getValue(function_name);
	if (result)
	{
		function = *result;
	}

	if (!function)
	{
		LL_ERRS() << "Can't bind key to function " << function_name << ", no function with this name found" << LL_ENDL;
		return FALSE;
	}

	// check for duplicate first and overwrite
    if (ignore)
    {
        for (index = 0; index < mKeyIgnoreMaskCount[mode]; index++)
        {
            if (key == mKeyIgnoreMask[mode][index].mKey)
                break;
        }
    }
    else
    {
        for (index = 0; index < mKeyBindingCount[mode]; index++)
        {
            if (key == mKeyBindings[mode][index].mKey && mask == mKeyBindings[mode][index].mMask)
                break;
        }
    }

	if (index >= MAX_KEY_BINDINGS)
	{
		LL_ERRS() << "LLKeyboard::bindKey() - too many keys for mode " << mode << LL_ENDL;
		return FALSE;
	}

	if (mode >= MODE_COUNT)
	{
		LL_ERRS() << "LLKeyboard::bindKey() - unknown mode passed" << mode << LL_ENDL;
		return FALSE;
	}

    if (ignore)
    {
        mKeyIgnoreMask[mode][index].mKey = key;
        mKeyIgnoreMask[mode][index].mFunction = function;

        if (index == mKeyIgnoreMaskCount[mode])
            mKeyIgnoreMaskCount[mode]++;
    }
    else
    {
        mKeyBindings[mode][index].mKey = key;
        mKeyBindings[mode][index].mMask = mask;
        mKeyBindings[mode][index].mFunction = function;

        if (index == mKeyBindingCount[mode])
            mKeyBindingCount[mode]++;
    }

	return TRUE;
}

BOOL LLViewerKeyboard::bindMouse(const S32 mode, const EMouseClickType mouse, const MASK mask, const bool ignore, const std::string& function_name)
{
    S32 index;
    typedef boost::function<void(EKeystate)> function_t;
    function_t function = NULL;

    function_t* result = LLKeyboardActionRegistry::getValue(function_name);
    if (result)
    {
        function = *result;
    }

    if (!function)
    {
        LL_ERRS() << "Can't bind key to function " << function_name << ", no function with this name found" << LL_ENDL;
        return FALSE;
    }

    // check for duplicate first and overwrite
    if (ignore)
    {
        for (index = 0; index < mMouseIgnoreMaskCount[mode]; index++)
        {
            if (mouse == mMouseIgnoreMask[mode][index].mMouse)
                break;
        }
    }
    else
    {
        for (index = 0; index < mMouseBindingCount[mode]; index++)
        {
            if (mouse == mMouseBindings[mode][index].mMouse && mask == mMouseBindings[mode][index].mMask)
                break;
        }
    }

    if (index >= MAX_KEY_BINDINGS)
    {
        LL_ERRS() << "LLKeyboard::bindKey() - too many keys for mode " << mode << LL_ENDL;
        return FALSE;
    }

    if (mode >= MODE_COUNT)
    {
        LL_ERRS() << "LLKeyboard::bindKey() - unknown mode passed" << mode << LL_ENDL;
        return FALSE;
    }

    if (ignore)
    {
        mMouseIgnoreMask[mode][index].mMouse = mouse;
        mMouseIgnoreMask[mode][index].mFunction = function;

        if (index == mMouseIgnoreMaskCount[mode])
            mMouseIgnoreMaskCount[mode]++;
    }
    else
    {
        mMouseBindings[mode][index].mMouse = mouse;
        mMouseBindings[mode][index].mMask = mask;
        mMouseBindings[mode][index].mFunction = function;

        if (index == mMouseBindingCount[mode])
            mMouseBindingCount[mode]++;
    }

    return TRUE;
}

LLViewerKeyboard::KeyBinding::KeyBinding()
:	key("key"),
	mouse("mouse"),
	mask("mask"),
	command("command"),
	ignore("ignore", false)
{}

LLViewerKeyboard::KeyMode::KeyMode()
:	bindings("binding")
{}

LLViewerKeyboard::Keys::Keys()
:	first_person("first_person"),
	third_person("third_person"),
	edit("edit"),
	sitting("sitting"),
	edit_avatar("edit_avatar")
{}

void LLViewerKeyboard::resetBindings()
{
    for (S32 i = 0; i < MODE_COUNT; i++)
    {
        mKeyBindingCount[i] = 0;
        mKeyIgnoreMaskCount[i] = 0;
        mMouseBindingCount[i] = 0;
        mMouseIgnoreMaskCount[i] = 0;
    }
}

S32 LLViewerKeyboard::loadBindingsXML(const std::string& filename)
{
    resetBindings();

	S32 binding_count = 0;
	Keys keys;
	LLSimpleXUIParser parser;

	if (parser.readXUI(filename, keys) 
		&& keys.validateBlock())
	{
		binding_count += loadBindingMode(keys.first_person, MODE_FIRST_PERSON);
		binding_count += loadBindingMode(keys.third_person, MODE_THIRD_PERSON);
		binding_count += loadBindingMode(keys.edit, MODE_EDIT);
		binding_count += loadBindingMode(keys.sitting, MODE_SITTING);
		binding_count += loadBindingMode(keys.edit_avatar, MODE_EDIT_AVATAR);
	}
	return binding_count;
}

S32 LLViewerKeyboard::loadBindingMode(const LLViewerKeyboard::KeyMode& keymode, S32 mode)
{
	S32 binding_count = 0;
	for (LLInitParam::ParamIterator<KeyBinding>::const_iterator it = keymode.bindings.begin(), 
			end_it = keymode.bindings.end();
		it != end_it;
		++it)
	{
        if (!it->key.getValue().empty())
        {
            KEY key;
            MASK mask;
            bool ignore = it->ignore.isProvided() ? it->ignore.getValue() : false;
            LLKeyboard::keyFromString(it->key, &key);
            LLKeyboard::maskFromString(it->mask, &mask);
            bindKey(mode, key, mask, ignore, it->command);
            binding_count++;
        }
        else if (it->mouse.isProvided() && !it->mouse.getValue().empty())
        {
            EMouseClickType mouse;
            MASK mask;
            bool ignore = it->ignore.isProvided() ? it->ignore.getValue() : false;
            mouseFromString(it->mouse.getValue(), &mouse);
            LLKeyboard::maskFromString(it->mask, &mask);
            bindMouse(mode, mouse, mask, ignore, it->command);
            binding_count++;
        }
	}

	return binding_count;
}

S32 LLViewerKeyboard::loadBindings(const std::string& filename)
{
	LLFILE *fp;
	const S32 BUFFER_SIZE = 2048;
	char buffer[BUFFER_SIZE];	/* Flawfinder: ignore */
	// *NOTE: This buffer size is hard coded into scanf() below.
	char mode_string[MAX_STRING] = "";	/* Flawfinder: ignore */
	char key_string[MAX_STRING] = "";	/* Flawfinder: ignore */
	char mask_string[MAX_STRING] = "";	/* Flawfinder: ignore */
	char function_string[MAX_STRING] = "";	/* Flawfinder: ignore */
	S32 mode = MODE_THIRD_PERSON;
	KEY key = 0;
	MASK mask = 0;
	S32 tokens_read;
	S32 binding_count = 0;
	S32 line_count = 0;

	if(filename.empty())
	{
		LL_ERRS() << " No filename specified" << LL_ENDL;
		return 0;
	}

	fp = LLFile::fopen(filename, "r");

	if (!fp)
	{
		return 0;
	}


	while (!feof(fp))
	{
		line_count++;
		if (!fgets(buffer, BUFFER_SIZE, fp))
			break;

		// skip over comments, blank lines
		if (buffer[0] == '#' || buffer[0] == '\n') continue;

		// grab the binding strings
		tokens_read = sscanf(	/* Flawfinder: ignore */
			buffer,
			"%254s %254s %254s %254s",
			mode_string,
			key_string,
			mask_string,
			function_string);

		if (tokens_read == EOF)
		{
			LL_INFOS() << "Unexpected end-of-file at line " << line_count << " of key binding file " << filename << LL_ENDL;
			fclose(fp);
			return 0;
		}
		else if (tokens_read < 4)
		{
			LL_INFOS() << "Can't read line " << line_count << " of key binding file " << filename << LL_ENDL;
			continue;
		}

		// convert mode
		if (!modeFromString(mode_string, &mode))
		{
			LL_INFOS() << "Unknown mode on line " << line_count << " of key binding file " << filename << LL_ENDL;
			LL_INFOS() << "Mode must be one of FIRST_PERSON, THIRD_PERSON, EDIT, EDIT_AVATAR" << LL_ENDL;
			continue;
		}

		// convert key
		if (!LLKeyboard::keyFromString(key_string, &key))
		{
			LL_INFOS() << "Can't interpret key on line " << line_count << " of key binding file " << filename << LL_ENDL;
			continue;
		}

		// convert mask
		if (!LLKeyboard::maskFromString(mask_string, &mask))
		{
			LL_INFOS() << "Can't interpret mask on line " << line_count << " of key binding file " << filename << LL_ENDL;
			continue;
		}

		// bind key
		if (bindKey(mode, key, mask, false, function_string))
		{
			binding_count++;
		}
	}

	fclose(fp);

	return binding_count;
}


EKeyboardMode LLViewerKeyboard::getMode() const
{
	if ( gAgentCamera.cameraMouselook() )
	{
		return MODE_FIRST_PERSON;
	}
	else if ( gMorphView && gMorphView->getVisible())
	{
		return MODE_EDIT_AVATAR;
	}
	else if (isAgentAvatarValid() && gAgentAvatarp->isSitting())
	{
		return MODE_SITTING;
	}
	else
	{
		return MODE_THIRD_PERSON;
	}
}

bool LLViewerKeyboard::scanKey(const LLKeyboardBinding* binding,
                               S32 binding_count,
                               KEY key,
                               MASK mask,
                               BOOL key_down,
                               BOOL key_up,
                               BOOL key_level,
                               bool repeat) const
{
	for (S32 i = 0; i < binding_count; i++)
	{
		if (binding[i].mKey == key)
		{
			if (binding[i].mMask == mask)
			{
				if (key_down && !repeat)
				{
					// ...key went down this frame, call function
					binding[i].mFunction( KEYSTATE_DOWN );
					return true;
				}
				else if (key_up)
				{
					// ...key went down this frame, call function
					binding[i].mFunction( KEYSTATE_UP );
					return true;
				}
				else if (key_level)
				{
					// ...key held down from previous frame
					// Not windows, just call the function.
					binding[i].mFunction( KEYSTATE_LEVEL );
					return true;
				}//if
				// Key+Mask combinations are supposed to be unique, so we won't find anything else
				return false;
			}//if
		}//if
	}//for
	return false;
}

// Called from scanKeyboard.
void LLViewerKeyboard::scanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level) const
{
	if (LLApp::isExiting())
	{
		return;
	}

	S32 mode = getMode();
	// Consider keyboard scanning as NOT mouse event. JC
	MASK mask = gKeyboard->currentMask(FALSE);

	if (mKeyHandledByUI[key])
	{
		return;
	}

	// don't process key down on repeated keys
	BOOL repeat = gKeyboard->getKeyRepeated(key);

    if (scanKey(mKeyBindings[mode], mKeyBindingCount[mode], key, mask, key_down, key_up, key_level, repeat))
    {
        // Nothing found, try ignore list
        scanKey(mKeyIgnoreMask[mode], mKeyIgnoreMaskCount[mode], key, MASK_NONE, key_down, key_up, key_level, repeat);
    }
}

BOOL LLViewerKeyboard::handleMouse(LLWindow *window_impl, LLCoordGL pos, MASK mask, EMouseClickType clicktype, BOOL down)
{
    BOOL handled = gViewerWindow->handleAnyMouseClick(window_impl, pos, mask, clicktype, down);

    if (clicktype != CLICK_NONE)
    {
        // special case
        // if UI doesn't handle double click, LMB click is issued, so supres LMB 'down' when doubleclick is set
        // handle !down as if we are handling doubleclick
        bool override_lmb = (clicktype == CLICK_LEFT
            && (mMouseLevel[CLICK_DOUBLELEFT] == MOUSE_STATE_DOWN || mMouseLevel[CLICK_DOUBLELEFT] == MOUSE_STATE_LEVEL));

        if (override_lmb && !down)
        {
            // process doubleclick instead
            clicktype = CLICK_DOUBLELEFT;
        }

        if (override_lmb && down)
        {
            // else-supress
        }
        // if UI handled 'down', it should handle 'up' as well
        // if we handle 'down' not by UI, then we should handle 'up'/'level' regardless of UI
        else if (handled && mMouseLevel[clicktype] != MOUSE_STATE_SILENT)
        {
            // UI handled new 'down' so iterupt whatever state we were in.
            mMouseLevel[clicktype] = MOUSE_STATE_UP;
        }
        else if (down)
        {
            if (mMouseLevel[clicktype] == MOUSE_STATE_DOWN)
            {
                // this is repeated hit (mouse does not repeat event until release)
                // for now treat rapid clicking like mouse being held
                mMouseLevel[clicktype] = MOUSE_STATE_LEVEL;
            }
            else
            {
                mMouseLevel[clicktype] = MOUSE_STATE_DOWN;
            }
        }
        else
        {
            // Released mouse key
            mMouseLevel[clicktype] = MOUSE_STATE_UP;
        }
    }

    return handled;
}

bool LLViewerKeyboard::scanMouse(const LLMouseBinding *binding, S32 binding_count, EMouseClickType mouse, MASK mask, EMouseState state) const
{
    for (S32 i = 0; i < binding_count; i++)
    {
        if (binding[i].mMouse == mouse && binding[i].mMask == mask)
        {
            switch (state)
            {
            case MOUSE_STATE_DOWN:
                binding[i].mFunction(KEYSTATE_DOWN);
                break;
            case MOUSE_STATE_LEVEL:
                binding[i].mFunction(KEYSTATE_LEVEL);
                break;
            case MOUSE_STATE_UP:
                binding[i].mFunction(KEYSTATE_UP);
                break;
            default:
                break;
            }
            // Key+Mask combinations are supposed to be unique, no need to continue
            return true;
        }
    }
    return false;
}

// todo: this recods key, scanMouse() triggers functions with EKeystate
bool LLViewerKeyboard::scanMouse(EMouseClickType click, EMouseState state) const
{
    bool res = false;
    S32 mode = getMode();
    MASK mask = gKeyboard->currentMask(TRUE);
    res = scanMouse(mMouseBindings[mode], mMouseBindingCount[mode], click, mask, state);
    if (!res)
    {
        res = scanMouse(mMouseIgnoreMask[mode], mMouseIgnoreMaskCount[mode], click, MASK_NONE, state);
    }
    return res;
}

void LLViewerKeyboard::scanMouse()
{
    for (S32 i = 0; i < CLICK_COUNT; i++)
    {
        if (mMouseLevel[i] != MOUSE_STATE_SILENT)
        {
            scanMouse((EMouseClickType)i, mMouseLevel[i]);
            if (mMouseLevel[i] == MOUSE_STATE_DOWN)
            {
                // mouse doesn't support 'continued' state like keyboard does, so after handling, switch to LEVEL
                mMouseLevel[i] = MOUSE_STATE_LEVEL;
            }
            else if (mMouseLevel[i] == MOUSE_STATE_UP)
            {
                mMouseLevel[i] = MOUSE_STATE_SILENT;
            }
        }
    }
}
