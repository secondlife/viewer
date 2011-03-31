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
#include "llnotificationsutil.h"
#include "llstatusbar.h"
#include "lltextbox.h"
#include "llviewchildren.h"
#include "llviewerwindow.h"
#include "lluictrlfactory.h"
#include "llweb.h"
#include "llwindow.h"
#include "llappviewer.h"

static const S32 STANDARD_BUY_AMOUNT = 2000;
static const S32 MINIMUM_BALANCE_AMOUNT = 0;

class LLFloaterBuyCurrencyUI
:	public LLFloater
{
public:
	LLFloaterBuyCurrencyUI(const LLSD& key);
	virtual ~LLFloaterBuyCurrencyUI();


public:
	LLViewChildren		mChildren;
	LLCurrencyUIManager	mManager;
	
	bool		mHasTarget;
	std::string	mTargetName;
	S32			mTargetPrice;
	
public:
	void noTarget();
	void target(const std::string& name, S32 price);
	
	virtual BOOL postBuild();
	
	void updateUI();

	virtual void draw();
	virtual BOOL canClose();

	void onClickBuy();
	void onClickCancel();
	void onClickErrorWeb();
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
:	LLFloater(key),
	mChildren(*this),
	mManager(*this)
{
}

LLFloaterBuyCurrencyUI::~LLFloaterBuyCurrencyUI()
{
}


void LLFloaterBuyCurrencyUI::noTarget()
{
	mHasTarget = false;
	mManager.setAmount(STANDARD_BUY_AMOUNT);
}

void LLFloaterBuyCurrencyUI::target(const std::string& name, S32 price)
{
	mHasTarget = true;
	mTargetName = name;
	mTargetPrice = price;
	
	S32 balance = gStatusBar->getBalance();
	S32 need = price - balance;
	if (need < 0)
	{
		need = 0;
	}
	
	mManager.setAmount(need + MINIMUM_BALANCE_AMOUNT);
}


// virtual
BOOL LLFloaterBuyCurrencyUI::postBuild()
{
	mManager.prepare();
	
	getChild<LLUICtrl>("buy_btn")->setCommitCallback( boost::bind(&LLFloaterBuyCurrencyUI::onClickBuy, this));
	getChild<LLUICtrl>("cancel_btn")->setCommitCallback( boost::bind(&LLFloaterBuyCurrencyUI::onClickCancel, this));
	getChild<LLUICtrl>("error_web")->setCommitCallback( boost::bind(&LLFloaterBuyCurrencyUI::onClickErrorWeb, this));
	
	center();
	
	updateUI();
	
	return TRUE;
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

BOOL LLFloaterBuyCurrencyUI::canClose()
{
	return mManager.canCancel();
}

void LLFloaterBuyCurrencyUI::updateUI()
{
	bool hasError = mManager.hasError();
	mManager.updateUI(!hasError && !mManager.buying());

	// hide most widgets - we'll turn them on as needed next
	getChildView("info_buying")->setVisible(FALSE);
	getChildView("info_cannot_buy")->setVisible(FALSE);
	getChildView("info_need_more")->setVisible(FALSE);	
	getChildView("purchase_warning_repurchase")->setVisible(FALSE);
	getChildView("purchase_warning_notenough")->setVisible(FALSE);
	getChildView("contacting")->setVisible(FALSE);
	getChildView("buy_action")->setVisible(FALSE);

	if (hasError)
	{
		// display an error from the server
		getChildView("normal_background")->setVisible(FALSE);
		getChildView("error_background")->setVisible(TRUE);
		getChildView("info_cannot_buy")->setVisible(TRUE);
		getChildView("cannot_buy_message")->setVisible(TRUE);
		getChildView("balance_label")->setVisible(FALSE);
		getChildView("balance_amount")->setVisible(FALSE);
		getChildView("buying_label")->setVisible(FALSE);
		getChildView("buying_amount")->setVisible(FALSE);
		getChildView("total_label")->setVisible(FALSE);
		getChildView("total_amount")->setVisible(FALSE);

        LLTextBox* message = getChild<LLTextBox>("cannot_buy_message");
        if (message)
		{
			message->setText(mManager.errorMessage());
		}

		getChildView("error_web")->setVisible( !mManager.errorURI().empty());
	}
	else
	{
		// display the main Buy L$ interface
		getChildView("normal_background")->setVisible(TRUE);
		getChildView("error_background")->setVisible(FALSE);
		getChildView("cannot_buy_message")->setVisible(FALSE);
		getChildView("error_web")->setVisible(FALSE);

		if (mHasTarget)
		{
			getChildView("info_need_more")->setVisible(TRUE);
		}
		else
		{
			getChildView("info_buying")->setVisible(TRUE);
		}

		if (mManager.buying())
		{
			getChildView("contacting")->setVisible( true);
		}
		else
		{
			if (mHasTarget)
			{
				getChildView("buy_action")->setVisible( true);
				getChild<LLUICtrl>("buy_action")->setTextArg("[ACTION]", mTargetName);
			}
		}
		
		S32 balance = gStatusBar->getBalance();
		getChildView("balance_label")->setVisible(TRUE);
		getChildView("balance_amount")->setVisible(TRUE);
		getChild<LLUICtrl>("balance_amount")->setTextArg("[AMT]", llformat("%d", balance));
		
		S32 buying = mManager.getAmount();
		getChildView("buying_label")->setVisible(TRUE);
		getChildView("buying_amount")->setVisible(TRUE);
		getChild<LLUICtrl>("buying_amount")->setTextArg("[AMT]", llformat("%d", buying));
		
		S32 total = balance + buying;
		getChildView("total_label")->setVisible(TRUE);
		getChildView("total_amount")->setVisible(TRUE);
		getChild<LLUICtrl>("total_amount")->setTextArg("[AMT]", llformat("%d", total));

		if (mHasTarget)
		{
			if (total >= mTargetPrice)
			{
				getChildView("purchase_warning_repurchase")->setVisible( true);
			}
			else
			{
				getChildView("purchase_warning_notenough")->setVisible( true);
			}
		}
	}

	getChildView("getting_data")->setVisible( !mManager.canBuy() && !hasError);
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

void LLFloaterBuyCurrencyUI::onClickErrorWeb()
{
	LLWeb::loadURLExternal(mManager.errorURI());
	closeFloater();
	// Update L$ balance
	LLStatusBar::sendMoneyBalanceRequest();
}

// static
void LLFloaterBuyCurrency::buyCurrency()
{
	LLFloaterBuyCurrencyUI* ui = LLFloaterReg::showTypedInstance<LLFloaterBuyCurrencyUI>("buy_currency");
	ui->noTarget();
	ui->updateUI();
}

// static
void LLFloaterBuyCurrency::buyCurrency(const std::string& name, S32 price)
{
	LLFloaterBuyCurrencyUI* ui = LLFloaterReg::showTypedInstance<LLFloaterBuyCurrencyUI>("buy_currency");
	ui->target(name, price);
	ui->updateUI();
}


