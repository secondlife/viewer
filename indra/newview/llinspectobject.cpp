/** 
 * @file llinspectobject.cpp
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llinspectobject.h"

// Viewer
#include "llfloatersidepanelcontainer.h"
#include "llinspect.h"
#include "llmediaentry.h"
#include "llnotificationsutil.h"	// *TODO: Eliminate, add LLNotificationsUtil wrapper
#include "llselectmgr.h"
#include "llslurl.h"
#include "llviewermenu.h"		// handle_object_touch(), handle_buy()
#include "llviewermedia.h"
#include "llviewermediafocus.h"
#include "llviewerobjectlist.h"	// to select the requested object

// Linden libraries
#include "llbutton.h"			// setLabel(), not virtual!
#include "llclickaction.h"
#include "llfloaterreg.h"
#include "llmenubutton.h"
#include "llresmgr.h"			// getMonetaryString
#include "llsafehandle.h"
#include "lltextbox.h"			// for description truncation
#include "lltoggleablemenu.h"
#include "lltrans.h"
#include "llui.h"				// positionViewNearMouse()
#include "lluictrl.h"

class LLViewerObject;

//////////////////////////////////////////////////////////////////////////////
// LLInspectObject
//////////////////////////////////////////////////////////////////////////////

// Object Inspector, a small information window used when clicking
// in the ambient inspector widget for objects in the 3D world.
class LLInspectObject : public LLInspect
{
	friend class LLFloaterReg;
	
public:
	// object_id - Root object ID for which to show information
	// Inspector will be positioned relative to current mouse position
	LLInspectObject(const LLSD& object_id);
	virtual ~LLInspectObject();
	
	/*virtual*/ BOOL postBuild(void);
	
	// Because floater is single instance, need to re-parse data on each spawn
	// (for example, inspector about same avatar but in different position)
	/*virtual*/ void onOpen(const LLSD& avatar_id);
	
	// Release the selection and do other cleanup
	/*virtual*/ void onClose(bool app_quitting);
	
	// override the inspector mouse leave so timer is only paused if 
	// gear menu is not open
	/* virtual */ void onMouseLeave(S32 x, S32 y, MASK mask);
	
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
	
	void updateMediaCurrentURL();	
	void updateSecureBrowsing();
		
	void onClickBuy();
	void onClickPay();
	void onClickTakeFreeCopy();
	void onClickTouch();
	void onClickSit();
	void onClickOpen();
	void onClickMoreInfo();
	void onClickZoomIn();  
	
private:
	LLUUID				mObjectID;
	LLUUID				mPreviousObjectID;
	S32					mObjectFace;
	viewer_media_t		mMediaImpl;
	LLMediaEntry*       mMediaEntry;
	LLSafeHandle<LLObjectSelection> mObjectSelection;
};

LLInspectObject::LLInspectObject(const LLSD& sd)
:	LLInspect( LLSD() ),	// single_instance, doesn't really need key
	mObjectID(NULL),			// set in onOpen()
	mObjectFace(0),
	mObjectSelection(NULL),
	mMediaImpl(NULL),
	mMediaEntry(NULL)
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
	mCommitCallbackRegistrar.add("InspectObject.ZoomIn", boost::bind(&LLInspectObject::onClickZoomIn, this));
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
	getChild<LLUICtrl>("object_media_url")->setValue("");
	// Set buttons invisible until we know what this object can do
	hideButtons();

	// Hide floater when name links clicked
	LLTextBox* textbox = getChild<LLTextBox>("object_creator");
	textbox->setURLClickedCallback(boost::bind(&LLInspectObject::closeFloater, this, false) );

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

	return TRUE;
}

// Multiple calls to showInstance("inspect_avatar", foo) will provide different
// LLSD for foo, which we will catch here.
//virtual
void LLInspectObject::onOpen(const LLSD& data)
{
	// Start animation
	LLInspect::onOpen(data);

	// Extract appropriate avatar id
	mObjectID = data["object_id"];
	
	if(data.has("object_face"))
	{
		mObjectFace = data["object_face"];
	}
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
		// Media focus and this code fight over the select manager.  
		// Make sure any media is unfocused before changing the selection here.
		LLViewerMediaFocus::getInstance()->clearFocus();
		
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
		
		// Does this face have media?
		const LLTextureEntry* tep = obj->getTE(mObjectFace);
		if (!tep)
			return;
		
		mMediaEntry = tep->hasMedia() ? tep->getMediaData() : NULL;
		if(!mMediaEntry)
			return;
		
		mMediaImpl = LLViewerMedia::getMediaImplFromTextureID(mMediaEntry->getMediaID());
	}
}

// virtual
void LLInspectObject::onClose(bool app_quitting)
{
	// Release selection to deselect
	mObjectSelection = NULL;
	mPreviousObjectID = mObjectID;

	getChild<LLMenuButton>("gear_btn")->hideMenu();
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

	// If we don't have fresh object info yet and it's the object we inspected last time,
	// keep showing the previously retrieved data until we get the update.
	if (!nodep->mValid && nodep->getObject()->getID() == mPreviousObjectID)
	{
		return;
	}

	updateButtons(nodep);
	updateName(nodep);
	updateDescription(nodep);
	updateCreator(nodep);
	updatePrice(nodep);
	
	LLViewerObject* obj = nodep->getObject();
	if(!obj)
		return;
	
	if ( mObjectFace < 0 
		||  mObjectFace >= obj->getNumTEs() )
	{
		return;
	}
	
	// Does this face have media?
	const LLTextureEntry* tep = obj->getTE(mObjectFace);
	if (!tep)
		return;
	
	mMediaEntry = tep->hasMedia() ? tep->getMediaData() : NULL;
	if(!mMediaEntry)
		return;
	
	mMediaImpl = LLViewerMedia::getMediaImplFromTextureID(mMediaEntry->getMediaID());
	
	updateMediaCurrentURL();
	updateSecureBrowsing();
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
}

void LLInspectObject::updateMediaCurrentURL()
{	
	if(!mMediaEntry)
		return;
	LLTextBox* textbox = getChild<LLTextBox>("object_media_url");
	std::string media_url = "";
	textbox->setValue(media_url);
	textbox->setToolTip(media_url);
	LLStringUtil::format_map_t args;
	
	if(mMediaImpl.notNull() && mMediaImpl->hasMedia())
	{
		
		LLPluginClassMedia* media_plugin = NULL;
		media_plugin = mMediaImpl->getMediaPlugin();
		if(media_plugin)
		{
			if(media_plugin->pluginSupportsMediaTime())
			{
				args["[CurrentURL]"] =  mMediaImpl->getMediaURL();
			}
			else
			{
				args["[CurrentURL]"] =  media_plugin->getLocation();
			}
			media_url = LLTrans::getString("CurrentURL", args);

		}
	}
	else if(mMediaEntry->getCurrentURL() != "")
	{
		args["[CurrentURL]"] = mMediaEntry->getCurrentURL();
		media_url = LLTrans::getString("CurrentURL", args);
	}

	textbox->setText(media_url);
	textbox->setToolTip(media_url);
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
			LLSLURL("agent", creator_id, "about").getSLURLString();
		args["[CREATOR]"] = creator_url;
				
		// created by one user but owned by another
		std::string owner_url;
		LLUUID owner_id;
		bool group_owned = nodep->mPermissions->isGroupOwned();
		if (group_owned)
		{
			owner_id = nodep->mPermissions->getGroup();
			owner_url =	LLSLURL("group", owner_id, "about").getSLURLString();
		}
		else
		{
			owner_id = nodep->mPermissions->getOwner();
			owner_url =	LLSLURL("agent", owner_id, "about").getSLURLString();
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

void LLInspectObject::updateSecureBrowsing()
{
	bool is_secure_browsing = false;
	
	if(mMediaImpl.notNull() 
	   && mMediaImpl->hasMedia())
	{
		LLPluginClassMedia* media_plugin = NULL;
		std::string current_url = "";
		media_plugin = mMediaImpl->getMediaPlugin();
		if(media_plugin)
		{
			if(media_plugin->pluginSupportsMediaTime())
			{
				current_url = mMediaImpl->getMediaURL();
			}
			else
			{
				current_url =  media_plugin->getLocation();
			}
		}
		
		std::string prefix =  std::string("https://");
		std::string test_prefix = current_url.substr(0, prefix.length());
		LLStringUtil::toLower(test_prefix);	
		if(test_prefix == prefix)
		{
			is_secure_browsing = true;
		}
	}
	getChild<LLUICtrl>("secure_browsing")->setVisible(is_secure_browsing);
}

// For the object inspector, only unpause the fade timer 
// if the gear menu is not open
void LLInspectObject::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLToggleableMenu* gear_menu = getChild<LLMenuButton>("gear_btn")->getMenu();
	if ( gear_menu && gear_menu->getVisible() )
	{
		return;
	}

	if(childHasVisiblePopupMenu())
	{
		return;
	}

	mOpenTimer.unpause();
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
	LLSD key;
	key["task"] = "task";
	LLFloaterSidePanelContainer::showPanel("inventory", key);
	closeFloater();
}

void LLInspectObject::onClickZoomIn() 
{
	handle_look_at_selection("zoom");
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

