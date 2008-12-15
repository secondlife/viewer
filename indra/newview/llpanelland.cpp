/** 
 * @file llpanelland.cpp
 * @brief Land information in the tool floater, NOT the "About Land" floater
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

#include "llviewerprecompiledheaders.h"

#include "llpanelland.h"

#include "llparcel.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llfloaterland.h"
#include "lltextbox.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "roles_constants.h"

#include "lluictrlfactory.h"

LLPanelLandSelectObserver* LLPanelLandInfo::sObserver = NULL;
LLPanelLandInfo* LLPanelLandInfo::sInstance = NULL;

class LLPanelLandSelectObserver : public LLParcelObserver
{
public:
	LLPanelLandSelectObserver() {}
	virtual ~LLPanelLandSelectObserver() {}
	virtual void changed() { LLPanelLandInfo::refreshAll(); }
};


BOOL	LLPanelLandInfo::postBuild()
{

	childSetAction("button buy land",onClickClaim,this);
	childSetAction("button abandon land",onClickRelease,this);
	childSetAction("button subdivide land",onClickDivide,this);
	childSetAction("button join land",onClickJoin,this);
	childSetAction("button about land",onClickAbout,this);
	childSetAction("button show owners help", onShowOwnersHelp, this);

	mCheckShowOwners = getChild<LLCheckBoxCtrl>("checkbox show owners");
	childSetValue("checkbox show owners", gSavedSettings.getBOOL("ShowParcelOwners"));

	return TRUE;
}
//
// Methods
//
LLPanelLandInfo::LLPanelLandInfo(const std::string& name)
:	LLPanel(name),
	mCheckShowOwners(NULL)
{
	if (!sInstance)
	{
		sInstance = this;
	}
	if (!sObserver)
	{
		sObserver = new LLPanelLandSelectObserver();
		LLViewerParcelMgr::getInstance()->addObserver( sObserver );
	}

}


// virtual
LLPanelLandInfo::~LLPanelLandInfo()
{
	LLViewerParcelMgr::getInstance()->removeObserver( sObserver );
	delete sObserver;
	sObserver = NULL;

	sInstance = NULL;
}


// static
void LLPanelLandInfo::refreshAll()
{
	if (sInstance)
	{
		sInstance->refresh();
	}
}


// public
void LLPanelLandInfo::refresh()
{
	LLParcel *parcel = LLViewerParcelMgr::getInstance()->getParcelSelection()->getParcel();
	LLViewerRegion *regionp = LLViewerParcelMgr::getInstance()->getSelectionRegion();

	if (!parcel || !regionp)
	{
		// nothing selected, disable panel
		childSetVisible("label_area_price",false);
		childSetVisible("label_area",false);

		//mTextPrice->setText(LLStringUtil::null);
		childSetText("textbox price",LLStringUtil::null);

		childSetEnabled("button buy land",FALSE);
		childSetEnabled("button abandon land",FALSE);
		childSetEnabled("button subdivide land",FALSE);
		childSetEnabled("button join land",FALSE);
		childSetEnabled("button about land",FALSE);
	}
	else
	{
		// something selected, hooray!
		const LLUUID& owner_id = parcel->getOwnerID();
		const LLUUID& auth_buyer_id = parcel->getAuthorizedBuyerID();

		BOOL is_public = parcel->isPublic();
		BOOL is_for_sale = parcel->getForSale()
			&& ((parcel->getSalePrice() > 0) || (auth_buyer_id.notNull()));
		BOOL can_buy = (is_for_sale
						&& (owner_id != gAgent.getID())
						&& ((gAgent.getID() == auth_buyer_id)
							|| (auth_buyer_id.isNull())));
			
		if (is_public)
		{
			childSetEnabled("button buy land",TRUE);
		}
		else
		{
			childSetEnabled("button buy land",can_buy);
		}

		BOOL owner_release = LLViewerParcelMgr::isParcelOwnedByAgent(parcel, GP_LAND_RELEASE);
		BOOL owner_divide =  LLViewerParcelMgr::isParcelOwnedByAgent(parcel, GP_LAND_DIVIDE_JOIN);

		BOOL manager_releaseable = ( gAgent.canManageEstate()
								  && (parcel->getOwnerID() == regionp->getOwner()) );
		
		BOOL manager_divideable = ( gAgent.canManageEstate()
								&& ((parcel->getOwnerID() == regionp->getOwner()) || owner_divide) );

		childSetEnabled("button abandon land",owner_release || manager_releaseable || gAgent.isGodlike());

		// only mainland sims are subdividable by owner
		if (regionp->getRegionFlags() && REGION_FLAGS_ALLOW_PARCEL_CHANGES)
		{
			childSetEnabled("button subdivide land",owner_divide || manager_divideable || gAgent.isGodlike());
		}
		else
		{
			childSetEnabled("button subdivide land",manager_divideable || gAgent.isGodlike());
		}
		
		// To join land, must have something selected,
		// not just a single unit of land,
		// you must own part of it,
		// and it must not be a whole parcel.
		if (LLViewerParcelMgr::getInstance()->getSelectedArea() > PARCEL_UNIT_AREA
			//&& LLViewerParcelMgr::getInstance()->getSelfCount() > 1
			&& !LLViewerParcelMgr::getInstance()->getParcelSelection()->getWholeParcelSelected())
		{
			childSetEnabled("button join land",TRUE);
		}
		else
		{
			lldebugs << "Invalid selection for joining land" << llendl;
			childSetEnabled("button join land",FALSE);
		}

		childSetEnabled("button about land",TRUE);

		// show pricing information
		S32 area;
		S32 claim_price;
		S32 rent_price;
		BOOL for_sale;
		F32 dwell;
		LLViewerParcelMgr::getInstance()->getDisplayInfo(&area,
								   &claim_price,
								   &rent_price,
								   &for_sale,
								   &dwell);
		if(is_public || (is_for_sale && LLViewerParcelMgr::getInstance()->getParcelSelection()->getWholeParcelSelected()))
		{
			childSetTextArg("label_area_price","[PRICE]", llformat("%d",claim_price));
			childSetTextArg("label_area_price","[AREA]", llformat("%d",area));
			childSetVisible("label_area_price",true);
			childSetVisible("label_area",false);
		}
		else
		{
			childSetVisible("label_area_price",false);
			childSetTextArg("label_area","[AREA]", llformat("%d",area));
			childSetVisible("label_area",true);
		}
	}
}


//static
void LLPanelLandInfo::onClickClaim(void*)
{
	LLViewerParcelMgr::getInstance()->startBuyLand();
}


//static
void LLPanelLandInfo::onClickRelease(void*)
{
	LLViewerParcelMgr::getInstance()->startReleaseLand();
}

// static
void LLPanelLandInfo::onClickDivide(void*)
{
	LLViewerParcelMgr::getInstance()->startDivideLand();
}

// static
void LLPanelLandInfo::onClickJoin(void*)
{
	LLViewerParcelMgr::getInstance()->startJoinLand();
}

//static
void LLPanelLandInfo::onClickAbout(void*)
{
	// Promote the rectangle selection to a parcel selection
	if (!LLViewerParcelMgr::getInstance()->getParcelSelection()->getWholeParcelSelected())
	{
		LLViewerParcelMgr::getInstance()->selectParcelInRectangle();
	}

	LLFloaterLand::showInstance();
}

void LLPanelLandInfo::onShowOwnersHelp(void* user_data)
{
	gViewerWindow->alertXml("ShowOwnersHelp");
}
