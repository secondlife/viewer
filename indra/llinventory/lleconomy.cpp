/** 
 * @file lleconomy.cpp
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

#include "linden_common.h"

#include "lleconomy.h"
#include "llerror.h"
#include "message.h"
#include "v3math.h"


LLGlobalEconomy::LLGlobalEconomy()
:	mObjectCount( -1 ),
	mObjectCapacity( -1 ),
	mPriceObjectClaim( -1 ),
	mPricePublicObjectDecay( -1 ),
	mPricePublicObjectDelete( -1 ),
	mPriceEnergyUnit( -1 ),
	mPriceUpload( -1 ),
	mPriceRentLight( -1 ),
	mTeleportMinPrice( -1 ),
	mTeleportPriceExponent( -1 ),
	mPriceGroupCreate( -1 )
{ }

LLGlobalEconomy::~LLGlobalEconomy()
{ }

void LLGlobalEconomy::addObserver(LLEconomyObserver* observer)
{
	mObservers.push_back(observer);
}

void LLGlobalEconomy::removeObserver(LLEconomyObserver* observer)
{
	std::list<LLEconomyObserver*>::iterator it =
		std::find(mObservers.begin(), mObservers.end(), observer);
	if (it != mObservers.end())
	{
		mObservers.erase(it);
	}
}

void LLGlobalEconomy::notifyObservers()
{
	for (std::list<LLEconomyObserver*>::iterator it = mObservers.begin();
		it != mObservers.end();
		++it)
	{
		(*it)->onEconomyDataChange();
	}
}

// static
void LLGlobalEconomy::processEconomyData(LLMessageSystem *msg, LLGlobalEconomy* econ_data)
{
	S32 i;
	F32 f;

	msg->getS32Fast(_PREHASH_Info, _PREHASH_ObjectCapacity, i);
	econ_data->setObjectCapacity(i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_ObjectCount, i);
	econ_data->setObjectCount(i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PriceEnergyUnit, i);
	econ_data->setPriceEnergyUnit(i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PriceObjectClaim, i);
	econ_data->setPriceObjectClaim(i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PricePublicObjectDecay, i);
	econ_data->setPricePublicObjectDecay(i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PricePublicObjectDelete, i);
	econ_data->setPricePublicObjectDelete(i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PriceUpload, i);
	econ_data->setPriceUpload(i);
#if LL_LINUX
	// We can optionally fake the received upload price for testing.
	// Note that the server is within its rights to not obey our fake
	// price. :)
	const char* fakeprice_str = getenv("LL_FAKE_UPLOAD_PRICE");
	if (fakeprice_str)
	{
		S32 fakeprice = (S32)atoi(fakeprice_str);
		llwarns << "LL_FAKE_UPLOAD_PRICE: Faking upload price as L$" << fakeprice << llendl;
		econ_data->setPriceUpload(fakeprice);
	}
#endif
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PriceRentLight, i);
	econ_data->setPriceRentLight(i);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_TeleportMinPrice, i);
	econ_data->setTeleportMinPrice(i);
	msg->getF32Fast(_PREHASH_Info, _PREHASH_TeleportPriceExponent, f);
	econ_data->setTeleportPriceExponent(f);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PriceGroupCreate, i);
	econ_data->setPriceGroupCreate(i);

	econ_data->notifyObservers();
}

S32	LLGlobalEconomy::calculateTeleportCost(F32 distance) const
{
	S32 min_cost = getTeleportMinPrice();
	F32 exponent = getTeleportPriceExponent();
	F32 divisor = 100.f * pow(3.f, exponent);
	S32 cost = (U32)(distance * pow(log10(distance), exponent) / divisor);
	if (cost < 0)
	{
		cost = 0;
	}
	else if (cost < min_cost)
	{
		cost = min_cost;
	}

	return cost;
}

S32	LLGlobalEconomy::calculateLightRent(const LLVector3& object_size) const
{
	F32 intensity_mod = llmax(object_size.magVec(), 1.f);
	return (S32)(intensity_mod * getPriceRentLight());
}

void LLGlobalEconomy::print()
{
	llinfos << "Global Economy Settings: " << llendl;
	llinfos << "Object Capacity: " << mObjectCapacity << llendl;
	llinfos << "Object Count: " << mObjectCount << llendl;
	llinfos << "Claim Price Per Object: " << mPriceObjectClaim << llendl;
	llinfos << "Claim Price Per Public Object: " << mPricePublicObjectDecay << llendl;
	llinfos << "Delete Price Per Public Object: " << mPricePublicObjectDelete << llendl;
	llinfos << "Release Price Per Public Object: " << getPricePublicObjectRelease() << llendl;
	llinfos << "Price Per Energy Unit: " << mPriceEnergyUnit << llendl;
	llinfos << "Price Per Upload: " << mPriceUpload << llendl;
	llinfos << "Light Base Price: " << mPriceRentLight << llendl;
	llinfos << "Teleport Min Price: " << mTeleportMinPrice << llendl;
	llinfos << "Teleport Price Exponent: " << mTeleportPriceExponent << llendl;
	llinfos << "Price for group creation: " << mPriceGroupCreate << llendl;
}

LLRegionEconomy::LLRegionEconomy()
:	LLGlobalEconomy(),
	mPriceObjectRent( -1.f ),
	mPriceObjectScaleFactor( -1.f ),
	mEnergyEfficiency( -1.f ),
	mBasePriceParcelClaimDefault(-1),
	mBasePriceParcelClaimActual(-1),
	mPriceParcelClaimFactor(-1.f),
	mBasePriceParcelRent(-1),
	mAreaOwned(-1.f),
	mAreaTotal(-1.f)
{ }

LLRegionEconomy::~LLRegionEconomy()
{ }

BOOL LLRegionEconomy::hasData() const
{
	return (mBasePriceParcelRent != -1);
}

// static
void LLRegionEconomy::processEconomyData(LLMessageSystem *msg, void** user_data)
{
	S32 i;
	F32 f;

	LLRegionEconomy *this_ptr = (LLRegionEconomy*)user_data;

	LLGlobalEconomy::processEconomyData(msg, this_ptr);

	msg->getS32Fast(_PREHASH_Info, _PREHASH_PriceParcelClaim, i);
	this_ptr->setBasePriceParcelClaimDefault(i);
	msg->getF32(_PREHASH_Info, _PREHASH_PriceParcelClaimFactor, f);
	this_ptr->setPriceParcelClaimFactor(f);
	msg->getF32Fast(_PREHASH_Info, _PREHASH_EnergyEfficiency, f);
	this_ptr->setEnergyEfficiency(f);
	msg->getF32Fast(_PREHASH_Info, _PREHASH_PriceObjectRent, f);
	this_ptr->setPriceObjectRent(f);
	msg->getF32Fast(_PREHASH_Info, _PREHASH_PriceObjectScaleFactor, f);
	this_ptr->setPriceObjectScaleFactor(f);
	msg->getS32Fast(_PREHASH_Info, _PREHASH_PriceParcelRent, i);
	this_ptr->setBasePriceParcelRent(i);
}

// static
void LLRegionEconomy::processEconomyDataRequest(LLMessageSystem *msg, void **user_data)
{
	LLRegionEconomy *this_ptr = (LLRegionEconomy*)user_data;
	if (!this_ptr->hasData())
	{
		llwarns << "Dropping EconomyDataRequest, because EconomyData message "
				<< "has not been processed" << llendl;
	}

	msg->newMessageFast(_PREHASH_EconomyData);
	msg->nextBlockFast(_PREHASH_Info);
	msg->addS32Fast(_PREHASH_ObjectCapacity, this_ptr->getObjectCapacity());
	msg->addS32Fast(_PREHASH_ObjectCount, this_ptr->getObjectCount());
	msg->addS32Fast(_PREHASH_PriceEnergyUnit, this_ptr->getPriceEnergyUnit());
	msg->addS32Fast(_PREHASH_PriceObjectClaim, this_ptr->getPriceObjectClaim());
	msg->addS32Fast(_PREHASH_PricePublicObjectDecay, this_ptr->getPricePublicObjectDecay());
	msg->addS32Fast(_PREHASH_PricePublicObjectDelete, this_ptr->getPricePublicObjectDelete());
	msg->addS32Fast(_PREHASH_PriceParcelClaim, this_ptr->mBasePriceParcelClaimActual);
	msg->addF32Fast(_PREHASH_PriceParcelClaimFactor, this_ptr->mPriceParcelClaimFactor);
	msg->addS32Fast(_PREHASH_PriceUpload, this_ptr->getPriceUpload());
	msg->addS32Fast(_PREHASH_PriceRentLight, this_ptr->getPriceRentLight());
	msg->addS32Fast(_PREHASH_TeleportMinPrice, this_ptr->getTeleportMinPrice());
	msg->addF32Fast(_PREHASH_TeleportPriceExponent, this_ptr->getTeleportPriceExponent());

	msg->addF32Fast(_PREHASH_EnergyEfficiency, this_ptr->getEnergyEfficiency());
	msg->addF32Fast(_PREHASH_PriceObjectRent, this_ptr->getPriceObjectRent());
	msg->addF32Fast(_PREHASH_PriceObjectScaleFactor, this_ptr->getPriceObjectScaleFactor());
	msg->addS32Fast(_PREHASH_PriceParcelRent, this_ptr->getPriceParcelRent());
	msg->addS32Fast(_PREHASH_PriceGroupCreate, this_ptr->getPriceGroupCreate());

	msg->sendReliable(msg->getSender());
}


S32 LLRegionEconomy::getPriceParcelClaim() const
{
	//return (S32)((F32)mBasePriceParcelClaim * (mAreaTotal / (mAreaTotal - mAreaOwned)));
	return (S32)((F32)mBasePriceParcelClaimActual * mPriceParcelClaimFactor);
}

S32 LLRegionEconomy::getPriceParcelRent() const
{
	return mBasePriceParcelRent;
}


void LLRegionEconomy::print()
{
	this->LLGlobalEconomy::print();

	llinfos << "Region Economy Settings: " << llendl;
	llinfos << "Land (square meters): " << mAreaTotal << llendl;
	llinfos << "Owned Land (square meters): " << mAreaOwned << llendl;
	llinfos << "Daily Object Rent: " << mPriceObjectRent << llendl;
	llinfos << "Daily Land Rent (per meter): " << getPriceParcelRent() << llendl;
	llinfos << "Energey Efficiency: " << mEnergyEfficiency << llendl;
}


void LLRegionEconomy::setBasePriceParcelClaimDefault(S32 val)
{
	mBasePriceParcelClaimDefault = val;
	if(mBasePriceParcelClaimActual == -1)
	{
		mBasePriceParcelClaimActual = val;
	}
}

void LLRegionEconomy::setBasePriceParcelClaimActual(S32 val)
{
	mBasePriceParcelClaimActual = val;
}

void LLRegionEconomy::setPriceParcelClaimFactor(F32 val)
{
	mPriceParcelClaimFactor = val;
}

void LLRegionEconomy::setBasePriceParcelRent(S32 val)
{
	mBasePriceParcelRent = val;
}
