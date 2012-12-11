/** 
 * @file llsaleinfo.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include <iostream>
#include "linden_common.h"

#include "llsaleinfo.h"

#include "llerror.h"
#include "message.h"
#include "llsdutil.h"

// use this to avoid temporary object creation
const LLSaleInfo LLSaleInfo::DEFAULT;

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

const char* FOR_SALE_NAMES[] =
{ 
	"not",
	"orig",
	"copy",
	"cntn"
};

///----------------------------------------------------------------------------
/// Class llsaleinfo
///----------------------------------------------------------------------------

// Default constructor
LLSaleInfo::LLSaleInfo() :
	mSaleType(LLSaleInfo::FS_NOT),
	mSalePrice(DEFAULT_PRICE)
{
}

LLSaleInfo::LLSaleInfo(EForSale sale_type, S32 sale_price) :
	mSaleType(sale_type),
	mSalePrice(sale_price)
{
	mSalePrice = llclamp(mSalePrice, 0, S32_MAX);
}

BOOL LLSaleInfo::isForSale() const
{
	return (FS_NOT != mSaleType);
}

U32 LLSaleInfo::getCRC32() const
{
	U32 rv = (U32)mSalePrice;
	rv += (mSaleType * 0x07073096);
	return rv;
}


BOOL LLSaleInfo::exportFile(LLFILE* fp) const
{
	llofstream ofs(fp);
	return exportStream(ofs);
}

BOOL LLSaleInfo::exportStream(std::ostream& output_stream) const
{
	if (!output_stream.good()) return FALSE;
	output_stream << "\tsale_info\t0\n\t{\n";
	output_stream << "\t\tsale_type\t" << lookup(mSaleType) << "\n";
	output_stream << "\t\tsale_price\t" << mSalePrice << "\n";
	output_stream <<"\t}\n";
	return TRUE;
}

LLSD LLSaleInfo::asLLSD() const
{
	LLSD sd = LLSD();
	sd["sale_type"] = lookup(mSaleType);
	sd["sale_price"] = mSalePrice;
	return sd;
}

bool LLSaleInfo::fromLLSD(const LLSD& sd, BOOL& has_perm_mask, U32& perm_mask)
{
	const char *w;

	if (sd["sale_type"].isString())
	{
		mSaleType = lookup(sd["sale_type"].asString().c_str());
	}
	else if(sd["sale_type"].isInteger())
	{
		S8 type = (U8)sd["sale_type"].asInteger();
		mSaleType = static_cast<LLSaleInfo::EForSale>(type);
	}

	mSalePrice = llclamp(sd["sale_price"].asInteger(), 0, S32_MAX);
	w = "perm_mask";
	if (sd.has(w))
	{
		has_perm_mask = TRUE;
		perm_mask = ll_U32_from_sd(sd[w]);
	}
	return true;
}

// Deleted LLSaleInfo::exportFileXML() and LLSaleInfo::importXML()
// because I can't find any non-test code references to it. 2009-05-04 JC

BOOL LLSaleInfo::importFile(LLFILE* fp, BOOL& has_perm_mask, U32& perm_mask)
{
	llifstream ifs(fp);
	return importStream(ifs, has_perm_mask, perm_mask);
}

BOOL LLSaleInfo::importStream(std::istream& input_stream, BOOL& has_perm_mask, U32& perm_mask)
{
	has_perm_mask = FALSE;

	const S32 BUFSIZE = 16384;

	// *NOTE: Changing the buffer size will require changing the scanf
	// calls below.
	char buffer[BUFSIZE];	/* Flawfinder: ignore */
	char keyword[255];	/* Flawfinder: ignore */
	char valuestr[255];	/* Flawfinder: ignore */

	keyword[0] = '\0';
	valuestr[0] = '\0';
	while(input_stream.good())
	{
		input_stream.getline(buffer, MAX_STRING);
		if (input_stream.eof())
		{
			llwarns << "Bad sale info: early end of input stream"
					<< llendl;
			return FALSE;
		}
		if (input_stream.fail())
		{
			llwarns << "Bad sale info: failed to read from input stream"
					<< llendl;
			return FALSE;
		}
		sscanf(	/* Flawfinder: ignore */
			buffer,
			" %254s %254s",
			keyword, valuestr);
		if(!keyword[0])
		{
			continue;
		}
		if(0 == strcmp("{",keyword))
		{
			continue;
		}
		if(0 == strcmp("}", keyword))
		{
			break;
		}
		else if(0 == strcmp("sale_type", keyword))
		{
			mSaleType = lookup(valuestr);
		}
		else if(0 == strcmp("sale_price", keyword))
		{
			sscanf(valuestr, "%d", &mSalePrice);
			mSalePrice = llclamp(mSalePrice, 0, S32_MAX);
		}
		else if (!strcmp("perm_mask", keyword))
		{
			//llinfos << "found deprecated keyword perm_mask" << llendl;
			has_perm_mask = TRUE;
			sscanf(valuestr, "%x", &perm_mask);
		}
		else
		{
			llwarns << "unknown keyword '" << keyword
					<< "' in sale info import" << llendl;
		}
	}
	return TRUE;
}

void LLSaleInfo::setSalePrice(S32 price)
{
	mSalePrice = price;
	mSalePrice = llclamp(mSalePrice, 0, S32_MAX);
}

LLSD LLSaleInfo::packMessage() const
{
	LLSD result;

	U8 sale_type = static_cast<U8>(mSaleType);
	result["sale-type"]		= (U8)sale_type;
	result["sale-price"]	= (S32)mSalePrice;
	//result[_PREHASH_NextOwnerMask] = mNextOwnerPermMask;
	return result;
}

void LLSaleInfo::packMessage(LLMessageSystem* msg) const
{
	U8 sale_type = static_cast<U8>(mSaleType);
	msg->addU8Fast(_PREHASH_SaleType, sale_type);
	msg->addS32Fast(_PREHASH_SalePrice, mSalePrice);
	//msg->addU32Fast(_PREHASH_NextOwnerMask, mNextOwnerPermMask);
}

void LLSaleInfo::unpackMessage(LLSD sales)
{
	U8 sale_type = (U8)sales["sale-type"].asInteger();
	mSaleType = static_cast<EForSale>(sale_type);

	mSalePrice = (S32)sales["sale-price"].asInteger();
	mSalePrice = llclamp(mSalePrice, 0, S32_MAX);
	//msg->getU32Fast(block, _PREHASH_NextOwnerMask, mNextOwnerPermMask);
}

void LLSaleInfo::unpackMessage(LLMessageSystem* msg, const char* block)
{
	U8 sale_type;
	msg->getU8Fast(block, _PREHASH_SaleType, sale_type);
	mSaleType = static_cast<EForSale>(sale_type);
	msg->getS32Fast(block, _PREHASH_SalePrice, mSalePrice);
	mSalePrice = llclamp(mSalePrice, 0, S32_MAX);
	//msg->getU32Fast(block, _PREHASH_NextOwnerMask, mNextOwnerPermMask);
}

void LLSaleInfo::unpackMultiMessage(LLMessageSystem* msg, const char* block,
									S32 block_num)
{
	U8 sale_type;
	msg->getU8Fast(block, _PREHASH_SaleType, sale_type, block_num);
	mSaleType = static_cast<EForSale>(sale_type);
	msg->getS32Fast(block, _PREHASH_SalePrice, mSalePrice, block_num);
	mSalePrice = llclamp(mSalePrice, 0, S32_MAX);
	//msg->getU32Fast(block, _PREHASH_NextOwnerMask, mNextOwnerPermMask, block_num);
}

LLSaleInfo::EForSale LLSaleInfo::lookup(const char* name)
{
	for(S32 i = 0; i < FS_COUNT; i++)
	{
		if(0 == strcmp(name, FOR_SALE_NAMES[i]))
		{
			// match
			return (EForSale)i;
		}
	}
	return FS_NOT;
}

const char* LLSaleInfo::lookup(EForSale type)
{
	if((type >= 0) && (type < FS_COUNT))
	{
		return FOR_SALE_NAMES[S32(type)];
	}
	else
	{
		return NULL;
	}
}

// Allow accumulation of sale info. The price of each is added,
// conflict in sale type results in FS_NOT, and the permissions are
// tightened.
void LLSaleInfo::accumulate(const LLSaleInfo& sale_info)
{
	if(mSaleType != sale_info.mSaleType)
	{
		mSaleType = FS_NOT;
	}
	mSalePrice += sale_info.mSalePrice;
	//mNextOwnerPermMask &= sale_info.mNextOwnerPermMask;
}

bool LLSaleInfo::operator==(const LLSaleInfo &rhs) const
{
	return (
		(mSaleType == rhs.mSaleType) &&
		(mSalePrice == rhs.mSalePrice) 
		);
}

bool LLSaleInfo::operator!=(const LLSaleInfo &rhs) const
{
	return (
		(mSaleType != rhs.mSaleType) ||
		(mSalePrice != rhs.mSalePrice) 
		);
}


///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
/// exported functions
///----------------------------------------------------------------------------
static const std::string ST_TYPE_LABEL("sale_type");
static const std::string ST_PRICE_LABEL("sale_price");

LLSD ll_create_sd_from_sale_info(const LLSaleInfo& sale)
{
	LLSD rv;
	const char* type = LLSaleInfo::lookup(sale.getSaleType());
	if(!type) type = LLSaleInfo::lookup(LLSaleInfo::FS_NOT);
	rv[ST_TYPE_LABEL] = type;
	rv[ST_PRICE_LABEL] = sale.getSalePrice();
	return rv;
}

LLSaleInfo ll_sale_info_from_sd(const LLSD& sd)
{
	LLSaleInfo rv;
	rv.setSaleType(LLSaleInfo::lookup(sd[ST_TYPE_LABEL].asString().c_str()));
	rv.setSalePrice(llclamp((S32)sd[ST_PRICE_LABEL], 0, S32_MAX));
	return rv;
}
