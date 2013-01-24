/** 
 * @file llfontgl.cpp
 * @brief Wrapper around FreeType
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

#include "llfontgl.h"

// Linden library includes
#include "llfontfreetype.h"
#include "llfontbitmapcache.h"
#include "llfontregistry.h"
#include "llgl.h"
#include "llimagegl.h"
#include "llrender.h"
#include "llstl.h"
#include "v4color.h"
#include "lltexture.h"
#include "lldir.h"

// Third party library includes
#include <boost/tokenizer.hpp>

const S32 BOLD_OFFSET = 1;

// static class members
F32 LLFontGL::sVertDPI = 96.f;
F32 LLFontGL::sHorizDPI = 96.f;
F32 LLFontGL::sScaleX = 1.f;
F32 LLFontGL::sScaleY = 1.f;
BOOL LLFontGL::sDisplayFont = TRUE ;
std::string LLFontGL::sAppDir;

LLColor4 LLFontGL::sShadowColor(0.f, 0.f, 0.f, 1.f);
LLFontRegistry* LLFontGL::sFontRegistry = NULL;

LLCoordGL LLFontGL::sCurOrigin;
F32 LLFontGL::sCurDepth;
std::vector<std::pair<LLCoordGL, F32> > LLFontGL::sOriginStack;

const F32 EXT_X_BEARING = 1.f;
const F32 EXT_Y_BEARING = 0.f;
const F32 EXT_KERNING = 1.f;
const F32 PIXEL_BORDER_THRESHOLD = 0.0001f;
const F32 PIXEL_CORRECTION_DISTANCE = 0.01f;

const F32 PAD_UVY = 0.5f; // half of vertical padding between glyphs in the glyph texture
const F32 DROP_SHADOW_SOFT_STRENGTH = 0.3f;

LLFontGL::LLFontGL()
{
}

LLFontGL::~LLFontGL()
{
}

void LLFontGL::reset()
{
	mFontFreetype->reset(sVertDPI, sHorizDPI);
}

void LLFontGL::destroyGL()
{
	mFontFreetype->destroyGL();
}

BOOL LLFontGL::loadFace(const std::string& filename, F32 point_size, F32 vert_dpi, F32 horz_dpi, S32 components, BOOL is_fallback)
{
	if(mFontFreetype == reinterpret_cast<LLFontFreetype*>(NULL))
	{
		mFontFreetype = new LLFontFreetype;
	}

	return mFontFreetype->loadFace(filename, point_size, vert_dpi, horz_dpi, components, is_fallback);
}

static LLFastTimer::DeclareTimer FTM_RENDER_FONTS("Fonts");

S32 LLFontGL::render(const LLWString &wstr, S32 begin_offset, const LLRect& rect, const LLColor4 &color, HAlign halign, VAlign valign, U8 style, 
					 ShadowType shadow, S32 max_chars, F32* right_x, BOOL use_ellipses) const
{
	F32 x = (F32)rect.mLeft;
	F32 y = 0.f;

	switch(valign)
	{
	case TOP:
		y = (F32)rect.mTop;
		break;
	case VCENTER:
		y = (F32)rect.getCenterY();
		break;
	case BASELINE:
	case BOTTOM:
		y = (F32)rect.mBottom;
		break;
	default:
		y = (F32)rect.mBottom;
		break;
	}
	return render(wstr, begin_offset, x, y, color, halign, valign, style, shadow, max_chars, rect.getWidth(), right_x, use_ellipses);
}


S32 LLFontGL::render(const LLWString &wstr, S32 begin_offset, F32 x, F32 y, const LLColor4 &color, HAlign halign, VAlign valign, U8 style, 
					 ShadowType shadow, S32 max_chars, S32 max_pixels, F32* right_x, BOOL use_ellipses) const
{
	LLFastTimer _(FTM_RENDER_FONTS);

	if(!sDisplayFont) //do not display texts
	{
		return wstr.length() ;
	}

	if (wstr.empty())
	{
		return 0;
	} 

	gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);

	S32 scaled_max_pixels = max_pixels == S32_MAX ? S32_MAX : llceil((F32)max_pixels * sScaleX);

	// determine which style flags need to be added programmatically by stripping off the
	// style bits that are drawn by the underlying Freetype font
	U8 style_to_add = (style | mFontDescriptor.getStyle()) & ~mFontFreetype->getStyle();

	F32 drop_shadow_strength = 0.f;
	if (shadow != NO_SHADOW)
	{
		F32 luminance;
		color.calcHSL(NULL, NULL, &luminance);
		drop_shadow_strength = clamp_rescale(luminance, 0.35f, 0.6f, 0.f, 1.f);
		if (luminance < 0.35f)
		{
			shadow = NO_SHADOW;
		}
	}

	gGL.pushUIMatrix();

	gGL.loadUIIdentity();
	
	LLVector2 origin(floorf(sCurOrigin.mX*sScaleX), floorf(sCurOrigin.mY*sScaleY));

	// Depth translation, so that floating text appears 'in-world'
	// and is correctly occluded.
	gGL.translatef(0.f,0.f,sCurDepth);

	S32 chars_drawn = 0;
	S32 i;
	S32 length;

	if (-1 == max_chars)
	{
		length = (S32)wstr.length() - begin_offset;
	}
	else
	{
		length = llmin((S32)wstr.length() - begin_offset, max_chars );
	}

	F32 cur_x, cur_y, cur_render_x, cur_render_y;

 	// Not guaranteed to be set correctly
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	
	cur_x = ((F32)x * sScaleX) + origin.mV[VX];
	cur_y = ((F32)y * sScaleY) + origin.mV[VY];

	// Offset y by vertical alignment.
	// use unscaled font metrics here
	switch (valign)
	{
	case TOP:
		cur_y -= llceil(mFontFreetype->getAscenderHeight());
		break;
	case BOTTOM:
		cur_y += llceil(mFontFreetype->getDescenderHeight());
		break;
	case VCENTER:
		cur_y -= llceil((llceil(mFontFreetype->getAscenderHeight()) - llceil(mFontFreetype->getDescenderHeight())) / 2.f);
		break;
	case BASELINE:
		// Baseline, do nothing.
		break;
	default:
		break;
	}

	switch (halign)
	{
	case LEFT:
		break;
	case RIGHT:
	  	cur_x -= llmin(scaled_max_pixels, llround(getWidthF32(wstr.c_str(), begin_offset, length) * sScaleX));
		break;
	case HCENTER:
	    cur_x -= llmin(scaled_max_pixels, llround(getWidthF32(wstr.c_str(), begin_offset, length) * sScaleX)) / 2;
		break;
	default:
		break;
	}

	cur_render_y = cur_y;
	cur_render_x = cur_x;

	F32 start_x = (F32)llround(cur_x);

	const LLFontBitmapCache* font_bitmap_cache = mFontFreetype->getFontBitmapCache();

	F32 inv_width = 1.f / font_bitmap_cache->getBitmapWidth();
	F32 inv_height = 1.f / font_bitmap_cache->getBitmapHeight();

	const S32 LAST_CHARACTER = LLFontFreetype::LAST_CHAR_FULL;


	BOOL draw_ellipses = FALSE;
	if (use_ellipses)
	{
		// check for too long of a string
		S32 string_width = llround(getWidthF32(wstr.c_str(), begin_offset, max_chars) * sScaleX);
		if (string_width > scaled_max_pixels)
		{
			// use four dots for ellipsis width to generate padding
			const LLWString dots(utf8str_to_wstring(std::string("....")));
			scaled_max_pixels = llmax(0, scaled_max_pixels - llround(getWidthF32(dots.c_str())));
			draw_ellipses = TRUE;
		}
	}

	const LLFontGlyphInfo* next_glyph = NULL;

	const S32 GLYPH_BATCH_SIZE = 30;
	LLVector3 vertices[GLYPH_BATCH_SIZE * 4];
	LLVector2 uvs[GLYPH_BATCH_SIZE * 4];
	LLColor4U colors[GLYPH_BATCH_SIZE * 4];

	LLColor4U text_color(color);

	S32 bitmap_num = -1;
	S32 glyph_count = 0;
	for (i = begin_offset; i < begin_offset + length; i++)
	{
		llwchar wch = wstr[i];

		const LLFontGlyphInfo* fgi = next_glyph;
		next_glyph = NULL;
		if(!fgi)
		{
			fgi = mFontFreetype->getGlyphInfo(wch);
		}
		if (!fgi)
		{
			llerrs << "Missing Glyph Info" << llendl;
			break;
		}
		// Per-glyph bitmap texture.
		S32 next_bitmap_num = fgi->mBitmapNum;
		if (next_bitmap_num != bitmap_num)
		{
			// Actually draw the queued glyphs before switching their texture;
			// otherwise the queued glyphs will be taken from wrong textures.
			if (glyph_count > 0)
			{
				gGL.begin(LLRender::QUADS);
				{
					gGL.vertexBatchPreTransformed(vertices, uvs, colors, glyph_count * 4);
				}
				gGL.end();
				glyph_count = 0;
			}

			bitmap_num = next_bitmap_num;
			LLImageGL *font_image = font_bitmap_cache->getImageGL(bitmap_num);
			gGL.getTexUnit(0)->bind(font_image);
		}
	
		if ((start_x + scaled_max_pixels) < (cur_x + fgi->mXBearing + fgi->mWidth))
		{
			// Not enough room for this character.
			break;
		}

		// Draw the text at the appropriate location
		//Specify vertices and texture coordinates
		LLRectf uv_rect((fgi->mXBitmapOffset) * inv_width,
				(fgi->mYBitmapOffset + fgi->mHeight + PAD_UVY) * inv_height,
				(fgi->mXBitmapOffset + fgi->mWidth) * inv_width,
				(fgi->mYBitmapOffset - PAD_UVY) * inv_height);
		// snap glyph origin to whole screen pixel
		LLRectf screen_rect((F32)llround(cur_render_x + (F32)fgi->mXBearing),
				    (F32)llround(cur_render_y + (F32)fgi->mYBearing),
				    (F32)llround(cur_render_x + (F32)fgi->mXBearing) + (F32)fgi->mWidth,
				    (F32)llround(cur_render_y + (F32)fgi->mYBearing) - (F32)fgi->mHeight);
		
		if (glyph_count >= GLYPH_BATCH_SIZE)
		{
			gGL.begin(LLRender::QUADS);
			{
				gGL.vertexBatchPreTransformed(vertices, uvs, colors, glyph_count * 4);
			}
			gGL.end();

			glyph_count = 0;
		}

		drawGlyph(glyph_count, vertices, uvs, colors, screen_rect, uv_rect, text_color, style_to_add, shadow, drop_shadow_strength);

		chars_drawn++;
		cur_x += fgi->mXAdvance;
		cur_y += fgi->mYAdvance;

		llwchar next_char = wstr[i+1];
		if (next_char && (next_char < LAST_CHARACTER))
		{
			// Kern this puppy.
			next_glyph = mFontFreetype->getGlyphInfo(next_char);
			cur_x += mFontFreetype->getXKerning(fgi, next_glyph);
		}

		// Round after kerning.
		// Must do this to cur_x, not just to cur_render_x, otherwise you
		// will squish sub-pixel kerned characters too close together.
		// For example, "CCCCC" looks bad.
		cur_x = (F32)llround(cur_x);
		//cur_y = (F32)llround(cur_y);

		cur_render_x = cur_x;
		cur_render_y = cur_y;
	}

	gGL.begin(LLRender::QUADS);
	{
		gGL.vertexBatchPreTransformed(vertices, uvs, colors, glyph_count * 4);
	}
	gGL.end();


	if (right_x)
	{
		*right_x = (cur_x - origin.mV[VX]) / sScaleX;
	}

	//FIXME: add underline as glyph?
	if (style_to_add & UNDERLINE)
	{
		F32 descender = (F32)llfloor(mFontFreetype->getDescenderHeight());

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.begin(LLRender::LINES);
		gGL.vertex2f(start_x, cur_y - descender);
		gGL.vertex2f(cur_x, cur_y - descender);
		gGL.end();
	}

	if (draw_ellipses)
	{
		
		// recursively render ellipses at end of string
		// we've already reserved enough room
		gGL.pushUIMatrix();
		renderUTF8(std::string("..."), 
				0,
				(cur_x - origin.mV[VX]) / sScaleX, (F32)y,
				color,
				LEFT, valign,
				style_to_add,
				shadow,
				S32_MAX, max_pixels,
				right_x,
				FALSE); 
		gGL.popUIMatrix();
	}

	gGL.popUIMatrix();

	return chars_drawn;
}

S32 LLFontGL::render(const LLWString &text, S32 begin_offset, F32 x, F32 y, const LLColor4 &color) const
{
	return render(text, begin_offset, x, y, color, LEFT, BASELINE, NORMAL, NO_SHADOW, S32_MAX, S32_MAX, NULL, FALSE);
}

S32 LLFontGL::renderUTF8(const std::string &text, S32 begin_offset, F32 x, F32 y, const LLColor4 &color, HAlign halign,  VAlign valign, U8 style, ShadowType shadow, S32 max_chars, S32 max_pixels,  F32* right_x, BOOL use_ellipses) const
{
	return render(utf8str_to_wstring(text), begin_offset, x, y, color, halign, valign, style, shadow, max_chars, max_pixels, right_x, use_ellipses);
}

S32 LLFontGL::renderUTF8(const std::string &text, S32 begin_offset, S32 x, S32 y, const LLColor4 &color) const
{
	return renderUTF8(text, begin_offset, (F32)x, (F32)y, color, LEFT, BASELINE, NORMAL, NO_SHADOW, S32_MAX, S32_MAX, NULL, FALSE);
}

S32 LLFontGL::renderUTF8(const std::string &text, S32 begin_offset, S32 x, S32 y, const LLColor4 &color, HAlign halign, VAlign valign, U8 style, ShadowType shadow) const
{
	return renderUTF8(text, begin_offset, (F32)x, (F32)y, color, halign, valign, style, shadow, S32_MAX, S32_MAX, NULL, FALSE);
}

// font metrics - override for LLFontFreetype that returns units of virtual pixels
F32 LLFontGL::getAscenderHeight() const
{ 
	return mFontFreetype->getAscenderHeight() / sScaleY;
}

F32 LLFontGL::getDescenderHeight() const
{ 
	return mFontFreetype->getDescenderHeight() / sScaleY;
}

S32 LLFontGL::getLineHeight() const
{ 
	return llceil(mFontFreetype->getAscenderHeight() / sScaleY) + llceil(mFontFreetype->getDescenderHeight() / sScaleY);
}

S32 LLFontGL::getWidth(const std::string& utf8text) const
{
	LLWString wtext = utf8str_to_wstring(utf8text);
	return getWidth(wtext.c_str(), 0, S32_MAX);
}

S32 LLFontGL::getWidth(const llwchar* wchars) const
{
	return getWidth(wchars, 0, S32_MAX);
}

S32 LLFontGL::getWidth(const std::string& utf8text, S32 begin_offset, S32 max_chars) const
{
	LLWString wtext = utf8str_to_wstring(utf8text);
	return getWidth(wtext.c_str(), begin_offset, max_chars);
}

S32 LLFontGL::getWidth(const llwchar* wchars, S32 begin_offset, S32 max_chars) const
{
	F32 width = getWidthF32(wchars, begin_offset, max_chars);
	return llround(width);
}

F32 LLFontGL::getWidthF32(const std::string& utf8text) const
{
	LLWString wtext = utf8str_to_wstring(utf8text);
	return getWidthF32(wtext.c_str(), 0, S32_MAX);
}

F32 LLFontGL::getWidthF32(const llwchar* wchars) const
{
	return getWidthF32(wchars, 0, S32_MAX);
}

F32 LLFontGL::getWidthF32(const std::string& utf8text, S32 begin_offset, S32 max_chars ) const
{
	LLWString wtext = utf8str_to_wstring(utf8text);
	return getWidthF32(wtext.c_str(), begin_offset, max_chars);
}

F32 LLFontGL::getWidthF32(const llwchar* wchars, S32 begin_offset, S32 max_chars) const
{
	const S32 LAST_CHARACTER = LLFontFreetype::LAST_CHAR_FULL;

	F32 cur_x = 0;
	const S32 max_index = begin_offset + max_chars;

	const LLFontGlyphInfo* next_glyph = NULL;

	F32 width_padding = 0.f;
	for (S32 i = begin_offset; i < max_index && wchars[i] != 0; i++)
	{
		llwchar wch = wchars[i];

		const LLFontGlyphInfo* fgi = next_glyph;
		next_glyph = NULL;
		if(!fgi)
		{
			fgi = mFontFreetype->getGlyphInfo(wch);
		}

		F32 advance = mFontFreetype->getXAdvance(fgi);

		// for the last character we want to measure the greater of its width and xadvance values
		// so keep track of the difference between these values for the each character we measure
		// so we can fix things up at the end
		width_padding = llmax(	0.f,											// always use positive padding amount
								width_padding - advance,						// previous padding left over after advance of current character
								(F32)(fgi->mWidth + fgi->mXBearing) - advance);	// difference between width of this character and advance to next character

		cur_x += advance;
		llwchar next_char = wchars[i+1];

		if (((i + 1) < begin_offset + max_chars) 
			&& next_char 
			&& (next_char < LAST_CHARACTER))
		{
			// Kern this puppy.
			next_glyph = mFontFreetype->getGlyphInfo(next_char);
			cur_x += mFontFreetype->getXKerning(fgi, next_glyph);
		}
		// Round after kerning.
		cur_x = (F32)llround(cur_x);
	}

	// add in extra pixels for last character's width past its xadvance
	cur_x += width_padding;

	return cur_x / sScaleX;
}

// Returns the max number of complete characters from text (up to max_chars) that can be drawn in max_pixels
S32 LLFontGL::maxDrawableChars(const llwchar* wchars, F32 max_pixels, S32 max_chars, EWordWrapStyle end_on_word_boundary) const
{
	if (!wchars || !wchars[0] || max_chars == 0)
	{
		return 0;
	}
	
	llassert(max_pixels >= 0.f);
	llassert(max_chars >= 0);
	
	BOOL clip = FALSE;
	F32 cur_x = 0;

	S32 start_of_last_word = 0;
	BOOL in_word = FALSE;

	// avoid S32 overflow when max_pixels == S32_MAX by staying in floating point
	F32 scaled_max_pixels =	max_pixels * sScaleX;
	F32 width_padding = 0.f;
	
	LLFontGlyphInfo* next_glyph = NULL;

	S32 i;
	for (i=0; (i < max_chars); i++)
	{
		llwchar wch = wchars[i];

		if(wch == 0)
		{
			// Null terminator.  We're done.
			break;
		}
			
		if (in_word)
		{
			if (iswspace(wch))
			{
				if(wch !=(0x00A0))
				{
					in_word = FALSE;
				}
			}
			if (iswindividual(wch))
			{
				if (iswpunct(wchars[i+1]))
				{
					in_word=TRUE;
				}
				else
				{
					in_word=FALSE;
					start_of_last_word = i;
				}
			}
		}
		else
		{
			start_of_last_word = i;
			if (!iswspace(wch)||!iswindividual(wch))
			{
				in_word = TRUE;
			}
		}
		
		LLFontGlyphInfo* fgi = next_glyph;
		next_glyph = NULL;
		if(!fgi)
		{
			fgi = mFontFreetype->getGlyphInfo(wch);
		}

		// account for glyphs that run beyond the starting point for the next glyphs
		width_padding = llmax(	0.f,													// always use positive padding amount
								width_padding - fgi->mXAdvance,							// previous padding left over after advance of current character
								(F32)(fgi->mWidth + fgi->mXBearing) - fgi->mXAdvance);	// difference between width of this character and advance to next character

		cur_x += fgi->mXAdvance;
		
		// clip if current character runs past scaled_max_pixels (using width_padding)
		if (scaled_max_pixels < cur_x + width_padding)
		{
			clip = TRUE;
			break;
		}

		if (((i+1) < max_chars) && wchars[i+1])
		{
			// Kern this puppy.
			next_glyph = mFontFreetype->getGlyphInfo(wchars[i+1]);
			cur_x += mFontFreetype->getXKerning(fgi, next_glyph);
		}

		// Round after kerning.
		cur_x = (F32)llround(cur_x);
	}

	if( clip )
	{
		switch (end_on_word_boundary)
		{
		case ONLY_WORD_BOUNDARIES:
			i = start_of_last_word;
			break;
		case WORD_BOUNDARY_IF_POSSIBLE:
			if (start_of_last_word != 0)
			{
				i = start_of_last_word;
			}
			break;
		default:
		case ANYWHERE:
			// do nothing
			break;
		}
	}
	return i;
}

S32	LLFontGL::firstDrawableChar(const llwchar* wchars, F32 max_pixels, S32 text_len, S32 start_pos, S32 max_chars) const
{
	if (!wchars || !wchars[0] || max_chars == 0)
	{
		return 0;
	}
	
	F32 total_width = 0.0;
	S32 drawable_chars = 0;

	F32 scaled_max_pixels =	max_pixels * sScaleX;

	S32 start = llmin(start_pos, text_len - 1);
	for (S32 i = start; i >= 0; i--)
	{
		llwchar wch = wchars[i];

		const LLFontGlyphInfo* fgi= mFontFreetype->getGlyphInfo(wch);

		// last character uses character width, since the whole character needs to be visible
		// other characters just use advance
		F32 width = (i == start) 
			? (F32)(fgi->mWidth + fgi->mXBearing)  	// use actual width for last character
			: fgi->mXAdvance;						// use advance for all other characters										

		if( scaled_max_pixels < (total_width + width) )
		{
			break;
		}

		total_width += width;
		drawable_chars++;

		if( max_chars >= 0 && drawable_chars >= max_chars )
		{
			break;
		}

		if ( i > 0 )
		{
			// kerning
			total_width += mFontFreetype->getXKerning(wchars[i-1], wch);
		}

		// Round after kerning.
		total_width = (F32)llround(total_width);
	}

	if (drawable_chars == 0)
	{
		return start_pos; // just draw last character
	}
	else
	{
		// if only 1 character is drawable, we want to return start_pos as the first character to draw
		// if 2 are drawable, return start_pos and character before start_pos, etc.
		return start_pos + 1 - drawable_chars;
	}
	
}

S32 LLFontGL::charFromPixelOffset(const llwchar* wchars, S32 begin_offset, F32 target_x, F32 max_pixels, S32 max_chars, BOOL round) const
{
	if (!wchars || !wchars[0] || max_chars == 0)
	{
		return 0;
	}
	
	F32 cur_x = 0;

	target_x *= sScaleX;

	// max_chars is S32_MAX by default, so make sure we don't get overflow
	const S32 max_index = begin_offset + llmin(S32_MAX - begin_offset, max_chars - 1);

	F32 scaled_max_pixels =	max_pixels * sScaleX;
	
	const LLFontGlyphInfo* next_glyph = NULL;

	S32 pos;
	for (pos = begin_offset; pos < max_index; pos++)
	{
		llwchar wch = wchars[pos];
		if (!wch)
		{
			break; // done
		}
		
		const LLFontGlyphInfo* glyph = next_glyph;
		next_glyph = NULL;
		if(!glyph)
		{
			glyph = mFontFreetype->getGlyphInfo(wch);
		}
		
		F32 char_width = mFontFreetype->getXAdvance(glyph);

		if (round)
		{
			// Note: if the mouse is on the left half of the character, the pick is to the character's left
			// If it's on the right half, the pick is to the right.
			if (target_x  < cur_x + char_width*0.5f)
			{
				break;
			}
		}
		else if (target_x  < cur_x + char_width)
		{
			break;
		}

		if (scaled_max_pixels < cur_x + char_width)
		{
			break;
		}

		cur_x += char_width;

		if (((pos + 1) < max_index)
			&& (wchars[(pos + 1)]))
		{
			// Kern this puppy.
			next_glyph = mFontFreetype->getGlyphInfo(wchars[pos + 1]);
			cur_x += mFontFreetype->getXKerning(glyph, next_glyph);
		}


		// Round after kerning.
		cur_x = (F32)llround(cur_x);
	}

	return llmin(max_chars, pos - begin_offset);
}

const LLFontDescriptor& LLFontGL::getFontDesc() const
{
	return mFontDescriptor;
}

// static
void LLFontGL::initClass(F32 screen_dpi, F32 x_scale, F32 y_scale, const std::string& app_dir, bool create_gl_textures)
{
	sVertDPI = (F32)llfloor(screen_dpi * y_scale);
	sHorizDPI = (F32)llfloor(screen_dpi * x_scale);
	sScaleX = x_scale;
	sScaleY = y_scale;
	sAppDir = app_dir;

	// Font registry init
	if (!sFontRegistry)
	{
		sFontRegistry = new LLFontRegistry(create_gl_textures);
		sFontRegistry->parseFontInfo("fonts.xml");
	}
	else
	{
		sFontRegistry->reset();
	}
}

// Force standard fonts to get generated up front.
// This is primarily for error detection purposes.
// Don't do this during initClass because it can be slow and we want to get
// the viewer window on screen first. JC
// static
bool LLFontGL::loadDefaultFonts()
{
	bool succ = true;
	succ &= (NULL != getFontSansSerifSmall());
	succ &= (NULL != getFontSansSerif());
	succ &= (NULL != getFontSansSerifBig());
	succ &= (NULL != getFontSansSerifHuge());
	succ &= (NULL != getFontSansSerifBold());
	succ &= (NULL != getFontMonospace());
	succ &= (NULL != getFontExtChar());
	return succ;
}

// static
void LLFontGL::destroyDefaultFonts()
{
	// Remove the actual fonts.
	delete sFontRegistry;
	sFontRegistry = NULL;
}

//static 
void LLFontGL::destroyAllGL()
{
	if (sFontRegistry)
	{
		sFontRegistry->destroyGL();
	}
}

// static
U8 LLFontGL::getStyleFromString(const std::string &style)
{
	S32 ret = 0;
	if (style.find("NORMAL") != style.npos)
	{
		ret |= NORMAL;
	}
	if (style.find("BOLD") != style.npos)
	{
		ret |= BOLD;
	}
	if (style.find("ITALIC") != style.npos)
	{
		ret |= ITALIC;
	}
	if (style.find("UNDERLINE") != style.npos)
	{
		ret |= UNDERLINE;
	}
	return ret;
}

// static
std::string LLFontGL::getStringFromStyle(U8 style)
{
	std::string style_string;
	if (style & NORMAL)
	{
		style_string += "|NORMAL";
	}
	if (style & BOLD)
	{
		style_string += "|BOLD";
	}
	if (style & ITALIC)
	{
		style_string += "|ITALIC";
	}
	if (style & UNDERLINE)
	{
		style_string += "|UNDERLINE";
	}
	return style_string;
}

// static
std::string LLFontGL::nameFromFont(const LLFontGL* fontp)
{
	return fontp->mFontDescriptor.getName();
}


// static
std::string LLFontGL::sizeFromFont(const LLFontGL* fontp)
{
	return fontp->mFontDescriptor.getSize();
}

// static
std::string LLFontGL::nameFromHAlign(LLFontGL::HAlign align)
{
	if (align == LEFT)			return std::string("left");
	else if (align == RIGHT)	return std::string("right");
	else if (align == HCENTER)	return std::string("center");
	else return std::string();
}

// static
LLFontGL::HAlign LLFontGL::hAlignFromName(const std::string& name)
{
	LLFontGL::HAlign gl_hfont_align = LLFontGL::LEFT;
	if (name == "left")
	{
		gl_hfont_align = LLFontGL::LEFT;
	}
	else if (name == "right")
	{
		gl_hfont_align = LLFontGL::RIGHT;
	}
	else if (name == "center")
	{
		gl_hfont_align = LLFontGL::HCENTER;
	}
	//else leave left
	return gl_hfont_align;
}

// static
std::string LLFontGL::nameFromVAlign(LLFontGL::VAlign align)
{
	if (align == TOP)			return std::string("top");
	else if (align == VCENTER)	return std::string("center");
	else if (align == BASELINE)	return std::string("baseline");
	else if (align == BOTTOM)	return std::string("bottom");
	else return std::string();
}

// static
LLFontGL::VAlign LLFontGL::vAlignFromName(const std::string& name)
{
	LLFontGL::VAlign gl_vfont_align = LLFontGL::BASELINE;
	if (name == "top")
	{
		gl_vfont_align = LLFontGL::TOP;
	}
	else if (name == "center")
	{
		gl_vfont_align = LLFontGL::VCENTER;
	}
	else if (name == "baseline")
	{
		gl_vfont_align = LLFontGL::BASELINE;
	}
	else if (name == "bottom")
	{
		gl_vfont_align = LLFontGL::BOTTOM;
	}
	//else leave baseline
	return gl_vfont_align;
}

//static
LLFontGL* LLFontGL::getFontMonospace()
{
	static LLFontGL* fontp = getFont(LLFontDescriptor("Monospace","Monospace",0));
	return fontp;
}

//static
LLFontGL* LLFontGL::getFontSansSerifSmall()
{
	static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif","Small",0));
	return fontp;
}

//static
LLFontGL* LLFontGL::getFontSansSerif()
{
	static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif","Medium",0));
	return fontp;
}

//static
LLFontGL* LLFontGL::getFontSansSerifBig()
{
	static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif","Large",0));
	return fontp;
}

//static 
LLFontGL* LLFontGL::getFontSansSerifHuge()
{
	static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif","Large",0));
	return fontp;
}

//static 
LLFontGL* LLFontGL::getFontSansSerifBold()
{
	static LLFontGL* fontp = getFont(LLFontDescriptor("SansSerif","Medium",BOLD));
	return fontp;
}

//static
LLFontGL* LLFontGL::getFontExtChar()
{
	return getFontSansSerif();
}

//static 
LLFontGL* LLFontGL::getFont(const LLFontDescriptor& desc)
{
	return sFontRegistry->getFont(desc);
}

//static
LLFontGL* LLFontGL::getFontByName(const std::string& name)
{
	// check for most common fonts first
	if (name == "SANSSERIF")
	{
		return getFontSansSerif();
	}
	else if (name == "SANSSERIF_SMALL")
	{
		return getFontSansSerifSmall();
	}
	else if (name == "SANSSERIF_BIG")
	{
		return getFontSansSerifBig();
	}
	else if (name == "SMALL" || name == "OCRA")
	{
		// *BUG: Should this be "MONOSPACE"?  Do we use "OCRA" anymore?
		// Does "SMALL" mean "SERIF"?
		return getFontMonospace();
	}
	else
	{
		return NULL;
	}
}

//static
LLFontGL* LLFontGL::getFontDefault()
{
	return getFontSansSerif(); // Fallback to sans serif as default font
}


// static 
std::string LLFontGL::getFontPathSystem()
{
	std::string system_path;

	// Try to figure out where the system's font files are stored.
	char *system_root = NULL;
#if LL_WINDOWS
	system_root = getenv("SystemRoot");	/* Flawfinder: ignore */
	if (!system_root)
	{
		llwarns << "SystemRoot not found, attempting to load fonts from default path." << llendl;
	}
#endif

	if (system_root)
	{
		system_path = llformat("%s/fonts/", system_root);
	}
	else
	{
#if LL_WINDOWS
		// HACK for windows 98/Me
		system_path = "/WINDOWS/FONTS/";
#elif LL_DARWIN
		// HACK for Mac OS X
		system_path = "/System/Library/Fonts/";
#endif
	}
	return system_path;
}


// static 
std::string LLFontGL::getFontPathLocal()
{
	std::string local_path;

	// Backup files if we can't load from system fonts directory.
	// We could store this in an end-user writable directory to allow
	// end users to switch fonts.
	if (LLFontGL::sAppDir.length())
	{
		// use specified application dir to look for fonts
		local_path = LLFontGL::sAppDir + "/fonts/";
	}
	else
	{
		// assume working directory is executable directory
		local_path = "./fonts/";
	}
	return local_path;
}

LLFontGL::LLFontGL(const LLFontGL &source)
{
	llerrs << "Not implemented!" << llendl;
}

LLFontGL &LLFontGL::operator=(const LLFontGL &source)
{
	llerrs << "Not implemented" << llendl;
	return *this;
}

void LLFontGL::renderQuad(LLVector3* vertex_out, LLVector2* uv_out, LLColor4U* colors_out, const LLRectf& screen_rect, const LLRectf& uv_rect, const LLColor4U& color, F32 slant_amt) const
{
	S32 index = 0;

	vertex_out[index] = LLVector3(screen_rect.mRight, screen_rect.mTop, 0.f);
	uv_out[index] = LLVector2(uv_rect.mRight, uv_rect.mTop);
	colors_out[index] = color;
	index++;

	vertex_out[index] = LLVector3(screen_rect.mLeft, screen_rect.mTop, 0.f);
	uv_out[index] = LLVector2(uv_rect.mLeft, uv_rect.mTop);
	colors_out[index] = color;
	index++;

	vertex_out[index] = LLVector3(screen_rect.mLeft, screen_rect.mBottom, 0.f);
	uv_out[index] = LLVector2(uv_rect.mLeft, uv_rect.mBottom);
	colors_out[index] = color;
	index++;

	vertex_out[index] = LLVector3(screen_rect.mRight, screen_rect.mBottom, 0.f);
	uv_out[index] = LLVector2(uv_rect.mRight, uv_rect.mBottom);
	colors_out[index] = color;
}

void LLFontGL::drawGlyph(S32& glyph_count, LLVector3* vertex_out, LLVector2* uv_out, LLColor4U* colors_out, const LLRectf& screen_rect, const LLRectf& uv_rect, const LLColor4U& color, U8 style, ShadowType shadow, F32 drop_shadow_strength) const
{
	F32 slant_offset;
	slant_offset = ((style & ITALIC) ? ( -mFontFreetype->getAscenderHeight() * 0.2f) : 0.f);

	//FIXME: bold and drop shadow are mutually exclusive only for convenience
	//Allow both when we need them.
	if (style & BOLD)
	{
		for (S32 pass = 0; pass < 2; pass++)
		{
			LLRectf screen_rect_offset = screen_rect;

			screen_rect_offset.translate((F32)(pass * BOLD_OFFSET), 0.f);
			renderQuad(&vertex_out[glyph_count * 4], &uv_out[glyph_count * 4], &colors_out[glyph_count * 4], screen_rect_offset, uv_rect, color, slant_offset);
			glyph_count++;
		}
	}
	else if (shadow == DROP_SHADOW_SOFT)
	{
		LLColor4U shadow_color = LLFontGL::sShadowColor;
		shadow_color.mV[VALPHA] = U8(color.mV[VALPHA] * drop_shadow_strength * DROP_SHADOW_SOFT_STRENGTH);
		for (S32 pass = 0; pass < 5; pass++)
		{
			LLRectf screen_rect_offset = screen_rect;

			switch(pass)
			{
			case 0:
				screen_rect_offset.translate(-1.f, -1.f);
				break;
			case 1:
				screen_rect_offset.translate(1.f, -1.f);
				break;
			case 2:
				screen_rect_offset.translate(1.f, 1.f);
				break;
			case 3:
				screen_rect_offset.translate(-1.f, 1.f);
				break;
			case 4:
				screen_rect_offset.translate(0, -2.f);
				break;
			}
		
			renderQuad(&vertex_out[glyph_count * 4], &uv_out[glyph_count * 4], &colors_out[glyph_count * 4], screen_rect_offset, uv_rect, shadow_color, slant_offset);
			glyph_count++;
		}
		renderQuad(&vertex_out[glyph_count * 4], &uv_out[glyph_count * 4], &colors_out[glyph_count * 4], screen_rect, uv_rect, color, slant_offset);
		glyph_count++;
	}
	else if (shadow == DROP_SHADOW)
	{
		LLColor4U shadow_color = LLFontGL::sShadowColor;
		shadow_color.mV[VALPHA] = U8(color.mV[VALPHA] * drop_shadow_strength);
		LLRectf screen_rect_shadow = screen_rect;
		screen_rect_shadow.translate(1.f, -1.f);
		renderQuad(&vertex_out[glyph_count * 4], &uv_out[glyph_count * 4], &colors_out[glyph_count * 4], screen_rect_shadow, uv_rect, shadow_color, slant_offset);
		glyph_count++;
		renderQuad(&vertex_out[glyph_count * 4], &uv_out[glyph_count * 4], &colors_out[glyph_count * 4], screen_rect, uv_rect, color, slant_offset);
		glyph_count++;
	}
	else // normal rendering
	{
		renderQuad(&vertex_out[glyph_count * 4], &uv_out[glyph_count * 4], &colors_out[glyph_count * 4], screen_rect, uv_rect, color, slant_offset);
		glyph_count++;
	}
}
