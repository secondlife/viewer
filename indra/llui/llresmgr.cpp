/** 
 * @file llresmgr.cpp
 * @brief Localized resource manager
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// NOTE: this is a MINIMAL implementation.  The interface will remain, but the implementation will
// (when the time is right) become dynamic and probably use external files.

#include "linden_common.h"

#include "llresmgr.h"
#include "llfontgl.h"
#include "llerror.h"
#include "llstring.h"

// Global singleton
LLResMgr* gResMgr = NULL;

LLResMgr::LLResMgr()
{
	U32 i;

	// Init values for each locale.
	// Note: This is only the most bare-bones version.  In the future, load these dynamically, on demand.

	//////////////////////////////////////////////////////////////////////////////
	// USA
	// USA Fonts
	for( i=0; i<LLFONT_COUNT; i++ )
	{
		mUSAFonts[i] = NULL;
	}
	mUSAFonts[ LLFONT_OCRA ]			= LLFontGL::sMonospace;
	mUSAFonts[ LLFONT_SANSSERIF ]		= LLFontGL::sSansSerif;
	mUSAFonts[ LLFONT_SANSSERIF_SMALL ]	= LLFontGL::sSansSerifSmall;
	mUSAFonts[ LLFONT_SANSSERIF_BIG ]	= LLFontGL::sSansSerifBig;
	mUSAFonts[ LLFONT_SMALL ]			= LLFontGL::sMonospace;
/*
	// USA Strings
	for( i=0; i<LLSTR_COUNT; i++ )
	{
		mUSAStrings[i] = "";
	}
	mUSAStrings[ LLSTR_HELLO ]			= "hello";
	mUSAStrings[ LLSTR_GOODBYE ]		= "goodbye";
	mUSAStrings[ LLSTR_CHAT_LABEL ]		= "Chat";
	mUSAStrings[ LLSTR_STATUS_LABEL ]	= "Properties";
	mUSAStrings[ LLSTR_X ]				= "X";
	mUSAStrings[ LLSTR_Y ]				= "Y";
	mUSAStrings[ LLSTR_Z ]				= "Z";
	mUSAStrings[ LLSTR_POSITION ]		= "Position (meters)";
	mUSAStrings[ LLSTR_SCALE ]			= "Size (meters)";
	mUSAStrings[ LLSTR_ROTATION ]		= "Rotation (degrees)";
	mUSAStrings[ LLSTR_HAS_PHYSICS ]	= "Has Physics";
	mUSAStrings[ LLSTR_SCRIPT ]			= "Script";
	mUSAStrings[ LLSTR_HELP ]			= "Help";
	mUSAStrings[ LLSTR_REMOVE ]			= "Remove";
	mUSAStrings[ LLSTR_CLEAR ]			= "Clear";
	mUSAStrings[ LLSTR_APPLY ]			= "Apply";
	mUSAStrings[ LLSTR_CANCEL ]			= "Cancel";
	mUSAStrings[ LLSTR_MATERIAL ]		= "Material";
	mUSAStrings[ LLSTR_FACE ]			= "Face";
	mUSAStrings[ LLSTR_TEXTURE ]		= "Texture";
	mUSAStrings[ LLSTR_TEXTURE_SIZE ]	= "Repeats per Face";
	mUSAStrings[ LLSTR_TEXTURE_OFFSET ]	= "Offset";
	mUSAStrings[ LLSTR_TEXTURE_ROTATION ]	= "Rotation (degrees)";
	mUSAStrings[ LLSTR_U ]				= "U";
	mUSAStrings[ LLSTR_V ]				= "V";
	mUSAStrings[ LLSTR_OWNERSHIP ]		= "Ownership";
	mUSAStrings[ LLSTR_PUBLIC ]			= "Public";
	mUSAStrings[ LLSTR_PRIVATE ]		= "Private";
	mUSAStrings[ LLSTR_REVERT ]			= "Revert";
	mUSAStrings[ LLSTR_INSERT_SAMPLE ]	= "Insert Sample";
	mUSAStrings[ LLSTR_SET_TEXTURE ]	= "Set Texture";
	mUSAStrings[ LLSTR_EDIT_SCRIPT ]	= "Edit Script...";
	mUSAStrings[ LLSTR_MOUSELOOK_INSTRUCTIONS ] = "Press ESC to leave Mouselook.";
	mUSAStrings[ LLSTR_EDIT_FACE_INSTRUCTIONS ] = "Click on face to select part.  Click and hold on a picture to look more like that.  Press ESC to leave Face Edit Mode.";
	mUSAStrings[ LLSTR_CLOSE ]			= "Close";
	mUSAStrings[ LLSTR_MOVE ]			= "Move";
	mUSAStrings[ LLSTR_ROTATE ]			= "Rotate";
	mUSAStrings[ LLSTR_RESIZE ]			= "Resize";
	mUSAStrings[ LLSTR_PLACE_BOX ]		= "Place Box";
	mUSAStrings[ LLSTR_PLACE_PRISM ]	= "Place Prism";
	mUSAStrings[ LLSTR_PLACE_PYRAMID ]	= "Place Pyramid";
	mUSAStrings[ LLSTR_PLACE_TETRAHEDRON ]	= "Place Tetrahedron";
	mUSAStrings[ LLSTR_PLACE_CYLINDER ]	= "Place Cylinder";
	mUSAStrings[ LLSTR_PLACE_HALF_CYLINDER ] = "Place Half-Cylinder";
	mUSAStrings[ LLSTR_PLACE_CONE ]		= "Place Cone";
	mUSAStrings[ LLSTR_PLACE_HALF_CONE ] = "Place Half-Cone";
	mUSAStrings[ LLSTR_PLACE_SPHERE ]	= "Place Sphere";
	mUSAStrings[ LLSTR_PLACE_HALF_SPHERE ] = "Place Half-Sphere";
	mUSAStrings[ LLSTR_PLACE_BIRD ]		= "Place Bird";
	mUSAStrings[ LLSTR_PLACE_SNAKE ]	= "Place Silly Snake";
	mUSAStrings[ LLSTR_PLACE_ROCK ]		= "Place Rock";
	mUSAStrings[ LLSTR_PLACE_TREE ]		= "Place Tree";
	mUSAStrings[ LLSTR_PLACE_GRASS ]	= "Place Grass";
	mUSAStrings[ LLSTR_MODIFY_LAND ]	= "Modify Land";
*/
	//////////////////////////////////////////////////////////////////////////////
	// UK
	// The Brits are a lot like us Americans, so initially assume we're the same and only code the exceptions.

	// UK Fonts
	for( i=0; i<LLFONT_COUNT; i++ )
	{
		mUKFonts[i] = mUSAFonts[i];
	}
/*
	// UK Strings
	for( i=0; i<LLSTR_COUNT; i++ )
	{
		mUKStrings[i] = mUSAStrings[i];
	}
	mUKStrings[ LLSTR_HELLO ]			= "hullo";
	mUKStrings[ LLSTR_GOODBYE ]			= "cheerio";
*/
	//////////////////////////////////////////////////////////////////////////////
	// Set default
	setLocale( LLLOCALE_USA );

}


void LLResMgr::setLocale( LLLOCALE_ID locale_id )
{
	mLocale = locale_id;

	//RN: for now, use normal 'C' locale for everything but specific UI input/output routines
	switch( locale_id )
	{
	case LLLOCALE_USA: 
//#if LL_WINDOWS
//		// Windows doesn't use ISO country codes.
//		llinfos << "Setting locale to " << setlocale( LC_ALL, "english-usa" ) << llendl;
//#else	
//		// posix version should work everywhere else.
//		llinfos << "Setting locale to " << setlocale( LC_ALL, "en_US" ) << llendl;
//#endif

//		mStrings	= mUSAStrings;
		mFonts		= mUSAFonts;
		break;
	case LLLOCALE_UK:
//#if LL_WINDOWS
//		// Windows doesn't use ISO country codes.
//		llinfos << "Setting locale to " << setlocale( LC_ALL, "english-uk" ) << llendl;
//#else
//		// posix version should work everywhere else.
//		llinfos << "Setting locale to " << setlocale( LC_ALL, "en_GB" ) << llendl;
//#endif

//		mStrings	= mUKStrings;
		mFonts		= mUKFonts;
		break;
	default:
		llassert(0);
		setLocale(LLLOCALE_USA);
		break;
	}
}

char LLResMgr::getDecimalPoint() const					
{ 
	char decimal = localeconv()->decimal_point[0]; 

#if LL_DARWIN
	// On the Mac, locale support is broken before 10.4, which causes things to go all pear-shaped.
	if(decimal == 0)
	{
		decimal = '.';
	}
#endif

	return decimal;
}

char LLResMgr::getThousandsSeparator() const			
{
	char separator = localeconv()->thousands_sep[0]; 

#if LL_DARWIN
	// On the Mac, locale support is broken before 10.4, which causes things to go all pear-shaped.
	if(separator == 0)
	{
		separator = ',';
	}
#endif

	return separator;
}

char LLResMgr::getMonetaryDecimalPoint() const
{
	char decimal = localeconv()->mon_decimal_point[0]; 

#if LL_DARWIN
	// On the Mac, locale support is broken before 10.4, which causes things to go all pear-shaped.
	if(decimal == 0)
	{
		decimal = '.';
	}
#endif

	return decimal;
}

char LLResMgr::getMonetaryThousandsSeparator() const	
{
	char separator = localeconv()->mon_thousands_sep[0]; 

#if LL_DARWIN
	// On the Mac, locale support is broken before 10.4, which causes things to go all pear-shaped.
	if(separator == 0)
	{
		separator = ',';
	}
#endif

	return separator;
}


// Sets output to a string of integers with monetary separators inserted according to the locale.
void LLResMgr::getMonetaryString( LLString& output, S32 input ) const
{
	LLLocale locale(LLLocale::USER_LOCALE);
	struct lconv *conv = localeconv();
	
#if LL_DARWIN
	// On the Mac, locale support is broken before 10.4, which causes things to go all pear-shaped.
	// Fake up a conv structure with some reasonable values for the fields this function uses.
	struct lconv fakeconv;
	if(conv->negative_sign[0] == 0)	// Real locales all seem to have something here...
	{
		fakeconv = *conv;	// start with what's there.
		switch(mLocale)
		{
			default:  			// Unknown -- use the US defaults.
			case LLLOCALE_USA: 
			case LLLOCALE_UK:	// UK ends up being the same as US for the items used here.
				fakeconv.negative_sign = "-";
				fakeconv.mon_grouping = "\x03\x03\x00";	// commas every 3 digits
				fakeconv.n_sign_posn = 1; // negative sign before the string
			break;
		}
		conv = &fakeconv;
	}
#endif

	char* negative_sign = conv->negative_sign;
	char separator = getMonetaryThousandsSeparator();
	char* grouping = conv->mon_grouping;
	
	// Note on mon_grouping:
	// Specifies a string that defines the size of each group of digits in formatted monetary quantities.
	// The operand for the mon_grouping keyword consists of a sequence of semicolon-separated integers. 
	// Each integer specifies the number of digits in a group. The initial integer defines the size of
	// the group immediately to the left of the decimal delimiter. The following integers define succeeding
	// groups to the left of the previous group. If the last integer is not -1, the size of the previous
	// group (if any) is repeatedly used for the remainder of the digits. If the last integer is -1, no
	// further grouping is performed. 


	// Note: we assume here that the currency symbol goes on the left. (Hey, it's Lindens! We can just decide.)
	BOOL negative = (input < 0 );
	BOOL negative_before = negative && (conv->n_sign_posn != 2);
	BOOL negative_after = negative && (conv->n_sign_posn == 2);

	LLString digits = llformat("%u", abs(input));
	if( !grouping || !grouping[0] )
	{
		output.assign("L$");
		if( negative_before )
		{
			output.append( negative_sign );
		}
		output.append( digits );
		if( negative_after )
		{
			output.append( negative_sign );
		}
		return;
	}

	S32 groupings[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	S32 cur_group;
	for( cur_group = 0; grouping[ cur_group ]; cur_group++ )
	{
		if( grouping[ cur_group ] != ';' )
		{
			groupings[cur_group] = grouping[ cur_group ];
		}
		cur_group++;

		if( groupings[cur_group] < 0 )
		{
			break;
		}
	}
	S32 group_count = cur_group;

	char reversed_output[20] = "";	/* Flawfinder: ignore */
	char forward_output[20] = "";	/* Flawfinder: ignore */
	S32 output_pos = 0;
	
	cur_group = 0;
	S32 pos = digits.size()-1;
	S32 count_within_group = 0;
	while( (pos >= 0) && (groupings[cur_group] >= 0) )
	{
		count_within_group++;
		if( count_within_group > groupings[cur_group] )
		{
			count_within_group = 1;
			reversed_output[ output_pos++ ] = separator;

			if( (cur_group + 1) >= group_count )
			{
				break;
			}
			else
			if( groupings[cur_group + 1] > 0 )
			{
				cur_group++;
			}
		}
		reversed_output[ output_pos++ ] = digits[pos--];
	}

	while( pos >= 0 )
	{
		reversed_output[ output_pos++ ] = digits[pos--];
	}


	reversed_output[ output_pos ] = '\0';
	forward_output[ output_pos ] = '\0';

	for( S32 i = 0; i < output_pos; i++ )
	{
		forward_output[ output_pos - 1 - i ] = reversed_output[ i ];
	}

	output.assign("L$");
	if( negative_before )
	{
		output.append( negative_sign );
	}
	output.append( forward_output );
	if( negative_after )
	{
		output.append( negative_sign );
	}
}

void LLResMgr::getIntegerString( LLString& output, S32 input ) const
{
	S32 fraction = 0;
	LLString fraction_string;
	S32 remaining_count = input;
	while(remaining_count > 0)
	{
		fraction = (remaining_count) % 1000;
		
		if (!output.empty())
		{
			if (fraction == remaining_count)
			{
				fraction_string = llformat("%d%c", fraction, getThousandsSeparator());
			}
			else
			{
				fraction_string = llformat("%3.3d%c", fraction, getThousandsSeparator());
			}
			output = fraction_string + output;
		}
		else
		{
			if (fraction == remaining_count)
			{
				fraction_string = llformat("%d", fraction);
			}
			else
			{
				fraction_string = llformat("%3.3d", fraction);
			}
			output = fraction_string;
		}
		remaining_count /= 1000;
	}
}

const LLString LLFONT_ID_NAMES[] =
{
	LLString("OCRA"),
	LLString("SANSSERIF"),
	LLString("SANSSERIF_SMALL"),
	LLString("SANSSERIF_BIG"),
	LLString("SMALL"),
};

const LLFontGL* LLResMgr::getRes( LLString font_id ) const
{
	for (S32 i=0; i<LLFONT_COUNT; ++i)
	{
		if (LLFONT_ID_NAMES[i] == font_id)
		{
			return getRes((LLFONT_ID)i);
		}
	}
	return NULL;
}

#if LL_WINDOWS
const LLString LLLocale::USER_LOCALE("English_United States.1252");// = LLString::null;
const LLString LLLocale::SYSTEM_LOCALE("English_United States.1252");
#elif LL_DARWIN
const LLString LLLocale::USER_LOCALE("en_US.iso8859-1");// = LLString::null;
const LLString LLLocale::SYSTEM_LOCALE("en_US.iso8859-1");
#else // LL_LINUX likes this
const LLString LLLocale::USER_LOCALE("en_US.utf8");
const LLString LLLocale::SYSTEM_LOCALE("C");
#endif


LLLocale::LLLocale(const LLString& locale_string)
{
	mPrevLocaleString = setlocale( LC_ALL, NULL );
	char* new_locale_string = setlocale( LC_ALL, locale_string.c_str());
	if ( new_locale_string == NULL)
	{
		llwarns << "Failed to set locale " << locale_string << llendl;
		setlocale(LC_ALL, SYSTEM_LOCALE.c_str());
	}
	//else
	//{
	//	llinfos << "Set locale to " << new_locale_string << llendl;
	//}
}

LLLocale::~LLLocale() 
{
	setlocale( LC_ALL, mPrevLocaleString.c_str() );
}
