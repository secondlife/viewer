/** 
 * @file lluictrl.cpp
 * @author James Cook, Richard Nelson, Tom Yedwab
 * @brief Abstract base class for UI controls
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

#define LLUICTRL_CPP
#include "lluictrl.h"

#include "llfocusmgr.h"
#include "llpanel.h"
#include "lluictrlfactory.h"

static LLDefaultChildRegistry::Register<LLUICtrl> r("ui_ctrl");

F32 LLUICtrl::sActiveControlTransparency = 1.0f;
F32 LLUICtrl::sInactiveControlTransparency = 1.0f;

// Compiler optimization, generate extern template
template class LLUICtrl* LLView::getChild<class LLUICtrl>(
	const std::string& name, BOOL recurse) const;

LLUICtrl::CallbackParam::CallbackParam()
:	name("name"),
	function_name("function"),
	parameter("parameter"),
	control_name("control") // Shortcut to control -> "control_name" for backwards compatability			
{
	addSynonym(parameter, "userdata");
}

LLUICtrl::EnableControls::EnableControls()
:	enabled("enabled_control"),
	disabled("disabled_control")
{}

LLUICtrl::ControlVisibility::ControlVisibility()
:	visible("visibility_control"),
	invisible("invisibility_control")
{
	addSynonym(visible, "visiblity_control");
	addSynonym(invisible, "invisiblity_control");
}

LLUICtrl::Params::Params()
:	tab_stop("tab_stop", true),
	chrome("chrome", false),
	label("label"),
	initial_value("value"),
	init_callback("init_callback"),
	commit_callback("commit_callback"),
	validate_callback("validate_callback"),
	mouseenter_callback("mouseenter_callback"),
	mouseleave_callback("mouseleave_callback"),
	control_name("control_name"),
	font("font", LLFontGL::getFontSansSerif()),
	font_halign("halign"),
	font_valign("valign"),
	length("length"), 	// ignore LLXMLNode cruft
	type("type")   		// ignore LLXMLNode cruft
{
	addSynonym(initial_value, "initial_value");
}

// NOTE: the LLFocusableElement implementation has been moved from here to llfocusmgr.cpp.

//static 
const LLUICtrl::Params& LLUICtrl::getDefaultParams()
{
	return LLUICtrlFactory::getDefaultParams<LLUICtrl>();
}


LLUICtrl::LLUICtrl(const LLUICtrl::Params& p, const LLViewModelPtr& viewmodel) 
:	LLView(p),
	mTentative(FALSE),
	mIsChrome(FALSE),
	mTabStop(FALSE),
    mViewModel(viewmodel),
	mControlVariable(NULL),
	mEnabledControlVariable(NULL),
	mDisabledControlVariable(NULL),
	mMakeVisibleControlVariable(NULL),
	mMakeInvisibleControlVariable(NULL),
	mCommitSignal(NULL),
	mValidateSignal(NULL),
	mMouseEnterSignal(NULL),
	mMouseLeaveSignal(NULL),
	mMouseDownSignal(NULL),
	mMouseUpSignal(NULL),
	mRightMouseDownSignal(NULL),
	mRightMouseUpSignal(NULL),
	mDoubleClickSignal(NULL),
	mTransparencyType(TT_DEFAULT)
{
	mUICtrlHandle.bind(this);
}

void LLUICtrl::initFromParams(const Params& p)
{
	LLView::initFromParams(p);

	setIsChrome(p.chrome);
	setControlName(p.control_name);
	if(p.enabled_controls.isProvided())
	{
		if (p.enabled_controls.enabled.isChosen())
		{
			LLControlVariable* control = findControl(p.enabled_controls.enabled);
			if (control)
				setEnabledControlVariable(control);
		}
		else if(p.enabled_controls.disabled.isChosen())
		{
			LLControlVariable* control = findControl(p.enabled_controls.disabled);
			if (control)
				setDisabledControlVariable(control);
		}
	}
	if(p.controls_visibility.isProvided())
	{
		if (p.controls_visibility.visible.isChosen())
		{
			LLControlVariable* control = findControl(p.controls_visibility.visible);
			if (control)
				setMakeVisibleControlVariable(control);
		}
		else if (p.controls_visibility.invisible.isChosen())
		{
			LLControlVariable* control = findControl(p.controls_visibility.invisible);
			if (control)
				setMakeInvisibleControlVariable(control);
		}
	}

	setTabStop(p.tab_stop);

	if (p.initial_value.isProvided() 
		&& !p.control_name.isProvided())
	{
        setValue(p.initial_value);
	}
	
	if (p.commit_callback.isProvided())
	{
		setCommitCallback(initCommitCallback(p.commit_callback));
	}
	
	if (p.validate_callback.isProvided())
	{
		setValidateCallback(initEnableCallback(p.validate_callback));
	}
	
	if (p.init_callback.isProvided())
	{
		if (p.init_callback.function.isProvided())
		{
			p.init_callback.function()(this, p.init_callback.parameter);
		}
		else
		{
			commit_callback_t* initfunc = (CommitCallbackRegistry::getValue(p.init_callback.function_name));
			if (initfunc)
			{
				(*initfunc)(this, p.init_callback.parameter);
			}
		}
	}

	if(p.mouseenter_callback.isProvided())
	{
		setMouseEnterCallback(initCommitCallback(p.mouseenter_callback));
	}

	if(p.mouseleave_callback.isProvided())
	{
		setMouseLeaveCallback(initCommitCallback(p.mouseleave_callback));
	}
}


LLUICtrl::~LLUICtrl()
{
	gFocusMgr.releaseFocusIfNeeded( this ); // calls onCommit()

	if( gFocusMgr.getTopCtrl() == this )
	{
		llwarns << "UI Control holding top ctrl deleted: " << getName() << ".  Top view removed." << llendl;
		gFocusMgr.removeTopCtrlWithoutCallback( this );
	}

	delete mCommitSignal;
	delete mValidateSignal;
	delete mMouseEnterSignal;
	delete mMouseLeaveSignal;
	delete mMouseDownSignal;
	delete mMouseUpSignal;
	delete mRightMouseDownSignal;
	delete mRightMouseUpSignal;
	delete mDoubleClickSignal;
}

void default_commit_handler(LLUICtrl* ctrl, const LLSD& param)
{}

bool default_enable_handler(LLUICtrl* ctrl, const LLSD& param)
{
	return true;
}


LLUICtrl::commit_signal_t::slot_type LLUICtrl::initCommitCallback(const CommitCallbackParam& cb)
{
	if (cb.function.isProvided())
	{
		if (cb.parameter.isProvided())
			return boost::bind(cb.function(), _1, cb.parameter);
		else
			return cb.function();
	}
	else
	{
		std::string function_name = cb.function_name;
		commit_callback_t* func = (CommitCallbackRegistry::getValue(function_name));
		if (func)
		{
			if (cb.parameter.isProvided())
				return boost::bind((*func), _1, cb.parameter);
			else
				return commit_signal_t::slot_type(*func);
		}
		else if (!function_name.empty())
		{
			llwarns << "No callback found for: '" << function_name << "' in control: " << getName() << llendl;
		}			
	}
	return default_commit_handler;
}

LLUICtrl::enable_signal_t::slot_type LLUICtrl::initEnableCallback(const EnableCallbackParam& cb)
{
	// Set the callback function
	if (cb.function.isProvided())
	{
		if (cb.parameter.isProvided())
			return boost::bind(cb.function(), this, cb.parameter);
		else
			return cb.function();
	}
	else
	{
		enable_callback_t* func = (EnableCallbackRegistry::getValue(cb.function_name));
		if (func)
		{
			if (cb.parameter.isProvided())
				return boost::bind((*func), this, cb.parameter);
			else
				return enable_signal_t::slot_type(*func);
		}
	}
	return default_enable_handler;
}

// virtual
void LLUICtrl::onMouseEnter(S32 x, S32 y, MASK mask)
{
	if (mMouseEnterSignal)
	{
		(*mMouseEnterSignal)(this, getValue());
	}
}

// virtual
void LLUICtrl::onMouseLeave(S32 x, S32 y, MASK mask)
{
	if(mMouseLeaveSignal)
	{
		(*mMouseLeaveSignal)(this, getValue());
	}
}

//virtual 
BOOL LLUICtrl::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled  = LLView::handleMouseDown(x,y,mask);
	if (mMouseDownSignal)
	{
		(*mMouseDownSignal)(this,x,y,mask);
	}
	return handled;
}

//virtual
BOOL LLUICtrl::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled  = LLView::handleMouseUp(x,y,mask);
	if (mMouseUpSignal)
	{
		(*mMouseUpSignal)(this,x,y,mask);
	}
	return handled;
}

//virtual
BOOL LLUICtrl::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled  = LLView::handleRightMouseDown(x,y,mask);
	if (mRightMouseDownSignal)
	{
		(*mRightMouseDownSignal)(this,x,y,mask);
	}
	return handled;
}

//virtual
BOOL LLUICtrl::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled  = LLView::handleRightMouseUp(x,y,mask);
	if(mRightMouseUpSignal)
	{
		(*mRightMouseUpSignal)(this,x,y,mask);
	}
	return handled;
}

BOOL LLUICtrl::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLView::handleDoubleClick(x, y, mask);
	if (mDoubleClickSignal)
	{
		(*mDoubleClickSignal)(this, x, y, mask);
	}
	return handled;
}

// can't tab to children of a non-tab-stop widget
BOOL LLUICtrl::canFocusChildren() const
{
	return hasTabStop();
}


void LLUICtrl::onCommit()
{
	if (mCommitSignal)
	(*mCommitSignal)(this, getValue());
}

//virtual
BOOL LLUICtrl::isCtrl() const
{
	return TRUE;
}

//virtual 
void LLUICtrl::setValue(const LLSD& value)
{
    mViewModel->setValue(value);
}

//virtual
LLSD LLUICtrl::getValue() const
{
	return mViewModel->getValue();
}

/// When two widgets are displaying the same data (e.g. during a skin
/// change), share their ViewModel.
void    LLUICtrl::shareViewModelFrom(const LLUICtrl& other)
{
    // Because mViewModel is an LLViewModelPtr, this assignment will quietly
    // dispose of the previous LLViewModel -- unless it's already shared by
    // somebody else.
    mViewModel = other.mViewModel;
}

//virtual
LLViewModel* LLUICtrl::getViewModel() const
{
	return mViewModel;
}

bool LLUICtrl::setControlValue(const LLSD& value)
{
	if (mControlVariable)
	{
		mControlVariable->set(value);
		return true;
	}
	return false;
}

void LLUICtrl::setControlVariable(LLControlVariable* control)
{
	if (mControlVariable)
	{
		//RN: this will happen in practice, should we try to avoid it?
		//llwarns << "setControlName called twice on same control!" << llendl;
		mControlConnection.disconnect(); // disconnect current signal
		mControlVariable = NULL;
	}
	
	if (control)
	{
		mControlVariable = control;
		mControlConnection = mControlVariable->getSignal()->connect(boost::bind(&controlListener, _2, getUICtrlHandle(), std::string("value")));
		setValue(mControlVariable->getValue());
	}
}

//virtual
void LLUICtrl::setControlName(const std::string& control_name, LLView *context)
{
	if (context == NULL)
	{
		context = this;
	}

	// Register new listener
	if (!control_name.empty())
	{
		LLControlVariable* control = context->findControl(control_name);
		setControlVariable(control);
	}
}

void LLUICtrl::setEnabledControlVariable(LLControlVariable* control)
{
	if (mEnabledControlVariable)
	{
		mEnabledControlConnection.disconnect(); // disconnect current signal
		mEnabledControlVariable = NULL;
	}
	if (control)
	{
		mEnabledControlVariable = control;
		mEnabledControlConnection = mEnabledControlVariable->getSignal()->connect(boost::bind(&controlListener, _2, getUICtrlHandle(), std::string("enabled")));
		setEnabled(mEnabledControlVariable->getValue().asBoolean());
	}
}

void LLUICtrl::setDisabledControlVariable(LLControlVariable* control)
{
	if (mDisabledControlVariable)
	{
		mDisabledControlConnection.disconnect(); // disconnect current signal
		mDisabledControlVariable = NULL;
	}
	if (control)
	{
		mDisabledControlVariable = control;
		mDisabledControlConnection = mDisabledControlVariable->getSignal()->connect(boost::bind(&controlListener, _2, getUICtrlHandle(), std::string("disabled")));
		setEnabled(!(mDisabledControlVariable->getValue().asBoolean()));
	}
}

void LLUICtrl::setMakeVisibleControlVariable(LLControlVariable* control)
{
	if (mMakeVisibleControlVariable)
	{
		mMakeVisibleControlConnection.disconnect(); // disconnect current signal
		mMakeVisibleControlVariable = NULL;
	}
	if (control)
	{
		mMakeVisibleControlVariable = control;
		mMakeVisibleControlConnection = mMakeVisibleControlVariable->getSignal()->connect(boost::bind(&controlListener, _2, getUICtrlHandle(), std::string("visible")));
		setVisible(mMakeVisibleControlVariable->getValue().asBoolean());
	}
}

void LLUICtrl::setMakeInvisibleControlVariable(LLControlVariable* control)
{
	if (mMakeInvisibleControlVariable)
	{
		mMakeInvisibleControlConnection.disconnect(); // disconnect current signal
		mMakeInvisibleControlVariable = NULL;
	}
	if (control)
	{
		mMakeInvisibleControlVariable = control;
		mMakeInvisibleControlConnection = mMakeInvisibleControlVariable->getSignal()->connect(boost::bind(&controlListener, _2, getUICtrlHandle(), std::string("invisible")));
		setVisible(!(mMakeInvisibleControlVariable->getValue().asBoolean()));
	}
}
// static
bool LLUICtrl::controlListener(const LLSD& newvalue, LLHandle<LLUICtrl> handle, std::string type)
{
	LLUICtrl* ctrl = handle.get();
	if (ctrl)
	{
		if (type == "value")
		{
			ctrl->setValue(newvalue);
			return true;
		}
		else if (type == "enabled")
		{
			ctrl->setEnabled(newvalue.asBoolean());
			return true;
		}
		else if(type =="disabled")
		{
			ctrl->setEnabled(!newvalue.asBoolean());
			return true;
		}
		else if (type == "visible")
		{
			ctrl->setVisible(newvalue.asBoolean());
			return true;
		}
		else if (type == "invisible")
		{
			ctrl->setVisible(!newvalue.asBoolean());
			return true;
		}
	}
	return false;
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
	return mViewModel->isDirty();
};

//virtual
void LLUICtrl::resetDirty()
{
    mViewModel->resetDirty();
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
	CompareByDefaultTabGroup(const LLView::child_tab_order_t& order, S32 default_tab_group):
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

LLFastTimer::DeclareTimer FTM_FOCUS_FIRST_ITEM("Focus First Item");

BOOL LLUICtrl::focusFirstItem(BOOL prefer_text_fields, BOOL focus_flash)
{
	LLFastTimer _(FTM_FOCUS_FIRST_ITEM);
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
	static LLUICachedControl<bool> tab_to_text_fields_only ("TabToTextFieldsOnly", false);
	if(text_fields_only || tab_to_text_fields_only)
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
	static LLUICachedControl<bool> tab_to_text_fields_only ("TabToTextFieldsOnly", false);
	if(text_fields_only || tab_to_text_fields_only)
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
	while(next_view && next_view->hasTabStop())
	{
		if (next_view->isFocusRoot())
		{
			focus_root = next_view;
		}
		next_view = next_view->getParentUICtrl();
	}

	return focus_root;
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

bool LLUICtrl::findHelpTopic(std::string& help_topic_out)
{
	LLUICtrl* ctrl = this;

	// search back through the control's parents for a panel
	// or tab with a help_topic string defined
	while (ctrl)
	{
		LLPanel *panel = dynamic_cast<LLPanel *>(ctrl);

		if (panel)
		{
			// does the panel have a sub-panel with a help topic?
			LLPanel *subpanel = panel->childGetVisiblePanelWithHelp();
			if (subpanel)
			{
				help_topic_out = subpanel->getHelpTopic();
				return true; // success (subpanel)
			}

			// does the panel have an active tab with a help topic?
			LLPanel *tab = panel->childGetVisibleTabWithHelp();
			if (tab)
			{
				help_topic_out = tab->getHelpTopic();
				return true; // success (tab)
			}

			// otherwise, does the panel have a help topic itself?
			if (!panel->getHelpTopic().empty())
			{
				help_topic_out = panel->getHelpTopic();
				return true; // success (panel)
			}		
		}

		ctrl = ctrl->getParentUICtrl();
	}

	return false; // no help topic found
}

// *TODO: Deprecate; for backwards compatability only:
boost::signals2::connection LLUICtrl::setCommitCallback( boost::function<void (LLUICtrl*,void*)> cb, void* data)
{
	return setCommitCallback( boost::bind(cb, _1, data));
}
boost::signals2::connection LLUICtrl::setValidateBeforeCommit( boost::function<bool (const LLSD& data)> cb )
{
	if (!mValidateSignal) mValidateSignal = new enable_signal_t();
	return mValidateSignal->connect(boost::bind(cb, _2));
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
void LLUICtrl::setColor(const LLColor4& color)							
{ }

F32 LLUICtrl::getCurrentTransparency()
{
	F32 alpha = 0;

	switch(mTransparencyType)
	{
	case TT_DEFAULT:
		alpha = getDrawContext().mAlpha;
		break;

	case TT_ACTIVE:
		alpha = sActiveControlTransparency;
		break;

	case TT_INACTIVE:
		alpha = sInactiveControlTransparency;
		break;

	case TT_FADING:
		alpha = sInactiveControlTransparency / 2;
		break;
	}

	return alpha;
}

void LLUICtrl::setTransparencyType(ETypeTransparency type)
{
	mTransparencyType = type;
}

boost::signals2::connection LLUICtrl::setCommitCallback( const commit_signal_t::slot_type& cb ) 
{ 
	if (!mCommitSignal) mCommitSignal = new commit_signal_t();
	return mCommitSignal->connect(cb); 
}

boost::signals2::connection LLUICtrl::setValidateCallback( const enable_signal_t::slot_type& cb ) 
{ 
	if (!mValidateSignal) mValidateSignal = new enable_signal_t();
	return mValidateSignal->connect(cb); 
}

boost::signals2::connection LLUICtrl::setMouseEnterCallback( const commit_signal_t::slot_type& cb ) 
{ 
	if (!mMouseEnterSignal) mMouseEnterSignal = new commit_signal_t();
	return mMouseEnterSignal->connect(cb); 
}

boost::signals2::connection LLUICtrl::setMouseLeaveCallback( const commit_signal_t::slot_type& cb ) 
{ 
	if (!mMouseLeaveSignal) mMouseLeaveSignal = new commit_signal_t();
	return mMouseLeaveSignal->connect(cb); 
}

boost::signals2::connection LLUICtrl::setMouseDownCallback( const mouse_signal_t::slot_type& cb ) 
{ 
	if (!mMouseDownSignal) mMouseDownSignal = new mouse_signal_t();
	return mMouseDownSignal->connect(cb); 
}

boost::signals2::connection LLUICtrl::setMouseUpCallback( const mouse_signal_t::slot_type& cb ) 
{ 
	if (!mMouseUpSignal) mMouseUpSignal = new mouse_signal_t();
	return mMouseUpSignal->connect(cb); 
}

boost::signals2::connection LLUICtrl::setRightMouseDownCallback( const mouse_signal_t::slot_type& cb ) 
{ 
	if (!mRightMouseDownSignal) mRightMouseDownSignal = new mouse_signal_t();
	return mRightMouseDownSignal->connect(cb); 
}

boost::signals2::connection LLUICtrl::setRightMouseUpCallback( const mouse_signal_t::slot_type& cb ) 
{ 
	if (!mRightMouseUpSignal) mRightMouseUpSignal = new mouse_signal_t();
	return mRightMouseUpSignal->connect(cb); 
}

boost::signals2::connection LLUICtrl::setDoubleClickCallback( const mouse_signal_t::slot_type& cb ) 
{ 
	if (!mDoubleClickSignal) mDoubleClickSignal = new mouse_signal_t();
	return mDoubleClickSignal->connect(cb); 
}
