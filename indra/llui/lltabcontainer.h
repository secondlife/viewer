/** 
 * @file lltabcontainer.h
 * @brief LLTabContainerCommon base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// Fear my script-fu!

#ifndef LL_TABCONTAINER_H
#define LL_TABCONTAINER_H

#include "llpanel.h"
#include "llframetimer.h"

class LLButton;
class LLTextBox;


class LLTabContainerCommon : public LLPanel
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
		RIGHT_OF_CURRENT
	} eInsertionPoint;

	LLTabContainerCommon( const LLString& name, 
		const LLRect& rect,
		TabPosition pos,
		void(*close_callback)(void*), void* callback_userdata, 
		BOOL bordered = TRUE);

	LLTabContainerCommon( const LLString& name, 		
		const LLString& rect_control,
		TabPosition pos,
		void(*close_callback)(void*), void* callback_userdata, 
		BOOL bordered = TRUE);

	virtual ~LLTabContainerCommon();

	virtual void initButtons() = 0;
	
	virtual void setValue(const LLSD& value) { selectTab((S32) value.asInteger()); }
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_TAB_CONTAINER; }
	virtual LLString getWidgetTag() const { return LL_TAB_CONTAINER_COMMON_TAG; }
	
	virtual LLView* getChildByName(const LLString& name, BOOL recurse = FALSE) const;

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	virtual void addTabPanel(LLPanel* child, 
							 const LLString& label, 
							 BOOL select = FALSE,  
							 void (*on_tab_clicked)(void*, bool) = NULL, 
							 void* userdata = NULL,
							 S32 indent = 0,
							 BOOL placeholder = FALSE,
							 eInsertionPoint insertion_point = END) = 0;
	virtual void		addPlaceholder(LLPanel* child, const LLString& label);
	virtual void		lockTabs();

	virtual void		enableTabButton(S32 which, BOOL enable);

	virtual void removeTabPanel( LLPanel* child );
	virtual void		deleteAllTabs();
	virtual LLPanel*	getCurrentPanel();
	virtual S32			getCurrentPanelIndex();
	virtual S32			getTabCount();
	virtual S32			getPanelIndexByTitle(const LLString& title);
	virtual LLPanel*	getPanelByIndex(S32 index);
	virtual LLPanel*	getPanelByName(const LLString& name);
	virtual S32			getIndexForPanel(LLPanel* panel);

	virtual void		setCurrentTabName(const LLString& name);


	virtual void		selectFirstTab();
	virtual void		selectLastTab();
	virtual BOOL selectTabPanel( LLPanel* child );
	virtual BOOL selectTab(S32 which) = 0;
	virtual BOOL 		selectTabByName(const LLString& title);
	virtual void		selectNextTab();
	virtual void		selectPrevTab();

	BOOL        getTabPanelFlashing(LLPanel* child);
	void		setTabPanelFlashing(LLPanel* child, BOOL state);
	virtual void setTabImage(LLPanel* child, std::string img_name);
	void		setTitle( const LLString& title );
	const LLString getPanelTitle(S32 index);

	void		setDragAndDropDelayTimer() { mDragAndDropDelayTimer.start(); }

	virtual void		setTopBorderHeight(S32 height);
	
	virtual void 		setTabChangeCallback(LLPanel* tab, void (*on_tab_clicked)(void*,bool));
	virtual void 		setTabUserData(LLPanel* tab, void* userdata);

	virtual void reshape(S32 width, S32 height, BOOL called_from_parent);

	static void	onCloseBtn(void* userdata);
	static void	onTabBtn(void* userdata);
	static void	onNextBtn(void* userdata);
	static void	onNextBtnHeld(void* userdata);
	static void	onPrevBtn(void* userdata);
	static void	onPrevBtnHeld(void* userdata);
	static void onJumpFirstBtn( void* userdata );
	static void onJumpLastBtn( void* userdata );

	virtual void		setRightTabBtnOffset( S32 offset ) { }
	virtual void		setPanelTitle(S32 index, const LLString& title) { }

	virtual TabPosition getTabPosition() { return mTabPosition; }


protected:
	// Structure used to map tab buttons to and from tab panels
	struct LLTabTuple
	{
		LLTabTuple( LLTabContainerCommon* c, LLPanel* p, LLButton* b,
					void (*cb)(void*,bool), void* userdata, LLTextBox* placeholder = NULL )
			:
			mTabContainer(c),
			mTabPanel(p),
			mButton(b),
			mOnChangeCallback( cb ),
			mUserData( userdata ),
			mOldState(FALSE),
			mPlaceholderText(placeholder)
			{}

		LLTabContainerCommon*  mTabContainer;
		LLPanel*		 mTabPanel;
		LLButton*		 mButton;
		void			 (*mOnChangeCallback)(void*, bool);
		void*			 mUserData;
		BOOL			 mOldState;
		LLTextBox*		 mPlaceholderText;
	};

	typedef std::vector<LLTabTuple*> tuple_list_t;
	tuple_list_t					mTabList;
	S32								mCurrentTabIdx;

	BOOL							mScrolled;
	LLFrameTimer					mScrollTimer;
	S32								mScrollPos;
	S32								mScrollPosPixels;
	S32								mMaxScrollPos;

	LLFrameTimer					mDragAndDropDelayTimer;

	void							(*mCloseCallback)(void*);
	void*							mCallbackUserdata;

	LLTextBox*						mTitleBox;

	S32								mTopBorderHeight;
	TabPosition 					mTabPosition;
	S32								mLockedTabCount;

protected:
	void		scrollPrev();
	void		scrollNext();

	virtual void		updateMaxScrollPos() = 0;
	virtual void		commitHoveredButton(S32 x, S32 y) = 0;
	LLTabTuple* getTabByPanel(LLPanel* child);
	void insertTuple(LLTabTuple * tuple, eInsertionPoint insertion_point);
};

class LLTabContainer : public LLTabContainerCommon
{
public:
	LLTabContainer( const LLString& name, const LLRect& rect, TabPosition pos, 
		void(*close_callback)(void*), void* callback_userdata, 
					const LLString& title=LLString::null, BOOL bordered = TRUE );

	LLTabContainer( const LLString& name, const LLString& rect_control, TabPosition pos, 
		void(*close_callback)(void*), void* callback_userdata, 
					const LLString& title=LLString::null, BOOL bordered = TRUE );
	
	~LLTabContainer();

	/*virtual*/ void initButtons();

	/*virtual*/ void draw();

	/*virtual*/ void addTabPanel(LLPanel* child, 
							 const LLString& label, 
							 BOOL select = FALSE,  
							 void (*on_tab_clicked)(void*, bool) = NULL, 
							 void* userdata = NULL,
							 S32 indent = 0,
							 BOOL placeholder = FALSE,
							 eInsertionPoint insertion_point = END);

	/*virtual*/ BOOL selectTab(S32 which);
	/*virtual*/ void removeTabPanel( LLPanel* child );

	/*virtual*/ void		setPanelTitle(S32 index, const LLString& title);
	/*virtual*/ void		setTabImage(LLPanel* child, std::string img_name);
	/*virtual*/ void		setRightTabBtnOffset( S32 offset );
	 
	/*virtual*/ void		setMinTabWidth(S32 width);
	/*virtual*/ void		setMaxTabWidth(S32 width);

	/*virtual*/ S32			getMinTabWidth() const;
	/*virtual*/ S32			getMaxTabWidth() const;

	/*virtual*/ BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleHover( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleMouseUp( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect );
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,	BOOL drop,
									   EDragAndDropType type, void* cargo_data,
									   EAcceptance* accept, LLString& tooltip);

	virtual LLXMLNodePtr getXML(bool save_children = true) const;


protected:

	LLButton*						mLeftArrowBtn;
	LLButton*						mJumpLeftArrowBtn;
	LLButton*						mRightArrowBtn;
	LLButton*						mJumpRightArrowBtn;

	S32								mRightTabBtnOffset; // Extra room to the right of the tab buttons.

protected:
	virtual void	updateMaxScrollPos();
	virtual void	commitHoveredButton(S32 x, S32 y);

	S32								mMinTabWidth;
	S32								mMaxTabWidth;
	S32								mTotalTabWidth;
};

const S32 TABCNTR_CLOSE_BTN_SIZE = 16;
const S32 TABCNTR_HEADER_HEIGHT = LLPANEL_BORDER_WIDTH + TABCNTR_CLOSE_BTN_SIZE;

#endif  // LL_TABCONTAINER_H
