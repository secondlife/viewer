/** 
 * @file lluictrl.cpp
 * @author James Cook, Richard Nelson, Tom Yedwab
 * @brief Abstract base class for UI controls
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

//#include "llviewerprecompiledheaders.h"
#include "linden_common.h"
#include "lluictrl.h"
#include "llfocusmgr.h"
#include "llpanel.h"

static LLRegisterWidget<LLUICtrl> r("ui_ctrl");

LLFocusableElement::LLFocusableElement()
:	mFocusLostCallback(NULL),
	mFocusReceivedCallback(NULL),
	mFocusChangedCallback(NULL),
	mFocusCallbackUserData(NULL)
{
}

//virtual
LLFocusableElement::~LLFocusableElement()
{
}

void LLFocusableElement::onFocusReceived()
{
	if( mFocusReceivedCallback )
	{
		mFocusReceivedCallback( this, mFocusCallbackUserData );
	}
	if( mFocusChangedCallback )
	{
		mFocusChangedCallback( this, mFocusCallbackUserData );
	}
}

void LLFocusableElement::onFocusLost()
{
	if( mFocusLostCallback )
	{
		mFocusLostCallback( this, mFocusCallbackUserData );
	}

	if( mFocusChangedCallback )
	{
		mFocusChangedCallback( this, mFocusCallbackUserData );
	}
}

BOOL LLFocusableElement::hasFocus() const
{
	return FALSE;
}

void LLFocusableElement::setFocus(BOOL b)
{
}



LLUICtrl::LLUICtrl() :
	mCommitCallback(NULL),
	mLostTopCallback(NULL),
	mValidateCallback(NULL),
	mCallbackUserData(NULL),
	mTentative(FALSE),
	mTabStop(TRUE),
	mIsChrome(FALSE)
{
}

LLUICtrl::LLUICtrl(const std::string& name, const LLRect& rect, BOOL mouse_opaque,
	void (*on_commit_callback)(LLUICtrl*, void*),
	void* callback_userdata,
	U32 reshape)
:	// can't make this automatically follow top and left, breaks lots
	// of buttons in the UI. JC 7/20/2002
	LLView( name, rect, mouse_opaque, reshape ),
	mCommitCallback( on_commit_callback) ,
	mLostTopCallback( NULL ),
	mValidateCallback( NULL ),
	mCallbackUserData( callback_userdata ),
	mTentative( FALSE ),
	mTabStop( TRUE ),
	mIsChrome(FALSE)
{
}

LLUICtrl::~LLUICtrl()
{
	gFocusMgr.releaseFocusIfNeeded( this ); // calls onCommit()

	if( gFocusMgr.getTopCtrl() == this )
	{
		llwarns << "UI Control holding top ctrl deleted: " << getName() << ".  Top view removed." << llendl;
		gFocusMgr.removeTopCtrlWithoutCallback( this );
	}
}

void LLUICtrl::onCommit()
{
	if( mCommitCallback )
	{
		mCommitCallback( this, mCallbackUserData );
	}
}

//virtual
BOOL LLUICtrl::isCtrl() const
{
	return TRUE;
}

//virtual
LLSD LLUICtrl::getValue() const
{
	return LLSD();
}

// virtual
BOOL LLUICtrl::setTextArg( const std::string& key, const LLStringExplicit& text ) 
{ 
	return FALSE; 
}

// virtual
BOOL LLUICtrl::setLabelArg( const std::string& key, const LLStringExplicit& text ) 
{ 
	return FALSE; 
}

// virtual
LLCtrlSelectionInterface* LLUICtrl::getSelectionInterface()	
{ 
	return NULL; 
}

// virtual
LLCtrlListInterface* LLUICtrl::getListInterface()				
{ 
	return NULL; 
}

// virtual
LLCtrlScrollInterface* LLUICtrl::getScrollInterface()			
{ 
	return NULL; 
}

BOOL LLUICtrl::hasFocus() const
{
	return (gFocusMgr.childHasKeyboardFocus(this));
}

void LLUICtrl::setFocus(BOOL b)
{
	// focus NEVER goes to ui ctrls that are disabled!
	if (!getEnabled())
	{
		return;
	}
	if( b )
	{
		if (!hasFocus())
		{
			gFocusMgr.setKeyboardFocus( this );
		}
	}
	else
	{
		if( gFocusMgr.childHasKeyboardFocus(this))
		{
			gFocusMgr.setKeyboardFocus( NULL );
		}
	}
}

void LLUICtrl::onFocusReceived()
{
	// trigger callbacks
	LLFocusableElement::onFocusReceived();

	// find first view in hierarchy above new focus that is a LLUICtrl
	LLView* viewp = getParent();
	LLUICtrl* last_focus = gFocusMgr.getLastKeyboardFocus();

	while (viewp && !viewp->isCtrl()) 
	{
		viewp = viewp->getParent();
	}

	// and if it has newly gained focus, call onFocusReceived()
	LLUICtrl* ctrlp = static_cast<LLUICtrl*>(viewp);
	if (ctrlp && (!last_focus || !last_focus->hasAncestor(ctrlp)))
	{
		ctrlp->onFocusReceived();
	}
}

void LLUICtrl::onFocusLost()
{
	// trigger callbacks
	LLFocusableElement::onFocusLost();

	// find first view in hierarchy above old focus that is a LLUICtrl
	LLView* viewp = getParent();
	while (viewp && !viewp->isCtrl()) 
	{
		viewp = viewp->getParent();
	}

	// and if it has just lost focus, call onFocusReceived()
	LLUICtrl* ctrlp = static_cast<LLUICtrl*>(viewp);
	// hasFocus() includes any descendants
	if (ctrlp && !ctrlp->hasFocus())
	{
		ctrlp->onFocusLost();
	}
}

void LLUICtrl::onLostTop()
{
	if (mLostTopCallback)
	{
		mLostTopCallback(this, mCallbackUserData);
	}
}


// virtual
void LLUICtrl::setTabStop( BOOL b )	
{ 
	mTabStop = b;
}

// virtual
BOOL LLUICtrl::hasTabStop() const		
{ 
	return mTabStop;
}

// virtual
BOOL LLUICtrl::acceptsTextInput() const
{ 
	return FALSE; 
}

//virtual
BOOL LLUICtrl::isDirty() const
{
	return FALSE;
};

//virtual
void LLUICtrl::resetDirty()
{
}

// virtual
void LLUICtrl::onTabInto()				
{
}

// virtual
void LLUICtrl::clear()					
{
}

// virtual
void LLUICtrl::setIsChrome(BOOL is_chrome)
{
	mIsChrome = is_chrome; 
}

// virtual
BOOL LLUICtrl::getIsChrome() const
{ 

	LLView* parent_ctrl = getParent();
	while(parent_ctrl)
	{
		if(parent_ctrl->isCtrl())
		{
			break;	
		}
		parent_ctrl = parent_ctrl->getParent();
	}
	
	if(parent_ctrl)
	{
		return mIsChrome || ((LLUICtrl*)parent_ctrl)->getIsChrome();
	}
	else
	{
		return mIsChrome ; 
	}
}

// this comparator uses the crazy disambiguating logic of LLCompareByTabOrder,
// but to switch up the order so that children that have the default tab group come first
// and those that are prior to the default tab group come last
class CompareByDefaultTabGroup: public LLCompareByTabOrder
{
public:
	CompareByDefaultTabGroup(LLView::child_tab_order_t order, S32 default_tab_group):
			LLCompareByTabOrder(order),
			mDefaultTabGroup(default_tab_group) {}
private:
	/*virtual*/ bool compareTabOrders(const LLView::tab_order_t & a, const LLView::tab_order_t & b) const
	{
		S32 ag = a.first; // tab group for a
		S32 bg = b.first; // tab group for b
		// these two ifs have the effect of moving elements prior to the default tab group to the end of the list 
		// (still sorted relative to each other, though)
		if(ag < mDefaultTabGroup && bg >= mDefaultTabGroup) return false;
		if(bg < mDefaultTabGroup && ag >= mDefaultTabGroup) return true;
		return a < b;  // sort correctly if they're both on the same side of the default tab group
	}
	S32 mDefaultTabGroup;
};


// Sorter for plugging into the query.
// I'd have defined it local to the one method that uses it but that broke the VS 05 compiler. -MG
class LLUICtrl::DefaultTabGroupFirstSorter : public LLQuerySorter, public LLSingleton<DefaultTabGroupFirstSorter>
{
public:
	/*virtual*/ void operator() (LLView * parent, viewList_t &children) const
	{
		children.sort(CompareByDefaultTabGroup(parent->getCtrlOrder(), parent->getDefaultTabGroup()));
	}
};

BOOL LLUICtrl::focusFirstItem(BOOL prefer_text_fields, BOOL focus_flash)
{
	// try to select default tab group child
	LLCtrlQuery query = getTabOrderQuery();
	// sort things such that the default tab group is at the front
	query.setSorter(DefaultTabGroupFirstSorter::getInstance());
	child_list_t result = query(this);
	if(result.size() > 0)
	{
		LLUICtrl * ctrl = static_cast<LLUICtrl*>(result.front());
		if(!ctrl->hasFocus())
		{
			ctrl->setFocus(TRUE);
			ctrl->onTabInto();  
			if(focus_flash)
			{
				gFocusMgr.triggerFocusFlash();
			}
		}
		return TRUE;
	}	
	// search for text field first
	if(prefer_text_fields)
	{
		LLCtrlQuery query = getTabOrderQuery();
		query.addPreFilter(LLUICtrl::LLTextInputFilter::getInstance());
		child_list_t result = query(this);
		if(result.size() > 0)
		{
			LLUICtrl * ctrl = static_cast<LLUICtrl*>(result.front());
			if(!ctrl->hasFocus())
			{
				ctrl->setFocus(TRUE);
				ctrl->onTabInto();  
				gFocusMgr.triggerFocusFlash();
			}
			return TRUE;
		}
	}
	// no text field found, or we don't care about text fields
	result = getTabOrderQuery().run(this);
	if(result.size() > 0)
	{
		LLUICtrl * ctrl = static_cast<LLUICtrl*>(result.front());
		if(!ctrl->hasFocus())
		{
			ctrl->setFocus(TRUE);
			ctrl->onTabInto();  
			gFocusMgr.triggerFocusFlash();
		}
		return TRUE;
	}	
	return FALSE;
}

BOOL LLUICtrl::focusLastItem(BOOL prefer_text_fields)
{
	// search for text field first
	if(prefer_text_fields)
	{
		LLCtrlQuery query = getTabOrderQuery();
		query.addPreFilter(LLUICtrl::LLTextInputFilter::getInstance());
		child_list_t result = query(this);
		if(result.size() > 0)
		{
			LLUICtrl * ctrl = static_cast<LLUICtrl*>(result.back());
			if(!ctrl->hasFocus())
			{
				ctrl->setFocus(TRUE);
				ctrl->onTabInto();  
				gFocusMgr.triggerFocusFlash();
			}
			return TRUE;
		}
	}
	// no text field found, or we don't care about text fields
	child_list_t result = getTabOrderQuery().run(this);
	if(result.size() > 0)
	{
		LLUICtrl * ctrl = static_cast<LLUICtrl*>(result.back());
		if(!ctrl->hasFocus())
		{
			ctrl->setFocus(TRUE);
			ctrl->onTabInto();  
			gFocusMgr.triggerFocusFlash();
		}
		return TRUE;
	}	
	return FALSE;
}

BOOL LLUICtrl::focusNextItem(BOOL text_fields_only)
{
	// this assumes that this method is called on the focus root.
	LLCtrlQuery query = getTabOrderQuery();
	if(text_fields_only || LLUI::sConfigGroup->getBOOL("TabToTextFieldsOnly"))
	{
		query.addPreFilter(LLUICtrl::LLTextInputFilter::getInstance());
	}
	child_list_t result = query(this);
	return focusNext(result);
}

BOOL LLUICtrl::focusPrevItem(BOOL text_fields_only)
{
	// this assumes that this method is called on the focus root.
	LLCtrlQuery query = getTabOrderQuery();
	if(text_fields_only || LLUI::sConfigGroup->getBOOL("TabToTextFieldsOnly"))
	{
		query.addPreFilter(LLUICtrl::LLTextInputFilter::getInstance());
	}
	child_list_t result = query(this);
	return focusPrev(result);
}

LLUICtrl* LLUICtrl::findRootMostFocusRoot()
{
	LLUICtrl* focus_root = NULL;
	LLUICtrl* next_view = this;
	while(next_view)
	{
		if (next_view->isFocusRoot())
		{
			focus_root = next_view;
		}
		next_view = next_view->getParentUICtrl();
	}

	return focus_root;
}


/*
// Don't let the children handle the tool tip.  Handle it here instead.
BOOL LLUICtrl::handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen)
{
	BOOL handled = FALSE;
	if (getVisible() && pointInView( x, y ) ) 
	{
		if( !mToolTipMsg.empty() )
		{
			msg = mToolTipMsg;

			// Convert rect local to screen coordinates
			localPointToScreen( 
				0, 0, 
				&(sticky_rect_screen->mLeft), &(sticky_rect_screen->mBottom) );
			localPointToScreen(
				getRect().getWidth(), getRect().getHeight(),
				&(sticky_rect_screen->mRight), &(sticky_rect_screen->mTop) );

			handled = TRUE;
		}
	}

	if (!handled)
	{
		return LLView::handleToolTip(x, y, msg, sticky_rect_screen);
	}

	return handled;
}*/

void LLUICtrl::initFromXML(LLXMLNodePtr node, LLView* parent)
{
	BOOL has_tab_stop = hasTabStop();
	node->getAttributeBOOL("tab_stop", has_tab_stop);

	setTabStop(has_tab_stop);

	LLView::initFromXML(node, parent);
}

LLXMLNodePtr LLUICtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLView::getXML(save_children);
	node->createChild("tab_stop", TRUE)->setBoolValue(hasTabStop());

	return node;
}

//static 
LLView* LLUICtrl::fromXML(LLXMLNodePtr node, LLView* parent, class LLUICtrlFactory* factory)
{
	LLUICtrl* ctrl = new LLUICtrl();
	ctrl->initFromXML(node, parent);
	return ctrl;
}


// Skip over any parents that are not LLUICtrl's
//  Used in focus logic since only LLUICtrl elements can have focus
LLUICtrl* LLUICtrl::getParentUICtrl() const
{
	LLView* parent = getParent();
	while (parent)
	{
		if (parent->isCtrl())
		{
			return (LLUICtrl*)(parent);
		}
		else
		{
			parent =  parent->getParent();
		}
	}
	return NULL;
}

// virtual
void LLUICtrl::setTentative(BOOL b)									
{ 
	mTentative = b; 
}

// virtual
BOOL LLUICtrl::getTentative() const									
{ 
	return mTentative; 
}

// virtual
void LLUICtrl::setDoubleClickCallback( void (*cb)(void*) )				
{ 
}

// virtual
void LLUICtrl::setColor(const LLColor4& color)							
{ }

// virtual
void LLUICtrl::setMinValue(LLSD min_value)								
{ }

// virtual
void LLUICtrl::setMaxValue(LLSD max_value)								
{ }
