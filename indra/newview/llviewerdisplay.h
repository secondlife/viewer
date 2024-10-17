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

void display_startup();
void display_cleanup();

void display(bool rebuild = true, F32 zoom_factor = 1.f, int subfield = 0, bool for_snapshot = false);

extern bool gDisplaySwapBuffers;
extern bool gDepthDirty;
extern bool gTeleportDisplay;
extern LLFrameTimer gTeleportDisplayTimer;
extern bool         gForceRenderLandFence;
extern bool gResizeScreenTexture;
extern bool gResizeShadowTexture;
extern bool gWindowResized;

#endif // LL_LLVIEWERDISPLAY_H
