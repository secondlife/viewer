/** 
 * @file lluictrl.cpp
 * @author James Cook, Richard Nelson, Tom Yedwab
 * @brief Abstract base class for UI controls
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

//#include "llviewerprecompiledheaders.h"
#include "linden_common.h"
#include "lluictrl.h"
#include "llfocusmgr.h"
#include "llpanel.h"
#include "lluictrlfactory.h"

static LLDefaultWidgetRegistry::Register<LLUICtrl> r("ui_ctrl");

LLUICtrl::Params::Params()
:	tab_stop("tab_stop", true),
	label("label"),
	initial_value("initial_value"),
	init_callback("init_callback"),
	commit_callback("commit_callback"),
	validate_callback("validate_callback"),
	rightclick_callback("rightclick_callback"),
	mouseenter_callback("mouseenter_callback"),
	mouseleave_callback("mouseleave_callback"),
	control_name("control_name")
{
	addSynonym(initial_value, "initial_val");
	// this is the canonical name for text contents of an xml node
	addSynonym(initial_value, "value");
}

LLFocusableElement::LLFocusableElement()
:	mFocusLostCallback(NULL),
	mFocusReceivedCallback(NULL),
	mFocusChangedCallback(NULL),
	mTopLostCallback(NULL),
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

void LLFocusableElement::onTopLost()
{
	if (mTopLostCallback)
	{
		mTopLostCallback(this, mFocusCallbackUserData);
	}
}

BOOL LLFocusableElement::hasFocus() const
{
	return FALSE;
}

void LLFocusableElement::setFocus(BOOL b)
{
}

//static 
const LLUICtrl::Params& LLUICtrl::getDefaultParams()
{
	return LLUICtrlFactory::getDefaultParams<LLUICtrl::Params>();
}


LLUICtrl::LLUICtrl(const LLUICtrl::Params& p, const LLViewModelPtr& viewmodel) 
:	LLView(p),
	mTentative(FALSE),
	mIsChrome(FALSE),
    mViewModel(viewmodel),
	mControlVariable(NULL),
	mEnabledControlVariable(NULL),
	mDisabledControlVariable(NULL)
{
	mUICtrlHandle.bind(this);
}

void LLUICtrl::initFromParams(const Params& p)
{
	LLView::initFromParams(p);

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
	setFocusLostCallback(p.focus_lost_callback());

	if (p.initial_value.isProvided() 
		&& !p.control_name.isProvided())
	{
        setValue(p.initial_value);
	}
	
	if (p.commit_callback.isProvided())
		initCommitCallback(p.commit_callback, mCommitSignal);
	
	if (p.validate_callback.isProvided())
		initEnableCallback(p.validate_callback, mValidateSignal);
	
	if (p.init_callback.isProvided())
	{
		if (p.init_callback.function.isProvided())
		{
			p.init_callback.function()(this, p.init_callback.parameter);
		}
		else
		{
			commit_callback_t* initfunc = (CallbackRegistry<commit_callback_t>::getValue(p.init_callback.function_name));
			if (initfunc)
			{
				(*initfunc)(this, p.init_callback.parameter);
			}
		}
	}

	if(p.rightclick_callback.isProvided())
		initCommitCallback(p.rightclick_callback, mRightClickSignal);

	if(p.mouseenter_callback.isProvided())
		initCommitCallback(p.mouseenter_callback, mMouseEnterSignal);

	if(p.mouseleave_callback.isProvided())
		initCommitCallback(p.mouseleave_callback, mMouseLeaveSignal);
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

void LLUICtrl::initCommitCallback(const CommitCallbackParam& cb, commit_signal_t& sig)
{
	if (cb.function.isProvided())
	{
		if (cb.parameter.isProvided())
			sig.connect(boost::bind(cb.function(), _1, cb.parameter));
		else
			sig.connect(cb.function());
	}
	else
	{
		std::string function_name = cb.function_name;
		commit_callback_t* func = (CallbackRegistry<commit_callback_t>::getValue(function_name));
		if (func)
		{
			if (cb.parameter.isProvided())
				sig.connect(boost::bind((*func), _1, cb.parameter));
			else
				sig.connect(*func);
		}
		else if (!function_name.empty())
		{
			llwarns << "No callback found for: '" << function_name << "' in control: " << getName() << llendl;
		}			
	}
}

void LLUICtrl::initEnableCallback(const EnableCallbackParam& cb, enable_signal_t& sig)
{
	// Set the callback function
	if (cb.function.isProvided())
	{
		if (cb.parameter.isProvided())
			sig.connect(boost::bind(cb.function(), this, cb.parameter));
		else
			sig.connect(cb.function());
	}
	else
	{
		enable_callback_t* func = (EnableCallbackRegistry::getValue(cb.function_name));
		if (func)
		{
			if (cb.parameter.isProvided())
				sig.connect(boost::bind((*func), this, cb.parameter));
			else
				sig.connect(*func);
		}
	}
}

// virtual
void LLUICtrl::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mMouseEnterSignal(this, getValue());
}

// virtual
void LLUICtrl::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mMouseLeaveSignal(this, getValue());
}

void LLUICtrl::onCommit()
{
	mCommitSignal(this, getValue());
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

void LLUICtrl::onTopLost()
{
	// trigger callbacks
	LLFocusableElement::onTopLost();
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

// *TODO: Deprecate; for backwards compatability only:
boost::signals2::connection LLUICtrl::setCommitCallback( boost::function<void (LLUICtrl*,void*)> cb, void* data)
{
	return setCommitCallback( boost::bind(cb, _1, data));
}
boost::signals2::connection LLUICtrl::setValidateBeforeCommit( boost::function<bool (const LLSD& data)> cb )
{
	return mValidateSignal.connect(boost::bind(cb, _2));
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

// virtual
void LLUICtrl::setMinValue(LLSD min_value)								
{ }

// virtual
void LLUICtrl::setMaxValue(LLSD max_value)								
{ }



namespace LLInitParam
{
    template<> 
	bool ParamCompare<LLUICtrl::commit_callback_t>::equals(
		const LLUICtrl::commit_callback_t &a, 
		const LLUICtrl::commit_callback_t &b)
    {
    	return false;
    }
    
    template<> 
	bool ParamCompare<LLUICtrl::focus_callback_t>::equals(
		const LLUICtrl::focus_callback_t &a, 
		const LLUICtrl::focus_callback_t &b)
    {
    	return false;
    }
    
    template<> 
	bool ParamCompare<LLUICtrl::enable_callback_t>::equals(
		const LLUICtrl::enable_callback_t &a, 
		const LLUICtrl::enable_callback_t &b)
    {
    	return false;
    }

    template<> 
	bool ParamCompare<LLLazyValue<LLColor4> >::equals(
		const LLLazyValue<LLColor4> &a, 
		const LLLazyValue<LLColor4> &b)    
    {
    	return a.get() == b.get();
    }
}
