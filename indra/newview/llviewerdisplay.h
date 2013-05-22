/** 
 * @file llviewerdisplay.h
 * @brief LLViewerDisplay class header file
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

#ifndef LL_LLVIEWERDISPLAY_H
#define LL_LLVIEWERDISPLAY_H

class LLPostProcess;

void display_startup();
void display_cleanup();

void display(BOOL rebuild = TRUE, F32 zoom_factor = 1.f, int subfield = 0, BOOL for_snapshot = FALSE);

extern BOOL gDisplaySwapBuffers;
extern BOOL gDepthDirty;
extern BOOL	gTeleportDisplay;
extern LLFrameTimer	gTeleportDisplayTimer;
extern BOOL			gForceRenderLandFence;
extern BOOL gResizeScreenTexture;
extern BOOL gWindowResized;

#endif // LL_LLVIEWERDISPLAY_H
