/** 
 * @file llpanelgroup.h
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LL_LLPANELGROUP_H
#define LL_LLPANELGROUP_H

#include "llgroupmgr.h"
#include "llpanel.h"
#include "lltimer.h"
#include "llvoiceclient.h"

struct LLOfferInfo;

const S32 UPDATE_MEMBERS_PER_FRAME = 500;

// Forward declares
class LLPanelGroupTab;
class LLTabContainer;
class LLAgent;


class LLPanelGroup : public LLPanel,
					 public LLGroupMgrObserver,
					 public LLVoiceClientStatusObserver
{
public:
	LLPanelGroup();
	virtual ~LLPanelGroup();

	virtual BOOL postBuild();

	void setGroupID(const LLUUID& group_id);

	void draw();

	void onOpen(const LLSD& key);

	// Group manager observer trigger.
	virtual void changed(LLGroupChange gc);

	// Implements LLVoiceClientStatusObserver::onChange() to enable the call
	// button when voice is available
	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);

	void showNotice(const std::string& subject,
					const std::string& message,
					const bool& has_inventory,
					const std::string& inventory_name,
					LLOfferInfo* inventory_offer);

	void notifyObservers();

	bool apply();
	void refreshData();
	void callGroup();
	void chatGroup();

	virtual void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	static void refreshCreatedGroup(const LLUUID& group_id);

	static void showNotice(const std::string& subject,
						   const std::string& message,
						   const LLUUID& group_id,
						   const bool& has_inventory,
						   const std::string& inventory_name,
						   LLOfferInfo* inventory_offer);


	bool	notifyChildren		(const LLSD& info);
	bool	handleNotifyCallback(const LLSD&, const LLSD&);

protected:
	virtual void update(LLGroupChange gc);

	void onBtnCreate();
	void onBackBtnClick();
	void onBtnJoin();
	void onBtnCancel();

	static void onBtnApply(void*);
	static void onBtnRefresh(void*);
	static void onBtnGroupCallClicked(void*);
	static void onBtnGroupChatClicked(void*);

	void reposButton(const std::string& name);
	void reposButtons();
	

protected:
	bool	apply(LLPanelGroupTab* tab);
	bool	canClose();

	bool	mShowingNotifyDialog;

	LLTimer mRefreshTimer;

	BOOL mSkipRefresh;

	std::string mDefaultNeedsApplyMesg;
	std::string mWantApplyMesg;

	std::vector<LLPanelGroupTab* > mTabs;

	LLButton*		mButtonJoin;
	LLUICtrl*		mJoinText;

};

class LLPanelGroupTab : public LLPanel
{
public:
	LLPanelGroupTab();
	virtual ~LLPanelGroupTab();

	// Triggered when the tab becomes active.
	virtual void activate() { }
	
	// Triggered when the tab becomes inactive.
	virtual void deactivate() { }

	// Asks if something needs to be applied.
	// If returning true, this function should modify the message to the user.
	virtual bool needsApply(std::string& mesg) { return false; }

	// Asks if there is currently a modal dialog being shown.
	virtual BOOL hasModal() { return mHasModal; }

	// Request to apply current data.
	// If returning fail, this function should modify the message to the user.
	virtual bool apply(std::string& mesg) { return true; }

	// Request a cancel of changes
	virtual void cancel() { }

	// Triggered when group information changes in the group manager.
	virtual void update(LLGroupChange gc) { }

	// This just connects the help button callback.
	virtual BOOL postBuild();

	virtual BOOL isVisibleByAgent(LLAgent* agentp);

	virtual void setGroupID(const LLUUID& id) {mGroupID = id;};

	void notifyObservers() {};

	const LLUUID& getGroupID() const { return mGroupID;}

	virtual void setupCtrls	(LLPanel* parent) {};

protected:
	LLUUID	mGroupID;
	BOOL mAllowEdit;
	BOOL mHasModal;
};

#endif // LL_LLPANELGROUP_H
