/** 
 * @file lleconomy.h
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#ifndef LL_LLECONOMY_H
#define LL_LLECONOMY_H

#include "llmemory.h"

class LLMessageSystem;
class LLVector3;

class LLGlobalEconomy
{
public:
	LLGlobalEconomy();
	virtual ~LLGlobalEconomy();

	// This class defines its singleton internally as a typedef instead of inheriting from
	// LLSingleton like most others because the LLRegionEconomy sub-class might also
	// become a singleton and this pattern will more easily disambiguate them.
	typedef LLSingleton<LLGlobalEconomy> Singleton;

	virtual void print();

	static void processEconomyData(LLMessageSystem *msg, LLGlobalEconomy* econ_data);

	S32		calculateTeleportCost(F32 distance) const;
	S32		calculateLightRent(const LLVector3& object_size) const;

	S32		getObjectCount() const				{ return mObjectCount; }
	S32		getObjectCapacity() const			{ return mObjectCapacity; }
	S32		getPriceObjectClaim() const			{ return mPriceObjectClaim; }
	S32		getPricePublicObjectDecay() const	{ return mPricePublicObjectDecay; }
	S32		getPricePublicObjectDelete() const	{ return mPricePublicObjectDelete; }
	S32		getPricePublicObjectRelease() const	{ return mPriceObjectClaim - mPricePublicObjectDelete; }
	S32		getPriceEnergyUnit() const			{ return mPriceEnergyUnit; }
	S32		getPriceUpload() const				{ return mPriceUpload; }
	S32		getPriceRentLight() const			{ return mPriceRentLight; }
	S32		getTeleportMinPrice() const			{ return mTeleportMinPrice; }
	F32		getTeleportPriceExponent() const 	{ return mTeleportPriceExponent; }
	S32		getPriceGroupCreate() const			{ return mPriceGroupCreate; }


	void	setObjectCount(S32 val)				{ mObjectCount = val; }
	void	setObjectCapacity(S32 val)			{ mObjectCapacity = val; }
	void	setPriceObjectClaim(S32 val)		{ mPriceObjectClaim = val; }
	void	setPricePublicObjectDecay(S32 val)	{ mPricePublicObjectDecay = val; }
	void	setPricePublicObjectDelete(S32 val)	{ mPricePublicObjectDelete = val; }
	void	setPriceEnergyUnit(S32 val)			{ mPriceEnergyUnit = val; }
	void	setPriceUpload(S32 val)				{ mPriceUpload = val; }
	void	setPriceRentLight(S32 val)			{ mPriceRentLight = val; }
	void	setTeleportMinPrice(S32 val)		{ mTeleportMinPrice = val; }
	void	setTeleportPriceExponent(F32 val) 	{ mTeleportPriceExponent = val; }
	void	setPriceGroupCreate(S32 val)		{ mPriceGroupCreate = val; }

private:
	S32		mObjectCount;
	S32		mObjectCapacity;
	S32		mPriceObjectClaim;			// per primitive
	S32		mPricePublicObjectDecay;	// per primitive
	S32		mPricePublicObjectDelete;	// per primitive
	S32		mPriceEnergyUnit;
	S32		mPriceUpload;
	S32		mPriceRentLight;
	S32		mTeleportMinPrice;
	F32		mTeleportPriceExponent;
	S32     mPriceGroupCreate;
};


class LLRegionEconomy : public LLGlobalEconomy
{
public:
	LLRegionEconomy();
	~LLRegionEconomy();

	static void processEconomyData(LLMessageSystem *msg, void **user_data);
	static void processEconomyDataRequest(LLMessageSystem *msg, void **user_data);

	void print();

	BOOL	hasData() const;
	F32		getPriceObjectRent() const	{ return mPriceObjectRent; }
	F32		getPriceObjectScaleFactor() const {return mPriceObjectScaleFactor;}
	F32		getEnergyEfficiency() const	{ return mEnergyEfficiency; }
	S32		getPriceParcelClaim() const;
	S32		getPriceParcelRent() const;
	F32		getAreaOwned() const		{ return mAreaOwned; }
	F32		getAreaTotal() const		{ return mAreaTotal; }
	S32 getBasePriceParcelClaimActual() const { return mBasePriceParcelClaimActual; }

	void	setPriceObjectRent(F32 val)			{ mPriceObjectRent = val; }
	void	setPriceObjectScaleFactor(F32 val) { mPriceObjectScaleFactor = val; }
	void	setEnergyEfficiency(F32 val)		{ mEnergyEfficiency = val; }

	void setBasePriceParcelClaimDefault(S32 val);
	void setBasePriceParcelClaimActual(S32 val);
	void setPriceParcelClaimFactor(F32 val);
	void setBasePriceParcelRent(S32 val);

	void	setAreaOwned(F32 val)				{ mAreaOwned = val; }
	void	setAreaTotal(F32 val)				{ mAreaTotal = val; }

private:
	F32		mPriceObjectRent;
	F32		mPriceObjectScaleFactor;
	F32		mEnergyEfficiency;

	S32	mBasePriceParcelClaimDefault;
	S32 mBasePriceParcelClaimActual;
	F32 mPriceParcelClaimFactor;
	S32 mBasePriceParcelRent;

	F32		mAreaOwned;
	F32		mAreaTotal;

};

#endif
