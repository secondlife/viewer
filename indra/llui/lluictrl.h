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

#include "llboost.h"
#include "llrect.h"
#include "llsd.h"
#include <boost/function.hpp>

#include "llinitparam.h"
#include "llview.h"
#include "llviewmodel.h"

const BOOL TAKE_FOCUS_YES = TRUE;
const BOOL TAKE_FOCUS_NO  = FALSE;

class LLFocusableElement
{
	friend class LLFocusMgr; // allow access to focus change handlers
public:
	LLFocusableElement();
	virtual ~LLFocusableElement();

	virtual void	setFocus( BOOL b );
	virtual BOOL	hasFocus() const;

	typedef boost::function<void(LLFocusableElement*, void*)> focus_callback_t;
	void	setFocusLostCallback(focus_callback_t cb, void* user_data = NULL)	{ mFocusLostCallback = cb; mFocusCallbackUserData = user_data; }
	void	setFocusReceivedCallback(focus_callback_t cb, void* user_data = NULL)	{ mFocusReceivedCallback = cb; mFocusCallbackUserData = user_data; }
	void	setFocusChangedCallback(focus_callback_t cb, void* user_data = NULL )	{ mFocusChangedCallback = cb; mFocusCallbackUserData = user_data; }
	void	setTopLostCallback(focus_callback_t cb, void* user_data = NULL )	{ mTopLostCallback = cb; mFocusCallbackUserData = user_data; }

protected:
	virtual void	onFocusReceived();
	virtual void	onFocusLost();
	virtual void	onTopLost();	// called when registered as top ctrl and user clicks elsewhere
	focus_callback_t mFocusLostCallback;
	focus_callback_t mFocusReceivedCallback;
	focus_callback_t mFocusChangedCallback;
	focus_callback_t mTopLostCallback;
	void*			mFocusCallbackUserData;
};

class LLUICtrl
	: public LLView, public LLFocusableElement, public boost::signals2::trackable
{
public:


	typedef boost::function<void (LLUICtrl* ctrl, const LLSD& param)> commit_callback_t;
	typedef boost::signals2::signal<void (LLUICtrl* ctrl, const LLSD& param)> commit_signal_t;
	
	typedef boost::function<bool (LLUICtrl* ctrl, const LLSD& param)> enable_callback_t;
	typedef boost::signals2::signal<bool (LLUICtrl* ctrl, const LLSD& param), boost_boolean_combiner> enable_signal_t;
	
	struct CallbackParam : public LLInitParam::Block<CallbackParam>
	{
		Deprecated				name;

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
	
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<std::string>			label;
		Optional<bool>					tab_stop;
		Optional<LLSD>					initial_value;

		Optional<CommitCallbackParam>	init_callback,
										commit_callback;
		Optional<EnableCallbackParam>	validate_callback;
		
		Optional<focus_callback_t>		focus_lost_callback;
		
		Optional<std::string>			control_name;
		Optional<std::string>			enabled_control;
		
		Params();
	};

	/*virtual*/ ~LLUICtrl();

	void initFromParams(const Params& p);
protected:
	friend class LLUICtrlFactory;
	LLUICtrl(const Params& p = LLUICtrl::Params(),
             const LLViewModelPtr& viewmodel=LLViewModelPtr(new LLViewModel));
	
	void initCommitCallback(const CommitCallbackParam& cb, commit_signal_t& sig);
	void initEnableCallback(const EnableCallbackParam& cb, enable_signal_t& sig);

	// We need this virtual so we can override it with derived versions
	virtual LLViewModel* getViewModel() const;
    // We shouldn't ever need to set this directly
    //virtual void    setViewModel(const LLViewModelPtr&);
	
public:
	// LLView interface
	/*virtual*/ BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );
	/*virtual*/ void	onFocusReceived();
	/*virtual*/ void	onFocusLost();
	/*virtual*/ void	onTopLost();
	/*virtual*/ BOOL	isCtrl() const;
	/*virtual*/ void	setTentative(BOOL b);
	/*virtual*/ BOOL	getTentative() const;

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
	virtual void	clear();
	virtual void	setColor(const LLColor4& color);
	virtual void	setMinValue(LLSD min_value);
	virtual void	setMaxValue(LLSD max_value);

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

	boost::signals2::connection setCommitCallback( const commit_signal_t::slot_type& cb ) { return mCommitSignal.connect(cb); }
	boost::signals2::connection setValidateCallback( const enable_signal_t::slot_type& cb ) { return mValidateSignal.connect(cb); }
	
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
	
	template <typename F> class CallbackRegistry : public LLRegistrySingleton<std::string, F, CallbackRegistry<F> >
	{};	

	typedef CallbackRegistry<commit_callback_t> CommitCallbackRegistry;
	typedef CallbackRegistry<enable_callback_t> EnableCallbackRegistry;
	
protected:

	static bool controlListener(const LLSD& newvalue, LLHandle<LLUICtrl> handle, std::string type);

	commit_signal_t	mCommitSignal;
	enable_signal_t mValidateSignal;

    LLViewModelPtr  mViewModel;

	LLControlVariable* mControlVariable;
	boost::signals2::connection mControlConnection;
	LLControlVariable* mEnabledControlVariable;
	boost::signals2::connection mEnabledControlConnection;

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
	bool ParamCompare<LLUICtrl::focus_callback_t>::equals(
		const LLUICtrl::focus_callback_t &a, 
		const LLUICtrl::focus_callback_t &b); 
}

#endif  // LL_LLUICTRL_H
