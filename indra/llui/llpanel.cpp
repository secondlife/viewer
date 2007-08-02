/** 
 * @file llpanel.cpp
 * @brief LLPanel base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// Opaque view with a background and a border.  Can contain LLUICtrls.

#include "linden_common.h"

#include "llpanel.h"

#include "llalertdialog.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "lltimer.h"

#include "llmenugl.h"
//#include "llstatusbar.h"
#include "llui.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llcontrol.h"
#include "lltextbox.h"
#include "lluictrl.h"
#include "lluictrlfactory.h"
#include "llviewborder.h"
#include "llbutton.h"

// LLLayoutStack
#include "llgl.h"
#include "llglheaders.h"
#include "llresizebar.h"
#include "llcriticaldamp.h"

LLPanel::panel_map_t LLPanel::sPanelMap;
LLPanel::alert_queue_t LLPanel::sAlertQueue;

const S32 RESIZE_BAR_OVERLAP = 1;
const S32 PANEL_STACK_GAP = RESIZE_BAR_HEIGHT;

void LLPanel::init()
{
	// mRectControl
	mBgColorAlpha        = LLUI::sColorsGroup->getColor( "DefaultBackgroundColor" );
	mBgColorOpaque       = LLUI::sColorsGroup->getColor( "FocusBackgroundColor" );
	mDefaultBtnHighlight = LLUI::sColorsGroup->getColor( "DefaultHighlightLight" );
	mBgVisible = FALSE;
	mBgOpaque = FALSE;
	mBorder = NULL;
	mDefaultBtn = NULL;
	setIsChrome(FALSE); //is this a decorator to a live window or a form?
	mLastTabGroup = 0;

	// add self to handle->panel map
	sPanelMap[mViewHandle] = this;
	setTabStop(FALSE);
}

LLPanel::LLPanel()
: mRectControl()
{
	init();
}

LLPanel::LLPanel(const LLString& name)
:	LLUICtrl(name, LLRect(0, 0, 0, 0), TRUE, NULL, NULL),
	mRectControl()
{
	init();
}


LLPanel::LLPanel(const LLString& name, const LLRect& rect, BOOL bordered)
:	LLUICtrl(name, rect, TRUE, NULL, NULL),
	mRectControl()
{
	init();
	if (bordered)
	{
		addBorder();
	}
}


LLPanel::LLPanel(const LLString& name, const LLString& rect_control, BOOL bordered)
:	LLUICtrl(name, LLUI::sConfigGroup->getRect(rect_control), TRUE, NULL, NULL),
	mRectControl( rect_control )
{
	init();
	if (bordered)
	{
		addBorder();
	}
}

void LLPanel::addBorder(LLViewBorder::EBevel border_bevel,
						LLViewBorder::EStyle border_style, S32 border_thickness)
{
	mBorder = new LLViewBorder( "panel border", 
								LLRect(0, mRect.getHeight(), mRect.getWidth(), 0), 
								border_bevel, border_style, border_thickness );
	mBorder->setSaveToXML(false);
	addChild( mBorder );
}

void LLPanel::removeBorder()
{
	delete mBorder;
	mBorder = NULL;
}


LLPanel::~LLPanel()
{
	storeRectControl();
	sPanelMap.erase(mViewHandle);
}


// virtual
EWidgetType LLPanel::getWidgetType() const
{
	return WIDGET_TYPE_PANEL;
}

// virtual
LLString LLPanel::getWidgetTag() const
{
	return LL_PANEL_TAG;
}

// virtual
BOOL LLPanel::isPanel()
{
	return TRUE;
}

// virtual
BOOL LLPanel::postBuild()
{
	return TRUE;
}

// virtual
void LLPanel::clearCtrls()
{
	LLView::ctrl_list_t ctrls = getCtrlList();
	for (LLView::ctrl_list_t::iterator ctrl_it = ctrls.begin(); ctrl_it != ctrls.end(); ++ctrl_it)
	{
		LLUICtrl* ctrl = *ctrl_it;
		ctrl->setFocus( FALSE );
		ctrl->setEnabled( FALSE );
		ctrl->clear();
	}
}

void LLPanel::setCtrlsEnabled( BOOL b )
{
	LLView::ctrl_list_t ctrls = getCtrlList();
	for (LLView::ctrl_list_t::iterator ctrl_it = ctrls.begin(); ctrl_it != ctrls.end(); ++ctrl_it)
	{
		LLUICtrl* ctrl = *ctrl_it;
		ctrl->setEnabled( b );
	}
}

void LLPanel::draw()
{
	// draw background
	if( mBgVisible )
	{
		//RN: I don't see the point of this
		S32 left = 0;//LLPANEL_BORDER_WIDTH;
		S32 top = mRect.getHeight();// - LLPANEL_BORDER_WIDTH;
		S32 right = mRect.getWidth();// - LLPANEL_BORDER_WIDTH;
		S32 bottom = 0;//LLPANEL_BORDER_WIDTH;

		if (mBgOpaque )
		{
			gl_rect_2d( left, top, right, bottom, mBgColorOpaque );
		}
		else
		{
			gl_rect_2d( left, top, right, bottom, mBgColorAlpha );
		}
	}

	if( mDefaultBtn)
	{
		if (gFocusMgr.childHasKeyboardFocus( this ) && mDefaultBtn->getEnabled())
		{
			LLUICtrl* focus_ctrl = gFocusMgr.getKeyboardFocus();
			BOOL focus_is_child_button = focus_ctrl->getWidgetType() == WIDGET_TYPE_BUTTON && static_cast<LLButton *>(focus_ctrl)->getCommitOnReturn();
			// only enable default button when current focus is not a return-capturing button
			mDefaultBtn->setBorderEnabled(!focus_is_child_button);
		}
		else
		{
			mDefaultBtn->setBorderEnabled(FALSE);
		}
	}

	LLView::draw();
}

void LLPanel::refresh()
{
	// do nothing by default
	// but is automatically called in setFocus(TRUE)
}

void LLPanel::setDefaultBtn(LLButton* btn)
{
	if (mDefaultBtn && mDefaultBtn->getEnabled())
	{
		mDefaultBtn->setBorderEnabled(FALSE);
	}
	mDefaultBtn = btn; 
	if (mDefaultBtn)
	{
		mDefaultBtn->setBorderEnabled(TRUE);
	}
}

void LLPanel::setDefaultBtn(const LLString& id)
{
	LLButton *button = LLUICtrlFactory::getButtonByName(this, id);
	if (button)
	{
		setDefaultBtn(button);
	}
	else
	{
		setDefaultBtn(NULL);
	}
}

BOOL LLPanel::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	BOOL handled = FALSE;
	if (getVisible() && getEnabled())
	{
		if( (mask == MASK_SHIFT) && (KEY_TAB == key))	
		{
			//SHIFT-TAB
			LLView* cur_focus = gFocusMgr.getKeyboardFocus();
			if (cur_focus && gFocusMgr.childHasKeyboardFocus(this))
			{
				LLView* focus_root = cur_focus;
				while(cur_focus->getParent())
				{
					cur_focus = cur_focus->getParent();
					if (cur_focus->isFocusRoot())
					{
						// this is the root-most focus root found so far
						focus_root = cur_focus;
					}
				}
				handled = focus_root->focusPrevItem(FALSE);
			}
			else if (!cur_focus && mIsFocusRoot)
			{
				handled = focusLastItem();
				if (!handled)
				{
					setFocus(TRUE);
					handled = TRUE;
				}
			}
		}
		else
		if( (mask == MASK_NONE ) && (KEY_TAB == key))	
		{
			//TAB
			LLView* cur_focus = gFocusMgr.getKeyboardFocus();
			if (cur_focus && gFocusMgr.childHasKeyboardFocus(this))
			{
				LLView* focus_root = cur_focus;
				while(cur_focus->getParent())
				{
					cur_focus = cur_focus->getParent();
					if (cur_focus->isFocusRoot())
					{
						focus_root = cur_focus;
					}
				}
				handled = focus_root->focusNextItem(FALSE);
			}
			else if (!cur_focus && mIsFocusRoot)
			{
				handled = focusFirstItem();
				if (!handled)
				{
					setFocus(TRUE);
					handled = TRUE;
				}
			}
		}
	}

	if (!handled)
	{
		handled = LLView::handleKey(key, mask, called_from_parent);
	}

	return handled;
}

void LLPanel::addCtrl( LLUICtrl* ctrl, S32 tab_group)
{
	mLastTabGroup = tab_group;

	LLView::addCtrl(ctrl, tab_group);
	// propagate chrome to children only if they have not been flagged as chrome
	if (!ctrl->getIsChrome())
	{
		ctrl->setIsChrome(getIsChrome());
	}
}

void LLPanel::addCtrlAtEnd( LLUICtrl* ctrl, S32 tab_group)
{
	mLastTabGroup = tab_group;

	LLView::addCtrlAtEnd(ctrl, tab_group);
	if (!ctrl->getIsChrome())
	{
		ctrl->setIsChrome(getIsChrome());
	}
}

BOOL LLPanel::handleKeyHere( KEY key, MASK mask, BOOL called_from_parent )
{
	BOOL handled = FALSE;

	if( getVisible() && getEnabled() && gFocusMgr.childHasKeyboardFocus(this) && KEY_ESCAPE == key )
	{
		gFocusMgr.setKeyboardFocus(NULL, NULL);
		return TRUE;
	}

	if( getVisible() && getEnabled() && 
		gFocusMgr.childHasKeyboardFocus(this) && !called_from_parent )
	{
		LLUICtrl* cur_focus = gFocusMgr.getKeyboardFocus();
		// If we have a default button, click it when
		// return is pressed, unless current focus is a return-capturing button
		// in which case *that* button will handle the return key
		if (cur_focus && !(cur_focus->getWidgetType() == WIDGET_TYPE_BUTTON && static_cast<LLButton *>(cur_focus)->getCommitOnReturn()))
		{
			// RETURN key means hit default button in this case
			if (key == KEY_RETURN && mask == MASK_NONE 
				&& mDefaultBtn != NULL 
				&& mDefaultBtn->getVisible()
				&& mDefaultBtn->getEnabled())
			{
				mDefaultBtn->onCommit();
				handled = TRUE;
			}
		}

		if (key == KEY_RETURN && mask == MASK_NONE)
		{
			// set keyboard focus to self to trigger commitOnFocusLost behavior on current ctrl
			if (cur_focus && cur_focus->acceptsTextInput())
			{
				cur_focus->onCommit();
				handled = TRUE;
			}
		}
	}

	return handled;
}

void LLPanel::requires(LLString name, EWidgetType type)
{
	mRequirements[name] = type;
}

BOOL LLPanel::checkRequirements()
{
	BOOL retval = TRUE;
	LLString message;

	for (requirements_map_t::iterator i = mRequirements.begin(); i != mRequirements.end(); ++i)
	{
		if (!this->getCtrlByNameAndType(i->first, i->second))
		{
			retval = FALSE;
			message += i->first + " " + LLUICtrlFactory::getWidgetType(i->second) + "\n";
		}
	}

	if (!retval)
	{
		LLString::format_map_t args;
		args["[COMPONENTS]"] = message;
		args["[FLOATER]"] = getName();

		llwarns << getName() << " failed requirements check on: \n"  
				<< message << llendl;
			
		alertXml("FailedRequirementsCheck", args);
	}

	return retval;
}

//static
void LLPanel::alertXml(LLString label, LLString::format_map_t args)
{
	sAlertQueue.push(LLAlertInfo(label,args));
}

//static
BOOL LLPanel::nextAlert(LLAlertInfo &alert)
{
	if (!sAlertQueue.empty())
	{
		alert = sAlertQueue.front();
		sAlertQueue.pop();
		return TRUE;
	}

	return FALSE;
}

void LLPanel::setFocus(BOOL b)
{
	if( b )
	{
		if (!gFocusMgr.childHasKeyboardFocus(this))
		{
			//refresh();
			if (!focusFirstItem())
			{
				LLUICtrl::setFocus(TRUE);
			}
			onFocusReceived();
		}
	}
	else
	{
		if( this == gFocusMgr.getKeyboardFocus() )
		{
			gFocusMgr.setKeyboardFocus( NULL, NULL );
		}
		else
		{
			//RN: why is this here?
			LLView::ctrl_list_t ctrls = getCtrlList();
			for (LLView::ctrl_list_t::iterator ctrl_it = ctrls.begin(); ctrl_it != ctrls.end(); ++ctrl_it)
			{
				LLUICtrl* ctrl = *ctrl_it;
				ctrl->setFocus( FALSE );
			}
		}
	}
}

void LLPanel::setBackgroundColor(const LLColor4& color)
{
	mBgColorOpaque = color;
}

LLColor4 LLPanel::getBackgroundColor()
{
	return mBgColorOpaque;
}

void LLPanel::setTransparentColor(const LLColor4& color)
{
	mBgColorAlpha = color;
}

void LLPanel::setBorderVisible(BOOL b)
{
	if (mBorder)
	{
		mBorder->setVisible( b );
	}
}

LLView* LLPanel::getCtrlByNameAndType(const LLString& name, EWidgetType type)
{
	LLView* view = getChildByName(name, TRUE);
	if (view)
	{
		if (type ==	WIDGET_TYPE_DONTCARE || view->getWidgetType() == type)
		{
			return view;
		}
		else
		{
			llwarns << "Widget " << name << " has improper type in panel " << mName << "\n"
					<< "Is: \t\t" << view->getWidgetType() << "\n" 
					<< "Should be: \t" << type 
					<< llendl;
		}
	}
	else
	{
		childNotFound(name);
	}
	return NULL;
}

// static 
LLPanel* LLPanel::getPanelByHandle(LLViewHandle handle)
{
	if (!sPanelMap.count(handle))
	{
		return NULL;
	}
 
	return sPanelMap[handle];
}

// virtual
LLXMLNodePtr LLPanel::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLView::getXML();

	if (mBorder && mBorder->getVisible())
	{
		node->createChild("border", TRUE)->setBoolValue(TRUE);
	}

	if (!mRectControl.empty())
	{
		node->createChild("rect_control", TRUE)->setStringValue(mRectControl);
	}

	if (!mLabel.empty())
	{
		node->createChild("label", TRUE)->setStringValue(mLabel);
	}

	if (save_children)
	{
		LLView::child_list_const_reverse_iter_t rit;
		for (rit = getChildList()->rbegin(); rit != getChildList()->rend(); ++rit)
		{
			LLView* childp = *rit;

			if (childp->getSaveToXML())
			{
				LLXMLNodePtr xml_node = childp->getXML();

				node->addChild(xml_node);
			}
		}
	}

	return node;
}

LLView* LLPanel::fromXML(LLXMLNodePtr node, LLView* parent, LLUICtrlFactory *factory)
{
	LLString name("panel");
	node->getAttributeString("name", name);

	LLPanel* panelp = factory->createFactoryPanel(name);
	// Fall back on a default panel, if there was no special factory.
	if (!panelp)
	{
		LLRect rect;
		createRect(node, rect, parent, LLRect());
		panelp = new LLPanel(name, rect);
		panelp->initPanelXML(node, parent, factory);
		// preserve panel's width and height, but override the location
		const LLRect& panelrect = panelp->getRect();
		S32 w = panelrect.getWidth();
		S32 h = panelrect.getHeight();
		rect.setLeftTopAndSize(rect.mLeft, rect.mTop, w, h);
		panelp->setRect(rect);
	}
	else
	{
		panelp->initPanelXML(node, parent, factory);
	}
	return panelp;
}

BOOL LLPanel::initPanelXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("panel");
	node->getAttributeString("name", name);
	setName(name);

	setPanelParameters(node, parent);

	initChildrenXML(node, factory);

	LLString xml_filename;
	node->getAttributeString("filename", xml_filename);

	BOOL didPost;

	if (!xml_filename.empty())
	{
		// Preserve postion of embedded panel but allow panel to dictate width/height
		LLRect rect(getRect());
		didPost = factory->buildPanel(this, xml_filename, NULL);
		S32 w = getRect().getWidth();
		S32 h = getRect().getHeight();
		rect.setLeftTopAndSize(rect.mLeft, rect.mTop, w, h);
		setRect(rect);
	}
	else
	{
		didPost = FALSE;
	}
	
	if (!didPost)
	{
		postBuild();
		didPost = TRUE;
	}

	return didPost;
}

void LLPanel::initChildrenXML(LLXMLNodePtr node, LLUICtrlFactory* factory)
{
	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		// look for string declarations for programmatic text
		if (child->hasName("string"))
		{
			LLString string_name;
			child->getAttributeString("name", string_name);
			if (!string_name.empty())
			{
				mUIStrings[string_name] = LLUIString(child->getTextContents());
			}
		}
		else
		{
			factory->createWidget(this, child);
		}
	}
}

void LLPanel::setPanelParameters(LLXMLNodePtr node, LLView* parent)
{
	/////// Rect, follows, tool_tip, enabled, visible attributes ///////
	initFromXML(node, parent);

	/////// Border attributes ///////
	BOOL border = FALSE;
	node->getAttributeBOOL("border", border);
	if (border)
	{
		LLViewBorder::EBevel bevel_style = LLViewBorder::BEVEL_OUT;
		LLViewBorder::getBevelFromAttribute(node, bevel_style);

		LLViewBorder::EStyle border_style = LLViewBorder::STYLE_LINE;
		LLString border_string;
		node->getAttributeString("border_style", border_string);
		LLString::toLower(border_string);

		if (border_string == "texture")
		{
			border_style = LLViewBorder::STYLE_TEXTURE;
		}

		S32 border_thickness = LLPANEL_BORDER_WIDTH;
		node->getAttributeS32("border_thickness", border_thickness);

		addBorder(bevel_style, border_style, border_thickness);
	}
	else
	{
		removeBorder();
	}

	/////// Background attributes ///////
	BOOL background_visible = FALSE;
	node->getAttributeBOOL("background_visible", background_visible);
	setBackgroundVisible(background_visible);
	
	BOOL background_opaque = FALSE;
	node->getAttributeBOOL("background_opaque", background_opaque);
	setBackgroundOpaque(background_opaque);

	LLColor4 color;
	color = LLUI::sColorsGroup->getColor( "FocusBackgroundColor" );
	LLUICtrlFactory::getAttributeColor(node,"bg_opaque_color", color);
	setBackgroundColor(color);

	color = LLUI::sColorsGroup->getColor( "DefaultBackgroundColor" );
	LLUICtrlFactory::getAttributeColor(node,"bg_alpha_color", color);
	setTransparentColor(color);

	LLString label;
	node->getAttributeString("label", label);
	setLabel(label);
}

LLString LLPanel::getFormattedUIString(const LLString& name, const LLString::format_map_t& args) const
{
	ui_string_map_t::const_iterator found_it = mUIStrings.find(name);
	if (found_it != mUIStrings.end())
	{
		// make a copy as format works in place
		LLUIString formatted_string = found_it->second;
		formatted_string.setArgList(args);
		return formatted_string.getString();
	}
	return LLString::null;
}

LLUIString LLPanel::getUIString(const LLString& name) const
{
	ui_string_map_t::const_iterator found_it = mUIStrings.find(name);
	if (found_it != mUIStrings.end())
	{
		return found_it->second;
	}
	return LLUIString(LLString::null);
}


void LLPanel::childSetVisible(const LLString& id, bool visible)
{
	LLView* child = getChildByName(id, true);
	if (child)
	{
		child->setVisible(visible);
	}
}

bool LLPanel::childIsVisible(const LLString& id) const
{
	LLView* child = getChildByName(id, true);
	if (child)
	{
		return (bool)child->getVisible();
	}
	return false;
}

void LLPanel::childSetEnabled(const LLString& id, bool enabled)
{
	LLView* child = getChildByName(id, true);
	if (child)
	{
		child->setEnabled(enabled);
	}
}

void LLPanel::childSetTentative(const LLString& id, bool tentative)
{
	LLView* child = getChildByName(id, true);
	if (child)
	{
		child->setTentative(tentative);
	}
}

bool LLPanel::childIsEnabled(const LLString& id) const
{
	LLView* child = getChildByName(id, true);
	if (child)
	{
		return (bool)child->getEnabled();
	}
	return false;
}


void LLPanel::childSetToolTip(const LLString& id, const LLString& msg)
{
	LLView* child = getChildByName(id, true);
	if (child)
	{
		child->setToolTip(msg);
	}
}

void LLPanel::childSetRect(const LLString& id, const LLRect& rect)
{
	LLView* child = getChildByName(id, true);
	if (child)
	{
		child->setRect(rect);
	}
}

bool LLPanel::childGetRect(const LLString& id, LLRect& rect) const
{
	LLView* child = getChildByName(id, true);
	if (child)
	{
		rect = child->getRect();
		return true;
	}
	return false;
}

void LLPanel::childSetFocus(const LLString& id, BOOL focus)
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		child->setFocus(focus);
	}
}

BOOL LLPanel::childHasFocus(const LLString& id)
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		return child->hasFocus();
	}
	else
	{
		childNotFound(id);
		return FALSE;
	}
}


void LLPanel::childSetFocusChangedCallback(const LLString& id, void (*cb)(LLUICtrl*, void*))
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		child->setFocusChangedCallback(cb);
	}
}

void LLPanel::childSetCommitCallback(const LLString& id, void (*cb)(LLUICtrl*, void*), void *userdata )
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		child->setCommitCallback(cb);
		child->setCallbackUserData(userdata);
	}
}

void LLPanel::childSetDoubleClickCallback(const LLString& id, void (*cb)(void*), void *userdata )
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		child->setDoubleClickCallback(cb);
		if (userdata)
		{
			child->setCallbackUserData(userdata);
		}
	}
}

void LLPanel::childSetValidate(const LLString& id, BOOL (*cb)(LLUICtrl*, void*))
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		child->setValidateBeforeCommit(cb);
	}
}

void LLPanel::childSetUserData(const LLString& id, void* userdata)
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		child->setCallbackUserData(userdata);
	}
}

void LLPanel::childSetColor(const LLString& id, const LLColor4& color)
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		child->setColor(color);
	}
}

LLCtrlSelectionInterface* LLPanel::childGetSelectionInterface(const LLString& id)
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		return child->getSelectionInterface();
	}
	return NULL;
}

LLCtrlListInterface* LLPanel::childGetListInterface(const LLString& id)
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		return child->getListInterface();
	}
	return NULL;
}

LLCtrlScrollInterface* LLPanel::childGetScrollInterface(const LLString& id)
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		return child->getScrollInterface();
	}
	return NULL;
}

void LLPanel::childSetValue(const LLString& id, LLSD value)
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		child->setValue(value);
	}
}

LLSD LLPanel::childGetValue(const LLString& id) const
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		return child->getValue();
	}
	// Not found => return undefined
	return LLSD();
}

BOOL LLPanel::childSetTextArg(const LLString& id, const LLString& key, const LLString& text)
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		return child->setTextArg(key, text);
	}
	return FALSE;
}

BOOL LLPanel::childSetLabelArg(const LLString& id, const LLString& key, const LLString& text)
{
	LLView* child = getChildByName(id, true);
	if (child)
	{
		return child->setLabelArg(key, text);
	}
	return FALSE;
}

void LLPanel::childSetMinValue(const LLString& id, LLSD min_value)
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		child->setMinValue(min_value);
	}
}

void LLPanel::childSetMaxValue(const LLString& id, LLSD max_value)
{
	LLUICtrl* child = (LLUICtrl*)getChildByName(id, true);
	if (child)
	{
		child->setMaxValue(max_value);
	}
}

void LLPanel::childShowTab(const LLString& id, const LLString& tabname, bool visible)
{
	LLTabContainerCommon* child = LLUICtrlFactory::getTabContainerByName(this, id);
	if (child)
	{
		child->selectTabByName(tabname);
	}
}

LLPanel *LLPanel::childGetVisibleTab(const LLString& id)
{
	LLTabContainerCommon* child = LLUICtrlFactory::getTabContainerByName(this, id);
	if (child)
	{
		return child->getCurrentPanel();
	}
	return NULL;
}

void LLPanel::childSetTabChangeCallback(const LLString& id, const LLString& tabname, void (*on_tab_clicked)(void*, bool), void *userdata)
{
	LLTabContainerCommon* child = LLUICtrlFactory::getTabContainerByName(this, id);
	if (child)
	{
		LLPanel *panel = child->getPanelByName(tabname);
		if (panel)
		{
			child->setTabChangeCallback(panel, on_tab_clicked);
			child->setTabUserData(panel, userdata);
		}
	}
}

void LLPanel::childSetText(const LLString& id, const LLString& text)
{
	childSetValue(id, LLSD(text));
}

void LLPanel::childSetKeystrokeCallback(const LLString& id, void (*keystroke_callback)(LLLineEditor* caller, void* user_data), void *user_data)
{
	LLLineEditor* child = LLUICtrlFactory::getLineEditorByName(this, id);
	if (child)
	{
		child->setKeystrokeCallback(keystroke_callback);
		if (user_data)
		{
			child->setCallbackUserData(user_data);
		}
	}
}

void LLPanel::childSetPrevalidate(const LLString& id, BOOL (*func)(const LLWString &) )
{
	LLLineEditor* child = LLUICtrlFactory::getLineEditorByName(this, id);
	if (child)
	{
		child->setPrevalidate(func);
	}
}

LLString LLPanel::childGetText(const LLString& id)
{
	return childGetValue(id).asString();
}

void LLPanel::childSetWrappedText(const LLString& id, const LLString& text, bool visible)
{
	LLTextBox* child = (LLTextBox*)getCtrlByNameAndType(id, WIDGET_TYPE_TEXT_BOX);
	if (child)
	{
		child->setVisible(visible);
		child->setWrappedText(text);
	}
}

void LLPanel::childSetAction(const LLString& id, void(*function)(void*), void* value)
{
	LLButton* button = (LLButton*)getCtrlByNameAndType(id, WIDGET_TYPE_BUTTON);
	if (button)
	{
		button->setClickedCallback(function, value);
	}
}

void LLPanel::childSetActionTextbox(const LLString& id, void(*function)(void*))
{
	LLTextBox* textbox = (LLTextBox*)getCtrlByNameAndType(id, WIDGET_TYPE_TEXT_BOX);
	if (textbox)
	{
		textbox->setClickedCallback(function);
	}
}

void LLPanel::childSetControlName(const LLString& id, const LLString& control_name)
{
	LLView* view = getChildByName(id, TRUE);
	if (view)
	{
		view->setControlName(control_name, NULL);
	}
}

//virtual
LLView* LLPanel::getChildByName(const LLString& name, BOOL recurse) const
{
	LLView* view = LLUICtrl::getChildByName(name, recurse);
	if (!view)
	{
		childNotFound(name);
	}
	return view;
}

void LLPanel::childNotFound(const LLString& id) const
{
	if (mExpectedMembers.find(id) == mExpectedMembers.end())
	{
		mNewExpectedMembers.insert(id);
	}
}

void LLPanel::childDisplayNotFound()
{
	if (mNewExpectedMembers.empty())
	{
		return;
	}
	LLString msg;
	expected_members_list_t::iterator itor;
	for (itor=mNewExpectedMembers.begin(); itor!=mNewExpectedMembers.end(); ++itor)
	{
		msg.append(*itor);
		msg.append("\n");
		mExpectedMembers.insert(*itor);
	}
	mNewExpectedMembers.clear();
	LLString::format_map_t args;
	args["[CONTROLS]"] = msg;
	LLAlertDialog::showXml("FloaterNotFound", args);
}

void LLPanel::storeRectControl()
{
	if( !mRectControl.empty() )
	{
		LLUI::sConfigGroup->setRect( mRectControl, mRect );
	}
}


//
// LLLayoutStack
//
struct LLLayoutStack::LLEmbeddedPanel
{
	LLEmbeddedPanel(LLPanel* panelp, eLayoutOrientation orientation, S32 min_width, S32 min_height, BOOL auto_resize) : 
			mPanel(panelp), 
			mMinWidth(min_width), 
			mMinHeight(min_height),
			mAutoResize(auto_resize),
			mOrientation(orientation),
			mVisibleAmt(1.f) // default to fully visible
	{
		LLResizeBar::Side side = (orientation == HORIZONTAL) ? LLResizeBar::RIGHT : LLResizeBar::BOTTOM;
		LLRect resize_bar_rect = panelp->getRect();

		S32 min_dim;
		if (orientation == HORIZONTAL)
		{
			min_dim = mMinHeight;
		}
		else
		{
			min_dim = mMinWidth;
		}
		mResizeBar = new LLResizeBar("resizer", mPanel, LLRect(), min_dim, S32_MAX, side);
		mResizeBar->setEnableSnapping(FALSE);
		// panels initialized as hidden should not start out partially visible
		if (!mPanel->getVisible())
		{
			mVisibleAmt = 0.f;
		}
	}

	LLPanel* mPanel;
	S32 mMinWidth;
	S32 mMinHeight;
	BOOL mAutoResize;
	LLResizeBar* mResizeBar;
	eLayoutOrientation mOrientation;
	F32 mVisibleAmt;
};

LLLayoutStack::LLLayoutStack(eLayoutOrientation orientation) : 
		mOrientation(orientation),
		mMinWidth(0),
		mMinHeight(0)
{
}

LLLayoutStack::~LLLayoutStack()
{
}

void LLLayoutStack::draw()
{
	updateLayout();
	{
		// clip if outside nominal bounds
		LLLocalClipRect clip(getLocalRect(), mRect.getWidth() > mMinWidth || mRect.getHeight() > mMinHeight);
		e_panel_list_t::iterator panel_it;
		for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
		{
			LLRect clip_rect = (*panel_it)->mPanel->getRect();
			// scale clipping rectangle by visible amount
			if (mOrientation == HORIZONTAL)
			{
				clip_rect.mRight = clip_rect.mLeft + llround(clip_rect.getWidth() * (*panel_it)->mVisibleAmt);
			}
			else
			{
				clip_rect.mBottom = clip_rect.mTop - llround(clip_rect.getHeight() * (*panel_it)->mVisibleAmt);
			}
			LLLocalClipRect clip(clip_rect, (*panel_it)->mVisibleAmt < 1.f);
			// only force drawing invisible children if visible amount is non-zero
			drawChild((*panel_it)->mPanel, 0, 0, (*panel_it)->mVisibleAmt > 0.f);
		}
	}
}

void LLLayoutStack::removeCtrl(LLUICtrl* ctrl)
{
	LLEmbeddedPanel* embedded_panelp = findEmbeddedPanel((LLPanel*)ctrl);

	if (embedded_panelp)
	{
		mPanels.erase(std::find(mPanels.begin(), mPanels.end(), embedded_panelp));
		delete embedded_panelp;
	}

	calcMinExtents();

	LLView::removeCtrl(ctrl);
}

void LLLayoutStack::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLView::reshape(width, height, called_from_parent);
	//updateLayout();
}

LLXMLNodePtr LLLayoutStack::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLView::getXML();
	return node;
}

//static 
LLView* LLLayoutStack::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString orientation_string("vertical");
	node->getAttributeString("orientation", orientation_string);

	eLayoutOrientation orientation = VERTICAL;

	if (orientation_string == "horizontal")
	{
		orientation = HORIZONTAL;
	}
	else if (orientation_string == "vertical")
	{
		orientation = VERTICAL;
	}
	else
	{
		llwarns << "Unknown orientation " << orientation_string << ", using vertical" << llendl;
	}

	LLLayoutStack* layout_stackp = new LLLayoutStack(orientation);

	layout_stackp->initFromXML(node, parent);

	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("layout_panel"))
		{
			S32 min_width = 0;
			S32 min_height = 0;
			BOOL auto_resize = TRUE;

			child->getAttributeS32("min_width", min_width);
			child->getAttributeS32("min_height", min_height);
			child->getAttributeBOOL("auto_resize", auto_resize);

			LLPanel* panelp = (LLPanel*)LLPanel::fromXML(child, layout_stackp, factory);
			panelp->setFollowsNone();
			if (panelp)
			{
				layout_stackp->addPanel(panelp, min_width, min_height, auto_resize);
			}
		}
	}

	return layout_stackp;
}

S32 LLLayoutStack::getMinWidth()
{
	return mMinWidth;
}

S32 LLLayoutStack::getMinHeight()
{
	return mMinHeight;
}

void LLLayoutStack::addPanel(LLPanel* panel, S32 min_width, S32 min_height, BOOL auto_resize, S32 index)
{
	LLEmbeddedPanel* embedded_panel = new LLEmbeddedPanel(panel, mOrientation, min_width, min_height, auto_resize);
	
	mPanels.insert(mPanels.begin() + llclamp(index, 0, (S32)mPanels.size()), embedded_panel);
	addChild(panel);
	addChild(embedded_panel->mResizeBar);

	// bring all resize bars to the front so that they are clickable even over the panels
	// with a bit of overlap
	for (e_panel_list_t::iterator panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		e_panel_list_t::iterator next_it = panel_it;
		++next_it;

		LLResizeBar* resize_barp = (*panel_it)->mResizeBar;
		sendChildToFront(resize_barp);
		// last resize bar is disabled, since its not between any two panels
		if ( next_it == mPanels.end() )
		{
			resize_barp->setEnabled(FALSE);
		}
		else
		{
			resize_barp->setEnabled(TRUE);
		}
	}

	//updateLayout();
}

void LLLayoutStack::removePanel(LLPanel* panel)
{
	removeChild(panel);
	//updateLayout();
}

void LLLayoutStack::updateLayout(BOOL force_resize)
{
	calcMinExtents();

	// calculate current extents
	S32 cur_width = 0;
	S32 cur_height = 0;

	const F32 ANIM_OPEN_TIME = 0.02f;
	const F32 ANIM_CLOSE_TIME = 0.02f;

	e_panel_list_t::iterator panel_it;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end();	++panel_it)
	{
		LLPanel* panelp = (*panel_it)->mPanel;
		if (panelp->getVisible()) 
		{
			(*panel_it)->mVisibleAmt = lerp((*panel_it)->mVisibleAmt, 1.f, LLCriticalDamp::getInterpolant(ANIM_OPEN_TIME));
			if ((*panel_it)->mVisibleAmt > 0.99f)
			{
				(*panel_it)->mVisibleAmt = 1.f;
			}
		}
		else // not visible
		{
			(*panel_it)->mVisibleAmt = lerp((*panel_it)->mVisibleAmt, 0.f, LLCriticalDamp::getInterpolant(ANIM_CLOSE_TIME));
			if ((*panel_it)->mVisibleAmt < 0.001f)
			{
				(*panel_it)->mVisibleAmt = 0.f;
			}
		}
		if (mOrientation == HORIZONTAL)
		{
 			// all panels get expanded to max of all the minimum dimensions
			cur_height = llmax(mMinHeight, panelp->getRect().getHeight());
        	cur_width += llround(panelp->getRect().getWidth() * (*panel_it)->mVisibleAmt);
			if (panel_it != mPanels.end())
			{
				cur_width += PANEL_STACK_GAP;
			}
		}
		else //VERTICAL
		{
            cur_width = llmax(mMinWidth, panelp->getRect().getWidth());
			cur_height += llround(panelp->getRect().getHeight() * (*panel_it)->mVisibleAmt);
			if (panel_it != mPanels.end())
			{
				cur_height += PANEL_STACK_GAP;
			}
		}
	}

	S32 num_resizable_panels = 0;
	S32 shrink_headroom_available = 0;
	S32 shrink_headroom_total = 0;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		// panels that are not fully visible do not count towards shrink headroom
		if ((*panel_it)->mVisibleAmt < 1.f) 
			continue;
		// if currently resizing a panel or the panel is flagged as not automatically resizing
		// only track total available headroom, but don't use it for automatic resize logic
		if ((*panel_it)->mResizeBar->hasMouseCapture() || (!(*panel_it)->mAutoResize && !force_resize))
		{
			if (mOrientation == HORIZONTAL)
			{
				shrink_headroom_total += (*panel_it)->mPanel->getRect().getWidth() - (*panel_it)->mMinWidth;
			}
			else //VERTICAL
			{
				shrink_headroom_total += (*panel_it)->mPanel->getRect().getHeight() - (*panel_it)->mMinHeight;
			}
		}
		else
		{
			num_resizable_panels++;
			if (mOrientation == HORIZONTAL)
			{
				shrink_headroom_available += (*panel_it)->mPanel->getRect().getWidth() - (*panel_it)->mMinWidth;
				shrink_headroom_total += (*panel_it)->mPanel->getRect().getWidth() - (*panel_it)->mMinWidth;
			}
			else //VERTICAL
			{
				shrink_headroom_available += (*panel_it)->mPanel->getRect().getHeight() - (*panel_it)->mMinHeight;
				shrink_headroom_total += (*panel_it)->mPanel->getRect().getHeight() - (*panel_it)->mMinHeight;
			}
		}
	}

	// positive means panels need to grow, negative means shrink
	S32 pixels_to_distribute;
	if (mOrientation == HORIZONTAL)
	{
		pixels_to_distribute = mRect.getWidth() - cur_width;
	}
	else //VERTICAL
	{
		pixels_to_distribute = mRect.getHeight() - cur_height;
	}

	S32 cur_x = 0;
	S32 cur_y = mRect.getHeight();

	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		LLPanel* panelp = (*panel_it)->mPanel;

		S32 cur_width = panelp->getRect().getWidth();
		S32 cur_height = panelp->getRect().getHeight();
		S32 new_width = llmax((*panel_it)->mMinWidth, cur_width);
		S32 new_height = llmax((*panel_it)->mMinHeight, cur_height);

		S32 delta_size = 0;

		// if panel can automatically resize (not animating, and resize flag set)...
		if ((*panel_it)->mVisibleAmt == 1.f && (force_resize || (*panel_it)->mAutoResize) && !(*panel_it)->mResizeBar->hasMouseCapture()) 
		{
			if (mOrientation == HORIZONTAL)
			{
				// if we're shrinking
				if (pixels_to_distribute < 0)
				{
					// shrink proportionally to amount over minimum
					delta_size = llround((F32)pixels_to_distribute * (F32)(cur_width - (*panel_it)->mMinWidth) / (F32)shrink_headroom_available);
				}
				else
				{
					// grow all elements equally
					delta_size = llround((F32)pixels_to_distribute / (F32)num_resizable_panels);
				}
				new_width = llmax((*panel_it)->mMinWidth, panelp->getRect().getWidth() + delta_size);
			}
			else
			{
				new_width = llmax(mMinWidth, mRect.getWidth());
			}

			if (mOrientation == VERTICAL)
			{
				if (pixels_to_distribute < 0)
				{
					// shrink proportionally to amount over minimum
					delta_size = llround((F32)pixels_to_distribute * (F32)(cur_height - (*panel_it)->mMinHeight) / (F32)shrink_headroom_available);
				}
				else
				{
					delta_size = llround((F32)pixels_to_distribute / (F32)num_resizable_panels);
				}
				new_height = llmax((*panel_it)->mMinHeight, panelp->getRect().getHeight() + delta_size);
			}
			else
			{
				new_height = llmax(mMinHeight, mRect.getHeight());
			}
		}
		else // don't resize
		{
			if (mOrientation == HORIZONTAL)
			{
				new_height = llmax(mMinHeight, mRect.getHeight());
			}
			else // VERTICAL
			{
				new_width = llmax(mMinWidth, mRect.getWidth());
			}
		}

		// adjust running headroom count based on new sizes
		shrink_headroom_total += delta_size;

		panelp->reshape(new_width, new_height);
		panelp->setOrigin(cur_x, cur_y - new_height);

		LLRect panel_rect = panelp->getRect();
		LLRect resize_bar_rect = panel_rect;
		if (mOrientation == HORIZONTAL)
		{
			resize_bar_rect.mLeft = panel_rect.mRight - RESIZE_BAR_OVERLAP;
			resize_bar_rect.mRight = panel_rect.mRight + PANEL_STACK_GAP + RESIZE_BAR_OVERLAP;
		}
		else
		{
			resize_bar_rect.mTop = panel_rect.mBottom + RESIZE_BAR_OVERLAP;
			resize_bar_rect.mBottom = panel_rect.mBottom - PANEL_STACK_GAP - RESIZE_BAR_OVERLAP;
		}
		(*panel_it)->mResizeBar->setRect(resize_bar_rect);

		if (mOrientation == HORIZONTAL)
		{
			cur_x += llround(new_width * (*panel_it)->mVisibleAmt) + PANEL_STACK_GAP;
		}
		else //VERTICAL
		{
			cur_y -= llround(new_height * (*panel_it)->mVisibleAmt) + PANEL_STACK_GAP;
		}
	}

	// update resize bars with new limits
	LLResizeBar* last_resize_bar = NULL;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		LLPanel* panelp = (*panel_it)->mPanel;

		if (mOrientation == HORIZONTAL)
		{
			(*panel_it)->mResizeBar->setResizeLimits((*panel_it)->mMinWidth, (*panel_it)->mMinWidth + shrink_headroom_total);
		}
		else //VERTICAL
		{
			(*panel_it)->mResizeBar->setResizeLimits((*panel_it)->mMinHeight, (*panel_it)->mMinHeight + shrink_headroom_total);
		}
		// hide resize bars for invisible panels
		(*panel_it)->mResizeBar->setVisible(panelp->getVisible());
		if (panelp->getVisible())
		{
			last_resize_bar = (*panel_it)->mResizeBar;
		}
	}

	// hide last resize bar as there is nothing past it
	if (last_resize_bar)
	{
		last_resize_bar->setVisible(FALSE);
	}

	// not enough room to fit existing contents
	if (!force_resize && 
		((cur_y != -PANEL_STACK_GAP) || (cur_x != mRect.getWidth() + PANEL_STACK_GAP)))
	{
		// do another layout pass with all stacked elements contributing
		// even those that don't usually resize
		llassert_always(force_resize == FALSE);
		updateLayout(TRUE);
	}
}

LLLayoutStack::LLEmbeddedPanel* LLLayoutStack::findEmbeddedPanel(LLPanel* panelp)
{
	e_panel_list_t::iterator panel_it;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		if ((*panel_it)->mPanel == panelp)
		{
			return *panel_it;
		}
	}
	return NULL;
}

void LLLayoutStack::calcMinExtents()
{
	mMinWidth = 0;
	mMinHeight = 0;

	e_panel_list_t::iterator panel_it;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		if (mOrientation == HORIZONTAL)
		{
			mMinHeight = llmax(mMinHeight, (*panel_it)->mMinHeight);
            mMinWidth += (*panel_it)->mMinWidth;
			if (panel_it != mPanels.begin())
			{
				mMinWidth += PANEL_STACK_GAP;
			}
		}
		else //VERTICAL
		{
            mMinWidth = llmax(mMinWidth, (*panel_it)->mMinWidth);
			mMinHeight += (*panel_it)->mMinHeight;
			if (panel_it != mPanels.begin())
			{
				mMinHeight += PANEL_STACK_GAP;
			}
		}
	}
}
