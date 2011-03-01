/** 
 * @file llpanelpicks.cpp
 * @brief LLPanelPicks and related class implementations
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

#include "llpanelpicks.h"

#include "llagent.h"
#include "llagentpicksinfo.h"
#include "llavatarconstants.h"
#include "llcommandhandler.h"
#include "lldispatcher.h"
#include "llflatlistview.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "llnotificationsutil.h"
#include "lltexturectrl.h"
#include "lltoggleablemenu.h"
#include "lltrans.h"
#include "llviewergenericmessage.h"	// send_generic_message
#include "llmenugl.h"
#include "llviewermenu.h"
#include "llregistry.h"

#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llavatarpropertiesprocessor.h"
#include "llpanelavatar.h"
#include "llpanelprofile.h"
#include "llpanelpick.h"
#include "llpanelclassified.h"
#include "llsidetray.h"

static const std::string XML_BTN_NEW = "new_btn";
static const std::string XML_BTN_DELETE = "trash_btn";
static const std::string XML_BTN_INFO = "info_btn";
static const std::string XML_BTN_TELEPORT = "teleport_btn";
static const std::string XML_BTN_SHOW_ON_MAP = "show_on_map_btn";

static const std::string PICK_ID("pick_id");
static const std::string PICK_CREATOR_ID("pick_creator_id");
static const std::string PICK_NAME("pick_name");

static const std::string CLASSIFIED_ID("classified_id");
static const std::string CLASSIFIED_NAME("classified_name");


static LLRegisterPanelClassWrapper<LLPanelPicks> t_panel_picks("panel_picks");

class LLPickHandler : public LLCommandHandler,
					  public LLAvatarPropertiesObserver
{
public:

	std::set<LLUUID> mPickIds;
	
	// requires trusted browser to trigger
	LLPickHandler() : LLCommandHandler("pick", UNTRUSTED_THROTTLE) { }

	bool handle(const LLSD& params, const LLSD& query_map,
		LLMediaCtrl* web)
	{
		if (!LLUI::sSettingGroups["config"]->getBOOL("EnablePicks"))
		{
			LLNotificationsUtil::add("NoPicks");
			return false;
		}

		// handle app/classified/create urls first
		if (params.size() == 1 && params[0].asString() == "create")
		{
			createPick();
			return true;
		}

		// then handle the general app/pick/{UUID}/{CMD} urls
		if (params.size() < 2)
		{
			return false;
		}

		// get the ID for the pick_id
		LLUUID pick_id;
		if (!pick_id.set(params[0], FALSE))
		{
			return false;
		}

		// edit the pick in the side tray.
		// need to ask the server for more info first though...
		const std::string verb = params[1].asString();
		if (verb == "edit")
		{		
			mPickIds.insert(pick_id);
			LLAvatarPropertiesProcessor::getInstance()->addObserver(LLUUID(), this);
			LLAvatarPropertiesProcessor::getInstance()->sendPickInfoRequest(gAgent.getID(),pick_id);
			return true;
		}
		else
		{
			llwarns << "unknown verb " << verb << llendl;
			return false;
		}
	}

	void createPick()
	{
		LLSD params;
		params["id"] = gAgent.getID();
		params["open_tab_name"] = "panel_picks";
		params["show_tab_panel"] = "create_pick";
		LLSideTray::getInstance()->showPanel("panel_me", params);
	}

	void editPick(LLPickData* pick_info)
	{
		LLSD params;
		params["open_tab_name"] = "panel_picks";
		params["show_tab_panel"] = "edit_pick";
		params["pick_id"] = pick_info->pick_id;
		params["avatar_id"] = pick_info->creator_id;
		params["snapshot_id"] = pick_info->snapshot_id;
		params["pick_name"] = pick_info->name;
		params["pick_desc"] = pick_info->desc;
		
		LLSideTray::getInstance()->showPanel("panel_me", params);
	}
	
	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type)
	{
		if (APT_PICK_INFO != type)
		{
			return;
		}

		// is this the pick that we asked for?
		LLPickData* pick_info = static_cast<LLPickData*>(data);
		if (!pick_info || mPickIds.find(pick_info->pick_id) == mPickIds.end())
		{
			return;
		}

		// open the edit side tray for this pick
		if (pick_info->creator_id == gAgent.getID())
		{
			editPick(pick_info);
		}
		else
		{
			llwarns << "Can't edit a pick you did not create" << llendl;
		}

		// remove our observer now that we're done
		mPickIds.erase(pick_info->pick_id);
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(LLUUID(), this);
	}
};

LLPickHandler gPickHandler;

class LLClassifiedHandler :
	public LLCommandHandler,
	public LLAvatarPropertiesObserver
{
public:
	// throttle calls from untrusted browsers
	LLClassifiedHandler() :	LLCommandHandler("classified", UNTRUSTED_THROTTLE) {}

	std::set<LLUUID> mClassifiedIds;

	std::string mRequestVerb;
	
	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (!LLUI::sSettingGroups["config"]->getBOOL("EnableClassifieds"))
		{
			LLNotificationsUtil::add("NoClassifieds");
			return false;
		}

		// handle app/classified/create urls first
		if (params.size() == 1 && params[0].asString() == "create")
		{
			createClassified();
			return true;
		}

		// then handle the general app/classified/{UUID}/{CMD} urls
		if (params.size() < 2)
		{
			return false;
		}

		// get the ID for the classified
		LLUUID classified_id;
		if (!classified_id.set(params[0], FALSE))
		{
			return false;
		}

		// show the classified in the side tray.
		// need to ask the server for more info first though...
		const std::string verb = params[1].asString();
		if (verb == "about")
		{
			mRequestVerb = verb;
			mClassifiedIds.insert(classified_id);
			LLAvatarPropertiesProcessor::getInstance()->addObserver(LLUUID(), this);
			LLAvatarPropertiesProcessor::getInstance()->sendClassifiedInfoRequest(classified_id);
			return true;
		}
		else if (verb == "edit")
		{
			mRequestVerb = verb;
			mClassifiedIds.insert(classified_id);
			LLAvatarPropertiesProcessor::getInstance()->addObserver(LLUUID(), this);
			LLAvatarPropertiesProcessor::getInstance()->sendClassifiedInfoRequest(classified_id);
			return true;
		}

		return false;
	}

	void createClassified()
	{
		// open the new classified panel on the Me > Picks sidetray
		LLSD params;
		params["id"] = gAgent.getID();
		params["open_tab_name"] = "panel_picks";
		params["show_tab_panel"] = "create_classified";
		LLSideTray::getInstance()->showPanel("panel_me", params);
	}

	void openClassified(LLAvatarClassifiedInfo* c_info)
	{
		if (mRequestVerb == "about")
		{
			// open the classified info panel on the Me > Picks sidetray
			LLSD params;
			params["id"] = c_info->creator_id;
			params["open_tab_name"] = "panel_picks";
			params["show_tab_panel"] = "classified_details";
			params["classified_id"] = c_info->classified_id;
			params["classified_creator_id"] = c_info->creator_id;
			params["classified_snapshot_id"] = c_info->snapshot_id;
			params["classified_name"] = c_info->name;
			params["classified_desc"] = c_info->description;
			params["from_search"] = true;
			LLSideTray::getInstance()->showPanel("panel_profile_view", params);
		}
		else if (mRequestVerb == "edit")
		{
			if (c_info->creator_id == gAgent.getID())
			{
				llwarns << "edit in progress" << llendl;
				// open the new classified panel on the Me > Picks sidetray
				LLSD params;
				params["id"] = gAgent.getID();
				params["open_tab_name"] = "panel_picks";
				params["show_tab_panel"] = "edit_classified";
				params["classified_id"] = c_info->classified_id;
				LLSideTray::getInstance()->showPanel("panel_me", params);
			}
			else
			{
				llwarns << "Can't edit a classified you did not create" << llendl;
			}
		}
	}

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type)
	{
		if (APT_CLASSIFIED_INFO != type)
		{
			return;
		}

		// is this the classified that we asked for?
		LLAvatarClassifiedInfo* c_info = static_cast<LLAvatarClassifiedInfo*>(data);
		if (!c_info || mClassifiedIds.find(c_info->classified_id) == mClassifiedIds.end())
		{
			return;
		}

		// open the detail side tray for this classified
		openClassified(c_info);

		// remove our observer now that we're done
		mClassifiedIds.erase(c_info->classified_id);
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(LLUUID(), this);
	}

};
LLClassifiedHandler gClassifiedHandler;

//////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
// LLPanelPicks
//-----------------------------------------------------------------------------
LLPanelPicks::LLPanelPicks()
:	LLPanelProfileTab(),
	mPopupMenu(NULL),
	mProfilePanel(NULL),
	mPickPanel(NULL),
	mPicksList(NULL),
	mClassifiedsList(NULL),
	mPanelPickInfo(NULL),
	mPanelPickEdit(NULL),
	mPlusMenu(NULL),
	mPicksAccTab(NULL),
	mClassifiedsAccTab(NULL),
	mPanelClassifiedInfo(NULL),
	mNoClassifieds(false),
	mNoPicks(false)
{
}

LLPanelPicks::~LLPanelPicks()
{
	if(getAvatarId().notNull())
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
	}
}

void* LLPanelPicks::create(void* data /* = NULL */)
{
	return new LLPanelPicks();
}

void LLPanelPicks::updateData()
{
	// Send Picks request only when we need to, not on every onOpen(during tab switch).
	if(isDirty())
	{
		mNoPicks = false;
		mNoClassifieds = false;

		mNoItemsLabel->setValue(LLTrans::getString("PicksClassifiedsLoadingText"));
		mNoItemsLabel->setVisible(TRUE);

		mPicksList->clear();
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarPicksRequest(getAvatarId());

		mClassifiedsList->clear();
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarClassifiedsRequest(getAvatarId());
	}
}

void LLPanelPicks::processProperties(void* data, EAvatarProcessorType type)
{
	if(APT_PICKS == type)
	{
		LLAvatarPicks* avatar_picks = static_cast<LLAvatarPicks*>(data);
		if(avatar_picks && getAvatarId() == avatar_picks->target_id)
		{
			std::string full_name;
			gCacheName->getFullName(getAvatarId(), full_name);
			getChild<LLUICtrl>("pick_title")->setTextArg("[NAME]", full_name);
			
			// Save selection, to be able to edit same item after saving changes. See EXT-3023.
			LLUUID selected_id = mPicksList->getSelectedValue()[PICK_ID];

			mPicksList->clear();

			LLAvatarPicks::picks_list_t::const_iterator it = avatar_picks->picks_list.begin();
			for(; avatar_picks->picks_list.end() != it; ++it)
			{
				LLUUID pick_id = it->first;
				std::string pick_name = it->second;

				LLPickItem* picture = LLPickItem::create();
				picture->childSetAction("info_chevron", boost::bind(&LLPanelPicks::onClickInfo, this));
				picture->setPickName(pick_name);
				picture->setPickId(pick_id);
				picture->setCreatorId(getAvatarId());

				LLAvatarPropertiesProcessor::instance().addObserver(getAvatarId(), picture);
				picture->update();

				LLSD pick_value = LLSD();
				pick_value.insert(PICK_ID, pick_id);
				pick_value.insert(PICK_NAME, pick_name);
				pick_value.insert(PICK_CREATOR_ID, getAvatarId());

				mPicksList->addItem(picture, pick_value);

				// Restore selection by item id. 
				if ( pick_id == selected_id )
					mPicksList->selectItemByValue(pick_value);

				picture->setDoubleClickCallback(boost::bind(&LLPanelPicks::onDoubleClickPickItem, this, _1));
				picture->setRightMouseUpCallback(boost::bind(&LLPanelPicks::onRightMouseUpItem, this, _1, _2, _3, _4));
				picture->setMouseUpCallback(boost::bind(&LLPanelPicks::updateButtons, this));
			}

			showAccordion("tab_picks", mPicksList->size());

			resetDirty();
			updateButtons();
		}
		
		mNoPicks = !mPicksList->size();
	}
	else if(APT_CLASSIFIEDS == type)
	{
		LLAvatarClassifieds* c_info = static_cast<LLAvatarClassifieds*>(data);
		if(c_info && getAvatarId() == c_info->target_id)
		{
			// do not clear classified list in case we will receive two or more data packets.
			// list has been cleared in updateData(). (fix for EXT-6436)

			LLAvatarClassifieds::classifieds_list_t::const_iterator it = c_info->classifieds_list.begin();
			for(; c_info->classifieds_list.end() != it; ++it)
			{
				LLAvatarClassifieds::classified_data c_data = *it;

				LLClassifiedItem* c_item = new LLClassifiedItem(getAvatarId(), c_data.classified_id);
				c_item->childSetAction("info_chevron", boost::bind(&LLPanelPicks::onClickInfo, this));
				c_item->setClassifiedName(c_data.name);

				LLSD pick_value = LLSD();
				pick_value.insert(CLASSIFIED_ID, c_data.classified_id);
				pick_value.insert(CLASSIFIED_NAME, c_data.name);

				if (!findClassifiedById(c_data.classified_id))
				{
					mClassifiedsList->addItem(c_item, pick_value);
				}

				c_item->setDoubleClickCallback(boost::bind(&LLPanelPicks::onDoubleClickClassifiedItem, this, _1));
				c_item->setRightMouseUpCallback(boost::bind(&LLPanelPicks::onRightMouseUpItem, this, _1, _2, _3, _4));
				c_item->setMouseUpCallback(boost::bind(&LLPanelPicks::updateButtons, this));
			}

			showAccordion("tab_classifieds", mClassifiedsList->size());

			resetDirty();
			updateButtons();
		}
		
		mNoClassifieds = !mClassifiedsList->size();
	}

	bool no_data = mNoPicks && mNoClassifieds;
	mNoItemsLabel->setVisible(no_data);
	if (no_data)
	{
		if(getAvatarId() == gAgentID)
		{
			mNoItemsLabel->setValue(LLTrans::getString("NoPicksClassifiedsText"));
		}
		else
		{
			mNoItemsLabel->setValue(LLTrans::getString("NoAvatarPicksClassifiedsText"));
		}
	}
}

LLPickItem* LLPanelPicks::getSelectedPickItem()
{
	LLPanel* selected_item = mPicksList->getSelectedItem();
	if (!selected_item) return NULL;

	return dynamic_cast<LLPickItem*>(selected_item);
}

LLClassifiedItem* LLPanelPicks::getSelectedClassifiedItem()
{
	LLPanel* selected_item = mClassifiedsList->getSelectedItem();
	if (!selected_item) 
	{
		return NULL;
	}
	return dynamic_cast<LLClassifiedItem*>(selected_item);
}

BOOL LLPanelPicks::postBuild()
{
	mPicksList = getChild<LLFlatListView>("picks_list");
	mClassifiedsList = getChild<LLFlatListView>("classifieds_list");

	mPicksList->setCommitOnSelectionChange(true);
	mClassifiedsList->setCommitOnSelectionChange(true);

	mPicksList->setCommitCallback(boost::bind(&LLPanelPicks::onListCommit, this, mPicksList));
	mClassifiedsList->setCommitCallback(boost::bind(&LLPanelPicks::onListCommit, this, mClassifiedsList));

	mPicksList->setNoItemsCommentText(getString("no_picks"));
	mClassifiedsList->setNoItemsCommentText(getString("no_classifieds"));

	mNoItemsLabel = getChild<LLUICtrl>("picks_panel_text");

	childSetAction(XML_BTN_NEW, boost::bind(&LLPanelPicks::onClickPlusBtn, this));
	childSetAction(XML_BTN_DELETE, boost::bind(&LLPanelPicks::onClickDelete, this));
	childSetAction(XML_BTN_TELEPORT, boost::bind(&LLPanelPicks::onClickTeleport, this));
	childSetAction(XML_BTN_SHOW_ON_MAP, boost::bind(&LLPanelPicks::onClickMap, this));
	childSetAction(XML_BTN_INFO, boost::bind(&LLPanelPicks::onClickInfo, this));

	mPicksAccTab = getChild<LLAccordionCtrlTab>("tab_picks");
	mPicksAccTab->setDropDownStateChangedCallback(boost::bind(&LLPanelPicks::onAccordionStateChanged, this, mPicksAccTab));
	mPicksAccTab->setDisplayChildren(true);

	mClassifiedsAccTab = getChild<LLAccordionCtrlTab>("tab_classifieds");
	mClassifiedsAccTab->setDropDownStateChangedCallback(boost::bind(&LLPanelPicks::onAccordionStateChanged, this, mClassifiedsAccTab));
	mClassifiedsAccTab->setDisplayChildren(false);
	
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registar;
	registar.add("Pick.Info", boost::bind(&LLPanelPicks::onClickInfo, this));
	registar.add("Pick.Edit", boost::bind(&LLPanelPicks::onClickMenuEdit, this)); 
	registar.add("Pick.Teleport", boost::bind(&LLPanelPicks::onClickTeleport, this));
	registar.add("Pick.Map", boost::bind(&LLPanelPicks::onClickMap, this));
	registar.add("Pick.Delete", boost::bind(&LLPanelPicks::onClickDelete, this));
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registar;
	enable_registar.add("Pick.Enable", boost::bind(&LLPanelPicks::onEnableMenuItem, this, _2));

	mPopupMenu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>("menu_picks.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar plus_registar;
	plus_registar.add("Picks.Plus.Action", boost::bind(&LLPanelPicks::onPlusMenuItemClicked, this, _2));
	mEnableCallbackRegistrar.add("Picks.Plus.Enable", boost::bind(&LLPanelPicks::isActionEnabled, this, _2));
	mPlusMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_picks_plus.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	
	return TRUE;
}

void LLPanelPicks::onPlusMenuItemClicked(const LLSD& param)
{
	std::string value = param.asString();

	if("new_pick" == value)
	{
		createNewPick();
	}
	else if("new_classified" == value)
	{
		createNewClassified();
	}
}

bool LLPanelPicks::isActionEnabled(const LLSD& userdata) const
{
	std::string command_name = userdata.asString();

	if (command_name == "new_pick" && LLAgentPicksInfo::getInstance()->isPickLimitReached())
	{
		return false;
	}

	return true;
}

bool LLPanelPicks::isClassifiedPublished(LLClassifiedItem* c_item)
{
	if(c_item)
	{
		LLPanelClassifiedEdit* panel = mEditClassifiedPanels[c_item->getClassifiedId()];
		if(panel)
		{
			 return !panel->isNewWithErrors();
		}

		// we've got this classified from server - it's published
		return true;
	}
	return false;
}

void LLPanelPicks::onAccordionStateChanged(const LLAccordionCtrlTab* acc_tab)
{
	if(!mPicksAccTab->getDisplayChildren())
	{
		mPicksList->resetSelection(true);
	}
	if(!mClassifiedsAccTab->getDisplayChildren())
	{
		mClassifiedsList->resetSelection(true);
	}

	updateButtons();
}

void LLPanelPicks::onOpen(const LLSD& key)
{
	const LLUUID id(key.asUUID());
	BOOL self = (gAgent.getID() == id);

	// only agent can edit her picks 
	getChildView("edit_panel")->setEnabled(self);
	getChildView("edit_panel")->setVisible( self);

	// Disable buttons when viewing profile for first time
	if(getAvatarId() != id)
	{
		getChildView(XML_BTN_INFO)->setEnabled(FALSE);
		getChildView(XML_BTN_TELEPORT)->setEnabled(FALSE);
		getChildView(XML_BTN_SHOW_ON_MAP)->setEnabled(FALSE);
	}

	// and see a special title - set as invisible by default in xml file
	if (self)
	{
		getChildView("pick_title")->setVisible( !self);
		getChildView("pick_title_agent")->setVisible( self);

		mPopupMenu->setItemVisible("pick_delete", TRUE);
		mPopupMenu->setItemVisible("pick_edit", TRUE);
		mPopupMenu->setItemVisible("pick_separator", TRUE);
	}

	if(getAvatarId() != id)
	{
		showAccordion("tab_picks", false);
		showAccordion("tab_classifieds", false);

		mPicksList->goToTop();
		// Set dummy value to make panel dirty and make it reload picks
		setValue(LLSD());
	}

	LLPanelProfileTab::onOpen(key);
}

void LLPanelPicks::onClosePanel()
{
	if (mPanelClassifiedInfo)
	{
		onPanelClassifiedClose(mPanelClassifiedInfo);
	}
	if (mPanelPickInfo)
	{
		onPanelPickClose(mPanelPickInfo);
	}
}

void LLPanelPicks::onListCommit(const LLFlatListView* f_list)
{
	// Make sure only one of the lists has selection.
	if(f_list == mPicksList)
	{
		mClassifiedsList->resetSelection(true);
	}
	else if(f_list == mClassifiedsList)
	{
		mPicksList->resetSelection(true);
	}
	else
	{
		llwarns << "Unknown list" << llendl;
	}

	updateButtons();
}

//static
void LLPanelPicks::onClickDelete()
{
	LLSD value = mPicksList->getSelectedValue();
	if (value.isDefined())
	{
		LLSD args; 
		args["PICK"] = value[PICK_NAME]; 
		LLNotificationsUtil::add("DeleteAvatarPick", args, LLSD(), boost::bind(&LLPanelPicks::callbackDeletePick, this, _1, _2)); 
		return;
	}

	value = mClassifiedsList->getSelectedValue();
	if(value.isDefined())
	{
		LLSD args; 
		args["NAME"] = value[CLASSIFIED_NAME]; 
		LLNotificationsUtil::add("DeleteClassified", args, LLSD(), boost::bind(&LLPanelPicks::callbackDeleteClassified, this, _1, _2)); 
		return;
	}
}

bool LLPanelPicks::callbackDeletePick(const LLSD& notification, const LLSD& response) 
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLSD pick_value = mPicksList->getSelectedValue();

	if (0 == option)
	{
		LLAvatarPropertiesProcessor::instance().sendPickDelete(pick_value[PICK_ID]);
		mPicksList->removeItemByValue(pick_value);
	}
	updateButtons();
	return false;
}

bool LLPanelPicks::callbackDeleteClassified(const LLSD& notification, const LLSD& response) 
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLSD value = mClassifiedsList->getSelectedValue();

	if (0 == option)
	{
		LLAvatarPropertiesProcessor::instance().sendClassifiedDelete(value[CLASSIFIED_ID]);
		mClassifiedsList->removeItemByValue(value);
	}
	updateButtons();
	return false;
}

bool LLPanelPicks::callbackTeleport( const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if (0 == option)
	{
		onClickTeleport();
	}
	return false;
}

//static
void LLPanelPicks::onClickTeleport()
{
	LLPickItem* pick_item = getSelectedPickItem();
	LLClassifiedItem* c_item = getSelectedClassifiedItem();

	LLVector3d pos;
	if(pick_item)
		pos = pick_item->getPosGlobal();
	else if(c_item)
	{
		pos = c_item->getPosGlobal();
		LLPanelClassifiedInfo::sendClickMessage("teleport", false,
			c_item->getClassifiedId(), LLUUID::null, pos, LLStringUtil::null);
	}

	if (!pos.isExactlyZero())
	{
		gAgent.teleportViaLocation(pos);
		LLFloaterWorldMap::getInstance()->trackLocation(pos);
	}
}

//static
void LLPanelPicks::onClickMap()
{
	LLPickItem* pick_item = getSelectedPickItem();
	LLClassifiedItem* c_item = getSelectedClassifiedItem();

	LLVector3d pos;
	if (pick_item)
		pos = pick_item->getPosGlobal();
	else if(c_item)
	{
		LLPanelClassifiedInfo::sendClickMessage("map", false,
			c_item->getClassifiedId(), LLUUID::null, pos, LLStringUtil::null);
		pos = c_item->getPosGlobal();
	}

	LLFloaterWorldMap::getInstance()->trackLocation(pos);
	LLFloaterReg::showInstance("world_map", "center");
}


void LLPanelPicks::onRightMouseUpItem(LLUICtrl* item, S32 x, S32 y, MASK mask)
{
	updateButtons();

	if (mPopupMenu)
	{
		mPopupMenu->buildDrawLabels();
		mPopupMenu->updateParent(LLMenuGL::sMenuContainer);
		((LLContextMenu*)mPopupMenu)->show(x, y);
		LLMenuGL::showPopup(item, mPopupMenu, x, y);
	}
}

void LLPanelPicks::onDoubleClickPickItem(LLUICtrl* item)
{
	LLSD pick_value = mPicksList->getSelectedValue();
	if (pick_value.isUndefined()) return;
	
	LLSD args; 
	args["PICK"] = pick_value[PICK_NAME]; 
	LLNotificationsUtil::add("TeleportToPick", args, LLSD(), boost::bind(&LLPanelPicks::callbackTeleport, this, _1, _2)); 
}

void LLPanelPicks::onDoubleClickClassifiedItem(LLUICtrl* item)
{
	LLSD value = mClassifiedsList->getSelectedValue();
	if (value.isUndefined()) return;

	LLSD args; 
	args["CLASSIFIED"] = value[CLASSIFIED_NAME]; 
	LLNotificationsUtil::add("TeleportToClassified", args, LLSD(), boost::bind(&LLPanelPicks::callbackTeleport, this, _1, _2)); 
}

void LLPanelPicks::updateButtons()
{
	bool has_selected = mPicksList->numSelected() > 0 || mClassifiedsList->numSelected() > 0;

	if (getAvatarId() == gAgentID)
	{
		getChildView(XML_BTN_DELETE)->setEnabled(has_selected);
	}

	getChildView(XML_BTN_INFO)->setEnabled(has_selected);
	getChildView(XML_BTN_TELEPORT)->setEnabled(has_selected);
	getChildView(XML_BTN_SHOW_ON_MAP)->setEnabled(has_selected);

	LLClassifiedItem* c_item = dynamic_cast<LLClassifiedItem*>(mClassifiedsList->getSelectedItem());
	if(c_item)
	{
		getChildView(XML_BTN_INFO)->setEnabled(isClassifiedPublished(c_item));
	}
}

void LLPanelPicks::setProfilePanel(LLPanelProfile* profile_panel)
{
	mProfilePanel = profile_panel;
}


void LLPanelPicks::buildPickPanel()
{
// 	if (mPickPanel == NULL)
// 	{
// 		mPickPanel = new LLPanelPick();
// 		mPickPanel->setExitCallback(boost::bind(&LLPanelPicks::onPanelPickClose, this, NULL));
// 	}
}

void LLPanelPicks::onClickPlusBtn()
{
	LLRect rect(getChildView(XML_BTN_NEW)->getRect());

	mPlusMenu->updateParent(LLMenuGL::sMenuContainer);
	mPlusMenu->setButtonRect(rect, this);
	LLMenuGL::showPopup(this, mPlusMenu, rect.mLeft, rect.mTop);
}

void LLPanelPicks::createNewPick()
{
	createPickEditPanel();

	getProfilePanel()->openPanel(mPanelPickEdit, LLSD());
}

void LLPanelPicks::createNewClassified()
{
	LLPanelClassifiedEdit* panel = NULL;
	createClassifiedEditPanel(&panel);

	getProfilePanel()->openPanel(panel, LLSD());
}

void LLPanelPicks::onClickInfo()
{
	if(mPicksList->numSelected() > 0)
	{
		openPickInfo();
	}
	else if(mClassifiedsList->numSelected() > 0)
	{
		openClassifiedInfo();
	}
}

void LLPanelPicks::openPickInfo()
{
	LLSD selected_value = mPicksList->getSelectedValue();
	if (selected_value.isUndefined()) return;

	LLPickItem* pick = (LLPickItem*)mPicksList->getSelectedItem();

	createPickInfoPanel();

	LLSD params;
	params["pick_id"] = pick->getPickId();
	params["avatar_id"] = pick->getCreatorId();
	params["snapshot_id"] = pick->getSnapshotId();
	params["pick_name"] = pick->getPickName();
	params["pick_desc"] = pick->getPickDesc();

	getProfilePanel()->openPanel(mPanelPickInfo, params);
}

void LLPanelPicks::openClassifiedInfo()
{
	LLSD selected_value = mClassifiedsList->getSelectedValue();
	if (selected_value.isUndefined()) return;

	LLClassifiedItem* c_item = getSelectedClassifiedItem();
	LLSD params;
	params["classified_id"] = c_item->getClassifiedId();
	params["classified_creator_id"] = c_item->getAvatarId();
	params["classified_snapshot_id"] = c_item->getSnapshotId();
	params["classified_name"] = c_item->getClassifiedName();
	params["classified_desc"] = c_item->getDescription();
	params["from_search"] = false;

	openClassifiedInfo(params);
}

void LLPanelPicks::openClassifiedInfo(const LLSD &params)
{
	createClassifiedInfoPanel();
	getProfilePanel()->openPanel(mPanelClassifiedInfo, params);
}

void LLPanelPicks::openClassifiedEdit(const LLSD& params)
{
	LLUUID classified_id = params["classified_id"].asUUID();;
	llinfos << "opening classified " << classified_id << " for edit" << llendl;
	editClassified(classified_id);
}

void LLPanelPicks::showAccordion(const std::string& name, bool show)
{
	LLAccordionCtrlTab* tab = getChild<LLAccordionCtrlTab>(name);
	tab->setVisible(show);
	LLAccordionCtrl* acc = getChild<LLAccordionCtrl>("accordion");
	acc->arrange();
}

void LLPanelPicks::onPanelPickClose(LLPanel* panel)
{
	getProfilePanel()->closePanel(panel);
}

void LLPanelPicks::onPanelPickSave(LLPanel* panel)
{
	onPanelPickClose(panel);
	updateButtons();
}

void LLPanelPicks::onPanelClassifiedSave(LLPanelClassifiedEdit* panel)
{
	if(!panel->canClose())
	{
		return;
	}

	if(panel->isNew())
	{
		mEditClassifiedPanels[panel->getClassifiedId()] = panel;

		LLClassifiedItem* c_item = new LLClassifiedItem(getAvatarId(), panel->getClassifiedId());
		c_item->fillIn(panel);

		LLSD c_value;
		c_value.insert(CLASSIFIED_ID, c_item->getClassifiedId());
		c_value.insert(CLASSIFIED_NAME, c_item->getClassifiedName());
		mClassifiedsList->addItem(c_item, c_value, ADD_TOP);

		c_item->setDoubleClickCallback(boost::bind(&LLPanelPicks::onDoubleClickClassifiedItem, this, _1));
		c_item->setRightMouseUpCallback(boost::bind(&LLPanelPicks::onRightMouseUpItem, this, _1, _2, _3, _4));
		c_item->setMouseUpCallback(boost::bind(&LLPanelPicks::updateButtons, this));
		c_item->childSetAction("info_chevron", boost::bind(&LLPanelPicks::onClickInfo, this));

		// order does matter, showAccordion will invoke arrange for accordions.
		mClassifiedsAccTab->changeOpenClose(false);
		showAccordion("tab_classifieds", true);
	}
	else if(panel->isNewWithErrors())
	{
		LLClassifiedItem* c_item = dynamic_cast<LLClassifiedItem*>(mClassifiedsList->getSelectedItem());
		llassert(c_item);
		if (c_item)
		{
			c_item->fillIn(panel);
		}
	}
	else 
	{
		onPanelClassifiedClose(panel);
		return;
	}

	onPanelPickClose(panel);
	updateButtons();
}

void LLPanelPicks::onPanelClassifiedClose(LLPanelClassifiedInfo* panel)
{
	if(panel->getInfoLoaded() && !panel->isDirty())
	{
		std::vector<LLSD> values;
		mClassifiedsList->getValues(values);
		for(size_t n = 0; n < values.size(); ++n)
		{
			LLUUID c_id = values[n][CLASSIFIED_ID].asUUID();
			if(panel->getClassifiedId() == c_id)
			{
				LLClassifiedItem* c_item = dynamic_cast<LLClassifiedItem*>(
					mClassifiedsList->getItemByValue(values[n]));
				llassert(c_item);
				if (c_item)
				{
					c_item->setClassifiedName(panel->getClassifiedName());
					c_item->setDescription(panel->getDescription());
					c_item->setSnapshotId(panel->getSnapshotId());
				}
			}
		}
	}

	onPanelPickClose(panel);
	updateButtons();
}

void LLPanelPicks::createPickInfoPanel()
{
	if(!mPanelPickInfo)
	{
		mPanelPickInfo = LLPanelPickInfo::create();
		mPanelPickInfo->setExitCallback(boost::bind(&LLPanelPicks::onPanelPickClose, this, mPanelPickInfo));
		mPanelPickInfo->setEditPickCallback(boost::bind(&LLPanelPicks::onPanelPickEdit, this));
		mPanelPickInfo->setVisible(FALSE);
	}
}

void LLPanelPicks::createClassifiedInfoPanel()
{
	if(!mPanelClassifiedInfo)
	{
		mPanelClassifiedInfo = LLPanelClassifiedInfo::create();
		mPanelClassifiedInfo->setExitCallback(boost::bind(&LLPanelPicks::onPanelClassifiedClose, this, mPanelClassifiedInfo));
		mPanelClassifiedInfo->setEditClassifiedCallback(boost::bind(&LLPanelPicks::onPanelClassifiedEdit, this));
		mPanelClassifiedInfo->setVisible(FALSE);
	}
}

void LLPanelPicks::createClassifiedEditPanel(LLPanelClassifiedEdit** panel)
{
	if(panel)
	{
		LLPanelClassifiedEdit* new_panel = LLPanelClassifiedEdit::create();
		new_panel->setExitCallback(boost::bind(&LLPanelPicks::onPanelClassifiedClose, this, new_panel));
		new_panel->setSaveCallback(boost::bind(&LLPanelPicks::onPanelClassifiedSave, this, new_panel));
		new_panel->setCancelCallback(boost::bind(&LLPanelPicks::onPanelClassifiedClose, this, new_panel));
		new_panel->setVisible(FALSE);
		*panel = new_panel;
	}
}

void LLPanelPicks::createPickEditPanel()
{
	if(!mPanelPickEdit)
	{
		mPanelPickEdit = LLPanelPickEdit::create();
		mPanelPickEdit->setExitCallback(boost::bind(&LLPanelPicks::onPanelPickClose, this, mPanelPickEdit));
		mPanelPickEdit->setSaveCallback(boost::bind(&LLPanelPicks::onPanelPickSave, this, mPanelPickEdit));
		mPanelPickEdit->setCancelCallback(boost::bind(&LLPanelPicks::onPanelPickClose, this, mPanelPickEdit));
		mPanelPickEdit->setVisible(FALSE);
	}
}

// void LLPanelPicks::openPickEditPanel(LLPickItem* pick)
// {
// 	if(!pick)
// 	{
// 		return;
// 	}
// }

// void LLPanelPicks::openPickInfoPanel(LLPickItem* pick)
// {
// 	if(!mPanelPickInfo)
// 	{
// 		mPanelPickInfo = LLPanelPickInfo::create();
// 		mPanelPickInfo->setExitCallback(boost::bind(&LLPanelPicks::onPanelPickClose, this, mPanelPickInfo));
// 		mPanelPickInfo->setEditPickCallback(boost::bind(&LLPanelPicks::onPanelPickEdit, this));
// 		mPanelPickInfo->setVisible(FALSE);
// 	}
// 
// 	LLSD params;
// 	params["pick_id"] = pick->getPickId();
// 	params["avatar_id"] = pick->getCreatorId();
// 	params["snapshot_id"] = pick->getSnapshotId();
// 	params["pick_name"] = pick->getPickName();
// 	params["pick_desc"] = pick->getPickDesc();
// 
// 	getProfilePanel()->openPanel(mPanelPickInfo, params);
// }

void LLPanelPicks::openPickEdit(const LLSD& params)
{
	createPickEditPanel();
	getProfilePanel()->openPanel(mPanelPickEdit, params);
}

void LLPanelPicks::onPanelPickEdit()
{
	LLSD selected_value = mPicksList->getSelectedValue();
	if (selected_value.isUndefined()) return;

	LLPickItem* pick = dynamic_cast<LLPickItem*>(mPicksList->getSelectedItem());
	
	createPickEditPanel();

	LLSD params;
	params["pick_id"] = pick->getPickId();
	params["avatar_id"] = pick->getCreatorId();
	params["snapshot_id"] = pick->getSnapshotId();
	params["pick_name"] = pick->getPickName();
	params["pick_desc"] = pick->getPickDesc();

	getProfilePanel()->openPanel(mPanelPickEdit, params);
}

void LLPanelPicks::onPanelClassifiedEdit()
{
	LLSD selected_value = mClassifiedsList->getSelectedValue();
	if (selected_value.isUndefined()) 
	{
		return;
	}

	LLClassifiedItem* c_item = dynamic_cast<LLClassifiedItem*>(mClassifiedsList->getSelectedItem());
	llassert(c_item);
	if (!c_item)
	{
		return;
	}
	editClassified(c_item->getClassifiedId());
}

LLClassifiedItem *LLPanelPicks::findClassifiedById(const LLUUID& classified_id)
{
	// HACK - find item by classified id.  Should be a better way.
	std::vector<LLPanel*> items;
	mClassifiedsList->getItems(items);
	LLClassifiedItem* c_item = NULL;
	for(std::vector<LLPanel*>::iterator it = items.begin(); it != items.end(); ++it)
	{
		LLClassifiedItem *test_item = dynamic_cast<LLClassifiedItem*>(*it);
		if (test_item && test_item->getClassifiedId() == classified_id)
		{
			c_item = test_item;
			break;
		}
	}
	return c_item;
}

void LLPanelPicks::editClassified(const LLUUID&  classified_id)
{
	LLClassifiedItem* c_item = findClassifiedById(classified_id);
	if (!c_item)
	{
		llwarns << "item not found for classified_id " << classified_id << llendl;
		return;
	}

	LLSD params;
	params["classified_id"] = c_item->getClassifiedId();
	params["classified_creator_id"] = c_item->getAvatarId();
	params["snapshot_id"] = c_item->getSnapshotId();
	params["name"] = c_item->getClassifiedName();
	params["desc"] = c_item->getDescription();
	params["category"] = (S32)c_item->getCategory();
	params["content_type"] = (S32)c_item->getContentType();
	params["auto_renew"] = c_item->getAutoRenew();
	params["price_for_listing"] = c_item->getPriceForListing();
	params["location_text"] = c_item->getLocationText();

	LLPanelClassifiedEdit* panel = mEditClassifiedPanels[c_item->getClassifiedId()];
	if(!panel)
	{
		createClassifiedEditPanel(&panel);
		mEditClassifiedPanels[c_item->getClassifiedId()] = panel;
	}
	getProfilePanel()->openPanel(panel, params);
	panel->setPosGlobal(c_item->getPosGlobal());
}

void LLPanelPicks::onClickMenuEdit()
{
	if(getSelectedPickItem())
	{
		onPanelPickEdit();
	}
	else if(getSelectedClassifiedItem())
	{
		onPanelClassifiedEdit();
	}
}

bool LLPanelPicks::onEnableMenuItem(const LLSD& user_data)
{
	std::string param = user_data.asString();

	LLClassifiedItem* c_item = dynamic_cast<LLClassifiedItem*>(mClassifiedsList->getSelectedItem());
	if(c_item && "info" == param)
	{
		// dont show Info panel if classified was not created
		return isClassifiedPublished(c_item);
	}

	return true;
}

inline LLPanelProfile* LLPanelPicks::getProfilePanel()
{
	llassert_always(NULL != mProfilePanel);
	return mProfilePanel;
}

//-----------------------------------------------------------------------------
// LLPanelPicks
//-----------------------------------------------------------------------------
LLPickItem::LLPickItem()
: LLPanel()
, mPickID(LLUUID::null)
, mCreatorID(LLUUID::null)
, mParcelID(LLUUID::null)
, mSnapshotID(LLUUID::null)
, mNeedData(true)
{
	buildFromFile("panel_pick_list_item.xml");
}

LLPickItem::~LLPickItem()
{
	if (mCreatorID.notNull())
	{
		LLAvatarPropertiesProcessor::instance().removeObserver(mCreatorID, this);
	}

}

LLPickItem* LLPickItem::create()
{
	return new LLPickItem();
}

void LLPickItem::init(LLPickData* pick_data)
{
	setPickDesc(pick_data->desc);
	setSnapshotId(pick_data->snapshot_id);
	mPosGlobal = pick_data->pos_global;
	mSimName = pick_data->sim_name;
	mPickDescription = pick_data->desc;
	mUserName = pick_data->user_name;
	mOriginalName = pick_data->original_name;

	LLTextureCtrl* picture = getChild<LLTextureCtrl>("picture");
	picture->setImageAssetID(pick_data->snapshot_id);
}

void LLPickItem::setPickName(const std::string& name)
{
	mPickName = name;
	getChild<LLUICtrl>("picture_name")->setValue(name);

}

const std::string& LLPickItem::getPickName()
{
	return mPickName;
}

const LLUUID& LLPickItem::getCreatorId()
{
	return mCreatorID;
}

const LLUUID& LLPickItem::getSnapshotId()
{
	return mSnapshotID;
}

void LLPickItem::setPickDesc(const std::string& descr)
{
	getChild<LLUICtrl>("picture_descr")->setValue(descr);
}

void LLPickItem::setPickId(const LLUUID& id)
{
	mPickID = id;
}

const LLUUID& LLPickItem::getPickId()
{
	return mPickID;
}

const LLVector3d& LLPickItem::getPosGlobal()
{
	return mPosGlobal;
}

const std::string LLPickItem::getDescription()
{
	return getChild<LLUICtrl>("picture_descr")->getValue().asString();
}

void LLPickItem::update()
{
	setNeedData(true);
	LLAvatarPropertiesProcessor::instance().sendPickInfoRequest(mCreatorID, mPickID);
}

void LLPickItem::processProperties(void *data, EAvatarProcessorType type)
{
	if (APT_PICK_INFO != type) 
	{
		return;
	}

	LLPickData* pick_data = static_cast<LLPickData *>(data);
	if (!pick_data || mPickID != pick_data->pick_id) 
	{
		return;
	}

	init(pick_data);
	setNeedData(false);
	LLAvatarPropertiesProcessor::instance().removeObserver(mCreatorID, this);
}

void set_child_visible(LLView* parent, const std::string& child_name, bool visible)
{
	parent->getChildView(child_name)->setVisible(visible);
}

BOOL LLPickItem::postBuild()
{
	setMouseEnterCallback(boost::bind(&set_child_visible, this, "hovered_icon", true));
	setMouseLeaveCallback(boost::bind(&set_child_visible, this, "hovered_icon", false));
	return TRUE;
}

void LLPickItem::setValue(const LLSD& value)
{
	if (!value.isMap()) return;;
	if (!value.has("selected")) return;
	getChildView("selected_icon")->setVisible( value["selected"]);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLClassifiedItem::LLClassifiedItem(const LLUUID& avatar_id, const LLUUID& classified_id)
 : LLPanel()
 , mAvatarId(avatar_id)
 , mClassifiedId(classified_id)
{
	buildFromFile("panel_classifieds_list_item.xml");

	LLAvatarPropertiesProcessor::getInstance()->addObserver(getAvatarId(), this);
	LLAvatarPropertiesProcessor::getInstance()->sendClassifiedInfoRequest(getClassifiedId());
}

LLClassifiedItem::~LLClassifiedItem()
{
	LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(), this);
}

void LLClassifiedItem::processProperties(void* data, EAvatarProcessorType type)
{
	if(APT_CLASSIFIED_INFO != type)
	{
		return;
	}

	LLAvatarClassifiedInfo* c_info = static_cast<LLAvatarClassifiedInfo*>(data);
	if( !c_info || c_info->classified_id != getClassifiedId() )
	{
		return;
	}

	setClassifiedName(c_info->name);
	setDescription(c_info->description);
	setSnapshotId(c_info->snapshot_id);
	setPosGlobal(c_info->pos_global);

	LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(), this);
}

BOOL LLClassifiedItem::postBuild()
{
	setMouseEnterCallback(boost::bind(&set_child_visible, this, "hovered_icon", true));
	setMouseLeaveCallback(boost::bind(&set_child_visible, this, "hovered_icon", false));
	return TRUE;
}

void LLClassifiedItem::setValue(const LLSD& value)
{
	if (!value.isMap()) return;;
	if (!value.has("selected")) return;
	getChildView("selected_icon")->setVisible( value["selected"]);
}

void LLClassifiedItem::fillIn(LLPanelClassifiedEdit* panel)
{
	if(!panel)
	{
		return;
	}

	setClassifiedName(panel->getClassifiedName());
	setDescription(panel->getDescription());
	setSnapshotId(panel->getSnapshotId());
	setCategory(panel->getCategory());
	setContentType(panel->getContentType());
	setAutoRenew(panel->getAutoRenew());
	setPriceForListing(panel->getPriceForListing());
	setPosGlobal(panel->getPosGlobal());
	setLocationText(panel->getClassifiedLocation());
}

void LLClassifiedItem::setClassifiedName(const std::string& name)
{
	getChild<LLUICtrl>("name")->setValue(name);
}

void LLClassifiedItem::setDescription(const std::string& desc)
{
	getChild<LLUICtrl>("description")->setValue(desc);
}

void LLClassifiedItem::setSnapshotId(const LLUUID& snapshot_id)
{
	getChild<LLUICtrl>("picture")->setValue(snapshot_id);
}

LLUUID LLClassifiedItem::getSnapshotId()
{
	return getChild<LLUICtrl>("picture")->getValue();
}

//EOF
