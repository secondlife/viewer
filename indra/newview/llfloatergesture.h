/** 
 * @file llfloatergesture.h
 * @brief Read-only list of gestures from your inventory.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

/**
 * (Also has legacy gesture editor for testing.)
 */

#ifndef LL_LLFLOATERGESTURE_H
#define LL_LLFLOATERGESTURE_H
#include <vector> 

#include "llfloater.h"
#include "llinventoryobserver.h"

class LLScrollContainer;
class LLView;
class LLButton;
class LLLineEditor;
class LLComboBox;
class LLViewerGesture;
class LLGestureOptions;
class LLScrollListCtrl;
class LLFloaterGestureObserver;
class LLFloaterGestureInventoryObserver;
class LLMultiGesture;
class LLMenuGL;

class LLFloaterGesture
:	public LLFloater, LLInventoryFetchDescendentsObserver
{
	LOG_CLASS(LLFloaterGesture);
public:
	LLFloaterGesture(const LLSD& key);
	virtual ~LLFloaterGesture();

	virtual BOOL postBuild();
	virtual void done ();
	void refreshAll();
	/**
	 * @brief Add new scrolllistitem into gesture_list.
	 * @param  item_id inventory id of gesture
	 * @param  gesture can be NULL , if item was not loaded yet
	 */
	void addGesture(const LLUUID& item_id, LLMultiGesture* gesture, LLCtrlListInterface * list);

protected:
	// Reads from the gesture manager's list of active gestures
	// and puts them in this list.
	void buildGestureList();
	void playGesture(LLUUID item_id);
private:
	void addToCurrentOutFit();
	/**
	 * @brief  This method is using to collect selected items. 
	 * In some places gesture_list can be rebuilt by gestureObservers during  iterating data from LLScrollListCtrl::getAllSelected().
	 * Therefore we have to copy these items to avoid viewer crash.
	 * @see LLFloaterGesture::onActivateBtnClick
	 */
	void getSelectedIds(uuid_vec_t& ids);
	bool isActionEnabled(const LLSD& command);
	/**
	 * @brief Activation rules:
	 *  According to Gesture Spec:
	 *  1. If all selected gestures are active: set to inactive
	 *  2. If all selected gestures are inactive: set to active
	 *  3. If selected gestures are in a mixed state: set all to active
	 */
	void onActivateBtnClick();
	void onClickEdit();
	void onClickPlay();
	void onClickNew();
	void onCommitList();
	void onCopyPasteAction(const LLSD& command);
	void onDeleteSelected();

	LLUUID mSelectedID;
	LLUUID mGestureFolderID;
	LLScrollListCtrl* mGestureList;

	LLFloaterGestureObserver* mObserver;
};


#endif
