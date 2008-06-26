/** 
 * @file llpanelgroupnotices.h
 * @brief A panel to display group notices.
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
	//virtual bool needsApply(std::string& mesg);
	//virtual bool apply(std::string& mesg);
	//virtual void update();
	
	virtual BOOL postBuild();
	virtual BOOL isVisibleByAgent(LLAgent* agentp);

	void setItem(LLPointer<LLInventoryItem> inv_item);

	static void processGroupNoticesListReply(LLMessageSystem* msg, void** data);

	void showNotice(const std::string& subject,
					const std::string& message,
					const bool& has_inventory,
					const std::string& inventory_name,
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
