/** 
 * @file lltextureentry.cpp
 * @brief LLTextureEntry base class
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

#include "lluuid.h"
#include "llmediaentry.h"
#include "lltextureentry.h"
#include "llsdutil_math.h"
#include "v4color.h"

const U8 DEFAULT_BUMP_CODE = 0;  // no bump or shininess

const LLTextureEntry LLTextureEntry::null;

// Some LLSD keys.  Do not change these!
#define OBJECT_ID_KEY_STR "object_id"
#define TEXTURE_INDEX_KEY_STR "texture_index"
#define OBJECT_MEDIA_VERSION_KEY_STR "object_media_version"
#define OBJECT_MEDIA_DATA_KEY_STR "object_media_data"
#define TEXTURE_MEDIA_DATA_KEY_STR "media_data"

/*static*/ const char* LLTextureEntry::OBJECT_ID_KEY = OBJECT_ID_KEY_STR;
/*static*/ const char* LLTextureEntry::OBJECT_MEDIA_DATA_KEY = OBJECT_MEDIA_DATA_KEY_STR;
/*static*/ const char* LLTextureEntry::MEDIA_VERSION_KEY = OBJECT_MEDIA_VERSION_KEY_STR;
/*static*/ const char* LLTextureEntry::TEXTURE_INDEX_KEY = TEXTURE_INDEX_KEY_STR;
/*static*/ const char* LLTextureEntry::TEXTURE_MEDIA_DATA_KEY = TEXTURE_MEDIA_DATA_KEY_STR;

static const std::string MEDIA_VERSION_STRING_PREFIX = "x-mv:";

// static 
LLTextureEntry* LLTextureEntry::newTextureEntry()
{
	return new LLTextureEntry();
}

//===============================================================
LLTextureEntry::LLTextureEntry()
  : mMediaEntry(NULL)
{
	init(LLUUID::null,1.f,1.f,0.f,0.f,0.f,DEFAULT_BUMP_CODE);
}

LLTextureEntry::LLTextureEntry(const LLUUID& tex_id)
  : mMediaEntry(NULL)
{
	init(tex_id,1.f,1.f,0.f,0.f,0.f,DEFAULT_BUMP_CODE);
}

LLTextureEntry::LLTextureEntry(const LLTextureEntry &rhs)
  : mMediaEntry(NULL)
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
	if (rhs.mMediaEntry != NULL) {
		// Make a copy
		mMediaEntry = new LLMediaEntry(*rhs.mMediaEntry);
	}
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
		if (mMediaEntry != NULL) {
			delete mMediaEntry;
		}
		if (rhs.mMediaEntry != NULL) {
			// Make a copy
			mMediaEntry = new LLMediaEntry(*rhs.mMediaEntry);
		}
		else {
			mMediaEntry = NULL;
		}
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
	if (mMediaEntry != NULL) {
	    delete mMediaEntry;
	}
	mMediaEntry = NULL;
}

LLTextureEntry::~LLTextureEntry()
{
	if(mMediaEntry)
	{
		delete mMediaEntry;
		mMediaEntry = NULL;
	}
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
	asLLSD(sd);
	return sd;
}

void LLTextureEntry::asLLSD(LLSD& sd) const
{
	sd["imageid"] = mID;
	sd["colors"] = ll_sd_from_color4(mColor);
	sd["scales"] = mScaleS;
	sd["scalet"] = mScaleT;
	sd["offsets"] = mOffsetS;
	sd["offsett"] = mOffsetT;
	sd["imagerot"] = mRotation;
	sd["bump"] = getBumpShiny();
	sd["fullbright"] = getFullbright();
	sd["media_flags"] = mMediaFlags;
	if (hasMedia()) {
		LLSD mediaData;
        if (NULL != getMediaData()) {
            getMediaData()->asLLSD(mediaData);
        }
		sd[TEXTURE_MEDIA_DATA_KEY] = mediaData;
	}
	sd["glow"] = mGlow;
}

bool LLTextureEntry::fromLLSD(const LLSD& sd)
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
	// If the "has media" flag doesn't match the fact that 
	// media data exists, updateMediaData will "fix" it
	// by either clearing or setting the flag
	w = TEXTURE_MEDIA_DATA_KEY;
	if (hasMedia() != sd.has(w))
	{
		llwarns << "LLTextureEntry::fromLLSD: media_flags (" << hasMedia() <<
			") does not match presence of media_data (" << sd.has(w) << ").  Fixing." << llendl;
	}
	updateMediaData(sd[w]);

	w = "glow";
	if (sd.has(w))
	{
		setGlow((F32)sd[w].asReal() );
	}

	return true;
fail:
	return false;
}

// virtual 
// override this method for each derived class
LLTextureEntry* LLTextureEntry::newBlank() const
{
	return new LLTextureEntry();
}

// virtual 
LLTextureEntry* LLTextureEntry::newCopy() const
{
	return new LLTextureEntry(*this);
}

S32 LLTextureEntry::setID(const LLUUID &tex_id)
{
	if (mID != tex_id)
	{
		mID = tex_id;
		return TEM_CHANGE_TEXTURE;
	}
	return TEM_CHANGE_NONE;
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

S32 LLTextureEntry::setScaleS(F32 s)
{
	S32 retval = TEM_CHANGE_NONE;
	if (mScaleS != s)
	{
		mScaleS = s;
		retval = TEM_CHANGE_TEXTURE;
	}
	return retval;
}

S32 LLTextureEntry::setScaleT(F32 t)
{
	S32 retval = TEM_CHANGE_NONE;
	if (mScaleT != t)
	{
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
	return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setColor(const LLColor3 &color)
{
	if (mColor != color)
	{
		// This preserves alpha.
		mColor.setVec(color);
		return TEM_CHANGE_COLOR;
	}
	return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setAlpha(const F32 alpha)
{
	if (mColor.mV[VW] != alpha)
	{
		mColor.mV[VW] = alpha;
		return TEM_CHANGE_COLOR;
	}
	return TEM_CHANGE_NONE;
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

S32 LLTextureEntry::setOffsetS(F32 s)
{
	S32 retval = 0;
	if (mOffsetS != s)
	{
		mOffsetS = s;
		retval = TEM_CHANGE_TEXTURE;
	}
	return retval;
}

S32 LLTextureEntry::setOffsetT(F32 t)
{
	S32 retval = 0;
	if (mOffsetT != t)
	{
		mOffsetT = t;
		retval = TEM_CHANGE_TEXTURE;
	}
	return retval;
}

S32 LLTextureEntry::setRotation(F32 theta)
{
	if (mRotation != theta && llfinite(theta))
	{
		mRotation = theta;
		return TEM_CHANGE_TEXTURE;
	}
	return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setBumpShinyFullbright(U8 bump)
{
	if (mBump != bump)
	{
		mBump = bump;
		return TEM_CHANGE_TEXTURE;
	}
	return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setMediaTexGen(U8 media)
{
	S32 result = TEM_CHANGE_NONE;
	result |= setTexGen(media & TEM_TEX_GEN_MASK);
	result |= setMediaFlags(media & TEM_MEDIA_MASK);
	return result;
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
	return TEM_CHANGE_NONE;
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
	return TEM_CHANGE_NONE;
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
	return TEM_CHANGE_NONE;
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
	return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setMediaFlags(U8 media_flags)
{
	media_flags &= TEM_MEDIA_MASK;
	if (getMediaFlags() != media_flags)
	{
		mMediaFlags &= ~TEM_MEDIA_MASK;
		mMediaFlags |= media_flags;
        
		// Special code for media handling
		if( hasMedia() && mMediaEntry == NULL)
		{
			mMediaEntry = new LLMediaEntry;
		}
        else if ( ! hasMedia() && mMediaEntry != NULL)
        {
            delete mMediaEntry;
            mMediaEntry = NULL;
        }
        
		return TEM_CHANGE_MEDIA;
	}
	return TEM_CHANGE_NONE;
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
	return TEM_CHANGE_NONE;
}

S32 LLTextureEntry::setGlow(F32 glow)
{
	if (mGlow != glow)
	{
		mGlow = glow;
		return TEM_CHANGE_TEXTURE;
	}
	return TEM_CHANGE_NONE;
}

void LLTextureEntry::setMediaData(const LLMediaEntry &media_entry)
{
    mMediaFlags |= MF_HAS_MEDIA;
    if (NULL != mMediaEntry)
    {
        delete mMediaEntry;
    }
    mMediaEntry = new LLMediaEntry(media_entry);
}

bool LLTextureEntry::updateMediaData(const LLSD& media_data)
{
	if (media_data.isUndefined())
	{
		// clear the media data
        clearMediaData();
		return false;
	}
	else {
		mMediaFlags |= MF_HAS_MEDIA;
		if (mMediaEntry == NULL)
		{
			mMediaEntry = new LLMediaEntry;
		}
        // *NOTE: this will *clobber* all of the fields in mMediaEntry 
        // with whatever fields are present (or not present) in media_data!
 		mMediaEntry->fromLLSD(media_data);
		return true;
	}
}

void LLTextureEntry::clearMediaData()
{
    mMediaFlags &= ~MF_HAS_MEDIA;
    if (mMediaEntry != NULL) {
        delete mMediaEntry;
    }
    mMediaEntry = NULL;
}    

void LLTextureEntry::mergeIntoMediaData(const LLSD& media_fields)
{
    mMediaFlags |= MF_HAS_MEDIA;
    if (mMediaEntry == NULL)
    {
        mMediaEntry = new LLMediaEntry;
    }
    // *NOTE: this will *merge* the data in media_fields
    // with the data in our media entry
    mMediaEntry->mergeFromLLSD(media_fields);
}

//static
std::string LLTextureEntry::touchMediaVersionString(const std::string &in_version, const LLUUID &agent_id)
{
    // XXX TODO: make media version string binary (base64-encoded?)
    // Media "URL" is a representation of a version and the last-touched agent
    // x-mv:nnnnn/agent-id
    // where "nnnnn" is version number
    // *NOTE: not the most efficient code in the world...
    U32 current_version = getVersionFromMediaVersionString(in_version) + 1;
    const size_t MAX_VERSION_LEN = 10; // 2^32 fits in 10 decimal digits
    char buf[MAX_VERSION_LEN+1];
    snprintf(buf, (int)MAX_VERSION_LEN+1, "%0*u", (int)MAX_VERSION_LEN, current_version);  // added int cast to fix warning/breakage on mac.
    return MEDIA_VERSION_STRING_PREFIX + buf + "/" + agent_id.asString();
}

//static
U32 LLTextureEntry::getVersionFromMediaVersionString(const std::string &version_string)
{
    U32 version = 0;
    if (!version_string.empty()) 
    {
        size_t found = version_string.find(MEDIA_VERSION_STRING_PREFIX);
        if (found != std::string::npos) 
        {
            found = version_string.find_first_of("/", found);
            std::string v = version_string.substr(MEDIA_VERSION_STRING_PREFIX.length(), found);
            version = strtoul(v.c_str(),NULL,10);
        }
    }
    return version;
}

//static
LLUUID LLTextureEntry::getAgentIDFromMediaVersionString(const std::string &version_string)
{
    LLUUID id;
    if (!version_string.empty()) 
    {
        size_t found = version_string.find(MEDIA_VERSION_STRING_PREFIX);
        if (found != std::string::npos) 
        {
            found = version_string.find_first_of("/", found);
            if (found != std::string::npos) 
            {
                std::string v = version_string.substr(found + 1);
                id.set(v);
            }
        }
    }
    return id;
}

//static
bool LLTextureEntry::isMediaVersionString(const std::string &version_string)
{
	return std::string::npos != version_string.find(MEDIA_VERSION_STRING_PREFIX);
}
