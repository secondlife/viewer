/** 
 * @file llpanel.cpp
 * @brief LLPanel base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#include "llresizebar.h"
#include "llcriticaldamp.h"

const S32 RESIZE_BAR_OVERLAP = 1;
const S32 RESIZE_BAR_HEIGHT = 3;

static LLRegisterWidget<LLPanel> r1("panel");

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
	setName(std::string("panel"));
}

LLPanel::LLPanel(const std::string& name)
:	LLUICtrl(name, LLRect(0, 0, 0, 0), TRUE, NULL, NULL),
	mRectControl()
{
	init();
}


LLPanel::LLPanel(const std::string& name, const LLRect& rect, BOOL bordered)
:	LLUICtrl(name, rect, TRUE, NULL, NULL),
	mRectControl()
{
	init();
	if (bordered)
	{
		addBorder();
	}
}


LLPanel::LLPanel(const std::string& name, const std::string& rect_control, BOOL bordered)
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
	mBorder = new LLViewBorder( std::string("panel border"), 
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

	LLView::draw();
}

void LLPanel::updateDefaultBtn()
{
	// This method does not call LLView::draw() so callers will need
	// to take care of that themselves at the appropriate place in
	// their rendering sequence

	if( mDefaultBtn)
	{
		if (gFocusMgr.childHasKeyboardFocus( this ) && mDefaultBtn->getEnabled())
		{
			LLUICtrl* focus_ctrl = gFocusMgr.getKeyboardFocus();
			LLButton* buttonp = dynamic_cast<LLButton*>(focus_ctrl);
			BOOL focus_is_child_button = buttonp && buttonp->getCommitOnReturn();
			// only enable default button when current focus is not a return-capturing button
			mDefaultBtn->setBorderEnabled(!focus_is_child_button);
		}
		else
		{
			mDefaultBtn->setBorderEnabled(FALSE);
		}
	}
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

void LLPanel::setDefaultBtn(const std::string& id)
{
	LLButton *button = getChild<LLButton>(id);
	if (button)
	{
		setDefaultBtn(button);
	}
	else
	{
		setDefaultBtn(NULL);
	}
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

BOOL LLPanel::handleKeyHere( KEY key, MASK mask )
{
	BOOL handled = FALSE;

	LLUICtrl* cur_focus = gFocusMgr.getKeyboardFocus();

	// handle user hitting ESC to defocus
	if (key == KEY_ESCAPE)
	{
		gFocusMgr.setKeyboardFocus(NULL);
		return TRUE;
	}
	else if( (mask == MASK_SHIFT) && (KEY_TAB == key))
	{
		//SHIFT-TAB
		if (cur_focus)
		{
			LLUICtrl* focus_root = cur_focus->findRootMostFocusRoot();
			if (focus_root)
			{
				handled = focus_root->focusPrevItem(FALSE);
			}
		}
	}
	else if( (mask == MASK_NONE ) && (KEY_TAB == key))	
	{
		//TAB
		if (cur_focus)
		{
			LLUICtrl* focus_root = cur_focus->findRootMostFocusRoot();
			if (focus_root)
			{
				handled = focus_root->focusNextItem(FALSE);
			}
		}
	}

	// If we have a default button, click it when
	// return is pressed, unless current focus is a return-capturing button
	// in which case *that* button will handle the return key
	LLButton* focused_button = dynamic_cast<LLButton*>(cur_focus);
	if (cur_focus && !(focused_button && focused_button->getCommitOnReturn()))
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

	return handled;
}

BOOL LLPanel::checkRequirements()
{
	if (!mRequirementsError.empty())
	{
		LLSD args;
		args["COMPONENTS"] = mRequirementsError;
		args["FLOATER"] = getName();

		llwarns << getName() << " failed requirements check on: \n"  
				<< mRequirementsError << llendl;
		
		LLNotifications::instance().add(LLNotification::Params("FailedRequirementsCheck").payload(args));
		mRequirementsError.clear();
		return FALSE;
	}

	return TRUE;
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
	std::string name("panel");
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
	std::string name = getName();
	node->getAttributeString("name", name);
	setName(name);

	setPanelParameters(node, parent);

	initChildrenXML(node, factory);

	std::string xml_filename;
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
			std::string string_name;
			child->getAttributeString("name", string_name);
			if (!string_name.empty())
			{
				mUIStrings[string_name] = child->getTextContents();
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
		std::string border_string;
		node->getAttributeString("border_style", border_string);
		LLStringUtil::toLower(border_string);

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

	std::string label = getLabel();
	node->getAttributeString("label", label);
	setLabel(label);
}

std::string LLPanel::getString(const std::string& name, const LLStringUtil::format_map_t& args) const
{
	ui_string_map_t::const_iterator found_it = mUIStrings.find(name);
	if (found_it != mUIStrings.end())
	{
		// make a copy as format works in place
		LLUIString formatted_string = LLUIString(found_it->second);
		formatted_string.setArgList(args);
		return formatted_string.getString();
	}
	std::string err_str("Failed to find string " + name + " in panel " + getName()); //*TODO: Translate
	// *TODO: once the QAR-369 ui-cleanup work on settings is in we need to change the following line to be
	//if(LLUI::sConfigGroup->getBOOL("QAMode"))
	if(LLUI::sQAMode)
	{
		llerrs << err_str << llendl;
	}
	else
	{
		llwarns << err_str << llendl;
	}
	return LLStringUtil::null;
}

std::string LLPanel::getString(const std::string& name) const
{
	ui_string_map_t::const_iterator found_it = mUIStrings.find(name);
	if (found_it != mUIStrings.end())
	{
		return found_it->second;
	}
	std::string err_str("Failed to find string " + name + " in panel " + getName()); //*TODO: Translate
	if(LLUI::sQAMode)
	{
		llerrs << err_str << llendl;
	}
	else
	{
		llwarns << err_str << llendl;
	}
	return LLStringUtil::null;
}


void LLPanel::childSetVisible(const std::string& id, bool visible)
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		child->setVisible(visible);
	}
}

bool LLPanel::childIsVisible(const std::string& id) const
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		return (bool)child->getVisible();
	}
	return false;
}

void LLPanel::childSetEnabled(const std::string& id, bool enabled)
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		child->setEnabled(enabled);
	}
}

void LLPanel::childSetTentative(const std::string& id, bool tentative)
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		child->setTentative(tentative);
	}
}

bool LLPanel::childIsEnabled(const std::string& id) const
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		return (bool)child->getEnabled();
	}
	return false;
}


void LLPanel::childSetToolTip(const std::string& id, const std::string& msg)
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		child->setToolTip(msg);
	}
}

void LLPanel::childSetRect(const std::string& id, const LLRect& rect)
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		child->setRect(rect);
	}
}

bool LLPanel::childGetRect(const std::string& id, LLRect& rect) const
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		rect = child->getRect();
		return true;
	}
	return false;
}

void LLPanel::childSetFocus(const std::string& id, BOOL focus)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setFocus(focus);
	}
}

BOOL LLPanel::childHasFocus(const std::string& id)
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


void LLPanel::childSetFocusChangedCallback(const std::string& id, void (*cb)(LLFocusableElement*, void*), void* user_data)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setFocusChangedCallback(cb, user_data);
	}
}

void LLPanel::childSetCommitCallback(const std::string& id, void (*cb)(LLUICtrl*, void*), void *userdata )
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setCommitCallback(cb);
		child->setCallbackUserData(userdata);
	}
}

void LLPanel::childSetDoubleClickCallback(const std::string& id, void (*cb)(void*), void *userdata )
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

void LLPanel::childSetValidate(const std::string& id, BOOL (*cb)(LLUICtrl*, void*))
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setValidateBeforeCommit(cb);
	}
}

void LLPanel::childSetUserData(const std::string& id, void* userdata)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setCallbackUserData(userdata);
	}
}

void LLPanel::childSetColor(const std::string& id, const LLColor4& color)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setColor(color);
	}
}

LLCtrlSelectionInterface* LLPanel::childGetSelectionInterface(const std::string& id) const
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		return child->getSelectionInterface();
	}
	return NULL;
}

LLCtrlListInterface* LLPanel::childGetListInterface(const std::string& id) const
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		return child->getListInterface();
	}
	return NULL;
}

LLCtrlScrollInterface* LLPanel::childGetScrollInterface(const std::string& id) const
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		return child->getScrollInterface();
	}
	return NULL;
}

void LLPanel::childSetValue(const std::string& id, LLSD value)
{
	LLView* child = getChild<LLView>(id, true);
	if (child)
	{
		child->setValue(value);
	}
}

LLSD LLPanel::childGetValue(const std::string& id) const
{
	LLView* child = getChild<LLView>(id, true);
	if (child)
	{
		return child->getValue();
	}
	// Not found => return undefined
	return LLSD();
}

BOOL LLPanel::childSetTextArg(const std::string& id, const std::string& key, const LLStringExplicit& text)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		return child->setTextArg(key, text);
	}
	return FALSE;
}

BOOL LLPanel::childSetLabelArg(const std::string& id, const std::string& key, const LLStringExplicit& text)
{
	LLView* child = getChild<LLView>(id);
	if (child)
	{
		return child->setLabelArg(key, text);
	}
	return FALSE;
}

BOOL LLPanel::childSetToolTipArg(const std::string& id, const std::string& key, const LLStringExplicit& text)
{
	LLView* child = getChildView(id, true, FALSE);
	if (child)
	{
		return child->setToolTipArg(key, text);
	}
	return FALSE;
}

void LLPanel::childSetMinValue(const std::string& id, LLSD min_value)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setMinValue(min_value);
	}
}

void LLPanel::childSetMaxValue(const std::string& id, LLSD max_value)
{
	LLUICtrl* child = getChild<LLUICtrl>(id, true);
	if (child)
	{
		child->setMaxValue(max_value);
	}
}

void LLPanel::childShowTab(const std::string& id, const std::string& tabname, bool visible)
{
	LLTabContainer* child = getChild<LLTabContainer>(id);
	if (child)
	{
		child->selectTabByName(tabname);
	}
}

LLPanel *LLPanel::childGetVisibleTab(const std::string& id) const
{
	LLTabContainer* child = getChild<LLTabContainer>(id);
	if (child)
	{
		return child->getCurrentPanel();
	}
	return NULL;
}

void LLPanel::childSetTabChangeCallback(const std::string& id, const std::string& tabname, void (*on_tab_clicked)(void*, bool), void *userdata)
{
	LLTabContainer* child = getChild<LLTabContainer>(id);
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

void LLPanel::childSetKeystrokeCallback(const std::string& id, void (*keystroke_callback)(LLLineEditor* caller, void* user_data), void *user_data)
{
	LLLineEditor* child = getChild<LLLineEditor>(id);
	if (child)
	{
		child->setKeystrokeCallback(keystroke_callback);
		if (user_data)
		{
			child->setCallbackUserData(user_data);
		}
	}
}

void LLPanel::childSetPrevalidate(const std::string& id, BOOL (*func)(const LLWString &) )
{
	LLLineEditor* child = getChild<LLLineEditor>(id);
	if (child)
	{
		child->setPrevalidate(func);
	}
}

void LLPanel::childSetWrappedText(const std::string& id, const std::string& text, bool visible)
{
	LLTextBox* child = getChild<LLTextBox>(id);
	if (child)
	{
		child->setVisible(visible);
		child->setWrappedText(text);
	}
}

void LLPanel::childSetAction(const std::string& id, void(*function)(void*), void* value)
{
	LLButton* button = getChild<LLButton>(id);
	if (button)
	{
		button->setClickedCallback(function, value);
	}
}

void LLPanel::childSetActionTextbox(const std::string& id, void(*function)(void*), void* value)
{
	LLTextBox* textbox = getChild<LLTextBox>(id);
	if (textbox)
	{
		textbox->setClickedCallback(function, value);
	}
}

void LLPanel::childSetControlName(const std::string& id, const std::string& control_name)
{
	LLView* view = getChild<LLView>(id);
	if (view)
	{
		view->setControlName(control_name, NULL);
	}
}

//virtual
LLView* LLPanel::getChildView(const std::string& name, BOOL recurse, BOOL create_if_missing) const
{
	// just get child, don't try to create a dummy one
	LLView* view = LLUICtrl::getChildView(name, recurse, FALSE);
	if (!view && !recurse)
	{
		childNotFound(name);
	}
	if (!view && create_if_missing)
	{
		view = createDummyWidget<LLView>(name);
	}
	return view;
}

void LLPanel::childNotFound(const std::string& id) const
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
	std::string msg;
	expected_members_list_t::iterator itor;
	for (itor=mNewExpectedMembers.begin(); itor!=mNewExpectedMembers.end(); ++itor)
	{
		msg.append(*itor);
		msg.append("\n");
		mExpectedMembers.insert(*itor);
	}
	mNewExpectedMembers.clear();
	LLSD args;
	args["CONTROLS"] = msg;
	LLNotifications::instance().add("FloaterNotFound", args);
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
			mCollapsed(FALSE),
			mCollapseAmt(0.f),
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
		mResizeBar = new LLResizeBar(std::string("resizer"), mPanel, LLRect(), min_dim, S32_MAX, side);
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

	F32 getCollapseFactor()
	{
		if (mOrientation == HORIZONTAL)
		{
			return mVisibleAmt * clamp_rescale(mCollapseAmt, 0.f, 1.f, 1.f, (F32)mMinWidth / (F32)mPanel->getRect().getWidth());
		}
		else
		{
			return mVisibleAmt * clamp_rescale(mCollapseAmt, 0.f, 1.f, 1.f, (F32)mMinHeight / (F32)mPanel->getRect().getHeight());
		}
	}

	LLPanel* mPanel;
	S32 mMinWidth;
	S32 mMinHeight;
	BOOL mAutoResize;
	BOOL mUserResize;
	BOOL mCollapsed;
	LLResizeBar* mResizeBar;
	eLayoutOrientation mOrientation;
	F32 mVisibleAmt;
	F32 mCollapseAmt;
};

static LLRegisterWidget<LLLayoutStack> r2("layout_stack");

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

void LLLayoutStack::draw()
{
	updateLayout();

	e_panel_list_t::iterator panel_it;
	for (panel_it = mPanels.begin(); panel_it != mPanels.end(); ++panel_it)
	{
		// clip to layout rectangle, not bounding rectangle
		LLRect clip_rect = (*panel_it)->mPanel->getRect();
		// scale clipping rectangle by visible amount
		if (mOrientation == HORIZONTAL)
		{
			clip_rect.mRight = clip_rect.mLeft + llround((F32)clip_rect.getWidth() * (*panel_it)->getCollapseFactor());
		}
		else
		{
			clip_rect.mBottom = clip_rect.mTop - llround((F32)clip_rect.getHeight() * (*panel_it)->getCollapseFactor());
		}

		LLPanel* panelp = (*panel_it)->mPanel;

		LLLocalClipRect clip(clip_rect);
		// only force drawing invisible children if visible amount is non-zero
		drawChild(panelp, 0, 0, !clip_rect.isNull());
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
	std::string orientation_string("vertical");
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

	std::string name("stack");
	node->getAttributeString("name", name);

	layout_stackp->setName(name);
	layout_stackp->initFromXML(node, parent);

	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		S32 min_width = 0;
		S32 min_height = 0;
		BOOL auto_resize = TRUE;

		child->getAttributeS32("min_width", min_width);
		child->getAttributeS32("min_height", min_height);
		child->getAttributeBOOL("auto_resize", auto_resize);

		if (child->hasName("layout_panel"))
		{
			BOOL user_resize = TRUE;
			child->getAttributeBOOL("user_resize", user_resize);
			LLPanel* panelp = (LLPanel*)LLPanel::fromXML(child, layout_stackp, factory);
			if (panelp)
			{
				panelp->setFollowsNone();
				layout_stackp->addPanel(panelp, min_width, min_height, auto_resize, user_resize);
			}
		}
		else
		{
			BOOL user_resize = FALSE;
			child->getAttributeBOOL("user_resize", user_resize);

			LLPanel* panelp = new LLPanel(std::string("auto_panel"));
			LLView* new_child = factory->createWidget(panelp, child);
			if (new_child)
			{
				// put child in new embedded panel
				layout_stackp->addPanel(panelp, min_width, min_height, auto_resize, user_resize);
				// resize panel to contain widget and move widget to be contained in panel
				panelp->setRect(new_child->getRect());
				new_child->setOrigin(0, 0);
			}
			else
			{
				panelp->die();
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

void LLLayoutStack::addPanel(LLPanel* panel, S32 min_width, S32 min_height, BOOL auto_resize, BOOL user_resize, EAnimate animate, S32 index)
{
	// panel starts off invisible (collapsed)
	if (animate == ANIMATE)
	{
		panel->setVisible(FALSE);
	}
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

	// start expanding panel animation
	if (animate == ANIMATE)
	{
		panel->setVisible(TRUE);
	}
}

void LLLayoutStack::removePanel(LLPanel* panel)
{
	removeChild(panel);
}

void LLLayoutStack::collapsePanel(LLPanel* panel, BOOL collapsed)
{
	LLEmbeddedPanel* panel_container = findEmbeddedPanel(panel);
	if (!panel_container) return;

	panel_container->mCollapsed = collapsed;
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

		if ((*panel_it)->mCollapsed)
		{
			(*panel_it)->mCollapseAmt = lerp((*panel_it)->mCollapseAmt, 1.f, LLCriticalDamp::getInterpolant(ANIM_CLOSE_TIME));
		}
		else
		{
			(*panel_it)->mCollapseAmt = lerp((*panel_it)->mCollapseAmt, 0.f, LLCriticalDamp::getInterpolant(ANIM_CLOSE_TIME));
		}

		if (mOrientation == HORIZONTAL)
		{
			// enforce minimize size constraint by default
			if (panelp->getRect().getWidth() < (*panel_it)->mMinWidth)
			{
				panelp->reshape((*panel_it)->mMinWidth, panelp->getRect().getHeight());
			}
        	total_width += llround(panelp->getRect().getWidth() * (*panel_it)->getCollapseFactor());
        	// want n-1 panel gaps for n panels
			if (panel_it != mPanels.begin())
			{
				total_width += mPanelSpacing;
			}
		}
		else //VERTICAL
		{
			// enforce minimize size constraint by default
			if (panelp->getRect().getHeight() < (*panel_it)->mMinHeight)
			{
				panelp->reshape(panelp->getRect().getWidth(), (*panel_it)->mMinHeight);
			}
			total_height += llround(panelp->getRect().getHeight() * (*panel_it)->getCollapseFactor());
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
		if ((*panel_it)->getCollapseFactor() < 1.f) 
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

	// calculate how many pixels need to be distributed among layout panels
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

	// now we distribute the pixels...
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
		if ((*panel_it)->getCollapseFactor() == 1.f 
			&& (force_resize || (*panel_it)->mAutoResize) 
			&& !(*panel_it)->mResizeBar->hasMouseCapture()) 
		{
			if (mOrientation == HORIZONTAL)
			{
				// if we're shrinking
				if (pixels_to_distribute < 0)
				{
					// shrink proportionally to amount over minimum
					// so we can do this in one pass
					delta_size = (shrink_headroom_available > 0) ? llround((F32)pixels_to_distribute * ((F32)(cur_width - (*panel_it)->mMinWidth) / (F32)shrink_headroom_available)) : 0;
					shrink_headroom_available -= (cur_width - (*panel_it)->mMinWidth);
				}
				else
				{
					// grow all elements equally
					delta_size = llround((F32)pixels_to_distribute / (F32)num_resizable_panels);
					num_resizable_panels--;
				}
				pixels_to_distribute -= delta_size;
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
					// so we can do this in one pass
					delta_size = (shrink_headroom_available > 0) ? llround((F32)pixels_to_distribute * ((F32)(cur_height - (*panel_it)->mMinHeight) / (F32)shrink_headroom_available)) : 0;
					shrink_headroom_available -= (cur_height - (*panel_it)->mMinHeight);
				}
				else
				{
					delta_size = llround((F32)pixels_to_distribute / (F32)num_resizable_panels);
					num_resizable_panels--;
				}
				pixels_to_distribute -= delta_size;
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
			cur_x += llround(new_width * (*panel_it)->getCollapseFactor()) + mPanelSpacing;
		}
		else //VERTICAL
		{
			cur_y -= llround(new_height * (*panel_it)->getCollapseFactor()) + mPanelSpacing;
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
