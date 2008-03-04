/** 
 * @file llcurrencyuimanager.cpp
 * @brief LLCurrencyUIManager class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#include "lluictrlfactory.h"
#include "lltextbox.h"
#include "lllineeditor.h"

#include "llcurrencyuimanager.h"

// viewer includes
#include "llagent.h"
#include "llconfirmationmanager.h"
#include "llframetimer.h"
#include "lllineeditor.h"
#include "llviewchildren.h"
#include "llxmlrpctransaction.h"
#include "llappviewer.h"


const F64 CURRENCY_ESTIMATE_FREQUENCY = 2.0;
	// how long of a pause in typing a currency buy amount before an
	// esimate is fetched from the server

class LLCurrencyUIManager::Impl
{
public:
	Impl(LLPanel& dialog);
	virtual ~Impl();

	LLPanel&		mPanel;

	bool			mHidden;
	
	bool			mError;
	std::string		mErrorMessage;
	std::string		mErrorURI;
	
	std::string		mZeroMessage;
	
	// user's choices
	S32				mUserCurrencyBuy;
	bool			mUserEnteredCurrencyBuy;
	
	// from website
	bool			mSiteCurrencyEstimated;
	S32				mSiteCurrencyEstimatedCost;
	std::string		mSiteConfirm;
	
	bool			mBought;
	
	enum TransactionType
	{
		TransactionNone,
		TransactionCurrency,
		TransactionBuy
	};

	TransactionType		 mTransactionType;
	LLXMLRPCTransaction* mTransaction;
	
	bool		 mCurrencyChanged;
	LLFrameTimer mCurrencyKeyTimer;


	void updateCurrencyInfo();
	void finishCurrencyInfo();
	
	void startCurrencyBuy(const std::string& password);
	void finishCurrencyBuy();
	
	void startTransaction(TransactionType type,
		const char* method, LLXMLRPCValue params);
	bool checkTransaction();
		// return true if update needed
	
	void setError(const std::string& message, const std::string& uri);
	void clearError();

	bool considerUpdateCurrency();
		// return true if update needed
	void currencyKey(S32);
	static void onCurrencyKey(LLLineEditor* caller, void* data);	

	void prepare();
	void updateUI();
};

// is potentially not fully constructed.
LLCurrencyUIManager::Impl::Impl(LLPanel& dialog)
	: mPanel(dialog),
	mHidden(false),
	mError(false),
	mUserCurrencyBuy(1000), mUserEnteredCurrencyBuy(false),
	mSiteCurrencyEstimated(false),
	mBought(false),
	mTransactionType(TransactionNone), mTransaction(0),
	mCurrencyChanged(false)
{
}

LLCurrencyUIManager::Impl::~Impl()
{
	delete mTransaction;
}

void LLCurrencyUIManager::Impl::updateCurrencyInfo()
{
	mSiteCurrencyEstimated = false;
	mSiteCurrencyEstimatedCost = 0;
	mBought = false;
	mCurrencyChanged = false;

	if (mUserCurrencyBuy == 0)
	{
		mSiteCurrencyEstimated = true;
		return;
	}
	
	LLXMLRPCValue keywordArgs = LLXMLRPCValue::createStruct();
	keywordArgs.appendString("agentId", gAgent.getID().asString());
	keywordArgs.appendString(
		"secureSessionId",
		gAgent.getSecureSessionID().asString());
	keywordArgs.appendInt("currencyBuy", mUserCurrencyBuy);
	
	LLXMLRPCValue params = LLXMLRPCValue::createArray();
	params.append(keywordArgs);

	startTransaction(TransactionCurrency, "getCurrencyQuote", params);
}

void LLCurrencyUIManager::Impl::finishCurrencyInfo()
{
	LLXMLRPCValue result = mTransaction->responseValue();

	bool success = result["success"].asBool();
	if (!success)
	{
		setError(
			result["errorMessage"].asString(),
			result["errorURI"].asString()
		);
		return;
	}
	
	LLXMLRPCValue currency = result["currency"];
	mSiteCurrencyEstimated = true;
	mSiteCurrencyEstimatedCost = currency["estimatedCost"].asInt();
	
	S32 newCurrencyBuy = currency["currencyBuy"].asInt();
	if (newCurrencyBuy != mUserCurrencyBuy)
	{
		mUserCurrencyBuy = newCurrencyBuy;
		mUserEnteredCurrencyBuy = false;
	}

	mSiteConfirm = result["confirm"].asString();
}

void LLCurrencyUIManager::Impl::startCurrencyBuy(const std::string& password)
{
	mSiteCurrencyEstimated = false;
	mSiteCurrencyEstimatedCost = 0;
	mCurrencyChanged = false;
	
	LLXMLRPCValue keywordArgs = LLXMLRPCValue::createStruct();
	keywordArgs.appendString("agentId", gAgent.getID().asString());
	keywordArgs.appendString(
		"secureSessionId",
		gAgent.getSecureSessionID().asString());
	keywordArgs.appendInt("currencyBuy", mUserCurrencyBuy);
	keywordArgs.appendInt("estimatedCost", mSiteCurrencyEstimatedCost);
	keywordArgs.appendString("confirm", mSiteConfirm);
	if (!password.empty())
	{
		keywordArgs.appendString("password", password);
	}
	
	LLXMLRPCValue params = LLXMLRPCValue::createArray();
	params.append(keywordArgs);

	startTransaction(TransactionBuy, "buyCurrency", params);
}

void LLCurrencyUIManager::Impl::finishCurrencyBuy()
{
	LLXMLRPCValue result = mTransaction->responseValue();

	bool success = result["success"].asBool();
	if (!success)
	{
		setError(
			result["errorMessage"].asString(),
			result["errorURI"].asString()
		);
	}
	else
	{
		mUserCurrencyBuy = 0;
		mUserEnteredCurrencyBuy = false;
		mBought = true;
	}
}

void LLCurrencyUIManager::Impl::startTransaction(TransactionType type,
		const char* method, LLXMLRPCValue params)
{
	static std::string transactionURI;
	if (transactionURI.empty())
	{
		transactionURI = LLAppViewer::instance()->getHelperURI() + "currency.php";
	}

	delete mTransaction;

	mTransactionType = type;
	mTransaction = new LLXMLRPCTransaction(
		transactionURI,
		method,
		params,
		false /* don't use gzip */
		);

	clearError();
}

bool LLCurrencyUIManager::Impl::checkTransaction()
{
	if (!mTransaction)
	{
		return false;
	}
	
	if (!mTransaction->process())
	{
		return false;
	}

	if (mTransaction->status(NULL) != LLXMLRPCTransaction::StatusComplete)
	{
		setError(mTransaction->statusMessage(), mTransaction->statusURI());
	}
	else {
		switch (mTransactionType)
		{	
			case TransactionCurrency:	finishCurrencyInfo();	break;
			case TransactionBuy:		finishCurrencyBuy();	break;
			default: ;
		}
	}
	
	delete mTransaction;
	mTransaction = NULL;
	mTransactionType = TransactionNone;
	
	return true;
}

void LLCurrencyUIManager::Impl::setError(
	const std::string& message, const std::string& uri)
{
	mError = true;
	mErrorMessage = message;
	mErrorURI = uri;
}

void LLCurrencyUIManager::Impl::clearError()
{
	mError = false;
	mErrorMessage.clear();
	mErrorURI.clear();
}

bool LLCurrencyUIManager::Impl::considerUpdateCurrency()
{
	if (mCurrencyChanged
	&&  !mTransaction 
	&&  mCurrencyKeyTimer.getElapsedTimeF32() >= CURRENCY_ESTIMATE_FREQUENCY)
	{
		updateCurrencyInfo();
		return true;
	}

	return false;
}

void LLCurrencyUIManager::Impl::currencyKey(S32 value)
{
	mUserEnteredCurrencyBuy = true;
	mCurrencyKeyTimer.reset();

	if (mUserCurrencyBuy == value)
	{
		return;
	}

	mUserCurrencyBuy = value;
	
	if (mSiteCurrencyEstimated) {
		mSiteCurrencyEstimated = false;
		//cannot just simply refresh the whole UI, as the edit field will
		// get reset and the cursor will change...
		
		mPanel.childHide("currency_est");
		mPanel.childSetVisible("getting_data",TRUE);
	}
	
	mCurrencyChanged = true;
}

// static
void LLCurrencyUIManager::Impl::onCurrencyKey(
		LLLineEditor* caller, void* data)
{
	S32 value = atoi(caller->getText().c_str());
	LLCurrencyUIManager::Impl* self = (LLCurrencyUIManager::Impl*)data;
	self->currencyKey(value);
}

void LLCurrencyUIManager::Impl::prepare()
{
	LLLineEditor* lindenAmount = LLUICtrlFactory::getLineEditorByName(&mPanel,"currency_amt");
	if (lindenAmount)
	{
		lindenAmount->setPrevalidate(LLLineEditor::prevalidateNonNegativeS32);
		lindenAmount->setKeystrokeCallback(onCurrencyKey);
		lindenAmount->setCallbackUserData(this);
	}
}

void LLCurrencyUIManager::Impl::updateUI()
{
	if (mHidden)
	{
		mPanel.childHide("currency_action");
		mPanel.childHide("currency_amt");
		mPanel.childHide("currency_est");
		return;
	}

	mPanel.childShow("currency_action");

	LLLineEditor* lindenAmount = LLUICtrlFactory::getLineEditorByName(&mPanel,"currency_amt");
	if (lindenAmount) 
	{
		lindenAmount->setVisible(true);
		lindenAmount->setLabel(mZeroMessage);
		
		if (!mUserEnteredCurrencyBuy)
		{
			if (!mZeroMessage.empty() && mUserCurrencyBuy == 0)
			{
				lindenAmount->setText(LLString::null);
			}
			else
			{
				lindenAmount->setText(llformat("%d", mUserCurrencyBuy));
			}

			lindenAmount->selectAll();
		}
	}

	mPanel.childSetTextArg("currency_est", "[USD]", llformat("%#.2f", mSiteCurrencyEstimatedCost / 100.0));
	mPanel.childSetVisible("currency_est", mSiteCurrencyEstimated && mUserCurrencyBuy > 0);

	if (mPanel.childIsEnabled("buy_btn")
		||mPanel.childIsVisible("currency_est")
		|| mPanel.childIsVisible("error_web"))
	{
		mPanel.childSetVisible("getting_data",FALSE);
	}
}



LLCurrencyUIManager::LLCurrencyUIManager(LLPanel& dialog)
	: impl(* new Impl(dialog))
{
}

LLCurrencyUIManager::~LLCurrencyUIManager()
{
	delete &impl;
}

void LLCurrencyUIManager::setAmount(int amount, bool noEstimate)
{
	impl.mUserCurrencyBuy = amount;
	impl.mUserEnteredCurrencyBuy = false;
	impl.updateUI();
	
	impl.mCurrencyChanged = !noEstimate;
}

int LLCurrencyUIManager::getAmount()
{
	return impl.mUserCurrencyBuy;
}

void LLCurrencyUIManager::setZeroMessage(const std::string& message)
{
	impl.mZeroMessage = message;
}

void LLCurrencyUIManager::setEstimate(int amount)
{
	impl.mSiteCurrencyEstimatedCost = amount;
	impl.mSiteCurrencyEstimated = true;
	impl.updateUI();
	
	impl.mCurrencyChanged = false;
}

int LLCurrencyUIManager::getEstimate()
{
	return impl.mSiteCurrencyEstimated ? impl.mSiteCurrencyEstimatedCost : 0;
}

void LLCurrencyUIManager::prepare()
{
	impl.prepare();
}

void LLCurrencyUIManager::updateUI(bool show)
{
	impl.mHidden = !show;
	impl.updateUI();
}

bool LLCurrencyUIManager::process()
{
	bool changed = false;
	changed |= impl.checkTransaction();
	changed |= impl.considerUpdateCurrency();
	return changed;
}

void LLCurrencyUIManager::buy(const LLString& buy_msg)
{
	if (!canBuy())
	{
		return;
	}

	LLUIString msg = buy_msg;
	msg.setArg("[LINDENS]", llformat("%d", impl.mUserCurrencyBuy));
	msg.setArg("[USD]", llformat("%#.2f", impl.mSiteCurrencyEstimatedCost / 100.0));
	LLConfirmationManager::confirm(impl.mSiteConfirm,
								   msg,
								   impl,
								   &LLCurrencyUIManager::Impl::startCurrencyBuy);
}


bool LLCurrencyUIManager::inProcess()
{
	return impl.mTransactionType != Impl::TransactionNone;
}

bool LLCurrencyUIManager::canCancel()
{
	return impl.mTransactionType != Impl::TransactionBuy;
}

bool LLCurrencyUIManager::canBuy()
{
	return impl.mTransactionType == Impl::TransactionNone
		&& impl.mSiteCurrencyEstimated
		&& impl.mUserCurrencyBuy > 0;
}

bool LLCurrencyUIManager::buying()
{
	return impl.mTransactionType == Impl::TransactionBuy;
}

bool LLCurrencyUIManager::bought()
{
	return impl.mBought;
}

bool LLCurrencyUIManager::hasError()
{
	return impl.mError;
}

std::string LLCurrencyUIManager::errorMessage()
{
	return impl.mErrorMessage;
}

std::string LLCurrencyUIManager::errorURI()
{
	return impl.mErrorURI;
}


