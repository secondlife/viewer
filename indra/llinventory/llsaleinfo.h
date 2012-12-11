/** 
 * @file llsaleinfo.h
 * @brief LLSaleInfo class header file.
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

#ifndef LL_LLSALEINFO_H
#define LL_LLSALEINFO_H

#include "llpermissionsflags.h"
#include "llsd.h"
#include "llxmlnode.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLSaleInfo
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// L$ default price for objects
const S32 DEFAULT_PRICE = 10;

class LLMessageSystem;

class LLSaleInfo
{
public:
	// use this to avoid temporary object creation
	static const LLSaleInfo DEFAULT;

	enum EForSale
	{
		// item is not to be considered for transactions
		FS_NOT = 0,

		// the origional is on sale
		FS_ORIGINAL = 1,

		// A copy is for sale
		FS_COPY = 2,

		// Valid only for tasks, the inventory is for sale
		// at the price in this structure.
		FS_CONTENTS = 3,

		FS_COUNT
	};

protected:
	EForSale mSaleType;
	S32 mSalePrice;

public:
	// default constructor is fine usually
	LLSaleInfo();
	LLSaleInfo(EForSale sale_type, S32 sale_price);

	// accessors
	BOOL isForSale() const;
	EForSale getSaleType() const { return mSaleType; }
	S32 getSalePrice() const { return mSalePrice; }
	U32 getCRC32() const;

	// mutators
	void setSaleType(EForSale type)		{ mSaleType = type; }
	void setSalePrice(S32 price);
	//void setNextOwnerPermMask(U32 mask)	{ mNextOwnerPermMask = mask; }


	// file serialization
	BOOL exportFile(LLFILE* fp) const;
	BOOL importFile(LLFILE* fp, BOOL& has_perm_mask, U32& perm_mask);

	BOOL exportStream(std::ostream& output_stream) const;
	LLSD asLLSD() const;
	operator LLSD() const { return asLLSD(); }
	bool fromLLSD(const LLSD& sd, BOOL& has_perm_mask, U32& perm_mask);
	BOOL importStream(std::istream& input_stream, BOOL& has_perm_mask, U32& perm_mask);

	LLSD packMessage() const;
	void unpackMessage(LLSD sales);

	// message serialization
	void packMessage(LLMessageSystem* msg) const;
	void unpackMessage(LLMessageSystem* msg, const char* block);
	void unpackMultiMessage(LLMessageSystem* msg, const char* block,
							S32 block_num);

	// static functionality for determine for sale status.
	static EForSale lookup(const char* name);
	static const char* lookup(EForSale type);

	// Allow accumulation of sale info. The price of each is added,
	// conflict in sale type results in FS_NOT, and the permissions
	// are tightened.
	void accumulate(const LLSaleInfo& sale_info);

	bool operator==(const LLSaleInfo &rhs) const;
	bool operator!=(const LLSaleInfo &rhs) const;
};

// These functions convert between structured data and sale info as
// appropriate for serialization.
LLSD ll_create_sd_from_sale_info(const LLSaleInfo& sale);
LLSaleInfo ll_sale_info_from_sd(const LLSD& sd);

#endif // LL_LLSALEINFO_H
