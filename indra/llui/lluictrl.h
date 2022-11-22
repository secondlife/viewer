/** 
 * @file lluictrl.h
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
#include "llsearchablecontrol.h"

const BOOL TAKE_FOCUS_YES = TRUE;
const BOOL TAKE_FOCUS_NO  = FALSE;

class LLUICtrl
	: public LLView, public boost::signals2::trackable
{
public:
	typedef boost::function<void (LLUICtrl* ctrl, const LLSD& param)> commit_callback_t;
	typedef boost::signals2::signal<void (LLUICtrl* ctrl, const LLSD& param)> commit_signal_t;
    typedef boost::signals2::signal<void (LLUICtrl* ctrl, const LLSD& param, bool tentative)> value_signal_t;
	// *TODO: add xml support for this type of signal in the future
	typedef boost::signals2::signal<void (LLUICtrl* ctrl, S32 x, S32 y, MASK mask)> mouse_signal_t;
	
	typedef boost::function<bool (LLUICtrl* ctrl, const LLSD& param)> enable_callback_t;
	typedef boost::signals2::signal<bool (LLUICtrl* ctrl, const LLSD& param), boost_boolean_combiner> enable_signal_t;
	
	struct CallbackParam : public LLInitParam::Block<CallbackParam>
	{
		Ignored					name;

		Optional<std::string>	function_name;
		Optional<LLSD>			parameter;

		Optional<std::string>	control_name;
		
		CallbackParam();
	};

	struct CommitCallbackParam : public LLInitParam::Block<CommitCallbackParam, CallbackParam >
	{
		Optional<commit_callback_t> function;
	};

	// also used for visible callbacks
	struct EnableCallbackParam : public LLInitParam::Block<EnableCallbackParam, CallbackParam >
	{
		Optional<enable_callback_t> function;
	};
		
	struct EnableControls : public LLInitParam::ChoiceBlock<EnableControls>
	{
		Alternative<std::string> enabled;
		Alternative<std::string> disabled;
		
		EnableControls();
	};	
	struct ControlVisibility : public LLInitParam::ChoiceBlock<ControlVisibility>
	{
		Alternative<std::string> visible;
		Alternative<std::string> invisible;

		ControlVisibility();
	};	
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<std::string>			label;
		Optional<bool>					tab_stop,
										chrome,
										requests_front;
		Optional<LLSD>					initial_value;

		Optional<CommitCallbackParam>	init_callback,
										commit_callback;
		Optional<EnableCallbackParam>	validate_callback;
		
		Optional<CommitCallbackParam>	mouseenter_callback,
										mouseleave_callback;
		
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

	enum ETypeTransparency
	{
		TT_DEFAULT,
		TT_ACTIVE,		// focused floater
		TT_INACTIVE,	// other floaters
		TT_FADING,		// fading toast
	};
	/*virtual*/ ~LLUICtrl();

	void initFromParams(const Params& p);
protected:
	friend class LLUICtrlFactory;
	static const Params& getDefaultParams();
	LLUICtrl(const Params& p = getDefaultParams(),
             const LLViewModelPtr& viewmodel=LLViewModelPtr(new LLViewModel));
	
	commit_signal_t::slot_type initCommitCallback(const CommitCallbackParam& cb);
	enable_signal_t::slot_type initEnableCallback(const EnableCallbackParam& cb);

	// We need this virtual so we can override it with derived versions
	virtual LLViewModel* getViewModel() const;
    // We shouldn't ever need to set this directly
    //virtual void    setViewModel(const LLViewModelPtr&);

	virtual BOOL	postBuild();
	
public:
	// LLView interface
	/*virtual*/ BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );
	/*virtual*/ BOOL	isCtrl() const;
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

	void setFunctionName(const std::string& function_name);
	
	virtual void	setTentative(BOOL b);
	virtual BOOL	getTentative() const;
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
    virtual void    onValue();

	// Call appropriate callback
	virtual void	onCommit();
	
	// Default to no-op:
	virtual void	onTabInto();

	// Clear any user-provided input (text in a text editor, checked checkbox,
	// selected radio button, etc.).  Defaults to no-op.
	virtual void	clear();

	virtual void	setColor(const LLColor4& color);

	// Ansariel: Changed to virtual. We might want to change the transparency ourself!
	virtual F32	getCurrentTransparency();

	void				setTransparencyType(ETypeTransparency type);
	ETypeTransparency	getTransparencyType() const {return mTransparencyType;}

	BOOL	focusNextItem(BOOL text_entry_only);
	BOOL	focusPrevItem(BOOL text_entry_only);
	BOOL 	focusFirstItem(BOOL prefer_text_fields = FALSE, BOOL focus_flash = TRUE );

	// Non Virtuals
	LLHandle<LLUICtrl> getHandle() const { return getDerivedHandle<LLUICtrl>(); }
	BOOL			getIsChrome() const;
	
	void			setTabStop( BOOL b );
	BOOL			hasTabStop() const;

	LLUICtrl*		getParentUICtrl() const;

	// return true if help topic found by crawling through parents -
	// topic then put in help_topic_out
	bool                    findHelpTopic(std::string& help_topic_out);

	boost::signals2::connection setCommitCallback(const CommitCallbackParam& cb);
	boost::signals2::connection setValidateCallback(const EnableCallbackParam& cb);

	boost::signals2::connection setCommitCallback( const commit_signal_t::slot_type& cb );
	boost::signals2::connection setValidateCallback( const enable_signal_t::slot_type& cb );
    boost::signals2::connection setValueCallback( const value_signal_t::slot_type& cb );

	boost::signals2::connection setMouseEnterCallback( const commit_signal_t::slot_type& cb );
	boost::signals2::connection setMouseLeaveCallback( const commit_signal_t::slot_type& cb );
	
	boost::signals2::connection setMouseDownCallback( const mouse_signal_t::slot_type& cb );
	boost::signals2::connection setMouseUpCallback( const mouse_signal_t::slot_type& cb );
	boost::signals2::connection setRightMouseDownCallback( const mouse_signal_t::slot_type& cb );
	boost::signals2::connection setRightMouseUpCallback( const mouse_signal_t::slot_type& cb );
	
	boost::signals2::connection setDoubleClickCallback( const mouse_signal_t::slot_type& cb );

	// *TODO: Deprecate; for backwards compatability only:
	boost::signals2::connection setCommitCallback( boost::function<void (LLUICtrl*,void*)> cb, void* data);	
	boost::signals2::connection setValidateBeforeCommit( boost::function<bool (const LLSD& data)> cb );

    // Propagates value changes and commits from this control to the receiver control
    void propagateCommits(LLUICtrl* receiver);

	LLUICtrl* findRootMostFocusRoot();

	class LLTextInputFilter : public LLQueryFilter, public LLSingleton<LLTextInputFilter>
	{
		LLSINGLETON_EMPTY_CTOR(LLTextInputFilter);
		/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const 
		{
			return filterResult_t(view->isCtrl() && static_cast<const LLUICtrl *>(view)->acceptsTextInput(), TRUE);
		}
	};

	template <typename F, typename DERIVED> class CallbackRegistry : public LLRegistrySingleton<std::string, F, DERIVED >
	{};	

	class CommitCallbackRegistry : public CallbackRegistry<commit_callback_t, CommitCallbackRegistry>
	{
		LLSINGLETON_EMPTY_CTOR(CommitCallbackRegistry);
	};
	// the enable callback registry is also used for visiblity callbacks
	class EnableCallbackRegistry : public CallbackRegistry<enable_callback_t, EnableCallbackRegistry>
	{
		LLSINGLETON_EMPTY_CTOR(EnableCallbackRegistry);
	};
		
protected:

	static bool controlListener(const LLSD& newvalue, LLHandle<LLUICtrl> handle, std::string type);

	commit_signal_t*		mCommitSignal;
	enable_signal_t*		mValidateSignal;
    value_signal_t*         mValueSignal;

	commit_signal_t*		mMouseEnterSignal;
	commit_signal_t*		mMouseLeaveSignal;
	
	mouse_signal_t*		mMouseDownSignal;
	mouse_signal_t*		mMouseUpSignal;
	mouse_signal_t*		mRightMouseDownSignal;
	mouse_signal_t*		mRightMouseUpSignal;

	mouse_signal_t*		mDoubleClickSignal;
	
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

	std::string mFunctionName;

	static F32 sActiveControlTransparency;
	static F32 sInactiveControlTransparency;
	
	virtual void addInfo(LLSD & info);
	
private:

	BOOL			mIsChrome;
	BOOL			mRequestsFront;
	BOOL			mTabStop;
	BOOL			mTentative;

	ETypeTransparency mTransparencyType;
};

// Build time optimization, generate once in .cpp file
#ifndef LLUICTRL_CPP
extern template class LLUICtrl* LLView::getChild<class LLUICtrl>(
	const std::string& name, BOOL recurse) const;
#endif

#endif  // LL_LLUICTRL_H
