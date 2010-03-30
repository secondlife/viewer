/** 
 * @file llpanelteleporthistory.h
 * @brief Teleport history represented by a scrolling list
 * class definition
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLPANELTELEPORTHISTORY_H
#define LL_LLPANELTELEPORTHISTORY_H

#include "lluictrlfactory.h"

#include "llpanelplacestab.h"
#include "llteleporthistory.h"
#include "llmenugl.h"

class LLTeleportHistoryStorage;
class LLAccordionCtrl;
class LLAccordionCtrlTab;
class LLFlatListView;

class LLTeleportHistoryPanel : public LLPanelPlacesTab
{
public:
	class ContextMenu
	{
	public:
		ContextMenu();
		void show(LLView* spawning_view, S32 index, S32 x, S32 y);

	private:
		LLContextMenu* createMenu();
		void onTeleport();
		void onInfo();
		void onCopyToClipboard();

		static void gotSLURLCallback(const std::string& slurl);

		LLContextMenu* mMenu;
		S32 mIndex;
	};

	LLTeleportHistoryPanel();
	virtual ~LLTeleportHistoryPanel();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();

	/*virtual*/ void onSearchEdit(const std::string& string);
	/*virtual*/ void onShowOnMap();
	/*virtual*/ void onTeleport();
	///*virtual*/ void onCopySLURL();
	/*virtual*/ void updateVerbs();

private:

	void onDoubleClickItem();
	void onReturnKeyPressed();
	void onAccordionTabRightClick(LLView *view, S32 x, S32 y, MASK mask);
	void onAccordionTabOpen(LLAccordionCtrlTab *tab);
	void onAccordionTabClose(LLAccordionCtrlTab *tab);
	void onExpandAllFolders();
	void onCollapseAllFolders();
	void onClearTeleportHistory();
	bool onClearTeleportHistoryDialog(const LLSD& notification, const LLSD& response);

	void refresh();
	void getNextTab(const LLDate& item_date, S32& curr_tab, LLDate& tab_date);
	void onTeleportHistoryChange(S32 removed_index);
	void replaceItem(S32 removed_index);
	void showTeleportHistory();
	void handleItemSelect(LLFlatListView* );
	LLFlatListView* getFlatListViewFromTab(LLAccordionCtrlTab *);
	void onGearButtonClicked();
	bool isActionEnabled(const LLSD& userdata) const;

	void setAccordionCollapsedByUser(LLUICtrl* acc_tab, bool collapsed);
	bool isAccordionCollapsedByUser(LLUICtrl* acc_tab);
	void onAccordionExpand(LLUICtrl* ctrl, const LLSD& param);

	static void confirmTeleport(S32 hist_idx);
	static bool onTeleportConfirmation(const LLSD& notification, const LLSD& response, S32 hist_idx);

	LLTeleportHistoryStorage*	mTeleportHistory;
	LLAccordionCtrl*		mHistoryAccordion;

	LLFlatListView*			mLastSelectedFlatlList;
	S32				mLastSelectedItemIndex;
	bool				mDirty;
	S32				mCurrentItem;

	typedef LLDynamicArray<LLAccordionCtrlTab*> item_containers_t;
	item_containers_t mItemContainers;

	ContextMenu mContextMenu;
	LLContextMenu*			mAccordionTabMenu;
	LLHandle<LLView>		mGearMenuHandle;
};

#endif //LL_LLPANELTELEPORTHISTORY_H
