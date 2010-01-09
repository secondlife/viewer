/** 
 * @file llfontgl.cpp
 * @brief Wrapper around FreeType
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

LLCoordFont LLFontGL::sCurOrigin;
std::vector<LLCoordFont> LLFontGL::sOriginStack;

const F32 EXT_X_BEARING = 1.f;
const F32 EXT_Y_BEARING = 0.f;
const F32 EXT_KERNING = 1.f;
const F32 PIXEL_BORDER_THRESHOLD = 0.0001f;
const F32 PIXEL_CORRECTION_DISTANCE = 0.01f;

const F32 PAD_UVY = 0.5f; // half of vertical padding between glyphs in the glyph texture
const F32 DROP_SHADOW_SOFT_STRENGTH = 0.3f;

static F32 llfont_round_x(F32 x)
{
	//return llfloor((x-LLFontGL::sCurOrigin.mX)/LLFontGL::sScaleX+0.5f)*LLFontGL::sScaleX+LLFontGL::sCurOrigin.mX;
	//return llfloor(x/LLFontGL::sScaleX+0.5f)*LLFontGL::sScaleY;
	return x;
}

static F32 llfont_round_y(F32 y)
{
	//return llfloor((y-LLFontGL::sCurOrigin.mY)/LLFontGL::sScaleY+0.5f)*LLFontGL::sScaleY+LLFontGL::sCurOrigin.mY;
	//return llfloor(y+0.5f);
	return y;
}

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

static LLFastTimerUtil::DeclareTimer FTM_RENDER_FONTS("Fonts");

S32 LLFontGL::render(const LLWString &wstr, S32 begin_offset, F32 x, F32 y, const LLColor4 &color, HAlign halign, VAlign valign, U8 style, 
					 ShadowType shadow, S32 max_chars, S32 max_pixels, F32* right_x, BOOL use_ellipses) const
{
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

	gGL.pushMatrix();
	glLoadIdentity();
	gGL.translatef(floorf(sCurOrigin.mX*sScaleX), floorf(sCurOrigin.mY*sScaleY), sCurOrigin.mZ);

	// this code snaps the text origin to a pixel grid to start with
	F32 pixel_offset_x = llround((F32)sCurOrigin.mX) - (sCurOrigin.mX);
	F32 pixel_offset_y = llround((F32)sCurOrigin.mY) - (sCurOrigin.mY);
	gGL.translatef(-pixel_offset_x, -pixel_offset_y, 0.f);

	LLFastTimer t(FTM_RENDER_FONTS);

	gGL.color4fv( color.mV );

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
	
	cur_x = ((F32)x * sScaleX);
	cur_y = ((F32)y * sScaleY);

	// Offset y by vertical alignment.
	switch (valign)
	{
	case TOP:
		cur_y -= mFontFreetype->getAscenderHeight();
		break;
	case BOTTOM:
		cur_y += mFontFreetype->getDescenderHeight();
		break;
	case VCENTER:
		cur_y -= (mFontFreetype->getAscenderHeight() - mFontFreetype->getDescenderHeight()) / 2.f;
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

	F32 start_x = llround(cur_x);

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


	// Remember last-used texture to avoid unnecesssary bind calls.
	LLImageGL *last_bound_texture = NULL;

	for (i = begin_offset; i < begin_offset + length; i++)
	{
		llwchar wch = wstr[i];

		const LLFontGlyphInfo* fgi= mFontFreetype->getGlyphInfo(wch);
		if (!fgi)
		{
			llerrs << "Missing Glyph Info" << llendl;
			break;
		}
		// Per-glyph bitmap texture.
		LLImageGL *image_gl = mFontFreetype->getFontBitmapCache()->getImageGL(fgi->mBitmapNum);
		if (last_bound_texture != image_gl)
		{
			gGL.getTexUnit(0)->bind(image_gl);
			last_bound_texture = image_gl;
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
		LLRectf screen_rect(llround(cur_render_x + (F32)fgi->mXBearing),
				    llround(cur_render_y + (F32)fgi->mYBearing),
				    llround(cur_render_x + (F32)fgi->mXBearing) + (F32)fgi->mWidth,
				    llround(cur_render_y + (F32)fgi->mYBearing) - (F32)fgi->mHeight);
		
		drawGlyph(screen_rect, uv_rect, color, style_to_add, shadow, drop_shadow_strength);

		chars_drawn++;
		cur_x += fgi->mXAdvance;
		cur_y += fgi->mYAdvance;

		llwchar next_char = wstr[i+1];
		if (next_char && (next_char < LAST_CHARACTER))
		{
			// Kern this puppy.
			cur_x += mFontFreetype->getXKerning(wch, next_char);
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

	if (right_x)
	{
		*right_x = cur_x / sScaleX;
	}

	if (style_to_add & UNDERLINE)
	{
		F32 descender = mFontFreetype->getDescenderHeight();

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.begin(LLRender::LINES);
		gGL.vertex2f(start_x, cur_y - (descender));
		gGL.vertex2f(cur_x, cur_y - (descender));
		gGL.end();
	}

	if (draw_ellipses)
	{
		
		// recursively render ellipses at end of string
		// we've already reserved enough room
		gGL.pushMatrix();
		//glLoadIdentity();
		//gGL.translatef(sCurOrigin.mX, sCurOrigin.mY, 0.0f);
		//glScalef(sScaleX, sScaleY, 1.f);
		renderUTF8(std::string("..."), 
				0,
				cur_x / sScaleX, (F32)y,
				color,
				LEFT, valign,
				style_to_add,
				shadow,
				S32_MAX, max_pixels,
				right_x,
				FALSE); 
		gGL.popMatrix();
	}

	gGL.popMatrix();

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
F32 LLFontGL::getLineHeight() const
{ 
	return (F32)llround(mFontFreetype->getLineHeight() / sScaleY); 
}

F32 LLFontGL::getAscenderHeight() const
{ 
	return (F32)llround(mFontFreetype->getAscenderHeight() / sScaleY); 
}

F32 LLFontGL::getDescenderHeight() const
{ 
	return (F32)llround(mFontFreetype->getDescenderHeight() / sScaleY); 
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

	F32 width_padding = 0.f;
	for (S32 i = begin_offset; i < max_index && wchars[i] != 0; i++)
	{
		llwchar wch = wchars[i];

		const LLFontGlyphInfo* fgi= mFontFreetype->getGlyphInfo(wch);

		F32 advance = mFontFreetype->getXAdvance(wch);

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
			cur_x += mFontFreetype->getXKerning(wch, next_char);
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
	F32 drawn_x = 0;

	S32 start_of_last_word = 0;
	BOOL in_word = FALSE;

	// avoid S32 overflow when max_pixels == S32_MAX by staying in floating point
	F32 scaled_max_pixels =	ceil(max_pixels * sScaleX);
	F32 width_padding = 0.f;

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

		LLFontGlyphInfo* fgi = mFontFreetype->getGlyphInfo(wch);

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
			cur_x += mFontFreetype->getXKerning(wch, wchars[i+1]);
		}

		// Round after kerning.
		cur_x = llround(cur_x);
		drawn_x = cur_x;
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

		F32 char_width = mFontFreetype->getXAdvance(wch);

		if( scaled_max_pixels < (total_width + char_width) )
		{
			break;
		}

		total_width += char_width;
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
		total_width = llround(total_width);
	}

	return start_pos - drawable_chars;
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
	const S32 max_index = begin_offset + llmin(S32_MAX - begin_offset, max_chars);

	F32 scaled_max_pixels =	max_pixels * sScaleX;

	S32 pos;
	for (pos = begin_offset; pos < max_index; pos++)
	{
		llwchar wch = wchars[pos];
		if (!wch)
		{
			break; // done
		}
		F32 char_width = mFontFreetype->getXAdvance(wch);

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
			llwchar next_char = wchars[pos + 1];
			// Kern this puppy.
			cur_x += mFontFreetype->getXKerning(wch, next_char);
		}

		// Round after kerning.
		cur_x = llround(cur_x);
	}

	return llmin(max_chars, pos - begin_offset);
}

const LLFontDescriptor& LLFontGL::getFontDesc() const
{
	return mFontDescriptor;
}

// static
void LLFontGL::initClass(F32 screen_dpi, F32 x_scale, F32 y_scale, const std::string& app_dir, const std::vector<std::string>& xui_paths, bool create_gl_textures)
{
	sVertDPI = (F32)llfloor(screen_dpi * y_scale);
	sHorizDPI = (F32)llfloor(screen_dpi * x_scale);
	sScaleX = x_scale;
	sScaleY = y_scale;
	sAppDir = app_dir;

	// Font registry init
	if (!sFontRegistry)
	{
		sFontRegistry = new LLFontRegistry(xui_paths, create_gl_textures);
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
	return getFont(LLFontDescriptor("Monospace","Monospace",0));
}

//static
LLFontGL* LLFontGL::getFontSansSerifSmall()
{
	return getFont(LLFontDescriptor("SansSerif","Small",0));
}

//static
LLFontGL* LLFontGL::getFontSansSerif()
{
	return getFont(LLFontDescriptor("SansSerif","Medium",0));
}

//static
LLFontGL* LLFontGL::getFontSansSerifBig()
{
	return getFont(LLFontDescriptor("SansSerif","Large",0));
}

//static 
LLFontGL* LLFontGL::getFontSansSerifHuge()
{
	return getFont(LLFontDescriptor("SansSerif","Huge",0));
}

//static 
LLFontGL* LLFontGL::getFontSansSerifBold()
{
	return getFont(LLFontDescriptor("SansSerif","Medium",BOLD));
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

void LLFontGL::renderQuad(const LLRectf& screen_rect, const LLRectf& uv_rect, F32 slant_amt) const
{
	gGL.texCoord2f(uv_rect.mRight, uv_rect.mTop);
	gGL.vertex2f(llfont_round_x(screen_rect.mRight), 
				llfont_round_y(screen_rect.mTop));

	gGL.texCoord2f(uv_rect.mLeft, uv_rect.mTop);
	gGL.vertex2f(llfont_round_x(screen_rect.mLeft), 
				llfont_round_y(screen_rect.mTop));

	gGL.texCoord2f(uv_rect.mLeft, uv_rect.mBottom);
	gGL.vertex2f(llfont_round_x(screen_rect.mLeft + slant_amt), 
				llfont_round_y(screen_rect.mBottom));

	gGL.texCoord2f(uv_rect.mRight, uv_rect.mBottom);
	gGL.vertex2f(llfont_round_x(screen_rect.mRight + slant_amt), 
				llfont_round_y(screen_rect.mBottom));
}

void LLFontGL::drawGlyph(const LLRectf& screen_rect, const LLRectf& uv_rect, const LLColor4& color, U8 style, ShadowType shadow, F32 drop_shadow_strength) const
{
	F32 slant_offset;
	slant_offset = ((style & ITALIC) ? ( -mFontFreetype->getAscenderHeight() * 0.2f) : 0.f);

	gGL.begin(LLRender::QUADS);
	{
		//FIXME: bold and drop shadow are mutually exclusive only for convenience
		//Allow both when we need them.
		if (style & BOLD)
		{
			gGL.color4fv(color.mV);
			for (S32 pass = 0; pass < 2; pass++)
			{
				LLRectf screen_rect_offset = screen_rect;

				screen_rect_offset.translate((F32)(pass * BOLD_OFFSET), 0.f);
				renderQuad(screen_rect_offset, uv_rect, slant_offset);
			}
		}
		else if (shadow == DROP_SHADOW_SOFT)
		{
			LLColor4 shadow_color = LLFontGL::sShadowColor;
			shadow_color.mV[VALPHA] = color.mV[VALPHA] * drop_shadow_strength * DROP_SHADOW_SOFT_STRENGTH;
			gGL.color4fv(shadow_color.mV);
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
			
				renderQuad(screen_rect_offset, uv_rect, slant_offset);
			}
			gGL.color4fv(color.mV);
			renderQuad(screen_rect, uv_rect, slant_offset);
		}
		else if (shadow == DROP_SHADOW)
		{
			LLColor4 shadow_color = LLFontGL::sShadowColor;
			shadow_color.mV[VALPHA] = color.mV[VALPHA] * drop_shadow_strength;
			gGL.color4fv(shadow_color.mV);
			LLRectf screen_rect_shadow = screen_rect;
			screen_rect_shadow.translate(1.f, -1.f);
			renderQuad(screen_rect_shadow, uv_rect, slant_offset);
			gGL.color4fv(color.mV);
			renderQuad(screen_rect, uv_rect, slant_offset);
		}
		else // normal rendering
		{
			gGL.color4fv(color.mV);
			renderQuad(screen_rect, uv_rect, slant_offset);
		}

	}
	gGL.end();
}
