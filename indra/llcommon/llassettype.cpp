/** 
 * @file llassettype.cpp
 * @brief Implementatino of LLAssetType functionality.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include <time.h>

#include "llassettype.h"
#include "lltimer.h"

// I added lookups for exact text of asset type enums in addition to the ones below, so shoot me. -Steve

struct asset_info_t
{
	LLAssetType::EType type;
	const char* desc;
};

asset_info_t asset_types[] =
{
	{ LLAssetType::AT_TEXTURE, "TEXTURE" },
	{ LLAssetType::AT_SOUND, "SOUND" },
	{ LLAssetType::AT_CALLINGCARD, "CALLINGCARD" },
	{ LLAssetType::AT_LANDMARK, "LANDMARK" },
	{ LLAssetType::AT_SCRIPT, "SCRIPT" },
	{ LLAssetType::AT_CLOTHING, "CLOTHING" },
	{ LLAssetType::AT_OBJECT, "OBJECT" },
	{ LLAssetType::AT_NOTECARD, "NOTECARD" },
	{ LLAssetType::AT_CATEGORY, "CATEGORY" },
	{ LLAssetType::AT_ROOT_CATEGORY, "ROOT_CATEGORY" },
	{ LLAssetType::AT_LSL_TEXT, "LSL_TEXT" },
	{ LLAssetType::AT_LSL_BYTECODE, "LSL_BYTECODE" },
	{ LLAssetType::AT_TEXTURE_TGA, "TEXTURE_TGA" },
	{ LLAssetType::AT_BODYPART, "BODYPART" },
	{ LLAssetType::AT_TRASH, "TRASH" },
	{ LLAssetType::AT_SNAPSHOT_CATEGORY, "SNAPSHOT_CATEGORY" },
	{ LLAssetType::AT_LOST_AND_FOUND, "LOST_AND_FOUND" },
	{ LLAssetType::AT_SOUND_WAV, "SOUND_WAV" },
	{ LLAssetType::AT_IMAGE_TGA, "IMAGE_TGA" },
	{ LLAssetType::AT_IMAGE_JPEG, "IMAGE_JPEG" },
	{ LLAssetType::AT_ANIMATION, "ANIMATION" },
	{ LLAssetType::AT_GESTURE, "GESTURE" },
	{ LLAssetType::AT_SIMSTATE, "SIMSTATE" },
	{ LLAssetType::AT_NONE, "NONE" },
};

LLAssetType::EType LLAssetType::getType(const LLString& sin)
{
	LLString s = sin;
	LLString::toUpper(s);
	for (S32 idx = 0; ;idx++)
	{
		asset_info_t* info = asset_types + idx;
		if (info->type == LLAssetType::AT_NONE)
			break;
		if (s == info->desc)
			return info->type;
	}
	return LLAssetType::AT_NONE;
}

LLString LLAssetType::getDesc(LLAssetType::EType type)
{
	for (S32 idx = 0; ;idx++)
	{
		asset_info_t* info = asset_types + idx;
		if (type == info->type)
			return LLString(info->desc);
		if (info->type == LLAssetType::AT_NONE)
			break;
	}
	return LLString("BAD TYPE");
}

//============================================================================

// The asset type names are limited to 8 characters.
// static
const char* LLAssetType::mAssetTypeNames[LLAssetType::AT_COUNT] =
{ 
	"texture",
	"sound",
	"callcard",
	"landmark",
	"script",
	"clothing",
	"object",
	"notecard",
	"category",
	"root",
	"lsltext",
	"lslbyte",
	"txtr_tga",// Intentionally spelled this way.  Limited to eight characters.
	"bodypart",
	"trash",
	"snapshot",
	"lstndfnd",
	"snd_wav",
	"img_tga",
	"jpeg",
	"animatn",
	"gesture",
	"simstate",
};

// This table is meant for decoding to human readable form. Put any
// and as many printable characters you want in each one.
// See also llinventory.cpp INVENTORY_TYPE_HUMAN_NAMES
const char* LLAssetType::mAssetTypeHumanNames[LLAssetType::AT_COUNT] =
{ 
	"texture",
	"sound",
	"calling card",
	"landmark",
	"legacy script",
	"clothing",
	"object",
	"note card",
	"folder",
	"root",
	"lsl2 script",
	"lsl bytecode",
	"tga texture",
	"body part",
	"trash",
	"snapshot",
	"lost and found",
	"sound",
	"targa image",
	"jpeg image",
	"animation",
	"gesture",
	"simstate",
};

///----------------------------------------------------------------------------
/// class LLAssetType
///----------------------------------------------------------------------------

// static
const char* LLAssetType::lookup( LLAssetType::EType type )
{
	if( (type >= 0) && (type < AT_COUNT ))
	{
		return mAssetTypeNames[ S32( type ) ];
	}
	else
	{
		return "-1";
	}
}

// static
LLAssetType::EType LLAssetType::lookup( const char* name )
{
	for( S32 i = 0; i < AT_COUNT; i++ )
	{
		if( 0 == strcmp(name, mAssetTypeNames[i]) )
		{
			// match
			return (EType)i;
		}
	}
	return AT_NONE;
}

// static
const char* LLAssetType::lookupHumanReadable(LLAssetType::EType type)
{
	if( (type >= 0) && (type < AT_COUNT ))
	{
		return mAssetTypeHumanNames[S32(type)];
	}
	else
	{
		return NULL;
	}
}

EDragAndDropType LLAssetType::lookupDragAndDropType( EType asset )
{
	switch( asset )
	{
	case AT_TEXTURE:		return DAD_TEXTURE;
	case AT_SOUND:			return DAD_SOUND;
	case AT_CALLINGCARD:	return DAD_CALLINGCARD;
	case AT_LANDMARK:		return DAD_LANDMARK;
	case AT_SCRIPT:			return DAD_NONE;
	case AT_CLOTHING:		return DAD_CLOTHING;
	case AT_OBJECT:			return DAD_OBJECT;
	case AT_NOTECARD:		return DAD_NOTECARD;
	case AT_CATEGORY:		return DAD_CATEGORY;
	case AT_ROOT_CATEGORY:	return DAD_ROOT_CATEGORY;
	case AT_LSL_TEXT:		return DAD_SCRIPT;
	case AT_BODYPART:		return DAD_BODYPART;
	case AT_ANIMATION:		return DAD_ANIMATION;
	case AT_GESTURE:		return DAD_GESTURE;
	default: 				return DAD_NONE;
	};
}

// static. Generate a good default description
void LLAssetType::generateDescriptionFor(LLAssetType::EType type,
										 LLString& desc)
{
	const S32 BUF_SIZE = 30;
	char time_str[BUF_SIZE];	/* Flawfinder: ignore */
	time_t now;
	time(&now);
	memset(time_str, '\0', BUF_SIZE);
	strftime(time_str, BUF_SIZE - 1, "%Y-%m-%d %H:%M:%S ", localtime(&now));
	desc.assign(time_str);
	desc.append(LLAssetType::lookupHumanReadable(type));
}
