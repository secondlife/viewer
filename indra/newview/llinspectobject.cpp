/** 
 * @file llinspectobject.cpp
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llinspectobject.h"

// Viewer
#include "llnotifications.h"	// *TODO: Eliminate, add LLNotificationsUtil wrapper
#include "llselectmgr.h"
#include "llslurl.h"
#include "llviewermenu.h"		// handle_object_touch(), handle_buy()
#include "llviewerobjectlist.h"	// to select the requested object

// Linden libraries
#include "llbutton.h"			// setLabel(), not virtual!
#include "llclickaction.h"
#include "llcontrol.h"			// LLCachedControl
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llresmgr.h"			// getMonetaryString
#include "llsafehandle.h"
#include "lltextbox.h"			// for description truncation
#include "lltrans.h"
#include "llui.h"				// positionViewNearMouse()
#include "lluictrl.h"

class LLViewerObject;

// *TODO: Abstract out base class for LLInspectObject and LLInspectObject

//////////////////////////////////////////////////////////////////////////////
// LLInspectObject
//////////////////////////////////////////////////////////////////////////////

// Object Inspector, a small information window used when clicking
// in the ambient inspector widget for objects in the 3D world.
class LLInspectObject : public LLFloater
{
	friend class LLFloaterReg;
	
public:
	// object_id - Root object ID for which to show information
	// Inspector will be positioned relative to current mouse position
	LLInspectObject(const LLSD& object_id);
	virtual ~LLInspectObject();
	
	/*virtual*/ BOOL postBuild(void);
	/*virtual*/ void draw();
	
	// Because floater is single instance, need to re-parse data on each spawn
	// (for example, inspector about same avatar but in different position)
	/*virtual*/ void onOpen(const LLSD& avatar_id);
	
	// Release the selection and do other cleanup
	void onClose();
	
	// Inspectors close themselves when they lose focus
	/*virtual*/ void onFocusLost();
	
private:
	// Refresh displayed data with information from selection manager
	void update();

	void hideButtons();
	void updateButtons(LLSelectNode* nodep);
	void updateSitLabel(LLSelectNode* nodep);
	void updateTouchLabel(LLSelectNode* nodep);

	void updateName(LLSelectNode* nodep);
	void updateDescription(LLSelectNode* nodep);
	void updatePrice(LLSelectNode* nodep);
	
	void updateCreator(LLSelectNode* nodep);
		
	void onClickBuy();
	void onClickPay();
	void onClickTakeFreeCopy();
	void onClickTouch();
	void onClickSit();
	void onClickOpen();
	void onClickMoreInfo();
	
private:
	LLUUID				mObjectID;
	LLFrameTimer		mOpenTimer;
	LLFrameTimer		mCloseTimer;
	LLSafeHandle<LLObjectSelection> mObjectSelection;
};

LLInspectObject::LLInspectObject(const LLSD& sd)
:	LLFloater( LLSD() ),	// single_instance, doesn't really need key
	mObjectID(),			// set in onOpen()
	mCloseTimer(),
	mOpenTimer()
{
	// can't make the properties request until the widgets are constructed
	// as it might return immediately, so do it in postBuild.
	mCommitCallbackRegistrar.add("InspectObject.Buy",	boost::bind(&LLInspectObject::onClickBuy, this));	
	mCommitCallbackRegistrar.add("InspectObject.Pay",	boost::bind(&LLInspectObject::onClickPay, this));	
	mCommitCallbackRegistrar.add("InspectObject.TakeFreeCopy",	boost::bind(&LLInspectObject::onClickTakeFreeCopy, this));	
	mCommitCallbackRegistrar.add("InspectObject.Touch",	boost::bind(&LLInspectObject::onClickTouch, this));	
	mCommitCallbackRegistrar.add("InspectObject.Sit",	boost::bind(&LLInspectObject::onClickSit, this));	
	mCommitCallbackRegistrar.add("InspectObject.Open",	boost::bind(&LLInspectObject::onClickOpen, this));	
	mCommitCallbackRegistrar.add("InspectObject.MoreInfo",	boost::bind(&LLInspectObject::onClickMoreInfo, this));	
}


LLInspectObject::~LLInspectObject()
{
}

/*virtual*/
BOOL LLInspectObject::postBuild(void)
{
	// The XML file has sample data in it.  Clear that out so we don't
	// flicker when data arrives off network.
	getChild<LLUICtrl>("object_name")->setValue("");
	getChild<LLUICtrl>("object_creator")->setValue("");
	getChild<LLUICtrl>("object_description")->setValue("");

	// Set buttons invisible until we know what this object can do
	hideButtons();

	// Hide floater when name links clicked
	LLTextBox* textbox = getChild<LLTextBox>("object_creator");
	textbox->mURLClickSignal.connect(
		boost::bind(&LLInspectObject::closeFloater, this, false) );

	// Hook up functionality
	getChild<LLUICtrl>("buy_btn")->setCommitCallback(
		boost::bind(&LLInspectObject::onClickBuy, this));
	getChild<LLUICtrl>("pay_btn")->setCommitCallback(
		boost::bind(&LLInspectObject::onClickPay, this));
	getChild<LLUICtrl>("take_free_copy_btn")->setCommitCallback(
		boost::bind(&LLInspectObject::onClickTakeFreeCopy, this));
	getChild<LLUICtrl>("touch_btn")->setCommitCallback(
		boost::bind(&LLInspectObject::onClickTouch, this));
	getChild<LLUICtrl>("sit_btn")->setCommitCallback(
		boost::bind(&LLInspectObject::onClickSit, this));
	getChild<LLUICtrl>("open_btn")->setCommitCallback(
		boost::bind(&LLInspectObject::onClickOpen, this));
	getChild<LLUICtrl>("more_info_btn")->setCommitCallback(
		boost::bind(&LLInspectObject::onClickMoreInfo, this));

	// Watch for updates to selection properties off the network
	LLSelectMgr::getInstance()->mUpdateSignal.connect(
		boost::bind(&LLInspectObject::update, this) );

	mCloseSignal.connect( boost::bind(&LLInspectObject::onClose, this) );

	return TRUE;
}

void LLInspectObject::draw()
{
	static LLCachedControl<F32> FADE_OUT_TIME(*LLUI::sSettingGroups["config"], "InspectorFadeTime", 1.f);
	if (mOpenTimer.getStarted())
	{
		F32 alpha = clamp_rescale(mOpenTimer.getElapsedTimeF32(), 0.f, FADE_OUT_TIME, 0.f, 1.f);
		LLViewDrawContext context(alpha);
		LLFloater::draw();
	}
	else if (mCloseTimer.getStarted())
	{
		F32 alpha = clamp_rescale(mCloseTimer.getElapsedTimeF32(), 0.f, FADE_OUT_TIME, 1.f, 0.f);
		LLViewDrawContext context(alpha);
		LLFloater::draw();
		if (mCloseTimer.getElapsedTimeF32() > FADE_OUT_TIME)
		{
			closeFloater(false);
		}
	}
	else
	{
		LLFloater::draw();
	}
}


// Multiple calls to showInstance("inspect_avatar", foo) will provide different
// LLSD for foo, which we will catch here.
//virtual
void LLInspectObject::onOpen(const LLSD& data)
{
	mCloseTimer.stop();
	mOpenTimer.start();

	// Extract appropriate avatar id
	mObjectID = data["object_id"];

	// Position the inspector relative to the mouse cursor
	// Similar to how tooltips are positioned
	// See LLToolTipMgr::createToolTip
	if (data.has("pos"))
	{
		LLUI::positionViewNearMouse(this, data["pos"]["x"].asInteger(), data["pos"]["y"].asInteger());
	}
	else
	{
		LLUI::positionViewNearMouse(this);
	}

	// Promote hovered object to a complete selection, which will also force
	// a request for selected object data off the network
	LLViewerObject* obj = gObjectList.findObject( mObjectID );
	if (obj)
	{
		LLSelectMgr::instance().deselectAll();
		mObjectSelection = LLSelectMgr::instance().selectObjectAndFamily(obj);

		// Mark this as a transient selection
		struct SetTransient : public LLSelectedNodeFunctor
		{
			bool apply(LLSelectNode* node)
			{
				node->setTransient(TRUE);
				return true;
			}
		} functor;
		mObjectSelection->applyToNodes(&functor);
	}
}

void LLInspectObject::onClose()
{
	// Release selection to deselect
	mObjectSelection = NULL;
}

//virtual
void LLInspectObject::onFocusLost()
{
	// Start closing when we lose focus
	mCloseTimer.start();
	mOpenTimer.stop();
}


void LLInspectObject::update()
{
	// Performance optimization, because we listen to updates from select mgr
	// but we're never destroyed.
	if (!getVisible()) return;

	LLObjectSelection* selection = LLSelectMgr::getInstance()->getSelection();
	if (!selection) return;

	LLSelectNode* nodep = selection->getFirstRootNode();
	if (!nodep) return;

	updateButtons(nodep);
	updateName(nodep);
	updateDescription(nodep);
	updateCreator(nodep);
	updatePrice(nodep);
}

void LLInspectObject::hideButtons()
{
	getChild<LLUICtrl>("buy_btn")->setVisible(false);
	getChild<LLUICtrl>("pay_btn")->setVisible(false);
	getChild<LLUICtrl>("take_free_copy_btn")->setVisible(false);
	getChild<LLUICtrl>("touch_btn")->setVisible(false);
	getChild<LLUICtrl>("sit_btn")->setVisible(false);
	getChild<LLUICtrl>("open_btn")->setVisible(false);
}

// *TODO: Extract this method from lltoolpie.cpp and put somewhere shared
extern U8 final_click_action(LLViewerObject*);

// Choose the "most relevant" operation for this object, and show a button for
// that operation as the left-most button in the inspector.
void LLInspectObject::updateButtons(LLSelectNode* nodep)
{
	// We'll start with everyone hidden and show the ones we need
	hideButtons();
	
	LLViewerObject* object = nodep->getObject();
	LLViewerObject *parent = (LLViewerObject*)object->getParent();
	bool for_copy = anyone_copy_selection(nodep);
	bool for_sale = enable_buy_object();
	S32 price = nodep->mSaleInfo.getSalePrice();
	U8 click_action = final_click_action(object);

	if (for_copy
		|| (for_sale && price == 0))
	{
		// Free copies have priority over other operations
		getChild<LLUICtrl>("take_free_copy_btn")->setVisible(true);
	}
	else if (for_sale)
	{
		getChild<LLUICtrl>("buy_btn")->setVisible(true);
	}
	else if ( enable_pay_object() )
	{
		getChild<LLUICtrl>("pay_btn")->setVisible(true);
	}
	else if (click_action == CLICK_ACTION_SIT)
	{
		// Click-action sit must come before "open" because many objects on
		// which you can sit have scripts, and hence can be opened
		getChild<LLUICtrl>("sit_btn")->setVisible(true);
		updateSitLabel(nodep);
	}
	else if (object->flagHandleTouch()
		|| (parent && parent->flagHandleTouch()))
	{
		getChild<LLUICtrl>("touch_btn")->setVisible(true);
		updateTouchLabel(nodep);
	}
	else if ( enable_object_open() )
	{
		// Open is last because anything with a script in it can be opened
		getChild<LLUICtrl>("open_btn")->setVisible(true);
	}
	else
	{
		// By default, we can sit on anything
		getChild<LLUICtrl>("sit_btn")->setVisible(true);
		updateSitLabel(nodep);
	}

	// No flash
	focusFirstItem(FALSE, FALSE);
}

void LLInspectObject::updateSitLabel(LLSelectNode* nodep)
{
	LLButton* sit_btn = getChild<LLButton>("sit_btn");
	if (!nodep->mSitName.empty())
	{
		sit_btn->setLabel( nodep->mSitName );
	}
	else
	{
		sit_btn->setLabel( getString("Sit") );
	}
}

void LLInspectObject::updateTouchLabel(LLSelectNode* nodep)
{
	LLButton* sit_btn = getChild<LLButton>("touch_btn");
	if (!nodep->mTouchName.empty())
	{
		sit_btn->setLabel( nodep->mTouchName );
	}
	else
	{
		sit_btn->setLabel( getString("Touch") );
	}
}

void LLInspectObject::updateName(LLSelectNode* nodep)
{
	std::string name;
	if (!nodep->mName.empty())
	{
		name = nodep->mName;
	}
	else
	{
		name = LLTrans::getString("TooltipNoName");
	}
	getChild<LLUICtrl>("object_name")->setValue(name);
}

void LLInspectObject::updateDescription(LLSelectNode* nodep)
{
	const char* const DEFAULT_DESC = "(No Description)";
	std::string desc;
	if (!nodep->mDescription.empty()
		&& nodep->mDescription != DEFAULT_DESC)
	{
		desc = nodep->mDescription;
	}

	LLTextBox* textbox = getChild<LLTextBox>("object_description");
	textbox->setValue(desc);

	// Truncate description text to fit in widget
	// *HACK: OMG, use lower-left corner to truncate text
	// Don't round the position, we want the left of the character
	S32 corner_index = textbox->getDocIndexFromLocalCoord( 0, 0, FALSE);
	LLWString desc_wide = textbox->getWText();
	// index == length if position is past last character
	if (corner_index < (S32)desc_wide.length())
	{
		desc_wide = desc_wide.substr(0, corner_index);
		textbox->setWText(desc_wide);
	}
}

void LLInspectObject::updateCreator(LLSelectNode* nodep)
{
	// final information for display
	LLStringUtil::format_map_t args;
	std::string text;
	
	// Leave text blank until data loaded
	if (nodep->mValid)
	{
		// Utilize automatic translation of SLURL into name to display 
		// a clickable link		
		// Objects cannot be created by a group, so use agent URL format
		LLUUID creator_id = nodep->mPermissions->getCreator();
		std::string creator_url =
			LLSLURL::buildCommand("agent", creator_id, "about");
		args["[CREATOR]"] = creator_url;
				
		// created by one user but owned by another
		std::string owner_url;
		LLUUID owner_id;
		bool group_owned = nodep->mPermissions->isGroupOwned();
		if (group_owned)
		{
			owner_id = nodep->mPermissions->getGroup();
			owner_url =	LLSLURL::buildCommand("group", owner_id, "about");
		}
		else
		{
			owner_id = nodep->mPermissions->getOwner();
			owner_url =	LLSLURL::buildCommand("agent", owner_id, "about");
		}
		args["[OWNER]"] = owner_url;
		
		if (creator_id == owner_id)
		{
			// common case, created and owned by one user
			text = getString("Creator", args);
		}
		else
		{
			text = getString("CreatorAndOwner", args);
		}
	}
	getChild<LLUICtrl>("object_creator")->setValue(text);
}

void LLInspectObject::updatePrice(LLSelectNode* nodep)
{
	// *TODO: Only look these up once and use for both updateButtons and here
	bool for_copy = anyone_copy_selection(nodep);
	bool for_sale = enable_buy_object();
	S32 price = nodep->mSaleInfo.getSalePrice();
	
	bool show_price_icon = false;
	std::string line;
	if (for_copy
		|| (for_sale && price == 0))
	{
		line = getString("PriceFree");
		show_price_icon = true;
	}
	else if (for_sale)
	{
		LLStringUtil::format_map_t args;
		args["[AMOUNT]"] = LLResMgr::getInstance()->getMonetaryString(price);
		line = getString("Price", args);
		show_price_icon = true;
	}
	getChild<LLUICtrl>("price_text")->setValue(line);
	getChild<LLUICtrl>("price_icon")->setVisible(show_price_icon);
}

void LLInspectObject::onClickBuy()
{
	handle_buy();
	closeFloater();
}

void LLInspectObject::onClickPay()
{
	handle_give_money_dialog();
	closeFloater();
}

void LLInspectObject::onClickTakeFreeCopy()
{
	LLObjectSelection* selection = LLSelectMgr::getInstance()->getSelection();
	if (!selection) return;

	LLSelectNode* nodep = selection->getFirstRootNode();
	if (!nodep) return;

	// Figure out if this is a "free buy" or a "take copy"
	bool for_copy = anyone_copy_selection(nodep);
	// Prefer to just take a free copy
	if (for_copy)
	{
		handle_take_copy();
	}
	else
	{
		// Buy for free (confusing, but that's how it is)
		handle_buy();
	}
	closeFloater();
}

void LLInspectObject::onClickTouch()
{
	handle_object_touch();
	closeFloater();
}

void LLInspectObject::onClickSit()
{
	handle_object_sit_or_stand();
	closeFloater();
}

void LLInspectObject::onClickOpen()
{
	LLFloaterReg::showInstance("openobject");
	closeFloater();
}

void LLInspectObject::onClickMoreInfo()
{
	// *TODO: Show object info side panel, once that is implemented.
	LLNotifications::instance().add("ClickUnimplemented");
	closeFloater();
}

//////////////////////////////////////////////////////////////////////////////
// LLInspectObjectUtil
//////////////////////////////////////////////////////////////////////////////
void LLInspectObjectUtil::registerFloater()
{
	LLFloaterReg::add("inspect_object", "inspect_object.xml",
					  &LLFloaterReg::build<LLInspectObject>);
}

