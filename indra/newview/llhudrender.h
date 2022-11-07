/** 
 * @file llhudrender.h
 * @brief LLHUDRender class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLHUDRENDER_H
#define LL_LLHUDRENDER_H

#include "llfontgl.h"

class LLVector3;
class LLFontGL;

// Utility classes for rendering HUD elements
void hud_render_text(const LLWString &wstr,
                     const LLVector3 &pos_agent,
                     const LLFontGL &font,
                     const U8 style,
                     const LLFontGL::ShadowType, 
                     const F32 x_offset,
                     const F32 y_offset,
                     const LLColor4& color,
                     const BOOL orthographic);

// Legacy, slower
void hud_render_utf8text(const std::string &str,
                         const LLVector3 &pos_agent,
                         const LLFontGL &font,
                         const U8 style,
                        const LLFontGL::ShadowType, 
                         const F32 x_offset,
                         const F32 y_offset,
                         const LLColor4& color,
                         const BOOL orthographic);


#endif //LL_LLHUDRENDER_H

