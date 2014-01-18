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
#include "llfloaterreporter.h"
#include "llslurl.h"
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

///----------------------------------------------------------------------------
/// Class LLFloaterPay
///----------------------------------------------------------------------------

class LLFloaterPay : public LLFloater
{
public:
	LLFloaterPay(const LLSD& key);
	virtual ~LLFloaterPay();
	/*virtual*/	BOOL	postBuild();
	/*virtual*/ void onClose(bool app_quitting);
	
	void setCallback(money_callback callback) { mCallback = callback; }
	

	static void payViaObject(money_callback callback, LLSafeHandle<LLObjectSelection> selection);
	
	static void payDirectly(money_callback callback,
							const LLUUID& target_id,
							bool is_group);

private:
	static void onCancel(void* data);
	static void onKeystroke(LLLineEditor* editor, void* data);
	static void onGive(void* data);
	void give(S32 amount);
	static void processPayPriceReply(LLMessageSystem* msg, void **userdata);
	void finishPayUI(const LLUUID& target_id, BOOL is_group);

protected:
	std::vector<LLGiveMoneyInfo*> mCallbackData;
	money_callback mCallback;
	LLTextBox* mObjectNameText;
	LLUUID mTargetUUID;
	BOOL mTargetIsGroup;
	BOOL mHaveName;

	LLButton* mQuickPayButton[MAX_PAY_BUTTONS];
	LLGiveMoneyInfo* mQuickPayInfo[MAX_PAY_BUTTONS];

	LLSafeHandle<LLObjectSelection> mObjectSelection;

	static S32 sLastAmount;
};


S32 LLFloaterPay::sLastAmount = 0;
const S32 MAX_AMOUNT_LENGTH = 10;
const S32 FASTPAY_BUTTON_WIDTH = 80;

LLFloaterPay::LLFloaterPay(const LLSD& key)
	: LLFloater(key),
	  mCallbackData(),
	  mCallback(NULL),
	  mObjectNameText(NULL),
	  mTargetUUID(key.asUUID()),
	  mTargetIsGroup(FALSE),
	  mHaveName(FALSE)
{
}

// Destroys the object
LLFloaterPay::~LLFloaterPay()
{
	std::for_each(mCallbackData.begin(), mCallbackData.end(), DeletePointer());
	// Name callbacks will be automatically disconnected since LLFloater is trackable
	
	// In case this floater is currently waiting for a reply.
	gMessageSystem->setHandlerFuncFast(_PREHASH_PayPriceReply, 0, 0);
}

BOOL LLFloaterPay::postBuild()
{
	S32 i = 0;
	
	LLGiveMoneyInfo* info = new LLGiveMoneyInfo(this, PAY_BUTTON_DEFAULT_0);
	mCallbackData.push_back(info);

	childSetAction("fastpay 1",&LLFloaterPay::onGive,info);
	getChildView("fastpay 1")->setVisible(FALSE);

	mQuickPayButton[i] = getChild<LLButton>("fastpay 1");
	mQuickPayInfo[i] = info;
	++i;

	info = new LLGiveMoneyInfo(this, PAY_BUTTON_DEFAULT_1);
	mCallbackData.push_back(info);

	childSetAction("fastpay 5",&LLFloaterPay::onGive,info);
	getChildView("fastpay 5")->setVisible(FALSE);

	mQuickPayButton[i] = getChild<LLButton>("fastpay 5");
	mQuickPayInfo[i] = info;
	++i;

	info = new LLGiveMoneyInfo(this, PAY_BUTTON_DEFAULT_2);
	mCallbackData.push_back(info);

	childSetAction("fastpay 10",&LLFloaterPay::onGive,info);
	getChildView("fastpay 10")->setVisible(FALSE);

	mQuickPayButton[i] = getChild<LLButton>("fastpay 10");
	mQuickPayInfo[i] = info;
	++i;

	info = new LLGiveMoneyInfo(this, PAY_BUTTON_DEFAULT_3);
	mCallbackData.push_back(info);

	childSetAction("fastpay 20",&LLFloaterPay::onGive,info);
	getChildView("fastpay 20")->setVisible(FALSE);

	mQuickPayButton[i] = getChild<LLButton>("fastpay 20");
	mQuickPayInfo[i] = info;
	++i;


	getChildView("amount text")->setVisible(FALSE);	

	std::string last_amount;
	if(sLastAmount > 0)
	{
		last_amount = llformat("%d", sLastAmount);
	}

	getChildView("amount")->setVisible(FALSE);

	getChild<LLLineEditor>("amount")->setKeystrokeCallback(&LLFloaterPay::onKeystroke, this);
	getChild<LLUICtrl>("amount")->setValue(last_amount);
	getChild<LLLineEditor>("amount")->setPrevalidate(LLTextValidate::validateNonNegativeS32);

	info = new LLGiveMoneyInfo(this, 0);
	mCallbackData.push_back(info);

	childSetAction("pay btn",&LLFloaterPay::onGive,info);
	setDefaultBtn("pay btn");
	getChildView("pay btn")->setVisible(FALSE);
	getChildView("pay btn")->setEnabled((sLastAmount > 0));

	childSetAction("cancel btn",&LLFloaterPay::onCancel,this);

	return TRUE;
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
			self->getChildView("amount")->setVisible(FALSE);
			self->getChildView("pay btn")->setVisible(FALSE);
			self->getChildView("amount text")->setVisible(FALSE);
		}
		else if (PAY_PRICE_DEFAULT == price)
		{			
			self->getChildView("amount")->setVisible(TRUE);
			self->getChildView("pay btn")->setVisible(TRUE);
			self->getChildView("amount text")->setVisible(TRUE);
		}
		else
		{
			// PAY_PRICE_HIDE and PAY_PRICE_DEFAULT are negative values
			// So we take the absolute value here after we have checked for those cases
			
			self->getChildView("amount")->setVisible(TRUE);
			self->getChildView("pay btn")->setVisible(TRUE);
			self->getChildView("pay btn")->setEnabled(TRUE);
			self->getChildView("amount text")->setVisible(TRUE);

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
				self->mQuickPayButton[i]->setVisible(TRUE);
				self->mQuickPayInfo[i]->mAmount = pay_button;
				self->getChildView("fastpay text")->setVisible(TRUE);

				if ( pay_button > max_pay_amount )
				{
					max_pay_amount = pay_button;
				}
			}
			else
			{
				self->mQuickPayButton[i]->setVisible(FALSE);
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
			self->mQuickPayButton[i]->setVisible(FALSE);
		}

		self->reshape( self->getRect().getWidth() + padding_required, self->getRect().getHeight(), FALSE );
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
		//FIXME: notify user object no longer exists
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
	BOOL is_group = FALSE;
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
	
	floater->getChildView("amount")->setVisible(TRUE);
	floater->getChildView("pay btn")->setVisible(TRUE);
	floater->getChildView("amount text")->setVisible(TRUE);

	floater->getChildView("fastpay text")->setVisible(TRUE);
	for(S32 i=0;i<MAX_PAY_BUTTONS;++i)
	{
		floater->mQuickPayButton[i]->setVisible(TRUE);
	}
	
	floater->finishPayUI(target_id, is_group);
}
	
void LLFloaterPay::finishPayUI(const LLUUID& target_id, BOOL is_group)
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
	amount->setFocus(TRUE);
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
void LLFloaterPay::onGive(void* data)
{
	LLGiveMoneyInfo* info = reinterpret_cast<LLGiveMoneyInfo*>(data);
	if(info && info->mFloater)
	{
		info->mFloater->give(info->mAmount);
		info->mFloater->closeFloater();
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
		sLastAmount = amount;

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
					mCallback(mTargetUUID, region, amount, FALSE, tx_type, object_name);
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
		}
		else
		{
			// just transfer the L$
			mCallback(mTargetUUID, gAgent.getRegion(), amount, mTargetIsGroup, TRANS_GIFT, LLStringUtil::null);

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
