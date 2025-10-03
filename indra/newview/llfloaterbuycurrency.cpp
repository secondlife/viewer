/**
 * @file llfloaterbuycurrency.cpp
 * @brief LLFloaterBuyCurrency class implementation
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "llfloaterbuycurrency.h"

// viewer includes
#include "llcurrencyuimanager.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "lllayoutstack.h"
#include "lliconctrl.h"
#include "llnotificationsutil.h"
#include "llstatusbar.h"
#include "lltextbox.h"
#include "llviewchildren.h"
#include "llviewerwindow.h"
#include "lluictrlfactory.h"
#include "llweb.h"
#include "llwindow.h"
#include "llappviewer.h"

static const S32 MINIMUM_BALANCE_AMOUNT = 0;

class LLFloaterBuyCurrencyUI
:   public LLFloater
{
public:
    LLFloaterBuyCurrencyUI(const LLSD& key);
    virtual ~LLFloaterBuyCurrencyUI();


public:
    LLViewChildren      mChildren;
    LLCurrencyUIManager mManager;

    bool        mHasTarget;
    S32         mTargetPrice;
    S32         mRequiredAmount;

public:
    void noTarget();
    void target(const std::string& name, S32 price);

    virtual bool postBuild();

    void updateUI();
    void collapsePanels(bool collapse);

    virtual void draw();
    virtual bool canClose();

    void onClickBuy();
    void onClickCancel();
};

LLFloater* LLFloaterBuyCurrency::buildFloater(const LLSD& key)
{
    LLFloaterBuyCurrencyUI* floater = new LLFloaterBuyCurrencyUI(key);
    return floater;
}

#if LL_WINDOWS
// passing 'this' during construction generates a warning. The callee
// only uses the pointer to hold a reference to 'this' which is
// already valid, so this call does the correct thing. Disable the
// warning so that we can compile without generating a warning.
#pragma warning(disable : 4355)
#endif
LLFloaterBuyCurrencyUI::LLFloaterBuyCurrencyUI(const LLSD& key)
:   LLFloater(key),
    mChildren(*this),
    mManager(*this),
    mHasTarget(false),
    mTargetPrice(0)
{
}

LLFloaterBuyCurrencyUI::~LLFloaterBuyCurrencyUI()
{
}


void LLFloaterBuyCurrencyUI::noTarget()
{
    mHasTarget = false;
    mTargetPrice = 0;
    mManager.setAmount(0);
}

void LLFloaterBuyCurrencyUI::target(const std::string& name, S32 price)
{
    mHasTarget = true;
    mTargetPrice = price;

    if (!name.empty())
    {
        getChild<LLUICtrl>("target_price_label")->setValue(name);
    }

    S32 balance = gStatusBar->getBalance();
    S32 need = price - balance;
    if (need < 0)
    {
        need = 0;
    }

    mRequiredAmount = need + MINIMUM_BALANCE_AMOUNT;
    mManager.setAmount(0);
}


// virtual
bool LLFloaterBuyCurrencyUI::postBuild()
{
    mManager.prepare();

    getChild<LLUICtrl>("buy_btn")->setCommitCallback( boost::bind(&LLFloaterBuyCurrencyUI::onClickBuy, this));
    getChild<LLUICtrl>("cancel_btn")->setCommitCallback( boost::bind(&LLFloaterBuyCurrencyUI::onClickCancel, this));

    center();

    updateUI();

    return true;
}

void LLFloaterBuyCurrencyUI::draw()
{
    if (mManager.process())
    {
        if (mManager.bought())
        {
            LLNotificationsUtil::add("BuyLindenDollarSuccess");
            closeFloater();
            return;
        }

        updateUI();
    }

    // disable the Buy button when we are not able to buy
    getChildView("buy_btn")->setEnabled(mManager.canBuy());

    LLFloater::draw();
}

bool LLFloaterBuyCurrencyUI::canClose()
{
    return mManager.canCancel();
}

void LLFloaterBuyCurrencyUI::updateUI()
{
    bool hasError = mManager.hasError();
    mManager.updateUI(!hasError && !mManager.buying());

    // hide most widgets - we'll turn them on as needed next
    getChildView("info_buying")->setVisible(false);
    getChildView("info_need_more")->setVisible(false);
    getChildView("purchase_warning_repurchase")->setVisible(false);
    getChildView("purchase_warning_notenough")->setVisible(false);
    getChildView("contacting")->setVisible(false);

    if (hasError)
    {
        // display an error from the server
        LLSD args;
        args["TITLE"] = getString("info_cannot_buy");
        args["MESSAGE"] = mManager.errorMessage();
        LLNotificationsUtil::add("CouldNotBuyCurrency", args);
        mManager.clearError();
        closeFloater();
    }
    else
    {
        // display the main Buy L$ interface
        getChildView("normal_background")->setVisible(true);

        if (mHasTarget)
        {
            getChildView("info_need_more")->setVisible(true);
        }
        else
        {
            getChildView("info_buying")->setVisible(true);
        }

        if (mManager.buying())
        {
            getChildView("contacting")->setVisible( true);
        }
        else
        {
            if (mHasTarget)
            {
                getChild<LLUICtrl>("target_price")->setTextArg("[AMT]", llformat("%d", mTargetPrice));
                getChild<LLUICtrl>("required_amount")->setTextArg("[AMT]", llformat("%d", mRequiredAmount));
            }
        }

        S32 balance = gStatusBar->getBalance();
        getChildView("balance_label")->setVisible(true);
        getChildView("balance_amount")->setVisible(true);
        getChild<LLUICtrl>("balance_amount")->setTextArg("[AMT]", llformat("%d", balance));

        S32 buying = mManager.getAmount();
        getChildView("buying_label")->setVisible(true);
        getChildView("buying_amount")->setVisible(true);
        getChild<LLUICtrl>("buying_amount")->setTextArg("[AMT]", llformat("%d", buying));

        S32 total = balance + buying;
        getChildView("total_label")->setVisible(true);
        getChildView("total_amount")->setVisible(true);
        getChild<LLUICtrl>("total_amount")->setTextArg("[AMT]", llformat("%d", total));

        if (mHasTarget)
        {
            getChildView("purchase_warning_repurchase")->setVisible( !getChildView("currency_links")->getVisible());
        }
    }

    getChildView("getting_data")->setVisible( !mManager.canBuy() && !hasError && !getChildView("currency_est")->getVisible());
}

void LLFloaterBuyCurrencyUI::collapsePanels(bool collapse)
{
    LLLayoutPanel* price_panel = getChild<LLLayoutPanel>("layout_panel_price");

    if (price_panel->isCollapsed() == collapse)
        return;

    LLLayoutStack* outer_stack = getChild<LLLayoutStack>("outer_stack");
    LLLayoutPanel* required_panel = getChild<LLLayoutPanel>("layout_panel_required");
    LLLayoutPanel* msg_panel = getChild<LLLayoutPanel>("layout_panel_msg");

    S32 delta_height = price_panel->getRect().getHeight() + required_panel->getRect().getHeight() + msg_panel->getRect().getHeight();
    delta_height *= (collapse ? -1 : 1);

    LLIconCtrl* icon = getChild<LLIconCtrl>("normal_background");
    LLRect rect = icon->getRect();
    icon->setRect(rect.setOriginAndSize(rect.mLeft, rect.mBottom - delta_height, rect.getWidth(), rect.getHeight() + delta_height));

    outer_stack->collapsePanel(price_panel, collapse);
    outer_stack->collapsePanel(required_panel, collapse);
    outer_stack->collapsePanel(msg_panel, collapse);

    outer_stack->updateLayout();

    LLRect floater_rect = getRect();
    floater_rect.mBottom -= delta_height;
    setShape(floater_rect, false);
}

void LLFloaterBuyCurrencyUI::onClickBuy()
{
    mManager.buy(getString("buy_currency"));
    updateUI();
    // Update L$ balance
    LLStatusBar::sendMoneyBalanceRequest();
}

void LLFloaterBuyCurrencyUI::onClickCancel()
{
    closeFloater();
    // Update L$ balance
    LLStatusBar::sendMoneyBalanceRequest();
}

LLFetchAvatarPaymentInfo* LLFloaterBuyCurrency::sPropertiesRequest = NULL;

// static
void LLFloaterBuyCurrency::buyCurrency()
{
    delete sPropertiesRequest;
    sPropertiesRequest = new LLFetchAvatarPaymentInfo(false);
}

// static
void LLFloaterBuyCurrency::buyCurrency(const std::string& name, S32 price)
{
    delete sPropertiesRequest;
    sPropertiesRequest = new LLFetchAvatarPaymentInfo(true, name, price);
}

// static
void LLFloaterBuyCurrency::handleBuyCurrency(bool has_piof, bool has_target, const std::string name, S32 price)
{
    delete sPropertiesRequest;
    sPropertiesRequest = NULL;

    if (has_piof)
    {
        LLFloaterBuyCurrencyUI* ui = LLFloaterReg::showTypedInstance<LLFloaterBuyCurrencyUI>("buy_currency");
        if (has_target)
        {
            ui->target(name, price);
        }
        else
        {
            ui->noTarget();
        }
        ui->updateUI();
        ui->collapsePanels(!has_target);
    }
    else
    {
        LLFloaterReg::showInstance("add_payment_method");
    }
}

LLFetchAvatarPaymentInfo::LLFetchAvatarPaymentInfo(bool has_target, const std::string& name, S32 price)
:   mAvatarID(gAgent.getID()),
    mHasTarget(has_target),
    mPrice(price),
    mName(name)
{
    LLAvatarPropertiesProcessor* processor = LLAvatarPropertiesProcessor::getInstance();
    // register ourselves as an observer
    processor->addObserver(mAvatarID, this);
    // send a request (duplicates will be suppressed inside the avatar
    // properties processor)
    processor->sendAvatarPropertiesRequest(mAvatarID);
}

LLFetchAvatarPaymentInfo::~LLFetchAvatarPaymentInfo()
{
    LLAvatarPropertiesProcessor::getInstance()->removeObserver(mAvatarID, this);
}

void LLFetchAvatarPaymentInfo::processProperties(void* data, EAvatarProcessorType type)
{
    if (data && type == APT_PROPERTIES)
    {
        LLAvatarData* avatar_data = static_cast<LLAvatarData*>(data);
        LLFloaterBuyCurrency::handleBuyCurrency(LLAvatarPropertiesProcessor::hasPaymentInfoOnFile(avatar_data), mHasTarget, mName, mPrice);
    }
}
