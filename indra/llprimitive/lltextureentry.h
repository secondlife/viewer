/** 
 * @file lltextureentry.h
 * @brief LLTextureEntry base class
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

#ifndef LL_LLTEXTUREENTRY_H
#define LL_LLTEXTUREENTRY_H

#include "lluuid.h"
#include "v4color.h"
#include "llsd.h"

const S32 TEM_CHANGE_COLOR = 0x1;
const S32 TEM_CHANGE_TEXTURE = 0x2;
const S32 TEM_INVALID = 0x4;

const S32 TEM_BUMPMAP_COUNT = 32;

// The Bump Shiny Fullbright values are bits in an eight bit field:
// +----------+
// | SSFBBBBB | S = Shiny, F = Fullbright, B = Bumpmap
// | 76543210 |
// +----------+
const S32 TEM_BUMP_MASK 		= 0x1f; // 5 bits
const S32 TEM_FULLBRIGHT_MASK 	= 0x01; // 1 bit
const S32 TEM_SHINY_MASK 		= 0x03; // 2 bits
const S32 TEM_BUMP_SHINY_MASK 	= (0xc0 | 0x1f);
const S32 TEM_FULLBRIGHT_SHIFT 	= 5;
const S32 TEM_SHINY_SHIFT 		= 6;

// The Media Tex Gen values are bits in a bit field:
// +----------+
// | .....TTM | M = Media Flags (web page), T = LLTextureEntry::eTexGen, . = unused
// | 76543210 |
// +----------+
const S32 TEM_MEDIA_MASK		= 0x01;
const S32 TEM_TEX_GEN_MASK		= 0x06;
const S32 TEM_TEX_GEN_SHIFT		= 1;


class LLTextureEntry
{
public:	

	typedef enum e_texgen
	{
		TEX_GEN_DEFAULT			= 0x00,
		TEX_GEN_PLANAR			= 0x02,
		TEX_GEN_SPHERICAL		= 0x04,
		TEX_GEN_CYLINDRICAL		= 0x06
	} eTexGen;

	LLTextureEntry();
	LLTextureEntry(const LLUUID& tex_id);
	LLTextureEntry(const LLTextureEntry &rhs);

	LLTextureEntry &operator=(const LLTextureEntry &rhs);
    ~LLTextureEntry();

	bool operator==(const LLTextureEntry &rhs) const;
	bool operator!=(const LLTextureEntry &rhs) const;

	LLSD asLLSD() const;
	operator LLSD() const { return asLLSD(); }
	bool fromLLSD(LLSD& sd);

	void init(const LLUUID& tex_id, F32 scale_s, F32 scale_t, F32 offset_s, F32 offset_t, F32 rotation, U8 bump);

	// These return a TEM_ flag from above to indicate if something changed.
	S32  setID (const LLUUID &tex_id);
	S32  setColor(const LLColor4 &color);
	S32  setColor(const LLColor3 &color);
	S32  setAlpha(const F32 alpha);
	S32  setScale(F32 s, F32 t);
	S32  setOffset(F32 s, F32 t);
	S32  setRotation(F32 theta);

	S32  setBumpmap(U8 bump);
	S32  setFullbright(U8 bump);
	S32  setShiny(U8 bump);
	S32  setBumpShiny(U8 bump);
 	S32  setBumpShinyFullbright(U8 bump);

	S32  setMediaFlags(U8 media_flags);
	S32	 setTexGen(U8 texGen);
	S32  setMediaTexGen(U8 media);
    S32  setGlow(F32 glow);
	
	const LLUUID &getID() const { return mID; }
	const LLColor4 &getColor() const { return mColor; }
	void getScale(F32 *s, F32 *t) const { *s = mScaleS; *t = mScaleT; }
	void getOffset(F32 *s, F32 *t) const { *s = mOffsetS; *t = mOffsetT; }
	F32  getRotation() const { return mRotation; }
	void getRotation(F32 *theta) const { *theta = mRotation; }

	U8	 getBumpmap() const { return mBump & TEM_BUMP_MASK; }
	U8	 getFullbright() const { return (mBump>>TEM_FULLBRIGHT_SHIFT) & TEM_FULLBRIGHT_MASK; }
	U8	 getShiny() const { return (mBump>>TEM_SHINY_SHIFT) & TEM_SHINY_MASK; }
	U8	 getBumpShiny() const { return mBump & TEM_BUMP_SHINY_MASK; }
 	U8	 getBumpShinyFullbright() const { return mBump; }

	U8	 getMediaFlags() const { return mMediaFlags & TEM_MEDIA_MASK; }
	U8	 getTexGen() const	{ return mMediaFlags & TEM_TEX_GEN_MASK; }
	U8	 getMediaTexGen() const { return mMediaFlags; }
    F32  getGlow() const { return mGlow; }
	
	// Media flags
	enum { MF_NONE = 0x0, MF_WEB_PAGE = 0x1 };

public:
	F32                 mScaleS;                // S, T offset
	F32                 mScaleT;                // S, T offset
	F32                 mOffsetS;               // S, T offset
	F32                 mOffsetT;               // S, T offset
	F32                 mRotation;              // anti-clockwise rotation in rad about the bottom left corner

	static const LLTextureEntry null;
protected:
	LLUUID				mID;					// Texture GUID
	LLColor4			mColor;
	U8					mBump;					// Bump map, shiny, and fullbright
	U8					mMediaFlags;			// replace with web page, movie, etc.
	F32                 mGlow;

	// NOTE: when adding new data to this class, in addition to adding it to the serializers asLLSD/fromLLSD and the
	// message packers (e.g. LLPrimitive::packTEMessage) you must also implement its copy in LLPrimitive::copyTEs()

	
};

#endif
