/** 
 * @file llview.cpp
 * @author James Cook
 * @brief Container for other views, anything that draws.
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

#include "linden_common.h"

#define LLVIEW_CPP
#include "llview.h"

#include <cassert>
#include <sstream>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include "llrender.h"
#include "llevent.h"
#include "llfocusmgr.h"
#include "llrect.h"
#include "llstl.h"
#include "llui.h"
#include "lluictrl.h"
#include "llwindow.h"
#include "v3color.h"
#include "lluictrlfactory.h"
#include "lltooltip.h"
#include "llsdutil.h"

// for ui edit hack
#include "llbutton.h"
#include "lllineeditor.h"
#include "lltexteditor.h"
#include "lltextbox.h"

S32		LLView::sDepth = 0;
bool	LLView::sDebugRects = false;
bool	LLView::sDebugRectsShowNames = true;
bool	LLView::sDebugKeys = false;
bool	LLView::sDebugMouseHandling = false;
std::string LLView::sMouseHandlerMessage;
BOOL	LLView::sForceReshape = FALSE;
std::set<LLView*> LLView::sPreviewHighlightedElements;
BOOL LLView::sHighlightingDiffs = FALSE;
LLView* LLView::sPreviewClickedElement = NULL;
BOOL	LLView::sDrawPreviewHighlights = FALSE;
S32		LLView::sLastLeftXML = S32_MIN;
S32		LLView::sLastBottomXML = S32_MIN;
std::vector<LLViewDrawContext*> LLViewDrawContext::sDrawContextStack;

LLView::DrilldownFunc LLView::sDrilldown =
	boost::bind(&LLView::pointInView, _1, _2, _3, HIT_TEST_USE_BOUNDING_RECT);

//#if LL_DEBUG
BOOL LLView::sIsDrawing = FALSE;
//#endif

// Compiler optimization, generate extern template
template class LLView* LLView::getChild<class LLView>(
	const std::string& name, BOOL recurse) const;

static LLDefaultChildRegistry::Register<LLView> r("view");

LLView::Follows::Follows()
:   string(""),
	flags("flags", FOLLOWS_LEFT | FOLLOWS_TOP)
{}

LLView::Params::Params()
:	name("name", std::string("unnamed")),
	enabled("enabled", true),
	visible("visible", true),
	mouse_opaque("mouse_opaque", true),
	follows("follows"),
	hover_cursor("hover_cursor", "UI_CURSOR_ARROW"),
	use_bounding_rect("use_bounding_rect", false),
	tab_group("tab_group", 0),
	default_tab_group("default_tab_group"),
	tool_tip("tool_tip"),
	sound_flags("sound_flags", MOUSE_UP),
	layout("layout"),
	rect("rect"),
	bottom_delta("bottom_delta", S32_MAX),
	top_pad("top_pad"),
	top_delta("top_delta", S32_MAX),
	left_pad("left_pad"),
	left_delta("left_delta", S32_MAX),
	from_xui("from_xui", false),
	focus_root("focus_root", false),
	needs_translate("translate"),
	xmlns("xmlns"),
	xmlns_xsi("xmlns:xsi"),
	xsi_schemaLocation("xsi:schemaLocation"),
	xsi_type("xsi:type")

{
	addSynonym(rect, "");
}

LLView::LLView(const LLView::Params& p)
:	mVisible(p.visible),
	mInDraw(false),
	mName(p.name),
	mParentView(NULL),
	mReshapeFlags(FOLLOWS_NONE),
	mFromXUI(p.from_xui),
	mIsFocusRoot(p.focus_root),
	mLastVisible(FALSE),
	mNextInsertionOrdinal(0),
	mHoverCursor(getCursorFromString(p.hover_cursor)),
	mEnabled(p.enabled),
	mMouseOpaque(p.mouse_opaque),
	mSoundFlags(p.sound_flags),
	mUseBoundingRect(p.use_bounding_rect),
	mDefaultTabGroup(p.default_tab_group),
	mLastTabGroup(0),
	mToolTipMsg((LLStringExplicit)p.tool_tip()),
	mDefaultWidgets(NULL)
{
	// create rect first, as this will supply initial follows flags
	setShape(p.rect);
	parseFollowsFlags(p);
}

LLView::~LLView()
{
	dirtyRect();
	//llinfos << "Deleting view " << mName << ":" << (void*) this << llendl;
	if (LLView::sIsDrawing)
	{
		lldebugs << "Deleting view " << mName << " during UI draw() phase" << llendl;
	}
// 	llassert(LLView::sIsDrawing == FALSE);
	
//	llassert_always(sDepth == 0); // avoid deleting views while drawing! It can subtly break list iterators
	
	if( hasMouseCapture() )
	{
		//llwarns << "View holding mouse capture deleted: " << getName() << ".  Mouse capture removed." << llendl;
		gFocusMgr.removeMouseCaptureWithoutCallback( this );
	}

	deleteAllChildren();

	if (mParentView != NULL)
	{
		mParentView->removeChild(this);
	}

	if (mDefaultWidgets)
	{
		delete mDefaultWidgets;
		mDefaultWidgets = NULL;
	}
}

// virtual
BOOL LLView::isCtrl() const
{
	return FALSE;
}

// virtual
BOOL LLView::isPanel() const
{
	return FALSE;
}

void LLView::setToolTip(const LLStringExplicit& msg)
{
	mToolTipMsg = msg;
}

BOOL LLView::setToolTipArg(const LLStringExplicit& key, const LLStringExplicit& text)
{
	mToolTipMsg.setArg(key, text);
	return TRUE;
}

void LLView::setToolTipArgs( const LLStringUtil::format_map_t& args )
{
	mToolTipMsg.setArgList(args);
}

// virtual
void LLView::setRect(const LLRect& rect)
{
	mRect = rect;
	updateBoundingRect();
}

void LLView::setUseBoundingRect( BOOL use_bounding_rect ) 
{
	if (mUseBoundingRect != use_bounding_rect)
	{
        mUseBoundingRect = use_bounding_rect; 
		updateBoundingRect();
	}
}

BOOL LLView::getUseBoundingRect() const
{
	return mUseBoundingRect;
}

// virtual
const std::string& LLView::getName() const
{
	static std::string no_name("(no name)");

	return mName.empty() ? no_name : mName;
}

void LLView::sendChildToFront(LLView* child)
{
// 	llassert_always(sDepth == 0); // Avoid re-ordering while drawing; it can cause subtle iterator bugs
	if (child && child->getParent() == this) 
	{
		// minor optimization, but more importantly,
		//  won't temporarily create an empty list
		if (child != mChildList.front())
		{
			mChildList.remove( child );
			mChildList.push_front(child);
		}
	}
}

void LLView::sendChildToBack(LLView* child)
{
// 	llassert_always(sDepth == 0); // Avoid re-ordering while drawing; it can cause subtle iterator bugs
	if (child && child->getParent() == this) 
	{
		// minor optimization, but more importantly,
		//  won't temporarily create an empty list
		if (child != mChildList.back())
		{
			mChildList.remove( child );
			mChildList.push_back(child);
		}
	}
}

void LLView::moveChildToFrontOfTabGroup(LLUICtrl* child)
{
	if(mCtrlOrder.find(child) != mCtrlOrder.end())
	{
		mCtrlOrder[child].second = -1 * mNextInsertionOrdinal++;
	}
}

void LLView::moveChildToBackOfTabGroup(LLUICtrl* child)
{
	if(mCtrlOrder.find(child) != mCtrlOrder.end())
	{
		mCtrlOrder[child].second = mNextInsertionOrdinal++;
	}
}

// virtual
bool LLView::addChild(LLView* child, S32 tab_group)
{
	if (!child)
	{
		return false;
	}
	if (mParentView == child) 
	{
		llerrs << "Adding view " << child->getName() << " as child of itself" << llendl;
	}

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
		LLUICtrl* ctrl = static_cast<LLUICtrl*>(child);
		mCtrlOrder.insert(tab_order_pair_t(ctrl,
							tab_order_t(tab_group, mNextInsertionOrdinal)));

		mNextInsertionOrdinal++;
	}

	child->mParentView = this;
	updateBoundingRect();
	mLastTabGroup = tab_group;
	return true;
}


bool LLView::addChildInBack(LLView* child, S32 tab_group)
{
	if(addChild(child, tab_group))
	{
		sendChildToBack(child);
		return true;
	}

	return false;
}

// remove the specified child from the view, and set it's parent to NULL.
void LLView::removeChild(LLView* child)
{
	//llassert_always(sDepth == 0); // Avoid re-ordering while drawing; it can cause subtle iterator bugs
	if (child->mParentView == this) 
	{
		// if we are removing an item we are currently iterating over, that would be bad
		llassert(child->mInDraw == false);
		mChildList.remove( child );
		child->mParentView = NULL;
		if (child->isCtrl())
		{
			child_tab_order_t::iterator found = mCtrlOrder.find(static_cast<LLUICtrl*>(child));
			if(found != mCtrlOrder.end())
			{
				mCtrlOrder.erase(found);
			}
		}
	}
	else
	{
		llwarns << child->getName() << "is not a child of " << getName() << llendl;
	}
	updateBoundingRect();
}

LLView::ctrl_list_t LLView::getCtrlList() const
{
	ctrl_list_t controls;
	BOOST_FOREACH(LLView* viewp, mChildList)
	{
		if(viewp->isCtrl())
		{
			controls.push_back(static_cast<LLUICtrl*>(viewp));
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
		const LLUICtrl * const a_ctrl = static_cast<const LLUICtrl*>(a);
		const LLUICtrl * const b_ctrl = static_cast<const LLUICtrl*>(b);
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
	BOOL visible = TRUE;

	const LLView* viewp = this;
	while(viewp)
	{
		if (!viewp->getVisible())
		{
			visible = FALSE;
			break;
		}
		viewp = viewp->getParent();
	}
	
	return visible;
}

BOOL LLView::isInEnabledChain() const
{
	BOOL enabled = TRUE;

	const LLView* viewp = this;
	while(viewp)
	{
		if (!viewp->getEnabled())
		{
			enabled = FALSE;
			break;
		}
		viewp = viewp->getParent();
	}
	
	return enabled;
}

static void buildPathname(std::ostream& out, const LLView* view)
{
	if (! (view && view->getParent()))
	{
		return; // Don't include root in the path.
	}

	buildPathname(out, view->getParent());

	// Build pathname into ostream on the way back from recursion.
	out << '/' << view->getName();
}

std::string LLView::getPathname() const
{
	std::ostringstream out;
	buildPathname(out, this);
	return out.str();
}

//static
std::string LLView::getPathname(const LLView* view)
{
    if (! view)
    {
        return "NULL";
    }
    return view->getPathname();
}

// virtual
BOOL LLView::canFocusChildren() const
{
	return TRUE;
}

//virtual
void LLView::setEnabled(BOOL enabled)
{
	mEnabled = enabled;
}

//virtual
bool LLView::isAvailable() const
{
    return isInEnabledChain() && isInVisibleChain();
}

//static
bool LLView::isAvailable(const LLView* view)
{
    return view && view->isAvailable();
}

//virtual
BOOL LLView::setLabelArg( const std::string& key, const LLStringExplicit& text )
{
	return FALSE;
}

//virtual
LLRect LLView::getSnapRect() const
{
	return mRect;
}

//virtual
LLRect LLView::getRequiredRect()
{
	return mRect;
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
	BOOST_FOREACH(LLView* viewp, mChildList)
	{
		viewp->setEnabled(b);
	}
}

// virtual
void LLView::setVisible(BOOL visible)
{
	if ( mVisible != visible )
	{
		mVisible = visible;

		// notify children of visibility change if root, or part of visible hierarchy
		if (!getParent() || getParent()->isInVisibleChain())
		{
			// tell all children of this view that the visibility may have changed
			dirtyRect();
			handleVisibilityChange( visible );
		}
		updateBoundingRect();
	}
}

// virtual
void LLView::handleVisibilityChange ( BOOL new_visibility )
{
	BOOST_FOREACH(LLView* viewp, mChildList)
	{
		// only views that are themselves visible will have their overall visibility affected by their ancestors
		if (viewp->getVisible())
		{
			viewp->handleVisibilityChange ( new_visibility );
		}
	}
}

// virtual
void LLView::translate(S32 x, S32 y)
{
	mRect.translate(x, y);
	updateBoundingRect();
}

// virtual
BOOL LLView::canSnapTo(const LLView* other_view)
{
	return other_view != this && other_view->getVisible();
}

// virtual
void LLView::setSnappedTo(const LLView* snap_view)
{
}

BOOL LLView::handleHover(S32 x, S32 y, MASK mask)
{
	return childrenHandleHover( x, y, mask ) != NULL;
}

void LLView::onMouseEnter(S32 x, S32 y, MASK mask)
{
	//llinfos << "Mouse entered " << getName() << llendl;
}

void LLView::onMouseLeave(S32 x, S32 y, MASK mask)
{
	//llinfos << "Mouse left " << getName() << llendl;
}

bool LLView::visibleAndContains(S32 local_x, S32 local_y)
{
	return sDrilldown(this, local_x, local_y)
		&& getVisible();
}

bool LLView::visibleEnabledAndContains(S32 local_x, S32 local_y)
{
	return visibleAndContains(local_x, local_y)
		&& getEnabled();
}

void LLView::logMouseEvent()
{
	if (sDebugMouseHandling)
	{
		sMouseHandlerMessage = std::string("/") + mName + sMouseHandlerMessage;
	}
}

template <typename METHOD, typename CHARTYPE>
LLView* LLView::childrenHandleCharEvent(const std::string& desc, const METHOD& method,
										CHARTYPE c, MASK mask)
{
	if ( getVisible() && getEnabled() )
	{
		BOOST_FOREACH(LLView* viewp, mChildList)
		{
			if ((viewp->*method)(c, mask, TRUE))
			{
				if (LLView::sDebugKeys)
				{
					llinfos << desc << " handled by " << viewp->getName() << llendl;
				}
				return viewp;
			}
		}
	}
    return NULL;
}

// XDATA might be MASK, or S32 clicks
template <typename METHOD, typename XDATA>
LLView* LLView::childrenHandleMouseEvent(const METHOD& method, S32 x, S32 y, XDATA extra, bool allow_mouse_block)
{
	BOOST_FOREACH(LLView* viewp, mChildList)
	{
		S32 local_x = x - viewp->getRect().mLeft;
		S32 local_y = y - viewp->getRect().mBottom;

		if (!viewp->visibleEnabledAndContains(local_x, local_y))
		{
			continue;
		}

		if ((viewp->*method)( local_x, local_y, extra )
			|| (allow_mouse_block && viewp->blockMouseEvent( local_x, local_y )))
		{
			viewp->logMouseEvent();
			return viewp;
		}
	}
	return NULL;
}

LLView* LLView::childrenHandleToolTip(S32 x, S32 y, MASK mask)
{
	BOOST_FOREACH(LLView* viewp, mChildList)
	{
		S32 local_x = x - viewp->getRect().mLeft;
		S32 local_y = y - viewp->getRect().mBottom;
		// Differs from childrenHandleMouseEvent() in that we want to offer
		// tooltips even for disabled widgets.
		if(!viewp->visibleAndContains(local_x, local_y))
		{
			continue;
		}

		if (viewp->handleToolTip(local_x, local_y, mask) 
			|| viewp->blockMouseEvent(local_x, local_y))
		{
			viewp->logMouseEvent();
			return viewp;
		}
	}
	return NULL;
}

LLView* LLView::childrenHandleDragAndDrop(S32 x, S32 y, MASK mask,
									   BOOL drop,
									   EDragAndDropType cargo_type,
									   void* cargo_data,
									   EAcceptance* accept,
									   std::string& tooltip_msg)
{
	// default to not accepting drag and drop, will be overridden by handler
	*accept = ACCEPT_NO;

	BOOST_FOREACH(LLView* viewp, mChildList)
	{
		S32 local_x = x - viewp->getRect().mLeft;
		S32 local_y = y - viewp->getRect().mBottom;
		if( !viewp->visibleEnabledAndContains(local_x, local_y))
		{
			continue;
		}

		// Differs from childrenHandleMouseEvent() simply in that this virtual
		// method call diverges pretty radically from the usual (x, y, int).
		if (viewp->handleDragAndDrop(local_x, local_y, mask, drop,
									 cargo_type,
									 cargo_data,
									 accept,
									 tooltip_msg)
			|| viewp->blockMouseEvent(local_x, local_y))
		{
			return viewp;
		}
	}
	return NULL;
}

LLView* LLView::childrenHandleHover(S32 x, S32 y, MASK mask)
{
	BOOST_FOREACH(LLView* viewp, mChildList)
	{
		S32 local_x = x - viewp->getRect().mLeft;
		S32 local_y = y - viewp->getRect().mBottom;
		if(!viewp->visibleEnabledAndContains(local_x, local_y))
		{
			continue;
		}

		// This call differentiates this method from childrenHandleMouseEvent().
		LLUI::sWindow->setCursor(viewp->getHoverCursor());

		if (viewp->handleHover(local_x, local_y, mask)
			|| viewp->blockMouseEvent(local_x, local_y))
		{
			viewp->logMouseEvent();
			return viewp;
		}
	}
	return NULL;
}

LLView*	LLView::childFromPoint(S32 x, S32 y, bool recur)
{
	if (!getVisible())
		return false;

	BOOST_FOREACH(LLView* viewp, mChildList)
	{
		S32 local_x = x - viewp->getRect().mLeft;
		S32 local_y = y - viewp->getRect().mBottom;
		if (!viewp->visibleAndContains(local_x, local_y))
		{
			continue;
		}
		// Here we've found the first (frontmost) visible child at this level
		// containing the specified point. Is the caller asking us to drill
		// down and return the innermost leaf child at this point, or just the
		// top-level child?
		if (recur)
		{
			LLView* leaf(viewp->childFromPoint(local_x, local_y, recur));
			// Maybe viewp is already a leaf LLView, or maybe it has children
			// but this particular (x, y) point falls between them. If the
			// recursive call returns non-NULL, great, use that; else just use
			// viewp.
			return leaf? leaf : viewp;
		}
		return viewp;

	}
	return 0;
}

BOOL LLView::handleToolTip(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;

	// parents provide tooltips first, which are optionally
	// overridden by children, in case child is mouse_opaque
	std::string tooltip = getToolTip();
	if (!tooltip.empty())
	{
		// allow "scrubbing" over ui by showing next tooltip immediately
		// if previous one was still visible
		F32 timeout = LLToolTipMgr::instance().toolTipVisible() 
			? LLUI::sSettingGroups["config"]->getF32( "ToolTipFastDelay" )
			: LLUI::sSettingGroups["config"]->getF32( "ToolTipDelay" );
		LLToolTipMgr::instance().show(LLToolTip::Params()
			.message(tooltip)
			.sticky_rect(calcScreenRect())
			.delay_time(timeout));

		handled = TRUE;
	}

	// child tooltips will override our own
	LLView* child_handler = childrenHandleToolTip(x, y, mask);
	if (child_handler)
	{
		handled = TRUE;
	}

	return handled;
}
BOOL LLView::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	BOOL handled = FALSE;

	if (getVisible() && getEnabled())
	{
		if( called_from_parent )
		{
			// Downward traversal
			handled = childrenHandleKey( key, mask ) != NULL;
		}

		if (!handled)
		{
			handled = handleKeyHere( key, mask );
			if (handled && LLView::sDebugKeys)
			{
				llinfos << "Key handled by " << getName() << llendl;
			}
		}
	}

	if( !handled && !called_from_parent && mParentView)
	{
		// Upward traversal
		handled = mParentView->handleKey( key, mask, FALSE );
	}
	return handled;
}

// Called from handleKey()
// Handles key in this object.  Checking parents and children happens in handleKey()
BOOL LLView::handleKeyHere(KEY key, MASK mask)
{
	return FALSE;
}

BOOL LLView::handleUnicodeChar(llwchar uni_char, BOOL called_from_parent)
{
	BOOL handled = FALSE;

	if (getVisible() && getEnabled())
	{
		if( called_from_parent )
		{
			// Downward traversal
			handled = childrenHandleUnicodeChar( uni_char ) != NULL;
		}

		if (!handled)
		{
			handled = handleUnicodeCharHere(uni_char);
			if (handled && LLView::sDebugKeys)
			{
				llinfos << "Unicode key handled by " << getName() << llendl;
			}
		}
	}

	if (!handled && !called_from_parent && mParentView)
	{
		// Upward traversal
		handled = mParentView->handleUnicodeChar(uni_char, FALSE);
	}

	return handled;
}


BOOL LLView::handleUnicodeCharHere(llwchar uni_char )
{
	return FALSE;
}


BOOL LLView::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
							   EDragAndDropType cargo_type, void* cargo_data,
							   EAcceptance* accept,
							   std::string& tooltip_msg)
{
	return childrenHandleDragAndDrop( x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg) != NULL;
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
	return childrenHandleMouseUp( x, y, mask ) != NULL;
}

BOOL LLView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseDown( x, y, mask ) != NULL;
}

BOOL LLView::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	return childrenHandleDoubleClick( x, y, mask ) != NULL;
}

BOOL LLView::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	return childrenHandleScrollWheel( x, y, clicks ) != NULL;
}

BOOL LLView::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	return childrenHandleRightMouseDown( x, y, mask ) != NULL;
}

BOOL LLView::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	return childrenHandleRightMouseUp( x, y, mask ) != NULL;
}
 
BOOL LLView::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	return childrenHandleMiddleMouseDown( x, y, mask ) != NULL;
}

BOOL LLView::handleMiddleMouseUp(S32 x, S32 y, MASK mask)
{
	return childrenHandleMiddleMouseUp( x, y, mask ) != NULL;
}

LLView* LLView::childrenHandleScrollWheel(S32 x, S32 y, S32 clicks)
{
	return childrenHandleMouseEvent(&LLView::handleScrollWheel, x, y, clicks, false);
}

// Called during downward traversal
LLView* LLView::childrenHandleKey(KEY key, MASK mask)
{
	return childrenHandleCharEvent("Key", &LLView::handleKey, key, mask);
}

// Called during downward traversal
LLView* LLView::childrenHandleUnicodeChar(llwchar uni_char)
{
	return childrenHandleCharEvent("Unicode character", &LLView::handleUnicodeCharWithDummyMask,
								   uni_char, MASK_NONE);
}

LLView* LLView::childrenHandleMouseDown(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleMouseDown, x, y, mask);
}

LLView* LLView::childrenHandleRightMouseDown(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleRightMouseDown, x, y, mask);
}

LLView* LLView::childrenHandleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleMiddleMouseDown, x, y, mask);
}

LLView* LLView::childrenHandleDoubleClick(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleDoubleClick, x, y, mask);
}

LLView* LLView::childrenHandleMouseUp(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleMouseUp, x, y, mask);
}

LLView* LLView::childrenHandleRightMouseUp(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleRightMouseUp, x, y, mask);
}

LLView* LLView::childrenHandleMiddleMouseUp(S32 x, S32 y, MASK mask)
{
	return childrenHandleMouseEvent(&LLView::handleMiddleMouseUp, x, y, mask);
}

void LLView::draw()
{
	drawChildren();
}

void LLView::drawChildren()
{
	if (!mChildList.empty())
	{
		LLView* rootp = LLUI::getRootView();		
		++sDepth;

		for (child_list_reverse_iter_t child_iter = mChildList.rbegin(); child_iter != mChildList.rend();)  // ++child_iter)
		{
			child_list_reverse_iter_t child = child_iter++;
			LLView *viewp = *child;
			
			if (viewp == NULL)
			{
				continue;
			}

			if (viewp->getVisible() && viewp->getRect().isValid())
			{
				LLRect screen_rect = viewp->calcScreenRect();
				if ( rootp->getLocalRect().overlaps(screen_rect)  && LLUI::sDirtyRect.overlaps(screen_rect))
				{
					LLUI::pushMatrix();
					{
						LLUI::translate((F32)viewp->getRect().mLeft, (F32)viewp->getRect().mBottom);
						// flag the fact we are in draw here, in case overridden draw() method attempts to remove this widget
						viewp->mInDraw = true;
						viewp->draw();
						viewp->mInDraw = false;

						if (sDebugRects)
						{
							viewp->drawDebugRect();

							// Check for bogus rectangle
							if (!getRect().isValid())
							{
								llwarns << "Bogus rectangle for " << getName() << " with " << mRect << llendl;
							}
						}
					}
					LLUI::popMatrix();
				}
			}

		}
		--sDepth;
	}
}

void LLView::dirtyRect()
{
	LLView* child = getParent();
	LLView* parent = child ? child->getParent() : NULL;
	LLView* cur = this;
	while (child && parent && parent->getParent())
	{ //find third to top-most view
		cur = child;
		child = parent;
		parent = parent->getParent();
	}

	LLUI::dirtyRect(cur->calcScreenRect());
}

//Draw a box for debugging.
void LLView::drawDebugRect()
{
	std::set<LLView*>::iterator preview_iter = std::find(sPreviewHighlightedElements.begin(), sPreviewHighlightedElements.end(), this);	// figure out if it's a previewed element

	LLUI::pushMatrix();
	{
		// drawing solids requires texturing be disabled
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		if (getUseBoundingRect())
		{
			LLUI::translate((F32)mBoundingRect.mLeft - (F32)mRect.mLeft, (F32)mBoundingRect.mBottom - (F32)mRect.mBottom);
		}

		LLRect debug_rect = getUseBoundingRect() ? mBoundingRect : mRect;

		// draw red rectangle for the border
		LLColor4 border_color(0.25f, 0.25f, 0.25f, 1.f);
		if(preview_iter != sPreviewHighlightedElements.end())
		{
			if(LLView::sPreviewClickedElement && this == sPreviewClickedElement)
			{
				border_color = LLColor4::red;
			}
			else
			{
				static LLUIColor scroll_highlighted_color = LLUIColorTable::instance().getColor("ScrollHighlightedColor");
				border_color = scroll_highlighted_color;
			}
		}
		else
		{
			border_color.mV[sDepth%3] = 1.f;
		}

		gGL.color4fv( border_color.mV );

		gGL.begin(LLRender::LINES);
			gGL.vertex2i(0, debug_rect.getHeight() - 1);
			gGL.vertex2i(0, 0);

			gGL.vertex2i(0, 0);
			gGL.vertex2i(debug_rect.getWidth() - 1, 0);

			gGL.vertex2i(debug_rect.getWidth() - 1, 0);
			gGL.vertex2i(debug_rect.getWidth() - 1, debug_rect.getHeight() - 1);

			gGL.vertex2i(debug_rect.getWidth() - 1, debug_rect.getHeight() - 1);
			gGL.vertex2i(0, debug_rect.getHeight() - 1);
		gGL.end();

		// Draw the name if it's not a leaf node or not in editing or preview mode
		if (mChildList.size()
			&& preview_iter == sPreviewHighlightedElements.end()
			&& sDebugRectsShowNames)
		{
			//char temp[256];
			S32 x, y;
			gGL.color4fv( border_color.mV );
			x = debug_rect.getWidth()/2;
			y = debug_rect.getHeight()/2;
			std::string debug_text = llformat("%s (%d x %d)", getName().c_str(),
										debug_rect.getWidth(), debug_rect.getHeight());
			LLFontGL::getFontSansSerifSmall()->renderUTF8(debug_text, 0, (F32)x, (F32)y, border_color,
												LLFontGL::HCENTER, LLFontGL::BASELINE, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
												S32_MAX, S32_MAX, NULL, FALSE);
		}
	}
	LLUI::popMatrix();
}

void LLView::drawChild(LLView* childp, S32 x_offset, S32 y_offset, BOOL force_draw)
{
	if (childp && childp->getParent() == this)
	{
		++sDepth;

		if ((childp->getVisible() && childp->getRect().isValid()) 
			|| force_draw)
		{
			gGL.matrixMode(LLRender::MM_MODELVIEW);
			LLUI::pushMatrix();
			{
				LLUI::translate((F32)childp->getRect().mLeft + x_offset, (F32)childp->getRect().mBottom + y_offset);
				childp->draw();
			}
			LLUI::popMatrix();
		}

		--sDepth;
	}
}


void LLView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	// compute how much things changed and apply reshape logic to children
	S32 delta_width = width - getRect().getWidth();
	S32 delta_height = height - getRect().getHeight();

	if (delta_width || delta_height || sForceReshape)
	{
		// adjust our rectangle
		mRect.mRight = getRect().mLeft + width;
		mRect.mTop = getRect().mBottom + height;

		// move child views according to reshape flags
		BOOST_FOREACH(LLView* viewp, mChildList)
		{
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

			S32 delta_x = child_rect.mLeft - viewp->getRect().mLeft;
			S32 delta_y = child_rect.mBottom - viewp->getRect().mBottom;
			viewp->translate( delta_x, delta_y );
			if (child_rect.getWidth() != viewp->getRect().getWidth() || child_rect.getHeight() != viewp->getRect().getHeight())
			{
				viewp->reshape(child_rect.getWidth(), child_rect.getHeight());
			}
		}
	}

	if (!called_from_parent)
	{
		if (mParentView)
		{
			mParentView->reshape(mParentView->getRect().getWidth(), mParentView->getRect().getHeight(), FALSE);
		}
	}

	updateBoundingRect();
}

LLRect LLView::calcBoundingRect()
{
	LLRect local_bounding_rect = LLRect::null;

	BOOST_FOREACH(LLView* childp, mChildList)
	{
		// ignore invisible and "top" children when calculating bounding rect
		// such as combobox popups
		if (!childp->getVisible() || childp == gFocusMgr.getTopCtrl()) 
		{
			continue;
		}

		LLRect child_bounding_rect = childp->getBoundingRect();

		if (local_bounding_rect.isEmpty())
		{
			// start out with bounding rect equal to first visible child's bounding rect
			local_bounding_rect = child_bounding_rect;
		}
		else
		{
			// accumulate non-null children rectangles
			if (!child_bounding_rect.isEmpty())
			{
				local_bounding_rect.unionWith(child_bounding_rect);
			}
		}
	}

	// convert to parent-relative coordinates
	local_bounding_rect.translate(mRect.mLeft, mRect.mBottom);
	return local_bounding_rect;
}


void LLView::updateBoundingRect()
{
	if (isDead()) return;

	LLRect cur_rect = mBoundingRect;

	if (getUseBoundingRect())
	{
		mBoundingRect = calcBoundingRect();
	}
	else
	{
		mBoundingRect = mRect;
	}

	// give parent view a chance to resize, in case we just moved, for example
	if (getParent() && getParent()->getUseBoundingRect())
	{
		getParent()->updateBoundingRect();
	}

	if (mBoundingRect != cur_rect)
	{
		dirtyRect();
	}

}

LLRect LLView::calcScreenRect() const
{
	LLRect screen_rect;
	localPointToScreen(0, 0, &screen_rect.mLeft, &screen_rect.mBottom);
	localPointToScreen(getRect().getWidth(), getRect().getHeight(), &screen_rect.mRight, &screen_rect.mTop);
	return screen_rect;
}

LLRect LLView::calcScreenBoundingRect() const
{
	LLRect screen_rect;
	// get bounding rect, if used
	LLRect bounding_rect = getUseBoundingRect() ? mBoundingRect : mRect;

	// convert to local coordinates, as defined by mRect
	bounding_rect.translate(-mRect.mLeft, -mRect.mBottom);

	localPointToScreen(bounding_rect.mLeft, bounding_rect.mBottom, &screen_rect.mLeft, &screen_rect.mBottom);
	localPointToScreen(bounding_rect.mRight, bounding_rect.mTop, &screen_rect.mRight, &screen_rect.mTop);
	return screen_rect;
}

LLRect LLView::getLocalBoundingRect() const
{
	LLRect local_bounding_rect = getBoundingRect();
	local_bounding_rect.translate(-mRect.mLeft, -mRect.mBottom);

	return local_bounding_rect;
}


LLRect LLView::getLocalRect() const
{
	LLRect local_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
	return local_rect;
}

LLRect LLView::getLocalSnapRect() const
{
	LLRect local_snap_rect = getSnapRect();
	local_snap_rect.translate(-getRect().mLeft, -getRect().mBottom);
	return local_snap_rect;
}

BOOL LLView::hasAncestor(const LLView* parentp) const
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

BOOL LLView::childHasKeyboardFocus( const std::string& childname ) const
{
	LLView *focus = dynamic_cast<LLView *>(gFocusMgr.getKeyboardFocus());
	
	while (focus != NULL)
	{
		if (focus->getName() == childname)
		{
			return TRUE;
		}
		
		focus = focus->getParent();
	}
	
	return FALSE;
}

//-----------------------------------------------------------------------------

BOOL LLView::hasChild(const std::string& childname, BOOL recurse) const
{
	return findChildView(childname, recurse) != NULL;
}

//-----------------------------------------------------------------------------
// getChildView()
//-----------------------------------------------------------------------------
LLView* LLView::getChildView(const std::string& name, BOOL recurse) const
{
	return getChild<LLView>(name, recurse);
}

static LLFastTimer::DeclareTimer FTM_FIND_VIEWS("Find Widgets");

LLView* LLView::findChildView(const std::string& name, BOOL recurse) const
{
	LLFastTimer ft(FTM_FIND_VIEWS);
	//richard: should we allow empty names?
	//if(name.empty())
	//	return NULL;
	// Look for direct children *first*
	BOOST_FOREACH(LLView* childp, mChildList)
	{
		llassert(childp);
		if (childp->getName() == name)
		{
			return childp;
		}
	}
	if (recurse)
	{
		// Look inside each child as well.
		BOOST_FOREACH(LLView* childp, mChildList)
		{
			llassert(childp);
			LLView* viewp = childp->findChildView(name, recurse);
			if ( viewp )
			{
				return viewp;
			}
		}
	}
	return NULL;
}

BOOL LLView::parentPointInView(S32 x, S32 y, EHitTestType type) const 
{ 
	return (getUseBoundingRect() && type == HIT_TEST_USE_BOUNDING_RECT)
		? mBoundingRect.pointInRect( x, y ) 
		: mRect.pointInRect( x, y ); 
}

BOOL LLView::pointInView(S32 x, S32 y, EHitTestType type) const 
{ 
	return (getUseBoundingRect() && type == HIT_TEST_USE_BOUNDING_RECT)
		? mBoundingRect.pointInRect( x + mRect.mLeft, y + mRect.mBottom ) 
		: mRect.localPointInRect( x, y ); 
}

BOOL LLView::blockMouseEvent(S32 x, S32 y) const
{
	return mMouseOpaque && pointInView(x, y, HIT_TEST_IGNORE_BOUNDING_RECT);
}

// virtual
void LLView::screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const
{
	*local_x = screen_x - getRect().mLeft;
	*local_y = screen_y - getRect().mBottom;

	const LLView* cur = this;
	while( cur->mParentView )
	{
		cur = cur->mParentView;
		*local_x -= cur->getRect().mLeft;
		*local_y -= cur->getRect().mBottom;
	}
}

void LLView::localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const
{
	*screen_x = local_x;
	*screen_y = local_y;

	const LLView* cur = this;
	do
	{
		LLRect cur_rect = cur->getRect();
		*screen_x += cur_rect.mLeft;
		*screen_y += cur_rect.mBottom;
		cur = cur->mParentView;
	}
	while( cur );
}

void LLView::screenRectToLocal(const LLRect& screen, LLRect* local) const
{
	*local = screen;
	local->translate( -getRect().mLeft, -getRect().mBottom );

	const LLView* cur = this;
	while( cur->mParentView )
	{
		cur = cur->mParentView;
		local->translate( -cur->getRect().mLeft, -cur->getRect().mBottom );
	}
}

void LLView::localRectToScreen(const LLRect& local, LLRect* screen) const
{
	*screen = local;
	screen->translate( getRect().mLeft, getRect().mBottom );

	const LLView* cur = this;
	while( cur->mParentView )
	{
		cur = cur->mParentView;
		screen->translate( cur->getRect().mLeft, cur->getRect().mBottom );
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

LLView* LLView::findPrevSibling(LLView* child)
{
	child_list_t::iterator prev_it = std::find(mChildList.begin(), mChildList.end(), child);
	if (prev_it != mChildList.end() && prev_it != mChildList.begin())
	{
		return *(--prev_it);
	}
	return NULL;
}

LLView* LLView::findNextSibling(LLView* child)
{
	child_list_t::iterator next_it = std::find(mChildList.begin(), mChildList.end(), child);
	if (next_it != mChildList.end())
	{
		next_it++;
	}

	return (next_it != mChildList.end()) ? *next_it : NULL;
}


LLCoordGL getNeededTranslation(const LLRect& input, const LLRect& constraint, S32 min_overlap_pixels)
{
	LLCoordGL delta;

	const S32 KEEP_ONSCREEN_PIXELS_WIDTH = llmin(min_overlap_pixels, input.getWidth());
	const S32 KEEP_ONSCREEN_PIXELS_HEIGHT = llmin(min_overlap_pixels, input.getHeight());

	if( input.mRight - KEEP_ONSCREEN_PIXELS_WIDTH < constraint.mLeft )
	{
		delta.mX = constraint.mLeft - (input.mRight - KEEP_ONSCREEN_PIXELS_WIDTH);
	}
	else if( input.mLeft + KEEP_ONSCREEN_PIXELS_WIDTH > constraint.mRight )
	{
		delta.mX = constraint.mRight - (input.mLeft + KEEP_ONSCREEN_PIXELS_WIDTH);
	}

	if( input.mTop > constraint.mTop )
	{
		delta.mY = constraint.mTop - input.mTop;
	}
	else
	if( input.mTop - KEEP_ONSCREEN_PIXELS_HEIGHT < constraint.mBottom )
	{
		delta.mY = constraint.mBottom - (input.mTop - KEEP_ONSCREEN_PIXELS_HEIGHT);
	}

	return delta;
}

// Moves the view so that it is entirely inside of constraint.
// If the view will not fit because it's too big, aligns with the top and left.
// (Why top and left?  That's where the drag bars are for floaters.)
BOOL LLView::translateIntoRect(const LLRect& constraint, S32 min_overlap_pixels)
{
	LLCoordGL translation = getNeededTranslation(getRect(), constraint, min_overlap_pixels);

	if (translation.mX != 0 || translation.mY != 0)
	{
		translate(translation.mX, translation.mY);
		return TRUE;
	}
	return FALSE;
}

// move this view into "inside" but not onto "exclude"
// NOTE: if this view is already contained in "inside", we ignore the "exclude" rect
BOOL LLView::translateIntoRectWithExclusion( const LLRect& inside, const LLRect& exclude, S32 min_overlap_pixels)
{
	LLCoordGL translation = getNeededTranslation(getRect(), inside, min_overlap_pixels);
	
	if (translation.mX != 0 || translation.mY != 0)
	{
		// translate ourselves into constraint rect
		translate(translation.mX, translation.mY);
	
		// do we overlap with exclusion area?
		// keep moving in the same direction to the other side of the exclusion rect
		if (exclude.overlaps(getRect()))
		{
			// moving right
			if (translation.mX > 0)
			{
				translate(exclude.mRight - getRect().mLeft, 0);
			}
			// moving left
			else if (translation.mX < 0)
			{
				translate(exclude.mLeft - getRect().mRight, 0);
			}

			// moving up
			if (translation.mY > 0)
			{
				translate(0, exclude.mTop - getRect().mBottom);
			}
			// moving down
			else if (translation.mY < 0)
			{
				translate(0, exclude.mBottom - getRect().mTop);
			}
		}

		return TRUE;
	}
	return FALSE;
}


void LLView::centerWithin(const LLRect& bounds)
{
	S32 left   = bounds.mLeft + (bounds.getWidth() - getRect().getWidth()) / 2;
	S32 bottom = bounds.mBottom + (bounds.getHeight() - getRect().getHeight()) / 2;

	translate( left - getRect().mLeft, bottom - getRect().mBottom );
}

BOOL LLView::localPointToOtherView( S32 x, S32 y, S32 *other_x, S32 *other_y, const LLView* other_view) const
{
	const LLView* cur_view = this;
	const LLView* root_view = NULL;

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

BOOL LLView::localRectToOtherView( const LLRect& local, LLRect* other, const LLView* other_view ) const
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

// static
const LLCtrlQuery & LLView::getTabOrderQuery()
{
	static LLCtrlQuery query;
	if(query.getPreFilters().size() == 0) {
		query.addPreFilter(LLVisibleFilter::getInstance());
		query.addPreFilter(LLEnabledFilter::getInstance());
		query.addPreFilter(LLTabStopFilter::getInstance());
		query.addPostFilter(LLLeavesFilter::getInstance());
	}
	return query;
}

// This class is only used internally by getFocusRootsQuery below. 
class LLFocusRootsFilter : public LLQueryFilter, public LLSingleton<LLFocusRootsFilter>
{
	/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const 
	{
		return filterResult_t(view->isCtrl() && view->isFocusRoot(), !view->isFocusRoot());
	}
};

// static
const LLCtrlQuery & LLView::getFocusRootsQuery()
{
	static LLCtrlQuery query;
	if(query.getPreFilters().size() == 0) {
		query.addPreFilter(LLVisibleFilter::getInstance());
		query.addPreFilter(LLEnabledFilter::getInstance());
		query.addPreFilter(LLFocusRootsFilter::getInstance());
		query.addPostFilter(LLRootsFilter::getInstance());
	}
	return query;
}


void	LLView::setShape(const LLRect& new_rect, bool by_user)
{
	if (new_rect != getRect())
	{
		handleReshape(new_rect, by_user);
	}
}

void LLView::handleReshape(const LLRect& new_rect, bool by_user)
{
	reshape(new_rect.getWidth(), new_rect.getHeight());
	translate(new_rect.mLeft - getRect().mLeft, new_rect.mBottom - getRect().mBottom);
}

LLView* LLView::findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir,
							 LLView::ESnapType snap_type, S32 threshold, S32 padding)
{
	new_rect = mRect;
	LLView* snap_view = NULL;

	if (!mParentView)
	{
		return NULL;
	}
	
	S32 delta_x = 0;
	S32 delta_y = 0;
	if (mouse_dir.mX >= 0)
	{
		S32 new_right = mRect.mRight;
		LLView* view = findSnapEdge(new_right, mouse_dir, SNAP_RIGHT, snap_type, threshold, padding);
		delta_x = new_right - mRect.mRight;
		snap_view = view ? view : snap_view;
	}

	if (mouse_dir.mX <= 0)
	{
		S32 new_left = mRect.mLeft;
		LLView* view = findSnapEdge(new_left, mouse_dir, SNAP_LEFT, snap_type, threshold, padding);
		delta_x = new_left - mRect.mLeft;
		snap_view = view ? view : snap_view;
	}

	if (mouse_dir.mY >= 0)
	{
		S32 new_top = mRect.mTop;
		LLView* view = findSnapEdge(new_top, mouse_dir, SNAP_TOP, snap_type, threshold, padding);
		delta_y = new_top - mRect.mTop;
		snap_view = view ? view : snap_view;
	}

	if (mouse_dir.mY <= 0)
	{
		S32 new_bottom = mRect.mBottom;
		LLView* view = findSnapEdge(new_bottom, mouse_dir, SNAP_BOTTOM, snap_type, threshold, padding);
		delta_y = new_bottom - mRect.mBottom;
		snap_view = view ? view : snap_view;
	}

	new_rect.translate(delta_x, delta_y);
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

	S32 x_threshold = threshold;
	S32 y_threshold = threshold;

	LLRect parent_local_snap_rect = mParentView->getLocalSnapRect();

	if (snap_type == SNAP_PARENT || snap_type == SNAP_PARENT_AND_SIBLINGS)
	{
		switch(snap_edge)
		{
		case SNAP_RIGHT:
			if (llabs(parent_local_snap_rect.mRight - test_rect.mRight) <= x_threshold 
				&& (parent_local_snap_rect.mRight - test_rect.mRight) * mouse_dir.mX >= 0)
			{
				snap_pos = parent_local_snap_rect.mRight - padding;
				snap_view = mParentView;
				x_threshold = llabs(parent_local_snap_rect.mRight - test_rect.mRight);
			}
			break;
		case SNAP_LEFT:
			if (llabs(test_rect.mLeft - parent_local_snap_rect.mLeft) <= x_threshold 
				&& test_rect.mLeft * mouse_dir.mX <= 0)
			{
				snap_pos = parent_local_snap_rect.mLeft + padding;
				snap_view = mParentView;
				x_threshold = llabs(test_rect.mLeft - parent_local_snap_rect.mLeft);
			}
			break;
		case SNAP_BOTTOM:
			if (llabs(test_rect.mBottom - parent_local_snap_rect.mBottom) <= y_threshold 
				&& test_rect.mBottom * mouse_dir.mY <= 0)
			{
				snap_pos = parent_local_snap_rect.mBottom + padding;
				snap_view = mParentView;
				y_threshold = llabs(test_rect.mBottom - parent_local_snap_rect.mBottom);
			}
			break;
		case SNAP_TOP:
			if (llabs(parent_local_snap_rect.mTop - test_rect.mTop) <= y_threshold && (parent_local_snap_rect.mTop - test_rect.mTop) * mouse_dir.mY >= 0)
			{
				snap_pos = parent_local_snap_rect.mTop - padding;
				snap_view = mParentView;
				y_threshold = llabs(parent_local_snap_rect.mTop - test_rect.mTop);
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

			if (!canSnapTo(siblingp)) continue;

			LLRect sibling_rect = siblingp->getSnapRect();

			switch(snap_edge)
			{
			case SNAP_RIGHT:
				if (llabs(test_rect.mRight - sibling_rect.mLeft) <= x_threshold 
					&& (test_rect.mRight - sibling_rect.mLeft) * mouse_dir.mX <= 0)
				{
					snap_pos = sibling_rect.mLeft - padding;
					snap_view = siblingp;
					x_threshold = llabs(test_rect.mRight - sibling_rect.mLeft);
				}
				// if snapped with sibling along other axis, check for shared edge
				else if (llabs(sibling_rect.mTop - (test_rect.mBottom - padding)) <= y_threshold 
					|| llabs(sibling_rect.mBottom - (test_rect.mTop + padding)) <= x_threshold)
				{
					if (llabs(test_rect.mRight - sibling_rect.mRight) <= x_threshold 
						&& (test_rect.mRight - sibling_rect.mRight) * mouse_dir.mX <= 0)
					{
						snap_pos = sibling_rect.mRight;
						snap_view = siblingp;
						x_threshold = llabs(test_rect.mRight - sibling_rect.mRight);
					}
				}
				break;
			case SNAP_LEFT:
				if (llabs(test_rect.mLeft - sibling_rect.mRight) <= x_threshold 
					&& (test_rect.mLeft - sibling_rect.mRight) * mouse_dir.mX <= 0)
				{
					snap_pos = sibling_rect.mRight + padding;
					snap_view = siblingp;
					x_threshold = llabs(test_rect.mLeft - sibling_rect.mRight);				
				}
				// if snapped with sibling along other axis, check for shared edge
				else if (llabs(sibling_rect.mTop - (test_rect.mBottom - padding)) <= y_threshold 
					|| llabs(sibling_rect.mBottom - (test_rect.mTop + padding)) <= y_threshold)
				{
					if (llabs(test_rect.mLeft - sibling_rect.mLeft) <= x_threshold 
						&& (test_rect.mLeft - sibling_rect.mLeft) * mouse_dir.mX <= 0)
					{
						snap_pos = sibling_rect.mLeft;
						snap_view = siblingp;
						x_threshold = llabs(test_rect.mLeft - sibling_rect.mLeft);									
					}
				}
				break;
			case SNAP_BOTTOM:
				if (llabs(test_rect.mBottom - sibling_rect.mTop) <= y_threshold 
					&& (test_rect.mBottom - sibling_rect.mTop) * mouse_dir.mY <= 0)
				{
					snap_pos = sibling_rect.mTop + padding;
					snap_view = siblingp;
					y_threshold = llabs(test_rect.mBottom - sibling_rect.mTop);
				}
				// if snapped with sibling along other axis, check for shared edge
				else if (llabs(sibling_rect.mRight - (test_rect.mLeft - padding)) <= x_threshold 
					|| llabs(sibling_rect.mLeft - (test_rect.mRight + padding)) <= x_threshold)
				{
					if (llabs(test_rect.mBottom - sibling_rect.mBottom) <= y_threshold 
						&& (test_rect.mBottom - sibling_rect.mBottom) * mouse_dir.mY <= 0)
					{
						snap_pos = sibling_rect.mBottom;
						snap_view = siblingp;
						y_threshold = llabs(test_rect.mBottom - sibling_rect.mBottom);
					}
				}
				break;
			case SNAP_TOP:
				if (llabs(test_rect.mTop - sibling_rect.mBottom) <= y_threshold 
					&& (test_rect.mTop - sibling_rect.mBottom) * mouse_dir.mY <= 0)
				{
					snap_pos = sibling_rect.mBottom - padding;
					snap_view = siblingp;
					y_threshold = llabs(test_rect.mTop - sibling_rect.mBottom);
				}
				// if snapped with sibling along other axis, check for shared edge
				else if (llabs(sibling_rect.mRight - (test_rect.mLeft - padding)) <= x_threshold 
					|| llabs(sibling_rect.mLeft - (test_rect.mRight + padding)) <= x_threshold)
				{
					if (llabs(test_rect.mTop - sibling_rect.mTop) <= y_threshold 
						&& (test_rect.mTop - sibling_rect.mTop) * mouse_dir.mY <= 0)
					{
						snap_pos = sibling_rect.mTop;
						snap_view = siblingp;
						y_threshold = llabs(test_rect.mTop - sibling_rect.mTop);
					}
				}
				break;
			default:
				llerrs << "Invalid snap edge" << llendl;
			}
		}
	}

	new_edge_val = snap_pos;
	return snap_view;
}

//-----------------------------------------------------------------------------
// Listener dispatch functions
//-----------------------------------------------------------------------------


LLControlVariable *LLView::findControl(const std::string& name)
{
	// parse the name to locate which group it belongs to
	std::size_t key_pos= name.find(".");
	if(key_pos!=  std::string::npos )
	{
		std::string control_group_key = name.substr(0, key_pos);
		LLControlVariable* control;
		// check if it's in the control group that name indicated
		if(LLUI::sSettingGroups[control_group_key])
		{
			control = LLUI::sSettingGroups[control_group_key]->getControl(name);
			if (control)
			{
				return control;
			}
		}
	}
	
	LLControlGroup& control_group = LLUI::getControlControlGroup(name);
	return control_group.getControl(name);	
}

const S32 FLOATER_H_MARGIN = 15;
const S32 MIN_WIDGET_HEIGHT = 10;
const S32 VPAD = 4;

void LLView::initFromParams(const LLView::Params& params)
{
	LLRect required_rect = getRequiredRect();

	S32 width = llmax(getRect().getWidth(), required_rect.getWidth());
	S32 height = llmax(getRect().getHeight(), required_rect.getHeight());

	reshape(width, height);

	// call virtual methods with most recent data
	// use getters because these values might not come through parameter block
	setEnabled(getEnabled());
	setVisible(getVisible());

	if (!params.name().empty())
	{
		setName(params.name());
	}

	mLayout = params.layout();
}

void LLView::parseFollowsFlags(const LLView::Params& params)
{
	// preserve follows flags set by code if user did not override
	if (!params.follows.isProvided()) 
	{
		return;
	}

	// interpret either string or bitfield version of follows
	if (params.follows.string.isChosen())
	{
		setFollows(FOLLOWS_NONE);

		std::string follows = params.follows.string;

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
	else if (params.follows.flags.isChosen())
	{
		setFollows(params.follows.flags);
	}
}


// static
//LLFontGL::HAlign LLView::selectFontHAlign(LLXMLNodePtr node)
//{
//	LLFontGL::HAlign gl_hfont_align = LLFontGL::LEFT;
//
//	if (node->hasAttribute("halign"))
//	{
//		std::string horizontal_align_name;
//		node->getAttributeString("halign", horizontal_align_name);
//		gl_hfont_align = LLFontGL::hAlignFromName(horizontal_align_name);
//	}
//	return gl_hfont_align;
//}

// Return the rectangle of the last-constructed child,
// if present and a first-class widget (eg, not a close box or drag handle)
// Returns true if found
static bool get_last_child_rect(LLView* parent, LLRect *rect)
{
	if (!parent) return false;

	LLView::child_list_t::const_iterator itor = 
		parent->getChildList()->begin();
	for (;itor != parent->getChildList()->end(); ++itor)
	{
		LLView *last_view = (*itor);
		if (last_view->getFromXUI())
		{
			*rect = last_view->getRect();
			return true;
		}
	}
	return false;
}

//static
void LLView::applyXUILayout(LLView::Params& p, LLView* parent, LLRect layout_rect)
{
	if (!parent) return;

	const S32 VPAD = 4;
	const S32 MIN_WIDGET_HEIGHT = 10;
	
	// *NOTE:  This will confuse export of floater/panel coordinates unless
	// the default is also "topleft".  JC
	if (p.layout().empty())
	{
		p.layout = parent->getLayout();
	}

	if (layout_rect.isEmpty())
	{
		layout_rect = parent->getLocalRect();
	}

	// overwrite uninitialized rect params, using context
	LLRect default_rect = parent->getLocalRect();

	bool layout_topleft = (p.layout() == "topleft");

	// convert negative or centered coordinates to parent relative values
	// Note: some of this logic matches the logic in TypedParam<LLRect>::setValueFromBlock()
	if (p.rect.left.isProvided()) 
	{
		p.rect.left = p.rect.left + ((p.rect.left >= 0) ? layout_rect.mLeft : layout_rect.mRight);
	}
	if (p.rect.right.isProvided())
	{
		p.rect.right = p.rect.right + ((p.rect.right >= 0) ? layout_rect.mLeft : layout_rect.mRight);
	}
	if (p.rect.bottom.isProvided()) 
	{
		p.rect.bottom = p.rect.bottom + ((p.rect.bottom >= 0) ? layout_rect.mBottom : layout_rect.mTop);
		if (layout_topleft)
		{
			//invert top to bottom
			p.rect.bottom = layout_rect.mBottom + layout_rect.mTop - p.rect.bottom;
		}
	}
	if (p.rect.top.isProvided())
	{
		p.rect.top = p.rect.top + ((p.rect.top >= 0) ? layout_rect.mBottom : layout_rect.mTop);
		if (layout_topleft)
		{
			//invert top to bottom
			p.rect.top = layout_rect.mBottom + layout_rect.mTop - p.rect.top;
		}
	}

	// DEPRECATE: automatically fall back to height of MIN_WIDGET_HEIGHT pixels
	if (!p.rect.height.isProvided() && !p.rect.top.isProvided() && p.rect.height == 0)
	{
		p.rect.height = MIN_WIDGET_HEIGHT;
	}

	default_rect.translate(0, default_rect.getHeight());

	// If there was a recently constructed child, use its rectangle
	get_last_child_rect(parent, &default_rect);

	if (layout_topleft)
	{
		// Invert the sense of bottom_delta for topleft layout
		if (p.bottom_delta.isProvided())
		{
			p.bottom_delta = -p.bottom_delta;
		}
		else if (p.top_pad.isProvided()) 
		{
			p.bottom_delta = -(p.rect.height + p.top_pad);
		}
		else if (p.top_delta.isProvided())
		{
			p.bottom_delta =
				-(p.top_delta + p.rect.height - default_rect.getHeight());
		}
		else if (!p.left_delta.isProvided()
					&& !p.left_pad.isProvided())
		{
			// set default position is just below last rect
			p.bottom_delta.set(-(p.rect.height + VPAD), false);
		}
		else
		{
			p.bottom_delta.set(0, false);
		}
	
		// default to same left edge
		if (!p.left_delta.isProvided())
		{
			p.left_delta.set(0, false);
		}
		if (p.left_pad.isProvided())
		{
			// left_pad is based on prior widget's right edge
			p.left_delta.set(p.left_pad + default_rect.getWidth(), false);
		}
			
		default_rect.translate(p.left_delta, p.bottom_delta);				
	}
	else
	{	
		// set default position is just below last rect
		if (!p.bottom_delta.isProvided())
		{
			p.bottom_delta.set(-(p.rect.height + VPAD), false);
		}
		if (!p.left_delta.isProvided())
		{
			p.left_delta.set(0, false);
		}
		default_rect.translate(p.left_delta, p.bottom_delta);
	}

	// this handles case where *both* x and x_delta are provided
	// ignore x in favor of default x + x_delta
	if (p.bottom_delta.isProvided()) p.rect.bottom.set(0, false);
	if (p.left_delta.isProvided()) p.rect.left.set(0, false);

	// selectively apply rectangle defaults, making sure that
	// params are not flagged as having been "provided"
	// as rect params are overconstrained and rely on provided flags
	if (!p.rect.left.isProvided())
	{
		p.rect.left.set(default_rect.mLeft, false);
		//HACK: get around the fact that setting a rect param component value won't invalidate the existing rect object value
		p.rect.paramChanged(p.rect.left, true);
	}
	if (!p.rect.bottom.isProvided())
	{
		p.rect.bottom.set(default_rect.mBottom, false);
		p.rect.paramChanged(p.rect.bottom, true);
	}
	if (!p.rect.top.isProvided())
	{
		p.rect.top.set(default_rect.mTop, false);
		p.rect.paramChanged(p.rect.top, true);
	}
	if (!p.rect.right.isProvided())
	{
		p.rect.right.set(default_rect.mRight, false);
		p.rect.paramChanged(p.rect.right, true);

	}
	if (!p.rect.width.isProvided())
	{
		p.rect.width.set(default_rect.getWidth(), false);
		p.rect.paramChanged(p.rect.width, true);
	}
	if (!p.rect.height.isProvided())
	{
		p.rect.height.set(default_rect.getHeight(), false);
		p.rect.paramChanged(p.rect.height, true);
	}
}

static S32 invert_vertical(S32 y, LLView* parent)
{
	if (y < 0)
	{
		// already based on top-left, just invert
		return -y;
	}
	else if (parent)
	{
		// use parent to flip coordinate
		S32 parent_height = parent->getRect().getHeight();
		return parent_height - y;
	}
	else
	{
		llwarns << "Attempting to convert layout to top-left with no parent" << llendl;
		return y;
	}
}

// Assumes that input is in bottom-left coordinates, hence must call
// _before_ convert_coords_to_top_left().
static void convert_to_relative_layout(LLView::Params& p, LLView* parent)
{
	// Use setupParams to get the final widget rectangle
	// according to our wacky layout rules.
	LLView::Params final = p;
	LLView::applyXUILayout(final, parent);
	// Must actually extract the rectangle to get consistent
	// right = left+width, top = bottom+height
	LLRect final_rect = final.rect;

	// We prefer to write out top edge instead of bottom, regardless
	// of whether we use relative positioning
	bool converted_top = false;

	// Look for a last rectangle
	LLRect last_rect;
	if (get_last_child_rect(parent, &last_rect))
	{
		// ...we have a previous widget to compare to
		const S32 EDGE_THRESHOLD_PIXELS = 4;
		S32 left_pad = final_rect.mLeft - last_rect.mRight;
		S32 left_delta = final_rect.mLeft - last_rect.mLeft;
		S32 top_pad = final_rect.mTop - last_rect.mBottom;
		S32 top_delta = final_rect.mTop - last_rect.mTop;
		// If my left edge is almost the same, or my top edge is
		// almost the same...
		if (llabs(left_delta) <= EDGE_THRESHOLD_PIXELS
			|| llabs(top_delta) <= EDGE_THRESHOLD_PIXELS)
		{
			// ...use relative positioning
			// prefer top_pad if widgets are stacking vertically
			// (coordinate system is still bottom-left here)
			if (top_pad < 0)
			{
				p.top_pad = top_pad;
				p.top_delta.setProvided(false);
			}
			else
			{
				p.top_pad.setProvided(false);
				p.top_delta = top_delta;
			}
			// null out other vertical specifiers
			p.rect.top.setProvided(false);
			p.rect.bottom.setProvided(false);
			p.bottom_delta.setProvided(false);
			converted_top = true;

			// prefer left_pad if widgets are stacking horizontally
			if (left_pad > 0)
			{
				p.left_pad = left_pad;
				p.left_delta.setProvided(false);
			}
			else
			{
				p.left_pad.setProvided(false);
				p.left_delta = left_delta;
			}
			p.rect.left.setProvided(false);
			p.rect.right.setProvided(false);
		}
	}

	if (!converted_top)
	{
		// ...this is the first widget, or one that wasn't aligned
		// prefer top/left specification
		p.rect.top = final_rect.mTop;
		p.rect.bottom.setProvided(false);
		p.bottom_delta.setProvided(false);
		p.top_pad.setProvided(false);
		p.top_delta.setProvided(false);
	}
}

static void convert_coords_to_top_left(LLView::Params& p, LLView* parent)
{
	// Convert the coordinate system to be top-left based.
	if (p.rect.top.isProvided())
	{
		p.rect.top = invert_vertical(p.rect.top, parent);
	}
	if (p.rect.bottom.isProvided())
	{
		p.rect.bottom = invert_vertical(p.rect.bottom, parent);
	}
	if (p.top_pad.isProvided())
	{
		p.top_pad = -p.top_pad;
	}
	if (p.top_delta.isProvided())
	{
		p.top_delta = -p.top_delta;
	}
	if (p.bottom_delta.isProvided())
	{
		p.bottom_delta = -p.bottom_delta;
	}
	p.layout = "topleft";
}

//static
void LLView::setupParamsForExport(Params& p, LLView* parent)
{
	// Don't convert if already top-left based
	if (p.layout() == "topleft") 
	{
		return;
	}

	// heuristic:  Many of our floaters and panels were bulk-exported.
	// These specify exactly bottom/left and height/width.
	// Others were done by hand using bottom_delta and/or left_delta.
	// Some rely on not specifying left to mean align with left edge.
	// Try to convert both to use relative layout, but using top-left
	// coordinates.
	// Avoid rectangles where top/bottom/left/right was specified.
	if (p.rect.height.isProvided() && p.rect.width.isProvided())
	{
		if (p.rect.bottom.isProvided() && p.rect.left.isProvided())
		{
			// standard bulk export, convert it
			convert_to_relative_layout(p, parent);
		}
		else if (p.rect.bottom.isProvided() && p.left_delta.isProvided())
		{
			// hand layout with left_delta
			convert_to_relative_layout(p, parent);
		}
		else if (p.bottom_delta.isProvided())
		{
			// hand layout with bottom_delta
			// don't check for p.rect.left or p.left_delta because sometimes
			// this layout doesn't set it for widgets that are left-aligned
			convert_to_relative_layout(p, parent);
		}
	}

	convert_coords_to_top_left(p, parent);
}

LLView::tree_iterator_t LLView::beginTreeDFS() 
{ 
	return tree_iterator_t(this, 
							boost::bind(boost::mem_fn(&LLView::beginChild), _1), 
							boost::bind(boost::mem_fn(&LLView::endChild), _1)); 
}

LLView::tree_iterator_t LLView::endTreeDFS() 
{ 
	// an empty iterator is an "end" iterator
	return tree_iterator_t();
}

LLView::tree_post_iterator_t LLView::beginTreeDFSPost() 
{ 
	return tree_post_iterator_t(this, 
							boost::bind(boost::mem_fn(&LLView::beginChild), _1), 
							boost::bind(boost::mem_fn(&LLView::endChild), _1)); 
}

LLView::tree_post_iterator_t LLView::endTreeDFSPost() 
{ 
	// an empty iterator is an "end" iterator
	return tree_post_iterator_t();
}

LLView::bfs_tree_iterator_t LLView::beginTreeBFS() 
{ 
	return bfs_tree_iterator_t(this, 
							boost::bind(boost::mem_fn(&LLView::beginChild), _1), 
							boost::bind(boost::mem_fn(&LLView::endChild), _1)); 
}

LLView::bfs_tree_iterator_t LLView::endTreeBFS() 
{ 
	// an empty iterator is an "end" iterator
	return bfs_tree_iterator_t();
}


LLView::root_to_view_iterator_t LLView::beginRootToView()
{
	return root_to_view_iterator_t(this, boost::bind(&LLView::getParent, _1));
}

LLView::root_to_view_iterator_t LLView::endRootToView()
{
	return root_to_view_iterator_t();
}


// only create maps on demand, as they incur heap allocation/deallocation cost
// when a view is constructed/deconstructed
LLView& LLView::getDefaultWidgetContainer() const
{
	if (!mDefaultWidgets)
	{
		LLView::Params p;
		p.name = "default widget container";
		p.visible = false; // ensures default widgets can't steal focus, etc.
		mDefaultWidgets = new LLView(p);
	}
	return *mDefaultWidgets;
}

S32	LLView::notifyParent(const LLSD& info)
{
	LLView* parent = getParent();
	if(parent)
		return parent->notifyParent(info);
	return 0;
}
bool	LLView::notifyChildren(const LLSD& info)
{
	bool ret = false;
	BOOST_FOREACH(LLView* childp, mChildList)
	{
		ret = ret || childp->notifyChildren(info);
	}
	return ret;
}

// convenient accessor for draw context
const LLViewDrawContext& LLView::getDrawContext()
{
	return LLViewDrawContext::getCurrentContext();
}

const LLViewDrawContext& LLViewDrawContext::getCurrentContext()
{
	static LLViewDrawContext default_context;

	if (sDrawContextStack.empty())
		return default_context;
		
	return *sDrawContextStack.back();
}

LLSD LLView::getInfo(void)
{
	LLSD info;
	addInfo(info);
	return info;
}

void LLView::addInfo(LLSD & info)
{
	info["path"] = getPathname();
	info["class"] = typeid(*this).name();
	info["visible"] = getVisible();
	info["visible_chain"] = isInVisibleChain();
	info["enabled"] = getEnabled();
	info["enabled_chain"] = isInEnabledChain();
	info["available"] = isAvailable();
	LLRect rect(calcScreenRect());
	info["rect"] = LLSDMap("left", rect.mLeft)("top", rect.mTop)
				("right", rect.mRight)("bottom", rect.mBottom);
}
