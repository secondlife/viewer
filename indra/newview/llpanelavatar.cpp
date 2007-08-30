/** 
 * @file llpanelavatar.cpp
 * @brief LLPanelAvatar and related class implementations
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llpanelavatar.h"

#include "llclassifiedflags.h"
#include "llfontgl.h"
#include "llcachename.h"

#include "llavatarconstants.h"
#include "lluiconstants.h"
#include "lltextbox.h"
#include "llviewertexteditor.h"
#include "lltexturectrl.h"
#include "llagent.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llcallingcard.h"
#include "llcheckboxctrl.h"
#include "llfloater.h"

#include "llfloaterfriends.h"
#include "llfloatergroupinfo.h"
#include "llfloaterworldmap.h"
#include "llfloatermute.h"
#include "llfloateravatarinfo.h"
#include "lliconctrl.h"
#include "llinventoryview.h"
#include "lllineeditor.h"
#include "llnameeditor.h"
#include "llmutelist.h"
#include "llpanelclassified.h"
#include "llpanelpick.h"
#include "llscrolllistctrl.h"
#include "llstatusbar.h"
#include "lltabcontainer.h"
#include "lltabcontainervertical.h"
#include "llimview.h"
#include "lltooldraganddrop.h"
#include "lluiconstants.h"
#include "llvoavatar.h"
#include "llviewermenu.h"		// *FIX: for is_agent_friend()
#include "llviewergenericmessage.h"	// send_generic_message
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewborder.h"
#include "llweb.h"
#include "llinventorymodel.h"
#include "viewer.h"				// for gUserServer
#include "roles_constants.h"

#define	kArraySize( _kArray ) ( sizeof( (_kArray) ) / sizeof( _kArray[0] ) )

#include "llvieweruictrlfactory.h"

// Statics
std::list<LLPanelAvatar*> LLPanelAvatar::sAllPanels;
BOOL LLPanelAvatar::sAllowFirstLife = FALSE;

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// RN: move these to lldbstrings.h
static const S32 DB_USER_FAVORITES_STR_LEN = 254;

const char LOADING_MSG[] = "Loading...";
static const char IM_DISABLED_TOOLTIP[] = "Instant Message (IM).\nDisabled because you do not have their card.";
static const char IM_ENABLED_TOOLTIP[] = "Instant Message (IM)";
static const S32 LEFT = HPAD;

static const S32 RULER0 = 65;
static const S32 RULER1 = RULER0 + 5;
static const S32 RULER2 = RULER1 + 90;
static const S32 RULER3 = RULER2 + 90;
static const S32 RULER4 = RULER3 + 10;

static const S32 PICT_WIDTH  = 180;
static const S32 PICT_HEIGHT = 135;

static const S32 RULER5  = RULER4 + 140;
static const S32 WIDTH = RULER5 + 16;

static const S32 MAX_CHARS = 254;

static const LLColor4 WHITE(1,1,1,1);
static const LLColor4 BLACK(0,0,0,1);
static const LLColor4 CLEAR(0,0,0,0);

extern void handle_lure(const LLUUID& invitee);
extern void handle_pay_by_id(const LLUUID& payee);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLDropTarget
//
// This handy class is a simple way to drop something on another
// view. It handles drop events, always setting itself to the size of
// its parent.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLDropTarget : public LLView
{
public:
	LLDropTarget(const std::string& name, const LLRect& rect, const LLUUID& agent_id);
	~LLDropTarget();

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	void doDrop(EDragAndDropType cargo_type, void* cargo_data);

	//
	// LLView functionality
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   LLString& tooltip_msg);
	void setAgentID(const LLUUID &agent_id)		{ mAgentID = agent_id; }
protected:
	LLUUID mAgentID;
};


LLDropTarget::LLDropTarget(const std::string& name, const LLRect& rect,
						   const LLUUID& agent_id) :
	LLView(name, rect, NOT_MOUSE_OPAQUE, FOLLOWS_ALL),
	mAgentID(agent_id)
{
}

LLDropTarget::~LLDropTarget()
{
}

EWidgetType LLDropTarget::getWidgetType() const
{
	return WIDGET_TYPE_DROP_TARGET;
}

LLString LLDropTarget::getWidgetTag() const
{
	return LL_DROP_TARGET_TAG;
}

void LLDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	llinfos << "LLDropTarget::doDrop()" << llendl;
}

BOOL LLDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 LLString& tooltip_msg)
{
	BOOL handled = FALSE;
	if(getParent())
	{
		// check if inside
		//LLRect parent_rect = mParentView->getRect();
		//mRect.set(0, parent_rect.getHeight(), parent_rect.getWidth(), 0);
		handled = TRUE;

		// check the type
		switch(cargo_type)
		{
		case DAD_TEXTURE:
		case DAD_SOUND:
		case DAD_LANDMARK:
		case DAD_SCRIPT:
		case DAD_OBJECT:
		case DAD_NOTECARD:
		case DAD_CLOTHING:
		case DAD_BODYPART:
		case DAD_ANIMATION:
		case DAD_GESTURE:
		{
			LLViewerInventoryItem* inv_item = (LLViewerInventoryItem*)cargo_data;
			if(gInventory.getItem(inv_item->getUUID())
				&& LLToolDragAndDrop::isInventoryGiveAcceptable(inv_item))
			{
				// *TODO: get multiple object transfers working
				*accept = ACCEPT_YES_COPY_SINGLE;
				if(drop)
				{
					LLToolDragAndDrop::giveInventory(mAgentID, inv_item);
				}
			}
			else
			{
				// It's not in the user's inventory (it's probably
				// in an object's contents), so disallow dragging
				// it here.  You can't give something you don't
				// yet have.
				*accept = ACCEPT_NO;
			}
			break;
		}
		case DAD_CATEGORY:
		{
			LLViewerInventoryCategory* inv_cat = (LLViewerInventoryCategory*)cargo_data;
			if( gInventory.getCategory( inv_cat->getUUID() ) )
			{
				// *TODO: get multiple object transfers working
				*accept = ACCEPT_YES_COPY_SINGLE;
				if(drop)
				{
					LLToolDragAndDrop::giveInventoryCategory(mAgentID,
																inv_cat);
				}
			}
			else
			{
				// It's not in the user's inventory (it's probably
				// in an object's contents), so disallow dragging
				// it here.  You can't give something you don't
				// yet have.
				*accept = ACCEPT_NO;
			}
			break;
		}
		case DAD_CALLINGCARD:
		default:
			*accept = ACCEPT_NO;
			break;
		}
	}
	return handled;
}


//-----------------------------------------------------------------------------
// LLPanelAvatarTab()
//-----------------------------------------------------------------------------
LLPanelAvatarTab::LLPanelAvatarTab(const std::string& name, const LLRect &rect, 
								   LLPanelAvatar* panel_avatar)
:	LLPanel(name, rect),
	mPanelAvatar(panel_avatar),
	mDataRequested(false)
{ }

// virtual
void LLPanelAvatarTab::draw()
{
	if (getVisible())
	{
		refresh();

		LLPanel::draw();
	}
}

void LLPanelAvatarTab::sendAvatarProfileRequestIfNeeded(const char* method)
{
	if (!mDataRequested)
	{
		std::vector<std::string> strings;
		strings.push_back( mPanelAvatar->getAvatarID().asString() );
		send_generic_message(method, strings);
		mDataRequested = true;
	}
}

//-----------------------------------------------------------------------------
// LLPanelAvatarSecondLife()
//-----------------------------------------------------------------------------
LLPanelAvatarSecondLife::LLPanelAvatarSecondLife(const std::string& name, 
												 const LLRect &rect, 
												 LLPanelAvatar* panel_avatar ) 
:	LLPanelAvatarTab(name, rect, panel_avatar),
	mPartnerID()
{
}

void LLPanelAvatarSecondLife::refresh()
{
	updatePartnerName();
}

void LLPanelAvatarSecondLife::updatePartnerName()
{
	if (mPartnerID.notNull())
	{
		char first[128];		/*Flawfinder: ignore*/
		char last[128];		/*Flawfinder: ignore*/
		BOOL found = gCacheName->getName(mPartnerID, first, last);
		if (found)
		{
			childSetTextArg("partner_edit", "[FIRST]", first);
			childSetTextArg("partner_edit", "[LAST]", last);
		}
	}
}

//-----------------------------------------------------------------------------
// clearControls()
// Empty the data out of the controls, since we have to wait for new
// data off the network.
//-----------------------------------------------------------------------------
void LLPanelAvatarSecondLife::clearControls()
{
	LLTextureCtrl*	image_ctrl = LLUICtrlFactory::getTexturePickerByName(this,"img");
	if(image_ctrl)
	{
		image_ctrl->setImageAssetID(LLUUID::null);
	}
	childSetValue("about", "");
	childSetValue("born", "");
	childSetValue("acct", "");

	childSetTextArg("partner_edit", "[FIRST]", "");
	childSetTextArg("partner_edit", "[LAST]", "");

	mPartnerID = LLUUID::null;
	
	LLScrollListCtrl*	group_list = LLUICtrlFactory::getScrollListByName(this,"groups"); 
	if(group_list)
	{
		group_list->deleteAllItems();
	}
	LLScrollListCtrl*	ratings_list = LLUICtrlFactory::getScrollListByName(this,"ratings"); 
	if(ratings_list)
	{
		ratings_list->deleteAllItems();
	}

}


//-----------------------------------------------------------------------------
// enableControls()
//-----------------------------------------------------------------------------
void LLPanelAvatarSecondLife::enableControls(BOOL self)
{
	childSetEnabled("img", self);
	childSetEnabled("about", self);
	childSetVisible("allow_publish", self);
	childSetEnabled("allow_publish", self);
	childSetVisible("?", self);
	childSetEnabled("?", self);

	if (!self)
	{
		// This is because the LLTextEditor
		// appears to reset the read only background color when
		// setEnable is called, for some reason
		LLTextEditor* about = LLUICtrlFactory::getTextEditorByName(this,"about");
		if (about) about->setReadOnlyBgColor(CLEAR);
	}
}


// static
void LLPanelAvatarSecondLife::onClickImage(void *)
{ }

// static
void LLPanelAvatarSecondLife::onDoubleClickGroup(void* data)
{
	LLPanelAvatarSecondLife* self = (LLPanelAvatarSecondLife*)data;

	
	LLScrollListCtrl*	group_list =  LLUICtrlFactory::getScrollListByName(self,"groups"); 
	if(group_list)
	{
		LLScrollListItem* item = group_list->getFirstSelected();
		if(item && item->getUUID().notNull())
		{
			llinfos << "Show group info " << item->getUUID() << llendl;

			LLFloaterGroupInfo::showFromUUID(item->getUUID());
		}
	}
}

// static
void LLPanelAvatarSecondLife::onClickPublishHelp(void *)
{
	gViewerWindow->alertXml("ClickPublishHelpAvatar");
}

// static
void LLPanelAvatarSecondLife::onClickPartnerHelp(void *)
{
    gViewerWindow->alertXml("ClickPartnerHelpAvatar", onClickPartnerHelpLoadURL, (void*) NULL);
}

// static 
void LLPanelAvatarSecondLife::onClickPartnerHelpLoadURL(S32 option, void* userdata)
{
  if (option == 0)
    LLWeb::loadURL("http://secondlife.com/partner");
}


//-----------------------------------------------------------------------------
// LLPanelAvatarFirstLife()
//-----------------------------------------------------------------------------
LLPanelAvatarFirstLife::LLPanelAvatarFirstLife(const std::string& name, 
											   const LLRect &rect, 
											   LLPanelAvatar* panel_avatar ) 
:	LLPanelAvatarTab(name, rect, panel_avatar)
{
}


void LLPanelAvatarFirstLife::enableControls(BOOL self)
{
	childSetEnabled("img", self);
	childSetEnabled("about", self);
}

//-----------------------------------------------------------------------------
// postBuild
//-----------------------------------------------------------------------------

BOOL LLPanelAvatarSecondLife::postBuild(void)
{
	childSetEnabled("born", FALSE);
	childSetEnabled("partner_edit", FALSE);
	childSetAction("partner_help",onClickPartnerHelp,this);
	
	childSetAction("?",onClickPublishHelp,this);
	BOOL own_avatar = (getPanelAvatar()->getAvatarID() == gAgent.getID() );
	enableControls(own_avatar);

	childSetVisible("About:",LLPanelAvatar::sAllowFirstLife);
	childSetVisible("(500 chars)",LLPanelAvatar::sAllowFirstLife);
	childSetVisible("about",LLPanelAvatar::sAllowFirstLife);
	
	childSetVisible("allow_publish",LLPanelAvatar::sAllowFirstLife);
	childSetVisible("?",LLPanelAvatar::sAllowFirstLife);

	childSetVisible("online_yes",FALSE);

	// These are cruft but may still exist in some xml files
	// TODO: remove the following 2 lines once translators grab these changes
	childSetVisible("online_unknown",FALSE);
	childSetVisible("online_no",FALSE);

	childSetAction("Show on Map", LLPanelAvatar::onClickTrack, getPanelAvatar());
	childSetAction("Instant Message...", LLPanelAvatar::onClickIM, getPanelAvatar());
	
	childSetAction("Add Friend...", LLPanelAvatar::onClickAddFriend, getPanelAvatar());
	childSetAction("Pay...", LLPanelAvatar::onClickPay, getPanelAvatar());
	childSetAction("Mute", LLPanelAvatar::onClickMute, getPanelAvatar() );	

	childSetAction("Offer Teleport...", LLPanelAvatar::onClickOfferTeleport, 
		getPanelAvatar() );

	childSetDoubleClickCallback("groups", onDoubleClickGroup, this );

	return TRUE;
}

BOOL LLPanelAvatarFirstLife::postBuild(void)
{
	BOOL own_avatar = (getPanelAvatar()->getAvatarID() == gAgent.getID() );
	enableControls(own_avatar);
	return TRUE;
}

BOOL LLPanelAvatarNotes::postBuild(void)
{
	childSetCommitCallback("notes edit",onCommitNotes,this);
	
	LLTextEditor*	te = LLUICtrlFactory::getTextEditorByName(this,"notes edit");
	if(te) te->setCommitOnFocusLost(TRUE);
	return TRUE;
}

BOOL LLPanelAvatarWeb::postBuild(void)
{
	childSetAction("load",onClickLoad,this);
	childSetAction("open",onClickOpen,this);
	childSetAction("home",onClickLoad,this);
	childSetAction("web_profile_help",onClickWebProfileHelp,this);

	childSetCommitCallback("url_edit",onCommitURL,this);

	childSetControlName("auto_load","AutoLoadWebProfiles");

#if LL_LIBXUL_ENABLED
	mWebBrowser = (LLWebBrowserCtrl*)getChildByName("profile_html");

	// links open in internally 
	mWebBrowser->setOpenInExternalBrowser( false );

	// observe browser events
	mWebBrowser->addObserver( this );
#endif // LL_LIBXUL_ENABLED

	return TRUE;
}

BOOL LLPanelAvatarClassified::postBuild(void)
{
	childSetAction("New...",onClickNew,NULL);
	childSetAction("Delete...",onClickDelete,NULL);
	return TRUE;
}

BOOL LLPanelAvatarPicks::postBuild(void)
{
	childSetAction("New...",onClickNew,NULL);
	childSetAction("Delete...",onClickDelete,NULL);
	return TRUE;
}

BOOL LLPanelAvatarAdvanced::postBuild()
{
	for(size_t ii = 0; ii < kArraySize(mWantToCheck); ++ii)
		mWantToCheck[ii] = NULL;
	for(size_t ii = 0; ii < kArraySize(mSkillsCheck); ++ii)
		mSkillsCheck[ii] = NULL;
	mWantToCount = (8>kArraySize(mWantToCheck))?kArraySize(mWantToCheck):8;
	for(S32 tt=0; tt < mWantToCount; ++tt)
	{	
		LLString ctlname = llformat("chk%d", tt);
		mWantToCheck[tt] = LLUICtrlFactory::getCheckBoxByName(this,ctlname);
	}	
	mSkillsCount = (6>kArraySize(mSkillsCheck))?kArraySize(mSkillsCheck):6;

	for(S32 tt=0; tt < mSkillsCount; ++tt)
	{
		//Find the Skills checkboxes and save off thier controls
		LLString ctlname = llformat("schk%d",tt);
		mSkillsCheck[tt] = LLUICtrlFactory::getCheckBoxByName(this,ctlname);
	}

	mWantToEdit = LLUICtrlFactory::getLineEditorByName(this,"want_to_edit");
	mSkillsEdit = LLUICtrlFactory::getLineEditorByName(this,"skills_edit");
	childSetVisible("skills_edit",LLPanelAvatar::sAllowFirstLife);
	childSetVisible("want_to_edit",LLPanelAvatar::sAllowFirstLife);

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLPanelAvatarWeb
//-----------------------------------------------------------------------------
LLPanelAvatarWeb::LLPanelAvatarWeb(const std::string& name, const LLRect& rect, 
								   LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar),
	mWebBrowser(NULL)
{
}

LLPanelAvatarWeb::~LLPanelAvatarWeb()
{
#if LL_LIBXUL_ENABLED
	// stop observing browser events
	if  ( mWebBrowser )
	{
		mWebBrowser->remObserver( this );
	};
#endif
}

void LLPanelAvatarWeb::enableControls(BOOL self)
{	
	childSetEnabled("url_edit",self);
	childSetVisible("status_text",!self);
}

void LLPanelAvatarWeb::setWebURL(std::string url)
{
	bool changed_url = (mURL != url);

	mURL = url;
	bool have_url = !mURL.empty();
	
	childSetText("url_edit",mURL);

	childSetEnabled("load",have_url);
	childSetEnabled("open",have_url);

	childSetVisible("home",false);
	childSetVisible("load",true);

	if (have_url
		&& gSavedSettings.getBOOL("AutoLoadWebProfiles"))
	{
		if (changed_url)
		{
			load();
		}
	}
	else
	{
		childSetVisible("profile_html",false);
	}
	
#if !LL_LIBXUL_ENABLED
	childSetVisible("load",false);
	childSetVisible("profile_html",false);
	childSetVisible("status_text",false);
#endif

}

// static
void LLPanelAvatarWeb::onCommitURL(LLUICtrl* ctrl, void* data)
{
	LLPanelAvatarWeb* self = (LLPanelAvatarWeb*)data;

	if (!self) return;
	
	self->load( self->childGetText("url_edit") );
}

// static
void LLPanelAvatarWeb::onClickWebProfileHelp(void *)
{
#if LL_LIBXUL_ENABLED
	gViewerWindow->alertXml("ClickWebProfileHelpAvatar");
#else
	gViewerWindow->alertXml("ClickWebProfileNoWebHelpAvatar");
#endif
}

void LLPanelAvatarWeb::load(std::string url)
{
	bool have_url = (!url.empty());

#if LL_LIBXUL_ENABLED
	if (have_url)
	{
		llinfos << "Loading " << url << llendl;
		mWebBrowser->navigateTo( url );
	}

	// If we have_url then we loaded so use the home button
	// Or if the url in address bar is not the home url show the home button.
	bool use_home = (have_url
					 || url != mURL);
					 
	childSetVisible("profile_html",have_url);
	childSetVisible("load",!use_home);	
	childSetVisible("home",use_home);

	childSetEnabled("load",!use_home);
	childSetEnabled("home",use_home);
	childSetEnabled("open",have_url);
	
#else
	childSetEnabled("open",have_url);
#endif
}

void LLPanelAvatarWeb::load()
{
	load(mURL);
}

// static
void LLPanelAvatarWeb::onClickLoad(void* data)
{
	LLPanelAvatarWeb* self = (LLPanelAvatarWeb*)data;

	if (!self) return;
	
	self->load();
}

// static
void LLPanelAvatarWeb::onClickOpen(void* data)
{
	LLPanelAvatarWeb* self = (LLPanelAvatarWeb*)data;

	if (!self) return;

	std::string url = self->childGetText("url_edit");
	if (!url.empty())
	{
		LLWeb::loadURLExternal(url);
	}
}

#if LL_LIBXUL_ENABLED
void LLPanelAvatarWeb::onStatusTextChange( const EventType& eventIn )
{
	childSetText("status_text", eventIn.getStringValue() );
}

void LLPanelAvatarWeb::onLocationChange( const EventType& eventIn )
{
	childSetText("url_edit", eventIn.getStringValue() );
}
#endif


//-----------------------------------------------------------------------------
// LLPanelAvatarAdvanced
//-----------------------------------------------------------------------------
LLPanelAvatarAdvanced::LLPanelAvatarAdvanced(const std::string& name, 
											 const LLRect& rect, 
											 LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar),
	mWantToCount(0),
	mSkillsCount(0),
	mWantToEdit( NULL ),
	mSkillsEdit( NULL )
{
}

void LLPanelAvatarAdvanced::enableControls(BOOL self)
{
	S32 t;
	for(t=0;t<mWantToCount;t++)
	{
		if(mWantToCheck[t])mWantToCheck[t]->setEnabled(self);
	}
	for(t=0;t<mSkillsCount;t++)
	{
		if(mSkillsCheck[t])mSkillsCheck[t]->setEnabled(self);
	}

	if (mWantToEdit) mWantToEdit->setEnabled(self);
	if (mSkillsEdit) mSkillsEdit->setEnabled(self);
	childSetEnabled("languages_edit",self);

	if (!self)
	{
		// This is because the LLTextEditor
		// appears to reset the read only background color when
		// setEnable is called, for some reason
		if (mWantToEdit) mWantToEdit->setReadOnlyBgColor(CLEAR);
		if (mSkillsEdit) mSkillsEdit->setReadOnlyBgColor(CLEAR);
		LLLineEditor* languages_edit = (LLLineEditor*)getChildByName("languages_edit");
		languages_edit->setReadOnlyBgColor(CLEAR);
	}
}

void LLPanelAvatarAdvanced::setWantSkills(U32 want_to_mask, const std::string& want_to_text,
										  U32 skills_mask, const std::string& skills_text,
										  const std::string& languages_text)
{
	for(int id =0;id<mWantToCount;id++)
	{
		mWantToCheck[id]->set( want_to_mask & 1<<id );
	}
	for(int id =0;id<mSkillsCount;id++)
	{
		mSkillsCheck[id]->set( skills_mask & 1<<id );
	}
	if (mWantToEdit && mSkillsEdit)
	{
		mWantToEdit->setText( want_to_text );
		mSkillsEdit->setText( skills_text );
	}

	childSetText("languages_edit",languages_text);
}

void LLPanelAvatarAdvanced::getWantSkills(U32* want_to_mask, std::string& want_to_text,
										  U32* skills_mask, std::string& skills_text,
										  std::string& languages_text)
{
	if (want_to_mask)
	{
		*want_to_mask = 0;
		for(int t=0;t<mWantToCount;t++)
		{
			if(mWantToCheck[t]->get())
				*want_to_mask |= 1<<t;
		}
	}
	if (skills_mask)
	{
		*skills_mask = 0;
		for(int t=0;t<mSkillsCount;t++)
		{
			if(mSkillsCheck[t]->get())
				*skills_mask |= 1<<t;
		}
	}
	if (mWantToEdit)
	{
		want_to_text = mWantToEdit->getText();
	}

	if (mSkillsEdit)
	{
		skills_text = mSkillsEdit->getText();
	}

	languages_text = childGetText("languages_edit");
}	

//-----------------------------------------------------------------------------
// LLPanelAvatarNotes()
//-----------------------------------------------------------------------------
LLPanelAvatarNotes::LLPanelAvatarNotes(const std::string& name, const LLRect& rect, LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar)
{
}

void LLPanelAvatarNotes::refresh()
{
	sendAvatarProfileRequestIfNeeded("avatarnotesrequest");
}

void LLPanelAvatarNotes::clearControls()
{
	childSetText("notes edit", LOADING_MSG);
	childSetEnabled("notes edit", false);
}

// static
void LLPanelAvatarNotes::onCommitNotes(LLUICtrl*, void* userdata)
{
	LLPanelAvatarNotes* self = (LLPanelAvatarNotes*)userdata;

	self->getPanelAvatar()->sendAvatarNotesUpdate();
}


//-----------------------------------------------------------------------------
// LLPanelAvatarClassified()
//-----------------------------------------------------------------------------
LLPanelAvatarClassified::LLPanelAvatarClassified(const LLString& name, const LLRect& rect,
									   LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar)
{
}


void LLPanelAvatarClassified::refresh()
{
	BOOL self = (gAgent.getID() == getPanelAvatar()->getAvatarID());
	
	LLTabContainerCommon* tabs = LLUICtrlFactory::getTabContainerByName(this,"classified tab");
	
	S32 tab_count = tabs ? tabs->getTabCount() : 0;

	bool allow_new = tab_count < MAX_CLASSIFIEDS;
	bool allow_delete = (tab_count > 0);
	bool show_help = (tab_count == 0);

	// *HACK: Don't allow making new classifieds from inside the directory.
	// The logic for save/don't save when closing is too hairy, and the 
	// directory is conceptually read-only. JC
	bool in_directory = false;
	LLView* view = this;
	while (view)
	{
		if (view->getName() == "directory")
		{
			in_directory = true;
			break;
		}
		view = view->getParent();
	}
	childSetEnabled("New...", self && !in_directory && allow_new);
	childSetVisible("New...", !in_directory);
	childSetEnabled("Delete...", self && !in_directory && allow_delete);
	childSetVisible("Delete...", !in_directory);
	childSetVisible("classified tab",!show_help);

	sendAvatarProfileRequestIfNeeded("avatarclassifiedsrequest");
}


BOOL LLPanelAvatarClassified::canClose()
{
	LLTabContainerCommon* tabs = LLViewerUICtrlFactory::getTabContainerByName(this, "classified tab");
	for (S32 i = 0; i < tabs->getTabCount(); i++)
	{
		LLPanelClassified* panel = (LLPanelClassified*)tabs->getPanelByIndex(i);
		if (!panel->canClose())
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL LLPanelAvatarClassified::titleIsValid()
{
	LLTabContainerCommon* tabs = LLViewerUICtrlFactory::getTabContainerByName(this, "classified tab");
	if ( tabs )
	{
		LLPanelClassified* panel = (LLPanelClassified*)tabs->getCurrentPanel();
		if ( panel )
		{
			if ( ! panel->titleIsValid() )
			{
				return FALSE;
			};
		};
	};

	return TRUE;
}

void LLPanelAvatarClassified::apply()
{
	LLTabContainerCommon* tabs = LLViewerUICtrlFactory::getTabContainerByName(this, "classified tab");
	for (S32 i = 0; i < tabs->getTabCount(); i++)
	{
		LLPanelClassified* panel = (LLPanelClassified*)tabs->getPanelByIndex(i);
		panel->apply();
	}
}


void LLPanelAvatarClassified::deleteClassifiedPanels()
{
	LLTabContainerCommon* tabs = LLViewerUICtrlFactory::getTabContainerByName(this,"classified tab");
	if (tabs)
	{
		tabs->deleteAllTabs();
	}

	childSetVisible("New...", false);
	childSetVisible("Delete...", false);
	childSetVisible("loading_text", true);
}


void LLPanelAvatarClassified::processAvatarClassifiedReply(LLMessageSystem* msg, void**)
{
	S32 block = 0;
	S32 block_count = 0;
	LLUUID classified_id;
	char classified_name[DB_PICK_NAME_SIZE];		/*Flawfinder: ignore*/
	LLPanelClassified* panel_classified = NULL;

	LLTabContainerCommon* tabs = LLViewerUICtrlFactory::getTabContainerByName(this,"classified tab");

	// Don't remove old panels.  We need to be able to process multiple
	// packets for people who have lots of classifieds. JC

	block_count = msg->getNumberOfBlocksFast(_PREHASH_Data);
	for (block = 0; block < block_count; block++)
	{
		msg->getUUIDFast(_PREHASH_Data, _PREHASH_ClassifiedID, classified_id, block);
		msg->getStringFast(_PREHASH_Data, _PREHASH_Name, DB_PICK_NAME_SIZE, classified_name, block);

		panel_classified = new LLPanelClassified(FALSE);

		panel_classified->setClassifiedID(classified_id);

		// This will request data from the server when the pick is first drawn.
		panel_classified->markForServerRequest();

		// The button should automatically truncate long names for us
		if(tabs)
		{
			tabs->addTabPanel(panel_classified, classified_name);
		}
	}

	// Make sure somebody is highlighted.  This works even if there
	// are no tabs in the container.
	if(tabs)
	{
		tabs->selectFirstTab();
	}

	childSetVisible("New...", true);
	childSetVisible("Delete...", true);
	childSetVisible("loading_text", false);
}


// Create a new classified panel.  It will automatically handle generating
// its own id when it's time to save.
// static
void LLPanelAvatarClassified::onClickNew(void* data)
{
	LLPanelAvatarClassified* self = (LLPanelAvatarClassified*)data;

	gViewerWindow->alertXml("AddClassified",callbackNew,self);
		
}

// static
void LLPanelAvatarClassified::callbackNew(S32 option, void* data)
{
	LLPanelAvatarClassified* self = (LLPanelAvatarClassified*)data;

	if (0 == option)
	{
		LLPanelClassified* panel_classified = new LLPanelClassified(FALSE);
		panel_classified->initNewClassified();
		LLTabContainerCommon*	tabs = LLViewerUICtrlFactory::getTabContainerByName(self,"classified tab");
		if(tabs)
		{
			tabs->addTabPanel(panel_classified, panel_classified->getClassifiedName());
			tabs->selectLastTab();
		}
	}
}


// static
void LLPanelAvatarClassified::onClickDelete(void* data)
{
	LLPanelAvatarClassified* self = (LLPanelAvatarClassified*)data;

	LLTabContainerCommon*	tabs = LLViewerUICtrlFactory::getTabContainerByName(self,"classified tab");
	LLPanelClassified* panel_classified = NULL;
	if(tabs)
	{
		panel_classified = (LLPanelClassified*)tabs->getCurrentPanel();
	}
	if (!panel_classified) return;

	LLStringBase<char>::format_map_t args;
	args["[NAME]"] = panel_classified->getClassifiedName();
	gViewerWindow->alertXml("DeleteClassified", args, callbackDelete, self);
		
}


// static
void LLPanelAvatarClassified::callbackDelete(S32 option, void* data)
{
	LLPanelAvatarClassified* self = (LLPanelAvatarClassified*)data;
	LLTabContainerCommon*	tabs = LLViewerUICtrlFactory::getTabContainerByName(self,"classified tab");
	LLPanelClassified* panel_classified=NULL;
	if(tabs)
	{
		panel_classified = (LLPanelClassified*)tabs->getCurrentPanel();
	}
	
	LLMessageSystem* msg = gMessageSystem;

	if (!panel_classified) return;

	if (0 == option)
	{
		msg->newMessageFast(_PREHASH_ClassifiedDelete);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Data);
		msg->addUUIDFast(_PREHASH_ClassifiedID, panel_classified->getClassifiedID());
		gAgent.sendReliableMessage();

		if(tabs)
		{
			tabs->removeTabPanel(panel_classified);
		}
		delete panel_classified;
		panel_classified = NULL;
	}
}


//-----------------------------------------------------------------------------
// LLPanelAvatarPicks()
//-----------------------------------------------------------------------------
LLPanelAvatarPicks::LLPanelAvatarPicks(const std::string& name, 
									   const LLRect& rect,
									   LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar)
{
}


void LLPanelAvatarPicks::refresh()
{
	BOOL self = (gAgent.getID() == getPanelAvatar()->getAvatarID());

	LLTabContainerCommon*	tabs = LLViewerUICtrlFactory::getTabContainerByName(this,"picks tab");
	S32 tab_count = tabs ? tabs->getTabCount() : 0;
	BOOL allow_new = (tab_count < MAX_AVATAR_PICKS);
	BOOL allow_delete = (tab_count > 0);

	childSetEnabled("New...",self && allow_new);
	childSetEnabled("Delete...",self && allow_delete);

	sendAvatarProfileRequestIfNeeded("avatarpicksrequest");
}


void LLPanelAvatarPicks::deletePickPanels()
{
	LLTabContainerCommon* tabs = LLUICtrlFactory::getTabContainerByName(this,"picks tab");
	if(tabs)
	{
		tabs->deleteAllTabs();
	}

	childSetVisible("New...", false);
	childSetVisible("Delete...", false);
	childSetVisible("loading_text", true);
}

void LLPanelAvatarPicks::processAvatarPicksReply(LLMessageSystem* msg, void**)
{
	S32 block = 0;
	S32 block_count = 0;
	LLUUID pick_id;
	char pick_name[DB_PICK_NAME_SIZE];		/*Flawfinder: ignore*/
	LLPanelPick* panel_pick = NULL;

	LLTabContainerCommon* tabs =  LLUICtrlFactory::getTabContainerByName(this,"picks tab");

	// Clear out all the old panels.  We'll replace them with the correct
	// number of new panels.
	deletePickPanels();

	// The database needs to know for which user to look up picks.
	LLUUID avatar_id = getPanelAvatar()->getAvatarID();
	
	block_count = msg->getNumberOfBlocks("Data");
	for (block = 0; block < block_count; block++)
	{
		msg->getUUID("Data", "PickID", pick_id, block);
		msg->getString("Data", "PickName", DB_PICK_NAME_SIZE, pick_name, block);

		panel_pick = new LLPanelPick(FALSE);

		panel_pick->setPickID(pick_id, avatar_id);

		// This will request data from the server when the pick is first
		// drawn.
		panel_pick->markForServerRequest();

		// The button should automatically truncate long names for us
		if(tabs)
		{
			tabs->addTabPanel(panel_pick, pick_name);
		}
	}

	// Make sure somebody is highlighted.  This works even if there
	// are no tabs in the container.
	if(tabs)
	{
		tabs->selectFirstTab();
	}

	childSetVisible("New...", true);
	childSetVisible("Delete...", true);
	childSetVisible("loading_text", false);
}


// Create a new pick panel.  It will automatically handle generating
// its own id when it's time to save.
// static
void LLPanelAvatarPicks::onClickNew(void* data)
{
	LLPanelAvatarPicks* self = (LLPanelAvatarPicks*)data;
	LLPanelPick* panel_pick = new LLPanelPick(FALSE);
	LLTabContainerCommon* tabs =  LLUICtrlFactory::getTabContainerByName(self,"picks tab");

	panel_pick->initNewPick();
	if(tabs)
	{
		tabs->addTabPanel(panel_pick, panel_pick->getPickName());
		tabs->selectLastTab();
	}
}


// static
void LLPanelAvatarPicks::onClickDelete(void* data)
{
	LLPanelAvatarPicks* self = (LLPanelAvatarPicks*)data;
	LLTabContainerCommon* tabs =  LLUICtrlFactory::getTabContainerByName(self,"picks tab");
	LLPanelPick* panel_pick = tabs?(LLPanelPick*)tabs->getCurrentPanel():NULL;

	if (!panel_pick) return;

	LLString::format_map_t args;
	args["[PICK]"] = panel_pick->getPickName();

	gViewerWindow->alertXml("DeleteAvatarPick", args,
		callbackDelete,
		self);
}


// static
void LLPanelAvatarPicks::callbackDelete(S32 option, void* data)
{
	LLPanelAvatarPicks* self = (LLPanelAvatarPicks*)data;
	LLTabContainerCommon* tabs = LLUICtrlFactory::getTabContainerByName(self,"picks tab");
	LLPanelPick* panel_pick = tabs?(LLPanelPick*)tabs->getCurrentPanel():NULL;
	LLMessageSystem* msg = gMessageSystem;

	if (!panel_pick) return;

	if (0 == option)
	{
		// If the viewer has a hacked god-mode, then this call will
		// fail.
		if(gAgent.isGodlike())
		{
			msg->newMessage("PickGodDelete");			
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID());
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->nextBlock("Data");
			msg->addUUID("PickID", panel_pick->getPickID());
			// *HACK: We need to send the pick's creator id to accomplish
			// the delete, and we don't use the query id for anything. JC
			msg->addUUID( "QueryID", panel_pick->getPickCreatorID() );
		}
		else
		{
			msg->newMessage("PickDelete");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID());
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->nextBlock("Data");
			msg->addUUID("PickID", panel_pick->getPickID());
		}
		gAgent.sendReliableMessage();

		if(tabs)
		{
			tabs->removeTabPanel(panel_pick);
		}
		delete panel_pick;
		panel_pick = NULL;
	}
}


//-----------------------------------------------------------------------------
// LLPanelAvatar
//-----------------------------------------------------------------------------
LLPanelAvatar::LLPanelAvatar(
	const std::string& name,
	const LLRect &rect,
	BOOL allow_edit)
	:
	LLPanel(name, rect, FALSE),
	mPanelSecondLife(NULL),
	mPanelAdvanced(NULL),
	mPanelClassified(NULL),
	mPanelPicks(NULL),
	mPanelNotes(NULL),
	mPanelFirstLife(NULL),
	mPanelWeb(NULL),
	mDropTarget(NULL),
	mAvatarID( LLUUID::null ),	// mAvatarID is set with 'setAvatar' or 'setAvatarID'
	mHaveProperties(FALSE),
	mHaveStatistics(FALSE),
	mHaveNotes(false),
	mLastNotes(),
	mAllowEdit(allow_edit)
{

	sAllPanels.push_back(this);

	LLCallbackMap::map_t factory_map;

	factory_map["2nd Life"] = LLCallbackMap(createPanelAvatarSecondLife, this);
	factory_map["WebProfile"] = LLCallbackMap(createPanelAvatarWeb, this);
	factory_map["Interests"] = LLCallbackMap(createPanelAvatarInterests, this);
	factory_map["Picks"] = LLCallbackMap(createPanelAvatarPicks, this);
	factory_map["Classified"] = LLCallbackMap(createPanelAvatarClassified, this);
	factory_map["1st Life"] = LLCallbackMap(createPanelAvatarFirstLife, this);
	factory_map["My Notes"] = LLCallbackMap(createPanelAvatarNotes, this);
	
	gUICtrlFactory->buildPanel(this, "panel_avatar.xml", &factory_map);

	selectTab(0);
	

}

BOOL LLPanelAvatar::postBuild(void)
{
	mTab = LLUICtrlFactory::getTabContainerByName(this,"tab");
	childSetAction("Kick",onClickKick,this);
	childSetAction("Freeze",onClickFreeze, this);
	childSetAction("Unfreeze", onClickUnfreeze, this);
	childSetAction("csr_btn", onClickCSR, this);
	childSetAction("OK", onClickOK, this);
	childSetAction("Cancel", onClickCancel, this);

	if(mTab && !sAllowFirstLife)
	{
		LLPanel* panel = mTab->getPanelByName("1st Life");
		if (panel) mTab->removeTabPanel(panel);

		panel = mTab->getPanelByName("WebProfile");
		if (panel) mTab->removeTabPanel(panel);
	}
	childSetVisible("Kick",FALSE);
	childSetEnabled("Kick",FALSE);
	childSetVisible("Freeze",FALSE);
	childSetEnabled("Freeze",FALSE);
	childSetVisible("Unfreeze",FALSE);
	childSetEnabled("Unfreeze",FALSE);
	childSetVisible("csr_btn", FALSE);
	childSetEnabled("csr_btn", FALSE);

	return TRUE;
}


LLPanelAvatar::~LLPanelAvatar()
{
	sAllPanels.remove(this);
}


BOOL LLPanelAvatar::canClose()
{
	return mPanelClassified && mPanelClassified->canClose();
}

void LLPanelAvatar::setAvatar(LLViewerObject *avatarp)
{
	// find the avatar and grab the name
	LLNameValue *firstname = avatarp->getNVPair("FirstName");
	LLNameValue *lastname = avatarp->getNVPair("LastName");

	LLString name;
	if (firstname && lastname)
	{
		name.assign( firstname->getString() );
		name.append(" ");
		name.append( lastname->getString() );
	}
	else
	{
		name.assign("");
	}

	// If we have an avatar pointer, they must be online.
	setAvatarID(avatarp->getID(), name, ONLINE_STATUS_YES);
}

void LLPanelAvatar::setOnlineStatus(EOnlineStatus online_status)
{
	// Online status NO could be because they are hidden
	// If they are a friend, we may know the truth!
	if ((ONLINE_STATUS_YES != online_status)
		&& mIsFriend
		&& (LLAvatarTracker::instance().isBuddyOnline( mAvatarID )))
	{
		online_status = ONLINE_STATUS_YES;
	}

	mPanelSecondLife->childSetVisible("online_yes", (online_status == ONLINE_STATUS_YES));

	// Since setOnlineStatus gets called after setAvatarID
	// need to make sure that "Offer Teleport" doesn't get set
	// to TRUE again for yourself
	if (mAvatarID != gAgent.getID())
	{
		childSetVisible("Offer Teleport...",TRUE);
	}

	BOOL in_prelude = gAgent.inPrelude();
	if(gAgent.isGodlike())
	{
		childSetEnabled("Offer Teleport...", TRUE);
		childSetToolTip("Offer Teleport...", childGetValue("TeleportGod").asString());
	}
	else if (in_prelude)
	{
		childSetEnabled("Offer Teleport...",FALSE);
		childSetToolTip("Offer Teleport...",childGetValue("TeleportPrelude").asString());
	}
	else
	{
		childSetEnabled("Offer Teleport...", (online_status == ONLINE_STATUS_YES));
		childSetToolTip("Offer Teleport...", childGetValue("TeleportNormal").asString());
	}
}

void LLPanelAvatar::setAvatarID(const LLUUID &avatar_id, const LLString &name,
								EOnlineStatus online_status)
{
	if (avatar_id.isNull()) return;

	BOOL avatar_changed = FALSE;
	if (avatar_id != mAvatarID)
	{
		avatar_changed = TRUE;
	}
	mAvatarID = avatar_id;

	// Determine if we have their calling card.
	mIsFriend = is_agent_friend(mAvatarID); 

	// setOnlineStatus uses mIsFriend
	setOnlineStatus(online_status);
	
	BOOL own_avatar = (mAvatarID == gAgent.getID() );
	BOOL avatar_is_friend = LLAvatarTracker::instance().getBuddyInfo(mAvatarID) != NULL;

	mPanelSecondLife->enableControls(own_avatar && mAllowEdit);
	mPanelWeb->enableControls(own_avatar && mAllowEdit);
	mPanelAdvanced->enableControls(own_avatar && mAllowEdit);
	// Teens don't have this.
	if (mPanelFirstLife) mPanelFirstLife->enableControls(own_avatar && mAllowEdit);

	LLView *target_view = getChildByName("drop_target_rect", TRUE);
	if(target_view)
	{
		if (mDropTarget)
		{
			delete mDropTarget;
		}
		mDropTarget = new LLDropTarget("drop target", target_view->getRect(), mAvatarID);
		addChild(mDropTarget);
		mDropTarget->setAgentID(mAvatarID);
	}

	LLNameEditor* name_edit = LLViewerUICtrlFactory::getNameEditorByName(this, "name");
	if(name_edit)
	{
		if (name.empty())
		{
			name_edit->setNameID(avatar_id, FALSE);
		}
		else
		{
			name_edit->setText(name);
		}
	}
// 	if (avatar_changed)
	{
		// While we're waiting for data off the network, clear out the
		// old data.
		mPanelSecondLife->clearControls();

		mPanelPicks->deletePickPanels();
		mPanelPicks->setDataRequested(false);

		mPanelClassified->deleteClassifiedPanels();
		mPanelClassified->setDataRequested(false);

		mPanelNotes->clearControls();
		mPanelNotes->setDataRequested(false);
		mHaveNotes = false;
		mLastNotes.clear();

		// Request just the first two pages of data.  The picks,
		// classifieds, and notes will be requested when that panel
		// is made visible. JC
		sendAvatarPropertiesRequest();

		if (own_avatar)
		{
			if (mAllowEdit)
			{
				// OK button disabled until properties data arrives
				childSetVisible("OK", true);
				childSetEnabled("OK", false);
				childSetVisible("Cancel",TRUE);
				childSetEnabled("Cancel",TRUE);
			}
			else
			{
				childSetVisible("OK",FALSE);
				childSetEnabled("OK",FALSE);
			}
			childSetVisible("Instant Message...",FALSE);
			childSetEnabled("Instant Message...",FALSE);
			childSetVisible("Mute",FALSE);
			childSetEnabled("Mute",FALSE);
			childSetVisible("Offer Teleport...",FALSE);
			childSetEnabled("Offer Teleport...",FALSE);
			childSetVisible("drop target",FALSE);
			childSetEnabled("drop target",FALSE);
			childSetVisible("Show on Map",FALSE);
			childSetEnabled("Show on Map",FALSE);
			childSetVisible("Add Friend...",FALSE);
			childSetEnabled("Add Friend...",FALSE);
			childSetVisible("Pay...",FALSE);
			childSetEnabled("Pay...",FALSE);
		}
		else
		{
			childSetVisible("OK",FALSE);
			childSetEnabled("OK",FALSE);

			childSetVisible("Cancel",FALSE);
			childSetEnabled("Cancel",FALSE);

			childSetVisible("Instant Message...",TRUE);
			childSetEnabled("Instant Message...",FALSE);
			childSetToolTip("Instant Message...",IM_ENABLED_TOOLTIP);
			childSetVisible("Mute",TRUE);
			childSetEnabled("Mute",FALSE);

			childSetVisible("drop target",TRUE);
			childSetEnabled("drop target",FALSE);

			childSetVisible("Show on Map",TRUE);
			// Note: we don't always know online status, so always allow gods to try to track
			BOOL enable_track = gAgent.isGodlike() || is_agent_mappable(mAvatarID);
			childSetEnabled("Show on Map",enable_track);
			if (!mIsFriend)
			{
				childSetToolTip("Show on Map",childGetValue("ShowOnMapNonFriend").asString());
			}
			else if (ONLINE_STATUS_YES != online_status)
			{
				childSetToolTip("Show on Map",childGetValue("ShowOnMapFriendOffline").asString());
			}
			else
			{
				childSetToolTip("Show on Map",childGetValue("ShowOnMapFriendOnline").asString());
			}
			childSetVisible("Add Friend...", true);
			childSetEnabled("Add Friend...", !avatar_is_friend);
			childSetVisible("Pay...",TRUE);
			childSetEnabled("Pay...",FALSE);
		}
	}
	
	BOOL is_god = FALSE;
	if (gAgent.isGodlike()) is_god = TRUE;
	
	childSetVisible("Kick", is_god);
	childSetEnabled("Kick", is_god);
	childSetVisible("Freeze", is_god);
	childSetEnabled("Freeze", is_god);
	childSetVisible("Unfreeze", is_god);
	childSetEnabled("Unfreeze", is_god);
	childSetVisible("csr_btn", is_god);
	childSetEnabled("csr_btn", is_god);
}


void LLPanelAvatar::resetGroupList()
{
	// only get these updates asynchronously via the group floater, which works on the agent only
	if (mAvatarID != gAgent.getID())
	{
		return;
	}
		
	if (mPanelSecondLife)
	{
		LLScrollListCtrl* group_list = LLUICtrlFactory::getScrollListByName(mPanelSecondLife,"groups");
		if (group_list)
		{
			group_list->deleteAllItems();
			
			S32 count = gAgent.mGroups.count();
			LLUUID id;
			
			for(S32 i = 0; i < count; ++i)
			{
				LLGroupData group_data = gAgent.mGroups.get(i);
				id = group_data.mID;
				std::string group_string;
				/* Show group title?  DUMMY_POWER for Don Grep
				   if(group_data.mOfficer)
				   {
				   group_string = "Officer of ";
				   }
				   else
				   {
				   group_string = "Member of ";
				   }
				*/

				group_string += group_data.mName;

				LLSD row;
				row["columns"][0]["value"] = group_string;
				row["columns"][0]["font"] = "SANSSERIF_SMALL";
				row["columns"][0]["width"] = 0;
				group_list->addElement(row);
			}
			group_list->sortByColumn(0, TRUE);
		}
	}
}

// static
//-----------------------------------------------------------------------------
// onClickIM()
//-----------------------------------------------------------------------------
void LLPanelAvatar::onClickIM(void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	gIMMgr->setFloaterOpen(TRUE);

	std::string name;
	LLNameEditor* nameedit = LLViewerUICtrlFactory::getNameEditorByName(self->mPanelSecondLife, "name");
	if (nameedit) name = nameedit->getText();
	gIMMgr->addSession(name, IM_NOTHING_SPECIAL, self->mAvatarID);
}


// static
//-----------------------------------------------------------------------------
// onClickTrack()
//-----------------------------------------------------------------------------
void LLPanelAvatar::onClickTrack(void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	
	if( gFloaterWorldMap )
	{
		std::string name;
		LLNameEditor* nameedit = LLViewerUICtrlFactory::getNameEditorByName(self->mPanelSecondLife, "name");
		if (nameedit) name = nameedit->getText();
		gFloaterWorldMap->trackAvatar(self->mAvatarID, name);
		LLFloaterWorldMap::show(NULL, TRUE);
	}
}


// static
void LLPanelAvatar::onClickAddFriend(void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	LLNameEditor* name_edit = LLViewerUICtrlFactory::getNameEditorByName(self->mPanelSecondLife, "name");	
	if (name_edit)
	{
		LLPanelFriends::requestFriendshipDialog(self->getAvatarID(),
												  name_edit->getText());
	}
}

//-----------------------------------------------------------------------------
// onClickMute()
//-----------------------------------------------------------------------------
void LLPanelAvatar::onClickMute(void *userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	
	LLUUID agent_id = self->getAvatarID();
	LLNameEditor* name_edit = LLViewerUICtrlFactory::getNameEditorByName(self->mPanelSecondLife, "name");
	
	if (name_edit)
	{
		std::string agent_name = name_edit->getText();
		gFloaterMute->show();
		
		if (gMuteListp->isMuted(agent_id))
		{
			gFloaterMute->selectMute(agent_id);
		}
		else
		{
			LLMute mute(agent_id, agent_name, LLMute::AGENT);
			gMuteListp->add(mute);
		}
	}
}


// static
void LLPanelAvatar::onClickOfferTeleport(void *userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;

	handle_lure(self->mAvatarID);
}


// static
void LLPanelAvatar::onClickPay(void *userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	handle_pay_by_id(self->mAvatarID);
}


// static
void LLPanelAvatar::onClickOK(void *userdata)
{
	LLPanelAvatar *self = (LLPanelAvatar *)userdata;

	// JC: Only save the data if we actually got the original
	// properties.  Otherwise we might save blanks into
	// the database.
	if (self
		&& self->mHaveProperties)
	{
		self->sendAvatarPropertiesUpdate();

		LLTabContainerCommon* tabs = LLUICtrlFactory::getTabContainerByName(self,"tab");
		if ( tabs->getCurrentPanel() != self->mPanelClassified )
		{
			self->mPanelClassified->apply();

			LLFloaterAvatarInfo *infop = LLFloaterAvatarInfo::getInstance(self->mAvatarID);
			if (infop)
			{
				infop->close();
			}
		}
		else
		{
			if ( self->mPanelClassified->titleIsValid() )
			{
				self->mPanelClassified->apply();

				LLFloaterAvatarInfo *infop = LLFloaterAvatarInfo::getInstance(self->mAvatarID);
				if (infop)
				{
					infop->close();
				}
			}
		}
	}
}

// static
void LLPanelAvatar::onClickCancel(void *userdata)
{
	LLPanelAvatar *self = (LLPanelAvatar *)userdata;

	if (self)
	{
		LLFloaterAvatarInfo *infop;
		if ((infop = LLFloaterAvatarInfo::getInstance(self->mAvatarID)))
		{
			infop->close();
		}
		else
		{
			// We're in the Search directory and are cancelling an edit
			// to our own profile, so reset.
			self->sendAvatarPropertiesRequest();
		}
	}
}


void LLPanelAvatar::sendAvatarPropertiesRequest()
{
	lldebugs << "LLPanelAvatar::sendAvatarPropertiesRequest()" << llendl; 
	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_AvatarPropertiesRequest);
	msg->nextBlockFast( _PREHASH_AgentData);
	msg->addUUIDFast(   _PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(   _PREHASH_AvatarID, mAvatarID);
	gAgent.sendReliableMessage();
}

void LLPanelAvatar::sendAvatarNotesUpdate()
{
	std::string notes = mPanelNotes->childGetValue("notes edit").asString();

	if (!mHaveNotes
		&& (notes.empty() || notes == LOADING_MSG))
	{
		// no notes from server and no user updates
		return;
	}
	if (notes == mLastNotes)
	{
		// Avatar notes unchanged
		return;
	}

	LLMessageSystem *msg = gMessageSystem;

	msg->newMessage("AvatarNotesUpdate");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlock("Data");
	msg->addUUID("TargetID", mAvatarID);
	msg->addString("Notes", notes);

	gAgent.sendReliableMessage();
}


// static
void LLPanelAvatar::processAvatarPropertiesReply(LLMessageSystem *msg, void**)
{
	LLUUID	agent_id;	// your id
	LLUUID	avatar_id;	// target of this panel
	LLUUID	image_id;
	LLUUID	fl_image_id;
	LLUUID	partner_id;
	char	about_text[DB_USER_ABOUT_BUF_SIZE];		/*Flawfinder: ignore*/
	char	fl_about_text[DB_USER_FL_ABOUT_BUF_SIZE];		/*Flawfinder: ignore*/
	char	born_on[DB_BORN_BUF_SIZE];		/*Flawfinder: ignore*/
	S32		charter_member_size = 0;
	BOOL	allow_publish = FALSE;
	//BOOL	mature = FALSE;
	BOOL	identified = FALSE;
	BOOL	transacted = FALSE;
	BOOL	online = FALSE;
	char	profile_url[DB_USER_PROFILE_URL_BUF_SIZE];		/*Flawfinder: ignore*/

	U32		flags = 0x0;

	//llinfos << "properties packet size " << msg->getReceiveSize() << llendl;

	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AvatarID, avatar_id );

	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID != avatar_id)
		{
			continue;
		}
		self->childSetEnabled("Instant Message...",TRUE);
		self->childSetEnabled("Pay...",TRUE);
		self->childSetEnabled("Mute",TRUE);

		self->childSetEnabled("drop target",TRUE);

		self->mHaveProperties = TRUE;
		self->enableOKIfReady();

		msg->getUUIDFast(  _PREHASH_PropertiesData,	_PREHASH_ImageID,		image_id );
		msg->getUUIDFast(  _PREHASH_PropertiesData,	_PREHASH_FLImageID,	fl_image_id );
		msg->getUUIDFast(_PREHASH_PropertiesData, _PREHASH_PartnerID, partner_id);
		msg->getStringFast(_PREHASH_PropertiesData, _PREHASH_AboutText,	DB_USER_ABOUT_BUF_SIZE,		about_text );
		msg->getStringFast(_PREHASH_PropertiesData, _PREHASH_FLAboutText,	DB_USER_FL_ABOUT_BUF_SIZE,		fl_about_text );
		msg->getStringFast(_PREHASH_PropertiesData, _PREHASH_BornOn, DB_BORN_BUF_SIZE, born_on);
		msg->getString("PropertiesData","ProfileURL", DB_USER_PROFILE_URL_BUF_SIZE, profile_url);
		msg->getU32Fast(_PREHASH_PropertiesData, _PREHASH_Flags, flags);
		
		identified = (flags & AVATAR_IDENTIFIED);
		transacted = (flags & AVATAR_TRANSACTED);
		allow_publish = (flags & AVATAR_ALLOW_PUBLISH);
		online = (flags & AVATAR_ONLINE);
		
		U8 caption_index = 0;
		LLString caption_text;
		charter_member_size = msg->getSize("PropertiesData", "CharterMember");
		if(1 == charter_member_size)
		{
			msg->getBinaryData("PropertiesData", "CharterMember", &caption_index, 1);
		}
		else if(1 < charter_member_size)
		{
			char caption[MAX_STRING];		/*Flawfinder: ignore*/
			msg->getString("PropertiesData", "CharterMember", MAX_STRING, caption);
			caption_text = caption;
		}
		

		if(caption_text.empty())
		{
			LLString::format_map_t args;
			caption_text = self->mPanelSecondLife->childGetValue("CaptionTextAcctInfo").asString();
			
			const char* ACCT_TYPE[] = {
				"AcctTypeResident",
				"AcctTypeTrial",
				"AcctTypeCharterMember",
				"AcctTypeEmployee"
			};
			caption_index = llclamp(caption_index, (U8)0, (U8)(sizeof(ACCT_TYPE)/sizeof(ACCT_TYPE[0])-1));
			args["[ACCTTYPE]"] = self->mPanelSecondLife->childGetValue(ACCT_TYPE[caption_index]).asString();

			LLString payment_text = " ";
			const S32 DEFAULT_CAPTION_LINDEN_INDEX = 3;
			if(caption_index != DEFAULT_CAPTION_LINDEN_INDEX)
			{			
				if(transacted)
				{
					payment_text = "PaymentInfoUsed";
				}
				else if (identified)
				{
					payment_text = "PaymentInfoOnFile";
				}
				else
				{
					payment_text = "NoPaymentInfoOnFile";
				}
				args["[PAYMENTINFO]"] = self->mPanelSecondLife->childGetValue(payment_text).asString();
			}
			else
			{
				args["[PAYMENTINFO]"] = " ";
			}
			LLString::format(caption_text, args);
		}
		
		self->mPanelSecondLife->childSetValue("acct", caption_text);
		self->mPanelSecondLife->childSetValue("born", born_on);

		EOnlineStatus online_status = (online) ? ONLINE_STATUS_YES : ONLINE_STATUS_NO;

		self->setOnlineStatus(online_status);

		self->mPanelWeb->setWebURL(std::string(profile_url));

		LLTextureCtrl*	image_ctrl = LLUICtrlFactory::getTexturePickerByName(self->mPanelSecondLife,"img");
		if(image_ctrl)
		{
			image_ctrl->setImageAssetID(image_id);
		}
		self->childSetValue("about", about_text);

		self->mPanelSecondLife->setPartnerID(partner_id);
		self->mPanelSecondLife->updatePartnerName();

		if (self->mPanelFirstLife)
		{
			// Teens don't get these
			self->mPanelFirstLife->childSetValue("about", fl_about_text);
			LLTextureCtrl*	image_ctrl = LLUICtrlFactory::getTexturePickerByName(self->mPanelFirstLife,"img");
			if(image_ctrl)
			{
				image_ctrl->setImageAssetID(fl_image_id);
			}

			self->mPanelSecondLife->childSetValue("allow_publish", allow_publish);

		}
	}
}

// static
void LLPanelAvatar::processAvatarInterestsReply(LLMessageSystem *msg, void**)
{
	LLUUID	agent_id;	// your id
	LLUUID	avatar_id;	// target of this panel

	U32		want_to_mask;
	char	want_to_text[DB_USER_WANT_TO_BUF_SIZE];		/*Flawfinder: ignore*/
	U32		skills_mask;
	char	skills_text[DB_USER_SKILLS_BUF_SIZE];		/*Flawfinder: ignore*/
	char	languages_text[DB_USER_SKILLS_BUF_SIZE];		/*Flawfinder: ignore*/

	//llinfos << "properties packet size " << msg->getReceiveSize() << llendl;

	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AvatarID, avatar_id );

	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID != avatar_id)
		{
			continue;
		}

		msg->getU32Fast(   _PREHASH_PropertiesData,	_PREHASH_WantToMask,	want_to_mask );
		msg->getStringFast(_PREHASH_PropertiesData, _PREHASH_WantToText,	DB_USER_WANT_TO_BUF_SIZE,	want_to_text );
		msg->getU32Fast(   _PREHASH_PropertiesData,	_PREHASH_SkillsMask,	skills_mask );
		msg->getStringFast(_PREHASH_PropertiesData, _PREHASH_SkillsText,	DB_USER_SKILLS_BUF_SIZE,	skills_text );
		msg->getString(_PREHASH_PropertiesData, "LanguagesText",	DB_USER_SKILLS_BUF_SIZE,	languages_text );

		self->mPanelAdvanced->setWantSkills(want_to_mask, want_to_text, skills_mask, skills_text, languages_text);
	}
}

// Separate function because the groups list can be very long, almost
// filling a packet. JC
// static
void LLPanelAvatar::processAvatarGroupsReply(LLMessageSystem *msg, void**)
{
	LLUUID	agent_id;	// your id
	LLUUID	avatar_id;	// target of this panel
	U64		group_powers;
	char	group_title[DB_GROUP_TITLE_BUF_SIZE];		/*Flawfinder: ignore*/
	LLUUID	group_id;
	char	group_name[DB_GROUP_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
	LLUUID	group_insignia_id;

	llinfos << "groups packet size " << msg->getReceiveSize() << llendl;

	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AvatarID, avatar_id );

	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID != avatar_id)
		{
			continue;
		}
		
		LLScrollListCtrl*	group_list = LLUICtrlFactory::getScrollListByName(self->mPanelSecondLife,"groups"); 
// 		if(group_list)
// 		{
// 			group_list->deleteAllItems();
// 		}
		
		S32 group_count = msg->getNumberOfBlocksFast(_PREHASH_GroupData);
		if (0 == group_count)
		{
			if(group_list) group_list->addSimpleItem("None");
		}
		else
		{
			for(S32 i = 0; i < group_count; ++i)
			{
				msg->getU64(    _PREHASH_GroupData, "GroupPowers",	group_powers, i );
				msg->getStringFast(_PREHASH_GroupData, _PREHASH_GroupTitle,	DB_GROUP_TITLE_BUF_SIZE,	group_title, i );
				msg->getUUIDFast(  _PREHASH_GroupData, _PREHASH_GroupID,		group_id, i);
				msg->getStringFast(_PREHASH_GroupData, _PREHASH_GroupName,	DB_GROUP_NAME_BUF_SIZE,		group_name, i );
				msg->getUUIDFast(  _PREHASH_GroupData, _PREHASH_GroupInsigniaID, group_insignia_id, i );

				LLString group_string;
				if (group_id.notNull())
				{
					group_string.assign("Member of ");
					group_string.append(group_name);
				}
				else
				{
					group_string.assign("");
				}

				// Is this really necessary?  Remove existing entry if it exists.
				// TODO: clear the whole list when a request for data is made
				if (group_list)
				{
					S32 index = group_list->getItemIndex(group_id);
					if ( index >= 0 )
					{
						group_list->deleteSingleItem(index);
					}
				}

				LLSD row;
				row["id"] = group_id;
				row["columns"][0]["value"] = group_string;
				row["columns"][0]["font"] = "SANSSERIF_SMALL";
				if (group_list)
				{
					group_list->addElement(row);
				}
			}
		}
		if(group_list) group_list->sortByColumn(0, TRUE);
	}
}

// Don't enable the OK button until you actually have the data.
// Otherwise you will write blanks back into the database.
void LLPanelAvatar::enableOKIfReady()
{
	if(mHaveProperties && childIsVisible("OK"))
	{
		childSetEnabled("OK", TRUE);
	}
	else
	{
		childSetEnabled("OK", FALSE);
	}
}

void LLPanelAvatar::sendAvatarPropertiesUpdate()
{
	llinfos << "Sending avatarinfo update" << llendl;
	BOOL allow_publish = FALSE;
	BOOL mature = FALSE;
	if (LLPanelAvatar::sAllowFirstLife)
	{
		allow_publish = childGetValue("allow_publish");
		//A profile should never be mature.
		mature = FALSE;
	}
	U32 want_to_mask = 0x0;
	U32 skills_mask = 0x0;
	std::string want_to_text;
	std::string skills_text;
	std::string languages_text;
	mPanelAdvanced->getWantSkills(&want_to_mask, want_to_text, &skills_mask, skills_text, languages_text);

	LLUUID first_life_image_id;
	LLString first_life_about_text;
	if (mPanelFirstLife)
	{
		first_life_about_text = mPanelFirstLife->childGetValue("about").asString();
		LLTextureCtrl*	image_ctrl = LLUICtrlFactory::getTexturePickerByName(mPanelFirstLife,"img");
		if(image_ctrl)
		{
			first_life_image_id = image_ctrl->getImageAssetID();
		}
	}

	LLString about_text = mPanelSecondLife->childGetValue("about").asString();

	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_AvatarPropertiesUpdate);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(	_PREHASH_AgentID,		gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_PropertiesData);
	
	LLTextureCtrl*	image_ctrl = LLUICtrlFactory::getTexturePickerByName(mPanelSecondLife,"img");
	if(image_ctrl)
	{
		msg->addUUIDFast(	_PREHASH_ImageID,	image_ctrl->getImageAssetID());
	}
	else
	{
		msg->addUUIDFast(	_PREHASH_ImageID,	LLUUID::null);
	}
//	msg->addUUIDFast(	_PREHASH_ImageID,		mPanelSecondLife->mimage_ctrl->getImageAssetID()	);
	msg->addUUIDFast(	_PREHASH_FLImageID,		first_life_image_id);
	msg->addStringFast(	_PREHASH_AboutText,		about_text);
	msg->addStringFast(	_PREHASH_FLAboutText,	first_life_about_text);

	msg->addBOOL("AllowPublish", allow_publish);
	msg->addBOOL("MaturePublish", mature);
	msg->addString("ProfileURL", mPanelWeb->childGetText("url_edit"));
	gAgent.sendReliableMessage();

	msg->newMessage("AvatarInterestsUpdate");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(	_PREHASH_AgentID,		gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_PropertiesData);
	msg->addU32Fast(	_PREHASH_WantToMask,	want_to_mask);
	msg->addStringFast(	_PREHASH_WantToText,	want_to_text);
	msg->addU32Fast(	_PREHASH_SkillsMask,	skills_mask);
	msg->addStringFast(	_PREHASH_SkillsText,	skills_text);
	msg->addString( "LanguagesText",			languages_text);
	gAgent.sendReliableMessage();
}

void LLPanelAvatar::selectTab(S32 tabnum)
{
	if(mTab)
	{
		mTab->selectTab(tabnum);
	}
}

void LLPanelAvatar::selectTabByName(std::string tab_name)
{
	if (mTab)
	{
		if (tab_name.empty())
		{
			mTab->selectFirstTab();
		}
		else
		{
			mTab->selectTabByName(tab_name);
		}
	}
}


void LLPanelAvatar::processAvatarNotesReply(LLMessageSystem *msg, void**)
{
	// extract the agent id
	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);

	LLUUID target_id;
	msg->getUUID("Data", "TargetID", target_id);

	// look up all panels which have this avatar
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID != target_id)
		{
			continue;
		}

		char text[DB_USER_NOTE_SIZE];		/*Flawfinder: ignore*/
		msg->getString("Data", "Notes", DB_USER_NOTE_SIZE, text);
		self->childSetValue("notes edit", text);
		self->childSetEnabled("notes edit", true);
		self->mHaveNotes = true;
		self->mLastNotes = text;
	}
}


void LLPanelAvatar::processAvatarClassifiedReply(LLMessageSystem *msg, void** userdata)
{
	LLUUID agent_id;
	LLUUID target_id;

	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_TargetID, target_id);

	// look up all panels which have this avatar target
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID != target_id)
		{
			continue;
		}

		self->mPanelClassified->processAvatarClassifiedReply(msg, userdata);
	}
}

void LLPanelAvatar::processAvatarPicksReply(LLMessageSystem *msg, void** userdata)
{
	LLUUID agent_id;
	LLUUID target_id;

	msg->getUUID("AgentData", "AgentID", agent_id);
	msg->getUUID("AgentData", "TargetID", target_id);

	// look up all panels which have this avatar target
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID != target_id)
		{
			continue;
		}

		self->mPanelPicks->processAvatarPicksReply(msg, userdata);
	}
}

// static
void LLPanelAvatar::onClickKick(void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;

	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect(left, top, left+400, top-300);

	gViewerWindow->alertXmlEditText("KickUser", LLString::format_map_t(),
									NULL, NULL,
									LLPanelAvatar::finishKick, self);
}

// static
void LLPanelAvatar::finishKick(S32 option, const LLString& text, void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;

	if (option == 0)
	{
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID,		gAgent.getID() );
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   self->mAvatarID );
		msg->addU32("KickFlags", KICK_FLAGS_DEFAULT );
		msg->addStringFast(_PREHASH_Reason,    text );
		gAgent.sendReliableMessage();
	}
}

// static
void LLPanelAvatar::onClickFreeze(void* userdata)
{
	gViewerWindow->alertXmlEditText("FreezeUser", LLString::format_map_t(),
									NULL, NULL,
									LLPanelAvatar::finishFreeze, userdata);
}

// static
void LLPanelAvatar::finishFreeze(S32 option, const LLString& text, void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;

	if (option == 0)
	{
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID,		gAgent.getID() );
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   self->mAvatarID );
		msg->addU32("KickFlags", KICK_FLAGS_FREEZE );
		msg->addStringFast(_PREHASH_Reason,    text );
		gAgent.sendReliableMessage();
	}
}

// static
void LLPanelAvatar::onClickUnfreeze(void* userdata)
{
	gViewerWindow->alertXmlEditText("UnFreezeUser", LLString::format_map_t(),
									NULL, NULL,
									LLPanelAvatar::finishUnfreeze, userdata);
}

// static
void LLPanelAvatar::finishUnfreeze(S32 option, const LLString& text, void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;

	if (option == 0)
	{
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID,		gAgent.getID() );
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   self->mAvatarID );
		msg->addU32("KickFlags", KICK_FLAGS_UNFREEZE );
		msg->addStringFast(_PREHASH_Reason,    text );
		gAgent.sendReliableMessage();
	}
}

// static
void LLPanelAvatar::onClickCSR(void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*)userdata;
	if (!self) return;
	
	LLNameEditor* name_edit = LLViewerUICtrlFactory::getNameEditorByName(self, "name");
	if (!name_edit) return;

	LLString name = name_edit->getText();
	if (name.empty()) return;
	
	LLString url = "http://csr.lindenlab.com/agent/";
	
	// slow and stupid, but it's late
	S32 len = name.length();
	for (S32 i = 0; i < len; i++)
	{
		if (name[i] == ' ')
		{
			url += "%20";
		}
		else
		{
			url += name[i];
		}
	}
	
	LLWeb::loadURL(url);
}


void*	LLPanelAvatar::createPanelAvatarSecondLife(void* data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelSecondLife = new LLPanelAvatarSecondLife("2nd Life",LLRect(),self);
	return self->mPanelSecondLife;
}

void*	LLPanelAvatar::createPanelAvatarWeb(void*	data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelWeb = new LLPanelAvatarWeb("Web",LLRect(),self);
	return self->mPanelWeb;
}

void*	LLPanelAvatar::createPanelAvatarInterests(void*	data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelAdvanced = new LLPanelAvatarAdvanced("Interests",LLRect(),self);
	return self->mPanelAdvanced;
}


void*	LLPanelAvatar::createPanelAvatarPicks(void*	data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelPicks = new LLPanelAvatarPicks("Picks",LLRect(),self);
	return self->mPanelPicks;
}

void*	LLPanelAvatar::createPanelAvatarClassified(void* data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelClassified = new LLPanelAvatarClassified("Classified",LLRect(),self);
	return self->mPanelClassified;
}

void*	LLPanelAvatar::createPanelAvatarFirstLife(void*	data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelFirstLife = new LLPanelAvatarFirstLife("1st Life", LLRect(), self);
	return self->mPanelFirstLife;
}

void*	LLPanelAvatar::createPanelAvatarNotes(void*	data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelNotes = new LLPanelAvatarNotes("My Notes",LLRect(),self);
	return self->mPanelNotes;
}
