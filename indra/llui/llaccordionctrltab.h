/** 
 * @file LLAccordionCtrlTab.h
 * @brief Collapsible box control implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_ACCORDIONCTRLTAB_H_
#define LL_ACCORDIONCTRLTAB_H_

#include <string>
#include "llrect.h"
#include "lluictrl.h"
#include "lluicolor.h"

class LLUICtrlFactory;
class LLUIImage;
class LLButton;
class LLTextBox;
class LLScrollbar;



// LLAccordionCtrlTab is a container for other controls. 
// It has a Header, by clicking on which hosted controls are shown or hidden.
// When hosted controls are show - LLAccordionCtrlTab is expanded.
// When hosted controls are hidden - LLAccordionCtrlTab is collapsed.

class LLAccordionCtrlTab : public LLUICtrl
{
// Interface
public:

	struct Params 
	 : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<bool>			display_children, //expanded or collapsed after initialization
								collapsible;

		Optional<std::string>	title;

		Optional<S32>			header_height,
								min_width,
								min_height;

		// Overlay images (arrows on the left)
		Mandatory<LLUIImage*>	header_expand_img,
								header_expand_img_pressed,
								header_collapse_img,
								header_collapse_img_pressed;

		// Background images for the accordion tabs
		Mandatory<LLUIImage*>	header_image,
								header_image_over,
								header_image_pressed,
								header_image_focused;

		Optional<LLUIColor>		header_bg_color,
								header_text_color,
								dropdown_bg_color;

		Optional<bool>			header_visible;

		Optional<bool>			fit_panel;

		Optional<S32>			padding_left;
		Optional<S32>			padding_right;
		Optional<S32>			padding_top;
		Optional<S32>			padding_bottom;

		Params();
	};

	typedef LLDefaultChildRegistry child_registry_t;

	virtual ~LLAccordionCtrlTab();
	
	// Registers callback for expand/collapse events.
	boost::signals2::connection setDropDownStateChangedCallback(commit_callback_t cb);

	// Changes expand/collapse state
	virtual void setDisplayChildren(bool display);

	// Returns expand/collapse state
	virtual bool getDisplayChildren() const {return mDisplayChildren;};

	//set LLAccordionCtrlTab panel
	void		setAccordionView(LLView* panel);
	LLView*		getAccordionView() { return mContainerPanel; };

	// Set text in LLAccordionCtrlTabHeader
	void setTitle(const std::string& title);

	boost::signals2::connection setFocusReceivedCallback(const focus_signal_t::slot_type& cb);
	boost::signals2::connection setFocusLostCallback(const focus_signal_t::slot_type& cb);

	bool getCollapsible() {return mCollapsible;};

	void setCollapsible(bool collapsible) {mCollapsible = collapsible;};
	void changeOpenClose(bool is_open);

	void canOpenClose(bool can_open_close) { mCanOpenClose = can_open_close;};
	bool canOpenClose() const { return mCanOpenClose; };

	virtual BOOL postBuild();

	S32	notifyParent(const LLSD& info);
	S32 notify(const LLSD& info);
	bool notifyChildren(const LLSD& info);

	void draw();

	void    storeOpenCloseState		();
	void    restoreOpenCloseState	();

protected:
	LLAccordionCtrlTab(const LLAccordionCtrlTab::Params&);
	friend class LLUICtrlFactory;

// Overrides
public:

	// Call reshape after changing size
	virtual void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	// Changes expand/collapse state and triggers expand/collapse callbacks
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);

	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);

	virtual BOOL handleToolTip(S32 x, S32 y, MASK mask);

	virtual bool addChild(LLView* child, S32 tab_group);

	bool isExpanded() { return mDisplayChildren; }

	S32 getHeaderHeight();

	// Min size functions

	void setHeaderVisible(bool value);

	bool getHeaderVisible() { return mHeaderVisible;}

	S32 mExpandedHeight; // Height of expanded ctrl.
						 // Used to restore height after expand.

	S32	getPaddingLeft() const { return mPaddingLeft;}
	S32	getPaddingRight() const { return mPaddingRight;}
	S32	getPaddingTop() const { return mPaddingTop;}
	S32	getPaddingBottom() const { return mPaddingBottom;}

	void showAndFocusHeader();

	void setFitPanel( bool fit ) { mFitPanel = true; }

protected:
	void adjustContainerPanel	(const LLRect& child_rect);
	void adjustContainerPanel	();
	S32	 getChildViewHeight		();

	void onScrollPosChangeCallback(S32, LLScrollbar*);

	void show_hide_scrollbar	(const LLRect& child_rect);
	void showScrollbar			(const LLRect& child_rect);
	void hideScrollbar			(const LLRect& child_rect);

	void updateLayout			( const LLRect& child_rect );
	void ctrlSetLeftTopAndSize	(LLView* panel, S32 left, S32 top, S32 width, S32 height);

	void drawChild(const LLRect& root_rect,LLView* child);

	LLView* findContainerView	();
private:

	class LLAccordionCtrlTabHeader;
	LLAccordionCtrlTabHeader* mHeader; //Header

	bool mDisplayChildren; //Expanded/collapsed
	bool mCollapsible;
	bool mHeaderVisible;

	bool mCanOpenClose;
	bool mFitPanel;

	S32	mPaddingLeft;
	S32	mPaddingRight;
	S32	mPaddingTop;
	S32	mPaddingBottom;

	bool mStoredOpenCloseState;
	bool mWasStateStored;

	LLScrollbar*	mScrollbar;
	LLView*			mContainerPanel;

	LLUIColor mDropdownBGColor;
};

#endif
