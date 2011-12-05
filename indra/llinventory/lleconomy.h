/** 
 * @file lleconomy.h
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

#ifndef LL_LLECONOMY_H
#define LL_LLECONOMY_H

#include "llsingleton.h"

class LLMessageSystem;
class LLVector3;

/**
 * Register an observer to be notified of economy data updates coming from server.
 */
class LLEconomyObserver
{
public:
	virtual ~LLEconomyObserver() {}
	virtual void onEconomyDataChange() = 0;
};

class LLGlobalEconomy
{
public:
	LLGlobalEconomy();
	virtual ~LLGlobalEconomy();

	// This class defines its singleton internally as a typedef instead of inheriting from
	// LLSingleton like most others because the LLRegionEconomy sub-class might also
	// become a singleton and this pattern will more easily disambiguate them.
	typedef LLSingleton<LLGlobalEconomy> Singleton;

	void initSingleton() { }

	virtual void print();

	void	addObserver(LLEconomyObserver* observer);
	void	removeObserver(LLEconomyObserver* observer);
	void	notifyObservers();

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

	std::list<LLEconomyObserver*> mObservers;
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
