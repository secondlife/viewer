/** 
 * @file llsplitbutton.h
 * @brief LLSplitButton base class
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

// A control that displays the name of the chosen item, which when clicked
// shows a scrolling box of choices.


#include "llbutton.h"
#include "llpanel.h"
#include "lluictrl.h"


#ifndef LL_LLSPLITBUTTON_H
#define LL_LLSPLITBUTTON_H

class LLSplitButton
	:	public LLUICtrl
{
public:
	typedef enum e_arrow_position
	{
		LEFT,
		RIGHT
	} EArrowPosition;

	struct ArrowPositionValues : public LLInitParam::TypeValuesHelper<EArrowPosition, ArrowPositionValues>
	{
		static void declareValues();
	};

	struct ItemParams : public LLInitParam::Block<ItemParams, LLButton::Params>
	{
		ItemParams();
	};

	struct Params :	public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<EArrowPosition, ArrowPositionValues> arrow_position;
		Optional<LLButton::Params> arrow_button;
		Optional<LLPanel::Params> items_panel;
		Multiple<ItemParams> items;

		Params();
	};


	virtual ~LLSplitButton() {};

	//Overridden
	virtual void	onFocusLost();
	virtual void	setFocus(BOOL b);
	virtual void	setEnabled(BOOL enabled);

	//Callbacks
	void	onArrowBtnDown();
	void	onHeldDownShownButton();
	void	onItemSelected(LLUICtrl* ctrl);
	void	setSelectionCallback(commit_callback_t cb) { mSelectionCallback = cb; }

	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);

	virtual void	showButtons();
	virtual void	hideButtons();


protected:
	friend class LLUICtrlFactory;
	LLSplitButton(const LLSplitButton::Params& p);

	LLButton* prepareItemButton(LLButton::Params params);
	LLPanel* prepareItemsPanel(LLPanel::Params params, S32 items_count);

	LLPanel* mItemsPanel;
	std::list<LLButton*> mHidenItems;
	LLButton* mArrowBtn;
	LLButton* mShownItem;
	EArrowPosition mArrowPosition;

	commit_callback_t mSelectionCallback;
};


#endif
