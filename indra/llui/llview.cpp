/** 
 * @file llview.cpp
 * @author James Cook
 * @brief Container for other views, anything that draws.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llview.h"

#include "llstring.h"
#include "llrect.h"
#include "llgl.h"
#include "llevent.h"
#include "llfontgl.h"
#include "llfocusmgr.h"
#include "llglheaders.h"
#include "llwindow.h"
#include "llstl.h"
#include "lluictrl.h"
#include "llui.h"	// colors saved settings
#include "v3color.h"
#include "llstl.h"

#include <boost/tokenizer.hpp>

#include <assert.h>

BOOL	LLView::sDebugRects = FALSE;
BOOL	LLView::sDebugKeys = FALSE;
S32		LLView::sDepth = 0;
BOOL	LLView::sDebugMouseHandling = FALSE;
LLString LLView::sMouseHandlerMessage;
S32	LLView::sSelectID = GL_NAME_UI_RESERVED;
BOOL	LLView::sEditingUI = FALSE;
BOOL	LLView::sForceReshape = FALSE;
LLView*	LLView::sEditingUIView = NULL;
S32		LLView::sLastLeftXML = S32_MIN;
S32		LLView::sLastBottomXML = S32_MIN;
std::map<LLViewHandle,LLView*> LLView::sViewHandleMap;

S32		LLViewHandle::sNextID = 0;
LLViewHandle LLViewHandle::sDeadHandle;

#if LL_DEBUG
BOOL LLView::sIsDrawing = FALSE;
#endif

//static
LLView* LLView::getViewByHandle(LLViewHandle handle)
{
	if (handle == LLViewHandle::sDeadHandle)
	{
		return NULL;
	}
	std::map<LLViewHandle,LLView*>::iterator iter = sViewHandleMap.find(handle);
	if (iter != sViewHandleMap.end())
	{
		return iter->second;
	}
	else
	{
		return NULL;
	}
}

//static
BOOL LLView::deleteViewByHandle(LLViewHandle handle)
{
	std::map<LLViewHandle,LLView*>::iterator iter = sViewHandleMap.find(handle);
	if (iter != sViewHandleMap.end())
	{
		delete iter->second; // will remove from map
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

LLView::LLView() :
	mParentView(NULL),
	mReshapeFlags(FOLLOWS_NONE),
	mDefaultTabGroup(0),
	mEnabled(TRUE),
	mMouseOpaque(TRUE),
	mSoundFlags(MOUSE_UP), // default to only make sound on mouse up
	mSaveToXML(TRUE),
	mIsFocusRoot(FALSE),
	mLastVisible(TRUE),
	mSpanChildren(FALSE),
	mVisible(TRUE),
	mHidden(FALSE),
	mNextInsertionOrdinal(0)
{
	mViewHandle.init();
	sViewHandleMap[mViewHandle] = this;
}

LLView::LLView(const LLString& name, BOOL mouse_opaque) :
	mParentView(NULL),
	mName(name),
	mReshapeFlags(FOLLOWS_NONE),
	mDefaultTabGroup(0),
	mEnabled(TRUE),
	mMouseOpaque(mouse_opaque),
	mSoundFlags(MOUSE_UP), // default to only make sound on mouse up
	mSaveToXML(TRUE),
	mIsFocusRoot(FALSE),
	mLastVisible(TRUE),
	mSpanChildren(FALSE),
	mVisible(TRUE),
	mHidden(FALSE),
	mNextInsertionOrdinal(0)
{
	mViewHandle.init();
	sViewHandleMap[mViewHandle] = this;
}


LLView::LLView(
	const LLString& name, const LLRect& rect, BOOL mouse_opaque, U32 reshape) :
	mParentView(NULL),
	mName(name),
	mRect(rect),
	mReshapeFlags(reshape),
	mDefaultTabGroup(0),
	mEnabled(TRUE),
	mMouseOpaque(mouse_opaque),
	mSoundFlags(MOUSE_UP), // default to only make sound on mouse up
	mSaveToXML(TRUE),
	mIsFocusRoot(FALSE),
	mLastVisible(TRUE),
	mSpanChildren(FALSE),
	mVisible(TRUE),
	mHidden(FALSE),
	mNextInsertionOrdinal(0)
{
	mViewHandle.init();
	sViewHandleMap[mViewHandle] = this;
}


LLView::~LLView()
{
	//llinfos << "Deleting view " << mName << ":" << (void*) this << llendl;
// 	llassert(LLView::sIsDrawing == FALSE);
	if( gFocusMgr.getKeyboardFocus() == this )
	{
		llwarns << "View holding keyboard focus deleted: " << getName() << ".  Keyboard focus removed." << llendl;
		gFocusMgr.removeKeyboardFocusWithoutCallback( this );
	}

	if( hasMouseCapture() )
	{
		llwarns << "View holding mouse capture deleted: " << getName() << ".  Mouse capture removed." << llendl;
		gFocusMgr.removeMouseCaptureWithoutCallback( this );
	}

	sViewHandleMap.erase(mViewHandle);

	deleteAllChildren();

	if (mParentView != NULL)
	{
		mParentView->removeChild(this);
	}

	dispatch_list_t::iterator itor;
	for (itor = mDispatchList.begin(); itor != mDispatchList.end(); ++itor)
	{
		(*itor).second->clearDispatchers();
		delete (*itor).second;
	}

	std::for_each(mFloaterControls.begin(), mFloaterControls.end(),
				  DeletePairedPointer());
}

// virtual
BOOL LLView::isView()
{
	return TRUE;
}

// virtual
BOOL LLView::isCtrl() const
{
	return FALSE;
}

// virtual
BOOL LLView::isPanel()
{
	return FALSE;
}

void LLView::setMouseOpaque(BOOL b)
{
	mMouseOpaque = b;
}

void LLView::setToolTip(const LLString& msg)
{
	mToolTipMsg = msg;
}

// virtual
void LLView::setRect(const LLRect& rect)
{
	mRect = rect;
}


void LLView::setFollows(U32 flags)			
{
	mReshapeFlags = flags;
}

void LLView::setFollowsNone() 				
{
	mReshapeFlags = FOLLOWS_NONE; 
}

void LLView::setFollowsLeft()				
{
	mReshapeFlags |= FOLLOWS_LEFT; 
}

void LLView::setFollowsTop()					
{
	mReshapeFlags |= FOLLOWS_TOP; 
}

void LLView::setFollowsRight()				
{
	mReshapeFlags |= FOLLOWS_RIGHT;
}

void LLView::setFollowsBottom()				
{
	mReshapeFlags |= FOLLOWS_BOTTOM;
}

void LLView::setFollowsAll()					
{
	mReshapeFlags |= FOLLOWS_ALL;
}

void LLView::setSoundFlags(U8 flags)         
{
	mSoundFlags = flags;
}

void LLView::setName(LLString name)			
{
	mName = name;
}

void LLView::setSpanChildren( BOOL span_children ) 
{
	mSpanChildren = span_children; updateRect();
}

const LLString& LLView::getToolTip()
{
	return mToolTipMsg;
}

// virtual
const LLString& LLView::getName() const
{
	static const LLString unnamed("(no name)");
	return mName.empty() ? unnamed : mName;
}

void LLView::sendChildToFront(LLView* child)
{
	if (child->mParentView == this) 
	{
		mChildList.remove( child );
		mChildList.push_front(child);
	}
}

void LLView::sendChildToBack(LLView* child)
{
	if (child->mParentView == this) 
	{
		mChildList.remove( child );
		mChildList.push_back(child);
	}
}

void LLView::moveChildToFrontOfTabGroup(LLUICtrl* child)
{
	if(mCtrlOrder.find(child) != mCtrlOrder.end())
	{
		mCtrlOrder[child].second = -1 * mNextInsertionOrdinal++;
	}
}

void LLView::addChild(LLView* child, S32 tab_group)
{
	// remove from current parent
	if (child->mParentView) 
	{
		child->mParentView->removeChild(child);
	}

	// add to front of child list, as normal
	mChildList.push_front(child);

	// add to ctrl list if is LLUICtrl
	if (child->isCtrl())
	{
		// controls are stored in reverse order from render order
		addCtrlAtEnd((LLUICtrl*) child, tab_group);
	}

	child->mParentView = this;
	updateRect();
}


void LLView::addChildAtEnd(LLView* child, S32 tab_group)
{
	// remove from current parent
	if (child->mParentView) 
	{
		child->mParentView->removeChild(child);
	}

	// add to back of child list
	mChildList.push_back(child);

	// add to ctrl list if is LLUICtrl
	if (child->isCtrl())
	{
		// controls are stored in reverse order from render order
		addCtrl((LLUICtrl*) child, tab_group);
	}
	
	child->mParentView = this;
	updateRect();
}

// remove the specified child from the view, and set it's parent to NULL.
void LLView::removeChild( LLView* child )
{
	if (child->mParentView == this) 
	{
		mChildList.remove( child );
		child->mParentView = NULL;
	}
	else
	{
		llerrs << "LLView::removeChild called with non-child" << llendl;
	}

	if (child->isCtrl())
	{
		removeCtrl((LLUICtrl*)child);
	}
}

void LLView::addCtrlAtEnd(LLUICtrl* ctrl, S32 tab_group)
{
	mCtrlOrder.insert(tab_order_pair_t(ctrl,
								tab_order_t(tab_group, mNextInsertionOrdinal++)));
}

void LLView::addCtrl( LLUICtrl* ctrl, S32 tab_group)
{
	// add to front of list by using negative ordinal, which monotonically increases
	mCtrlOrder.insert(tab_order_pair_t(ctrl,
								tab_order_t(tab_group, -1 * mNextInsertionOrdinal++)));
}

void LLView::removeCtrl(LLUICtrl* ctrl)
{
	child_tab_order_t::iterator found = mCtrlOrder.find(ctrl);
	if(found != mCtrlOrder.end())
	{
		mCtrlOrder.erase(found);
	}
}

S32 LLView::getDefaultTabGroup() const { return mDefaultTabGroup; }

LLView::ctrl_list_t LLView::getCtrlList() const
{
	ctrl_list_t controls;
	for(child_list_const_iter_t iter = mChildList.begin();
		iter != mChildList.end();
		iter++)
	{
		if((*iter)->isCtrl())
		{
			controls.push_back(static_cast<LLUICtrl*>(*iter));
		}
	}
	return controls;
}

LLView::ctrl_list_t LLView::getCtrlListSorted() const
{
	ctrl_list_t controls = getCtrlList();
	std::sort(controls.begin(), controls.end(), LLCompareByTabOrder(mCtrlOrder));
	return controls;
}

LLCompareByTabOrder::LLCompareByTabOrder(LLView::child_tab_order_t order): mTabOrder(order) {}

bool LLCompareByTabOrder::compareTabOrders(const LLView::tab_order_t & a, const LLView::tab_order_t & b) const
{
	return a < b;
}

// This method compares two LLViews by the tab order specified in the comparator object.  The
// code for this is a little convoluted because each argument can have four states:
// 1) not a control, 2) a control but not in the tab order, 3) a control in the tab order, 4) null
bool LLCompareByTabOrder::operator() (const LLView* const a, const LLView* const b) const
{
	S32 a_score = 0, b_score = 0;
	if(a) a_score--;
	if(b) b_score--;
	if(a && a->isCtrl()) a_score--;
	if(b && b->isCtrl()) b_score--;
	if(a_score == -2 && b_score == -2)
	{
		const LLUICtrl * const a_ctrl = static_cast<const LLUICtrl* const>(a);
		const LLUICtrl * const b_ctrl = static_cast<const LLUICtrl* const>(b);
		LLView::child_tab_order_const_iter_t a_found = mTabOrder.find(a_ctrl), b_found = mTabOrder.find(b_ctrl);
		if(a_found != mTabOrder.end()) a_score--;
		if(b_found != mTabOrder.end()) b_score--;
		if(a_score == -3 && b_score == -3)
		{
			// whew!  Once we're in here, they're both in the tab order, and we can compare based on that
			return compareTabOrders(a_found->second, b_found->second);
		}
	}
	return (a_score == b_score) ? a < b : a_score < b_score;
}

BOOL LLView::isInVisibleChain() const
{
	const LLView* cur_view = this;
	while(cur_view)
	{
		if (!cur_view->getVisible())
		{
			return FALSE;
		}
		cur_view = cur_view->getParent();
	}
	return TRUE;
}

BOOL LLView::isInEnabledChain() const
{
	const LLView* cur_view = this;
	while(cur_view)
	{
		if (!cur_view->getEnabled())
		{
			return FALSE;
		}
		cur_view = cur_view->getParent();
	}
	return TRUE;
}

BOOL LLView::isFocusRoot() const
{
	return mIsFocusRoot;
}

LLView* LLView::findRootMostFocusRoot() 
{
	LLView* focus_root = NULL;
	LLView* next_view = this;
	while(next_view)
	{
		if (next_view->isFocusRoot())
		{
			focus_root = next_view;
		}
		next_view = next_view->getParent();
	}
	return focus_root;
}

BOOL LLView::canFocusChildren() const
{
	return TRUE;
}

BOOL LLView::focusNextRoot()
{
	LLView::child_list_t result = LLView::getFocusRootsQuery().run(this);
	return LLView::focusNext(result);
}

BOOL LLView::focusPrevRoot()
{
	LLView::child_list_t result = LLView::getFocusRootsQuery().run(this);
	return LLView::focusPrev(result);
}

BOOL LLView::focusNextItem(BOOL text_fields_only)
{
	// this assumes that this method is called on the focus root.
	LLCtrlQuery query = LLView::getTabOrderQuery();
	if(text_fields_only || LLUI::sConfigGroup->getBOOL("TabToTextFieldsOnly"))
	{
		query.addPreFilter(LLUICtrl::LLTextInputFilter::getInstance());
	}
	LLView::child_list_t result = query(this);
	return LLView::focusNext(result);
}

BOOL LLView::focusPrevItem(BOOL text_fields_only)
{
	// this assumes that this method is called on the focus root.
	LLCtrlQuery query = LLView::getTabOrderQuery();
	if(text_fields_only || LLUI::sConfigGroup->getBOOL("TabToTextFieldsOnly"))
	{
		query.addPreFilter(LLUICtrl::LLTextInputFilter::getInstance());
	}
	LLView::child_list_t result = query(this);
	return LLView::focusPrev(result);
}


// static
BOOL LLView::focusNext(LLView::child_list_t & result)
{
	LLView::child_list_iter_t focused = result.end();
	for(LLView::child_list_iter_t iter = result.begin();
		iter != result.end();
		++iter)
	{
		if(gFocusMgr.childHasKeyboardFocus(*iter))
		{
			focused = iter;
			break;
		}
	}
	LLView::child_list_iter_t next = focused;
	next = (next == result.end()) ? result.begin() : ++next;
	while(next != focused)
	{
		// wrap around to beginning if necessary
		if(next == result.end())
		{
			next = result.begin();
		}
		if((*next)->isCtrl())
		{
			LLUICtrl * ctrl = static_cast<LLUICtrl*>(*next);
			ctrl->setFocus(TRUE);
			ctrl->onTabInto();  
			gFocusMgr.triggerFocusFlash();
			return TRUE;
		}
		++next;
	}
	return FALSE;
}

// static
BOOL LLView::focusPrev(LLView::child_list_t & result)
{
	LLView::child_list_reverse_iter_t focused = result.rend();
	for(LLView::child_list_reverse_iter_t iter = result.rbegin();
		iter != result.rend();
		++iter)
	{
		if(gFocusMgr.childHasKeyboardFocus(*iter))
		{
			focused = iter;
			break;
		}
	}
	LLView::child_list_reverse_iter_t next = focused;
	next = (next == result.rend()) ? result.rbegin() : ++next;
	while(next != focused)
	{
		// wrap around to beginning if necessary
		if(next == result.rend())
		{
			next = result.rbegin();
		}
		if((*next)->isCtrl())
		{
			LLUICtrl * ctrl = static_cast<LLUICtrl*>(*next);
			if (!ctrl->hasFocus())
			{
				ctrl->setFocus(TRUE);
				ctrl->onTabInto();  
				gFocusMgr.triggerFocusFlash();
			}
			return TRUE;
		}
		++next;
	}
	return FALSE;
}

BOOL LLView::focusFirstItem(BOOL prefer_text_fields)
{
	// search for text field first
	if(prefer_text_fields)
	{
		LLCtrlQuery query = LLView::getTabOrderQuery();
		query.addPreFilter(LLUICtrl::LLTextInputFilter::getInstance());
		LLView::child_list_t result = query(this);
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
	LLView::child_list_t result = LLView::getTabOrderQuery().run(this);
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

BOOL LLView::focusLastItem(BOOL prefer_text_fields)
{
	// search for text field first
	if(prefer_text_fields)
	{
		LLCtrlQuery query = LLView::getTabOrderQuery();
		query.addPreFilter(LLUICtrl::LLTextInputFilter::getInstance());
		LLView::child_list_t result = query(this);
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
	LLView::child_list_t result = LLView::getTabOrderQuery().run(this);
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



// delete all children. Override this function if you need to
// perform any extra clean up such as cached pointers to selected
// children, etc.
void LLView::deleteAllChildren()
{
	// clear out the control ordering
	mCtrlOrder.clear();

	while (!mChildList.empty())
	{
		LLView* viewp = mChildList.front();
		delete viewp; // will remove the child from mChildList
	}
}

void LLView::setAllChildrenEnabled(BOOL b)
{
	for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
	{
		LLView* viewp = *child_it;
		viewp->setEnabled(b);
	}
}

// virtual
void LLView::setTentative(BOOL b)
{
}

// virtual
BOOL LLView::getTentative() const
{
	return FALSE;
}

// virtual
void LLView::setEnabled(BOOL enabled)
{
	mEnabled = enabled;
}

// virtual
void LLView::setVisible(BOOL visible)
{
	if( !visible && (gFocusMgr.getTopCtrl() == this) )
	{
		gFocusMgr.setTopCtrl( NULL );
	}

	if ( mVisible != visible )
	{
		// tell all children of this view that the visibility may have changed
		onVisibilityChange ( visible );
	}

	mVisible = visible;
}

// virtual
void LLView::setHidden(BOOL hidden)
{
	mHidden = hidden;
}

// virtual
BOOL LLView::setLabelArg(const LLString& key, const LLString& text)
{
	return FALSE;
}

void LLView::onVisibilityChange ( BOOL curVisibilityIn )
{
	for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
	{
		LLView* viewp = *child_it;
		// only views that are themselves visible will have their overall visibility affected by their ancestors
		if (viewp->getVisible())
		{
			viewp->onVisibilityChange ( curVisibilityIn );
		}
	}
}

// virtual
void LLView::translate(S32 x, S32 y)
{
	mRect.translate(x, y);
}

// virtual
BOOL LLView::canSnapTo(LLView* other_view)
{
	return other_view->getVisible();
}

// virtual
void LLView::snappedTo(LLView* snap_view)
{
}

BOOL LLView::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = childrenHandleHover( x, y, mask ) != NULL;
	if( !handled && mMouseOpaque && pointInView( x, y ) )
	{
		LLUI::sWindow->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << llendl;
		handled = TRUE;
	}

	return handled;
}

LLString LLView::getShowNamesToolTip()
{
	LLView* view = getParent();
	LLString name;
	LLString tool_tip = mName;

	while (view)
	{
		name = view->getName();

		if (name == "root") break;

		if (view->getToolTip().find(".xml") != LLString::npos)
		{
			tool_tip = view->getToolTip() + "/" +  tool_tip;
			break;
		}
		else
		{
			tool_tip = view->getName() + "/" +  tool_tip;
		}

		view = view->getParent();
	}

	return "/" + tool_tip;
}


BOOL LLView::handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect_screen)
{
	BOOL handled = FALSE;

    LLString tool_tip;

	if ( getVisible() && getEnabled())
	{
		for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			S32 local_x = x - viewp->mRect.mLeft;
			S32 local_y = y - viewp->mRect.mBottom;
			if( viewp->handleToolTip(local_x, local_y, msg, sticky_rect_screen ) )
			{
				handled = TRUE;
				break;
			}
		}

		if (LLUI::sShowXUINames && (mToolTipMsg.find(".xml", 0) == LLString::npos) && 
			(mName.find("Drag", 0) == LLString::npos))
		{
			tool_tip = getShowNamesToolTip();
		}
		else
		{
			tool_tip = mToolTipMsg;
		}
		


		BOOL showNamesTextBox = LLUI::sShowXUINames && (getWidgetType() == WIDGET_TYPE_TEXT_BOX);

		if( !handled && (mMouseOpaque || showNamesTextBox) && pointInView( x, y ) && !tool_tip.empty())
		{

			msg = tool_tip;

			// Convert rect local to screen coordinates
			localPointToScreen(
				0, 0,
				&(sticky_rect_screen->mLeft), &(sticky_rect_screen->mBottom) );
			localPointToScreen(
				mRect.getWidth(), mRect.getHeight(),
				&(sticky_rect_screen->mRight), &(sticky_rect_screen->mTop) );
			
			handled = TRUE;
		}
	}

	return handled;
}

BOOL LLView::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	BOOL handled = FALSE;

	if( called_from_parent )
	{
		// Downward traversal
		if (getVisible() && mEnabled)
		{
			handled = childrenHandleKey( key, mask ) != NULL;
		}
	}

	// JC: Must pass to disabled views, since they could have
	// keyboard focus, which requires the escape key to exit.
	if (!handled && getVisible())
	{
		handled = handleKeyHere( key, mask, called_from_parent );
		if (handled && LLView::sDebugKeys)
		{
			llinfos << "Key handled by " << getName() << llendl;
		}
	}

	if( !handled && !called_from_parent)
	{
		if (mIsFocusRoot)
		{
			// stop processing at focus root
			handled = FALSE;
		}
		else if (mParentView)
		{
			// Upward traversal
			handled = mParentView->handleKey( key, mask, FALSE );
		}
	}
	return handled;
}

// Called from handleKey()
// Handles key in this object.  Checking parents and children happens in handleKey()
BOOL LLView::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	return FALSE;
}

BOOL LLView::handleUnicodeChar(llwchar uni_char, BOOL called_from_parent)
{
	BOOL handled = FALSE;

	if( called_from_parent )
	{
		// Downward traversal
		if (getVisible() && mEnabled)
		{
			handled = childrenHandleUnicodeChar( uni_char ) != NULL;
		}
	}

	if (!handled && getVisible())
	{
		handled = handleUnicodeCharHere(uni_char, called_from_parent);
		if (handled && LLView::sDebugKeys)
		{
			llinfos << "Unicode key handled by " << getName() << llendl;
		}
	}


	if (!handled && !called_from_parent)
	{
		if (mIsFocusRoot)
		{
			// stop processing at focus root
			handled = FALSE;
		}
		else if(mParentView)
		{
			// Upward traversal
			handled = mParentView->handleUnicodeChar(uni_char, FALSE);
		}
	}

	return handled;
}


BOOL LLView::handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent )
{
	return FALSE;
}


BOOL LLView::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
							   EDragAndDropType cargo_type, void* cargo_data,
							   EAcceptance* accept,
							   LLString& tooltip_msg)
{
	// CRO this is an experiment to allow drag and drop into object inventory based on the DragAndDrop tool's permissions rather than the parent
	BOOL handled = childrenHandleDragAndDrop( x, y, mask, drop,
											cargo_type,
											cargo_data,
											accept,
											tooltip_msg) != NULL;
	if( !handled && mMouseOpaque )
	{
		*accept = ACCEPT_NO;
		handled = TRUE;
		lldebugst(LLERR_USER_INPUT) << "dragAndDrop handled by LLView " << getName() << llendl;
	}

	return handled;
}

LLView* LLView::childrenHandleDragAndDrop(S32 x, S32 y, MASK mask,
									   BOOL drop,
									   EDragAndDropType cargo_type,
									   void* cargo_data,
									   EAcceptance* accept,
									   LLString& tooltip_msg)
{
	LLView* handled_view = FALSE;
	// CRO this is an experiment to allow drag and drop into object inventory based on the DragAndDrop tool's permissions rather than the parent
	if( getVisible() )
//	if( getVisible() && mEnabled )
	{
		for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			S32 local_x = x - viewp->mRect.mLeft;
			S32 local_y = y - viewp->mRect.mBottom;
			if( viewp->pointInView(local_x, local_y) && 
				viewp->getVisible() &&
				viewp->mEnabled &&
				viewp->handleDragAndDrop(local_x, local_y, mask, drop,
										 cargo_type,
										 cargo_data,
										 accept,
										 tooltip_msg))
			{
				handled_view = viewp;
				break;
			}
		}
	}
	return handled_view;
}

void LLView::onMouseCaptureLost()
{
}

BOOL LLView::hasMouseCapture()
{ 
	return gFocusMgr.getMouseCapture() == this; 
}

BOOL LLView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = childrenHandleMouseUp( x, y, mask ) != NULL;
	if( !handled && mMouseOpaque )
	{
		handled = TRUE;
	}
	return handled;
}

BOOL LLView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	LLView* handled_view = childrenHandleMouseDown( x, y, mask );
	BOOL handled = (handled_view != NULL);
	if( !handled && mMouseOpaque )
	{
		handled = TRUE;
		handled_view = this;
	}

	// HACK If we're editing UI, select the leaf view that ate the click.
	if (sEditingUI && handled_view)
	{
		// need to find leaf views, big hack
		EWidgetType type = handled_view->getWidgetType();
		if (type == WIDGET_TYPE_BUTTON
			|| type == WIDGET_TYPE_LINE_EDITOR
			|| type == WIDGET_TYPE_TEXT_EDITOR
			|| type == WIDGET_TYPE_TEXT_BOX)
		{
			sEditingUIView = handled_view;
		}
	}

	return handled;
}

BOOL LLView::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	BOOL handled = childrenHandleDoubleClick( x, y, mask ) != NULL;
	if( !handled && mMouseOpaque )
	{
		handleMouseDown(x, y, mask);
		handled = TRUE;
	}
	return handled;
}

BOOL LLView::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	BOOL handled = FALSE;
	if( getVisible() && mEnabled )
	{
		handled = childrenHandleScrollWheel( x, y, clicks ) != NULL;
		if( !handled && mMouseOpaque )
		{
			handled = TRUE;
		}
	}
	return handled;
}

BOOL LLView::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = childrenHandleRightMouseDown( x, y, mask ) != NULL;
	if( !handled && mMouseOpaque )
	{
		handled = TRUE;
	}
	return handled;
}

BOOL LLView::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = childrenHandleRightMouseUp( x, y, mask ) != NULL;
	if( !handled && mMouseOpaque )
	{
		handled = TRUE;
	}
	return handled;
}

LLView* LLView::childrenHandleScrollWheel(S32 x, S32 y, S32 clicks)
{
	LLView* handled_view = NULL;
	if (getVisible() && mEnabled )
	{
		for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			S32 local_x = x - viewp->mRect.mLeft;
			S32 local_y = y - viewp->mRect.mBottom;
			if (viewp->pointInView(local_x, local_y) && 
				viewp->handleScrollWheel( local_x, local_y, clicks ))
			{
				if (sDebugMouseHandling)
				{
					sMouseHandlerMessage = LLString("->") + viewp->mName.c_str() + sMouseHandlerMessage;
				}

				handled_view = viewp;
				break;
			}
		}
	}
	return handled_view;
}

LLView* LLView::childrenHandleHover(S32 x, S32 y, MASK mask)
{
	LLView* handled_view = NULL;
	if (getVisible() && mEnabled )
	{
		for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			S32 local_x = x - viewp->mRect.mLeft;
			S32 local_y = y - viewp->mRect.mBottom;
			if(viewp->pointInView(local_x, local_y) &&
				viewp->getVisible() &&
				viewp->getEnabled() &&
				viewp->handleHover(local_x, local_y, mask) )
			{
				if (sDebugMouseHandling)
				{
					sMouseHandlerMessage = LLString("->") + viewp->mName.c_str() + sMouseHandlerMessage;
				}

				handled_view = viewp;
				break;
			}
		}
	}
	return handled_view;
}

// Called during downward traversal
LLView* LLView::childrenHandleKey(KEY key, MASK mask)
{
	LLView* handled_view = NULL;

	if ( getVisible() && mEnabled )
	{
		for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			if (viewp->handleKey(key, mask, TRUE))
			{
				if (LLView::sDebugKeys)
				{
					llinfos << "Key handled by " << viewp->getName() << llendl;
				}
				handled_view = viewp;
				break;
			}
		}
	}

	return handled_view;
}

// Called during downward traversal
LLView* LLView::childrenHandleUnicodeChar(llwchar uni_char)
{
	LLView* handled_view = NULL;

	if ( getVisible() && mEnabled )
	{
		for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			if (viewp->handleUnicodeChar(uni_char, TRUE))
			{
				if (LLView::sDebugKeys)
				{
					llinfos << "Unicode character handled by " << viewp->getName() << llendl;
				}
				handled_view = viewp;
				break;
			}
		}
	}

	return handled_view;
}

LLView* LLView::childrenHandleMouseDown(S32 x, S32 y, MASK mask)
{
	LLView* handled_view = NULL;

	for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
	{
		LLView* viewp = *child_it;
		S32 local_x = x - viewp->mRect.mLeft;
		S32 local_y = y - viewp->mRect.mBottom;

		if (viewp->pointInView(local_x, local_y) && 
			viewp->getVisible() && 
			viewp->mEnabled && 
			viewp->handleMouseDown( local_x, local_y, mask ))
		{
			if (sDebugMouseHandling)
			{
				sMouseHandlerMessage = LLString("->") + viewp->mName.c_str() + sMouseHandlerMessage;
			}
			handled_view = viewp;
			break;
		}
	}
	return handled_view;
}

LLView* LLView::childrenHandleRightMouseDown(S32 x, S32 y, MASK mask)
{
	LLView* handled_view = NULL;

	if (getVisible() && mEnabled )
	{
		for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			S32 local_x = x - viewp->mRect.mLeft;
			S32 local_y = y - viewp->mRect.mBottom;
			if (viewp->pointInView(local_x, local_y) &&
				viewp->getVisible() &&
				viewp->mEnabled &&
				viewp->handleRightMouseDown( local_x, local_y, mask ))
			{
				if (sDebugMouseHandling)
				{
					sMouseHandlerMessage = LLString("->") + viewp->mName.c_str() + sMouseHandlerMessage;
				}
				handled_view = viewp;
				break;
			}
		}
	}
	return handled_view;
}

LLView* LLView::childrenHandleDoubleClick(S32 x, S32 y, MASK mask)
{
	LLView* handled_view = NULL;

	if (getVisible() && mEnabled )
	{
		for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			S32 local_x = x - viewp->mRect.mLeft;
			S32 local_y = y - viewp->mRect.mBottom;
			if (viewp->pointInView(local_x, local_y) &&
				viewp->getVisible() &&
				viewp->mEnabled &&
				viewp->handleDoubleClick( local_x, local_y, mask ))
			{
				if (sDebugMouseHandling)
				{
					sMouseHandlerMessage = LLString("->") + viewp->mName.c_str() + sMouseHandlerMessage;
				}
				handled_view = viewp;
				break;
			}
		}
	}
	return handled_view;
}

LLView* LLView::childrenHandleMouseUp(S32 x, S32 y, MASK mask)
{
	LLView* handled_view = NULL;
	if( getVisible() && mEnabled )
	{
		for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			S32 local_x = x - viewp->mRect.mLeft;
			S32 local_y = y - viewp->mRect.mBottom;
			if (!viewp->pointInView(local_x, local_y))
				continue;
			if (!viewp->getVisible())
				continue;
			if (!viewp->mEnabled)
				continue;
			if (viewp->handleMouseUp( local_x, local_y, mask ))
			{
				if (sDebugMouseHandling)
				{
					sMouseHandlerMessage = LLString("->") + viewp->mName.c_str() + sMouseHandlerMessage;
				}
				handled_view = viewp;
				break;
			}
		}
	}
	return handled_view;
}

LLView* LLView::childrenHandleRightMouseUp(S32 x, S32 y, MASK mask)
{
	LLView* handled_view = NULL;
	if( getVisible() && mEnabled )
	{
		for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			S32 local_x = x - viewp->mRect.mLeft;
			S32 local_y = y - viewp->mRect.mBottom;
			if (viewp->pointInView(local_x, local_y) &&
				viewp->getVisible() &&
				viewp->mEnabled &&
				viewp->handleRightMouseUp( local_x, local_y, mask ))
			{
				if (sDebugMouseHandling)
				{
					sMouseHandlerMessage = LLString("->") + viewp->mName.c_str() + sMouseHandlerMessage;
				}
				handled_view = viewp;
				break;
			}
		}
	}
	return handled_view;
}


void LLView::draw()
{
	if (getVisible())
	{
		if (sDebugRects)
		{
			drawDebugRect();

			// Check for bogus rectangle
			if (mRect.mRight <= mRect.mLeft
				|| mRect.mTop <= mRect.mBottom)
			{
				llwarns << "Bogus rectangle for " << getName() << " with " << mRect << llendl;
			}
		}

		LLRect rootRect = getRootView()->getRect();
		LLRect screenRect;

		// draw focused control on top of everything else
		LLView* focus_view = gFocusMgr.getKeyboardFocus();
		if (focus_view && focus_view->getParent() != this)
		{
			focus_view = NULL;
		}

		for (child_list_reverse_iter_t child_iter = mChildList.rbegin(); child_iter != mChildList.rend(); ++child_iter)
		{
			LLView *viewp = *child_iter;
			++sDepth;

			if (viewp->getVisible() && viewp != focus_view)
			{
				// Only draw views that are within the root view
				localRectToScreen(viewp->getRect(),&screenRect);
				if ( rootRect.rectInRect(&screenRect) )
				{
					glMatrixMode(GL_MODELVIEW);
					LLUI::pushMatrix();
					{
						LLUI::translate((F32)viewp->getRect().mLeft, (F32)viewp->getRect().mBottom, 0.f);
						viewp->draw();
					}
					LLUI::popMatrix();
				}
			}

			--sDepth;
		}

		if (focus_view && focus_view->getVisible())
		{
			drawChild(focus_view);
		}

		// HACK
		if (sEditingUI && this == sEditingUIView)
		{
			drawDebugRect();
		}
	}
}

//Draw a box for debugging.
void LLView::drawDebugRect()
{
	// drawing solids requires texturing be disabled
	LLGLSNoTexture no_texture;

	// draw red rectangle for the border
	LLColor4 border_color(0.f, 0.f, 0.f, 1.f);
	if (sEditingUI)
	{
		border_color.mV[0] = 1.f;
	}
	else
	{
		border_color.mV[sDepth%3] = 1.f;
	}

	glColor4fv( border_color.mV );

	glBegin(GL_LINES);
		glVertex2i(0, mRect.getHeight() - 1);
		glVertex2i(0, 0);

		glVertex2i(0, 0);
		glVertex2i(mRect.getWidth() - 1, 0);

		glVertex2i(mRect.getWidth() - 1, 0);
		glVertex2i(mRect.getWidth() - 1, mRect.getHeight() - 1);

		glVertex2i(mRect.getWidth() - 1, mRect.getHeight() - 1);
		glVertex2i(0, mRect.getHeight() - 1);
	glEnd();

	// Draw the name if it's not a leaf node
	if (mChildList.size() && !sEditingUI)
	{
		//char temp[256];
		S32 x, y;
		glColor4fv( border_color.mV );
		x = mRect.getWidth()/2;
		y = mRect.getHeight()/2;
		LLString debug_text = llformat("%s (%d x %d)", getName().c_str(),
									   mRect.getWidth(), mRect.getHeight());
		LLFontGL::sSansSerifSmall->renderUTF8(debug_text, 0, (F32)x, (F32)y, border_color,
											  LLFontGL::HCENTER, LLFontGL::BASELINE, LLFontGL::NORMAL,
											  S32_MAX, S32_MAX, NULL, FALSE);
	}
}

void LLView::drawChild(LLView* childp, S32 x_offset, S32 y_offset)
{
	if (childp && childp->getParent() == this)
	{
		++sDepth;

		if (childp->getVisible())
		{
			glMatrixMode(GL_MODELVIEW);
			LLUI::pushMatrix();
			{
				LLUI::translate((F32)childp->getRect().mLeft + x_offset, (F32)childp->getRect().mBottom + y_offset, 0.f);
				childp->draw();
			}
			LLUI::popMatrix();
		}

		--sDepth;
	}
}


void LLView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	// make sure this view contains all its children
	updateRect();

	// compute how much things changed and apply reshape logic to children
	S32 delta_width = width - mRect.getWidth();
	S32 delta_height = height - mRect.getHeight();

	if (delta_width || delta_height || sForceReshape)
	{
		// adjust our rectangle
		mRect.mRight = mRect.mLeft + width;
		mRect.mTop = mRect.mBottom + height;

		// move child views according to reshape flags
		for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			LLRect child_rect( viewp->mRect );

			if (viewp->followsRight() && viewp->followsLeft())
			{
				child_rect.mRight += delta_width;
			}
			else if (viewp->followsRight())
			{
				child_rect.mLeft += delta_width;
				child_rect.mRight += delta_width;
			}
			else if (viewp->followsLeft())
			{
				// left is 0, don't need to adjust coords
			}
			else
			{
				// BUG what to do when we don't follow anyone?
				// for now, same as followsLeft
			}

			if (viewp->followsTop() && viewp->followsBottom())
			{
				child_rect.mTop += delta_height;
			}
			else if (viewp->followsTop())
			{
				child_rect.mTop += delta_height;
				child_rect.mBottom += delta_height;
			}
			else if (viewp->followsBottom())
			{
				// bottom is 0, so don't need to adjust coords
			}
			else
			{
				// BUG what to do when we don't follow?
				// for now, same as bottom
			}

			S32 delta_x = child_rect.mLeft - viewp->mRect.mLeft;
			S32 delta_y = child_rect.mBottom - viewp->mRect.mBottom;
			viewp->translate( delta_x, delta_y );
			viewp->reshape(child_rect.getWidth(), child_rect.getHeight());
		}
	}

	if (!called_from_parent)
	{
		if (mParentView)
		{
			mParentView->reshape(mParentView->getRect().getWidth(), mParentView->getRect().getHeight(), FALSE);
		}
	}
}

LLRect LLView::getRequiredRect()
{
	return mRect;
}

const LLRect LLView::getScreenRect() const
{
	// *FIX: check for one-off error
	LLRect screen_rect;
	localPointToScreen(0, 0, &screen_rect.mLeft, &screen_rect.mBottom);
	localPointToScreen(mRect.getWidth(), mRect.getHeight(), &screen_rect.mRight, &screen_rect.mTop);
	return screen_rect;
}

const LLRect LLView::getLocalRect() const
{
	LLRect local_rect(0, mRect.getHeight(), mRect.getWidth(), 0);
	return local_rect;
}

const LLRect LLView::getLocalSnapRect() const
{
	LLRect local_snap_rect = getSnapRect();
	local_snap_rect.translate(-mRect.mLeft, -mRect.mBottom);
	return local_snap_rect;
}

void LLView::updateRect()
{
	if (mSpanChildren && mChildList.size())
	{
		LLView* first_child = (*mChildList.begin());
		LLRect child_spanning_rect = first_child->mRect;

		for ( child_list_iter_t child_it = ++mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			if (viewp->getVisible())
			{
				child_spanning_rect |=  viewp->mRect;
			}
		}

		S32 translate_x = llmin(0, child_spanning_rect.mLeft);
		S32 translate_y = llmin(0, child_spanning_rect.mBottom);
		S32 new_width	= llmax(mRect.getWidth() + translate_x, child_spanning_rect.getWidth());
		S32 new_height	= llmax(mRect.getHeight() + translate_y, child_spanning_rect.getHeight());

		mRect.setOriginAndSize(mRect.mLeft + translate_x, mRect.mBottom + translate_y, new_width, new_height);

		for ( child_list_iter_t child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* viewp = *child_it;
			viewp->mRect.translate(-translate_x, -translate_y);
		}
	}
}

BOOL LLView::hasAncestor(LLView* parentp)
{
	if (!parentp)
	{
		return FALSE;
	}

	LLView* viewp = getParent();
	while(viewp)
	{
		if (viewp == parentp)
		{
			return TRUE;
		}
		viewp = viewp->getParent();
	}

	return FALSE;
}

//-----------------------------------------------------------------------------

BOOL LLView::childHasKeyboardFocus( const LLString& childname ) const
{
	LLView *child = getChildByName(childname);
	if (child)
	{
		return gFocusMgr.childHasKeyboardFocus(child);
	}
	else
	{
		return FALSE;
	}
}

//-----------------------------------------------------------------------------

BOOL LLView::hasChild(const LLString& childname, BOOL recurse) const
{
	return getChildByName(childname, recurse) ? TRUE : FALSE;
}

//-----------------------------------------------------------------------------
// getChildByName()
//-----------------------------------------------------------------------------
LLView* LLView::getChildByName(const LLString& name, BOOL recurse) const
{
	if(name.empty()) return NULL;
	child_list_const_iter_t child_it;
	// Look for direct children *first*
	for ( child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
	{
		LLView* childp = *child_it;
		if (childp->getName() == name)
		{
			return childp;
		}
	}
	if (recurse)
	{
		// Look inside the child as well.
		for ( child_it = mChildList.begin(); child_it != mChildList.end(); ++child_it)
		{
			LLView* childp = *child_it;
			LLView* viewp = childp->getChildByName(name, recurse);
			if ( viewp )
			{
				return viewp;
			}
		}
	}
	return NULL;
}

// virtual
void LLView::onFocusLost()
{
}

// virtual
void LLView::onFocusReceived()
{
}

// virtual
void LLView::screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const
{
	*local_x = screen_x - mRect.mLeft;
	*local_y = screen_y - mRect.mBottom;

	const LLView* cur = this;
	while( cur->mParentView )
	{
		cur = cur->mParentView;
		*local_x -= cur->mRect.mLeft;
		*local_y -= cur->mRect.mBottom;
	}
}

void LLView::localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const
{
	*screen_x = local_x + mRect.mLeft;
	*screen_y = local_y + mRect.mBottom;

	const LLView* cur = this;
	while( cur->mParentView )
	{
		cur = cur->mParentView;
		*screen_x += cur->mRect.mLeft;
		*screen_y += cur->mRect.mBottom;
	}
}

void LLView::screenRectToLocal(const LLRect& screen, LLRect* local) const
{
	*local = screen;
	local->translate( -mRect.mLeft, -mRect.mBottom );

	const LLView* cur = this;
	while( cur->mParentView )
	{
		cur = cur->mParentView;
		local->translate( -cur->mRect.mLeft, -cur->mRect.mBottom );
	}
}

void LLView::localRectToScreen(const LLRect& local, LLRect* screen) const
{
	*screen = local;
	screen->translate( mRect.mLeft, mRect.mBottom );

	const LLView* cur = this;
	while( cur->mParentView )
	{
		cur = cur->mParentView;
		screen->translate( cur->mRect.mLeft, cur->mRect.mBottom );
	}
}

LLView* LLView::getRootView()
{
	LLView* view = this;
	while( view->mParentView )
	{
		view = view->mParentView;
	}
	return view;
}

//static 
LLWindow* LLView::getWindow(void)
{
	return LLUI::sWindow;
}

// Moves the view so that it is entirely inside of constraint.
// If the view will not fit because it's too big, aligns with the top and left.
// (Why top and left?  That's where the drag bars are for floaters.)
BOOL LLView::translateIntoRect(const LLRect& constraint, BOOL allow_partial_outside )
{
	S32 delta_x = 0;
	S32 delta_y = 0;

	if (allow_partial_outside)
	{
		const S32 KEEP_ONSCREEN_PIXELS = 16;

		if( mRect.mRight - KEEP_ONSCREEN_PIXELS < constraint.mLeft )
		{
			delta_x = constraint.mLeft - (mRect.mRight - KEEP_ONSCREEN_PIXELS);
		}
		else
		if( mRect.mLeft + KEEP_ONSCREEN_PIXELS > constraint.mRight )
		{
			delta_x = constraint.mRight - (mRect.mLeft + KEEP_ONSCREEN_PIXELS);
			delta_x += llmax( 0, mRect.getWidth() - constraint.getWidth() );
		}

		if( mRect.mTop > constraint.mTop )
		{
			delta_y = constraint.mTop - mRect.mTop;
		}
		else
		if( mRect.mTop - KEEP_ONSCREEN_PIXELS < constraint.mBottom )
		{
			delta_y = constraint.mBottom - (mRect.mTop - KEEP_ONSCREEN_PIXELS);
			delta_y -= llmax( 0, mRect.getHeight() - constraint.getHeight() );
		}
	}
	else
	{
		if( mRect.mLeft < constraint.mLeft )
		{
			delta_x = constraint.mLeft - mRect.mLeft;
		}
		else
		if( mRect.mRight > constraint.mRight )
		{
			delta_x = constraint.mRight - mRect.mRight;
			delta_x += llmax( 0, mRect.getWidth() - constraint.getWidth() );
		}

		if( mRect.mTop > constraint.mTop )
		{
			delta_y = constraint.mTop - mRect.mTop;
		}
		else
		if( mRect.mBottom < constraint.mBottom )
		{
			delta_y = constraint.mBottom - mRect.mBottom;
			delta_y -= llmax( 0, mRect.getHeight() - constraint.getHeight() );
		}
	}

	if (delta_x != 0 || delta_y != 0)
	{
		translate(delta_x, delta_y);
		return TRUE;
	}
	return FALSE;
}

BOOL LLView::localPointToOtherView( S32 x, S32 y, S32 *other_x, S32 *other_y, LLView* other_view)
{
	LLView* cur_view = this;
	LLView* root_view = NULL;

	while (cur_view)
	{
		if (cur_view == other_view)
		{
			*other_x = x;
			*other_y = y;
			return TRUE;
		}

		x += cur_view->getRect().mLeft;
		y += cur_view->getRect().mBottom;

		cur_view = cur_view->getParent();
		root_view = cur_view;
	}

	// assuming common root between two views, chase other_view's parents up to root
	cur_view = other_view;
	while (cur_view)
	{
		x -= cur_view->getRect().mLeft;
		y -= cur_view->getRect().mBottom;

		cur_view = cur_view->getParent();

		if (cur_view == root_view)
		{
			*other_x = x;
			*other_y = y;
			return TRUE;
		}
	}

	*other_x = x;
	*other_y = y;
	return FALSE;
}

BOOL LLView::localRectToOtherView( const LLRect& local, LLRect* other, LLView* other_view ) const
{
	LLRect cur_rect = local;
	const LLView* cur_view = this;
	const LLView* root_view = NULL;

	while (cur_view)
	{
		if (cur_view == other_view)
		{
			*other = cur_rect;
			return TRUE;
		}

		cur_rect.translate(cur_view->getRect().mLeft, cur_view->getRect().mBottom);

		cur_view = cur_view->getParent();
		root_view = cur_view;
	}

	// assuming common root between two views, chase other_view's parents up to root
	cur_view = other_view;
	while (cur_view)
	{
		cur_rect.translate(-cur_view->getRect().mLeft, -cur_view->getRect().mBottom);

		cur_view = cur_view->getParent();

		if (cur_view == root_view)
		{
			*other = cur_rect;
			return TRUE;
		}
	}

	*other = cur_rect;
	return FALSE;
}

// virtual
LLXMLNodePtr LLView::getXML(bool save_children) const
{
	const LLString& type_name = getWidgetTag();

	LLXMLNodePtr node = new LLXMLNode(type_name, FALSE);

	node->createChild("name", TRUE)->setStringValue(getName());
	node->createChild("width", TRUE)->setIntValue(getRect().getWidth());
	node->createChild("height", TRUE)->setIntValue(getRect().getHeight());

	LLView* parent = getParent();
	S32 left = mRect.mLeft;
	S32 bottom = mRect.mBottom;
	if (parent) bottom -= parent->getRect().getHeight();

	node->createChild("left", TRUE)->setIntValue(left);
	node->createChild("bottom", TRUE)->setIntValue(bottom);

	U32 follows_flags = getFollows();
	if (follows_flags)
	{
		std::stringstream buffer;
		bool pipe = false;
		if (followsLeft())
		{
			buffer << "left";
			pipe = true;
		}
		if (followsTop())
		{
			if (pipe) buffer << "|";
			buffer << "top";
			pipe = true;
		}
		if (followsRight())
		{
			if (pipe) buffer << "|";
			buffer << "right";
			pipe = true;
		}
		if (followsBottom())
		{
			if (pipe) buffer << "|";
			buffer << "bottom";
		}
		node->createChild("follows", TRUE)->setStringValue(buffer.str());
	}
	// Export all widgets as enabled and visible - code must disable.
	node->createChild("hidden", TRUE)->setBoolValue(mHidden);
	node->createChild("mouse_opaque", TRUE)->setBoolValue(mMouseOpaque );
	if (!mToolTipMsg.empty())
	{
		node->createChild("tool_tip", TRUE)->setStringValue(mToolTipMsg);
	}
	if (mSoundFlags != MOUSE_UP)
	{
		node->createChild("sound_flags", TRUE)->setIntValue((S32)mSoundFlags);
	}

	node->createChild("enabled", TRUE)->setBoolValue(mEnabled);

	if (!mControlName.empty())
	{
		node->createChild("control_name", TRUE)->setStringValue(mControlName);
	}
	return node;
}

// static
void LLView::addColorXML(LLXMLNodePtr node, const LLColor4& color,
							const LLString& xml_name, const LLString& control_name)
{
	if (color != LLUI::sColorsGroup->getColor(control_name))
	{
		node->createChild(xml_name, TRUE)->setFloatValue(4, color.mV);
	}
}

// static
void LLView::saveColorToXML(std::ostream& out, const LLColor4& color,
							const LLString& xml_name, const LLString& control_name,
							const LLString& indent)
{
	if (color != LLUI::sColorsGroup->getColor(control_name))
	{
		out << indent << xml_name << "=\"" 
			<< color.mV[VRED] << ", " 
			<< color.mV[VGREEN] << ", " 
			<< color.mV[VBLUE] << ", " 
			<< color.mV[VALPHA] << "\"\n";
	}
}

//static 
LLString LLView::escapeXML(const LLString& xml, LLString& indent)
{
	LLString ret = indent + "\"" + LLXMLNode::escapeXML(xml);

	//replace every newline with a close quote, new line, indent, open quote
	size_t index = ret.size()-1;
	size_t fnd;
	
	while ((fnd = ret.rfind("\n", index)) != std::string::npos)
	{
		ret.replace(fnd, 1, "\"\n" + indent + "\"");
		index = fnd-1;
	}

	//append close quote
	ret.append("\"");
	
	return ret;	
}

// static
LLWString LLView::escapeXML(const LLWString& xml)
{
	LLWString out;
	for (LLWString::size_type i = 0; i < xml.size(); ++i)
	{
		llwchar c = xml[i];
		switch(c)
		{
		case '"':	out.append(utf8string_to_wstring("&quot;"));	break;
		case '\'':	out.append(utf8string_to_wstring("&apos;"));	break;
		case '&':	out.append(utf8string_to_wstring("&amp;"));		break;
		case '<':	out.append(utf8string_to_wstring("&lt;"));		break;
		case '>':	out.append(utf8string_to_wstring("&gt;"));		break;
		default:	out.push_back(c); break;
		}
	}
	return out;
}

// static
const LLCtrlQuery & LLView::getTabOrderQuery()
{
	static LLCtrlQuery query;
	if(query.getPreFilters().size() == 0) {
		query.addPreFilter(LLVisibleFilter::getInstance());
		query.addPreFilter(LLEnabledFilter::getInstance());
		query.addPreFilter(LLTabStopFilter::getInstance());
		query.addPostFilter(LLUICtrl::LLTabStopPostFilter::getInstance());
	}
	return query;
}

// static
const LLCtrlQuery & LLView::getFocusRootsQuery()
{
	static LLCtrlQuery query;
	if(query.getPreFilters().size() == 0) {
		query.addPreFilter(LLVisibleFilter::getInstance());
		query.addPreFilter(LLEnabledFilter::getInstance());
		query.addPreFilter(LLView::LLFocusRootsFilter::getInstance());
	}
	return query;
}


void	LLView::userSetShape(const LLRect& new_rect)
{
	reshape(new_rect.getWidth(), new_rect.getHeight());
	translate(new_rect.mLeft - mRect.mLeft, new_rect.mBottom - mRect.mBottom);
}

LLView* LLView::findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir,
							 LLView::ESnapType snap_type, S32 threshold, S32 padding)
{
	LLView* snap_view = NULL;

	if (!mParentView)
	{
		new_rect = mRect;
		return snap_view;
	}

	// If the view is near the edge of its parent, snap it to
	// the edge.
	LLRect test_rect = getSnapRect();
	LLRect view_rect = getSnapRect();
	test_rect.stretch(padding);
	view_rect.stretch(padding);

	BOOL snapped_x = FALSE;
	BOOL snapped_y = FALSE;

	LLRect parent_local_snap_rect = mParentView->getLocalSnapRect();

	if (snap_type == SNAP_PARENT || snap_type == SNAP_PARENT_AND_SIBLINGS)
	{
		if (llabs(parent_local_snap_rect.mRight - test_rect.mRight) <= threshold && (parent_local_snap_rect.mRight - test_rect.mRight) * mouse_dir.mX >= 0)
		{
			view_rect.translate(parent_local_snap_rect.mRight - view_rect.mRight, 0);
			snap_view = mParentView;
			snapped_x = TRUE;
		}

		if (llabs(test_rect.mLeft - parent_local_snap_rect.mLeft) <= threshold && test_rect.mLeft * mouse_dir.mX <= 0)
		{
			view_rect.translate(parent_local_snap_rect.mLeft - view_rect.mLeft, 0);
			snap_view = mParentView;
			snapped_x = TRUE;
		}

		if (llabs(test_rect.mBottom - parent_local_snap_rect.mBottom) <= threshold && test_rect.mBottom * mouse_dir.mY <= 0)
		{
			view_rect.translate(0, parent_local_snap_rect.mBottom - view_rect.mBottom);
			snap_view = mParentView;
			snapped_y = TRUE;
		}

		if (llabs(parent_local_snap_rect.mTop - test_rect.mTop) <= threshold && (parent_local_snap_rect.mTop - test_rect.mTop) * mouse_dir.mY >= 0)
		{
			view_rect.translate(0, parent_local_snap_rect.mTop - view_rect.mTop);
			snap_view = mParentView;
			snapped_y = TRUE;
		}
	}
	if (snap_type == SNAP_SIBLINGS || snap_type == SNAP_PARENT_AND_SIBLINGS)
	{
		for ( child_list_const_iter_t child_it = mParentView->getChildList()->begin();
			  child_it != mParentView->getChildList()->end(); ++child_it)
		{
			LLView* siblingp = *child_it;
			// skip self
			if (siblingp == this || !siblingp->getVisible() || !canSnapTo(siblingp))
			{
				continue;
			}

			LLRect sibling_rect = siblingp->getSnapRect();

			if (!snapped_x && llabs(test_rect.mRight - sibling_rect.mLeft) <= threshold && (test_rect.mRight - sibling_rect.mLeft) * mouse_dir.mX <= 0)
			{
				view_rect.translate(sibling_rect.mLeft - view_rect.mRight, 0);
				if (!snapped_y)
				{
					if (llabs(test_rect.mTop - sibling_rect.mTop) <= threshold && (test_rect.mTop - sibling_rect.mTop) * mouse_dir.mY <= 0)
					{
						view_rect.translate(0, sibling_rect.mTop - test_rect.mTop);
						snapped_y = TRUE;
					}
					else if (llabs(test_rect.mBottom - sibling_rect.mBottom) <= threshold && (test_rect.mBottom - sibling_rect.mBottom) * mouse_dir.mY <= 0)
					{
						view_rect.translate(0, sibling_rect.mBottom - test_rect.mBottom);
						snapped_y = TRUE;
					}
				}
				snap_view = siblingp;
				snapped_x = TRUE;
			}

			if (!snapped_x && llabs(test_rect.mLeft - sibling_rect.mRight) <= threshold && (test_rect.mLeft - sibling_rect.mRight) * mouse_dir.mX <= 0)
			{
				view_rect.translate(sibling_rect.mRight - view_rect.mLeft, 0);
				if (!snapped_y)
				{
					if (llabs(test_rect.mTop - sibling_rect.mTop) <= threshold && (test_rect.mTop - sibling_rect.mTop) * mouse_dir.mY <= 0)
					{
						view_rect.translate(0, sibling_rect.mTop - test_rect.mTop);
						snapped_y = TRUE;
					}
					else if (llabs(test_rect.mBottom - sibling_rect.mBottom) <= threshold && (test_rect.mBottom - sibling_rect.mBottom) * mouse_dir.mY <= 0)
					{
						view_rect.translate(0, sibling_rect.mBottom - test_rect.mBottom);
						snapped_y = TRUE;
					}
				}
				snap_view = siblingp;
				snapped_x = TRUE;
			}

			if (!snapped_y && llabs(test_rect.mBottom - sibling_rect.mTop) <= threshold && (test_rect.mBottom - sibling_rect.mTop) * mouse_dir.mY <= 0)
			{
				view_rect.translate(0, sibling_rect.mTop - view_rect.mBottom);
				if (!snapped_x)
				{
					if (llabs(test_rect.mLeft - sibling_rect.mLeft) <= threshold && (test_rect.mLeft - sibling_rect.mLeft) * mouse_dir.mX <= 0)
					{
						view_rect.translate(sibling_rect.mLeft - test_rect.mLeft, 0);
						snapped_x = TRUE;
					}
					else if (llabs(test_rect.mRight - sibling_rect.mRight) <= threshold && (test_rect.mRight - sibling_rect.mRight) * mouse_dir.mX <= 0)
					{
						view_rect.translate(sibling_rect.mRight - test_rect.mRight, 0);
						snapped_x = TRUE;
					}
				}
				snap_view = siblingp;
				snapped_y = TRUE;
			}

			if (!snapped_y && llabs(test_rect.mTop - sibling_rect.mBottom) <= threshold && (test_rect.mTop - sibling_rect.mBottom) * mouse_dir.mY <= 0)
			{
				view_rect.translate(0, sibling_rect.mBottom - view_rect.mTop);
				if (!snapped_x)
				{
					if (llabs(test_rect.mLeft - sibling_rect.mLeft) <= threshold && (test_rect.mLeft - sibling_rect.mLeft) * mouse_dir.mX <= 0)
					{
						view_rect.translate(sibling_rect.mLeft - test_rect.mLeft, 0);
						snapped_x = TRUE;
					}
					else if (llabs(test_rect.mRight - sibling_rect.mRight) <= threshold && (test_rect.mRight - sibling_rect.mRight) * mouse_dir.mX <= 0)
					{
						view_rect.translate(sibling_rect.mRight - test_rect.mRight, 0);
						snapped_x = TRUE;
					}
				}
				snap_view = siblingp;
				snapped_y = TRUE;
			}
			
			if (snapped_x && snapped_y)
			{
				break;
			}
		}
	}

	// shrink actual view rect back down
	view_rect.stretch(-padding);
	new_rect = view_rect;
	return snap_view;
}

LLView*	LLView::findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding)
{
	LLRect snap_rect = getSnapRect();
	S32 snap_pos = 0;
	switch(snap_edge)
	{
	case SNAP_LEFT:
		snap_pos = snap_rect.mLeft;
		break;
	case SNAP_RIGHT:
		snap_pos = snap_rect.mRight;
		break;
	case SNAP_TOP:
		snap_pos = snap_rect.mTop;
		break;
	case SNAP_BOTTOM:
		snap_pos = snap_rect.mBottom;
		break;
	}

	if (!mParentView)
	{
		new_edge_val = snap_pos;
		return NULL;
	}

	LLView* snap_view = NULL;

	// If the view is near the edge of its parent, snap it to
	// the edge.
	LLRect test_rect = snap_rect;
	test_rect.stretch(padding);

	BOOL snapped_x = FALSE;
	BOOL snapped_y = FALSE;

	LLRect parent_local_snap_rect = mParentView->getLocalSnapRect();

	if (snap_type == SNAP_PARENT || snap_type == SNAP_PARENT_AND_SIBLINGS)
	{
		switch(snap_edge)
		{
		case SNAP_RIGHT:
			if (llabs(parent_local_snap_rect.mRight - test_rect.mRight) <= threshold && (parent_local_snap_rect.mRight - test_rect.mRight) * mouse_dir.mX >= 0)
			{
				snap_pos = parent_local_snap_rect.mRight - padding;
				snap_view = mParentView;
				snapped_x = TRUE;
			}
			break;
		case SNAP_LEFT:
			if (llabs(test_rect.mLeft - parent_local_snap_rect.mLeft) <= threshold && test_rect.mLeft * mouse_dir.mX <= 0)
			{
				snap_pos = parent_local_snap_rect.mLeft + padding;
				snap_view = mParentView;
				snapped_x = TRUE;
			}
			break;
		case SNAP_BOTTOM:
			if (llabs(test_rect.mBottom - parent_local_snap_rect.mBottom) <= threshold && test_rect.mBottom * mouse_dir.mY <= 0)
			{
				snap_pos = parent_local_snap_rect.mBottom + padding;
				snap_view = mParentView;
				snapped_y = TRUE;
			}
			break;
		case SNAP_TOP:
			if (llabs(parent_local_snap_rect.mTop - test_rect.mTop) <= threshold && (parent_local_snap_rect.mTop - test_rect.mTop) * mouse_dir.mY >= 0)
			{
				snap_pos = parent_local_snap_rect.mTop - padding;
				snap_view = mParentView;
				snapped_y = TRUE;
			}
			break;
		default:
			llerrs << "Invalid snap edge" << llendl;
		}
	}

	if (snap_type == SNAP_SIBLINGS || snap_type == SNAP_PARENT_AND_SIBLINGS)
	{
		for ( child_list_const_iter_t child_it = mParentView->getChildList()->begin();
			  child_it != mParentView->getChildList()->end(); ++child_it)
		{
			LLView* siblingp = *child_it;
			// skip self
			if (siblingp == this || !siblingp->getVisible() || !canSnapTo(siblingp))
			{
				continue;
			}

			LLRect sibling_rect = siblingp->getSnapRect();

			switch(snap_edge)
			{
			case SNAP_RIGHT:
				if (!snapped_x)
				{
					if (llabs(test_rect.mRight - sibling_rect.mLeft) <= threshold && (test_rect.mRight - sibling_rect.mLeft) * mouse_dir.mX <= 0)
					{
						snap_pos = sibling_rect.mLeft - padding;
						snap_view = siblingp;
						snapped_x = TRUE;
					}
					// if snapped with sibling along other axis, check for shared edge
					else if (llabs(sibling_rect.mTop - (test_rect.mBottom - padding)) <= threshold ||
						llabs(sibling_rect.mBottom - (test_rect.mTop + padding)) <= threshold)
					{
						if (llabs(test_rect.mRight - sibling_rect.mRight) <= threshold && (test_rect.mRight - sibling_rect.mRight) * mouse_dir.mX <= 0)
						{
							snap_pos = sibling_rect.mRight;
							snap_view = siblingp;
							snapped_x = TRUE;
						}
					}
				}
				break;
			case SNAP_LEFT:
				if (!snapped_x)
				{
					if (llabs(test_rect.mLeft - sibling_rect.mRight) <= threshold && (test_rect.mLeft - sibling_rect.mRight) * mouse_dir.mX <= 0)
					{
						snap_pos = sibling_rect.mRight + padding;
						snap_view = siblingp;
						snapped_x = TRUE;
					}
					// if snapped with sibling along other axis, check for shared edge
					else if (llabs(sibling_rect.mTop - (test_rect.mBottom - padding)) <= threshold ||
						llabs(sibling_rect.mBottom - (test_rect.mTop + padding)) <= threshold)
					{
						if (llabs(test_rect.mLeft - sibling_rect.mLeft) <= threshold && (test_rect.mLeft - sibling_rect.mLeft) * mouse_dir.mX <= 0)
						{
							snap_pos = sibling_rect.mLeft;
							snap_view = siblingp;
							snapped_x = TRUE;
						}
					}
				}
				break;
			case SNAP_BOTTOM:
				if (!snapped_y)
				{
					if (llabs(test_rect.mBottom - sibling_rect.mTop) <= threshold && (test_rect.mBottom - sibling_rect.mTop) * mouse_dir.mY <= 0)
					{
						snap_pos = sibling_rect.mTop + padding;
						snap_view = siblingp;
						snapped_y = TRUE;
					}
					// if snapped with sibling along other axis, check for shared edge
					else if (llabs(sibling_rect.mRight - (test_rect.mLeft - padding)) <= threshold ||
						llabs(sibling_rect.mLeft - (test_rect.mRight + padding)) <= threshold)
					{
						if (llabs(test_rect.mBottom - sibling_rect.mBottom) <= threshold && (test_rect.mBottom - sibling_rect.mBottom) * mouse_dir.mY <= 0)
						{
							snap_pos = sibling_rect.mBottom;
							snap_view = siblingp;
							snapped_y = TRUE;
						}
					}
				}
				break;
			case SNAP_TOP:
				if (!snapped_y)
				{
					if (llabs(test_rect.mTop - sibling_rect.mBottom) <= threshold && (test_rect.mTop - sibling_rect.mBottom) * mouse_dir.mY <= 0)
					{
						snap_pos = sibling_rect.mBottom - padding;
						snap_view = siblingp;
						snapped_y = TRUE;
					}
					// if snapped with sibling along other axis, check for shared edge
					else if (llabs(sibling_rect.mRight - (test_rect.mLeft - padding)) <= threshold ||
						llabs(sibling_rect.mLeft - (test_rect.mRight + padding)) <= threshold)
					{
						if (llabs(test_rect.mTop - sibling_rect.mTop) <= threshold && (test_rect.mTop - sibling_rect.mTop) * mouse_dir.mY <= 0)
						{
							snap_pos = sibling_rect.mTop;
							snap_view = siblingp;
							snapped_y = TRUE;
						}
					}
				}
				break;
			default:
				llerrs << "Invalid snap edge" << llendl;
			}
			if (snapped_x && snapped_y)
			{
				break;
			}
		}
	}

	new_edge_val = snap_pos;
	return snap_view;
}

bool operator==(const LLViewHandle& lhs, const LLViewHandle& rhs)
{
	return lhs.mID == rhs.mID;
}

bool operator!=(const LLViewHandle& lhs, const LLViewHandle& rhs)
{
	return lhs.mID != rhs.mID;
}

bool operator<(const LLViewHandle &lhs, const LLViewHandle &rhs)
{
	return lhs.mID < rhs.mID;
}

//-----------------------------------------------------------------------------
// Listener dispatch functions
//-----------------------------------------------------------------------------

void LLView::registerEventListener(LLString name, LLSimpleListener* function)
{
	mDispatchList.insert(std::pair<LLString, LLSimpleListener*>(name, function));
}

void LLView::deregisterEventListener(LLString name)
{
	dispatch_list_t::iterator itor = mDispatchList.find(name);
	if (itor != mDispatchList.end())
	{
		delete itor->second;
		mDispatchList.erase(itor);
	}
}

LLString LLView::findEventListener(LLSimpleListener *listener) const
{
	dispatch_list_t::const_iterator itor;
	for (itor = mDispatchList.begin(); itor != mDispatchList.end(); ++itor)
	{
		if (itor->second == listener)
		{
			return itor->first;
		}
	}
	if (mParentView)
	{
		return mParentView->findEventListener(listener);
	}
	return LLString::null;
}

LLSimpleListener* LLView::getListenerByName(const LLString& callback_name)
{
	LLSimpleListener* callback = NULL;
	dispatch_list_t::iterator itor = mDispatchList.find(callback_name);
	if (itor != mDispatchList.end())
	{
		callback = itor->second;
	}
	else if (mParentView)
	{
		callback = mParentView->getListenerByName(callback_name);
	}
	return callback;
}

void LLView::addListenerToControl(LLEventDispatcher *dispatcher, const LLString& name, LLSD filter, LLSD userdata)
{
	LLSimpleListener* listener = getListenerByName(name);
	if (listener)
	{
		dispatcher->addListener(listener, filter, userdata);
	}
}

LLControlBase *LLView::findControl(LLString name)
{
	control_map_t::iterator itor = mFloaterControls.find(name);
	if (itor != mFloaterControls.end())
	{
		return itor->second;
	}
	if (mParentView)
	{
		return mParentView->findControl(name);
	}
	return LLUI::sConfigGroup->getControl(name);
}

const S32 FLOATER_H_MARGIN = 15;
const S32 MIN_WIDGET_HEIGHT = 10;
const S32 VPAD = 4;

// static
U32 LLView::createRect(LLXMLNodePtr node, LLRect &rect, LLView* parent_view, const LLRect &required_rect)
{
	U32 follows = 0;
	S32 x = FLOATER_H_MARGIN;
	S32 y = 0;
	S32 w = 0;
	S32 h = 0;

	U32 last_x = 0;
	U32 last_y = 0;
	if (parent_view)
	{
		last_y = parent_view->getRect().getHeight();
		child_list_t::const_iterator itor = parent_view->getChildList()->begin();
		if (itor != parent_view->getChildList()->end())
		{
			LLView *last_view = (*itor);
			if (last_view->getSaveToXML())
			{
				last_x = last_view->getRect().mLeft;
				last_y = last_view->getRect().mBottom;
			}
		}
	}

	LLString rect_control;
	node->getAttributeString("rect_control", rect_control);
	if (! rect_control.empty())
	{
		LLRect rect = LLUI::sConfigGroup->getRect(rect_control);
		x = rect.mLeft;
		y = rect.mBottom;
		w = rect.getWidth();
		h = rect.getHeight();
	}
	
	if (node->hasAttribute("left"))
	{
		node->getAttributeS32("left", x);
	}
	if (node->hasAttribute("bottom"))
	{
		node->getAttributeS32("bottom", y);
	}

	// Make your width the width of the containing
	// view if you don't specify a width.
	if (parent_view)
	{
		w = llmax(required_rect.getWidth(), parent_view->getRect().getWidth() - (FLOATER_H_MARGIN) - x);
		h = llmax(MIN_WIDGET_HEIGHT, required_rect.getHeight());
	}

	if (node->hasAttribute("width"))
	{
		node->getAttributeS32("width", w);
	}
	if (node->hasAttribute("height"))
	{
		node->getAttributeS32("height", h);
	}

	if (parent_view)
	{
		if (node->hasAttribute("left_delta"))
		{
			S32 left_delta = 0;
			node->getAttributeS32("left_delta", left_delta);
			x = last_x + left_delta;
		}
		else if (node->hasAttribute("left") && node->hasAttribute("right"))
		{
			// compute width based on left and right
			S32 right = 0;
			node->getAttributeS32("right", right);
			if (right < 0)
			{
				right = parent_view->getRect().getWidth() + right;
			}
			w = right - x;
		}
		else if (node->hasAttribute("left"))
		{
			if (x < 0)
			{
				x = parent_view->getRect().getWidth() + x;
				follows |= FOLLOWS_RIGHT;
			}
			else
			{
				follows |= FOLLOWS_LEFT;
			}
		}
		else if (node->hasAttribute("width") && node->hasAttribute("right"))
		{
			S32 right = 0;
			node->getAttributeS32("right", right);
			if (right < 0)
			{
				right = parent_view->getRect().getWidth() + right;
			}
			x = right - w;
		}
		else
		{
			// left not specified, same as last
			x = last_x;
		}

		if (node->hasAttribute("bottom_delta"))
		{
			S32 bottom_delta = 0;
			node->getAttributeS32("bottom_delta", bottom_delta);
			y = last_y + bottom_delta;
		}
		else if (node->hasAttribute("top"))
		{
			// compute height based on top
			S32 top = 0;
			node->getAttributeS32("top", top);
			if (top < 0)
			{
				top = parent_view->getRect().getHeight() + top;
			}
			h = top - y;
		}
		else if (node->hasAttribute("bottom"))
		{
			if (y < 0)
			{
				y = parent_view->getRect().getHeight() + y;
				follows |= FOLLOWS_TOP;
			}
			else
			{
				follows |= FOLLOWS_BOTTOM;
			}
		}
		else
		{
			// if bottom not specified, generate automatically
			if (last_y == 0)
			{
				// treat first child as "bottom"
				y = parent_view->getRect().getHeight() - (h + VPAD);
				follows |= FOLLOWS_TOP;
			}
			else
			{
				// treat subsequent children as "bottom_delta"
				y = last_y - (h + VPAD);
			}
		}
	}
	else
	{
		x = llmax(x, 0);
		y = llmax(y, 0);
		follows = FOLLOWS_LEFT | FOLLOWS_TOP;
	}
	rect.setOriginAndSize(x, y, w, h);

	return follows;
}

void LLView::initFromXML(LLXMLNodePtr node, LLView* parent)
{
	// create rect first, as this will supply initial follows flags
	LLRect view_rect;
	U32 follows_flags = createRect(node, view_rect, parent, getRequiredRect());
	// call reshape in case there are any child elements that need to be layed out
	reshape(view_rect.getWidth(), view_rect.getHeight());
	setRect(view_rect);
	setFollows(follows_flags);

	if (node->hasAttribute("follows"))
	{
		setFollowsNone();

		LLString follows;
		node->getAttributeString("follows", follows);

		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep("|");
		tokenizer tokens(follows, sep);
		tokenizer::iterator token_iter = tokens.begin();

		while(token_iter != tokens.end())
		{
			const std::string& token_str = *token_iter;
			if (token_str == "left")
			{
				setFollowsLeft();
			}
			else if (token_str == "right")
			{
				setFollowsRight();
			}
			else if (token_str == "top")
			{
				setFollowsTop();
			}
			else if (token_str == "bottom")
			{
				setFollowsBottom();
			}
			else if (token_str == "all")
			{
				setFollowsAll();
			}
			++token_iter;
		}
	}

	if (node->hasAttribute("control_name"))
	{
		LLString control_name;
		node->getAttributeString("control_name", control_name);
		setControlName(control_name, NULL);
	}

	if (node->hasAttribute("tool_tip"))
	{
		LLString tool_tip_msg("");
		node->getAttributeString("tool_tip", tool_tip_msg);
		setToolTip(tool_tip_msg);
	}

	if (node->hasAttribute("enabled"))
	{
		BOOL enabled;
		node->getAttributeBOOL("enabled", enabled);
		setEnabled(enabled);
	}
	
	if (node->hasAttribute("visible"))
	{
		BOOL visible;
		node->getAttributeBOOL("visible", visible);
		setVisible(visible);
	}
	
	if (node->hasAttribute("hidden"))
	{
		BOOL hidden;
		node->getAttributeBOOL("hidden", hidden);
		setHidden(hidden);
	}

	node->getAttributeS32("default_tab_group", mDefaultTabGroup);
	
	reshape(view_rect.getWidth(), view_rect.getHeight());
}

// static
LLFontGL* LLView::selectFont(LLXMLNodePtr node)
{
	LLFontGL* gl_font = NULL;

	if (node->hasAttribute("font"))
	{
		LLString font_name;
		node->getAttributeString("font", font_name);

		gl_font = LLFontGL::fontFromName(font_name.c_str());
	}
	return gl_font;
}

// static
LLFontGL::HAlign LLView::selectFontHAlign(LLXMLNodePtr node)
{
	LLFontGL::HAlign gl_hfont_align = LLFontGL::LEFT;

	if (node->hasAttribute("halign"))
	{
		LLString horizontal_align_name;
		node->getAttributeString("halign", horizontal_align_name);
		gl_hfont_align = LLFontGL::hAlignFromName(horizontal_align_name);
	}
	return gl_hfont_align;
}

// static
LLFontGL::VAlign LLView::selectFontVAlign(LLXMLNodePtr node)
{
	LLFontGL::VAlign gl_vfont_align = LLFontGL::BASELINE;

	if (node->hasAttribute("valign"))
	{
		LLString vert_align_name;
		node->getAttributeString("valign", vert_align_name);
		gl_vfont_align = LLFontGL::vAlignFromName(vert_align_name);
	}
	return gl_vfont_align;
}

// static
LLFontGL::StyleFlags LLView::selectFontStyle(LLXMLNodePtr node)
{
	LLFontGL::StyleFlags gl_font_style = LLFontGL::NORMAL;

	if (node->hasAttribute("style"))
	{
		LLString style_flags_name;
		node->getAttributeString("style", style_flags_name);

		if (style_flags_name == "normal")
		{
			gl_font_style = LLFontGL::NORMAL;
		}
		else if (style_flags_name == "bold")
		{
			gl_font_style = LLFontGL::BOLD;
		}
		else if (style_flags_name == "italic")
		{
			gl_font_style = LLFontGL::ITALIC;
		}
		else if (style_flags_name == "underline")
		{
			gl_font_style = LLFontGL::UNDERLINE;
		}
		//else leave left
	}
	return gl_font_style;
}

void LLView::setControlValue(const LLSD& value)
{
	LLUI::sConfigGroup->setValue(getControlName(), value);
}

//virtual
LLString LLView::getControlName() const
{
	return mControlName;
}

//virtual
void LLView::setControlName(const LLString& control_name, LLView *context)
{
	if (context == NULL)
	{
		context = this;
	}

	// Unregister from existing listeners
	if (!mControlName.empty())
	{
		clearDispatchers();
	}

	// Register new listener
	if (!control_name.empty())
	{
		LLControlBase *control = context->findControl(control_name);
		if (control)
		{
			mControlName = control_name;
			LLSD state = control->registerListener(this, "DEFAULT");
			setValue(state);
		}
	}
}

// virtual
bool LLView::handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
{
	if (userdata.asString() == "DEFAULT" && event->desc() == "value_changed")
	{
		LLSD state = event->getValue();
		setValue(state);
		return TRUE;
	}
	return FALSE;
}

void LLView::setValue(const LLSD& value)
{
}


void LLView::addBoolControl(LLString name, bool initial_value)
{
	mFloaterControls[name] = new LLControl(name, TYPE_BOOLEAN, initial_value, "Internal floater control");
}

LLControlBase *LLView::getControl(LLString name)
{
	control_map_t::iterator itor = mFloaterControls.find(name);
	if (itor != mFloaterControls.end())
	{
		return itor->second;
	}
	return NULL;
}
