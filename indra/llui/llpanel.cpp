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

#define LLPANEL_CPP
#include "llpanel.h"

#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "lltimer.h"

#include "llbutton.h"
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
#include "lltabcontainer.h"

static LLDefaultChildRegistry::Register<LLPanel> r1("panel", &LLPanel::fromXML);

// Compiler optimization, generate extern template
template class LLPanel* LLView::getChild<class LLPanel>(
	const std::string& name, BOOL recurse) const;

LLPanel::LocalizedString::LocalizedString()
:	name("name"),
	value("value")
{}

const LLPanel::Params& LLPanel::getDefaultParams() 
{ 
	return LLUICtrlFactory::getDefaultParams<LLPanel>(); 
}

LLPanel::Params::Params()
:	has_border("border", false),
	border(""),
	background_visible("background_visible", false),
	background_opaque("background_opaque", false),
	bg_opaque_color("bg_opaque_color"),
	bg_alpha_color("bg_alpha_color"),
	bg_opaque_image("bg_opaque_image"),
	bg_alpha_image("bg_alpha_image"),
	min_width("min_width", 100),
	min_height("min_height", 100),
	strings("string"),
	filename("filename"),
	class_name("class"),
	help_topic("help_topic"),
	visible_callback("visible_callback")
{
	name = "panel";
	addSynonym(background_visible, "bg_visible");
	addSynonym(has_border, "border_visible");
	addSynonym(label, "title");
}


LLPanel::LLPanel(const LLPanel::Params& p)
:	LLUICtrl(p),
	mBgVisible(p.background_visible),
	mBgOpaque(p.background_opaque),
	mBgOpaqueColor(p.bg_opaque_color()),
	mBgAlphaColor(p.bg_alpha_color()),
	mBgOpaqueImage(p.bg_opaque_image()),
	mBgAlphaImage(p.bg_alpha_image()),
	mDefaultBtn(NULL),
	mBorder(NULL),
	mLabel(p.label),
	mHelpTopic(p.help_topic),
	mCommitCallbackRegistrar(false),
	mEnableCallbackRegistrar(false),
	mXMLFilename(p.filename),
	mVisibleSignal(NULL)
	// *NOTE: Be sure to also change LLPanel::initFromParams().  We have too
	// many classes derived from LLPanel to retrofit them all to pass in params.
{
	if (p.has_border)
	{
		addBorder(p.border);
	}
	
	mPanelHandle.bind(this);
}

LLPanel::~LLPanel()
{
	delete mVisibleSignal;
}

// virtual
BOOL LLPanel::isPanel() const
{
	return TRUE;
}

void LLPanel::addBorder(LLViewBorder::Params p)
{
	removeBorder();
	p.rect = getLocalRect();

	mBorder = LLUICtrlFactory::create<LLViewBorder>(p);
	addChild( mBorder );
}

void LLPanel::addBorder() 
{  
	LLViewBorder::Params p; 
	p.border_thickness(LLPANEL_BORDER_WIDTH); 
	addBorder(p); 
}


void LLPanel::removeBorder()
{
	if (mBorder)
	{
		removeChild(mBorder);
		delete mBorder;
		mBorder = NULL;
	}
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
	F32 alpha = getDrawContext().mAlpha;

	// draw background
	if( mBgVisible )
	{
		LLRect local_rect = getLocalRect();
		if (mBgOpaque )
		{
			// opaque, in-front look
			if (mBgOpaqueImage.notNull())
			{
				mBgOpaqueImage->draw( local_rect, UI_VERTEX_COLOR % alpha );
			}
			else
			{
				// fallback to flat colors when there are no images
				gl_rect_2d( local_rect, mBgOpaqueColor.get() % alpha);
			}
		}
		else
		{
			// transparent, in-back look
			if (mBgAlphaImage.notNull())
			{
				mBgAlphaImage->draw( local_rect, UI_VERTEX_COLOR % alpha );
			}
			else
			{
				gl_rect_2d( local_rect, mBgAlphaColor.get() % alpha );
			}
		}
	}

	updateDefaultBtn();

	LLView::draw();
}

void LLPanel::updateDefaultBtn()
{
	if( mDefaultBtn)
	{
		if (gFocusMgr.childHasKeyboardFocus( this ) && mDefaultBtn->getEnabled())
		{
			LLButton* buttonp = dynamic_cast<LLButton*>(gFocusMgr.getKeyboardFocus());
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

BOOL LLPanel::handleKeyHere( KEY key, MASK mask )
{
	BOOL handled = FALSE;

	LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());

	// handle user hitting ESC to defocus
	if (key == KEY_ESCAPE)
	{
		setFocus(FALSE);
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
	
	// If RETURN was pressed and something has focus, call onCommit()
	if (!handled && cur_focus && key == KEY_RETURN && mask == MASK_NONE)
	{
		LLButton* focused_button = dynamic_cast<LLButton*>(cur_focus);
		if (focused_button && focused_button->getCommitOnReturn())
		{
			// current focus is a return-capturing button,
			// let *that* button handle the return key
			handled = FALSE; 
		}
		else if (mDefaultBtn && mDefaultBtn->getVisible() && mDefaultBtn->getEnabled())
		{
			// If we have a default button, click it when return is pressed
			mDefaultBtn->onCommit();
			handled = TRUE;
		}
		else if (cur_focus->acceptsTextInput())
		{
			// call onCommit for text input handling control
			cur_focus->onCommit();
			handled = TRUE;
		}
	}

	return handled;
}

void LLPanel::handleVisibilityChange ( BOOL new_visibility )
{
	LLUICtrl::handleVisibilityChange ( new_visibility );
	if (mVisibleSignal)
		(*mVisibleSignal)(this, LLSD(new_visibility) ); // Pass BOOL as LLSD
}

void LLPanel::setFocus(BOOL b)
{
	if( b && !hasFocus())
	{
		// give ourselves focus preemptively, to avoid infinite loop
		LLUICtrl::setFocus(TRUE);
		// then try to pass to first valid child
		focusFirstItem();
	}
	else
	{
		LLUICtrl::setFocus(b);
	}
}

void LLPanel::setBorderVisible(BOOL b)
{
	if (mBorder)
	{
		mBorder->setVisible( b );
	}
}

LLFastTimerUtil::DeclareTimer FTM_PANEL_CONSTRUCTION("Panel Construction");

LLView* LLPanel::fromXML(LLXMLNodePtr node, LLView* parent, LLXMLNodePtr output_node)
{
	std::string name("panel");
	node->getAttributeString("name", name);

	std::string class_attr;
	node->getAttributeString("class", class_attr);

	LLPanel* panelp = NULL;
	
	{
		LLFastTimer timer(FTM_PANEL_CONSTRUCTION);
		
		if(!class_attr.empty())
		{
			panelp = LLRegisterPanelClass::instance().createPanelClass(class_attr);
			if (!panelp)
			{
				llwarns << "Panel class \"" << class_attr << "\" not registered." << llendl;
			}
		}

		if (!panelp)
		{
			panelp = LLUICtrlFactory::getInstance()->createFactoryPanel(name);
		}

	}
	// factory panels may have registered their own factory maps
	if (!panelp->getFactoryMap().empty())
	{
		LLUICtrlFactory::instance().pushFactoryFunctions(&panelp->getFactoryMap());
	}
	// for local registry callbacks; define in constructor, referenced in XUI or postBuild
	panelp->mCommitCallbackRegistrar.pushScope(); 
	panelp->mEnableCallbackRegistrar.pushScope();

	panelp->initPanelXML(node, parent, output_node);
	
	panelp->mCommitCallbackRegistrar.popScope();
	panelp->mEnableCallbackRegistrar.popScope();

	if (panelp && !panelp->getFactoryMap().empty())
	{
		LLUICtrlFactory::instance().popFactoryFunctions();
	}

	return panelp;
}

void LLPanel::initFromParams(const LLPanel::Params& p)
{
    //setting these here since panel constructor not called with params
    //and LLView::initFromParams will use them to set visible and enabled  
	setVisible(p.visible);
	setEnabled(p.enabled);

	setSoundFlags(p.sound_flags);

	 // control_name, tab_stop, focus_lost_callback, initial_value, rect, enabled, visible
	LLUICtrl::initFromParams(p);
	
	// visible callback 
	if (p.visible_callback.isProvided())
	{
		setVisibleCallback(initCommitCallback(p.visible_callback));
	}
	
	for (LLInitParam::ParamIterator<LocalizedString>::const_iterator it = p.strings().begin();
		it != p.strings().end();
		++it)
	{
		mUIStrings[it->name] = it->value;
	}

	setLabel(p.label());
	setHelpTopic(p.help_topic);
	setShape(p.rect);
	parseFollowsFlags(p);

	setToolTip(p.tool_tip());
	setFromXUI(p.from_xui);
	
	mHoverCursor = getCursorFromString(p.hover_cursor);
	
	if (p.has_border)
	{
		addBorder(p.border);
	}
	// let constructors set this value if not provided
	if (p.use_bounding_rect.isProvided())
	{
		setUseBoundingRect(p.use_bounding_rect);
	}
	setDefaultTabGroup(p.default_tab_group);
	setMouseOpaque(p.mouse_opaque);
	
	setBackgroundVisible(p.background_visible);
	setBackgroundOpaque(p.background_opaque);
	setBackgroundColor(p.bg_opaque_color().get());
	setTransparentColor(p.bg_alpha_color().get());
	mBgOpaqueImage = p.bg_opaque_image();
	mBgAlphaImage = p.bg_alpha_image();
}

static LLFastTimerUtil::DeclareTimer FTM_PANEL_SETUP("Panel Setup");
static LLFastTimerUtil::DeclareTimer FTM_EXTERNAL_PANEL_LOAD("Load Extern Panel Reference");
static LLFastTimerUtil::DeclareTimer FTM_PANEL_POSTBUILD("Panel PostBuild");

BOOL LLPanel::initPanelXML(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node)
{
	const LLPanel::Params& default_params(LLUICtrlFactory::getDefaultParams<LLPanel>());
	Params params(default_params);

	{
		LLFastTimer timer(FTM_PANEL_SETUP);

		LLXMLNodePtr referenced_xml;
		std::string xml_filename = mXMLFilename;
		
		// if the panel didn't provide a filename, check the node
		if (xml_filename.empty())
		{
			node->getAttributeString("filename", xml_filename);
		}

		if (!xml_filename.empty())
		{
			LLFastTimer timer(FTM_EXTERNAL_PANEL_LOAD);
			if (output_node)
			{
				//if we are exporting, we want to export the current xml
				//not the referenced xml
				LLXUIParser::instance().readXUI(node, params, xml_filename);
				Params output_params(params);
				setupParamsForExport(output_params, parent);
				output_node->setName(node->getName()->mString);
				LLXUIParser::instance().writeXUI(
					output_node, output_params, &default_params);
				return TRUE;
			}
		
			if (!LLUICtrlFactory::getLayeredXMLNode(xml_filename, referenced_xml))
			{
				llwarns << "Couldn't parse panel from: " << xml_filename << llendl;

				return FALSE;
			}

			LLXUIParser::instance().readXUI(referenced_xml, params, xml_filename);

			// add children using dimensions from referenced xml for consistent layout
			setShape(params.rect);
			LLUICtrlFactory::createChildren(this, referenced_xml, child_registry_t::instance());

			setXMLFilename(xml_filename);
		}

		// ask LLUICtrlFactory for filename, since xml_filename might be empty
		LLXUIParser::instance().readXUI(node, params, LLUICtrlFactory::getInstance()->getCurFileName());

		if (output_node)
		{
			Params output_params(params);
			setupParamsForExport(output_params, parent);
			output_node->setName(node->getName()->mString);
			LLXUIParser::instance().writeXUI(
				output_node, output_params, &default_params);
		}
		
		params.from_xui = true;
		applyXUILayout(params, parent);
		{
			LLFastTimer timer(FTM_PANEL_CONSTRUCTION);
			initFromParams(params);
		}

		// add children
		LLUICtrlFactory::createChildren(this, node, child_registry_t::instance(), output_node);

		// Connect to parent after children are built, because tab containers
		// do a reshape() on their child panels, which requires that the children
		// be built/added. JC
		if (parent)
		{
			S32 tab_group = params.tab_group.isProvided() ? params.tab_group() : parent->getLastTabGroup();
			parent->addChild(this, tab_group);
		}

		{
			LLFastTimer timer(FTM_PANEL_POSTBUILD);
			postBuild();
		}
	}
	return TRUE;
}

bool LLPanel::hasString(const std::string& name)
{
	return mUIStrings.find(name) != mUIStrings.end();
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
	if(LLUI::sSettingGroups["config"]->getBOOL("QAMode"))
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
	if(LLUI::sSettingGroups["config"]->getBOOL("QAMode"))
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
	LLView* child = findChild<LLView>(id);
	if (child)
	{
		child->setVisible(visible);
	}
}

bool LLPanel::childIsVisible(const std::string& id) const
{
	LLView* child = findChild<LLView>(id);
	if (child)
	{
		return (bool)child->getVisible();
	}
	return false;
}

void LLPanel::childSetEnabled(const std::string& id, bool enabled)
{
	LLView* child = findChild<LLView>(id);
	if (child)
	{
		child->setEnabled(enabled);
	}
}

void LLPanel::childSetTentative(const std::string& id, bool tentative)
{
	LLView* child = findChild<LLView>(id);
	if (child)
	{
		child->setTentative(tentative);
	}
}

bool LLPanel::childIsEnabled(const std::string& id) const
{
	LLView* child = findChild<LLView>(id);
	if (child)
	{
		return (bool)child->getEnabled();
	}
	return false;
}


void LLPanel::childSetToolTip(const std::string& id, const std::string& msg)
{
	LLView* child = findChild<LLView>(id);
	if (child)
	{
		child->setToolTip(msg);
	}
}

void LLPanel::childSetRect(const std::string& id, const LLRect& rect)
{
	LLView* child = findChild<LLView>(id);
	if (child)
	{
		child->setRect(rect);
	}
}

bool LLPanel::childGetRect(const std::string& id, LLRect& rect) const
{
	LLView* child = findChild<LLView>(id);
	if (child)
	{
		rect = child->getRect();
		return true;
	}
	return false;
}

void LLPanel::childSetFocus(const std::string& id, BOOL focus)
{
	LLUICtrl* child = findChild<LLUICtrl>(id);
	if (child)
	{
		child->setFocus(focus);
	}
}

BOOL LLPanel::childHasFocus(const std::string& id)
{
	LLUICtrl* child = findChild<LLUICtrl>(id);
	if (child)
	{
		return child->hasFocus();
	}
	else
	{
		return FALSE;
	}
}

// *TODO: Deprecate; for backwards compatability only:
// Prefer getChild<LLUICtrl>("foo")->setCommitCallback(boost:bind(...)),
// which takes a generic slot.  Or use mCommitCallbackRegistrar.add() with
// a named callback and reference it in XML.
void LLPanel::childSetCommitCallback(const std::string& id, boost::function<void (LLUICtrl*,void*)> cb, void* data)
{
	LLUICtrl* child = findChild<LLUICtrl>(id);
	if (child)
	{
		child->setCommitCallback(boost::bind(cb, child, data));
	}
}

void LLPanel::childSetValidate(const std::string& id, boost::function<bool (const LLSD& data)> cb)
{
	LLUICtrl* child = findChild<LLUICtrl>(id);
	if (child)
	{
		child->setValidateBeforeCommit(cb);
	}
}

void LLPanel::childSetColor(const std::string& id, const LLColor4& color)
{
	LLUICtrl* child = findChild<LLUICtrl>(id);
	if (child)
	{
		child->setColor(color);
	}
}

LLCtrlSelectionInterface* LLPanel::childGetSelectionInterface(const std::string& id) const
{
	LLUICtrl* child = findChild<LLUICtrl>(id);
	if (child)
	{
		return child->getSelectionInterface();
	}
	return NULL;
}

LLCtrlListInterface* LLPanel::childGetListInterface(const std::string& id) const
{
	LLUICtrl* child = findChild<LLUICtrl>(id);
	if (child)
	{
		return child->getListInterface();
	}
	return NULL;
}

LLCtrlScrollInterface* LLPanel::childGetScrollInterface(const std::string& id) const
{
	LLUICtrl* child = findChild<LLUICtrl>(id);
	if (child)
	{
		return child->getScrollInterface();
	}
	return NULL;
}

void LLPanel::childSetValue(const std::string& id, LLSD value)
{
	LLUICtrl* child = findChild<LLUICtrl>(id);
	if (child)
	{
		child->setValue(value);
	}
}

LLSD LLPanel::childGetValue(const std::string& id) const
{
	LLUICtrl* child = findChild<LLUICtrl>(id);
	if (child)
	{
		return child->getValue();
	}
	// Not found => return undefined
	return LLSD();
}

BOOL LLPanel::childSetTextArg(const std::string& id, const std::string& key, const LLStringExplicit& text)
{
	LLUICtrl* child = findChild<LLUICtrl>(id);
	if (child)
	{
		return child->setTextArg(key, text);
	}
	return FALSE;
}

BOOL LLPanel::childSetLabelArg(const std::string& id, const std::string& key, const LLStringExplicit& text)
{
	LLView* child = findChild<LLView>(id);
	if (child)
	{
		return child->setLabelArg(key, text);
	}
	return FALSE;
}

BOOL LLPanel::childSetToolTipArg(const std::string& id, const std::string& key, const LLStringExplicit& text)
{
	LLView* child = findChild<LLView>(id);
	if (child)
	{
		return child->setToolTipArg(key, text);
	}
	return FALSE;
}

void LLPanel::childShowTab(const std::string& id, const std::string& tabname, bool visible)
{
	LLTabContainer* child = findChild<LLTabContainer>(id);
	if (child)
	{
		child->selectTabByName(tabname);
	}
}

LLPanel *LLPanel::childGetVisibleTab(const std::string& id) const
{
	LLTabContainer* child = findChild<LLTabContainer>(id);
	if (child)
	{
		return child->getCurrentPanel();
	}
	return NULL;
}

static LLPanel *childGetVisibleTabWithHelp(LLView *parent)
{
	LLView *child;

	// look through immediate children first for an active tab with help
	for (child = parent->getFirstChild(); child; child = parent->findNextSibling(child))
	{
		LLTabContainer *tab = dynamic_cast<LLTabContainer *>(child);
		if (tab && tab->getVisible())
		{
			LLPanel *curTabPanel = tab->getCurrentPanel();
			if (curTabPanel && !curTabPanel->getHelpTopic().empty())
			{
				return curTabPanel;
			}
		}
	}

	// then try a bit harder and recurse through all children
	for (child = parent->getFirstChild(); child; child = parent->findNextSibling(child))
	{
		if (child->getVisible())
		{
			LLPanel* tab = ::childGetVisibleTabWithHelp(child);
			if (tab)
			{
				return tab;
			}
		}
	}

	// couldn't find any active tabs with a help topic string
	return NULL;
}

LLPanel *LLPanel::childGetVisibleTabWithHelp()
{
	// find a visible tab with a help topic (to determine help context)
	return ::childGetVisibleTabWithHelp(this);
}

void LLPanel::childSetPrevalidate(const std::string& id, BOOL (*func)(const LLWString &) )
{
	LLLineEditor* child = findChild<LLLineEditor>(id);
	if (child)
	{
		child->setPrevalidate(func);
	}
}


void LLPanel::childSetAction(const std::string& id, boost::function<void(void*)> function, void* value)
{
	LLButton* button = findChild<LLButton>(id);
	if (button)
	{
		button->setClickedCallback(boost::bind(function, value));
	}
}

void LLPanel::childSetActionTextbox(const std::string& id, boost::function<void(void*)> function, void* value)
{
	LLTextBox* textbox = findChild<LLTextBox>(id);
	if (textbox)
	{
		textbox->setClickedCallback(boost::bind(function, value));
	}
}

void LLPanel::childSetControlName(const std::string& id, const std::string& control_name)
{
	LLUICtrl* view = findChild<LLUICtrl>(id);
	if (view)
	{
		view->setControlName(control_name, NULL);
	}
}

boost::signals2::connection LLPanel::setVisibleCallback( const commit_signal_t::slot_type& cb )
{
	if (!mVisibleSignal)
	{
		mVisibleSignal = new commit_signal_t();
	}

	return mVisibleSignal->connect(cb);
}
