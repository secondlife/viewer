/** 
 * @file llfontgl.h
 * @author Doug Soo
 * @brief Wrapper around FreeType
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLFONTGL_H
#define LL_LLFONTGL_H

#include "llfont.h"
#include "llimagegl.h"
#include "v2math.h"
#include "llcoord.h"
#include "llrect.h"

class LLColor4;

class LLFontGL : public LLFont
{
public:
	enum HAlign
	{
		// Horizontal location of x,y coord to render.
		LEFT = 0,		// Left align
		RIGHT = 1,		// Right align
		HCENTER = 2,	// Center
	};

	enum VAlign
	{
		// Vertical location of x,y coord to render.
		TOP = 3,		// Top align
		VCENTER = 4,	// Center
		BASELINE = 5,	// Baseline
		BOTTOM = 6		// Bottom
	};

	enum StyleFlags
	{
		// text style to render.  May be combined (these are bit flags)
		NORMAL = 0,	
		BOLD = 1,
		ITALIC = 2,
		UNDERLINE = 4,
		DROP_SHADOW = 8,
		DROP_SHADOW_SOFT = 16
	};
	
	// Takes a string with potentially several flags, i.e. "NORMAL|BOLD|ITALIC"
	static U8 getStyleFromString(const LLString &style);

	LLFontGL();
	LLFontGL(const LLFontGL &source);
	~LLFontGL();

	void init(); // Internal init, or reinitialization
	void reset(); // Reset a font after GL cleanup.  ONLY works on an already loaded font.

	LLFontGL &operator=(const LLFontGL &source);

	static BOOL initDefaultFonts(F32 screen_dpi, F32 x_scale, F32 y_scale,
								 const LLString& monospace_file, F32 monospace_size,
								 const LLString& sansserif_file,
								 const LLString& sansserif_fallback_file, F32 ss_fallback_scale,
								 F32 small_size, F32 medium_size, F32 large_size, F32 huge_size,
								 const LLString& sansserif_bold_file, F32 bold_size,
								 const LLString& app_dir = LLString::null);

	static void	destroyDefaultFonts();
	static void destroyGL();

	static bool loadFaceFallback(LLFontList *fontp, const LLString& fontname, const F32 point_size);
	static bool loadFace(LLFontGL *fontp, const LLString& fontname, const F32 point_size, LLFontList *fallback_fontp);
	BOOL loadFace(const LLString& filename, const F32 point_size, const F32 vert_dpi, const F32 horz_dpi);


	S32 renderUTF8(const LLString &text, const S32 begin_offset,
				   S32 x, S32 y,
				   const LLColor4 &color) const
	{
		return renderUTF8(text, begin_offset, (F32)x, (F32)y, color,
						  LEFT, BASELINE, NORMAL,
						  S32_MAX, S32_MAX, NULL, FALSE);
	}
	
	S32 renderUTF8(const LLString &text, const S32 begin_offset,
				   S32 x, S32 y,
				   const LLColor4 &color,
				   HAlign halign, VAlign valign, U8 style = NORMAL) const
	{
		return renderUTF8(text, begin_offset, (F32)x, (F32)y, color,
						  halign, valign, style,
						  S32_MAX, S32_MAX, NULL, FALSE);
	}
	
	// renderUTF8 does a conversion, so is slower!
	S32 renderUTF8(const LLString &text,
		S32 begin_offset,
		F32 x, F32 y,
		const LLColor4 &color,
		HAlign halign, 
		VAlign valign,
		U8 style,
		S32 max_chars,
		S32 max_pixels, 
		F32* right_x,
		BOOL use_ellipses) const;

	S32 render(const LLWString &text, const S32 begin_offset,
			   F32 x, F32 y,
			   const LLColor4 &color) const
	{
		return render(text, begin_offset, x, y, color,
					  LEFT, BASELINE, NORMAL,
					  S32_MAX, S32_MAX, NULL, FALSE, FALSE);
	}
	

	S32 render(const LLWString &text,
		S32 begin_offset,
		F32 x, F32 y,
		const LLColor4 &color,
		HAlign halign = LEFT, 
		VAlign valign = BASELINE,
		U8 style = NORMAL,
		S32 max_chars = S32_MAX,
		S32 max_pixels = S32_MAX, 
		F32* right_x=NULL,
		BOOL use_embedded = FALSE,
		BOOL use_ellipses = FALSE) const;

	// font metrics - override for LLFont that returns units of virtual pixels
	/*virtual*/ F32 getLineHeight() const		{ return (F32)llround(mLineHeight / sScaleY); }
	/*virtual*/ F32 getAscenderHeight() const	{ return (F32)llround(mAscender / sScaleY); }
	/*virtual*/ F32 getDescenderHeight() const	{ return (F32)llround(mDescender / sScaleY); }
	
	virtual S32 getWidth(const LLString& utf8text) const;
	virtual S32 getWidth(const llwchar* wchars) const;
	virtual S32 getWidth(const LLString& utf8text, const S32 offset, const S32 max_chars ) const;
	virtual S32 getWidth(const llwchar* wchars, const S32 offset, const S32 max_chars, BOOL use_embedded = FALSE) const;

	virtual F32 getWidthF32(const LLString& utf8text) const;
	virtual F32 getWidthF32(const llwchar* wchars) const;
	virtual F32 getWidthF32(const LLString& text, const S32 offset, const S32 max_chars ) const;
	virtual F32 getWidthF32(const llwchar* wchars, const S32 offset, const S32 max_chars, BOOL use_embedded = FALSE ) const;

	// The following are called often, frequently with large buffers, so do not use a string interface
	
	// Returns the max number of complete characters from text (up to max_chars) that can be drawn in max_pixels
	virtual S32	maxDrawableChars(const llwchar* wchars, F32 max_pixels, S32 max_chars = S32_MAX,
								 BOOL end_on_word_boundary = FALSE, const BOOL use_embedded = FALSE,
								 F32* drawn_pixels = NULL) const;

	// Returns the index of the first complete characters from text that can be drawn in max_pixels
	// starting on the right side (at character start_pos).
	virtual S32	firstDrawableChar(const llwchar* wchars, F32 max_pixels, S32 text_len, S32 start_pos=S32_MAX, S32 max_chars = S32_MAX) const;

	// Returns the index of the character closest to pixel position x (ignoring text to the right of max_pixels and max_chars)
	virtual S32 charFromPixelOffset(const llwchar* wchars, const S32 char_offset,
									F32 x, F32 max_pixels=F32_MAX, S32 max_chars = S32_MAX,
									BOOL round = TRUE, BOOL use_embedded = FALSE) const;


	LLImageGL *getImageGL() const;

	void	   addEmbeddedChar( llwchar wc, LLImageGL* image, const LLString& label);
	void	   addEmbeddedChar( llwchar wc, LLImageGL* image, const LLWString& label);
	void	   removeEmbeddedChar( llwchar wc );

	static LLString nameFromFont(const LLFontGL* fontp);
	static LLFontGL* fontFromName(const LLString& name);

	static LLString nameFromHAlign(LLFontGL::HAlign align);
	static LLFontGL::HAlign hAlignFromName(const LLString& name);

	static LLString nameFromVAlign(LLFontGL::VAlign align);
	static LLFontGL::VAlign vAlignFromName(const LLString& name);

protected:
	struct embedded_data_t
	{
		embedded_data_t(LLImageGL* image, const LLWString& label) : mImage(image), mLabel(label) {}
		LLPointer<LLImageGL> mImage;
		LLWString			 mLabel;
	};
	const embedded_data_t* getEmbeddedCharData(const llwchar wch) const;
	F32 getEmbeddedCharAdvance(const embedded_data_t* ext_data) const;
	void clearEmbeddedChars();
	void renderQuad(const LLRectf& screen_rect, const LLRectf& uv_rect, F32 slant_amt) const;
	void drawGlyph(const LLRectf& screen_rect, const LLRectf& uv_rect, const LLColor4& color, U8 style, F32 drop_shadow_fade) const;

public:
	static F32 sVertDPI;
	static F32 sHorizDPI;
	static F32 sScaleX;
	static F32 sScaleY;
	static LLString sAppDir;			// For loading fonts
		
	static LLFontGL*	sMonospace;		// medium

	static LLFontGL*	sSansSerifSmall;	// small
	static LLFontList*	sSSSmallFallback;
	static LLFontGL*	sSansSerif;		// medium
	static LLFontList*	sSSFallback;
	static LLFontGL*	sSansSerifBig;		// large
	static LLFontList*	sSSBigFallback;
	static LLFontGL*	sSansSerifHuge;	// very large
	static LLFontList*	sSSHugeFallback;

	static LLFontGL*	sSansSerifBold;	// medium, bolded
	static LLFontList*	sSSBoldFallback;

	static LLColor4 sShadowColor;

	friend class LLTextBillboard;
	friend class LLHUDText;

protected:
	/*virtual*/ BOOL addChar(const llwchar wch);
	static LLString getFontPathLocal();
	static LLString getFontPathSystem();

protected:
	LLPointer<LLImageRaw>	mRawImageGLp;
	LLPointer<LLImageGL>	mImageGLp;
	typedef std::map<llwchar,embedded_data_t*> embedded_map_t;
	embedded_map_t mEmbeddedChars;
	
public:
	static LLCoordFont sCurOrigin;
	static std::vector<LLCoordFont> sOriginStack;
};

#endif
