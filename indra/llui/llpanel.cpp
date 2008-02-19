/** 
 * @file llpanel.cpp
 * @brief LLPanel base class
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

LLPanel::alert_queue_t LLPanel::sAlertQueue;

const S32 RESIZE_BAR_OVERLAP = 1;
const S32 RESIZE_BAR_HEIGHT = 3;

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

	mPanelHandle.bind(this);
	setTabStop(FALSE);
}

LLPanel::LLPanel()
: mRectControl()
{
	init();
	setName("panel");
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

LLPanel::~LLPanel()
{
	storeRectControl();
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
BOOL LLPanel::isPanel() const
{
	return TRUE;
}

// virtual
BOOL LLPanel::postBuild()
{
	return TRUE;
}

void LLPanel::addBorder(LLViewBorder::EBevel border_bevel,
						LLViewBorder::EStyle border_style, S32 border_thickness)
{
	removeBorder();
	mBorder = new LLViewBorder( "panel border", 
								LLRect(0, getRect().getHeight(), getRect().getWidth(), 0), 
								border_bevel, border_style, border_thickness );
	mBorder->setSaveToXML(false);
	addChild( mBorder );
}

void LLPanel::removeBorder()
{
	delete mBorder;
	mBorder = NULL;
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
		S32 top = getRect().getHeight();// - LLPANEL_BORDER_WIDTH;
		S32 right = getRect().getWidth();// - LLPANEL_BORDER_WIDTH;
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

	updateDefaultBtn();
}

void LLPanel::updateDefaultBtn()
{
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
			LLUICtrl* cur_focus = gFocusMgr.getKeyboardFocus();
			if (cur_focus && gFocusMgr.childHasKeyboardFocus(this))
			{
				LLUICtrl* focus_root = cur_focus;
				while(cur_focus->getParentUICtrl())
				{
					cur_focus = cur_focus->getParentUICtrl();
					if (cur_focus->isFocusRoot())
					{
						// this is the root-most focus root found so far
						focus_root = cur_focus;
					}
				}
				handled = focus_root->focusPrevItem(FALSE);
			}
			else if (!cur_focus && isFocusRoot())
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
			LLUICtrl* cur_focus = gFocusMgr.getKeyboardFocus();
			if (cur_focus && gFocusMgr.childHasKeyboardFocus(this))
			{
				LLUICtrl* focus_root = cur_focus;
				while(cur_focus->getParentUICtrl())
				{
					cur_focus = cur_focus->getParentUICtrl();
					if (cur_focus->isFocusRoot())
					{
						focus_root = cur_focus;
					}
				}
				handled = focus_root->focusNextItem(FALSE);
			}
			else if (!cur_focus && isFocusRoot())
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
}

void LLPanel::addCtrlAtEnd( LLUICtrl* ctrl, S32 tab_group)
{
	mLastTabGroup = tab_group;

	LLView::addCtrlAtEnd(ctrl, tab_group);
}

BOOL LLPanel::handleKeyHere( KEY key, MASK mask, BOOL called_from_parent )
{
	BOOL handled = FALSE;

	if( getVisible() && getEnabled() && 
		gFocusMgr.childHasKeyboardFocus(this) && !called_from_parent )
	{
		// handle user hitting ESC to defocus
		if (key == KEY_ESCAPE)
		{
			gFocusMgr.setKeyboardFocus(NULL);
			return TRUE;
		}

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

BOOL LLPanel::checkRequirements() const
{
	BOOL retval = TRUE;
	LLString message;

	for (requirements_map_t::const_iterator i = mRequirements.begin(); i != mRequirements.end(); ++i)
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
			gFocusMgr.setKeyboardFocus( NULL );
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

void LLPanel::setBorderVisible(BOOL b)
{
	if (mBorder)
	{
		mBorder->setVisible( b );
	}
}

LLUICtrl* LLPanel::getCtrlByNameAndType(const LLString& name, EWidgetType type) const
{
	LLView* view = getChildByName(name, TRUE);
	if (view && view->isCtrl())
	{
		if (type ==	WIDGET_TYPE_DONTCARE || view->getWidgetType() == type)
		{
			return (LLUICtrl*)view;
		}
		else
		{
			llwarns << "Widget " << name << " has improper type in panel " << getName() << "\n"
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
		// create a new panel without a border, by default
		panelp = new LLPanel(name, rect, FALSE);
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
	LLString name = getName();
	node->getAttributeString("name", name);
	setName(name);

	setPanelParameters(node, parent);

	initChildrenXML(node, factory);

	LLString xml_filename;
	node->getAttributeString("filename", xml_filename);

	BOOL didPost;

	if (!xml_filename.empty())
	{
		didPost = factory->buildPanel(this, xml_filename, NULL);

		LLRect new_rect = getRect();
		// override rectangle with embedding parameters as provided
		createRect(node, new_rect, parent);
		setOrigin(new_rect.mLeft, new_rect.mBottom);
		reshape(new_rect.getWidth(), new_rect.getHeight());
		// optionally override follows flags from including nodes
		parseFollowsFlags(node);
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
	BOOL border = mBorder != NULL;
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
	BOOL background_visible = mBgVisible;
	node->getAttributeBOOL("background_visible", background_visible);
	setBackgroundVisible(background_visible);
	
	BOOL background_opaque = mBgOpaque;
	node->getAttributeBOOL("background_opaque", background_opaque);
	setBackgroundOpaque(background_opaque);

	LLColor4 color;
	color = mBgColorOpaque;
	LLUICtrlFactory::getAttributeColor(node,"bg_opaque_color", color);
	setBackgroundColor(color);

	color = mBgColorAlpha;
	LLUICtrlFactory::getAttributeColor(node,"bg_alpha_color", color);
	setTransparentColor(color);

	LLString label = getLabel();
	node->getAttributeString("label", label);
	setLabel(label);
}

LLString LLPanel::getString(const LLString& name, const LLString::format_map_t& args) const
{
	ui_string_map_t::const_iterator found_it = mUIStrings.find(name);
	if (found_it != mUIStrings.end())
	{
		// make a copy as format works in place
		LLUIString formatted_string = found_it->second;
		formatted_string.setArgList(args);
		return formatted_string.getString();
	}
	llerrs << "Failed to find string " << name << " in panel " << getName() << llendl;
	return LLString::null;
}

LLUIString LLPanel::getUIString(const LLString& name) const
{
	ui_string_map_t::const_iterator found_it = mUIStrings.find(name);
	if (found_it != mUIStrings.end())
	{
		return found_it->second;
	}
	llerrs << "Failed to find string " << name << " in panel " << getName() << llendl;
	return LLUIString(LLString::null);
}


void LLPanel::childSetVisible(const LLString& id, bool visible)
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		child->setVisible(visible);
	}
}

bool LLPanel::childIsVisible(const LLString& id) const
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		return (bool)child->getVisible();
	}
	return false;
}

void LLPanel::childSetEnabled(const LLString& id, bool enabled)
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		child->setEnabled(enabled);
	}
}

void LLPanel::childSetTentative(const LLString& id, bool tentative)
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		child->setTentative(tentative);
	}
}

bool LLPanel::childIsEnabled(const LLString& id) const
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		return (bool)child->getEnabled();
	}
	return false;
}


void LLPanel::childSetToolTip(const LLString& id, const LLString& msg)
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		child->setToolTip(msg);
	}
}

void LLPanel::childSetRect(const LLString& id, const LLRect& rect)
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		child->setRect(rect);
	}
}

bool LLPanel::childGetRect(const LLString& id, LLRect& rect) const
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		rect = child->getRect();
		return true;
	}
	return false;
}

void LLPanel::childSetFocus(const LLString& id, BOOL focus)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setFocus(focus);
	}
}

BOOL LLPanel::childHasFocus(const LLString& id)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
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


void LLPanel::childSetFocusChangedCallback(const LLString& id, void (*cb)(LLFocusableElement*, void*), void* user_data)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setFocusChangedCallback(cb, user_data);
	}
}

void LLPanel::childSetCommitCallback(const LLString& id, void (*cb)(LLUICtrl*, void*), void *userdata )
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setCommitCallback(cb);
		child->setCallbackUserData(userdata);
	}
}

void LLPanel::childSetDoubleClickCallback(const LLString& id, void (*cb)(void*), void *userdata )
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
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
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setValidateBeforeCommit(cb);
	}
}

void LLPanel::childSetUserData(const LLString& id, void* userdata)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setCallbackUserData(userdata);
	}
}

void LLPanel::childSetColor(const LLString& id, const LLColor4& color)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setColor(color);
	}
}

LLCtrlSelectionInterface* LLPanel::childGetSelectionInterface(const LLString& id) const
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		return child->getSelectionInterface();
	}
	return NULL;
}

LLCtrlListInterface* LLPanel::childGetListInterface(const LLString& id) const
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		return child->getListInterface();
	}
	return NULL;
}

LLCtrlScrollInterface* LLPanel::childGetScrollInterface(const LLString& id) const
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		return child->getScrollInterface();
	}
	return NULL;
}

void LLPanel::childSetValue(const LLString& id, LLSD value)
{
	LLView* child = getChild<LLView>(id, true);
	if (child)
	{
		child->setValue(value);
	}
}

LLSD LLPanel::childGetValue(const LLString& id) const
{
	LLView* child = getChild<LLView>(id, true);
	if (child)
	{
		return child->getValue();
	}
	// Not found => return undefined
	return LLSD();
}

BOOL LLPanel::childSetTextArg(const LLString& id, const LLString& key, const LLStringExplicit& text)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		return child->setTextArg(key, text);
	}
	return FALSE;
}

BOOL LLPanel::childSetLabelArg(const LLString& id, const LLString& key, const LLStringExplicit& text)
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		return child->setLabelArg(key, text);
	}
	return FALSE;
}

BOOL LLPanel::childSetToolTipArg(const LLString& id, const LLString& key, const LLStringExplicit& text)
{
	LLView* child = getChildByName(id, true);
	if (child)
	{
		return child->setToolTipArg(key, text);
	}
	return FALSE;
}

void LLPanel::childSetMinValue(const LLString& id, LLSD min_value)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setMinValue(min_value);
	}
}

void LLPanel::childSetMaxValue(const LLString& id, LLSD max_value)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setMaxValue(max_value);
	}
}

void LLPanel::childShowTab(const LLString& id, const LLString& tabname, bool visible)
{
	LLTabContainer* child = LLUICtrlFactory::getTabContainerByName(this, id);
	if (child)
	{
		child->selectTabByName(tabname);
	}
}

LLPanel *LLPanel::childGetVisibleTab(const LLString& id) const
{
	LLTabContainer* child = LLUICtrlFactory::getTabContainerByName(this, id);
	if (child)
	{
		return child->getCurrentPanel();
	}
	return NULL;
}

void LLPanel::childSetTabChangeCallback(const LLString& id, const LLString& tabname, void (*on_tab_clicked)(void*, bool), void *userdata)
{
	LLTabContainer* child = LLUICtrlFactory::getTabContainerByName(this, id);
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
	LLView* view = getChild<LLView>(id);
	if (view)
	{
		view->setControlName(control_name, NULL);
	}
}

//virtual
LLView* LLPanel::getChildByName(const LLString& name, BOOL recurse) const
{
	LLView* view = LLUICtrl::getChildByName(name, recurse);
	if (!view && !recurse)
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
		LLUI::sConfigGroup->setRect( mRectControl, getRect() );
	}
}


//
// LLLayoutStack
//
struct LLLayoutStack::LLEmbeddedPanel
{
	LLEmbeddedPanel(LLPanel* panelp, eLayoutOrientation orientation, S32 min_width, S32 min_height, BOOL auto_resize, BOOL user_resize) : 
			mPanel(panelp), 
			mMinWidth(min_width), 
			mMinHeight(min_height),
			mAutoResize(auto_resize),
			mUserResize(user_resize),
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

	~LLEmbeddedPanel()
	{
		// probably not necessary, but...
		delete mResizeBar;
		mResizeBar = NULL;
	}

	LLPanel* mPanel;
	S32 mMinWidth;
	S32 mMinHeight;
	BOOL mAutoResize;
	BOOL mUserResize;
	LLResizeBar* mResizeBar;
	eLayoutOrientation mOrientation;
	F32 mVisibleAmt;
};

LLLayoutStack::LLLayoutStack(eLayoutOrientation orientation) : 
		mOrientation(orientation),
		mMinWidth(0),
		mMinHeight(0),
		mPanelSpacing(RESIZE_BAR_HEIGHT)
{
}

LLLayoutStack::~LLLayoutStack()
{
	std::for_each(mPanels.begin(), mPanels.end(), DeletePointer());
}

// virtual
EWidgetType LLLayoutStack::getWidgetType() const
{
	return WIDGET_TYPE_LAYOUT_STACK;
}

// virtual
LLString LLLayoutStack::getWidgetTag() const
{
	return LL_LAYOUT_STACK_TAG;
}


void LLLayoutStack::draw()
{
	updateLayout();
	{
		e_panel_list_t::iterator panel_it;
		for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
		{
			// clip to layout rectangle, not bounding rectangle
			LLRect clip_rect = (*panel_it)->mPanel->getRect();
			// scale clipping rectangle by visible amount
			if (mOrientation == HORIZONTAL)
			{
				clip_rect.mRight = clip_rect.mLeft + llround((F32)clip_rect.getWidth() * (*panel_it)->mVisibleAmt);
			}
			else
			{
				clip_rect.mBottom = clip_rect.mTop - llround((F32)clip_rect.getHeight() * (*panel_it)->mVisibleAmt);
			}

			LLPanel* panelp = (*panel_it)->mPanel;

			LLLocalClipRect clip(clip_rect);
			// only force drawing invisible children if visible amount is non-zero
			drawChild(panelp, 0, 0, !clip_rect.isNull());
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

	// need to update resizebars

	calcMinExtents();

	LLView::removeCtrl(ctrl);
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

	node->getAttributeS32("border_size", layout_stackp->mPanelSpacing);
	// don't allow negative spacing values
	layout_stackp->mPanelSpacing = llmax(layout_stackp->mPanelSpacing, 0);

	LLString name("stack");
	node->getAttributeString("name", name);

	layout_stackp->setName(name);
	layout_stackp->initFromXML(node, parent);

	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("layout_panel"))
		{
			S32 min_width = 0;
			S32 min_height = 0;
			BOOL auto_resize = TRUE;
			BOOL user_resize = TRUE;

			child->getAttributeS32("min_width", min_width);
			child->getAttributeS32("min_height", min_height);
			child->getAttributeBOOL("auto_resize", auto_resize);
			child->getAttributeBOOL("user_resize", user_resize);

			LLPanel* panelp = (LLPanel*)LLPanel::fromXML(child, layout_stackp, factory);
			if (panelp)
			{
				panelp->setFollowsNone();
				layout_stackp->addPanel(panelp, min_width, min_height, auto_resize, user_resize);
			}
		}
	}
	layout_stackp->updateLayout();

	return layout_stackp;
}

S32 LLLayoutStack::getDefaultHeight(S32 cur_height)
{
	// if we are spanning our children (crude upward propagation of size)
	// then don't enforce our size on our children
	if (mOrientation == HORIZONTAL)
	{
		cur_height = llmax(mMinHeight, getRect().getHeight());
	}

	return cur_height;
}

S32 LLLayoutStack::getDefaultWidth(S32 cur_width)
{
	// if we are spanning our children (crude upward propagation of size)
	// then don't enforce our size on our children
	if (mOrientation == VERTICAL)
	{
		cur_width = llmax(mMinWidth, getRect().getWidth());
	}

	return cur_width;
}

void LLLayoutStack::addPanel(LLPanel* panel, S32 min_width, S32 min_height, BOOL auto_resize, BOOL user_resize, S32 index)
{
	LLEmbeddedPanel* embedded_panel = new LLEmbeddedPanel(panel, mOrientation, min_width, min_height, auto_resize, user_resize);
	
	mPanels.insert(mPanels.begin() + llclamp(index, 0, (S32)mPanels.size()), embedded_panel);
	
	addChild(panel);
	addChild(embedded_panel->mResizeBar);

	// bring all resize bars to the front so that they are clickable even over the panels
	// with a bit of overlap
	for (e_panel_list_t::iterator panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		LLResizeBar* resize_barp = (*panel_it)->mResizeBar;
		sendChildToFront(resize_barp);
	}

}

void LLLayoutStack::removePanel(LLPanel* panel)
{
	removeChild(panel);
}

void LLLayoutStack::updateLayout(BOOL force_resize)
{
	calcMinExtents();

	// calculate current extents
	S32 total_width = 0;
	S32 total_height = 0;

	const F32 ANIM_OPEN_TIME = 0.02f;
	const F32 ANIM_CLOSE_TIME = 0.03f;

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
        	total_width += llround(panelp->getRect().getWidth() * (*panel_it)->mVisibleAmt);
        	// want n-1 panel gaps for n panels
			if (panel_it != mPanels.begin())
			{
				total_width += mPanelSpacing;
			}
		}
		else //VERTICAL
		{
			total_height += llround(panelp->getRect().getHeight() * (*panel_it)->mVisibleAmt);
			if (panel_it != mPanels.begin())
			{
				total_height += mPanelSpacing;
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
		{
			continue;
		}
		// if currently resizing a panel or the panel is flagged as not automatically resizing
		// only track total available headroom, but don't use it for automatic resize logic
		if ((*panel_it)->mResizeBar->hasMouseCapture() 
			|| (!(*panel_it)->mAutoResize 
				&& !force_resize))
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
		pixels_to_distribute = getRect().getWidth() - total_width;
	}
	else //VERTICAL
	{
		pixels_to_distribute = getRect().getHeight() - total_height;
	}

	S32 cur_x = 0;
	S32 cur_y = getRect().getHeight();

	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		LLPanel* panelp = (*panel_it)->mPanel;

		S32 cur_width = panelp->getRect().getWidth();
		S32 cur_height = panelp->getRect().getHeight();
		S32 new_width = llmax((*panel_it)->mMinWidth, cur_width);
		S32 new_height = llmax((*panel_it)->mMinHeight, cur_height); 

		S32 delta_size = 0;

		// if panel can automatically resize (not animating, and resize flag set)...
		if ((*panel_it)->mVisibleAmt == 1.f 
			&& (force_resize || (*panel_it)->mAutoResize) 
			&& !(*panel_it)->mResizeBar->hasMouseCapture()) 
		{
			if (mOrientation == HORIZONTAL)
			{
				// if we're shrinking
				if (pixels_to_distribute < 0)
				{
					// shrink proportionally to amount over minimum
					delta_size = (shrink_headroom_available > 0) ? llround((F32)pixels_to_distribute * (F32)(cur_width - (*panel_it)->mMinWidth) / (F32)shrink_headroom_available) : 0;
				}
				else
				{
					// grow all elements equally
					delta_size = llround((F32)pixels_to_distribute / (F32)num_resizable_panels);
				}
				new_width = llmax((*panel_it)->mMinWidth, cur_width + delta_size);
			}
			else
			{
				new_width = getDefaultWidth(new_width);
			}

			if (mOrientation == VERTICAL)
			{
				if (pixels_to_distribute < 0)
				{
					// shrink proportionally to amount over minimum
					delta_size = (shrink_headroom_available > 0) ? llround((F32)pixels_to_distribute * (F32)(cur_height - (*panel_it)->mMinHeight) / (F32)shrink_headroom_available) : 0;
				}
				else
				{
					delta_size = llround((F32)pixels_to_distribute / (F32)num_resizable_panels);
				}
				new_height = llmax((*panel_it)->mMinHeight, cur_height + delta_size);
			}
			else
			{
				new_height = getDefaultHeight(new_height);
			}
		}
		else
		{
			if (mOrientation == HORIZONTAL)
			{
				new_height = getDefaultHeight(new_height);
			}
			else // VERTICAL
			{
				new_width = getDefaultWidth(new_width);
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
			resize_bar_rect.mRight = panel_rect.mRight + mPanelSpacing + RESIZE_BAR_OVERLAP;
		}
		else
		{
			resize_bar_rect.mTop = panel_rect.mBottom + RESIZE_BAR_OVERLAP;
			resize_bar_rect.mBottom = panel_rect.mBottom - mPanelSpacing - RESIZE_BAR_OVERLAP;
		}
		(*panel_it)->mResizeBar->setRect(resize_bar_rect);

		if (mOrientation == HORIZONTAL)
		{
			cur_x += llround(new_width * (*panel_it)->mVisibleAmt) + mPanelSpacing;
		}
		else //VERTICAL
		{
			cur_y -= llround(new_height * (*panel_it)->mVisibleAmt) + mPanelSpacing;
		}
	}

	// update resize bars with new limits
	LLResizeBar* last_resize_bar = NULL;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		LLPanel* panelp = (*panel_it)->mPanel;

		if (mOrientation == HORIZONTAL)
		{
			(*panel_it)->mResizeBar->setResizeLimits(
				(*panel_it)->mMinWidth, 
				(*panel_it)->mMinWidth + shrink_headroom_total);
		}
		else //VERTICAL
		{
			(*panel_it)->mResizeBar->setResizeLimits(
				(*panel_it)->mMinHeight, 
				(*panel_it)->mMinHeight + shrink_headroom_total);
		}

		// toggle resize bars based on panel visibility, resizability, etc
		BOOL resize_bar_enabled = panelp->getVisible() && (*panel_it)->mUserResize;
		(*panel_it)->mResizeBar->setVisible(resize_bar_enabled);

		if (resize_bar_enabled)
		{
			last_resize_bar = (*panel_it)->mResizeBar;
		}
	}

	// hide last resize bar as there is nothing past it
	// resize bars need to be in between two resizable panels
	if (last_resize_bar)
	{
		last_resize_bar->setVisible(FALSE);
	}

	// not enough room to fit existing contents
	if (force_resize == FALSE
		// layout did not complete by reaching target position
		&& ((mOrientation == VERTICAL && cur_y != -mPanelSpacing)
			|| (mOrientation == HORIZONTAL && cur_x != getRect().getWidth() + mPanelSpacing)))
	{
		// do another layout pass with all stacked elements contributing
		// even those that don't usually resize
		llassert_always(force_resize == FALSE);
		updateLayout(TRUE);
	}
} // end LLLayoutStack::updateLayout


LLLayoutStack::LLEmbeddedPanel* LLLayoutStack::findEmbeddedPanel(LLPanel* panelp) const
{
	e_panel_list_t::const_iterator panel_it;
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
			mMinHeight = llmax(	mMinHeight, 
								(*panel_it)->mMinHeight);
            mMinWidth += (*panel_it)->mMinWidth;
			if (panel_it != mPanels.begin())
			{
				mMinWidth += mPanelSpacing;
			}
		}
		else //VERTICAL
		{
	        mMinWidth = llmax(	mMinWidth, 
								(*panel_it)->mMinWidth);
			mMinHeight += (*panel_it)->mMinHeight;
			if (panel_it != mPanels.begin())
			{
				mMinHeight += mPanelSpacing;
			}
		}
	}
}
