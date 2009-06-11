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
	static LLFloaterBuyCurrencyUI* soleInstance(bool createIfNeeded);

private:
	static LLFloaterBuyCurrencyUI* sInstance;

	LLFloaterBuyCurrencyUI();
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
	virtual void onClose(bool app_quitting);

	static void onClickBuy(void* data);
	static void onClickCancel(void* data);
	static void onClickErrorWeb(void* data);
};


// static
LLFloaterBuyCurrencyUI* LLFloaterBuyCurrencyUI::sInstance = NULL;

// static
LLFloaterBuyCurrencyUI* LLFloaterBuyCurrencyUI::soleInstance(bool createIfNeeded)
{
	if (!sInstance  &&  createIfNeeded)
	{
		sInstance = new LLFloaterBuyCurrencyUI();

		LLUICtrlFactory::getInstance()->buildFloater(sInstance, "floater_buy_currency.xml");
		sInstance->center();
	}
	
	return sInstance;
}


#if LL_WINDOWS
// passing 'this' during construction generates a warning. The callee
// only uses the pointer to hold a reference to 'this' which is
// already valid, so this call does the correct thing. Disable the
// warning so that we can compile without generating a warning.
#pragma warning(disable : 4355)
#endif 
LLFloaterBuyCurrencyUI::LLFloaterBuyCurrencyUI()
:	LLFloater(std::string("Buy Currency")),
	mChildren(*this),
	mManager(*this)
{
}

LLFloaterBuyCurrencyUI::~LLFloaterBuyCurrencyUI()
{
	if (sInstance == this)
	{
		sInstance = NULL;
	}
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
	
	childSetAction("buy_btn", onClickBuy, this);
	childSetAction("cancel_btn", onClickCancel, this);
	childSetAction("error_web", onClickErrorWeb, this);

	updateUI();
	
	return TRUE;
}

void LLFloaterBuyCurrencyUI::draw()
{
	if (mManager.process())
	{
		if (mManager.bought())
		{
			close();
			return;
		}
		
		updateUI();
	}

	LLFloater::draw();
}

BOOL LLFloaterBuyCurrencyUI::canClose()
{
	return mManager.canCancel();
}

void LLFloaterBuyCurrencyUI::onClose(bool app_quitting)
{
	LLFloater::onClose(app_quitting);
	destroy();
}

void LLFloaterBuyCurrencyUI::updateUI()
{
	bool hasError = mManager.hasError();
	mManager.updateUI(!hasError && !mManager.buying());

	// section zero: title area
	{
		childSetVisible("info_buying", false);
		childSetVisible("info_cannot_buy", false);
		childSetVisible("info_need_more", false);
		if (hasError)
		{
			childSetVisible("info_cannot_buy", true);
		}
		else if (mHasTarget)
		{
			childSetVisible("info_need_more", true);
		}
		else
		{
			childSetVisible("info_buying", true);
		}
	}
	
	// error section
	if (hasError)
	{
		mChildren.setBadge(std::string("step_error"), LLViewChildren::BADGE_ERROR);
		
		LLTextBox* message = getChild<LLTextBox>("error_message");
		if (message)
		{
			message->setVisible(true);
			message->setWrappedText(mManager.errorMessage());
		}

		childSetVisible("error_web", !mManager.errorURI().empty());
		if (!mManager.errorURI().empty())
		{
			childHide("getting_data");
		}
	}
	else
	{
		childHide("step_error");
		childHide("error_message");
		childHide("error_web");
	}
	
	
	//  currency
	childSetVisible("contacting", false);
	childSetVisible("buy_action", false);
	childSetVisible("buy_action_unknown", false);
	
	if (!hasError)
	{
		mChildren.setBadge(std::string("step_1"), LLViewChildren::BADGE_NOTE);

		if (mManager.buying())
		{
			childSetVisible("contacting", true);
		}
		else
		{
			if (mHasTarget)
			{
				childSetVisible("buy_action", true);
				childSetTextArg("buy_action", "[NAME]", mTargetName);
				childSetTextArg("buy_action", "[PRICE]", llformat("%d",mTargetPrice));
			}
			else
			{
				childSetVisible("buy_action_unknown", true);
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

		childSetVisible("purchase_warning_repurchase", false);
		childSetVisible("purchase_warning_notenough", false);
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
	else
	{
		childHide("step_1");
		childHide("balance_label");
		childHide("balance_amount");
		childHide("buying_label");
		childHide("buying_amount");
		childHide("total_label");
		childHide("total_amount");
		childHide("purchase_warning_repurchase");
		childHide("purchase_warning_notenough");
	}
	
	childSetEnabled("buy_btn", mManager.canBuy());

	if (!mManager.canBuy() && !childIsVisible("error_web"))
	{
		childShow("getting_data");
	}
}

// static
void LLFloaterBuyCurrencyUI::onClickBuy(void* data)
{
	LLFloaterBuyCurrencyUI* self = LLFloaterBuyCurrencyUI::soleInstance(false);
	if (self)
	{
		self->mManager.buy(self->getString("buy_currency"));
		self->updateUI();
		// JC: updateUI() doesn't get called again until progress is made
		// with transaction processing, so the "Purchase" button would be
		// left enabled for some time.  Pre-emptively disable.
		self->childSetEnabled("buy_btn", false);
	}
}

// static
void LLFloaterBuyCurrencyUI::onClickCancel(void* data)
{
	LLFloaterBuyCurrencyUI* self = LLFloaterBuyCurrencyUI::soleInstance(false);
	if (self)
	{
		self->close();
	}
}

// static
void LLFloaterBuyCurrencyUI::onClickErrorWeb(void* data)
{
	LLFloaterBuyCurrencyUI* self = LLFloaterBuyCurrencyUI::soleInstance(false);
	if (self)
	{
		LLWeb::loadURLExternal(self->mManager.errorURI());
		self->close();
	}
}

// static
void LLFloaterBuyCurrency::buyCurrency()
{
	LLFloaterBuyCurrencyUI* ui = LLFloaterBuyCurrencyUI::soleInstance(true);
	ui->noTarget();
	ui->updateUI();
	ui->open();
}

void LLFloaterBuyCurrency::buyCurrency(const std::string& name, S32 price)
{
	LLFloaterBuyCurrencyUI* ui = LLFloaterBuyCurrencyUI::soleInstance(true);
	ui->target(name, price);
	ui->updateUI();
	ui->open();
}


