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

#include <boost/tokenizer.hpp>

#include "llfont.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llrender.h"
#include "v4color.h"
#include "llstl.h"

const S32 BOLD_OFFSET = 1;

// static class members
F32 LLFontGL::sVertDPI = 96.f;
F32 LLFontGL::sHorizDPI = 96.f;
F32 LLFontGL::sScaleX = 1.f;
F32 LLFontGL::sScaleY = 1.f;
BOOL LLFontGL::sDisplayFont = TRUE ;
std::string LLFontGL::sAppDir;

LLFontGL* LLFontGL::sMonospace = NULL;
LLFontGL* LLFontGL::sSansSerifSmall = NULL;
LLFontGL* LLFontGL::sSansSerif = NULL;
LLFontGL* LLFontGL::sSansSerifBig = NULL;
LLFontGL* LLFontGL::sSansSerifHuge = NULL;
LLFontGL* LLFontGL::sSansSerifBold = NULL;
LLFontList*	LLFontGL::sMonospaceFallback = NULL;
LLFontList*	LLFontGL::sSSFallback = NULL;
LLFontList*	LLFontGL::sSSSmallFallback = NULL;
LLFontList*	LLFontGL::sSSBigFallback = NULL;
LLFontList*	LLFontGL::sSSHugeFallback = NULL;
LLFontList*	LLFontGL::sSSBoldFallback = NULL;
LLColor4 LLFontGL::sShadowColor(0.f, 0.f, 0.f, 1.f);

LLCoordFont LLFontGL::sCurOrigin;
std::vector<LLCoordFont> LLFontGL::sOriginStack;

LLFontGL*& gExtCharFont = LLFontGL::sSansSerif;

const F32 EXT_X_BEARING = 1.f;
const F32 EXT_Y_BEARING = 0.f;
const F32 EXT_KERNING = 1.f;
const F32 PIXEL_BORDER_THRESHOLD = 0.0001f;
const F32 PIXEL_CORRECTION_DISTANCE = 0.01f;

const F32 PAD_UVY = 0.5f; // half of vertical padding between glyphs in the glyph texture
const F32 DROP_SHADOW_SOFT_STRENGTH = 0.3f;

F32 llfont_round_x(F32 x)
{
	//return llfloor((x-LLFontGL::sCurOrigin.mX)/LLFontGL::sScaleX+0.5f)*LLFontGL::sScaleX+LLFontGL::sCurOrigin.mX;
	//return llfloor(x/LLFontGL::sScaleX+0.5f)*LLFontGL::sScaleY;
	return x;
}

F32 llfont_round_y(F32 y)
{
	//return llfloor((y-LLFontGL::sCurOrigin.mY)/LLFontGL::sScaleY+0.5f)*LLFontGL::sScaleY+LLFontGL::sCurOrigin.mY;
	//return llfloor(y+0.5f);
	return y;
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
	if (style.find("SHADOW") != style.npos)
	{
		ret |= DROP_SHADOW;
	}
	if (style.find("SOFT_SHADOW") != style.npos)
	{
		ret |= DROP_SHADOW_SOFT;
	}
	return ret;
}

LLFontGL::LLFontGL()
	: LLFont()
{
	init();
	clearEmbeddedChars();
}

LLFontGL::LLFontGL(const LLFontGL &source)
{
	llerrs << "Not implemented!" << llendl;
}

LLFontGL::~LLFontGL()
{
	mImageGLp = NULL;
	mRawImageGLp = NULL;
	clearEmbeddedChars();
}

void LLFontGL::init()
{
	if (mImageGLp.isNull())
	{
		mImageGLp = new LLImageGL(FALSE);
		//RN: use nearest mipmap filtering to obviate the need to do pixel-accurate positioning
		gGL.getTexUnit(0)->bind(mImageGLp);
		// we allow bilinear filtering to get sub-pixel positioning for drop shadows
		//mImageGLp->setMipFilterNearest(TRUE, TRUE);
	}
	if (mRawImageGLp.isNull())
	{
		mRawImageGLp = new LLImageRaw; // Note LLFontGL owns the image, not LLFont.
	}
	setRawImage( mRawImageGLp );  
}

void LLFontGL::reset()
{
	init();
	resetBitmap();
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

//static
bool LLFontGL::loadFaceFallback(LLFontList *fontlistp, const std::string& fontname, const F32 point_size)
{
	std::string local_path = getFontPathLocal();
	std::string sys_path = getFontPathSystem();
	
	// The fontname string may contain multiple font file names separated by semicolons.
	// Break it apart and try loading each one, in order.
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(";");
	tokenizer tokens(fontname, sep);
	tokenizer::iterator token_iter;

	for(token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		LLFont *fontp = new LLFont();
		std::string font_path = local_path + *token_iter;
		if (!fontp->loadFace(font_path, point_size, sVertDPI, sHorizDPI, 2, TRUE))
		{
			font_path = sys_path + *token_iter;
			if (!fontp->loadFace(font_path, point_size, sVertDPI, sHorizDPI, 2, TRUE))
			{
				LL_INFOS_ONCE("ViewerImages") << "Couldn't load font " << *token_iter << LL_ENDL;
				delete fontp;
				fontp = NULL;
			}
		}
		
		if(fontp)
		{
			fontlistp->addAtEnd(fontp);
		}
	}
	
	// We want to return true if at least one fallback font loaded correctly.
	return (fontlistp->size() > 0);
}

//static
bool LLFontGL::loadFace(LLFontGL *fontp, const std::string& fontname, const F32 point_size, LLFontList *fallback_fontp)
{
	std::string local_path = getFontPathLocal();
	std::string font_path = local_path + fontname;
	if (!fontp->loadFace(font_path, point_size, sVertDPI, sHorizDPI, 2, FALSE))
	{
		std::string sys_path = getFontPathSystem();
		font_path = sys_path + fontname;
		if (!fontp->loadFace(font_path, point_size, sVertDPI, sHorizDPI, 2, FALSE))
		{
			LL_WARNS("ViewerImages") << "Couldn't load font " << fontname << LL_ENDL;
			return false;
		}
	}

	fontp->setFallbackFont(fallback_fontp);
	return true;
}


// static
BOOL LLFontGL::initDefaultFonts(F32 screen_dpi, F32 x_scale, F32 y_scale,
				const std::string& monospace_file, F32 monospace_size,
				const std::string& sansserif_file,
				const std::string& sanserif_fallback_file, F32 ss_fallback_scale,
				F32 small_size, F32 medium_size, F32 big_size, F32 huge_size,
				const std::string& sansserif_bold_file, F32 bold_size,
				const std::string& app_dir)
{
	BOOL failed = FALSE;
	sVertDPI = (F32)llfloor(screen_dpi * y_scale);
	sHorizDPI = (F32)llfloor(screen_dpi * x_scale);
	sScaleX = x_scale;
	sScaleY = y_scale;
	sAppDir = app_dir;

	//
	// Monospace font
	//

	if (!sMonospace)
	{
		sMonospace = new LLFontGL();
	}
	else
	{
		sMonospace->reset();
	}

	if (sMonospaceFallback)
	{
		delete sMonospaceFallback;
	}
	sMonospaceFallback = new LLFontList();
	if (!loadFaceFallback(
			sMonospaceFallback,
			sanserif_fallback_file,
			monospace_size * ss_fallback_scale))
	{
		delete sMonospaceFallback;
		sMonospaceFallback = NULL;
	}

	failed |= !loadFace(sMonospace, monospace_file, monospace_size, sMonospaceFallback);

	//
	// Sans-serif fonts
	//
	if(!sSansSerifHuge)
	{
		sSansSerifHuge = new LLFontGL();
	}
	else
	{
		sSansSerifHuge->reset();
	}

	if (sSSHugeFallback)
	{
		delete sSSHugeFallback;
	}
	sSSHugeFallback = new LLFontList();
	if (!loadFaceFallback(
			sSSHugeFallback,
			sanserif_fallback_file,
			huge_size*ss_fallback_scale))
	{
		delete sSSHugeFallback;
		sSSHugeFallback = NULL;
	}

	failed |= !loadFace(sSansSerifHuge, sansserif_file, huge_size, sSSHugeFallback);


	if(!sSansSerifBig)
	{
		sSansSerifBig = new LLFontGL();
	}
	else
	{
		sSansSerifBig->reset();
	}

	if (sSSBigFallback)
	{
		delete sSSBigFallback;
	}
	sSSBigFallback = new LLFontList();
	if (!loadFaceFallback(
			sSSBigFallback,
			sanserif_fallback_file,
			big_size*ss_fallback_scale))
	{
		delete sSSBigFallback;
		sSSBigFallback = NULL;
	}

	failed |= !loadFace(sSansSerifBig, sansserif_file, big_size, sSSBigFallback);


	if(!sSansSerif)
	{
		sSansSerif = new LLFontGL();
	}
	else
	{
		sSansSerif->reset();
	}

	if (sSSFallback)
	{
		delete sSSFallback;
	}
	sSSFallback = new LLFontList();
	if (!loadFaceFallback(
			sSSFallback,
			sanserif_fallback_file,
			medium_size*ss_fallback_scale))
	{
		delete sSSFallback;
		sSSFallback = NULL;
	}
	failed |= !loadFace(sSansSerif, sansserif_file, medium_size, sSSFallback);


	if(!sSansSerifSmall)
	{
		sSansSerifSmall = new LLFontGL();
	}
	else
	{
		sSansSerifSmall->reset();
	}

	if(sSSSmallFallback)
	{
		delete sSSSmallFallback;
	}
	sSSSmallFallback = new LLFontList();
	if (!loadFaceFallback(
			sSSSmallFallback,
			sanserif_fallback_file,
			small_size*ss_fallback_scale))
	{
		delete sSSSmallFallback;
		sSSSmallFallback = NULL;
	}
	failed |= !loadFace(sSansSerifSmall, sansserif_file, small_size, sSSSmallFallback);


	//
	// Sans-serif bold
	//
	if(!sSansSerifBold)
	{
		sSansSerifBold = new LLFontGL();
	}
	else
	{
		sSansSerifBold->reset();
	}

	if (sSSBoldFallback)
	{
		delete sSSBoldFallback;
	}
	sSSBoldFallback = new LLFontList();
	if (!loadFaceFallback(
			sSSBoldFallback,
			sanserif_fallback_file,
			medium_size*ss_fallback_scale))
	{
		delete sSSBoldFallback;
		sSSBoldFallback = NULL;
	}
	failed |= !loadFace(sSansSerifBold, sansserif_bold_file, medium_size, sSSBoldFallback);

	return !failed;
}



// static
void LLFontGL::destroyDefaultFonts()
{
	delete sMonospace;
	sMonospace = NULL;

	delete sSansSerifHuge;
	sSansSerifHuge = NULL;

	delete sSansSerifBig;
	sSansSerifBig = NULL;

	delete sSansSerif;
	sSansSerif = NULL;

	delete sSansSerifSmall;
	sSansSerifSmall = NULL;

	delete sSansSerifBold;
	sSansSerifBold = NULL;

	delete sMonospaceFallback;
	sMonospaceFallback = NULL;

	delete sSSHugeFallback;
	sSSHugeFallback = NULL;

	delete sSSBigFallback;
	sSSBigFallback = NULL;

	delete sSSFallback;
	sSSFallback = NULL;

	delete sSSSmallFallback;
	sSSSmallFallback = NULL;

	delete sSSBoldFallback;
	sSSBoldFallback = NULL;
}

//static 
void LLFontGL::destroyGL()
{
	if (!sMonospace)
	{
		// Already all destroyed.
		return;
	}
	sMonospace->mImageGLp->destroyGLTexture();
	sSansSerifHuge->mImageGLp->destroyGLTexture();
	sSansSerifSmall->mImageGLp->destroyGLTexture();
	sSansSerif->mImageGLp->destroyGLTexture();
	sSansSerifBig->mImageGLp->destroyGLTexture();
	sSansSerifBold->mImageGLp->destroyGLTexture();
}



LLFontGL &LLFontGL::operator=(const LLFontGL &source)
{
	llerrs << "Not implemented" << llendl;
	return *this;
}

BOOL LLFontGL::loadFace(const std::string& filename,
						const F32 point_size, const F32 vert_dpi, const F32 horz_dpi,
						const S32 components, BOOL is_fallback)
{
	if (!LLFont::loadFace(filename, point_size, vert_dpi, horz_dpi, components, is_fallback))
	{
		return FALSE;
	}
	mImageGLp->createGLTexture(0, mRawImageGLp);
	gGL.getTexUnit(0)->bind(mImageGLp);
	mImageGLp->setFilteringOption(LLTexUnit::TFO_POINT);
	return TRUE;
}

BOOL LLFontGL::addChar(const llwchar wch)
{
	if (!LLFont::addChar(wch))
	{
		return FALSE;
	}

	stop_glerror();
	mImageGLp->setSubImage(mRawImageGLp, 0, 0, mImageGLp->getWidth(), mImageGLp->getHeight());
	gGL.getTexUnit(0)->bind(mImageGLp);
	mImageGLp->setFilteringOption(LLTexUnit::TFO_POINT);
	stop_glerror();
	return TRUE;
}


S32 LLFontGL::renderUTF8(const std::string &text, const S32 offset, 
					 const F32 x, const F32 y,
					 const LLColor4 &color,
					 const HAlign halign, const VAlign valign,
					 U8 style,
					 const S32 max_chars, const S32 max_pixels,
					 F32* right_x,
					 BOOL use_ellipses) const
{
	LLWString wstr = utf8str_to_wstring(text);
	return render(wstr, offset, x, y, color, halign, valign, style, max_chars, max_pixels, right_x, FALSE, use_ellipses);
}

S32 LLFontGL::render(const LLWString &wstr, 
					 const S32 begin_offset,
					 const F32 x, const F32 y,
					 const LLColor4 &color,
					 const HAlign halign, const VAlign valign,
					 U8 style,
					 const S32 max_chars, S32 max_pixels,
					 F32* right_x,
					 BOOL use_embedded,
					 BOOL use_ellipses) const
{
	if(!sDisplayFont) //do not display texts
	{
		return wstr.length() ;
	}

	gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);

	if (wstr.empty())
	{
		return 0;
	} 

	S32 scaled_max_pixels = max_pixels == S32_MAX ? S32_MAX : llceil((F32)max_pixels * sScaleX);

	// HACK for better bolding
	if (style & BOLD)
	{
		if (this == LLFontGL::sSansSerif)
		{
			return LLFontGL::sSansSerifBold->render(
				wstr, begin_offset,
				x, y,
				color,
				halign, valign, 
				(style & ~BOLD),
				max_chars, max_pixels,
				right_x, use_embedded);
		}
	}

	F32 drop_shadow_strength = 0.f;
	if (style & (DROP_SHADOW | DROP_SHADOW_SOFT))
	{
		F32 luminance;
		color.calcHSL(NULL, NULL, &luminance);
		drop_shadow_strength = clamp_rescale(luminance, 0.35f, 0.6f, 0.f, 1.f);
		if (luminance < 0.35f)
		{
			style = style & ~(DROP_SHADOW | DROP_SHADOW_SOFT);
		}
	}

	gGL.pushMatrix();
	glLoadIdentity();
	gGL.translatef(floorf(sCurOrigin.mX*sScaleX), floorf(sCurOrigin.mY*sScaleY), sCurOrigin.mZ);

	// this code snaps the text origin to a pixel grid to start with
	F32 pixel_offset_x = llround((F32)sCurOrigin.mX) - (sCurOrigin.mX);
	F32 pixel_offset_y = llround((F32)sCurOrigin.mY) - (sCurOrigin.mY);
	gGL.translatef(-pixel_offset_x, -pixel_offset_y, 0.f);

	LLFastTimer t(LLFastTimer::FTM_RENDER_FONTS);

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

	// Bind the font texture
	
	gGL.getTexUnit(0)->bind(mImageGLp);
	
 	// Not guaranteed to be set correctly
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
	
	cur_x = ((F32)x * sScaleX);
	cur_y = ((F32)y * sScaleY);

	// Offset y by vertical alignment.
	switch (valign)
	{
	case TOP:
		cur_y -= mAscender;
		break;
	case BOTTOM:
		cur_y += mDescender;
		break;
	case VCENTER:
		cur_y -= ((mAscender - mDescender)/2.f);
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
	  	cur_x -= llmin(scaled_max_pixels, llround(getWidthF32(wstr.c_str(), 0, length) * sScaleX));
		break;
	case HCENTER:
	    cur_x -= llmin(scaled_max_pixels, llround(getWidthF32(wstr.c_str(), 0, length) * sScaleX)) / 2;
		break;
	default:
		break;
	}

	cur_render_y = cur_y;
	cur_render_x = cur_x;

	F32 start_x = cur_x;

	F32 inv_width = 1.f / mImageGLp->getWidth();
	F32 inv_height = 1.f / mImageGLp->getHeight();

	const S32 LAST_CHARACTER = LLFont::LAST_CHAR_FULL;


	BOOL draw_ellipses = FALSE;
	if (use_ellipses && halign == LEFT)
	{
		// check for too long of a string
		if (getWidthF32(wstr.c_str(), 0, max_chars) * sScaleX > scaled_max_pixels)
		{
			// use four dots for ellipsis width to generate padding
			const LLWString dots(utf8str_to_wstring(std::string("....")));
			scaled_max_pixels = llmax(0, scaled_max_pixels - llround(getWidthF32(dots.c_str())));
			draw_ellipses = TRUE;
		}
	}


	for (i = begin_offset; i < begin_offset + length; i++)
	{
		llwchar wch = wstr[i];

		// Handle embedded characters first, if they're enabled.
		// Embedded characters are a hack for notecards
		const embedded_data_t* ext_data = use_embedded ? getEmbeddedCharData(wch) : NULL;
		if (ext_data)
		{
			LLImageGL* ext_image = ext_data->mImage;
			const LLWString& label = ext_data->mLabel;

			F32 ext_height = (F32)ext_image->getHeight() * sScaleY;

			F32 ext_width = (F32)ext_image->getWidth() * sScaleX;
			F32 ext_advance = (EXT_X_BEARING * sScaleX) + ext_width;

			if (!label.empty())
			{
				ext_advance += (EXT_X_BEARING + gExtCharFont->getWidthF32( label.c_str() )) * sScaleX;
			}

			if (start_x + scaled_max_pixels < cur_x + ext_advance)
			{
				// Not enough room for this character.
				break;
			}

			gGL.getTexUnit(0)->bind(ext_image);
			// snap origin to whole screen pixel
			const F32 ext_x = (F32)llround(cur_render_x + (EXT_X_BEARING * sScaleX));
			const F32 ext_y = (F32)llround(cur_render_y + (EXT_Y_BEARING * sScaleY + mAscender - mLineHeight));

			LLRectf uv_rect(0.f, 1.f, 1.f, 0.f);
			LLRectf screen_rect(ext_x, ext_y + ext_height, ext_x + ext_width, ext_y);
			drawGlyph(screen_rect, uv_rect, LLColor4::white, style, drop_shadow_strength);

			if (!label.empty())
			{
				gGL.pushMatrix();
				//glLoadIdentity();
				//gGL.translatef(sCurOrigin.mX, sCurOrigin.mY, 0.0f);
				//glScalef(sScaleX, sScaleY, 1.f);
				gExtCharFont->render(label, 0,
									 /*llfloor*/((ext_x + (F32)ext_image->getWidth() + EXT_X_BEARING) / sScaleX), 
									 /*llfloor*/(cur_y / sScaleY),
									 color,
									 halign, BASELINE, NORMAL, S32_MAX, S32_MAX, NULL,
									 TRUE );
				gGL.popMatrix();
			}

			gGL.color4fv(color.mV);

			chars_drawn++;
			cur_x += ext_advance;
			if (((i + 1) < length) && wstr[i+1])
			{
				cur_x += EXT_KERNING * sScaleX;
			}
			cur_render_x = cur_x;

			// Bind the font texture
			gGL.getTexUnit(0)->bind(mImageGLp);
		}
		else
		{
			if (!hasGlyph(wch))
			{
				(const_cast<LLFontGL*>(this))->addChar(wch);
			}

			const LLFontGlyphInfo* fgi= getGlyphInfo(wch);
			if (!fgi)
			{
				llerrs << "Missing Glyph Info" << llendl;
				break;
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
			
			drawGlyph(screen_rect, uv_rect, color, style, drop_shadow_strength);

			chars_drawn++;
			cur_x += fgi->mXAdvance;
			cur_y += fgi->mYAdvance;

			llwchar next_char = wstr[i+1];
			if (next_char && (next_char < LAST_CHARACTER))
			{
				// Kern this puppy.
				if (!hasGlyph(next_char))
				{
					(const_cast<LLFontGL*>(this))->addChar(next_char);
				}
				cur_x += getXKerning(wch, next_char);
			}

			// Round after kerning.
			// Must do this to cur_x, not just to cur_render_x, otherwise you
			// will squish sub-pixel kerned characters too close together.
			// For example, "CCCCC" looks bad.
			cur_x = (F32)llfloor(cur_x + 0.5f);
			//cur_y = (F32)llfloor(cur_y + 0.5f);

			cur_render_x = cur_x;
			cur_render_y = cur_y;
		}
	}

	if (right_x)
	{
		*right_x = cur_x / sScaleX;
	}

	if (style & UNDERLINE)
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.begin(LLRender::LINES);
		gGL.vertex2f(start_x, cur_y - (mDescender));
		gGL.vertex2f(cur_x, cur_y - (mDescender));
		gGL.end();
	}

	// *FIX: get this working in all alignment cases, etc.
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
				style,
				S32_MAX, max_pixels,
				right_x,
				FALSE); 
		gGL.popMatrix();
	}

	gGL.popMatrix();

	return chars_drawn;
}

 
LLImageGL *LLFontGL::getImageGL() const
{
	return mImageGLp;
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

S32 LLFontGL::getWidth(const std::string& utf8text, const S32 begin_offset, const S32 max_chars) const
{
	LLWString wtext = utf8str_to_wstring(utf8text);
	return getWidth(wtext.c_str(), begin_offset, max_chars);
}

S32 LLFontGL::getWidth(const llwchar* wchars, const S32 begin_offset, const S32 max_chars, BOOL use_embedded) const
{
	F32 width = getWidthF32(wchars, begin_offset, max_chars, use_embedded);
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

F32 LLFontGL::getWidthF32(const std::string& utf8text, const S32 begin_offset, const S32 max_chars ) const
{
	LLWString wtext = utf8str_to_wstring(utf8text);
	return getWidthF32(wtext.c_str(), begin_offset, max_chars);
}

F32 LLFontGL::getWidthF32(const llwchar* wchars, const S32 begin_offset, const S32 max_chars, BOOL use_embedded) const
{
	const S32 LAST_CHARACTER = LLFont::LAST_CHAR_FULL;

	F32 cur_x = 0;
	const S32 max_index = begin_offset + max_chars;
	for (S32 i = begin_offset; i < max_index; i++)
	{
		const llwchar wch = wchars[i];
		if (wch == 0)
		{
			break; // done
		}
		const embedded_data_t* ext_data = use_embedded ? getEmbeddedCharData(wch) : NULL;
		if (ext_data)
		{
			// Handle crappy embedded hack
			cur_x += getEmbeddedCharAdvance(ext_data);

			if( ((i+1) < max_chars) && (i+1 < max_index))
			{
				cur_x += EXT_KERNING * sScaleX;
			}
		}
		else
		{
			cur_x += getXAdvance(wch);
			llwchar next_char = wchars[i+1];

			if (((i + 1) < max_chars) 
				&& next_char 
				&& (next_char < LAST_CHARACTER))
			{
				// Kern this puppy.
				cur_x += getXKerning(wch, next_char);
			}
		}
		// Round after kerning.
		cur_x = (F32)llfloor(cur_x + 0.5f);
	}

	return cur_x / sScaleX;
}



// Returns the max number of complete characters from text (up to max_chars) that can be drawn in max_pixels
S32 LLFontGL::maxDrawableChars(const llwchar* wchars, F32 max_pixels, S32 max_chars,
							   BOOL end_on_word_boundary, const BOOL use_embedded,
							   F32* drawn_pixels) const
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

	F32 scaled_max_pixels =	(F32)llceil(max_pixels * sScaleX);

	S32 i;
	for (i=0; (i < max_chars); i++)
	{
		llwchar wch = wchars[i];

		if(wch == 0)
		{
			// Null terminator.  We're done.
			break;
		}
			
		const embedded_data_t* ext_data = use_embedded ? getEmbeddedCharData(wch) : NULL;
		if (ext_data)
		{
			if (in_word)
			{
				in_word = FALSE;
			}
			else
			{
				start_of_last_word = i;
			}
			cur_x += getEmbeddedCharAdvance(ext_data);
			
			if (scaled_max_pixels < cur_x)
			{
				clip = TRUE;
				break;
			}
			
			if (((i+1) < max_chars) && wchars[i+1])
			{
				cur_x += EXT_KERNING * sScaleX;
			}

			if( scaled_max_pixels < cur_x )
			{
				clip = TRUE;
				break;
			}
		}
		else
		{
			if (in_word)
			{
				if (iswspace(wch))
				{
					in_word = FALSE;
				}
			}
			else
			{
				start_of_last_word = i;
				if (!iswspace(wch))
				{
					in_word = TRUE;
				}
			}

			cur_x += getXAdvance(wch);
			
			if (scaled_max_pixels < cur_x)
			{
				clip = TRUE;
				break;
			}

			if (((i+1) < max_chars) && wchars[i+1])
			{
				// Kern this puppy.
				cur_x += getXKerning(wch, wchars[i+1]);
			}
		}
		// Round after kerning.
		cur_x = (F32)llfloor(cur_x + 0.5f);
		drawn_x = cur_x;
	}

	if( clip && end_on_word_boundary && (start_of_last_word != 0) )
	{
		i = start_of_last_word;
	}
	if (drawn_pixels)
	{
		*drawn_pixels = drawn_x;
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

		const embedded_data_t* ext_data = getEmbeddedCharData(wch);
		F32 char_width = ext_data ? getEmbeddedCharAdvance(ext_data) : getXAdvance(wch);

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
			total_width += ext_data ? (EXT_KERNING * sScaleX) : getXKerning(wchars[i-1], wch);
		}

		// Round after kerning.
		total_width = llround(total_width);
	}

	return start_pos - drawable_chars;
}


S32 LLFontGL::charFromPixelOffset(const llwchar* wchars, const S32 begin_offset, F32 target_x, F32 max_pixels, S32 max_chars, BOOL round, BOOL use_embedded) const
{
	if (!wchars || !wchars[0] || max_chars == 0)
	{
		return 0;
	}
	
	F32 cur_x = 0;
	S32 pos = 0;

	target_x *= sScaleX;

	// max_chars is S32_MAX by default, so make sure we don't get overflow
	const S32 max_index = begin_offset + llmin(S32_MAX - begin_offset, max_chars);

	F32 scaled_max_pixels =	max_pixels * sScaleX;

	for (S32 i = begin_offset; (i < max_index); i++)
	{
		llwchar wch = wchars[i];
		if (!wch)
		{
			break; // done
		}
		const embedded_data_t* ext_data = use_embedded ? getEmbeddedCharData(wch) : NULL;
		if (ext_data)
		{
			F32 ext_advance = getEmbeddedCharAdvance(ext_data);

			if (round)
			{
				// Note: if the mouse is on the left half of the character, the pick is to the character's left
				// If it's on the right half, the pick is to the right.
				if (target_x  < cur_x + ext_advance/2)
				{
					break;
				}
			}
			else
			{
				if (target_x  < cur_x + ext_advance)
				{
					break;
				}
			}

			if (scaled_max_pixels < cur_x + ext_advance)
			{
				break;
			}

			pos++;
			cur_x += ext_advance;

			if (((i + 1) < max_index)
				&& (wchars[(i + 1)]))
			{
				cur_x += EXT_KERNING * sScaleX;
			}
			// Round after kerning.
			cur_x = (F32)llfloor(cur_x + 0.5f);
		}
		else
		{
			F32 char_width = getXAdvance(wch);

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

			pos++;
			cur_x += char_width;

			if (((i + 1) < max_index)
				&& (wchars[(i + 1)]))
			{
				llwchar next_char = wchars[i + 1];
				// Kern this puppy.
				cur_x += getXKerning(wch, next_char);
			}

			// Round after kerning.
			cur_x = (F32)llfloor(cur_x + 0.5f);
		}
	}

	return pos;
}


const LLFontGL::embedded_data_t* LLFontGL::getEmbeddedCharData(const llwchar wch) const
{
	// Handle crappy embedded hack
	embedded_map_t::const_iterator iter = mEmbeddedChars.find(wch);
	if (iter != mEmbeddedChars.end())
	{
		return iter->second;
	}
	return NULL;
}


F32 LLFontGL::getEmbeddedCharAdvance(const embedded_data_t* ext_data) const
{
	const LLWString& label = ext_data->mLabel;
	LLImageGL* ext_image = ext_data->mImage;

	F32 ext_width = (F32)ext_image->getWidth();
	if( !label.empty() )
	{
		ext_width += (EXT_X_BEARING + gExtCharFont->getWidthF32(label.c_str())) * sScaleX;
	}

	return (EXT_X_BEARING * sScaleX) + ext_width;
}


void LLFontGL::clearEmbeddedChars()
{
	for_each(mEmbeddedChars.begin(), mEmbeddedChars.end(), DeletePairedPointer());
	mEmbeddedChars.clear();
}

void LLFontGL::addEmbeddedChar( llwchar wc, LLImageGL* image, const std::string& label )
{
	LLWString wlabel = utf8str_to_wstring(label);
	addEmbeddedChar(wc, image, wlabel);
}

void LLFontGL::addEmbeddedChar( llwchar wc, LLImageGL* image, const LLWString& wlabel )
{
	embedded_data_t* ext_data = new embedded_data_t(image, wlabel);
	mEmbeddedChars[wc] = ext_data;
}

void LLFontGL::removeEmbeddedChar( llwchar wc )
{
	embedded_map_t::iterator iter = mEmbeddedChars.find(wc);
	if (iter != mEmbeddedChars.end())
	{
		delete iter->second;
		mEmbeddedChars.erase(wc);
	}
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

void LLFontGL::drawGlyph(const LLRectf& screen_rect, const LLRectf& uv_rect, const LLColor4& color, U8 style, F32 drop_shadow_strength) const
{
	F32 slant_offset;
	slant_offset = ((style & ITALIC) ? ( -mAscender * 0.2f) : 0.f);

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
		else if (style & DROP_SHADOW_SOFT)
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
		else if (style & DROP_SHADOW)
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

// static
std::string LLFontGL::nameFromFont(const LLFontGL* fontp)
{
	if (fontp == sSansSerifHuge)
	{
		return std::string("SansSerifHuge");
	}
	else if (fontp == sSansSerifSmall)
	{
		return std::string("SansSerifSmall");
	}
	else if (fontp == sSansSerif)
	{
		return std::string("SansSerif");
	}
	else if (fontp == sSansSerifBig)
	{
		return std::string("SansSerifBig");
	}
	else if (fontp == sSansSerifBold)
	{
		return std::string("SansSerifBold");
	}
	else if (fontp == sMonospace)
	{
		return std::string("Monospace");
	}
	else
	{
		return std::string();
	}
}

// static
LLFontGL* LLFontGL::fontFromName(const std::string& font_name)
{
	LLFontGL* gl_font = NULL;
	if (font_name == "SansSerifHuge")
	{
		gl_font = LLFontGL::sSansSerifHuge;
	}
	else if (font_name == "SansSerifSmall")
	{
		gl_font = LLFontGL::sSansSerifSmall;
	}
	else if (font_name == "SansSerif")
	{
		gl_font = LLFontGL::sSansSerif;
	}
	else if (font_name == "SansSerifBig")
	{
		gl_font = LLFontGL::sSansSerifBig;
	}
	else if (font_name == "SansSerifBold")
	{
		gl_font = LLFontGL::sSansSerifBold;
	}
	else if (font_name == "Monospace")
	{
		gl_font = LLFontGL::sMonospace;
	}
	return gl_font;
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
