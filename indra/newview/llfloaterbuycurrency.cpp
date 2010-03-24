/** 
 * @file llfloaterbuycurrency.cpp
 * @brief LLFloaterBuyCurrency class implementation
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
	childSetEnabled("buy_btn", mManager.canBuy());

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
	childHide("info_buying");
	childHide("info_cannot_buy");
	childHide("info_need_more");	
	childHide("purchase_warning_repurchase");
	childHide("purchase_warning_notenough");
	childHide("contacting");
	childHide("buy_action");

	if (hasError)
	{
		// display an error from the server
		childHide("normal_background");
		childShow("error_background");
		childShow("info_cannot_buy");
		childShow("cannot_buy_message");
		childHide("balance_label");
		childHide("balance_amount");
		childHide("buying_label");
		childHide("buying_amount");
		childHide("total_label");
		childHide("total_amount");

        LLTextBox* message = getChild<LLTextBox>("cannot_buy_message");
        if (message)
		{
			message->setText(mManager.errorMessage());
		}

		childSetVisible("error_web", !mManager.errorURI().empty());
	}
	else
	{
		// display the main Buy L$ interface
		childShow("normal_background");
		childHide("error_background");
		childHide("cannot_buy_message");
		childHide("error_web");

		if (mHasTarget)
		{
			childShow("info_need_more");
		}
		else
		{
			childShow("info_buying");
		}

		if (mManager.buying())
		{
			childSetVisible("contacting", true);
		}
		else
		{
			if (mHasTarget)
			{
				childSetVisible("buy_action", true);
				childSetTextArg("buy_action", "[ACTION]", mTargetName);
			}
		}
		
		S32 balance = gStatusBar->getBalance();
		childShow("balance_label");
		childShow("balance_amount");
		childSetTextArg("balance_amount", "[AMT]", llformat("%d", balance));
		
		S32 buying = mManager.getAmount();
		childShow("buying_label");
		childShow("buying_amount");
		childSetTextArg("buying_amount", "[AMT]", llformat("%d", buying));
		
		S32 total = balance + buying;
		childShow("total_label");
		childShow("total_amount");
		childSetTextArg("total_amount", "[AMT]", llformat("%d", total));

		if (mHasTarget)
		{
			if (total >= mTargetPrice)
			{
				childSetVisible("purchase_warning_repurchase", true);
			}
			else
			{
				childSetVisible("purchase_warning_notenough", true);
			}
		}
	}

	childSetVisible("getting_data", !mManager.canBuy() && !hasError);
}

void LLFloaterBuyCurrencyUI::onClickBuy()
{
	mManager.buy(getString("buy_currency"));
	updateUI();
}

void LLFloaterBuyCurrencyUI::onClickCancel()
{
	closeFloater();
}

void LLFloaterBuyCurrencyUI::onClickErrorWeb()
{
	LLWeb::loadURLExternal(mManager.errorURI());
	closeFloater();
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


