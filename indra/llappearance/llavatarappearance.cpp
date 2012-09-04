/** 
 * @File llavatarappearance.cpp
 * @brief Implementation of LLAvatarAppearance class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#include "llavatarappearance.h"
#include "lldeleteutils.h"
#include "lltexglobalcolor.h"

const LLColor4 DUMMY_COLOR = LLColor4(0.5,0.5,0.5,1.0);

LLAvatarAppearance::LLAvatarAppearance() :
	LLCharacter(),
	mTexSkinColor( NULL ),
	mTexHairColor( NULL ),
	mTexEyeColor( NULL ),
	mIsDummy(FALSE)
{
}

// virtual
LLAvatarAppearance::~LLAvatarAppearance()
{
	deleteAndClear(mTexSkinColor);
	deleteAndClear(mTexHairColor);
	deleteAndClear(mTexEyeColor);
}

using namespace LLAvatarAppearanceDefines;

//static
BOOL LLAvatarAppearance::teToColorParams( ETextureIndex te, U32 *param_name )
{
	switch( te )
	{
		case TEX_UPPER_SHIRT:
			param_name[0] = 803; //"shirt_red";
			param_name[1] = 804; //"shirt_green";
			param_name[2] = 805; //"shirt_blue";
			break;

		case TEX_LOWER_PANTS:
			param_name[0] = 806; //"pants_red";
			param_name[1] = 807; //"pants_green";
			param_name[2] = 808; //"pants_blue";
			break;

		case TEX_LOWER_SHOES:
			param_name[0] = 812; //"shoes_red";
			param_name[1] = 813; //"shoes_green";
			param_name[2] = 817; //"shoes_blue";
			break;

		case TEX_LOWER_SOCKS:
			param_name[0] = 818; //"socks_red";
			param_name[1] = 819; //"socks_green";
			param_name[2] = 820; //"socks_blue";
			break;

		case TEX_UPPER_JACKET:
		case TEX_LOWER_JACKET:
			param_name[0] = 834; //"jacket_red";
			param_name[1] = 835; //"jacket_green";
			param_name[2] = 836; //"jacket_blue";
			break;

		case TEX_UPPER_GLOVES:
			param_name[0] = 827; //"gloves_red";
			param_name[1] = 829; //"gloves_green";
			param_name[2] = 830; //"gloves_blue";
			break;

		case TEX_UPPER_UNDERSHIRT:
			param_name[0] = 821; //"undershirt_red";
			param_name[1] = 822; //"undershirt_green";
			param_name[2] = 823; //"undershirt_blue";
			break;
	
		case TEX_LOWER_UNDERPANTS:
			param_name[0] = 824; //"underpants_red";
			param_name[1] = 825; //"underpants_green";
			param_name[2] = 826; //"underpants_blue";
			break;

		case TEX_SKIRT:
			param_name[0] = 921; //"skirt_red";
			param_name[1] = 922; //"skirt_green";
			param_name[2] = 923; //"skirt_blue";
			break;

		case TEX_HEAD_TATTOO:
		case TEX_LOWER_TATTOO:
		case TEX_UPPER_TATTOO:
			param_name[0] = 1071; //"tattoo_red";
			param_name[1] = 1072; //"tattoo_green";
			param_name[2] = 1073; //"tattoo_blue";
			break;	

		default:
			llassert(0);
			return FALSE;
	}

	return TRUE;
}

void LLAvatarAppearance::setClothesColor( ETextureIndex te, const LLColor4& new_color, BOOL upload_bake )
{
	U32 param_name[3];
	if( teToColorParams( te, param_name ) )
	{
		setVisualParamWeight( param_name[0], new_color.mV[VX], upload_bake );
		setVisualParamWeight( param_name[1], new_color.mV[VY], upload_bake );
		setVisualParamWeight( param_name[2], new_color.mV[VZ], upload_bake );
	}
}

LLColor4 LLAvatarAppearance::getClothesColor( ETextureIndex te )
{
	LLColor4 color;
	U32 param_name[3];
	if( teToColorParams( te, param_name ) )
	{
		color.mV[VX] = getVisualParamWeight( param_name[0] );
		color.mV[VY] = getVisualParamWeight( param_name[1] );
		color.mV[VZ] = getVisualParamWeight( param_name[2] );
	}
	return color;
}

// static
LLColor4 LLAvatarAppearance::getDummyColor()
{
	return DUMMY_COLOR;
}

LLColor4 LLAvatarAppearance::getGlobalColor( const std::string& color_name ) const
{
	if (color_name=="skin_color" && mTexSkinColor)
	{
		return mTexSkinColor->getColor();
	}
	else if(color_name=="hair_color" && mTexHairColor)
	{
		return mTexHairColor->getColor();
	}
	if(color_name=="eye_color" && mTexEyeColor)
	{
		return mTexEyeColor->getColor();
	}
	else
	{
//		return LLColor4( .5f, .5f, .5f, .5f );
		return LLColor4( 0.f, 1.f, 1.f, 1.f ); // good debugging color
	}
}



