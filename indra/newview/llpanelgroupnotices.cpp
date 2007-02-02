/** 
 * @file llpanelgroupnotices.cpp
 * @brief A panel to display group notices.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llpanelgroupnotices.h"

#include "llview.h"

#include "llinventory.h"
#include "llviewerinventory.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "llagent.h"
#include "lltooldraganddrop.h"

#include "lllineeditor.h"
#include "lltexteditor.h"
#include "llbutton.h"
#include "lliconctrl.h"
#include "llcheckboxctrl.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"

#include "roles_constants.h"
#include "llviewerwindow.h"
#include "llviewermessage.h"

/////////////////////////
// LLPanelGroupNotices //
/////////////////////////
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLDropTarget
//
// This handy class is a simple way to drop something on another
// view. It handles drop events, always setting itself to the size of
// its parent.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLGroupDropTarget : public LLView
{
public:
	LLGroupDropTarget(const std::string& name, const LLRect& rect, LLPanelGroupNotices* panel, const LLUUID& group_id);
	~LLGroupDropTarget() {};

	void doDrop(EDragAndDropType cargo_type, void* cargo_data);

	//
	// LLView functionality
	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   LLString& tooltip_msg);
protected:
	LLPanelGroupNotices* mGroupNoticesPanel;
	LLUUID	mGroupID;
};

LLGroupDropTarget::LLGroupDropTarget(const std::string& name, const LLRect& rect,
						   LLPanelGroupNotices* panel, const LLUUID& group_id) :
	LLView(name, rect, NOT_MOUSE_OPAQUE, FOLLOWS_ALL),
	mGroupNoticesPanel(panel),
	mGroupID(group_id)
{
}

EWidgetType LLGroupDropTarget::getWidgetType() const
{
	return WIDGET_TYPE_DONTCARE;
}

LLString LLGroupDropTarget::getWidgetTag() const
{
	return LL_GROUP_DROP_TARGET_TAG;
}

void LLGroupDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	llinfos << "LLGroupDropTarget::doDrop()" << llendl;
}

BOOL LLGroupDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 LLString& tooltip_msg)
{
	BOOL handled = FALSE;

	if (!gAgent.hasPowerInGroup(mGroupID,GP_NOTICES_SEND))
	{
		*accept = ACCEPT_NO;
		return TRUE;
	}

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
				&& LLToolDragAndDrop::isInventoryGroupGiveAcceptable(inv_item))
			{
				// *TODO: get multiple object transfers working
				*accept = ACCEPT_YES_COPY_SINGLE;
				if(drop)
				{
					mGroupNoticesPanel->setItem(inv_item);
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
		case DAD_CALLINGCARD:
		default:
			*accept = ACCEPT_NO;
			break;
		}
	}
	return handled;
}

//-----------------------------------------------------------------------------
// LLPanelGroupNotices
//-----------------------------------------------------------------------------
char* build_notice_date(const time_t& the_time, char* buffer)
{
	time_t t = the_time;
	if (!t) time(&t);
	tm* lt = localtime(&t);
	//for some reason, the month is off by 1.  See other uses of
	//"local" time in the code...
	snprintf(buffer, sizeof(buffer), "%i/%i/%i", lt->tm_mon + 1, lt->tm_mday, lt->tm_year + 1900);		/*Flawfinder: ignore*/

	return buffer;
}

LLPanelGroupNotices::LLPanelGroupNotices(const std::string& name,
									const LLUUID& group_id) :
	LLPanelGroupTab(name,group_id),
	mInventoryItem(NULL),
	mInventoryOffer(NULL)
{
	sInstances[group_id] = this;
}

LLPanelGroupNotices::~LLPanelGroupNotices()
{
	sInstances.erase(mGroupID);

	if (mInventoryOffer)
	{
		// Cancel the inventory offer.
		inventory_offer_callback( 1 , mInventoryOffer); 
		mInventoryOffer = NULL;
	}
}

// static
void* LLPanelGroupNotices::createTab(void* data)
{
	LLUUID* group_id = static_cast<LLUUID*>(data);
	return new LLPanelGroupNotices("panel group notices", *group_id);
}

BOOL LLPanelGroupNotices::isVisibleByAgent(LLAgent* agentp)
{
	return mAllowEdit &&
		agentp->hasPowerInGroup(mGroupID, GP_NOTICES_SEND | GP_NOTICES_RECEIVE);
}

BOOL LLPanelGroupNotices::postBuild()
{
	bool recurse = true;

	mNoticesList = (LLScrollListCtrl*)getChildByName("notice_list",recurse);
	mNoticesList->setCommitOnSelectionChange(TRUE);
	mNoticesList->setCommitCallback(onSelectNotice);
	mNoticesList->setCallbackUserData(this);

	mBtnNewMessage = (LLButton*)getChildByName("create_new_notice",recurse);
	mBtnNewMessage->setClickedCallback(onClickNewMessage);
	mBtnNewMessage->setCallbackUserData(this);
	mBtnNewMessage->setEnabled(gAgent.hasPowerInGroup(mGroupID, GP_NOTICES_SEND));

	mBtnGetPastNotices = (LLButton*)getChildByName("refresh_notices",recurse);
	mBtnGetPastNotices->setClickedCallback(onClickRefreshNotices);
	mBtnGetPastNotices->setCallbackUserData(this);

	// Create
	mCreateSubject = (LLLineEditor*)getChildByName("create_subject",recurse);
	mCreateMessage = (LLTextEditor*)getChildByName("create_message",recurse);

	mCreateInventoryName =  (LLLineEditor*)getChildByName("create_inventory_name",recurse);
	mCreateInventoryName->setTabStop(FALSE);
	mCreateInventoryName->setEnabled(FALSE);

	mCreateInventoryIcon = (LLIconCtrl*)getChildByName("create_inv_icon",recurse);
	mCreateInventoryIcon->setVisible(FALSE);

	mBtnSendMessage = (LLButton*)getChildByName("send_notice",recurse);
	mBtnSendMessage->setClickedCallback(onClickSendMessage);
	mBtnSendMessage->setCallbackUserData(this);

	mBtnRemoveAttachment = (LLButton*)getChildByName("remove_attachment",recurse);
	mBtnRemoveAttachment->setClickedCallback(onClickRemoveAttachment);
	mBtnRemoveAttachment->setCallbackUserData(this);
	mBtnRemoveAttachment->setEnabled(FALSE);

	// View
	mViewSubject = (LLLineEditor*)getChildByName("view_subject",recurse);
	mViewMessage = (LLTextEditor*)getChildByName("view_message",recurse);

	mViewInventoryName =  (LLLineEditor*)getChildByName("view_inventory_name",recurse);
	mViewInventoryName->setTabStop(FALSE);
	mViewInventoryName->setEnabled(FALSE);

	mViewInventoryIcon = (LLIconCtrl*)getChildByName("view_inv_icon",recurse);
	mViewInventoryIcon->setVisible(FALSE);

	mBtnOpenAttachment = (LLButton*)getChildByName("open_attachment",recurse);
	mBtnOpenAttachment->setClickedCallback(onClickOpenAttachment);
	mBtnOpenAttachment->setCallbackUserData(this);

	LLTextBox *txt = (LLTextBox*) getChildByName("no_notices_text",false);
	if (txt)
	{
		mNoNoticesStr = txt->getText();
		removeChild(txt);
	}

	mPanelCreateNotice = (LLPanel*) getChildByName("panel_create_new_notice",recurse);
	mPanelViewNotice = (LLPanel*) getChildByName("panel_view_past_notice",recurse);

	// Must be in front of all other UI elements.
	LLPanel* dtv = (LLPanel*)getChildByName("drop_target",recurse);
	LLGroupDropTarget* target = new LLGroupDropTarget("drop_target",
											dtv->getRect(),
											this, mGroupID);
	target->setEnabled(TRUE);
	target->setToolTip(dtv->getToolTip());

	mPanelCreateNotice->addChild(target);
	mPanelCreateNotice->removeChild(dtv);

	arrangeNoticeView(VIEW_PAST_NOTICE);

	return LLPanelGroupTab::postBuild();
}

void LLPanelGroupNotices::activate()
{
	BOOL can_send = gAgent.hasPowerInGroup(mGroupID,GP_NOTICES_SEND);
	BOOL can_receive = gAgent.hasPowerInGroup(mGroupID,GP_NOTICES_RECEIVE);

	mPanelViewNotice->setEnabled(can_receive);
	mPanelCreateNotice->setEnabled(can_send);

	// Always disabled to stop direct editing of attachment names
	mCreateInventoryName->setEnabled(FALSE);
	mViewInventoryName->setEnabled(FALSE);

	// If we can receive notices, grab them right away.
	if (can_receive)
	{
		onClickRefreshNotices(this);
	}
}

void LLPanelGroupNotices::setItem(LLPointer<LLInventoryItem> inv_item)
{
	mInventoryItem = inv_item;

	LLViewerImage* item_icon = get_item_icon(inv_item->getType(),
										inv_item->getInventoryType(),
										inv_item->getFlags());

	mCreateInventoryIcon->setImage(item_icon->getID());
	mCreateInventoryIcon->setVisible(TRUE);

	std::stringstream ss;
	ss << "        " << mInventoryItem->getName();

	mCreateInventoryName->setText(ss.str());
	mBtnRemoveAttachment->setEnabled(TRUE);
}

void LLPanelGroupNotices::onClickRemoveAttachment(void* data)
{
	LLPanelGroupNotices* self = (LLPanelGroupNotices*)data;
	self->mInventoryItem = NULL;
	self->mCreateInventoryName->clear();
	self->mCreateInventoryIcon->setVisible(FALSE);
	self->mBtnRemoveAttachment->setEnabled(FALSE);
}

//static 
void LLPanelGroupNotices::onClickOpenAttachment(void* data)
{
	LLPanelGroupNotices* self = (LLPanelGroupNotices*)data;

	inventory_offer_callback( 0 , self->mInventoryOffer);
	self->mInventoryOffer = NULL;
	self->mBtnOpenAttachment->setEnabled(FALSE);
}

void LLPanelGroupNotices::onClickSendMessage(void* data)
{
	LLPanelGroupNotices* self = (LLPanelGroupNotices*)data;
	
	if (self->mCreateSubject->getText().empty())
	{
		// Must supply a subject
		gViewerWindow->alertXml("MustSpecifyGroupNoticeSubject");
		return;
	}
	send_group_notice(
			self->mGroupID,
			self->mCreateSubject->getText().c_str(),
			self->mCreateMessage->getText().c_str(),
			self->mInventoryItem);

	self->mCreateMessage->clear();
	self->mCreateSubject->clear();
	onClickRemoveAttachment(data);

	self->arrangeNoticeView(VIEW_PAST_NOTICE);
	onClickRefreshNotices(self);

}

//static 
void LLPanelGroupNotices::onClickNewMessage(void* data)
{
	LLPanelGroupNotices* self = (LLPanelGroupNotices*)data;

	self->arrangeNoticeView(CREATE_NEW_NOTICE);

	if (self->mInventoryOffer)
	{
		inventory_offer_callback( 1 , self->mInventoryOffer);
		self->mInventoryOffer = NULL;
	}

	self->mCreateSubject->clear();
	self->mCreateMessage->clear();
	if (self->mInventoryItem) onClickRemoveAttachment(self);
	self->mNoticesList->deselectAllItems(TRUE); // TRUE == don't commit on chnage
}

void LLPanelGroupNotices::onClickRefreshNotices(void* data)
{
	lldebugs << "LLPanelGroupNotices::onClickGetPastNotices" << llendl;
	LLPanelGroupNotices* self = (LLPanelGroupNotices*)data;
	
	self->mNoticesList->deleteAllItems();

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GroupNoticesListRequest");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID",gAgent.getID());
	msg->addUUID("SessionID",gAgent.getSessionID());
	msg->nextBlock("Data");
	msg->addUUID("GroupID",self->mGroupID);
	gAgent.sendReliableMessage();
}

//static
std::map<LLUUID,LLPanelGroupNotices*> LLPanelGroupNotices::sInstances;

// static
void LLPanelGroupNotices::processGroupNoticesListReply(LLMessageSystem* msg, void** data)
{
	LLUUID group_id;
	msg->getUUID("AgentData", "GroupID", group_id);

	std::map<LLUUID,LLPanelGroupNotices*>::iterator it = sInstances.find(group_id);
	if (it == sInstances.end())
	{
		llinfos << "Group Panel Notices " << group_id << " no longer in existence."
				<< llendl;
		return;
	}
	
	LLPanelGroupNotices* selfp = it->second;
	if(!selfp)
	{
		llinfos << "Group Panel Notices " << group_id << " no longer in existence."
				<< llendl;
		return;
	}

	selfp->processNotices(msg);
}

void LLPanelGroupNotices::processNotices(LLMessageSystem* msg)
{
	LLUUID id;
	char subj[MAX_STRING];		/*Flawfinder: ignore*/
	char name[MAX_STRING];		/*Flawfinder: ignore*/
	U32 timestamp;
	BOOL has_attachment;
	U8 asset_type;

	S32 i=0;
	S32 count = msg->getNumberOfBlocks("Data");
	for (;i<count;++i)
	{
		msg->getUUID("Data","NoticeID",id,i);
		if (1 == count && id.isNull())
		{
			// Only one entry, the dummy entry.
			mNoticesList->addSimpleItem(mNoNoticesStr,ADD_BOTTOM,FALSE);
			mNoticesList->setEnabled(FALSE);
			return;
		}
			
		msg->getString("Data","Subject",MAX_STRING,subj,i);
		msg->getString("Data","FromName",MAX_STRING,name,i);
		msg->getBOOL("Data","HasAttachment",has_attachment,i);
		msg->getU8("Data","AssetType",asset_type,i);
		msg->getU32("Data","Timestamp",timestamp,i);
		time_t t = timestamp;

		LLSD row;
		row["id"] = id;
		
		row["columns"][0]["column"] = "icon";
		if (has_attachment)
		{
			LLUUID icon_id = get_item_icon_uuid(
						(LLAssetType::EType)asset_type,
						LLInventoryType::IT_NONE,FALSE);
			row["columns"][0]["type"] = "icon";
			row["columns"][0]["value"] = icon_id;
		}

		row["columns"][1]["column"] = "subject";
		row["columns"][1]["value"] = subj;

		row["columns"][2]["column"] = "from";
		row["columns"][2]["value"] = name;

		char buffer[30];		/*Flawfinder: ignore*/
		build_notice_date(t, buffer);
		row["columns"][3]["column"] = "date";
		row["columns"][3]["value"] = buffer;

		snprintf(buffer, 30, "%u", timestamp);		/*Flawfinder: ignore*/
		row["columns"][4]["column"] = "sort";
		row["columns"][4]["value"] = buffer;

		mNoticesList->addElement(row, ADD_SORTED);
	}
}

void LLPanelGroupNotices::onSelectNotice(LLUICtrl* ctrl, void* data)
{
	LLPanelGroupNotices* self = (LLPanelGroupNotices*)data;

	if(!self) return;
	LLScrollListItem* item = self->mNoticesList->getFirstSelected();
	if (!item) return;
	
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GroupNoticeRequest");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID",gAgent.getID());
	msg->addUUID("SessionID",gAgent.getSessionID());
	msg->nextBlock("Data");
	msg->addUUID("GroupNoticeID",item->getUUID());
	gAgent.sendReliableMessage();

	lldebugs << "Item " << item->getUUID().getString().c_str() << " selected." << llendl;
}

void LLPanelGroupNotices::showNotice(const char* subject,
										const char* message,
										const bool& has_inventory,
										const char* inventory_name,
										LLOfferInfo* inventory_offer)
{
	arrangeNoticeView(VIEW_PAST_NOTICE);

	if(mViewSubject) mViewSubject->setText(subject);
	if(mViewMessage) mViewMessage->setText(message);
	
	if (mInventoryOffer)
	{
		// Cancel the inventory offer for the previously viewed notice
		inventory_offer_callback( 1 , mInventoryOffer); 
		mInventoryOffer = NULL;
	}

	if (inventory_offer)
	{
		mInventoryOffer = inventory_offer;

		LLViewerImage* item_icon = get_item_icon(mInventoryOffer->mType,
												LLInventoryType::IT_TEXTURE,
												0);

		mViewInventoryIcon->setImage(item_icon->getID());
		mViewInventoryIcon->setVisible(TRUE);

		std::stringstream ss;
		ss << "        " << inventory_name;

		mViewInventoryName->setText(ss.str());
		mBtnOpenAttachment->setEnabled(TRUE);
	}
	else
	{
		mViewInventoryName->clear();
		mViewInventoryIcon->setVisible(FALSE);
		mBtnOpenAttachment->setEnabled(FALSE);
	}
}

void LLPanelGroupNotices::arrangeNoticeView(ENoticeView view_type)
{
	if (CREATE_NEW_NOTICE == view_type)
	{
        mPanelCreateNotice->setVisible(TRUE);
		mPanelViewNotice->setVisible(FALSE);
	}
	else
	{
		mPanelCreateNotice->setVisible(FALSE);
		mPanelViewNotice->setVisible(TRUE);
		mBtnOpenAttachment->setEnabled(FALSE);
	}
}
