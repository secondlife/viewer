/** 
 * @file llpanelgroupnotices.h
 * @brief A panel to display group notices.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPANELGROUPNOTICES_H
#define LL_LLPANELGROUPNOTICES_H

#include "llpanelgroup.h"
#include "llmemory.h"
#include "llinventory.h"

class LLLineEditor;
class LLTextEditor;
class LLButton;
class LLIconCtrl;
class LLCheckBoxCtrl;
class LLScrollListCtrl;

class LLPanelGroupNotices : public LLPanelGroupTab
{
public:
	LLPanelGroupNotices(const std::string& name, const LLUUID& group_id);
	virtual ~LLPanelGroupNotices();

	// LLPanelGroupTab
	static void* createTab(void* data);
	virtual void activate();
	//virtual bool needsApply(LLString& mesg);
	//virtual bool apply(LLString& mesg);
	//virtual void update();
	
	virtual BOOL postBuild();
	virtual BOOL isVisibleByAgent(LLAgent* agentp);

	void setItem(LLPointer<LLInventoryItem> inv_item);

	static void processGroupNoticesListReply(LLMessageSystem* msg, void** data);

	void showNotice(const char* subject,
							const char* message,
							const bool& has_inventory,
							const char* inventory_name,
							LLOfferInfo* inventory_offer);

private:
	static void onClickRemoveAttachment(void* data);
	static void onClickOpenAttachment(void* data);
	static void onClickSendMessage(void* data);
	static void onClickNewMessage(void* data);
	static void onClickRefreshNotices(void* data);

	void processNotices(LLMessageSystem* msg);
	static void onSelectNotice(LLUICtrl* ctrl, void* data);

	enum ENoticeView
	{
		VIEW_PAST_NOTICE,
		CREATE_NEW_NOTICE
	};

	void arrangeNoticeView(ENoticeView view_type);

	LLPointer<LLInventoryItem>	mInventoryItem;
	
	LLLineEditor		*mCreateSubject;
    LLLineEditor		*mCreateInventoryName;
	LLTextEditor		*mCreateMessage;

	LLLineEditor		*mViewSubject;
    LLLineEditor		*mViewInventoryName;
	LLTextEditor		*mViewMessage;
	
	LLButton			*mBtnSendMessage;
	LLButton			*mBtnNewMessage;
	LLButton			*mBtnRemoveAttachment;
	LLButton			*mBtnOpenAttachment;
	LLButton			*mBtnGetPastNotices;

	LLPanel				*mPanelCreateNotice;
	LLPanel				*mPanelViewNotice;

	LLIconCtrl		 *mCreateInventoryIcon;
	LLIconCtrl		 *mViewInventoryIcon;
	
	LLScrollListCtrl *mNoticesList;

	std::string		mNoNoticesStr;

	LLOfferInfo* mInventoryOffer;

	static std::map<LLUUID,LLPanelGroupNotices*>	sInstances;
};

#endif
