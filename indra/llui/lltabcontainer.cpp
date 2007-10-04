/** 
 * @file lltabcontainer.cpp
 * @brief LLTabContainerCommon base class
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

#include "linden_common.h"

#include "lltabcontainer.h"

#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llgl.h"

#include "llbutton.h"
#include "llrect.h"
#include "llpanel.h"
#include "llresmgr.h"
#include "llkeyboard.h"
#include "llresizehandle.h"
#include "llui.h"
#include "lltextbox.h"
#include "llcontrol.h"
#include "llcriticaldamp.h"
#include "lluictrlfactory.h"

#include "lltabcontainervertical.h"

#include "llglheaders.h"

const F32 SCROLL_STEP_TIME = 0.4f;
const F32 SCROLL_DELAY_TIME = 0.5f;
const S32 TAB_PADDING = 15;
const S32 TABCNTR_TAB_MIN_WIDTH = 60;
const S32 TABCNTR_TAB_MAX_WIDTH = 150;
const S32 TABCNTR_TAB_PARTIAL_WIDTH = 12;	// When tabs are parially obscured, how much can you still see.
const S32 TABCNTR_TAB_HEIGHT = 16;
const S32 TABCNTR_ARROW_BTN_SIZE = 16;
const S32 TABCNTR_BUTTON_PANEL_OVERLAP = 1;  // how many pixels the tab buttons and tab panels overlap.
const S32 TABCNTR_TAB_H_PAD = 4;


LLTabContainerCommon::LLTabContainerCommon( 
	const LLString& name, const LLRect& rect, 
	TabPosition pos, 
	void(*close_callback)(void*), void* callback_userdata,
	BOOL bordered )
	: 
	LLPanel(name, rect, bordered),
	mCurrentTabIdx(-1),
	mTabsHidden(FALSE),
	mScrolled(FALSE),
	mScrollPos(0),
	mScrollPosPixels(0),
	mMaxScrollPos(0),
	mCloseCallback( close_callback ),
	mCallbackUserdata( callback_userdata ),
	mTitleBox(NULL),
	mTopBorderHeight(LLPANEL_BORDER_WIDTH),
	mTabPosition(pos),
	mLockedTabCount(0)
{ 
	setMouseOpaque(FALSE);
	mDragAndDropDelayTimer.stop();
}


LLTabContainerCommon::LLTabContainerCommon( 
	const LLString& name,
	const LLString& rect_control,
	TabPosition pos,
	void(*close_callback)(void*), void* callback_userdata,
	BOOL bordered )
	: 
	LLPanel(name, rect_control, bordered),
	mCurrentTabIdx(-1),
	mTabsHidden(FALSE),
	mScrolled(FALSE),
	mScrollPos(0),
	mScrollPosPixels(0),
	mMaxScrollPos(0),
	mCloseCallback( close_callback ),
	mCallbackUserdata( callback_userdata ),
	mTitleBox(NULL),
	mTopBorderHeight(LLPANEL_BORDER_WIDTH),
	mTabPosition(pos),
	mLockedTabCount(0)
{
	setMouseOpaque(FALSE);
	mDragAndDropDelayTimer.stop();
}


LLTabContainerCommon::~LLTabContainerCommon()
{
	std::for_each(mTabList.begin(), mTabList.end(), DeletePointer());
}

LLView* LLTabContainerCommon::getChildByName(const LLString& name, BOOL recurse) const
{
	tuple_list_t::const_iterator itor;
	for (itor = mTabList.begin(); itor != mTabList.end(); ++itor)
	{
		LLPanel *panel = (*itor)->mTabPanel;
		if (panel->getName() == name)
		{
			return panel;
		}
	}
	if (recurse)
	{
		for (itor = mTabList.begin(); itor != mTabList.end(); ++itor)
		{
			LLPanel *panel = (*itor)->mTabPanel;
			LLView *child = panel->getChildByName(name, recurse);
			if (child)
			{
				return child;
			}
		}
	}
	return LLView::getChildByName(name, recurse);
}

void LLTabContainerCommon::addPlaceholder(LLPanel* child, const LLString& label)
{
	addTabPanel(child, label, FALSE, NULL, NULL, 0, TRUE);
}

void LLTabContainerCommon::lockTabs(S32 num_tabs)
{
	// count current tabs or use supplied value and ensure no new tabs get
	// inserted between them
	mLockedTabCount = num_tabs > 0 ? num_tabs : getTabCount();
}

void LLTabContainerCommon::removeTabPanel(LLPanel* child)
{
	BOOL has_focus = gFocusMgr.childHasKeyboardFocus(this);

	// If the tab being deleted is the selected one, select a different tab.
	for(std::vector<LLTabTuple*>::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;
		if( tuple->mTabPanel == child )
		{
 			removeChild( tuple->mButton );
 			delete tuple->mButton;

 			removeChild( tuple->mTabPanel );
// 			delete tuple->mTabPanel;
			
			mTabList.erase( iter );
			delete tuple;

			break;
		}
	}

	// make sure we don't have more locked tabs than we have tabs
	mLockedTabCount = llmin(getTabCount(), mLockedTabCount);

	if (mCurrentTabIdx >= (S32)mTabList.size())
	{
		mCurrentTabIdx = mTabList.size()-1;
	}
	selectTab(mCurrentTabIdx);
	if (has_focus)
	{
		LLPanel* panelp = getPanelByIndex(mCurrentTabIdx);
		if (panelp)
		{
			panelp->setFocus(TRUE);
		}
	}

	updateMaxScrollPos();
}

void LLTabContainerCommon::deleteAllTabs()
{
	// Remove all the tab buttons and delete them.  Also, unlink all the child panels.
	for(std::vector<LLTabTuple*>::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;

		removeChild( tuple->mButton );
		delete tuple->mButton;

 		removeChild( tuple->mTabPanel );
// 		delete tuple->mTabPanel;
	}

	// Actually delete the tuples themselves
	std::for_each(mTabList.begin(), mTabList.end(), DeletePointer());
	mTabList.clear();
	
	// And there isn't a current tab any more
	mCurrentTabIdx = -1;
}


LLPanel* LLTabContainerCommon::getCurrentPanel()
{
	if (mCurrentTabIdx < 0 || mCurrentTabIdx >= (S32) mTabList.size()) return NULL;
		
	return mTabList[mCurrentTabIdx]->mTabPanel;
}

LLTabContainerCommon::LLTabTuple* LLTabContainerCommon::getTabByPanel(LLPanel* child)
{
	for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;
		if( tuple->mTabPanel == child )
		{
			return tuple;
		}
	}
	return NULL;
}

void LLTabContainerCommon::setTabChangeCallback(LLPanel* tab, void (*on_tab_clicked)(void*, bool))
{
	LLTabTuple* tuplep = getTabByPanel(tab);
	if (tuplep)
	{
		tuplep->mOnChangeCallback = on_tab_clicked;
	}
}

void LLTabContainerCommon::setTabUserData(LLPanel* tab, void* userdata)
{
	LLTabTuple* tuplep = getTabByPanel(tab);
	if (tuplep)
	{
		tuplep->mUserData = userdata;
	}
}

S32 LLTabContainerCommon::getCurrentPanelIndex()
{
	return mCurrentTabIdx;
}

S32 LLTabContainerCommon::getTabCount()
{
	return mTabList.size();
}

LLPanel* LLTabContainerCommon::getPanelByIndex(S32 index)
{
	if (index >= 0 && index < (S32)mTabList.size())
	{
		return mTabList[index]->mTabPanel;
	}
	return NULL;
}

S32 LLTabContainerCommon::getIndexForPanel(LLPanel* panel)
{
	for (S32 index = 0; index < (S32)mTabList.size(); index++)
	{
		if (mTabList[index]->mTabPanel == panel)
		{
			return index;
		}
	}
	return -1;
}

S32 LLTabContainerCommon::getPanelIndexByTitle(const LLString& title)
{
	for (S32 index = 0 ; index < (S32)mTabList.size(); index++)
	{
		if (title == mTabList[index]->mButton->getLabelSelected())
		{
			return index;
		}
	}
	return -1;
}

LLPanel *LLTabContainerCommon::getPanelByName(const LLString& name)
{
	for (S32 index = 0 ; index < (S32)mTabList.size(); index++)
	{
		LLPanel *panel = mTabList[index]->mTabPanel;
		if (name == panel->getName())
		{
			return panel;
		}
	}
	return NULL;
}


void LLTabContainerCommon::scrollNext()
{
	// No wrap
	if( mScrollPos < mMaxScrollPos )
	{
		mScrollPos++;
	}
}

void LLTabContainerCommon::scrollPrev()
{
	// No wrap
	if( mScrollPos > 0 )
	{
		mScrollPos--;
	}
}

void LLTabContainerCommon::enableTabButton(S32 which, BOOL enable)
{
	if (which >= 0 && which < (S32)mTabList.size())
	{
		mTabList[which]->mButton->setEnabled(enable);
	}
}

BOOL LLTabContainerCommon::selectTabPanel(LLPanel* child)
{
	S32 idx = 0;
	for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;
		if( tuple->mTabPanel == child )
		{
			return selectTab( idx );
		}
		idx++;
	}
	return FALSE;
}

BOOL LLTabContainerCommon::selectTabByName(const LLString& name)
{
	LLPanel* panel = getPanelByName(name);
	if (!panel)
	{
		llwarns << "LLTabContainerCommon::selectTabByName("
			<< name << ") failed" << llendl;
		return FALSE;
	}

	BOOL result = selectTabPanel(panel);
	return result;
}


void LLTabContainerCommon::selectFirstTab()
{
	selectTab( 0 );
}


void LLTabContainerCommon::selectLastTab()
{
	selectTab( mTabList.size()-1 );
}


void LLTabContainerCommon::selectNextTab()
{
	BOOL tab_has_focus = FALSE;
	if (mCurrentTabIdx >= 0 && mTabList[mCurrentTabIdx]->mButton->hasFocus())
	{
		tab_has_focus = TRUE;
	}
	S32 idx = mCurrentTabIdx+1;
	if (idx >= (S32)mTabList.size())
		idx = 0;
	while (!selectTab(idx) && idx != mCurrentTabIdx)
	{
		idx = (idx + 1 ) % (S32)mTabList.size();
	}

	if (tab_has_focus)
	{
		mTabList[idx]->mButton->setFocus(TRUE);
	}
}

void LLTabContainerCommon::selectPrevTab()
{
	BOOL tab_has_focus = FALSE;
	if (mCurrentTabIdx >= 0 && mTabList[mCurrentTabIdx]->mButton->hasFocus())
	{
		tab_has_focus = TRUE;
	}
	S32 idx = mCurrentTabIdx-1;
	if (idx < 0)
		idx = mTabList.size()-1;
	while (!selectTab(idx) && idx != mCurrentTabIdx)
	{
		idx = idx - 1;
		if (idx < 0)
			idx = mTabList.size()-1;
	}
	if (tab_has_focus)
	{
		mTabList[idx]->mButton->setFocus(TRUE);
	}
}	


void LLTabContainerCommon::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape( width, height, called_from_parent );
	updateMaxScrollPos();
}

// static 
void LLTabContainerCommon::onTabBtn( void* userdata )
{
	LLTabTuple* tuple = (LLTabTuple*) userdata;
	LLTabContainerCommon* self = tuple->mTabContainer;
	self->selectTabPanel( tuple->mTabPanel );
	
	if( tuple->mOnChangeCallback )
	{
		tuple->mOnChangeCallback( tuple->mUserData, true );
	}

	tuple->mTabPanel->setFocus(TRUE);
}

// static 
void LLTabContainerCommon::onCloseBtn( void* userdata )
{
	LLTabContainer* self = (LLTabContainer*) userdata;
	if( self->mCloseCallback )
	{
		self->mCloseCallback( self->mCallbackUserdata );
	}
}

// static 
void LLTabContainerCommon::onNextBtn( void* userdata )
{
	// Scroll tabs to the left
	LLTabContainer* self = (LLTabContainer*) userdata;
	if (!self->mScrolled)
	{
		self->scrollNext();
	}
	self->mScrolled = FALSE;
}

// static 
void LLTabContainerCommon::onNextBtnHeld( void* userdata )
{
	LLTabContainer* self = (LLTabContainer*) userdata;
	if (self->mScrollTimer.getElapsedTimeF32() > SCROLL_STEP_TIME)
	{
		self->mScrollTimer.reset();
		self->scrollNext();
		self->mScrolled = TRUE;
	}
}

// static 
void LLTabContainerCommon::onPrevBtn( void* userdata )
{
	LLTabContainer* self = (LLTabContainer*) userdata;
	if (!self->mScrolled)
	{
		self->scrollPrev();
	}
	self->mScrolled = FALSE;
}


void LLTabContainerCommon::onJumpFirstBtn( void* userdata )
{
	LLTabContainer* self = (LLTabContainer*) userdata;
	
	self->mScrollPos = 0;

}


void LLTabContainerCommon::onJumpLastBtn( void* userdata )
{
	LLTabContainer* self = (LLTabContainer*) userdata;

	self->mScrollPos = self->mMaxScrollPos;
}


// static 
void LLTabContainerCommon::onPrevBtnHeld( void* userdata )
{
	LLTabContainer* self = (LLTabContainer*) userdata;
	if (self->mScrollTimer.getElapsedTimeF32() > SCROLL_STEP_TIME)
	{
		self->mScrollTimer.reset();
		self->scrollPrev();
		self->mScrolled = TRUE;
	}
}

BOOL LLTabContainerCommon::getTabPanelFlashing(LLPanel *child)
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		return tuple->mButton->getFlashing();
	}
	return FALSE;
}

void LLTabContainerCommon::setTabPanelFlashing(LLPanel* child, BOOL state )
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		tuple->mButton->setFlashing( state );
	}
}

void LLTabContainerCommon::setTabImage(LLPanel* child, std::string img_name, const LLColor4& color)
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		tuple->mButton->setImageOverlay(img_name, LLFontGL::RIGHT, color);
	}
}

void LLTabContainerCommon::setTitle(const LLString& title)
{
	if (mTitleBox)
	{
		mTitleBox->setText( title );
	}
}

const LLString LLTabContainerCommon::getPanelTitle(S32 index)
{
	if (index >= 0 && index < (S32)mTabList.size())
	{
		LLButton* tab_button = mTabList[index]->mButton;
		return tab_button->getLabelSelected();
	}
	return LLString::null;
}

void LLTabContainerCommon::setTopBorderHeight(S32 height)
{
	mTopBorderHeight = height;
}

// Change the name of the button for the current tab.
void LLTabContainerCommon::setCurrentTabName(const LLString& name)
{
	// Might not have a tab selected
	if (mCurrentTabIdx < 0) return;

	mTabList[mCurrentTabIdx]->mButton->setLabelSelected(name);
	mTabList[mCurrentTabIdx]->mButton->setLabelUnselected(name);
}

LLView* LLTabContainerCommon::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("tab_container");
	node->getAttributeString("name", name);

	// Figure out if we are creating a vertical or horizontal tab container.
	bool is_vertical = false;
	LLTabContainer::TabPosition tab_position = LLTabContainer::TOP;
	if (node->hasAttribute("tab_position"))
	{
		LLString tab_position_string;
		node->getAttributeString("tab_position", tab_position_string);
		LLString::toLower(tab_position_string);

		if ("top" == tab_position_string)
		{
			tab_position = LLTabContainer::TOP;
			is_vertical = false;
		}
		else if ("bottom" == tab_position_string)
		{
			tab_position = LLTabContainer::BOTTOM;
			is_vertical = false;
		}
		else if ("left" == tab_position_string)
		{
			is_vertical = true;
		}
	}
	BOOL border = FALSE;
	node->getAttributeBOOL("border", border);

	// Create the correct container type.
	LLTabContainerCommon* 	tab_container = NULL;

	if (is_vertical)
	{
		// Vertical tabs can specify tab width
		U32 tab_width = TABCNTRV_TAB_WIDTH;
		if (node->hasAttribute("tab_width"))
		{
			node->getAttributeU32("tab_width", tab_width);
		}

		tab_container = new LLTabContainerVertical(name,
				LLRect::null,
				NULL,
				NULL,
				tab_width,
				border);

	}
	else // horizontal tab container
	{
		// Horizontal tabs can have a title (?)
		LLString title(LLString::null);
		if (node->hasAttribute("title"))
		{
			node->getAttributeString("title", title);
		}

		tab_container = new LLTabContainer(name,
				LLRect::null,
				tab_position,
				NULL,
				NULL,
				title,
				border);
		
		if(node->hasAttribute("tab_min_width"))
		{
			S32	minTabWidth=0;
			node->getAttributeS32("tab_min_width",minTabWidth);
			((LLTabContainer*)tab_container)->setMinTabWidth(minTabWidth); 
		}
		if(node->hasAttribute("tab_max_width"))
		{
			S32	maxTabWidth=0;
			node->getAttributeS32("tab_max_width",maxTabWidth);
			((LLTabContainer*)tab_container)->setMaxTabWidth(maxTabWidth); 
		}
	}
	
	node->getAttributeBOOL("hide_tabs", tab_container->mTabsHidden);

	tab_container->setPanelParameters(node, parent);

	if (LLFloater::getFloaterHost())
	{
		LLFloater::getFloaterHost()->setTabContainer(tab_container);
	}

	//parent->addChild(tab_container);

	// Add all tab panels.
	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		LLView *control = factory->createCtrlWidget(tab_container, child);
		if (control && control->isPanel())
		{
			LLPanel* panelp = (LLPanel*)control;
			LLString label;
			child->getAttributeString("label", label);
			if (label.empty())
			{
				label = panelp->getLabel();
			}
			BOOL placeholder = FALSE;
			child->getAttributeBOOL("placeholder", placeholder);
			tab_container->addTabPanel(panelp, label.c_str(), false,
									   NULL, NULL, 0, placeholder);
		}
	}

	tab_container->selectFirstTab();

	tab_container->postBuild();

	tab_container->initButtons(); // now that we have the correct rect
	
	return tab_container;
}

void LLTabContainerCommon::insertTuple(LLTabTuple * tuple, eInsertionPoint insertion_point)
{
	switch(insertion_point)
	{
	case START:
		// insert the new tab in the front of the list
		mTabList.insert(mTabList.begin() + mLockedTabCount, tuple);
		break;
	case RIGHT_OF_CURRENT:
		// insert the new tab after the current tab (but not before mLockedTabCount)
		{
		tuple_list_t::iterator current_iter = mTabList.begin() + llmax(mLockedTabCount, mCurrentTabIdx + 1);
		mTabList.insert(current_iter, tuple);
		}
		break;
	case END:
	default:
		mTabList.push_back( tuple );
	}
}


LLTabContainer::LLTabContainer( 
	const LLString& name, const LLRect& rect, TabPosition pos,
	void(*close_callback)(void*), void* callback_userdata,
	const LLString& title, BOOL bordered )
	: 
	LLTabContainerCommon(name, rect, pos, close_callback, callback_userdata, bordered),
	mLeftArrowBtn(NULL),
	mJumpLeftArrowBtn(NULL),
	mRightArrowBtn(NULL),
	mJumpRightArrowBtn(NULL),
	mRightTabBtnOffset(0),
	mMinTabWidth(TABCNTR_TAB_MIN_WIDTH),
	mMaxTabWidth(TABCNTR_TAB_MAX_WIDTH),
	mTotalTabWidth(0)
{
	initButtons( );
}

LLTabContainer::LLTabContainer( 
	const LLString& name, const LLString& rect_control, TabPosition pos,
	void(*close_callback)(void*), void* callback_userdata,
	const LLString& title, BOOL bordered )
	: 
	LLTabContainerCommon(name, rect_control, pos, close_callback, callback_userdata, bordered),
	mLeftArrowBtn(NULL),
	mJumpLeftArrowBtn(NULL),
	mRightArrowBtn(NULL),
	mJumpRightArrowBtn(NULL),
	mRightTabBtnOffset(0),
	mMinTabWidth(TABCNTR_TAB_MIN_WIDTH),
	mMaxTabWidth(TABCNTR_TAB_MAX_WIDTH),
	mTotalTabWidth(0)
{
	initButtons( );
}

void LLTabContainer::initButtons()
{
	// Hack:
	if (mRect.getHeight() == 0 || mLeftArrowBtn)
	{
		return; // Don't have a rect yet or already got called
	}
	
	LLString out_id;
	LLString in_id;

	S32 arrow_fudge = 1;		//  match new art better 

	// tabs on bottom reserve room for resize handle (just in case)
	if (mTabPosition == BOTTOM)
	{
		mRightTabBtnOffset = RESIZE_HANDLE_WIDTH;
	}

	// Left and right scroll arrows (for when there are too many tabs to show all at once).
	S32 btn_top = (mTabPosition == TOP ) ? mRect.getHeight() - mTopBorderHeight : TABCNTR_ARROW_BTN_SIZE + 1;

	LLRect left_arrow_btn_rect;
	left_arrow_btn_rect.setLeftTopAndSize( LLPANEL_BORDER_WIDTH+1+TABCNTR_ARROW_BTN_SIZE, btn_top + arrow_fudge, TABCNTR_ARROW_BTN_SIZE, TABCNTR_ARROW_BTN_SIZE );

	LLRect jump_left_arrow_btn_rect;
	jump_left_arrow_btn_rect.setLeftTopAndSize( LLPANEL_BORDER_WIDTH+1, btn_top + arrow_fudge, TABCNTR_ARROW_BTN_SIZE, TABCNTR_ARROW_BTN_SIZE );

	S32 right_pad = TABCNTR_ARROW_BTN_SIZE + LLPANEL_BORDER_WIDTH + 1;

	LLRect right_arrow_btn_rect;
	right_arrow_btn_rect.setLeftTopAndSize( mRect.getWidth() - mRightTabBtnOffset - right_pad - TABCNTR_ARROW_BTN_SIZE,
											btn_top + arrow_fudge,
											TABCNTR_ARROW_BTN_SIZE, TABCNTR_ARROW_BTN_SIZE );


	LLRect jump_right_arrow_btn_rect;
	jump_right_arrow_btn_rect.setLeftTopAndSize( mRect.getWidth() - mRightTabBtnOffset - right_pad,
											btn_top + arrow_fudge,
											TABCNTR_ARROW_BTN_SIZE, TABCNTR_ARROW_BTN_SIZE );

	out_id = "UIImgBtnJumpLeftOutUUID";
	in_id = "UIImgBtnJumpLeftInUUID";
	mJumpLeftArrowBtn = new LLButton(
		"Jump Left Arrow", jump_left_arrow_btn_rect,
		out_id, in_id, "",
		&LLTabContainer::onJumpFirstBtn, this, LLFontGL::sSansSerif );
	mJumpLeftArrowBtn->setFollowsLeft();
	mJumpLeftArrowBtn->setSaveToXML(false);
	mJumpLeftArrowBtn->setTabStop(FALSE);
	addChild(mJumpLeftArrowBtn);

	out_id = "UIImgBtnScrollLeftOutUUID";
	in_id = "UIImgBtnScrollLeftInUUID";
	mLeftArrowBtn = new LLButton(
		"Left Arrow", left_arrow_btn_rect,
		out_id, in_id, "",
		&LLTabContainer::onPrevBtn, this, LLFontGL::sSansSerif );
	mLeftArrowBtn->setHeldDownCallback(onPrevBtnHeld);
	mLeftArrowBtn->setFollowsLeft();
	mLeftArrowBtn->setSaveToXML(false);
	mLeftArrowBtn->setTabStop(FALSE);
	addChild(mLeftArrowBtn);
	
	out_id = "UIImgBtnJumpRightOutUUID";
	in_id = "UIImgBtnJumpRightInUUID";
	mJumpRightArrowBtn = new LLButton(
		"Jump Right Arrow", jump_right_arrow_btn_rect,
		out_id, in_id, "",
		&LLTabContainer::onJumpLastBtn, this,
		LLFontGL::sSansSerif);
	mJumpRightArrowBtn->setFollowsRight();
	mJumpRightArrowBtn->setSaveToXML(false);
	mJumpRightArrowBtn->setTabStop(FALSE);
	addChild(mJumpRightArrowBtn);

	out_id = "UIImgBtnScrollRightOutUUID";
	in_id = "UIImgBtnScrollRightInUUID";
	mRightArrowBtn = new LLButton(
		"Right Arrow", right_arrow_btn_rect,
		out_id, in_id, "",
		&LLTabContainer::onNextBtn, this,
		LLFontGL::sSansSerif);
	mRightArrowBtn->setFollowsRight();
	mRightArrowBtn->setHeldDownCallback(onNextBtnHeld);
	mRightArrowBtn->setSaveToXML(false);
	mRightArrowBtn->setTabStop(FALSE);
	addChild(mRightArrowBtn);


	if( mTabPosition == TOP )
	{
		mRightArrowBtn->setFollowsTop();
		mLeftArrowBtn->setFollowsTop();
		mJumpLeftArrowBtn->setFollowsTop();
		mJumpRightArrowBtn->setFollowsTop();
	}
	else
	{
		mRightArrowBtn->setFollowsBottom();
		mLeftArrowBtn->setFollowsBottom();
		mJumpLeftArrowBtn->setFollowsBottom();
		mJumpRightArrowBtn->setFollowsBottom();
	}

	// set default tab group to be panel contents
	mDefaultTabGroup = 1;
}

LLTabContainer::~LLTabContainer()
{
}

void LLTabContainer::addTabPanel(LLPanel* child, 
								 const LLString& label, 
								 BOOL select, 
								 void (*on_tab_clicked)(void*, bool), 
								 void* userdata,
								 S32 indent,
								 BOOL placeholder,
								 eInsertionPoint insertion_point)
{
	if (child->getParent() == this)
	{
		// already a child of mine
		return;
	}
	const LLFontGL* font = gResMgr->getRes( LLFONT_SANSSERIF_SMALL );

	// Store the original label for possible xml export.
	child->setLabel(label);
	LLString trimmed_label = label;
	LLString::trim(trimmed_label);

	S32 button_width = llclamp(font->getWidth(trimmed_label) + TAB_PADDING, mMinTabWidth, mMaxTabWidth);

	// Tab panel
	S32 tab_panel_top;
	S32 tab_panel_bottom;
	if( LLTabContainer::TOP == mTabPosition )
	{
		tab_panel_top = mRect.getHeight() - mTopBorderHeight - (TABCNTR_TAB_HEIGHT - TABCNTR_BUTTON_PANEL_OVERLAP);	
		tab_panel_bottom = LLPANEL_BORDER_WIDTH;
	}
	else
	{
		tab_panel_top = mRect.getHeight() - mTopBorderHeight;
		tab_panel_bottom = (TABCNTR_TAB_HEIGHT - TABCNTR_BUTTON_PANEL_OVERLAP);  // Run to the edge, covering up the border
	}
	
	LLRect tab_panel_rect(
		LLPANEL_BORDER_WIDTH, 
		tab_panel_top,
		mRect.getWidth()-LLPANEL_BORDER_WIDTH,
		tab_panel_bottom );

	child->setFollowsAll();
	child->translate( tab_panel_rect.mLeft - child->getRect().mLeft, tab_panel_rect.mBottom - child->getRect().mBottom);
	child->reshape( tab_panel_rect.getWidth(), tab_panel_rect.getHeight(), TRUE );
	child->setBackgroundVisible( FALSE );  // No need to overdraw
	// add this child later

	child->setVisible( FALSE );  // Will be made visible when selected

	mTotalTabWidth += button_width;

	// Tab button
	LLRect btn_rect;  // Note: btn_rect.mLeft is just a dummy.  Will be updated in draw().
	LLString tab_img;
	LLString tab_selected_img;
	S32 tab_fudge = 1;		//  To make new tab art look better, nudge buttons up 1 pel

	if( LLTabContainer::TOP == mTabPosition )
	{
		btn_rect.setLeftTopAndSize( 0, mRect.getHeight() - mTopBorderHeight + tab_fudge, button_width, TABCNTR_TAB_HEIGHT );
		tab_img = "UIImgBtnTabTopOutUUID";
		tab_selected_img = "UIImgBtnTabTopInUUID";
	}
	else
	{
		btn_rect.setOriginAndSize( 0, 0 + tab_fudge, button_width, TABCNTR_TAB_HEIGHT );
		tab_img = "UIImgBtnTabBottomOutUUID";
		tab_selected_img = "UIImgBtnTabBottomInUUID";
	}

	if (placeholder)
	{
		// *FIX: wont work for horizontal tabs
		btn_rect.translate(0, -LLBUTTON_V_PAD-2);
		LLString box_label = trimmed_label;
		LLTextBox* text = new LLTextBox(box_label, btn_rect, box_label, font);
		addChild( text, 0 );

		LLButton* btn = new LLButton("", LLRect(0,0,0,0));
		LLTabTuple* tuple = new LLTabTuple( this, child, btn, on_tab_clicked, userdata, text );
		addChild( btn, 0 );
		addChild( child, 1 );
		insertTuple(tuple, insertion_point);
	}
	else
	{
		LLString tooltip = trimmed_label;
		tooltip += "\nAlt-Left arrow for previous tab";
		tooltip += "\nAlt-Right arrow for next tab";

		LLButton* btn = new LLButton(
			LLString(child->getName()) + " tab",
			btn_rect, 
			tab_img, tab_selected_img, "",
			&LLTabContainer::onTabBtn, NULL, // set userdata below
			font,
			trimmed_label, trimmed_label );
		btn->setSaveToXML(false);
		btn->setVisible( FALSE );
		btn->setToolTip( tooltip );
		btn->setScaleImage(TRUE);
		btn->setFixedBorder(14, 14);

		// Try to squeeze in a bit more text
		btn->setLeftHPad( 4 );
		btn->setRightHPad( 2 );
		btn->setHAlign(LLFontGL::LEFT);
		btn->setTabStop(FALSE);
		if (indent)
		{
			btn->setLeftHPad(indent);
		}

		if( mTabPosition == TOP )
		{
			btn->setFollowsTop();
		}
		else
		{
			btn->setFollowsBottom();
		}

		LLTabTuple* tuple = new LLTabTuple( this, child, btn, on_tab_clicked, userdata );
		btn->setCallbackUserData( tuple );
		addChild( btn, 0 );
		addChild( child, 1 );
		insertTuple(tuple, insertion_point);
	}

	updateMaxScrollPos();

	if( select )
	{
		selectLastTab();
	}
}

void LLTabContainer::removeTabPanel(LLPanel* child)
{
	// Adjust the total tab width.
	for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
	{
		LLTabTuple* tuple = *iter;
		if( tuple->mTabPanel == child )
		{
			mTotalTabWidth -= tuple->mButton->getRect().getWidth();
			break;
		}
	}

	LLTabContainerCommon::removeTabPanel(child);
}

void LLTabContainer::setPanelTitle(S32 index, const LLString& title)
{
	if (index >= 0 && index < (S32)mTabList.size())
	{
		LLTabTuple* tuple = mTabList[index];
		LLButton* tab_button = tuple->mButton;
		const LLFontGL* fontp = gResMgr->getRes( LLFONT_SANSSERIF_SMALL );
		mTotalTabWidth -= tab_button->getRect().getWidth();
		tab_button->reshape(llclamp(fontp->getWidth(title) + TAB_PADDING + tuple->mPadding, mMinTabWidth, mMaxTabWidth), tab_button->getRect().getHeight());
		mTotalTabWidth += tab_button->getRect().getWidth();
		tab_button->setLabelSelected(title);
		tab_button->setLabelUnselected(title);
	}
	updateMaxScrollPos();
}


void LLTabContainer::updateMaxScrollPos()
{
	S32 tab_space = 0;
	S32 available_space = 0;
	tab_space = mTotalTabWidth;
	available_space = mRect.getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_TAB_H_PAD);

	if( tab_space > available_space )
	{
		S32 available_width_with_arrows = mRect.getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_ARROW_BTN_SIZE  + TABCNTR_ARROW_BTN_SIZE + 1);
		// subtract off reserved portion on left
		available_width_with_arrows -= TABCNTR_TAB_PARTIAL_WIDTH;

		S32 running_tab_width = 0;
		mMaxScrollPos = mTabList.size();
		for(tuple_list_t::reverse_iterator tab_it = mTabList.rbegin(); tab_it != mTabList.rend(); ++tab_it)
		{
			running_tab_width += (*tab_it)->mButton->getRect().getWidth();
			if (running_tab_width > available_width_with_arrows)
			{
				break;
			}
			mMaxScrollPos--;
		}
		// in case last tab doesn't actually fit on screen, make it the last scrolling position
		mMaxScrollPos = llmin(mMaxScrollPos, (S32)mTabList.size() - 1);
	}
	else
	{
		mMaxScrollPos = 0;
		mScrollPos = 0;
	}
	if (mScrollPos > mMaxScrollPos)
	{
		mScrollPos = mMaxScrollPos;
	}
}

void LLTabContainer::commitHoveredButton(S32 x, S32 y)
{
	if (hasMouseCapture())
	{
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			tuple->mButton->setVisible( TRUE );
			S32 local_x = x - tuple->mButton->getRect().mLeft;
			S32 local_y = y - tuple->mButton->getRect().mBottom;
			if (tuple->mButton->pointInView(local_x, local_y) && tuple->mButton->getEnabled() && !tuple->mTabPanel->getVisible())
			{
				tuple->mButton->onCommit();
			}
		}
	}
}

void LLTabContainer::setMinTabWidth(S32 width)
{
	mMinTabWidth = width;
}

void LLTabContainer::setMaxTabWidth(S32 width)
{
	mMaxTabWidth = width;
}

S32 LLTabContainer::getMinTabWidth() const
{
	return mMinTabWidth;
}

S32 LLTabContainer::getMaxTabWidth() const
{
	return mMaxTabWidth;
}

BOOL LLTabContainer::selectTab(S32 which)
{
	if (which >= (S32)mTabList.size()) return FALSE;
	if (which < 0) return FALSE;

	//if( gFocusMgr.childHasKeyboardFocus( this ) )
	//{
	//	gFocusMgr.setKeyboardFocus( NULL, NULL );
	//}

	LLTabTuple* selected_tuple = mTabList[which];
	if (!selected_tuple)
	{
		return FALSE;
	}
	
	if (mTabList[which]->mButton->getEnabled())
	{
		mCurrentTabIdx = which;

		S32 i = 0;
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			BOOL is_selected = ( tuple == selected_tuple );
			tuple->mTabPanel->setVisible( is_selected );
// 			tuple->mTabPanel->setFocus(is_selected); // not clear that we want to do this here.
			tuple->mButton->setToggleState( is_selected );
			// RN: this limits tab-stops to active button only, which would require arrow keys to switch tabs
			tuple->mButton->setTabStop( is_selected );
			
			if( is_selected && mMaxScrollPos > 0)
			{
				// Make sure selected tab is within scroll region
				if( i < mScrollPos )
				{
					mScrollPos = i;
				}
				else
				{
					S32 available_width_with_arrows = mRect.getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_ARROW_BTN_SIZE  + TABCNTR_ARROW_BTN_SIZE + 1);
					S32 running_tab_width = tuple->mButton->getRect().getWidth();
					S32 j = i - 1;
					S32 min_scroll_pos = i;
					if (running_tab_width < available_width_with_arrows)
					{
						while (j >= 0)
						{
							LLTabTuple* other_tuple = mTabList[j];
							running_tab_width += other_tuple->mButton->getRect().getWidth();
							if (running_tab_width > available_width_with_arrows)
							{
								break;
							}
							j--;
						}
						min_scroll_pos = j + 1;
					}
					mScrollPos = llclamp(mScrollPos, min_scroll_pos, i);
					mScrollPos = llmin(mScrollPos, mMaxScrollPos);
				}
			}
			i++;
		}
		if( selected_tuple->mOnChangeCallback )
		{
			selected_tuple->mOnChangeCallback( selected_tuple->mUserData, false );
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void LLTabContainer::draw()
{
	S32 target_pixel_scroll = 0;
	S32 cur_scroll_pos = mScrollPos;
	if (cur_scroll_pos > 0)
	{
		S32 available_width_with_arrows = mRect.getWidth() - mRightTabBtnOffset - 2 * (LLPANEL_BORDER_WIDTH + TABCNTR_ARROW_BTN_SIZE  + TABCNTR_ARROW_BTN_SIZE + 1);
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			if (cur_scroll_pos == 0)
			{
				break;
			}
			target_pixel_scroll += (*iter)->mButton->getRect().getWidth();
			cur_scroll_pos--;
		}

		// Show part of the tab to the left of what is fully visible
		target_pixel_scroll -= TABCNTR_TAB_PARTIAL_WIDTH;
		// clamp so that rightmost tab never leaves right side of screen
		target_pixel_scroll = llmin(mTotalTabWidth - available_width_with_arrows, target_pixel_scroll);
	}

	mScrollPosPixels = (S32)lerp((F32)mScrollPosPixels, (F32)target_pixel_scroll, LLCriticalDamp::getInterpolant(0.08f));
	if( getVisible() )
	{
		BOOL has_scroll_arrows = (mMaxScrollPos > 0) || (mScrollPosPixels > 0);
		mJumpLeftArrowBtn->setVisible( has_scroll_arrows );
		mJumpRightArrowBtn->setVisible( has_scroll_arrows );
		mLeftArrowBtn->setVisible( has_scroll_arrows );
		mRightArrowBtn->setVisible( has_scroll_arrows );

		// Set the leftmost position of the tab buttons.
		S32 left = LLPANEL_BORDER_WIDTH + (has_scroll_arrows ? (TABCNTR_ARROW_BTN_SIZE * 2) : TABCNTR_TAB_H_PAD);
		left -= mScrollPosPixels;

		// Hide all the buttons
		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			tuple->mButton->setVisible( FALSE );
		}

		LLPanel::draw();

		// if tabs are hidden, don't draw them and leave them in the invisible state
		if (!mTabsHidden)
		{
			// Show all the buttons
			for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
			{
				LLTabTuple* tuple = *iter;
				tuple->mButton->setVisible( TRUE );
			}

			// Draw some of the buttons...
			LLRect clip_rect = getLocalRect();
			if (has_scroll_arrows)
			{
				// ...but clip them.
				clip_rect.mLeft = mLeftArrowBtn->getRect().mRight;
				clip_rect.mRight = mRightArrowBtn->getRect().mLeft;
			}
			LLLocalClipRect clip(clip_rect);

			S32 max_scroll_visible = mTabList.size() - mMaxScrollPos + mScrollPos;
			S32 idx = 0;
			for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
			{
				LLTabTuple* tuple = *iter;

				tuple->mButton->translate( left - tuple->mButton->getRect().mLeft, 0 );
				left += tuple->mButton->getRect().getWidth();

				if( idx < mScrollPos )
				{
					if( tuple->mButton->getFlashing() )
					{
						mLeftArrowBtn->setFlashing( TRUE );
					}
				}
				else
				if( max_scroll_visible < idx )
				{
					if( tuple->mButton->getFlashing() )
					{
						mRightArrowBtn->setFlashing( TRUE );
					}
				}

				LLUI::pushMatrix();
				{
					LLUI::translate((F32)tuple->mButton->getRect().mLeft, (F32)tuple->mButton->getRect().mBottom, 0.f);
					tuple->mButton->draw();
				}
				LLUI::popMatrix();
			
				idx++;
			}
		}

		mLeftArrowBtn->setFlashing(FALSE);
		mRightArrowBtn->setFlashing(FALSE);
	}
}


void LLTabContainer::setRightTabBtnOffset(S32 offset)
{
	mRightArrowBtn->translate( -offset - mRightTabBtnOffset, 0 );
	mRightTabBtnOffset = offset;
	updateMaxScrollPos();
}

BOOL LLTabContainer::handleMouseDown( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	BOOL has_scroll_arrows = (mMaxScrollPos > 0);

	if (has_scroll_arrows)
	{		
		if (mJumpLeftArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpLeftArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpLeftArrowBtn->getRect().mBottom;
			handled = mJumpLeftArrowBtn->handleMouseDown(local_x, local_y, mask);
		}
		if (mJumpRightArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpRightArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpRightArrowBtn->getRect().mBottom;
			handled = mJumpRightArrowBtn->handleMouseDown(local_x, local_y, mask);
		}
		if (mLeftArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mLeftArrowBtn->getRect().mLeft;
			S32 local_y = y - mLeftArrowBtn->getRect().mBottom;
			handled = mLeftArrowBtn->handleMouseDown(local_x, local_y, mask);
		}
		else if (mRightArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mRightArrowBtn->getRect().mLeft;
			S32 local_y = y - mRightArrowBtn->getRect().mBottom;
			handled = mRightArrowBtn->handleMouseDown(local_x, local_y, mask);
		}
	}
	if (!handled)
	{
		handled = LLPanel::handleMouseDown( x, y, mask );
	}

	if (mTabList.size() > 0)
	{
		LLTabTuple* firsttuple = mTabList[0];
		LLRect tab_rect(has_scroll_arrows ? mLeftArrowBtn->getRect().mRight : mJumpLeftArrowBtn->getRect().mLeft,
						firsttuple->mButton->getRect().mTop,
						has_scroll_arrows ? mRightArrowBtn->getRect().mLeft : mJumpRightArrowBtn->getRect().mRight,
						firsttuple->mButton->getRect().mBottom );
		if( tab_rect.pointInRect( x, y ) )
		{
			LLButton* tab_button = mTabList[getCurrentPanelIndex()]->mButton;
			gFocusMgr.setMouseCapture(this);
			gFocusMgr.setKeyboardFocus(tab_button, NULL);
		}
	}
	return handled;
}

BOOL LLTabContainer::handleHover( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	BOOL has_scroll_arrows = (mMaxScrollPos > 0);

	if (has_scroll_arrows)
	{		
		if (mJumpLeftArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpLeftArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpLeftArrowBtn->getRect().mBottom;
			handled = mJumpLeftArrowBtn->handleHover(local_x, local_y, mask);
		}
		if (mJumpRightArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpRightArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpRightArrowBtn->getRect().mBottom;
			handled = mJumpRightArrowBtn->handleHover(local_x, local_y, mask);
		}
		if (mLeftArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mLeftArrowBtn->getRect().mLeft;
			S32 local_y = y - mLeftArrowBtn->getRect().mBottom;
			handled = mLeftArrowBtn->handleHover(local_x, local_y, mask);
		}
		else if (mRightArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mRightArrowBtn->getRect().mLeft;
			S32 local_y = y - mRightArrowBtn->getRect().mBottom;
			handled = mRightArrowBtn->handleHover(local_x, local_y, mask);
		}
	}
	if (!handled)
	{
		handled = LLPanel::handleHover(x, y, mask);
	}

	commitHoveredButton(x, y);
	return handled;
}

BOOL LLTabContainer::handleMouseUp( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	BOOL has_scroll_arrows = (mMaxScrollPos > 0);

	if (has_scroll_arrows)
	{
		if (mJumpLeftArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mJumpLeftArrowBtn->getRect().mLeft;
			S32 local_y = y - mJumpLeftArrowBtn->getRect().mBottom;
			handled = mJumpLeftArrowBtn->handleMouseUp(local_x, local_y, mask);
		}		
		if (mJumpRightArrowBtn->getRect().pointInRect(x,	y))
		{
			S32	local_x	= x	- mJumpRightArrowBtn->getRect().mLeft;
			S32	local_y	= y	- mJumpRightArrowBtn->getRect().mBottom;
			handled = mJumpRightArrowBtn->handleMouseUp(local_x,	local_y, mask);
		}
		if (mLeftArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mLeftArrowBtn->getRect().mLeft;
			S32 local_y = y - mLeftArrowBtn->getRect().mBottom;
			handled = mLeftArrowBtn->handleMouseUp(local_x, local_y, mask);
		}
		else if (mRightArrowBtn->getRect().pointInRect(x, y))
		{
			S32 local_x = x - mRightArrowBtn->getRect().mLeft;
			S32 local_y = y - mRightArrowBtn->getRect().mBottom;
			handled = mRightArrowBtn->handleMouseUp(local_x, local_y, mask);
		}
	}
	if (!handled)
	{
		handled = LLPanel::handleMouseUp( x, y, mask );
	}

	commitHoveredButton(x, y);
	LLPanel* cur_panel = getCurrentPanel();
	if (hasMouseCapture())
	{
		if (cur_panel)
		{
			if (!cur_panel->focusFirstItem(FALSE))
			{
				// if nothing in the panel gets focus, make sure the new tab does
				// otherwise the last tab might keep focus
				mTabList[getCurrentPanelIndex()]->mButton->setFocus(TRUE);
			}
		}
		gFocusMgr.setMouseCapture(NULL);
	}
	return handled;
}

BOOL LLTabContainer::handleToolTip( S32 x, S32 y, LLString& msg, LLRect* sticky_rect )
{
	BOOL handled = LLPanel::handleToolTip( x, y, msg, sticky_rect );
	if (!handled && mTabList.size() > 0 && getVisible() && pointInView( x, y ) ) 
	{
		LLTabTuple* firsttuple = mTabList[0];

		BOOL has_scroll_arrows = (mMaxScrollPos > 0);
		LLRect clip(
			has_scroll_arrows ? mJumpLeftArrowBtn->getRect().mRight : mJumpLeftArrowBtn->getRect().mLeft,
			firsttuple->mButton->getRect().mTop,
			has_scroll_arrows ? mRightArrowBtn->getRect().mLeft : mRightArrowBtn->getRect().mRight,
			0 );
		if( clip.pointInRect( x, y ) )
		{
			for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
			{
				LLTabTuple* tuple = *iter;
				tuple->mButton->setVisible( TRUE );
				S32 local_x = x - tuple->mButton->getRect().mLeft;
				S32 local_y = y - tuple->mButton->getRect().mBottom;
				handled = tuple->mButton->handleToolTip( local_x, local_y, msg, sticky_rect );
				if( handled )
				{
					break;
				}
			}
		}

		for(tuple_list_t::iterator iter = mTabList.begin(); iter != mTabList.end(); ++iter)
		{
			LLTabTuple* tuple = *iter;
			tuple->mButton->setVisible( FALSE );
		}
	}
	return handled;
}

BOOL LLTabContainer::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	if (!getEnabled()) return FALSE;

	if (!gFocusMgr.childHasKeyboardFocus(this)) return FALSE;

	BOOL handled = FALSE;
	if (key == KEY_LEFT && mask == MASK_ALT)
	{
		selectPrevTab();
		handled = TRUE;
	}
	else if (key == KEY_RIGHT && mask == MASK_ALT)
	{
		selectNextTab();
		handled = TRUE;
	}

	if (handled)
	{
		if (getCurrentPanel())
		{
			getCurrentPanel()->setFocus(TRUE);
		}
	}

	if (!gFocusMgr.childHasKeyboardFocus(getCurrentPanel()))
	{
		// if child has focus, but not the current panel, focus
		// is on a button
		switch(key)
		{
		case KEY_UP:
			if (getTabPosition() == BOTTOM && getCurrentPanel())
			{
				getCurrentPanel()->setFocus(TRUE);
			}
			handled = TRUE;
			break;
		case KEY_DOWN:
			if (getTabPosition() == TOP && getCurrentPanel())
			{
				getCurrentPanel()->setFocus(TRUE);
			}
			handled = TRUE;
			break;
		case KEY_LEFT:
		  	selectPrevTab();
			handled = TRUE;
			break;
		case KEY_RIGHT:
		  	selectNextTab();
			handled = TRUE;
			break;
		default:
			break;
		}
	}
	return handled;
}

// virtual
LLXMLNodePtr LLTabContainer::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLTabContainerCommon::getXML();

	node->createChild("tab_position", TRUE)->setStringValue((mTabPosition == TOP ? "top" : "bottom"));

	return node;
}

BOOL LLTabContainer::handleDragAndDrop(S32 x, S32 y, MASK mask,	BOOL drop,	EDragAndDropType type, void* cargo_data, EAcceptance *accept, LLString	&tooltip)
{
	BOOL has_scroll_arrows = (mMaxScrollPos	> 0);

	if( mDragAndDropDelayTimer.getElapsedTimeF32() > SCROLL_DELAY_TIME )
	{

		if (has_scroll_arrows)
		{
			if (mJumpLeftArrowBtn->getRect().pointInRect(x,	y))
			{
				S32	local_x	= x	- mJumpLeftArrowBtn->getRect().mLeft;
				S32	local_y	= y	- mJumpLeftArrowBtn->getRect().mBottom;
				mJumpLeftArrowBtn->handleHover(local_x,	local_y, mask);
			}
			if (mJumpRightArrowBtn->getRect().pointInRect(x,	y))
			{
				S32	local_x	= x	- mJumpRightArrowBtn->getRect().mLeft;
				S32	local_y	= y	- mJumpRightArrowBtn->getRect().mBottom;
				mJumpRightArrowBtn->handleHover(local_x,	local_y, mask);
			}
			if (mLeftArrowBtn->getRect().pointInRect(x,	y))
			{
				S32	local_x	= x	- mLeftArrowBtn->getRect().mLeft;
				S32	local_y	= y	- mLeftArrowBtn->getRect().mBottom;
				mLeftArrowBtn->handleHover(local_x,	local_y, mask);
			}
			else if	(mRightArrowBtn->getRect().pointInRect(x, y))
			{
				S32	local_x	= x	- mRightArrowBtn->getRect().mLeft;
				S32	local_y	= y	- mRightArrowBtn->getRect().mBottom;
				mRightArrowBtn->handleHover(local_x, local_y, mask);
			}
		}

		for(tuple_list_t::iterator iter	= mTabList.begin();	iter !=	 mTabList.end(); ++iter)
		{
			LLTabTuple*	tuple =	*iter;
			tuple->mButton->setVisible(	TRUE );
			S32	local_x	= x	- tuple->mButton->getRect().mLeft;
			S32	local_y	= y	- tuple->mButton->getRect().mBottom;
			if (tuple->mButton->pointInView(local_x, local_y) &&  tuple->mButton->getEnabled() && !tuple->mTabPanel->getVisible())
			{
				tuple->mButton->onCommit();
				mDragAndDropDelayTimer.stop();
			}
		}
	}

	return LLView::handleDragAndDrop(x,	y, mask, drop, type, cargo_data,  accept, tooltip);
}

void LLTabContainer::setTabImage(LLPanel* child, std::string image_name, const LLColor4& color)
{
	LLTabTuple* tuple = getTabByPanel(child);
	if( tuple )
	{
		tuple->mButton->setImageOverlay(image_name, LLFontGL::RIGHT, color);

		const LLFontGL* fontp = gResMgr->getRes( LLFONT_SANSSERIF_SMALL );
		// remove current width from total tab strip width
		mTotalTabWidth -= tuple->mButton->getRect().getWidth();

		S32 image_overlay_width = tuple->mButton->getImageOverlay().notNull() ? 
			tuple->mButton->getImageOverlay()->getWidth(0) :
			0;

		tuple->mPadding = image_overlay_width;

		tuple->mButton->setRightHPad(tuple->mPadding + LLBUTTON_H_PAD);
		tuple->mButton->reshape(llclamp(fontp->getWidth(tuple->mButton->getLabelSelected()) + TAB_PADDING + tuple->mPadding, mMinTabWidth, mMaxTabWidth), 
								tuple->mButton->getRect().getHeight());
		// add back in button width to total tab strip width
		mTotalTabWidth += tuple->mButton->getRect().getWidth();

		// tabs have changed size, might need to scroll to see current tab
		updateMaxScrollPos();
	}
}
