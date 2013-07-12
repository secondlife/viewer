/** 
 * @file llpanel.cpp
 * @brief LLPanel base class
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

// Opaque view with a background and a border.  Can contain LLUICtrls.

#include "linden_common.h"

#define LLPANEL_CPP
#include "llpanel.h"

#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "lldir.h"
#include "lltimer.h"

#include "llaccordionctrltab.h"
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
LLPanel::factory_stack_t	LLPanel::sFactoryStack;


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
	bg_opaque_image_overlay("bg_opaque_image_overlay"),
	bg_alpha_image_overlay("bg_alpha_image_overlay"),
	bg_opaque_image("bg_opaque_image"),
	bg_alpha_image("bg_alpha_image"),
	min_width("min_width", 100),
	min_height("min_height", 100),
	strings("string"),
	filename("filename"),
	class_name("class"),
	help_topic("help_topic"),
	visible_callback("visible_callback"),
	accepts_badge("accepts_badge")
{
	addSynonym(background_visible, "bg_visible");
	addSynonym(has_border, "border_visible");
	addSynonym(label, "title");
}


LLPanel::LLPanel(const LLPanel::Params& p)
:	LLUICtrl(p),
	LLBadgeHolder(p.accepts_badge),
	mBgVisible(p.background_visible),
	mBgOpaque(p.background_opaque),
	mBgOpaqueColor(p.bg_opaque_color()),
	mBgAlphaColor(p.bg_alpha_color()),
	mBgOpaqueImageOverlay(p.bg_opaque_image_overlay),
	mBgAlphaImageOverlay(p.bg_alpha_image_overlay),
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
		alpha = getCurrentTransparency();

		LLRect local_rect = getLocalRect();
		if (mBgOpaque )
		{
			// opaque, in-front look
			if (mBgOpaqueImage.notNull())
			{
				mBgOpaqueImage->draw( local_rect, mBgOpaqueImageOverlay % alpha );
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
				mBgAlphaImage->draw( local_rect, mBgAlphaImageOverlay % alpha );
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

LLFastTimer::DeclareTimer FTM_PANEL_CONSTRUCTION("Panel Construction");

LLView* LLPanel::fromXML(LLXMLNodePtr node, LLView* parent, LLXMLNodePtr output_node)
{
	std::string name("panel");
	node->getAttributeString("name", name);

	std::string class_attr;
	node->getAttributeString("class", class_attr);

	LLPanel* panelp = NULL;
	
	{	LLFastTimer _(FTM_PANEL_CONSTRUCTION);
		
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
			panelp = createFactoryPanel(name);
			llassert(panelp);
			
			if (!panelp)
			{
				return NULL; // :(
			}
		}

	}
	// factory panels may have registered their own factory maps
	if (!panelp->getFactoryMap().empty())
	{
		sFactoryStack.push_back(&panelp->getFactoryMap());
	}
	// for local registry callbacks; define in constructor, referenced in XUI or postBuild
	panelp->mCommitCallbackRegistrar.pushScope(); 
	panelp->mEnableCallbackRegistrar.pushScope();

	panelp->initPanelXML(node, parent, output_node, LLUICtrlFactory::getDefaultParams<LLPanel>());
	
	panelp->mCommitCallbackRegistrar.popScope();
	panelp->mEnableCallbackRegistrar.popScope();

	if (!panelp->getFactoryMap().empty())
	{
		sFactoryStack.pop_back();
	}

	return panelp;
}

void LLPanel::initFromParams(const LLPanel::Params& p)
{
    //setting these here since panel constructor not called with params
    //and LLView::initFromParams will use them to set visible and enabled  
	setVisible(p.visible);
	setEnabled(p.enabled);
	setFocusRoot(p.focus_root);
	setSoundFlags(p.sound_flags);

	 // control_name, tab_stop, focus_lost_callback, initial_value, rect, enabled, visible
	LLUICtrl::initFromParams(p);
	
	// visible callback 
	if (p.visible_callback.isProvided())
	{
		setVisibleCallback(initCommitCallback(p.visible_callback));
	}
	
	for (LLInitParam::ParamIterator<LocalizedString>::const_iterator it = p.strings.begin();
		it != p.strings.end();
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
	mBgOpaqueImageOverlay = p.bg_opaque_image_overlay;
	mBgAlphaImageOverlay = p.bg_alpha_image_overlay;

	setAcceptsBadge(p.accepts_badge);
}

static LLFastTimer::DeclareTimer FTM_PANEL_SETUP("Panel Setup");
static LLFastTimer::DeclareTimer FTM_EXTERNAL_PANEL_LOAD("Load Extern Panel Reference");
static LLFastTimer::DeclareTimer FTM_PANEL_POSTBUILD("Panel PostBuild");

BOOL LLPanel::initPanelXML(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node, const LLPanel::Params& default_params)
{
	Params params(default_params);
	{
		LLFastTimer timer(FTM_PANEL_SETUP);

		LLXMLNodePtr referenced_xml;
		std::string xml_filename = mXMLFilename;
		
		// if the panel didn't provide a filename, check the node
		if (xml_filename.empty())
		{
			node->getAttributeString("filename", xml_filename);
			setXMLFilename(xml_filename);
		}

		LLXUIParser parser;

		if (!xml_filename.empty())
		{
			if (output_node)
			{
				//if we are exporting, we want to export the current xml
				//not the referenced xml
				parser.readXUI(node, params, LLUICtrlFactory::getInstance()->getCurFileName());
				Params output_params(params);
				setupParamsForExport(output_params, parent);
				output_node->setName(node->getName()->mString);
				parser.writeXUI(output_node, output_params, &default_params);
				return TRUE;
			}
		
			LLUICtrlFactory::instance().pushFileName(xml_filename);

			LLFastTimer timer(FTM_EXTERNAL_PANEL_LOAD);
			if (!LLUICtrlFactory::getLayeredXMLNode(xml_filename, referenced_xml))
			{
				llwarns << "Couldn't parse panel from: " << xml_filename << llendl;

				return FALSE;
			}

			parser.readXUI(referenced_xml, params, LLUICtrlFactory::getInstance()->getCurFileName());

			// add children using dimensions from referenced xml for consistent layout
			setShape(params.rect);
			LLUICtrlFactory::createChildren(this, referenced_xml, child_registry_t::instance());

			LLUICtrlFactory::instance().popFileName();
		}

		// ask LLUICtrlFactory for filename, since xml_filename might be empty
		parser.readXUI(node, params, LLUICtrlFactory::getInstance()->getCurFileName());

		if (output_node)
		{
			Params output_params(params);
			setupParamsForExport(output_params, parent);
			output_node->setName(node->getName()->mString);
			parser.writeXUI(output_node, output_params, &default_params);
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
	LLUICtrl* child = findChild<LLUICtrl>(id);
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

LLPanel* LLPanel::childGetVisibleTabWithHelp()
{
	LLView *child;

	bfs_tree_iterator_t it = beginTreeBFS();
	// skip ourselves
	++it;
	for (; it != endTreeBFS(); ++it)
	{
		child = *it;
		LLPanel *curTabPanel = NULL;

		// do we have a tab container?
		LLTabContainer *tab = dynamic_cast<LLTabContainer *>(child);
		if (tab && tab->getVisible())
		{
			curTabPanel = tab->getCurrentPanel();
		}

		// do we have an accordion tab?
		LLAccordionCtrlTab* accordion = dynamic_cast<LLAccordionCtrlTab *>(child);
		if (accordion && accordion->getDisplayChildren())
		{
			curTabPanel = dynamic_cast<LLPanel *>(accordion->getAccordionView());
		}

		// if we found a valid tab, does it have a help topic?
		if (curTabPanel && !curTabPanel->getHelpTopic().empty())
		{
			return curTabPanel;
		}
	}

	// couldn't find any active tabs with a help topic string
	return NULL;
}


LLPanel *LLPanel::childGetVisiblePanelWithHelp()
{
	LLView *child;

	bfs_tree_iterator_t it = beginTreeBFS();
	// skip ourselves
	++it;
	for (; it != endTreeBFS(); ++it)
	{
		child = *it;
		// do we have a panel with a help topic?
		LLPanel *panel = dynamic_cast<LLPanel *>(child);
		if (panel && panel->isInVisibleChain() && !panel->getHelpTopic().empty())
		{
			return panel;
		}
	}

	// couldn't find any active panels with a help topic string
	return NULL;
}

void LLPanel::childSetAction(const std::string& id, const commit_signal_t::slot_type& function)
{
	LLButton* button = findChild<LLButton>(id);
	if (button)
	{
		button->setClickedCallback(function);
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

static LLFastTimer::DeclareTimer FTM_BUILD_PANELS("Build Panels");

//-----------------------------------------------------------------------------
// buildPanel()
//-----------------------------------------------------------------------------
BOOL LLPanel::buildFromFile(const std::string& filename, const LLPanel::Params& default_params)
{
	LLFastTimer timer(FTM_BUILD_PANELS);
	BOOL didPost = FALSE;
	LLXMLNodePtr root;

	if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		llwarns << "Couldn't parse panel from: " << filename << llendl;
		return didPost;
	}

	// root must be called panel
	if( !root->hasName("panel" ) )
	{
		llwarns << "Root node should be named panel in : " << filename << llendl;
		return didPost;
	}

	lldebugs << "Building panel " << filename << llendl;

	LLUICtrlFactory::instance().pushFileName(filename);
	{
		if (!getFactoryMap().empty())
		{
			sFactoryStack.push_back(&getFactoryMap());
		}
		
		 // for local registry callbacks; define in constructor, referenced in XUI or postBuild
		getCommitCallbackRegistrar().pushScope();
		getEnableCallbackRegistrar().pushScope();
		
		didPost = initPanelXML(root, NULL, NULL, default_params);

		getCommitCallbackRegistrar().popScope();
		getEnableCallbackRegistrar().popScope();
		
		setXMLFilename(filename);

		if (!getFactoryMap().empty())
		{
			sFactoryStack.pop_back();
		}
	}
	LLUICtrlFactory::instance().popFileName();
	return didPost;
}

//-----------------------------------------------------------------------------
// createFactoryPanel()
//-----------------------------------------------------------------------------
LLPanel* LLPanel::createFactoryPanel(const std::string& name)
{
	std::deque<const LLCallbackMap::map_t*>::iterator itor;
	for (itor = sFactoryStack.begin(); itor != sFactoryStack.end(); ++itor)
	{
		const LLCallbackMap::map_t* factory_map = *itor;

		// Look up this panel's name in the map.
		LLCallbackMap::map_const_iter_t iter = factory_map->find( name );
		if (iter != factory_map->end())
		{
			// Use the factory to create the panel, instead of using a default LLPanel.
			LLPanel *ret = (LLPanel*) iter->second.mCallback( iter->second.mData );
			return ret;
		}
	}
	LLPanel::Params panel_p;
	return LLUICtrlFactory::create<LLPanel>(panel_p);
}
