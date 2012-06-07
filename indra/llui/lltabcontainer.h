/** 
 * @file lltabcontainer.h
 * @brief LLTabContainer class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_TABCONTAINER_H
#define LL_TABCONTAINER_H

#include "llpanel.h"
#include "lltextbox.h"
#include "llframetimer.h"
#include "lliconctrl.h"
#include "llbutton.h"

class LLTabTuple;

class LLTabContainer : public LLPanel
{
public:
	enum TabPosition
	{
		TOP,
		BOTTOM,
		LEFT
	};
	typedef enum e_insertion_point
	{
		START,
		END,
		LEFT_OF_CURRENT,
		RIGHT_OF_CURRENT
	} eInsertionPoint;

	struct TabPositions : public LLInitParam::TypeValuesHelper<LLTabContainer::TabPosition, TabPositions>
	{
		static void declareValues();
	};

	struct TabParams : public LLInitParam::Block<TabParams>
	{
		Optional<LLUIImage*>				tab_top_image_unselected,
											tab_top_image_selected,
											tab_top_image_flash,
											tab_bottom_image_unselected,
											tab_bottom_image_selected,
											tab_bottom_image_flash,
											tab_left_image_unselected,
											tab_left_image_selected,
											tab_left_image_flash;		
		TabParams();
	};

	struct Params
	:	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<TabPosition, TabPositions>	tab_position;
		Optional<S32>						tab_width,
											tab_min_width,
											tab_max_width,
											tab_height,
											label_pad_bottom,
											label_pad_left;

		Optional<bool>						hide_tabs;
		Optional<S32>						tab_padding_right;

		Optional<TabParams>					first_tab,
											middle_tab,
											last_tab;

		/**
		 * Tab label horizontal alignment
		 */
		Optional<LLFontGL::HAlign>			font_halign;

		/**
		 * Tab label ellipses
		 */
		Optional<bool>						use_ellipses;

		/**
		 * Use LLCustomButtonIconCtrl or LLButton in LLTabTuple
		 */
		Optional<bool>						use_custom_icon_ctrl;

		/**
		 * Open tabs on hover in drag and drop situations
		 */
		Optional<bool>						open_tabs_on_drag_and_drop;
		
		/**
		 *  Paddings for LLIconCtrl in case of LLCustomButtonIconCtrl usage(use_custom_icon_ctrl = true)
		 */
		Optional<S32>						tab_icon_ctrl_pad;

		Params();
	};

protected:
	LLTabContainer(const Params&);
	friend class LLUICtrlFactory;

public:
	//LLTabContainer( const std::string& name, const LLRect& rect, TabPosition pos,
	//				BOOL bordered, BOOL is_vertical);

	/*virtual*/ ~LLTabContainer();

	// from LLView
	/*virtual*/ void setValue(const LLSD& value);

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	/*virtual*/ void draw();
	/*virtual*/ BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleHover( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleMouseUp( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleToolTip(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask);
	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,	BOOL drop,
									   EDragAndDropType type, void* cargo_data,
									   EAcceptance* accept, std::string& tooltip);
	/*virtual*/ LLView* getChildView(const std::string& name, BOOL recurse = TRUE) const;
	/*virtual*/ LLView* findChildView(const std::string& name, BOOL recurse = TRUE) const;
	/*virtual*/ void initFromParams(const LLPanel::Params& p);
	/*virtual*/ bool addChild(LLView* view, S32 tab_group = 0);
	/*virtual*/ BOOL postBuild();

	struct TabPanelParams : public LLInitParam::Block<TabPanelParams>
	{
		Mandatory<LLPanel*>			panel;
		
		Optional<std::string>		label;
		Optional<bool>				select_tab,
									is_placeholder;
		Optional<S32>				indent;
		Optional<eInsertionPoint>	insert_at;
		Optional<void*>				user_data;

		TabPanelParams()
		:	panel("panel", NULL),
			label("label"),
			select_tab("select_tab"),
			is_placeholder("is_placeholder"),
			indent("indent"),
			insert_at("insert_at", END)
		{}
	};

	void		addTabPanel(LLPanel* panel);
	void		addTabPanel(const TabPanelParams& panel);
	void 		addPlaceholder(LLPanel* child, const std::string& label);
	void 		removeTabPanel( LLPanel* child );
	void 		lockTabs(S32 num_tabs = 0);
	void 		unlockTabs();
	S32 		getNumLockedTabs() { return mLockedTabCount; }
	void 		enableTabButton(S32 which, BOOL enable);
	void 		deleteAllTabs();
	LLPanel*	getCurrentPanel();
	S32			getCurrentPanelIndex();
	S32			getTabCount();
	LLPanel*	getPanelByIndex(S32 index);
	S32			getIndexForPanel(LLPanel* panel);
	S32			getPanelIndexByTitle(const std::string& title);
	LLPanel*	getPanelByName(const std::string& name);
	void		setCurrentTabName(const std::string& name);

	void		selectFirstTab();
	void		selectLastTab();
	void		selectNextTab();
	 void		selectPrevTab();
	BOOL 		selectTabPanel( LLPanel* child );
	BOOL 		selectTab(S32 which);
	BOOL 		selectTabByName(const std::string& title);

	BOOL        getTabPanelFlashing(LLPanel* child);
	void		setTabPanelFlashing(LLPanel* child, BOOL state);
	void 		setTabImage(LLPanel* child, std::string img_name, const LLColor4& color = LLColor4::white);
	void 		setTabImage(LLPanel* child, const LLUUID& img_id, const LLColor4& color = LLColor4::white);
	void		setTabImage(LLPanel* child, LLIconCtrl* icon);
	void		setTitle( const std::string& title );
	const std::string getPanelTitle(S32 index);

	void		setTopBorderHeight(S32 height);
	S32			getTopBorderHeight() const;
	
	void 		setRightTabBtnOffset( S32 offset );
	void 		setPanelTitle(S32 index, const std::string& title);

	TabPosition getTabPosition() const { return mTabPosition; }
	void		setMinTabWidth(S32 width) { mMinTabWidth = width; }
	void		setMaxTabWidth(S32 width) { mMaxTabWidth = width; }
	S32			getMinTabWidth() const { return mMinTabWidth; }
	S32			getMaxTabWidth() const { return mMaxTabWidth; }

	void		startDragAndDropDelayTimer() { mDragAndDropDelayTimer.start(); }
	
	void onTabBtn( const LLSD& data, LLPanel* panel );
	void onNextBtn(const LLSD& data);
	void onNextBtnHeld(const LLSD& data);
	void onPrevBtn(const LLSD& data);
	void onPrevBtnHeld(const LLSD& data);
	void onJumpFirstBtn( const LLSD& data );
	void onJumpLastBtn( const LLSD& data );

private:

	void initButtons();
	
	BOOL		setTab(S32 which);

	LLTabTuple* getTab(S32 index) 		{ return mTabList[index]; }
	LLTabTuple* getTabByPanel(LLPanel* child);
	void insertTuple(LLTabTuple * tuple, eInsertionPoint insertion_point);

	S32 getScrollPos() const			{ return mScrollPos; }
	void setScrollPos(S32 pos)			{ mScrollPos = pos; }
	S32 getMaxScrollPos() const			{ return mMaxScrollPos; }
	void setMaxScrollPos(S32 pos)		{ mMaxScrollPos = pos; }
	S32 getScrollPosPixels() const		{ return mScrollPosPixels; }
	void setScrollPosPixels(S32 pixels)	{ mScrollPosPixels = pixels; }

	void setTabsHidden(BOOL hidden)		{ mTabsHidden = hidden; }
	BOOL getTabsHidden() const			{ return mTabsHidden; }
	
	void setCurrentPanelIndex(S32 index) { mCurrentTabIdx = index; }

	void scrollPrev() { mScrollPos = llmax(0, mScrollPos-1); } // No wrap
	void scrollNext() { mScrollPos = llmin(mScrollPos+1, mMaxScrollPos); } // No wrap

	void updateMaxScrollPos();
	void commitHoveredButton(S32 x, S32 y);

	// updates tab button images given the tuple, tab position and the corresponding params
	void update_images(LLTabTuple* tuple, TabParams params, LLTabContainer::TabPosition pos);
	void reshapeTuple(LLTabTuple* tuple);

	// Variables
	
	typedef std::vector<LLTabTuple*> tuple_list_t;
	tuple_list_t					mTabList;
	
	S32								mCurrentTabIdx;
	BOOL							mTabsHidden;

	BOOL							mScrolled;
	LLFrameTimer					mScrollTimer;
	S32								mScrollPos;
	S32								mScrollPosPixels;
	S32								mMaxScrollPos;

	LLTextBox*						mTitleBox;

	S32								mTopBorderHeight;
	TabPosition 					mTabPosition;
	S32								mLockedTabCount;
	S32								mMinTabWidth;
	LLButton*						mPrevArrowBtn;
	LLButton*						mNextArrowBtn;

	BOOL							mIsVertical;

	// Horizontal specific
	LLButton*						mJumpPrevArrowBtn;
	LLButton*						mJumpNextArrowBtn;

	S32								mRightTabBtnOffset; // Extra room to the right of the tab buttons.

	S32								mMaxTabWidth;
	S32								mTotalTabWidth;
	S32								mTabHeight;

	// Padding under the text labels of tab buttons
	S32								mLabelPadBottom;
	// Padding to the left of text labels of tab buttons
	S32								mLabelPadLeft;

	LLFrameTimer					mDragAndDropDelayTimer;
	
	LLFontGL::HAlign                mFontHalign;
	const LLFontGL*					mFont;

	TabParams						mFirstTabParams;
	TabParams						mMiddleTabParams;
	TabParams						mLastTabParams;

	bool							mCustomIconCtrlUsed;
	bool							mOpenTabsOnDragAndDrop;
	S32								mTabIconCtrlPad;
	bool							mUseTabEllipses;
};

#endif  // LL_TABCONTAINER_H
