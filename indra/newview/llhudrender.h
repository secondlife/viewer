/** 
 * @file llhudrender.h
 * @brief LLHUDRender class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

