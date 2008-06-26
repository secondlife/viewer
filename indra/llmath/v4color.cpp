/** 
 * @file v4color.cpp
 * @brief LLColor4 class implementation.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2007, Linden Research, Inc.
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

#include "llboost.h"

#include "v4color.h"
#include "v4coloru.h"
#include "v3color.h"
#include "v4math.h"
#include "llmath.h"

// LLColor4

//////////////////////////////////////////////////////////////////////////////

LLColor4 LLColor4::red(		1.f, 0.f, 0.f, 1.f);
LLColor4 LLColor4::green(	0.f, 1.f, 0.f, 1.f);
LLColor4 LLColor4::blue(	0.f, 0.f, 1.f, 1.f);
LLColor4 LLColor4::black(	0.f, 0.f, 0.f, 1.f);
LLColor4 LLColor4::yellow(	1.f, 1.f, 0.f, 1.f);
LLColor4 LLColor4::magenta( 1.0f, 0.0f, 1.0f, 1.0f);
LLColor4 LLColor4::cyan(	0.0f, 1.0f, 1.0f, 1.0f);
LLColor4 LLColor4::white(	1.f, 1.f, 1.f, 1.f);
LLColor4 LLColor4::smoke(	0.5f, 0.5f, 0.5f, 0.5f);
LLColor4 LLColor4::grey(	0.5f, 0.5f, 0.5f, 1.0f);
LLColor4 LLColor4::orange(	1.f, 0.5, 0.f, 1.f );
LLColor4 LLColor4::purple(	0.6f, 0.2f, 0.8f, 1.0f);
LLColor4 LLColor4::pink(	1.0f, 0.5f, 0.8f, 1.0f);
LLColor4 LLColor4::transparent(	0.f, 0.f, 0.f, 0.f );

//////////////////////////////////////////////////////////////////////////////

LLColor4 LLColor4::grey1(0.8f, 0.8f, 0.8f, 1.0f);
LLColor4 LLColor4::grey2(0.6f, 0.6f, 0.6f, 1.0f);
LLColor4 LLColor4::grey3(0.4f, 0.4f, 0.4f, 1.0f);
LLColor4 LLColor4::grey4(0.3f, 0.3f, 0.3f, 1.0f);

LLColor4 LLColor4::red1(1.0f, 0.0f, 0.0f, 1.0f);
LLColor4 LLColor4::red2(0.6f, 0.0f, 0.0f, 1.0f);
LLColor4 LLColor4::red3(1.0f, 0.2f, 0.2f, 1.0f);
LLColor4 LLColor4::red4(0.5f, 0.1f, 0.1f, 1.0f);
LLColor4 LLColor4::red5(0.8f, 0.1f, 0.0f, 1.0f);

LLColor4 LLColor4::green1(0.0f, 1.0f, 0.0f, 1.0f);
LLColor4 LLColor4::green2(0.0f, 0.6f, 0.0f, 1.0f);
LLColor4 LLColor4::green3(0.0f, 0.4f, 0.0f, 1.0f);
LLColor4 LLColor4::green4(0.0f, 1.0f, 0.4f, 1.0f);
LLColor4 LLColor4::green5(0.2f, 0.6f, 0.4f, 1.0f);
LLColor4 LLColor4::green6(0.4f, 0.6f, 0.2f, 1.0f);

LLColor4 LLColor4::blue1(0.0f, 0.0f, 1.0f, 1.0f);
LLColor4 LLColor4::blue2(0.0f, 0.4f, 1.0f, 1.0f);
LLColor4 LLColor4::blue3(0.2f, 0.2f, 0.8f, 1.0f);
LLColor4 LLColor4::blue4(0.0f, 0.0f, 0.6f, 1.0f);
LLColor4 LLColor4::blue5(0.4f, 0.2f, 1.0f, 1.0f);
LLColor4 LLColor4::blue6(0.4f, 0.5f, 1.0f, 1.0f);

LLColor4 LLColor4::yellow1(1.0f, 1.0f, 0.0f, 1.0f);
LLColor4 LLColor4::yellow2(0.6f, 0.6f, 0.0f, 1.0f);
LLColor4 LLColor4::yellow3(0.8f, 1.0f, 0.2f, 1.0f);
LLColor4 LLColor4::yellow4(1.0f, 1.0f, 0.4f, 1.0f);
LLColor4 LLColor4::yellow5(0.6f, 0.4f, 0.2f, 1.0f);
LLColor4 LLColor4::yellow6(1.0f, 0.8f, 0.4f, 1.0f);
LLColor4 LLColor4::yellow7(0.8f, 0.8f, 0.0f, 1.0f);
LLColor4 LLColor4::yellow8(0.8f, 0.8f, 0.2f, 1.0f);
LLColor4 LLColor4::yellow9(0.8f, 0.8f, 0.4f, 1.0f);

LLColor4 LLColor4::orange1(1.0f, 0.8f, 0.0f, 1.0f);
LLColor4 LLColor4::orange2(1.0f, 0.6f, 0.0f, 1.0f);
LLColor4 LLColor4::orange3(1.0f, 0.4f, 0.2f, 1.0f);
LLColor4 LLColor4::orange4(0.8f, 0.4f, 0.0f, 1.0f);
LLColor4 LLColor4::orange5(0.9f, 0.5f, 0.0f, 1.0f);
LLColor4 LLColor4::orange6(1.0f, 0.8f, 0.2f, 1.0f);

LLColor4 LLColor4::magenta1(1.0f, 0.0f, 1.0f, 1.0f);
LLColor4 LLColor4::magenta2(0.6f, 0.2f, 0.4f, 1.0f);
LLColor4 LLColor4::magenta3(1.0f, 0.4f, 0.6f, 1.0f);
LLColor4 LLColor4::magenta4(1.0f, 0.2f, 0.8f, 1.0f);

LLColor4 LLColor4::purple1(0.6f, 0.2f, 0.8f, 1.0f);
LLColor4 LLColor4::purple2(0.8f, 0.2f, 1.0f, 1.0f);
LLColor4 LLColor4::purple3(0.6f, 0.0f, 1.0f, 1.0f);
LLColor4 LLColor4::purple4(0.4f, 0.0f, 0.8f, 1.0f);
LLColor4 LLColor4::purple5(0.6f, 0.0f, 0.8f, 1.0f);
LLColor4 LLColor4::purple6(0.8f, 0.0f, 0.6f, 1.0f);

LLColor4 LLColor4::pink1(1.0f, 0.5f, 0.8f, 1.0f);
LLColor4 LLColor4::pink2(1.0f, 0.8f, 0.9f, 1.0f);

LLColor4 LLColor4::cyan1(0.0f, 1.0f, 1.0f, 1.0f);
LLColor4 LLColor4::cyan2(0.4f, 0.8f, 0.8f, 1.0f);
LLColor4 LLColor4::cyan3(0.0f, 1.0f, 0.6f, 1.0f);
LLColor4 LLColor4::cyan4(0.6f, 1.0f, 1.0f, 1.0f);
LLColor4 LLColor4::cyan5(0.2f, 0.6f, 1.0f, 1.0f);
LLColor4 LLColor4::cyan6(0.2f, 0.6f, 0.6f, 1.0f);

//////////////////////////////////////////////////////////////////////////////

// conversion
LLColor4::operator const LLColor4U() const
{
	return LLColor4U(
		(U8)llclampb(llround(mV[VRED]*255.f)),
		(U8)llclampb(llround(mV[VGREEN]*255.f)),
		(U8)llclampb(llround(mV[VBLUE]*255.f)),
		(U8)llclampb(llround(mV[VALPHA]*255.f)));
}

LLColor4::LLColor4(const LLColor3 &vec, F32 a)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = a;
}

LLColor4::LLColor4(const LLColor4U& color4u)
{
	const F32 SCALE = 1.f/255.f;
	mV[VX] = color4u.mV[VX] * SCALE;
	mV[VY] = color4u.mV[VY] * SCALE;
	mV[VZ] = color4u.mV[VZ] * SCALE;
	mV[VW] = color4u.mV[VW] * SCALE;
}

LLColor4::LLColor4(const LLVector4& vector4)
{
	mV[VX] = vector4.mV[VX];
	mV[VY] = vector4.mV[VY];
	mV[VZ] = vector4.mV[VZ];
	mV[VW] = vector4.mV[VW];
}

const LLColor4&	LLColor4::setVec(const LLColor4U& color4u)
{
	const F32 SCALE = 1.f/255.f;
	mV[VX] = color4u.mV[VX] * SCALE;
	mV[VY] = color4u.mV[VY] * SCALE;
	mV[VZ] = color4u.mV[VZ] * SCALE;
	mV[VW] = color4u.mV[VW] * SCALE;
	return (*this);
}

const LLColor4&	LLColor4::setVec(const LLColor3 &vec)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];

//  no change to alpha!
//	mV[VW] = 1.f;  

	return (*this);
}

const LLColor4&	LLColor4::setVec(const LLColor3 &vec, F32 a)
{
	mV[VX] = vec.mV[VX];
	mV[VY] = vec.mV[VY];
	mV[VZ] = vec.mV[VZ];
	mV[VW] = a;
	return (*this);
}

const LLColor4& LLColor4::operator=(const LLColor3 &a)
{
	mV[VX] = a.mV[VX];
	mV[VY] = a.mV[VY];
	mV[VZ] = a.mV[VZ];

// converting from an rgb sets a=1 (opaque)
	mV[VW] = 1.f;
	return (*this);
}


std::ostream& operator<<(std::ostream& s, const LLColor4 &a) 
{
	s << "{ " << a.mV[VX] << ", " << a.mV[VY] << ", " << a.mV[VZ] << ", " << a.mV[VW] << " }";
	return s;
}

bool operator==(const LLColor4 &a, const LLColor3 &b)
{
	return (  (a.mV[VX] == b.mV[VX])
			&&(a.mV[VY] == b.mV[VY])
			&&(a.mV[VZ] == b.mV[VZ]));
}

bool operator!=(const LLColor4 &a, const LLColor3 &b)
{
	return (  (a.mV[VX] != b.mV[VX])
			||(a.mV[VY] != b.mV[VY])
			||(a.mV[VZ] != b.mV[VZ]));
}

LLColor3	vec4to3(const LLColor4 &vec)
{
	LLColor3	temp(vec.mV[VX], vec.mV[VY], vec.mV[VZ]);
	return temp;
}

LLColor4	vec3to4(const LLColor3 &vec)
{
	LLColor3	temp(vec.mV[VX], vec.mV[VY], vec.mV[VZ]);
	return temp;
}

void LLColor4::calcHSL(F32* hue, F32* saturation, F32* luminance) const
{
	F32 var_R = mV[VRED];
	F32 var_G = mV[VGREEN];
	F32 var_B = mV[VBLUE];

	F32 var_Min = ( var_R < ( var_G < var_B ? var_G : var_B ) ? var_R : ( var_G < var_B ? var_G : var_B ) );
	F32 var_Max = ( var_R > ( var_G > var_B ? var_G : var_B ) ? var_R : ( var_G > var_B ? var_G : var_B ) );

	F32 del_Max = var_Max - var_Min;

	F32 L = ( var_Max + var_Min ) / 2.0f;
	F32 H = 0.0f;
	F32 S = 0.0f;

	if ( del_Max == 0.0f )
	{
	   H = 0.0f;
	   S = 0.0f;
	}
	else
	{
		if ( L < 0.5 )
			S = del_Max / ( var_Max + var_Min );
		else
			S = del_Max / ( 2.0f - var_Max - var_Min );

		F32 del_R = ( ( ( var_Max - var_R ) / 6.0f ) + ( del_Max / 2.0f ) ) / del_Max;
		F32 del_G = ( ( ( var_Max - var_G ) / 6.0f ) + ( del_Max / 2.0f ) ) / del_Max;
		F32 del_B = ( ( ( var_Max - var_B ) / 6.0f ) + ( del_Max / 2.0f ) ) / del_Max;

		if ( var_R >= var_Max )
			H = del_B - del_G;
		else
		if ( var_G >= var_Max )
			H = ( 1.0f / 3.0f ) + del_R - del_B;
		else
		if ( var_B >= var_Max )
			H = ( 2.0f / 3.0f ) + del_G - del_R;

		if ( H < 0.0f ) H += 1.0f;
		if ( H > 1.0f ) H -= 1.0f;
	}

	if (hue) *hue = H;
	if (saturation) *saturation = S;
	if (luminance) *luminance = L;
}

// static
BOOL LLColor4::parseColor(const std::string& buf, LLColor4* color)
{
	if( buf.empty() || color == NULL)
	{
		return FALSE;
	}

	boost_tokenizer tokens(buf, boost::char_separator<char>(", "));
	boost_tokenizer::iterator token_iter = tokens.begin();
	if (token_iter == tokens.end())
	{
		return FALSE;
	}

	// Grab the first token into a string, since we don't know
	// if this is a float or a color name.
	std::string color_name( (*token_iter) );
	++token_iter;

	if (token_iter != tokens.end())
	{
		// There are more tokens to read.  This must be a vector.
		LLColor4 v;
		LLStringUtil::convertToF32( color_name,  v.mV[VX] );
		LLStringUtil::convertToF32( *token_iter, v.mV[VY] );
		v.mV[VZ] = 0.0f;
		v.mV[VW] = 1.0f;

		++token_iter;
		if (token_iter == tokens.end())
		{
			// This is a malformed vector.
			llwarns << "LLColor4::parseColor() malformed color " << buf << llendl;
		}
		else
		{
			// There is a z-component.
			LLStringUtil::convertToF32( *token_iter, v.mV[VZ] );

			++token_iter;
			if (token_iter != tokens.end())
			{
				// There is an alpha component.
				LLStringUtil::convertToF32( *token_iter, v.mV[VW] );
			}
		}

		//  Make sure all values are between 0 and 1.
		if (v.mV[VX] > 1.f || v.mV[VY] > 1.f || v.mV[VZ] > 1.f || v.mV[VW] > 1.f)
		{
			v = v * (1.f / 255.f);
		}
		color->setVec( v );
	}
	else // Single value.  Read as a named color.
	{
		// We have a color name
		if ( "red" == color_name )
		{
			color->setVec(LLColor4::red);
		}
		else if ( "red1" == color_name )
		{
			color->setVec(LLColor4::red1);
		}
		else if ( "red2" == color_name )
		{
			color->setVec(LLColor4::red2);
		}
		else if ( "red3" == color_name )
		{
			color->setVec(LLColor4::red3);
		}
		else if ( "red4" == color_name )
		{
			color->setVec(LLColor4::red4);
		}
		else if ( "red5" == color_name )
		{
			color->setVec(LLColor4::red5);
		}
		else if( "green" == color_name )
		{
			color->setVec(LLColor4::green);
		}
		else if( "green1" == color_name )
		{
			color->setVec(LLColor4::green1);
		}
		else if( "green2" == color_name )
		{
			color->setVec(LLColor4::green2);
		}
		else if( "green3" == color_name )
		{
			color->setVec(LLColor4::green3);
		}
		else if( "green4" == color_name )
		{
			color->setVec(LLColor4::green4);
		}
		else if( "green5" == color_name )
		{
			color->setVec(LLColor4::green5);
		}
		else if( "green6" == color_name )
		{
			color->setVec(LLColor4::green6);
		}
		else if( "blue" == color_name )
		{
			color->setVec(LLColor4::blue);
		}
		else if( "blue1" == color_name )
		{
			color->setVec(LLColor4::blue1);
		}
		else if( "blue2" == color_name )
		{
			color->setVec(LLColor4::blue2);
		}
		else if( "blue3" == color_name )
		{
			color->setVec(LLColor4::blue3);
		}
		else if( "blue4" == color_name )
		{
			color->setVec(LLColor4::blue4);
		}
		else if( "blue5" == color_name )
		{
			color->setVec(LLColor4::blue5);
		}
		else if( "blue6" == color_name )
		{
			color->setVec(LLColor4::blue6);
		}
		else if( "black" == color_name )
		{
			color->setVec(LLColor4::black);
		}
		else if( "white" == color_name )
		{
			color->setVec(LLColor4::white);
		}
		else if( "yellow" == color_name )
		{
			color->setVec(LLColor4::yellow);
		}
		else if( "yellow1" == color_name )
		{
			color->setVec(LLColor4::yellow1);
		}
		else if( "yellow2" == color_name )
		{
			color->setVec(LLColor4::yellow2);
		}
		else if( "yellow3" == color_name )
		{
			color->setVec(LLColor4::yellow3);
		}
		else if( "yellow4" == color_name )
		{
			color->setVec(LLColor4::yellow4);
		}
		else if( "yellow5" == color_name )
		{
			color->setVec(LLColor4::yellow5);
		}
		else if( "yellow6" == color_name )
		{
			color->setVec(LLColor4::yellow6);
		}
		else if( "magenta" == color_name )
		{
			color->setVec(LLColor4::magenta);
		}
		else if( "magenta1" == color_name )
		{
			color->setVec(LLColor4::magenta1);
		}
		else if( "magenta2" == color_name )
		{
			color->setVec(LLColor4::magenta2);
		}
		else if( "magenta3" == color_name )
		{
			color->setVec(LLColor4::magenta3);
		}
		else if( "magenta4" == color_name )
		{
			color->setVec(LLColor4::magenta4);
		}
		else if( "purple" == color_name )
		{
			color->setVec(LLColor4::purple);
		}
		else if( "purple1" == color_name )
		{
			color->setVec(LLColor4::purple1);
		}
		else if( "purple2" == color_name )
		{
			color->setVec(LLColor4::purple2);
		}
		else if( "purple3" == color_name )
		{
			color->setVec(LLColor4::purple3);
		}
		else if( "purple4" == color_name )
		{
			color->setVec(LLColor4::purple4);
		}
		else if( "purple5" == color_name )
		{
			color->setVec(LLColor4::purple5);
		}
		else if( "purple6" == color_name )
		{
			color->setVec(LLColor4::purple6);
		}
		else if( "pink" == color_name )
		{
			color->setVec(LLColor4::pink);
		}
		else if( "pink1" == color_name )
		{
			color->setVec(LLColor4::pink1);
		}
		else if( "pink2" == color_name )
		{
			color->setVec(LLColor4::pink2);
		}
		else if( "cyan" == color_name )
		{
			color->setVec(LLColor4::cyan);
		}
		else if( "cyan1" == color_name )
		{
			color->setVec(LLColor4::cyan1);
		}
		else if( "cyan2" == color_name )
		{
			color->setVec(LLColor4::cyan2);
		}
		else if( "cyan3" == color_name )
		{
			color->setVec(LLColor4::cyan3);
		}
		else if( "cyan4" == color_name )
		{
			color->setVec(LLColor4::cyan4);
		}
		else if( "cyan5" == color_name )
		{
			color->setVec(LLColor4::cyan5);
		}
		else if( "cyan6" == color_name )
		{
			color->setVec(LLColor4::cyan6);
		}
		else if( "smoke" == color_name )
		{
			color->setVec(LLColor4::smoke);
		}
		else if( "grey" == color_name )
		{
			color->setVec(LLColor4::grey);
		}
		else if( "grey1" == color_name )
		{
			color->setVec(LLColor4::grey1);
		}
		else if( "grey2" == color_name )
		{
			color->setVec(LLColor4::grey2);
		}
		else if( "grey3" == color_name )
		{
			color->setVec(LLColor4::grey3);
		}
		else if( "grey4" == color_name )
		{
			color->setVec(LLColor4::grey4);
		}
		else if( "orange" == color_name )
		{
			color->setVec(LLColor4::orange);
		}
		else if( "orange1" == color_name )
		{
			color->setVec(LLColor4::orange1);
		}
		else if( "orange2" == color_name )
		{
			color->setVec(LLColor4::orange2);
		}
		else if( "orange3" == color_name )
		{
			color->setVec(LLColor4::orange3);
		}
		else if( "orange4" == color_name )
		{
			color->setVec(LLColor4::orange4);
		}
		else if( "orange5" == color_name )
		{
			color->setVec(LLColor4::orange5);
		}
		else if( "orange6" == color_name )
		{
			color->setVec(LLColor4::orange6);
		}
		else if ( "clear" == color_name )
		{
			color->setVec(0.f, 0.f, 0.f, 0.f);
		}
		else
		{
			llwarns << "invalid color " << color_name << llendl;
		}
	}

	return TRUE;
}

// static
BOOL LLColor4::parseColor4(const std::string& buf, LLColor4* value)
{
	if( buf.empty() || value == NULL)
	{
		return FALSE;
	}

	LLColor4 v;
	S32 count = sscanf( buf.c_str(), "%f, %f, %f, %f", v.mV + 0, v.mV + 1, v.mV + 2, v.mV + 3 );
	if (1 == count )
	{
		// try this format
		count = sscanf( buf.c_str(), "%f %f %f %f", v.mV + 0, v.mV + 1, v.mV + 2, v.mV + 3 );
	}
	if( 4 == count )
	{
		value->setVec( v );
		return TRUE;
	}

	return FALSE;
}

// EOF
