/** 
 * @file llpanelland.cpp
 * @brief Land information in the tool floater, NOT the "About Land" floater
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

#include "llviewerprecompiledheaders.h"

#include "llpanelland.h"

#include "llparcel.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llfloaterland.h"
#include "llfloaterreg.h"
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
	childSetAction("button buy land",boost::bind(onClickClaim));
	childSetAction("button abandon land", boost::bind(onClickRelease));
	childSetAction("button subdivide land", boost::bind(onClickDivide));
	childSetAction("button join land", boost::bind(onClickJoin));
	childSetAction("button about land", boost::bind(onClickAbout));

	mCheckShowOwners = getChild<LLCheckBoxCtrl>("checkbox show owners");
	getChild<LLUICtrl>("checkbox show owners")->setValue(gSavedSettings.getBOOL("ShowParcelOwners"));

	return TRUE;
}
//
// Methods
//
LLPanelLandInfo::LLPanelLandInfo()
:	LLPanel(),
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
		getChildView("label_area_price")->setVisible(false);
		getChildView("label_area")->setVisible(false);

		//mTextPrice->setText(LLStringUtil::null);
		getChild<LLUICtrl>("textbox price")->setValue(LLStringUtil::null);

		getChildView("button buy land")->setEnabled(FALSE);
		getChildView("button abandon land")->setEnabled(FALSE);
		getChildView("button subdivide land")->setEnabled(FALSE);
		getChildView("button join land")->setEnabled(FALSE);
		getChildView("button about land")->setEnabled(FALSE);
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
			getChildView("button buy land")->setEnabled(TRUE);
		}
		else
		{
			getChildView("button buy land")->setEnabled(can_buy);
		}

		BOOL owner_release = LLViewerParcelMgr::isParcelOwnedByAgent(parcel, GP_LAND_RELEASE);
		BOOL owner_divide =  LLViewerParcelMgr::isParcelOwnedByAgent(parcel, GP_LAND_DIVIDE_JOIN);

		BOOL manager_releaseable = ( gAgent.canManageEstate()
								  && (parcel->getOwnerID() == regionp->getOwner()) );
		
		BOOL manager_divideable = ( gAgent.canManageEstate()
								&& ((parcel->getOwnerID() == regionp->getOwner()) || owner_divide) );

		getChildView("button abandon land")->setEnabled(owner_release || manager_releaseable || gAgent.isGodlike());

		// only mainland sims are subdividable by owner
		if (regionp->getRegionFlag(REGION_FLAGS_ALLOW_PARCEL_CHANGES))
		{
			getChildView("button subdivide land")->setEnabled(owner_divide || manager_divideable || gAgent.isGodlike());
		}
		else
		{
			getChildView("button subdivide land")->setEnabled(manager_divideable || gAgent.isGodlike());
		}
		
		// To join land, must have something selected,
		// not just a single unit of land,
		// you must own part of it,
		// and it must not be a whole parcel.
		if (LLViewerParcelMgr::getInstance()->getSelectedArea() > PARCEL_UNIT_AREA
			//&& LLViewerParcelMgr::getInstance()->getSelfCount() > 1
			&& !LLViewerParcelMgr::getInstance()->getParcelSelection()->getWholeParcelSelected())
		{
			getChildView("button join land")->setEnabled(TRUE);
		}
		else
		{
			lldebugs << "Invalid selection for joining land" << llendl;
			getChildView("button join land")->setEnabled(FALSE);
		}

		getChildView("button about land")->setEnabled(TRUE);

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
			getChild<LLUICtrl>("label_area_price")->setTextArg("[PRICE]", llformat("%d",claim_price));
			getChild<LLUICtrl>("label_area_price")->setTextArg("[AREA]", llformat("%d",area));
			getChildView("label_area_price")->setVisible(true);
			getChildView("label_area")->setVisible(false);
		}
		else
		{
			getChildView("label_area_price")->setVisible(false);
			getChild<LLUICtrl>("label_area")->setTextArg("[AREA]", llformat("%d",area));
			getChildView("label_area")->setVisible(true);
		}
	}
}


//static
void LLPanelLandInfo::onClickClaim()
{
	LLViewerParcelMgr::getInstance()->startBuyLand();
}


//static
void LLPanelLandInfo::onClickRelease()
{
	LLViewerParcelMgr::getInstance()->startReleaseLand();
}

// static
void LLPanelLandInfo::onClickDivide()
{
	LLViewerParcelMgr::getInstance()->startDivideLand();
}

// static
void LLPanelLandInfo::onClickJoin()
{
	LLViewerParcelMgr::getInstance()->startJoinLand();
}

//static
void LLPanelLandInfo::onClickAbout()
{
	// Promote the rectangle selection to a parcel selection
	if (!LLViewerParcelMgr::getInstance()->getParcelSelection()->getWholeParcelSelected())
	{
		LLViewerParcelMgr::getInstance()->selectParcelInRectangle();
	}

	LLFloaterReg::showInstance("about_land");
}
