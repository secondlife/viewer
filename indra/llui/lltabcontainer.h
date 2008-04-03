/** 
 * @file lltabcontainer.h
 * @brief LLTabContainer class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_TABCONTAINER_H
#define LL_TABCONTAINER_H

#include "llpanel.h"
#include "lltextbox.h"
#include "llframetimer.h"

extern const S32 TABCNTR_HEADER_HEIGHT;

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

	LLTabContainer( const LLString& name, const LLRect& rect, TabPosition pos,
					BOOL bordered, BOOL is_vertical);

	/*virtual*/ ~LLTabContainer();

	// from LLView
	/*virtual*/ void setValue(const LLSD& value);

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	/*virtual*/ void draw();
	/*virtual*/ BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleHover( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleMouseUp( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect );
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask);
	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,	BOOL drop,
									   EDragAndDropType type, void* cargo_data,
									   EAcceptance* accept, LLString& tooltip);
	/*virtual*/ LLXMLNodePtr getXML(bool save_children = true) const;
	/*virtual*/ LLView* getChildView(const LLString& name, BOOL recurse = TRUE, BOOL create_if_missing = TRUE) const;

	void 		addTabPanel(LLPanel* child, 
							const LLString& label, 
							BOOL select = FALSE,  
							void (*on_tab_clicked)(void*, bool) = NULL, 
							void* userdata = NULL,
							S32 indent = 0,
							BOOL placeholder = FALSE,
							eInsertionPoint insertion_point = END);
	void 		addPlaceholder(LLPanel* child, const LLString& label);
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
	S32			getPanelIndexByTitle(const LLString& title);
	LLPanel*	getPanelByName(const LLString& name);
	void		setCurrentTabName(const LLString& name);

	void		selectFirstTab();
	void		selectLastTab();
	void		selectNextTab();
	 void		selectPrevTab();
	BOOL 		selectTabPanel( LLPanel* child );
	BOOL 		selectTab(S32 which);
	BOOL 		selectTabByName(const LLString& title);

	BOOL        getTabPanelFlashing(LLPanel* child);
	void		setTabPanelFlashing(LLPanel* child, BOOL state);
	void 		setTabImage(LLPanel* child, std::string img_name, const LLColor4& color = LLColor4::white);
	void		setTitle( const LLString& title );
	const LLString getPanelTitle(S32 index);

	void		setTopBorderHeight(S32 height);
	S32			getTopBorderHeight() const;
	
	void 		setTabChangeCallback(LLPanel* tab, void (*on_tab_clicked)(void*,bool));
	void 		setTabUserData(LLPanel* tab, void* userdata);

	void 		setRightTabBtnOffset( S32 offset );
	void 		setPanelTitle(S32 index, const LLString& title);

	TabPosition getTabPosition() const { return mTabPosition; }
	void		setMinTabWidth(S32 width) { mMinTabWidth = width; }
	void		setMaxTabWidth(S32 width) { mMaxTabWidth = width; }
	S32			getMinTabWidth() const { return mMinTabWidth; }
	S32			getMaxTabWidth() const { return mMaxTabWidth; }

	void		startDragAndDropDelayTimer() { mDragAndDropDelayTimer.start(); }
	
	static void	onCloseBtn(void* userdata);
	static void	onTabBtn(void* userdata);
	static void	onNextBtn(void* userdata);
	static void	onNextBtnHeld(void* userdata);
	static void	onPrevBtn(void* userdata);
	static void	onPrevBtnHeld(void* userdata);
	static void onJumpFirstBtn( void* userdata );
	static void onJumpLastBtn( void* userdata );

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

private:
	// Structure used to map tab buttons to and from tab panels
	struct LLTabTuple
	{
		LLTabTuple( LLTabContainer* c, LLPanel* p, LLButton* b,
					void (*cb)(void*,bool), void* userdata, LLTextBox* placeholder = NULL )
			:
			mTabContainer(c),
			mTabPanel(p),
			mButton(b),
			mOnChangeCallback( cb ),
			mUserData( userdata ),
			mOldState(FALSE),
			mPlaceholderText(placeholder),
			mPadding(0)
			{}

		LLTabContainer*  mTabContainer;
		LLPanel*		 mTabPanel;
		LLButton*		 mButton;
		void			 (*mOnChangeCallback)(void*, bool);
		void*			 mUserData;
		BOOL			 mOldState;
		LLTextBox*		 mPlaceholderText;
		S32				 mPadding;
	};

	void initButtons();
	
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

	void							(*mCloseCallback)(void*);
	void*							mCallbackUserdata;

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

	LLFrameTimer					mDragAndDropDelayTimer;
};


#endif  // LL_TABCONTAINER_H
