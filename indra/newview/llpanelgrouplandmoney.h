/** 
 * @file llpanelgrouplandmoney.h
 * @brief Panel for group land and money.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_PANEL_GROUP_LAND_MONEY_H
#define LL_PANEL_GROUP_LAND_MONEY_H

#include "llpanelgroup.h"
#include "llmap.h"
#include "lluuid.h"

#include "llbutton.h"
#include "lltexteditor.h"
#include "llpanel.h"

class LLPanelGroupLandMoney : public LLPanelGroupTab
{
public:
	LLPanelGroupLandMoney(const std::string& name, const LLUUID& group_id);
	virtual ~LLPanelGroupLandMoney();
	virtual BOOL postBuild();
	virtual BOOL isVisibleByAgent(LLAgent* agentp);

	static void* createTab(void* data);

	virtual void activate();
	virtual bool needsApply(LLString& mesg);
	virtual bool apply(LLString& mesg);
	virtual void cancel();
	virtual void update(LLGroupChange gc);

	static void processPlacesReply(LLMessageSystem* msg, void**);

	static LLMap<LLUUID, LLPanelGroupLandMoney*> sGroupIDs;
protected:
	class impl;
	impl* mImplementationp;
};

class LLGroupMoneyTabEventHandler
{
public:
	LLGroupMoneyTabEventHandler(LLButton* earlier_button,
								LLButton* later_button,
								LLTextEditor* text_editor,
								LLTabContainerCommon* tab_containerp,
								LLPanel* panelp,
								const LLUUID& group_id,
								S32 interval_length_days,
								S32 max_interval_days);
	virtual ~LLGroupMoneyTabEventHandler();

	virtual void requestData(LLMessageSystem* msg);
	virtual void processReply(LLMessageSystem* msg, void** data);

	virtual void onClickEarlier();
	virtual void onClickLater();
	virtual void onClickTab();

	static void clickEarlierCallback(void* data);
	static void clickLaterCallback(void* data);
	static void clickTabCallback(void* user_data, bool from_click);

	static LLMap<LLUUID, LLGroupMoneyTabEventHandler*> sInstanceIDs;
	static std::map<LLPanel*, LLGroupMoneyTabEventHandler*> sTabsToHandlers;
protected:
	class impl;
	impl* mImplementationp;
};

class LLGroupMoneyDetailsTabEventHandler : public LLGroupMoneyTabEventHandler
{
public:
	LLGroupMoneyDetailsTabEventHandler(LLButton* earlier_buttonp,
									   LLButton* later_buttonp,
									   LLTextEditor* text_editorp,
									   LLTabContainerCommon* tab_containerp,
									   LLPanel* panelp,
									   const LLUUID& group_id);
	virtual ~LLGroupMoneyDetailsTabEventHandler();

	virtual void requestData(LLMessageSystem* msg);
	virtual void processReply(LLMessageSystem* msg, void** data);

	static void processGroupAccountDetailsReply(LLMessageSystem* msg, 
												void** data);
};

class LLGroupMoneySalesTabEventHandler : public LLGroupMoneyTabEventHandler
{
public:
	LLGroupMoneySalesTabEventHandler(LLButton* earlier_buttonp,
									 LLButton* later_buttonp,
									 LLTextEditor* text_editorp,
									 LLTabContainerCommon* tab_containerp,
									 LLPanel* panelp,
									 const LLUUID& group_id);
	virtual ~LLGroupMoneySalesTabEventHandler();

	virtual void requestData(LLMessageSystem* msg);
	virtual void processReply(LLMessageSystem* msg, void** data);

	static void processGroupAccountTransactionsReply(LLMessageSystem* msg, 
													 void** data);
};

class LLGroupMoneyPlanningTabEventHandler : public LLGroupMoneyTabEventHandler
{
public:
	LLGroupMoneyPlanningTabEventHandler(LLTextEditor* text_editor,
										LLTabContainerCommon* tab_containerp,
										LLPanel* panelp,
										const LLUUID& group_id);
	virtual ~LLGroupMoneyPlanningTabEventHandler();

	virtual void requestData(LLMessageSystem* msg);
	virtual void processReply(LLMessageSystem* msg, void** data);

	static void processGroupAccountSummaryReply(LLMessageSystem* msg,
												void** data);
};

#endif // LL_PANEL_GROUP_LAND_MONEY_H
