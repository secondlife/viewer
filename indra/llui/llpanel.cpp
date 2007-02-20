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

LLPanel::panel_map_t LLPanel::sPanelMap;
LLPanel::alert_queue_t LLPanel::sAlertQueue;

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


LLPanel::~LLPanel()
{
	if( !mRectControl.empty() )
	{
		LLUI::sConfigGroup->setRect( mRectControl, mRect );
	}
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
	if( getVisible() )
	{
		// draw background
		if( mBgVisible )
		{
			S32 left = LLPANEL_BORDER_WIDTH;
			S32 top = mRect.getHeight() - LLPANEL_BORDER_WIDTH;
			S32 right = mRect.getWidth() - LLPANEL_BORDER_WIDTH;
			S32 bottom = LLPANEL_BORDER_WIDTH;

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
		if (!(cur_focus->getWidgetType() == WIDGET_TYPE_BUTTON && static_cast<LLButton *>(cur_focus)->getCommitOnReturn()))
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

LLView* LLPanel::fromXML(LLXMLNodePtr node, LLView* parentp, LLUICtrlFactory *factory)
{
	LLString name("panel");
	node->getAttributeString("name", name);

	LLPanel* panelp = factory->createFactoryPanel(name);
	// Fall back on a default panel, if there was no special factory.
	if (!panelp)
	{
		panelp = new LLPanel("tab panel");
	}

	panelp->initPanelXML(node, parentp, factory);

	return panelp;
}

void LLPanel::initPanelXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("panel");
	node->getAttributeString("name", name);
	setName(name);

	setPanelParameters(node, parent);

	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		factory->createWidget(this, child);
	}

	LLString xml_filename;
	node->getAttributeString("filename", xml_filename);
	if (!xml_filename.empty())
	{
		factory->buildPanel(this, xml_filename, NULL);
	}
	
	postBuild();
}

void LLPanel::setPanelParameters(LLXMLNodePtr node, LLView* parentp)
{
	/////// Rect, follows, tool_tip, enabled, visible attributes ///////
	initFromXML(node, parentp);

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

