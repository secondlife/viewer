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
#include "llviewerkeyboard.h"
#include "llmath.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llnearbychat.h"
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
const F32 YAW_NUDGE_RATE = 0.05f;  // fraction of normal speed

struct LLKeyboardActionRegistry 
:	public LLRegistrySingleton<std::string, boost::function<void (EKeystate keystate)>, LLKeyboardActionRegistry>
{
};

LLViewerKeyboard gViewerKeyboard;

void agent_jump( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	F32 time = gKeyboard->getCurKeyElapsedTime();
	S32 frame_count = llround(gKeyboard->getCurKeyElapsedFrameCount());

	if( time < FLY_TIME 
		|| frame_count <= FLY_FRAMES 
		|| gAgent.upGrabbed()
		|| !gSavedSettings.getBOOL("AutomaticFly"))
	{
		gAgent.moveUp(1);
	}
	else
	{
		gAgent.setFlying(TRUE);
		gAgent.moveUp(1);
	}
}

void agent_push_down( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	gAgent.moveUp(-1);
}

static void agent_handle_doubletap_run(EKeystate s, LLAgent::EDoubleTapRunMode mode)
{
	if (KEYSTATE_UP == s)
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
	S32 frame_count = llround(gKeyboard->getCurKeyElapsedFrameCount());

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
	//in free camera control mode we need to intercept keyboard events for avatar movements
	if (LLFloaterCamera::inFreeCameraMode())
	{
		camera_move_forward(s);
		return;
	}
	agent_push_forwardbackward(s, 1, LLAgent::DOUBLETAP_FORWARD);
}

void camera_move_backward( EKeystate s );

void agent_push_backward( EKeystate s )
{
	//in free camera control mode we need to intercept keyboard events for avatar movements
	if (LLFloaterCamera::inFreeCameraMode())
	{
		camera_move_backward(s);
		return;
	}
	agent_push_forwardbackward(s, -1, LLAgent::DOUBLETAP_BACKWARD);
}

static void agent_slide_leftright( EKeystate s, S32 direction, LLAgent::EDoubleTapRunMode mode )
{
	agent_handle_doubletap_run(s, mode);
	if( KEYSTATE_UP == s ) return;
	F32 time = gKeyboard->getCurKeyElapsedTime();
	S32 frame_count = llround(gKeyboard->getCurKeyElapsedFrameCount());

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
	agent_slide_leftright(s, 1, LLAgent::DOUBLETAP_SLIDELEFT);
}


void agent_slide_right( EKeystate s )
{
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

	if (LLToolCamera::getInstance()->mouseSteerMode())
	{
		agent_slide_left(s);
	}
	else
	{
		if (KEYSTATE_UP == s) return;
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

	if (LLToolCamera::getInstance()->mouseSteerMode())
	{
		agent_slide_right(s);
	}
	else
	{
		if (KEYSTATE_UP == s) return;
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
		//llinfos << rate << llendl;
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
	if( KEYSTATE_UP == s ) return;
	if (gAgent.rotateGrabbed() || gAgentCamera.sitCameraEnabled())
	{
		//send keystrokes, but do not change camera
		agent_turn_right(s);
	}
	else
	{
		//change camera but do not send keystrokes
		gAgentCamera.setOrbitLeftKey( get_orbit_rate() );
	}
}


void camera_spin_around_cw_sitting( EKeystate s )
{
	if( KEYSTATE_UP == s  ) return;
	if (gAgent.rotateGrabbed() || gAgentCamera.sitCameraEnabled())
	{
		//send keystrokes, but do not change camera
		agent_turn_left(s);
	}
	else
	{
		//change camera but do not send keystrokes
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
	if( KEYSTATE_UP == s  ) return;
	if (gAgent.forwardGrabbed() || gAgentCamera.sitCameraEnabled())
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
	if( KEYSTATE_UP == s  ) return;

	if (gAgent.backwardGrabbed() || gAgentCamera.sitCameraEnabled())
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
	// start chat
	LLNearbyChat::startChat(NULL);
}

void start_gesture( EKeystate s )
{
	LLUICtrl* focus_ctrlp = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
	if (KEYSTATE_UP == s &&
		! (focus_ctrlp && focus_ctrlp->acceptsTextInput()))
	{
 		if (LLNearbyChat::getInstance()->getCurrentChat().empty())
 		{
 			// No existing chat in chat editor, insert '/'
 			LLNearbyChat::startChat("/");
 		}
 		else
 		{
 			// Don't overwrite existing text in chat editor
 			LLNearbyChat::startChat(NULL);
 		}
	}
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
#undef REGISTER_KEYBOARD_ACTION

LLViewerKeyboard::LLViewerKeyboard()
{
	for (S32 i = 0; i < MODE_COUNT; i++)
	{
		mBindingCount[i] = 0;
	}

	for (S32 i = 0; i < KEY_COUNT; i++)
	{
		mKeyHandledByUI[i] = FALSE;
	}
	// we want the UI to never see these keys so that they can always control the avatar/camera
	for(KEY k = KEY_PAD_UP; k <= KEY_PAD_DIVIDE; k++) 
	{
		mKeysSkippedByUI.insert(k);	
	}
}

BOOL LLViewerKeyboard::modeFromString(const std::string& string, S32 *mode)
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

	lldebugst(LLERR_USER_INPUT) << "keydown -" << translated_key << "-" << llendl;
	// skip skipped keys
	if(mKeysSkippedByUI.find(translated_key) != mKeysSkippedByUI.end()) 
	{
		mKeyHandledByUI[translated_key] = FALSE;
	}
	else
	{
		// it is sufficient to set this value once per call to handlekey
		// without clearing it, as it is only used in the subsequent call to scanKey
		mKeyHandledByUI[translated_key] = gViewerWindow->handleKey(translated_key, translated_mask);
	}
	return mKeyHandledByUI[translated_key];
}



BOOL LLViewerKeyboard::bindKey(const S32 mode, const KEY key, const MASK mask, const std::string& function_name)
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
		llerrs << "Can't bind key to function " << function_name << ", no function with this name found" << llendl;
		return FALSE;
	}

	// check for duplicate first and overwrite
	for (index = 0; index < mBindingCount[mode]; index++)
	{
		if (key == mBindings[mode][index].mKey && mask == mBindings[mode][index].mMask)
			break;
	}

	if (index >= MAX_KEY_BINDINGS)
	{
		llerrs << "LLKeyboard::bindKey() - too many keys for mode " << mode << llendl;
		return FALSE;
	}

	if (mode >= MODE_COUNT)
	{
		llerror("LLKeyboard::bindKey() - unknown mode passed", mode);
		return FALSE;
	}

	mBindings[mode][index].mKey = key;
	mBindings[mode][index].mMask = mask;
	mBindings[mode][index].mFunction = function;

	if (index == mBindingCount[mode])
		mBindingCount[mode]++;

	return TRUE;
}

LLViewerKeyboard::KeyBinding::KeyBinding()
:	key("key"),
	mask("mask"),
	command("command")
{}

LLViewerKeyboard::KeyMode::KeyMode(EKeyboardMode _mode)
:	bindings("binding"),
	mode(_mode)
{}

LLViewerKeyboard::Keys::Keys()
:	first_person("first_person", KeyMode(MODE_FIRST_PERSON)),
	third_person("third_person", KeyMode(MODE_THIRD_PERSON)),
	edit("edit", KeyMode(MODE_EDIT)),
	sitting("sitting", KeyMode(MODE_SITTING)),
	edit_avatar("edit_avatar", KeyMode(MODE_EDIT_AVATAR))
{}

S32 LLViewerKeyboard::loadBindingsXML(const std::string& filename)
{
	S32 binding_count = 0;
	Keys keys;
	LLSimpleXUIParser parser;

	if (parser.readXUI(filename, keys) 
		&& keys.validateBlock())
	{
		binding_count += loadBindingMode(keys.first_person);
		binding_count += loadBindingMode(keys.third_person);
		binding_count += loadBindingMode(keys.edit);
		binding_count += loadBindingMode(keys.sitting);
		binding_count += loadBindingMode(keys.edit_avatar);
	}
	return binding_count;
}

S32 LLViewerKeyboard::loadBindingMode(const LLViewerKeyboard::KeyMode& keymode)
{
	S32 binding_count = 0;
	for (LLInitParam::ParamIterator<KeyBinding>::const_iterator it = keymode.bindings.begin(), 
			end_it = keymode.bindings.end();
		it != end_it;
		++it)
	{
		KEY key;
		MASK mask;
		LLKeyboard::keyFromString(it->key, &key);
		LLKeyboard::maskFromString(it->mask, &mask);
		bindKey(keymode.mode, key, mask, it->command);
		binding_count++;
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
		llerrs << " No filename specified" << llendl;
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
			llinfos << "Unexpected end-of-file at line " << line_count << " of key binding file " << filename << llendl;
			fclose(fp);
			return 0;
		}
		else if (tokens_read < 4)
		{
			llinfos << "Can't read line " << line_count << " of key binding file " << filename << llendl;
			continue;
		}

		// convert mode
		if (!modeFromString(mode_string, &mode))
		{
			llinfos << "Unknown mode on line " << line_count << " of key binding file " << filename << llendl;
			llinfos << "Mode must be one of FIRST_PERSON, THIRD_PERSON, EDIT, EDIT_AVATAR" << llendl;
			continue;
		}

		// convert key
		if (!LLKeyboard::keyFromString(key_string, &key))
		{
			llinfos << "Can't interpret key on line " << line_count << " of key binding file " << filename << llendl;
			continue;
		}

		// convert mask
		if (!LLKeyboard::maskFromString(mask_string, &mask))
		{
			llinfos << "Can't interpret mask on line " << line_count << " of key binding file " << filename << llendl;
			continue;
		}

		// bind key
		if (bindKey(mode, key, mask, function_string))
		{
			binding_count++;
		}
	}

	fclose(fp);

	return binding_count;
}


EKeyboardMode LLViewerKeyboard::getMode()
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


// Called from scanKeyboard.
void LLViewerKeyboard::scanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level)
{
	S32 mode = getMode();
	// Consider keyboard scanning as NOT mouse event. JC
	MASK mask = gKeyboard->currentMask(FALSE);

	LLKeyBinding* binding = mBindings[mode];
	S32 binding_count = mBindingCount[mode];


	if (mKeyHandledByUI[key])
	{
		return;
	}

	// don't process key down on repeated keys
	BOOL repeat = gKeyboard->getKeyRepeated(key);

	for (S32 i = 0; i < binding_count; i++)
	{
		//for (S32 key = 0; key < KEY_COUNT; key++)
		if (binding[i].mKey == key)
		{
			//if (binding[i].mKey == key && binding[i].mMask == mask)
			if (binding[i].mMask == mask)
			{
				if (key_down && !repeat)
				{
					// ...key went down this frame, call function
					binding[i].mFunction( KEYSTATE_DOWN );
				}
				else if (key_up)
				{
					// ...key went down this frame, call function
					binding[i].mFunction( KEYSTATE_UP );
				}
				else if (key_level)
				{
					// ...key held down from previous frame
					// Not windows, just call the function.
					binding[i].mFunction( KEYSTATE_LEVEL );
				}//if
			}//if
		}//for
	}//for
}
