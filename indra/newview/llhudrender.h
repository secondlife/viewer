/** 
 * @file llhudrender.h
 * @brief LLHUDRender class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
					 const F32 x_offset,
					 const F32 y_offset,
					 const LLColor4& color,
					 const BOOL orthographic);

// Legacy, slower
void hud_render_utf8text(const std::string &str,
						 const LLVector3 &pos_agent,
						 const LLFontGL &font,
						 const U8 style,
						 const F32 x_offset,
						 const F32 y_offset,
						 const LLColor4& color,
						 const BOOL orthographic);


#endif //LL_LLHUDRENDER_H

