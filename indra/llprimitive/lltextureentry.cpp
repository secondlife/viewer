/** 
 * @file lltextureentry.cpp
 * @brief LLTextureEntry base class
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

#include "linden_common.h"

#include "lltextureentry.h"
#include "llsdutil.h"

const U8 DEFAULT_BUMP_CODE = 0;  // no bump or shininess

const LLTextureEntry LLTextureEntry::null;

//===============================================================
LLTextureEntry::LLTextureEntry()
{
	init(LLUUID::null,1.f,1.f,0.f,0.f,0.f,DEFAULT_BUMP_CODE);
}

LLTextureEntry::LLTextureEntry(const LLUUID& tex_id)
{
	init(tex_id,1.f,1.f,0.f,0.f,0.f,DEFAULT_BUMP_CODE);
}

LLTextureEntry::LLTextureEntry(const LLTextureEntry &rhs)
{
	mID = rhs.mID;
	mScaleS = rhs.mScaleS;
	mScaleT = rhs.mScaleT;
	mOffsetS = rhs.mOffsetS;
	mOffsetT = rhs.mOffsetT;
	mRotation = rhs.mRotation;
	mColor = rhs.mColor;
	mBump = rhs.mBump;
	mMediaFlags = rhs.mMediaFlags;
	mGlow = rhs.mGlow;
}

LLTextureEntry &LLTextureEntry::operator=(const LLTextureEntry &rhs)
{
	if (this != &rhs)
	{
		mID = rhs.mID;
		mScaleS = rhs.mScaleS;
		mScaleT = rhs.mScaleT;
		mOffsetS = rhs.mOffsetS;
		mOffsetT = rhs.mOffsetT;
		mRotation = rhs.mRotation;
		mColor = rhs.mColor;
		mBump = rhs.mBump;
		mMediaFlags = rhs.mMediaFlags;
		mGlow = rhs.mGlow;
	}

	return *this;
}

void LLTextureEntry::init(const LLUUID& tex_id, F32 scale_s, F32 scale_t, F32 offset_s, F32 offset_t, F32 rotation, U8 bump)
{
	setID(tex_id);

	mScaleS = scale_s;
	mScaleT = scale_t;
	mOffsetS = offset_s;
	mOffsetT = offset_t;
	mRotation = rotation;
	mBump = bump;
	mMediaFlags = 0x0;
    mGlow = 0;
	
	setColor(LLColor4(1.f, 1.f, 1.f, 1.f));
}

LLTextureEntry::~LLTextureEntry()
{
}

bool LLTextureEntry::operator!=(const LLTextureEntry &rhs) const
{
	if (mID != rhs.mID) return(true);
	if (mScaleS != rhs.mScaleS) return(true);
	if (mScaleT != rhs.mScaleT) return(true);
	if (mOffsetS != rhs.mOffsetS) return(true);
	if (mOffsetT != rhs.mOffsetT) return(true);
	if (mRotation != rhs.mRotation) return(true);
	if (mColor != rhs.mColor) return (true);
	if (mBump != rhs.mBump) return (true);
	if (mMediaFlags != rhs.mMediaFlags) return (true);
	if (mGlow != rhs.mGlow) return (true);
	return(false);
}

bool LLTextureEntry::operator==(const LLTextureEntry &rhs) const
{
	if (mID != rhs.mID) return(false);
	if (mScaleS != rhs.mScaleS) return(false);
	if (mScaleT != rhs.mScaleT) return(false);
	if (mOffsetS != rhs.mOffsetS) return(false);
	if (mOffsetT != rhs.mOffsetT) return(false);
	if (mRotation != rhs.mRotation) return(false);
	if (mColor != rhs.mColor) return (false);
	if (mBump != rhs.mBump) return (false);
	if (mMediaFlags != rhs.mMediaFlags) return false;
	if (mGlow != rhs.mGlow) return false;
	return(true);
}

LLSD LLTextureEntry::asLLSD() const
{
	LLSD sd;

	sd["imageid"] = getID();
	sd["colors"] = ll_sd_from_color4(getColor());
	sd["scales"] = mScaleS;
	sd["scalet"] = mScaleT;
	sd["offsets"] = mOffsetS;
	sd["offsett"] = mOffsetT;
	sd["imagerot"] = getRotation();
	sd["bump"] = getBumpShiny();
	sd["fullbright"] = getFullbright();
	sd["media_flags"] = getMediaTexGen();
	sd["glow"] = getGlow();
	
	return sd;
}

bool LLTextureEntry::fromLLSD(LLSD& sd)
{
	const char *w, *x;
	w = "imageid";
	if (sd.has(w))
	{
		setID( sd[w] );
	} else goto fail;
	w = "colors";
	if (sd.has(w))
	{
		setColor( ll_color4_from_sd(sd["colors"]) );
	} else goto fail;
	w = "scales";
	x = "scalet";
	if (sd.has(w) && sd.has(x))
	{
		setScale( (F32)sd[w].asReal(), (F32)sd[x].asReal() );
	} else goto fail;
	w = "offsets";
	x = "offsett";
	if (sd.has(w) && sd.has(x))
	{
		setOffset( (F32)sd[w].asReal(), (F32)sd[x].asReal() );
	} else goto fail;
	w = "imagerot";
	if (sd.has(w))
	{
		setRotation( (F32)sd[w].asReal() );
	} else goto fail;
	w = "bump";
	if (sd.has(w))
	{
		setBumpShiny( sd[w].asInteger() );
	} else goto fail;
	w = "fullbright";
	if (sd.has(w))
	{
		setFullbright( sd[w].asInteger() );
	} else goto fail;
	w = "media_flags";
	if (sd.has(w))
	{
		setMediaTexGen( sd[w].asInteger() );
	} else goto fail;
	w = "glow";
	if (sd.has(w))
	{
		setGlow((F32)sd[w].asReal() );
	}

	return true;
fail:
	return false;
}

S32 LLTextureEntry::setID(const LLUUID &tex_id)
{
	if (mID != tex_id)
	{
		mID = tex_id;
		return TEM_CHANGE_TEXTURE;
	}
	return 0;
}

S32 LLTextureEntry::setScale(F32 s, F32 t)
{
	S32 retval = 0;

	if (  (mScaleS != s)
		||(mScaleT != t))
	{
		mScaleS = s;
		mScaleT = t;

		retval = TEM_CHANGE_TEXTURE;
	}
	return retval;
}

S32 LLTextureEntry::setColor(const LLColor4 &color)
{
	if (mColor != color)
	{
		mColor = color;
		return TEM_CHANGE_COLOR;
	}
	return 0;
}

S32 LLTextureEntry::setColor(const LLColor3 &color)
{
	if (mColor != color)
	{
		// This preserves alpha.
		mColor.setVec(color);
		return TEM_CHANGE_COLOR;
	}
	return 0;
}

S32 LLTextureEntry::setAlpha(const F32 alpha)
{
	if (mColor.mV[VW] != alpha)
	{
		mColor.mV[VW] = alpha;
		return TEM_CHANGE_COLOR;
	}
	return 0;
}

S32 LLTextureEntry::setOffset(F32 s, F32 t)
{
	S32 retval = 0;

	if (  (mOffsetS != s)
		||(mOffsetT != t))
	{
		mOffsetS = s;
		mOffsetT = t;

		retval = TEM_CHANGE_TEXTURE;
	}
	return retval;
}

S32 LLTextureEntry::setRotation(F32 theta)
{
	if (mRotation != theta)
	{
		mRotation = theta;
		return TEM_CHANGE_TEXTURE;
	}
	return 0;
}

S32 LLTextureEntry::setBumpShinyFullbright(U8 bump)
{
	if (mBump != bump)
	{
		mBump = bump;
		return TEM_CHANGE_TEXTURE;
	}
	return 0;
}

S32 LLTextureEntry::setMediaTexGen(U8 media)
{
	if (mMediaFlags != media)
	{
		mMediaFlags = media;
		return TEM_CHANGE_TEXTURE;
	}
	return 0;
}

S32 LLTextureEntry::setBumpmap(U8 bump)
{
	bump &= TEM_BUMP_MASK;
	if (getBumpmap() != bump)
	{
		mBump &= ~TEM_BUMP_MASK;
		mBump |= bump;
		return TEM_CHANGE_TEXTURE;
	}
	return 0;
}

S32 LLTextureEntry::setFullbright(U8 fullbright)
{
	fullbright &= TEM_FULLBRIGHT_MASK;
	if (getFullbright() != fullbright)
	{
		mBump &= ~(TEM_FULLBRIGHT_MASK<<TEM_FULLBRIGHT_SHIFT);
		mBump |= fullbright << TEM_FULLBRIGHT_SHIFT;
		return TEM_CHANGE_TEXTURE;
	}
	return 0;
}

S32 LLTextureEntry::setShiny(U8 shiny)
{
	shiny &= TEM_SHINY_MASK;
	if (getShiny() != shiny)
	{
		mBump &= ~(TEM_SHINY_MASK<<TEM_SHINY_SHIFT);
		mBump |= shiny << TEM_SHINY_SHIFT;
		return TEM_CHANGE_TEXTURE;
	}
	return 0;
}

S32 LLTextureEntry::setBumpShiny(U8 bump_shiny)
{
	bump_shiny &= TEM_BUMP_SHINY_MASK;
	if (getBumpShiny() != bump_shiny)
	{
		mBump &= ~TEM_BUMP_SHINY_MASK;
		mBump |= bump_shiny;
		return TEM_CHANGE_TEXTURE;
	}
	return 0;
}

S32 LLTextureEntry::setMediaFlags(U8 media_flags)
{
	media_flags &= TEM_MEDIA_MASK;
	if (getMediaFlags() != media_flags)
	{
		mMediaFlags &= ~TEM_MEDIA_MASK;
		mMediaFlags |= media_flags;
		return TEM_CHANGE_TEXTURE;
	}
	return 0;
}

S32 LLTextureEntry::setTexGen(U8 tex_gen)
{
	tex_gen &= TEM_TEX_GEN_MASK;
	if (getTexGen() != tex_gen)
	{
		mMediaFlags &= ~TEM_TEX_GEN_MASK;
		mMediaFlags |= tex_gen;
		return TEM_CHANGE_TEXTURE;
	}
	return 0;
}

S32 LLTextureEntry::setGlow(F32 glow)
{
	if (mGlow != glow)
	{
		mGlow = glow;
		return TEM_CHANGE_TEXTURE;
	}
	return 0;
}

