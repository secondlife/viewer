/** 
 * @file llviewerjoystick.cpp
 * @brief Joystick functionality.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llviewerjoystick.h"
#include "viewer.h"
#include "llkeyboard.h"

static LLQuaternion sFlycamRotation;
static LLVector3 sFlycamPosition;
static F32		sFlycamZoom;

BOOL  LLViewerJoystick::sOverrideCamera = FALSE;

void LLViewerJoystick::updateCamera(BOOL reset)
{
	static F32 last_delta[] = {0,0,0,0,0,0,0};
	static F32 delta[] = { 0,0,0,0,0,0,0 };

	LLWindow* window = gViewerWindow->getWindow();

	F32 time = gFrameIntervalSeconds;

	S32 axis[] = 
	{
		gSavedSettings.getS32("FlycamAxis0"),
		gSavedSettings.getS32("FlycamAxis1"),
		gSavedSettings.getS32("FlycamAxis2"),
		gSavedSettings.getS32("FlycamAxis3"),
		gSavedSettings.getS32("FlycamAxis4"),
		gSavedSettings.getS32("FlycamAxis5"),
		gSavedSettings.getS32("FlycamAxis6")
	};

	F32 axis_scale[] =
	{
		gSavedSettings.getF32("FlycamAxisScale0"),
		gSavedSettings.getF32("FlycamAxisScale1"),
		gSavedSettings.getF32("FlycamAxisScale2"),
		gSavedSettings.getF32("FlycamAxisScale3"),
		gSavedSettings.getF32("FlycamAxisScale4"),
		gSavedSettings.getF32("FlycamAxisScale5"),
		gSavedSettings.getF32("FlycamAxisScale6")
	};

	F32 dead_zone[] =
	{
		gSavedSettings.getF32("FlycamAxisDeadZone0"),
		gSavedSettings.getF32("FlycamAxisDeadZone1"),
		gSavedSettings.getF32("FlycamAxisDeadZone2"),
		gSavedSettings.getF32("FlycamAxisDeadZone3"),
		gSavedSettings.getF32("FlycamAxisDeadZone4"),
		gSavedSettings.getF32("FlycamAxisDeadZone5"),
		gSavedSettings.getF32("FlycamAxisDeadZone6")
	};

	if (reset)
	{
		sFlycamPosition = gCamera->getOrigin();
		sFlycamRotation = gCamera->getQuaternion();
		sFlycamZoom = gCamera->getView();

		for (U32 i = 0; i < 7; i++)
		{
			last_delta[i] = -window->getJoystickAxis(axis[i]);
			delta[i] = 0.f;
		}
		return;
	}

	F32 cur_delta[7];
	F32 feather = gSavedSettings.getF32("FlycamFeathering");
	BOOL absolute = gSavedSettings.getBOOL("FlycamAbsolute");

	for (U32 i = 0; i < 7; i++)
	{
		cur_delta[i] = -window->getJoystickAxis(axis[i]);	
		F32 tmp = cur_delta[i];
		if (absolute)
		{
			cur_delta[i] = cur_delta[i] - last_delta[i];
		}
		last_delta[i] = tmp;

		if (cur_delta[i] > 0)
		{
			cur_delta[i] = llmax(cur_delta[i]-dead_zone[i], 0.f);
		}
		else
		{
			cur_delta[i] = llmin(cur_delta[i]+dead_zone[i], 0.f);
		}
		cur_delta[i] *= axis_scale[i];
		
		if (!absolute)
		{
			cur_delta[i] *= time;
		}

		delta[i] = delta[i] + (cur_delta[i]-delta[i])*time*feather;
	}
	
	sFlycamPosition += LLVector3(delta) * sFlycamRotation;

	LLMatrix3 rot_mat(delta[3],
					  delta[4],
					  delta[5]);
	
	sFlycamRotation = LLQuaternion(rot_mat)*sFlycamRotation;

	if (gSavedSettings.getBOOL("FlycamAutoLeveling"))
	{
		LLMatrix3 level(sFlycamRotation);

		LLVector3 x = LLVector3(level.mMatrix[0]);
		LLVector3 y = LLVector3(level.mMatrix[1]);
		LLVector3 z = LLVector3(level.mMatrix[2]);

		y.mV[2] = 0.f;
		y.normVec();

		level.setRows(x,y,z);
		level.orthogonalize();
				
		LLQuaternion quat = LLQuaternion(level);
		sFlycamRotation = nlerp(llmin(feather*time,1.f), sFlycamRotation, quat);
	}

	if (gSavedSettings.getBOOL("FlycamZoomDirect"))
	{
		sFlycamZoom = last_delta[6]*axis_scale[6]+dead_zone[6];
	}
	else
	{
		sFlycamZoom += delta[6];
	}

	LLMatrix3 mat(sFlycamRotation);

	gCamera->setView(sFlycamZoom);
	gCamera->setOrigin(sFlycamPosition);
	gCamera->mXAxis = LLVector3(mat.mMatrix[0]);
	gCamera->mYAxis = LLVector3(mat.mMatrix[1]);
	gCamera->mZAxis = LLVector3(mat.mMatrix[2]);
}


void LLViewerJoystick::scanJoystick()
{
	if (!sOverrideCamera)
	{
		static U32 joystick_state = 0;
		static U32 button_state = 0;

		F32 xval = gViewerWindow->getWindow()->getJoystickAxis(0);
		F32 yval = gViewerWindow->getWindow()->getJoystickAxis(1);

		if (xval <= -0.5f)
		{
			if (!(joystick_state & 0x1))
			{
				gKeyboard->handleTranslatedKeyDown(KEY_PAD_LEFT, 0);
				joystick_state |= 0x1;
			}
		}
		else 
		{
			if (joystick_state & 0x1)
			{
				gKeyboard->handleTranslatedKeyUp(KEY_PAD_LEFT, 0);
				joystick_state &= ~0x1;
			}
		}
		if (xval >= 0.5f)
		{
			if (!(joystick_state & 0x2))
			{
				gKeyboard->handleTranslatedKeyDown(KEY_PAD_RIGHT, 0);
				joystick_state |= 0x2;
			}
		}
		else 
		{
			if (joystick_state & 0x2)
			{
				gKeyboard->handleTranslatedKeyUp(KEY_PAD_RIGHT, 0);
				joystick_state &= ~0x2;
			}
		}
		if (yval <= -0.5f)
		{
			if (!(joystick_state & 0x4))
			{
				gKeyboard->handleTranslatedKeyDown(KEY_PAD_UP, 0);
				joystick_state |= 0x4;
			}
		}
		else 
		{
			if (joystick_state & 0x4)
			{
				gKeyboard->handleTranslatedKeyUp(KEY_PAD_UP, 0);
				joystick_state &= ~0x4;
			}
		}
		if (yval >=  0.5f)
		{
			if (!(joystick_state & 0x8))
			{
				gKeyboard->handleTranslatedKeyDown(KEY_PAD_DOWN, 0);
				joystick_state |= 0x8;
			}
		}
		else 
		{
			if (joystick_state & 0x8)
			{
				gKeyboard->handleTranslatedKeyUp(KEY_PAD_DOWN, 0);
				joystick_state &= ~0x8;
			}
		}

		for( int i = 0; i < 15; i++ )
		{
			if ( gViewerWindow->getWindow()->getJoystickButton(i) & 0x80 )
			{
				if (!(button_state & (1<<i)))
				{
					gKeyboard->handleTranslatedKeyDown(KEY_BUTTON1+i, 0);
					button_state |= (1<<i);
				}
			}
			else
			{
				if (button_state & (1<<i))
				{
					gKeyboard->handleTranslatedKeyUp(KEY_BUTTON1+i, 0);
					button_state &= ~(1<<i);
				}
			}
		}
	}
}

