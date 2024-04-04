/** 
 * @file llfloaterpay.cpp
 * @author Aaron Brashears, Kelly Washington, James Cook
 * @brief Implementation of the LLFloaterPay class.
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

#include "llfloaterpay.h"

#include "message.h"
#include "llfloater.h"
#include "lllslconstants.h"		// MAX_PAY_BUTTONS
#include "lluuid.h"

#include "llagent.h"
#include "llfloaterreg.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "lllineeditor.h"
#include "llmutelist.h"
#include "llnotificationsutil.h"
#include "llfloaterreporter.h"
#include "llslurl.h"
#include "llstatusbar.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llselectmgr.h"
#include "lltransactiontypes.h"
#include "lluictrlfactory.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLGiveMoneyInfo
//
// A small class used to track callback information
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLFloaterPay;

struct LLGiveMoneyInfo
{
	LLFloaterPay* mFloater;
	S32 mAmount;
	LLGiveMoneyInfo(LLFloaterPay* floater, S32 amount) :
		mFloater(floater), mAmount(amount){}
};

typedef std::shared_ptr<LLGiveMoneyInfo> give_money_ptr;

///----------------------------------------------------------------------------
/// Class LLFloaterPay
///----------------------------------------------------------------------------

class LLFloaterPay : public LLFloater
{
public:
	LLFloaterPay(const LLSD& key);
	virtual ~LLFloaterPay();
	/*virtual*/	bool	postBuild();
	/*virtual*/ void onClose(bool app_quitting);
	
	void setCallback(money_callback callback) { mCallback = callback; }
	

	static void payViaObject(money_callback callback, LLSafeHandle<LLObjectSelection> selection);
	
	static void payDirectly(money_callback callback,
							const LLUUID& target_id,
							bool is_group);
	static bool payConfirmationCallback(const LLSD& notification,
										const LLSD& response,
										give_money_ptr info);

private:
	static void onCancel(void* data);
	static void onKeystroke(LLLineEditor* editor, void* data);
	static void onGive(give_money_ptr info);
	void give(S32 amount);
	static void processPayPriceReply(LLMessageSystem* msg, void **userdata);
	void finishPayUI(const LLUUID& target_id, bool is_group);

protected:
	std::vector<give_money_ptr> mCallbackData;
	money_callback mCallback;
	LLTextBox* mObjectNameText;
	LLUUID mTargetUUID;
	bool mTargetIsGroup;
	bool mHaveName;

	LLButton* mQuickPayButton[MAX_PAY_BUTTONS];
	give_money_ptr mQuickPayInfo[MAX_PAY_BUTTONS];

	LLSafeHandle<LLObjectSelection> mObjectSelection;
};


const S32 FASTPAY_BUTTON_WIDTH = 80;
const S32 PAY_AMOUNT_NOTIFICATION = 200;

LLFloaterPay::LLFloaterPay(const LLSD& key)
	: LLFloater(key),
	  mCallbackData(),
	  mCallback(NULL),
	  mObjectNameText(NULL),
	  mTargetUUID(key.asUUID()),
	  mTargetIsGroup(false),
	  mHaveName(false)
{
}

// Destroys the object
LLFloaterPay::~LLFloaterPay()
{
    std::vector<give_money_ptr>::iterator iter;
    for (iter = mCallbackData.begin(); iter != mCallbackData.end(); ++iter)
    {
        (*iter)->mFloater = NULL;
    }
	mCallbackData.clear();
	// Name callbacks will be automatically disconnected since LLFloater is trackable
	
	// In case this floater is currently waiting for a reply.
	gMessageSystem->setHandlerFuncFast(_PREHASH_PayPriceReply, 0, 0);
}

bool LLFloaterPay::postBuild()
{
	S32 i = 0;
	
	give_money_ptr info = give_money_ptr(new LLGiveMoneyInfo(this, PAY_BUTTON_DEFAULT_0));
	mCallbackData.push_back(info);

	childSetAction("fastpay 1", boost::bind(LLFloaterPay::onGive, info));
	getChildView("fastpay 1")->setVisible(false);

	mQuickPayButton[i] = getChild<LLButton>("fastpay 1");
	mQuickPayInfo[i] = info;
	++i;

	info = give_money_ptr(new LLGiveMoneyInfo(this, PAY_BUTTON_DEFAULT_1));
	mCallbackData.push_back(info);

	childSetAction("fastpay 5", boost::bind(LLFloaterPay::onGive, info));
	getChildView("fastpay 5")->setVisible(false);

	mQuickPayButton[i] = getChild<LLButton>("fastpay 5");
	mQuickPayInfo[i] = info;
	++i;

	info = give_money_ptr(new LLGiveMoneyInfo(this, PAY_BUTTON_DEFAULT_2));
	mCallbackData.push_back(info);

	childSetAction("fastpay 10", boost::bind(LLFloaterPay::onGive, info));
	getChildView("fastpay 10")->setVisible(false);

	mQuickPayButton[i] = getChild<LLButton>("fastpay 10");
	mQuickPayInfo[i] = info;
	++i;

	info = give_money_ptr(new LLGiveMoneyInfo(this, PAY_BUTTON_DEFAULT_3));
	mCallbackData.push_back(info);

	childSetAction("fastpay 20", boost::bind(LLFloaterPay::onGive, info));
	getChildView("fastpay 20")->setVisible(false);

	mQuickPayButton[i] = getChild<LLButton>("fastpay 20");
	mQuickPayInfo[i] = info;
	++i;


	getChildView("amount text")->setVisible(false);
	getChildView("amount")->setVisible(false);

	getChild<LLLineEditor>("amount")->setKeystrokeCallback(&LLFloaterPay::onKeystroke, this);
	getChild<LLLineEditor>("amount")->setPrevalidate(LLTextValidate::validateNonNegativeS32);

	info = give_money_ptr(new LLGiveMoneyInfo(this, 0));
	mCallbackData.push_back(info);

	childSetAction("pay btn", boost::bind(LLFloaterPay::onGive, info));
	setDefaultBtn("pay btn");
	getChildView("pay btn")->setVisible(false);
	getChildView("pay btn")->setEnabled(false);

	childSetAction("cancel btn",&LLFloaterPay::onCancel,this);

	return true;
}

// virtual
void LLFloaterPay::onClose(bool app_quitting)
{
	// Deselect the objects
	mObjectSelection = NULL;
}

// static
void LLFloaterPay::processPayPriceReply(LLMessageSystem* msg, void **userdata)
{
	LLFloaterPay* self = (LLFloaterPay*)userdata;
	if (self)
	{
		S32 price;
		LLUUID target;

		msg->getUUIDFast(_PREHASH_ObjectData,_PREHASH_ObjectID,target);
		if (target != self->mTargetUUID)
		{
			// This is a message for a different object's pay info
			return;
		}

		msg->getS32Fast(_PREHASH_ObjectData,_PREHASH_DefaultPayPrice,price);
		
		if (PAY_PRICE_HIDE == price)
		{
			self->getChildView("amount")->setVisible(false);
			self->getChildView("pay btn")->setVisible(false);
			self->getChildView("amount text")->setVisible(false);
		}
		else if (PAY_PRICE_DEFAULT == price)
		{			
			self->getChildView("amount")->setVisible(true);
			self->getChildView("pay btn")->setVisible(true);
			self->getChildView("amount text")->setVisible(true);
		}
		else
		{
			// PAY_PRICE_HIDE and PAY_PRICE_DEFAULT are negative values
			// So we take the absolute value here after we have checked for those cases
			
			self->getChildView("amount")->setVisible(true);
			self->getChildView("pay btn")->setVisible(true);
			self->getChildView("pay btn")->setEnabled(true);
			self->getChildView("amount text")->setVisible(true);

			self->getChild<LLUICtrl>("amount")->setValue(llformat("%d", llabs(price)));
		}

		S32 num_blocks = msg->getNumberOfBlocksFast(_PREHASH_ButtonData);
		S32 i = 0;
		if (num_blocks > MAX_PAY_BUTTONS) num_blocks = MAX_PAY_BUTTONS;

		S32 max_pay_amount = 0;
		S32 padding_required = 0;

		for (i=0;i<num_blocks;++i)
		{
			S32 pay_button;
			msg->getS32Fast(_PREHASH_ButtonData,_PREHASH_PayButton,pay_button,i);
			if (pay_button > 0)
			{
				std::string button_str = "L$";
				button_str += LLResMgr::getInstance()->getMonetaryString( pay_button );

				self->mQuickPayButton[i]->setLabelSelected(button_str);
				self->mQuickPayButton[i]->setLabelUnselected(button_str);
				self->mQuickPayButton[i]->setVisible(true);
				self->mQuickPayInfo[i]->mAmount = pay_button;

				if ( pay_button > max_pay_amount )
				{
					max_pay_amount = pay_button;
				}
			}
			else
			{
				self->mQuickPayButton[i]->setVisible(false);
			}
		}

		// build a string containing the maximum value and calc nerw button width from it.
		std::string balance_str = "L$";
		balance_str += LLResMgr::getInstance()->getMonetaryString( max_pay_amount );
		const LLFontGL* font = LLFontGL::getFontSansSerif();
		S32 new_button_width = font->getWidth( std::string(balance_str));
		new_button_width += ( 12 + 12 );	// padding

		// dialong is sized for 2 digit pay amounts - larger pay values need to be scaled
		const S32 threshold = 100000;
		if ( max_pay_amount >= threshold )
		{
			S32 num_digits_threshold = (S32)log10((double)threshold) + 1;
			S32 num_digits_max = (S32)log10((double)max_pay_amount) + 1;
				
			// calculate the extra width required by 2 buttons with max amount and some commas
			padding_required = ( num_digits_max - num_digits_threshold + ( num_digits_max / 3 ) ) * font->getWidth( std::string("0") );
		};

		// change in button width
		S32 button_delta = new_button_width - FASTPAY_BUTTON_WIDTH;
		if ( button_delta < 0 ) 
			button_delta = 0;

		// now we know the maximum amount, we can resize all the buttons to be 
		for (i=0;i<num_blocks;++i)
		{
			LLRect r;
			r = self->mQuickPayButton[i]->getRect();

			// RHS button colum needs to move further because LHS changed too
			if ( i % 2 )
			{
				r.setCenterAndSize( r.getCenterX() + ( button_delta * 3 ) / 2 , 
					r.getCenterY(), 
						r.getWidth() + button_delta, 
							r.getHeight() ); 
			}
			else
			{
				r.setCenterAndSize( r.getCenterX() + button_delta / 2, 
					r.getCenterY(), 
						r.getWidth() + button_delta, 
						r.getHeight() ); 
			}
			self->mQuickPayButton[i]->setRect( r );
		}

		for (i=num_blocks;i<MAX_PAY_BUTTONS;++i)
		{
			self->mQuickPayButton[i]->setVisible(false);
		}

		self->reshape( self->getRect().getWidth() + padding_required, self->getRect().getHeight(), false );
	}
	msg->setHandlerFunc("PayPriceReply",NULL,NULL);
}

// static
void LLFloaterPay::payViaObject(money_callback callback, LLSafeHandle<LLObjectSelection> selection)
{
	// Object that lead to the selection, may be child
	LLViewerObject* object = selection->getPrimaryObject();
	if (!object)
		return;
	
	LLFloaterPay *floater = LLFloaterReg::showTypedInstance<LLFloaterPay>("pay_object", LLSD(object->getID()));
	if (!floater)
		return;
	
	floater->setCallback(callback);
	// Hold onto the selection until we close
	floater->mObjectSelection = selection;

	LLSelectNode* node = selection->getFirstRootNode();
	if (!node) 
	{
		// object no longer exists
		LLNotificationsUtil::add("PayObjectFailed");
		floater->closeFloater();
		return;
	}
	
	LLHost target_region = object->getRegion()->getHost();
	
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RequestPayPrice);
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addUUIDFast(_PREHASH_ObjectID, object->getID());
	msg->sendReliable(target_region);
	msg->setHandlerFuncFast(_PREHASH_PayPriceReply, processPayPriceReply,(void **)floater);
	
	LLUUID owner_id;
	bool is_group = false;
	node->mPermissions->getOwnership(owner_id, is_group);
	
	floater->getChild<LLUICtrl>("object_name_text")->setValue(node->mName);

	floater->finishPayUI(owner_id, is_group);
}

void LLFloaterPay::payDirectly(money_callback callback, 
							   const LLUUID& target_id,
							   bool is_group)
{
	LLFloaterPay *floater = LLFloaterReg::showTypedInstance<LLFloaterPay>("pay_resident", LLSD(target_id));
	if (!floater)
		return;
	
	floater->setCallback(callback);
	floater->mObjectSelection = NULL;
	
	floater->getChildView("amount")->setVisible(true);
	floater->getChildView("pay btn")->setVisible(true);
	floater->getChildView("amount text")->setVisible(true);

	for(S32 i=0;i<MAX_PAY_BUTTONS;++i)
	{
		floater->mQuickPayButton[i]->setVisible(true);
	}
	
	floater->finishPayUI(target_id, is_group);
}

bool LLFloaterPay::payConfirmationCallback(const LLSD& notification, const LLSD& response, give_money_ptr info)
{
	if (!info.get() || !info->mFloater)
	{
		return false;
	}

	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		info->mFloater->give(info->mAmount);
		info->mFloater->closeFloater();
	}

	return false;
}

void LLFloaterPay::finishPayUI(const LLUUID& target_id, bool is_group)
{
	std::string slurl;
	if (is_group)
	{
		setTitle(getString("payee_group"));
		slurl = LLSLURL("group", target_id, "inspect").getSLURLString();
	}
	else
	{
		setTitle(getString("payee_resident"));
		slurl = LLSLURL("agent", target_id, "inspect").getSLURLString();
	}
	getChild<LLTextBox>("payee_name")->setText(slurl);
	
	// Make sure the amount field has focus

	LLLineEditor* amount = getChild<LLLineEditor>("amount");
	amount->setFocus(true);
	amount->selectAll();

	mTargetIsGroup = is_group;
}

// static
void LLFloaterPay::onCancel(void* data)
{
	LLFloaterPay* self = reinterpret_cast<LLFloaterPay*>(data);
	if(self)
	{
		self->closeFloater();
	}
}

// static
void LLFloaterPay::onKeystroke(LLLineEditor*, void* data)
{
	LLFloaterPay* self = reinterpret_cast<LLFloaterPay*>(data);
	if(self)
	{
		// enable the Pay button when amount is non-empty and positive, disable otherwise
		std::string amtstr = self->getChild<LLUICtrl>("amount")->getValue().asString();
		self->getChildView("pay btn")->setEnabled(!amtstr.empty() && atoi(amtstr.c_str()) > 0);
	}
}

// static
void LLFloaterPay::onGive(give_money_ptr info)
{
    if (!info.get() || !info->mFloater)
    {
        return;
    }

    LLFloaterPay* floater = info->mFloater;
    S32 amount = info->mAmount;
    if (amount == 0)
    {
        LLUICtrl* text_field = floater->getChild<LLUICtrl>("amount");
        if (!text_field)
        {
            return;
        }
        amount = atoi(text_field->getValue().asString().c_str());
    }

    if (amount > PAY_AMOUNT_NOTIFICATION && gStatusBar && gStatusBar->getBalance() > amount)
    {
        LLUUID payee_id = LLUUID::null;
        bool is_group = false;
        if (floater->mObjectSelection.notNull())
        {
            LLSelectNode* node = floater->mObjectSelection->getFirstRootNode();
            if (node)
            {
                node->mPermissions->getOwnership(payee_id, is_group);
            }
            else
            {
                // object no longer exists
                LLNotificationsUtil::add("PayObjectFailed");
                floater->closeFloater();
                return;
            }
        }
        else
        {
            is_group = floater->mTargetIsGroup;
            payee_id = floater->mTargetUUID;
        }

        LLSD args;
        args["TARGET"] = LLSLURL(is_group ? "group" : "agent", payee_id, "completename").getSLURLString();
        args["AMOUNT"] = amount;

        LLNotificationsUtil::add("PayConfirmation", args, LLSD(), boost::bind(&LLFloaterPay::payConfirmationCallback, _1, _2, info));
    }
    else
    {
        floater->give(amount);
        floater->closeFloater();
    }
}

void LLFloaterPay::give(S32 amount)
{
	if(mCallback)
	{
		// if the amount is 0, that menas that we should use the
		// text field.
		if(amount == 0)
		{
			amount = atoi(getChild<LLUICtrl>("amount")->getValue().asString().c_str());
		}

		// Try to pay an object.
		if (mObjectSelection.notNull())
		{
			LLViewerObject* dest_object = gObjectList.findObject(mTargetUUID);
			if(dest_object)
			{
				LLViewerRegion* region = dest_object->getRegion();
				if (region)
				{
					// Find the name of the root object
					LLSelectNode* node = mObjectSelection->getFirstRootNode();
					std::string object_name;
					if (node)
					{
						object_name = node->mName;
					}
					S32 tx_type = TRANS_PAY_OBJECT;
					if(dest_object->isAvatar()) tx_type = TRANS_GIFT;
					mCallback(mTargetUUID, region, amount, false, tx_type, object_name);
					mObjectSelection = NULL;

					// request the object owner in order to check if the owner needs to be unmuted
					LLMessageSystem* msg = gMessageSystem;
					msg->newMessageFast(_PREHASH_RequestObjectPropertiesFamily);
					msg->nextBlockFast(_PREHASH_AgentData);
					msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
					msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
					msg->nextBlockFast(_PREHASH_ObjectData);
					msg->addU32Fast(_PREHASH_RequestFlags, OBJECT_PAY_REQUEST );
					msg->addUUIDFast(_PREHASH_ObjectID, 	mTargetUUID);
					msg->sendReliable( region->getHost() );
				}
			}
			else
			{
				LLNotificationsUtil::add("PayObjectFailed");
			}
		}
		else
		{
			// just transfer the L$
			std::string paymentMessage(getChild<LLLineEditor>("payment_message")->getValue().asString());
			mCallback(mTargetUUID, gAgent.getRegion(), amount, mTargetIsGroup, TRANS_GIFT, (paymentMessage.empty() ? LLStringUtil::null : paymentMessage));

			// check if the payee needs to be unmuted
			LLMuteList::getInstance()->autoRemove(mTargetUUID, LLMuteList::AR_MONEY);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Namespace LLFloaterPayUtil
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void LLFloaterPayUtil::registerFloater()
{
	// Sneaky, use same code but different XML for dialogs
	LLFloaterReg::add("pay_resident", "floater_pay.xml", 
		&LLFloaterReg::build<LLFloaterPay>);
	LLFloaterReg::add("pay_object", "floater_pay_object.xml", 
		&LLFloaterReg::build<LLFloaterPay>);
}

void LLFloaterPayUtil::payViaObject(money_callback callback,
									LLSafeHandle<LLObjectSelection> selection)
{
	LLFloaterPay::payViaObject(callback, selection);
}

void LLFloaterPayUtil::payDirectly(money_callback callback,
								   const LLUUID& target_id,
								   bool is_group)
{
	LLFloaterPay::payDirectly(callback, target_id, is_group);
}
