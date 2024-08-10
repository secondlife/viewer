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


bool    LLPanelLandInfo::postBuild()
{
    mButtonBuyLand = getChild<LLButton>("button buy land");
    mButtonBuyLand->setCommitCallback(boost::bind(&LLPanelLandInfo::onClickClaim, this));

    mButtonAbandonLand = getChild<LLButton>("button abandon land");
    mButtonAbandonLand->setCommitCallback(boost::bind(&LLPanelLandInfo::onClickRelease, this));

    mButtonSubdivLand = getChild<LLButton>("button subdivide land");
    mButtonSubdivLand->setCommitCallback(boost::bind(&LLPanelLandInfo::onClickDivide, this));

    mButtonJoinLand = getChild<LLButton>("button join land");
    mButtonJoinLand->setCommitCallback(boost::bind(&LLPanelLandInfo::onClickJoin, this));

    mButtonAboutLand = getChild<LLButton>("button about land");
    mButtonAboutLand->setCommitCallback(boost::bind(&LLPanelLandInfo::onClickAbout, this));

    mCheckShowOwners = getChild<LLCheckBoxCtrl>("checkbox show owners");
    mCheckShowOwners->setValue(gSavedSettings.getBOOL("ShowParcelOwners"));

    mTextArea = getChild<LLTextBox>("label_area");
    mTextAreaPrice = getChild<LLTextBox>("label_area_price");

    return true;
}
//
// Methods
//
LLPanelLandInfo::LLPanelLandInfo()
:   LLPanel(),
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
        mTextAreaPrice->setVisible(false);
        mTextArea->setVisible(false);

        mButtonBuyLand->setEnabled(false);
        mButtonAbandonLand->setEnabled(false);
        mButtonSubdivLand->setEnabled(false);
        mButtonJoinLand->setEnabled(false);
        mButtonAboutLand->setEnabled(false);
    }
    else
    {
        // something selected, hooray!
        const LLUUID& owner_id = parcel->getOwnerID();
        const LLUUID& auth_buyer_id = parcel->getAuthorizedBuyerID();

        bool is_public = parcel->isPublic();
        bool is_for_sale = parcel->getForSale()
            && ((parcel->getSalePrice() > 0) || (auth_buyer_id.notNull()));
        bool can_buy = (is_for_sale
                        && (owner_id != gAgent.getID())
                        && ((gAgent.getID() == auth_buyer_id)
                            || (auth_buyer_id.isNull())));

        if (is_public && !LLViewerParcelMgr::getInstance()->getParcelSelection()->getMultipleOwners())
        {
            mButtonBuyLand->setEnabled(true);
        }
        else
        {
            mButtonBuyLand->setEnabled(can_buy);
        }

        bool owner_release = LLViewerParcelMgr::isParcelOwnedByAgent(parcel, GP_LAND_RELEASE);
        bool owner_divide =  LLViewerParcelMgr::isParcelOwnedByAgent(parcel, GP_LAND_DIVIDE_JOIN);

        bool manager_releaseable = ( gAgent.canManageEstate()
                                  && (parcel->getOwnerID() == regionp->getOwner()) );

        bool manager_divideable = ( gAgent.canManageEstate()
                                && ((parcel->getOwnerID() == regionp->getOwner()) || owner_divide) );

        mButtonAbandonLand->setEnabled(owner_release || manager_releaseable || gAgent.isGodlike());

        // only mainland sims are subdividable by owner
        if (regionp->getRegionFlag(REGION_FLAGS_ALLOW_PARCEL_CHANGES))
        {
            mButtonSubdivLand->setEnabled(owner_divide || manager_divideable || gAgent.isGodlike());
        }
        else
        {
            mButtonSubdivLand->setEnabled(manager_divideable || gAgent.isGodlike());
        }

        // To join land, must have something selected,
        // not just a single unit of land,
        // you must own part of it,
        // and it must not be a whole parcel.
        if (LLViewerParcelMgr::getInstance()->getSelectedArea() > PARCEL_UNIT_AREA
            //&& LLViewerParcelMgr::getInstance()->getSelfCount() > 1
            && !LLViewerParcelMgr::getInstance()->getParcelSelection()->getWholeParcelSelected())
        {
            mButtonJoinLand->setEnabled(true);
        }
        else
        {
            LL_DEBUGS() << "Invalid selection for joining land" << LL_ENDL;
            mButtonJoinLand->setEnabled(false);
        }

        mButtonAboutLand->setEnabled(true);

        // show pricing information
        S32 area;
        S32 claim_price;
        S32 rent_price;
        bool for_sale;
        F32 dwell;
        LLViewerParcelMgr::getInstance()->getDisplayInfo(&area,
                                   &claim_price,
                                   &rent_price,
                                   &for_sale,
                                   &dwell);
        if(is_public || (is_for_sale && LLViewerParcelMgr::getInstance()->getParcelSelection()->getWholeParcelSelected()))
        {
            mTextAreaPrice->setTextArg("[PRICE]", llformat("%d",claim_price));
            mTextAreaPrice->setTextArg("[AREA]", llformat("%d",area));
            mTextAreaPrice->setVisible(true);
            mTextArea->setVisible(false);
        }
        else
        {
            mTextAreaPrice->setVisible(false);
            mTextArea->setTextArg("[AREA]", llformat("%d",area));
            mTextArea->setVisible(true);
        }
    }
}


void LLPanelLandInfo::onClickClaim()
{
    LLViewerParcelMgr::getInstance()->startBuyLand();
}


void LLPanelLandInfo::onClickRelease()
{
    LLViewerParcelMgr::getInstance()->startReleaseLand();
}

void LLPanelLandInfo::onClickDivide()
{
    LLViewerParcelMgr::getInstance()->startDivideLand();
}

void LLPanelLandInfo::onClickJoin()
{
    LLViewerParcelMgr::getInstance()->startJoinLand();
}

void LLPanelLandInfo::onClickAbout()
{
    // Promote the rectangle selection to a parcel selection
    if (!LLViewerParcelMgr::getInstance()->getParcelSelection()->getWholeParcelSelected())
    {
        LLViewerParcelMgr::getInstance()->selectParcelInRectangle();
    }

    LLFloaterReg::showInstance("about_land");
}
