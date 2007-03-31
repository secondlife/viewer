/** 
 * @file llviewerjoystick.h
 * @brief Viewer joystick functionality.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERJOYSTICK_H
#define LL_LLVIEWERJOYSTICK_H

class LLViewerJoystick
{
public:
	static BOOL sOverrideCamera;
	static void scanJoystick();
	static void updateCamera(BOOL reset = FALSE);
};

#endif
