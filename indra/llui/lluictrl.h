/** 
 * @file lluictrl.h
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

#ifndef LL_LLUICTRL_H
#define LL_LLUICTRL_H

//#include "llboost.h"
#include "llrect.h"
#include "llsd.h"
#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include "llinitparam.h"
#include "llview.h"
#include "llviewmodel.h"		// *TODO move dependency to .cpp file

const BOOL TAKE_FOCUS_YES = TRUE;
const BOOL TAKE_FOCUS_NO  = FALSE;

// NOTE: the LLFocusableElement class declaration has been moved from here to llfocusmgr.h.

class LLUICtrl
	: public LLView, public boost::signals2::trackable
{
public:


	typedef boost::function<void (LLUICtrl* ctrl, const LLSD& param)> commit_callback_t;
	typedef boost::signals2::signal<void (LLUICtrl* ctrl, const LLSD& param)> commit_signal_t;
	// *TODO: add xml support for this type of signal in the future
	typedef boost::signals2::signal<void (LLUICtrl* ctrl, S32 x, S32 y, MASK mask)> mouse_signal_t;
	
	typedef boost::function<bool (LLUICtrl* ctrl, const LLSD& param)> enable_callback_t;
	typedef boost::signals2::signal<bool (LLUICtrl* ctrl, const LLSD& param), boost_boolean_combiner> enable_signal_t;
	
	typedef boost::function<bool (LLUICtrl* ctrl, const LLSD& param)> visible_callback_t;
	typedef boost::signals2::signal<bool (LLUICtrl* ctrl, const LLSD& param), boost_boolean_combiner> visible_signal_t;
	
	struct CallbackParam : public LLInitParam::Block<CallbackParam>
	{
		Ignored					name;

		Optional<std::string>	function_name;
		Optional<LLSD>			parameter;

		Optional<std::string>	control_name;
		
		CallbackParam()
		  :	name("name"),
			function_name("function"),
			parameter("parameter"),
			control_name("control") // Shortcut to control -> "control_name" for backwards compatability			
		{
			addSynonym(parameter, "userdata");
		}
	};

	struct CommitCallbackParam : public LLInitParam::Block<CommitCallbackParam, CallbackParam >
	{
		Optional<commit_callback_t> function;
	};

	struct EnableCallbackParam : public LLInitParam::Block<EnableCallbackParam, CallbackParam >
	{
		Optional<enable_callback_t> function;
	};
	
	struct VisibleCallbackParam : public LLInitParam::Block<VisibleCallbackParam, CallbackParam >
	{
		Optional<visible_callback_t> function;
	};
	
	struct EnableControls : public LLInitParam::Choice<EnableControls>
	{
		Alternative<std::string> enabled;
		Alternative<std::string> disabled;
		
		EnableControls()
		: enabled("enabled_control"),
		  disabled("disabled_control")
		{}
	};	
	struct ControlVisibility : public LLInitParam::Choice<ControlVisibility>
	{
		Alternative<std::string> visible;
		Alternative<std::string> invisible;

		ControlVisibility()
		:	visible("visibility_control"),
			invisible("invisibility_control")
		{
			addSynonym(visible, "visiblity_control");
			addSynonym(invisible, "invisiblity_control");
		}
	};	
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<std::string>			label;
		Optional<bool>					tab_stop;
		Optional<LLSD>					initial_value;

		Optional<CommitCallbackParam>	init_callback,
										commit_callback;
		Optional<EnableCallbackParam>	validate_callback;
		
		Optional<CommitCallbackParam>	mouseenter_callback;
		Optional<CommitCallbackParam>	mouseleave_callback;
		
		Optional<std::string>			control_name;
		Optional<EnableControls>		enabled_controls;
		Optional<ControlVisibility>		controls_visibility;
		
		// font params
		Optional<const LLFontGL*>		font;
		Optional<LLFontGL::HAlign>		font_halign;
		Optional<LLFontGL::VAlign>		font_valign;

		// cruft from LLXMLNode implementation
		Ignored							type,
										length;

		Params();
	};

	/*virtual*/ ~LLUICtrl();

	void initFromParams(const Params& p);
protected:
	friend class LLUICtrlFactory;
	static const Params& getDefaultParams();
	LLUICtrl(const Params& p = getDefaultParams(),
             const LLViewModelPtr& viewmodel=LLViewModelPtr(new LLViewModel));
	
	void initCommitCallback(const CommitCallbackParam& cb, commit_signal_t& sig);
	void initEnableCallback(const EnableCallbackParam& cb, enable_signal_t& sig);
	void initVisibleCallback(const VisibleCallbackParam& cb, visible_signal_t& sig);

	// We need this virtual so we can override it with derived versions
	virtual LLViewModel* getViewModel() const;
    // We shouldn't ever need to set this directly
    //virtual void    setViewModel(const LLViewModelPtr&);
	
public:
	// LLView interface
	/*virtual*/ BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );
	/*virtual*/ BOOL	isCtrl() const;
	/*virtual*/ void	setTentative(BOOL b);
	/*virtual*/ BOOL	getTentative() const;
	/*virtual*/ void	onMouseEnter(S32 x, S32 y, MASK mask);
	/*virtual*/ void	onMouseLeave(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	canFocusChildren() const;
	/*virtual*/ BOOL 	handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL 	handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleRightMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);

	// From LLFocusableElement
	/*virtual*/ void	setFocus( BOOL b );
	/*virtual*/ BOOL	hasFocus() const;
	
	// New virtuals

	// Return NULL by default (overrride if the class has the appropriate interface)
	virtual class LLCtrlSelectionInterface* getSelectionInterface();
	virtual class LLCtrlListInterface* getListInterface();
	virtual class LLCtrlScrollInterface* getScrollInterface();

	bool setControlValue(const LLSD& value);
	void setControlVariable(LLControlVariable* control);
	virtual void setControlName(const std::string& control, LLView *context = NULL);
	
	LLControlVariable* getControlVariable() { return mControlVariable; } 
	
	void setEnabledControlVariable(LLControlVariable* control);
	void setDisabledControlVariable(LLControlVariable* control);
	void setMakeVisibleControlVariable(LLControlVariable* control);
	void setMakeInvisibleControlVariable(LLControlVariable* control);

	virtual void	setValue(const LLSD& value);
	virtual LLSD	getValue() const;
    /// When two widgets are displaying the same data (e.g. during a skin
    /// change), share their ViewModel.
    virtual void    shareViewModelFrom(const LLUICtrl& other);

	virtual BOOL	setTextArg(  const std::string& key, const LLStringExplicit& text );
	virtual void	setIsChrome(BOOL is_chrome);

	virtual BOOL	acceptsTextInput() const; // Defaults to false

	// A control is dirty if the user has modified its value.
	// Editable controls should override this.
	virtual BOOL	isDirty() const; // Defauls to false
	virtual void	resetDirty(); //Defaults to no-op
	
	// Call appropriate callback
	virtual void	onCommit();
	
	// Default to no-op:
	virtual void	onTabInto();

	// Clear any user-provided input (text in a text editor, checked checkbox,
	// selected radio button, etc.).  Defaults to no-op.
	virtual void	clear();

	virtual void	setColor(const LLColor4& color);

	BOOL	focusNextItem(BOOL text_entry_only);
	BOOL	focusPrevItem(BOOL text_entry_only);
	BOOL 	focusFirstItem(BOOL prefer_text_fields = FALSE, BOOL focus_flash = TRUE );
	BOOL	focusLastItem(BOOL prefer_text_fields = FALSE);

	// Non Virtuals
	LLHandle<LLUICtrl> getUICtrlHandle() const { return mUICtrlHandle; }
	BOOL			getIsChrome() const;
	
	void			setTabStop( BOOL b );
	BOOL			hasTabStop() const;

	LLUICtrl*		getParentUICtrl() const;

	// return true if help topic found by crawling through parents -
	// topic then put in help_topic_out
	bool                    findHelpTopic(std::string& help_topic_out);

	boost::signals2::connection setCommitCallback( const commit_signal_t::slot_type& cb ) { return mCommitSignal.connect(cb); }
	boost::signals2::connection setValidateCallback( const enable_signal_t::slot_type& cb ) { return mValidateSignal.connect(cb); }

	boost::signals2::connection setMouseEnterCallback( const commit_signal_t::slot_type& cb ) { return mMouseEnterSignal.connect(cb); }
	boost::signals2::connection setMouseLeaveCallback( const commit_signal_t::slot_type& cb ) { return mMouseLeaveSignal.connect(cb); }
	
	boost::signals2::connection setMouseDownCallback( const mouse_signal_t::slot_type& cb ) { return mMouseDownSignal.connect(cb); }
	boost::signals2::connection setMouseUpCallback( const mouse_signal_t::slot_type& cb ) { return mMouseUpSignal.connect(cb); }
	boost::signals2::connection setRightMouseDownCallback( const mouse_signal_t::slot_type& cb ) { return mRightMouseDownSignal.connect(cb); }
	boost::signals2::connection setRightMouseUpCallback( const mouse_signal_t::slot_type& cb ) { return mRightMouseUpSignal.connect(cb); }
	
	boost::signals2::connection setDoubleClickCallback( const mouse_signal_t::slot_type& cb ) { return mDoubleClickSignal.connect(cb); }

	// *TODO: Deprecate; for backwards compatability only:
	boost::signals2::connection setCommitCallback( boost::function<void (LLUICtrl*,void*)> cb, void* data);	
	boost::signals2::connection setValidateBeforeCommit( boost::function<bool (const LLSD& data)> cb );

	LLUICtrl* findRootMostFocusRoot();

	class LLTextInputFilter : public LLQueryFilter, public LLSingleton<LLTextInputFilter>
	{
		/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const 
		{
			return filterResult_t(view->isCtrl() && static_cast<const LLUICtrl *>(view)->acceptsTextInput(), TRUE);
		}
	};
	
	template <typename F, typename DERIVED> class CallbackRegistry : public LLRegistrySingleton<std::string, F, DERIVED >
	{};	

	class CommitCallbackRegistry : public CallbackRegistry<commit_callback_t, CommitCallbackRegistry>{};
	class EnableCallbackRegistry : public CallbackRegistry<enable_callback_t, EnableCallbackRegistry>{};
	class VisibleCallbackRegistry : public CallbackRegistry<visible_callback_t, VisibleCallbackRegistry>{};

	
protected:

	static bool controlListener(const LLSD& newvalue, LLHandle<LLUICtrl> handle, std::string type);

	commit_signal_t		mCommitSignal;
	enable_signal_t		mValidateSignal;

	commit_signal_t		mMouseEnterSignal;
	commit_signal_t		mMouseLeaveSignal;
	
	mouse_signal_t		mMouseDownSignal;
	mouse_signal_t		mMouseUpSignal;
	mouse_signal_t		mRightMouseDownSignal;
	mouse_signal_t		mRightMouseUpSignal;

	mouse_signal_t		mDoubleClickSignal;
	
    LLViewModelPtr  mViewModel;

	LLControlVariable* mControlVariable;
	boost::signals2::connection mControlConnection;
	LLControlVariable* mEnabledControlVariable;
	boost::signals2::connection mEnabledControlConnection;
	LLControlVariable* mDisabledControlVariable;
	boost::signals2::connection mDisabledControlConnection;
	LLControlVariable* mMakeVisibleControlVariable;
	boost::signals2::connection mMakeVisibleControlConnection;
	LLControlVariable* mMakeInvisibleControlVariable;
	boost::signals2::connection mMakeInvisibleControlConnection;
private:

	BOOL			mTabStop;
	BOOL			mIsChrome;
	BOOL			mTentative;
	LLRootHandle<LLUICtrl> mUICtrlHandle;

	class DefaultTabGroupFirstSorter;
};

namespace LLInitParam
{   
    template<> 
	bool ParamCompare<LLUICtrl::commit_callback_t>::equals(
		const LLUICtrl::commit_callback_t &a, 
		const LLUICtrl::commit_callback_t &b); 
		
    template<> 
	bool ParamCompare<LLUICtrl::enable_callback_t>::equals(
		const LLUICtrl::enable_callback_t &a, 
		const LLUICtrl::enable_callback_t &b); 
    
    template<>
	bool ParamCompare<LLLazyValue<LLColor4> >::equals(
		const LLLazyValue<LLColor4> &a, const LLLazyValue<LLColor4> &b); 
}

#endif  // LL_LLUICTRL_H
