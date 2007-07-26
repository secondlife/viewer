/** 
 * @file lluictrl.h
 * @author James Cook, Richard Nelson, Tom Yedwab
 * @brief Abstract base class for UI controls
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLUICTRL_H
#define LL_LLUICTRL_H

#include "llview.h"
#include "llrect.h"
#include "llsd.h"

//
// Classes
//
class LLFontGL;
class LLButton;
class LLTextBox;
class LLLineEditor;
class LLUICtrl;
class LLPanel;
class LLCtrlSelectionInterface;
class LLCtrlListInterface;
class LLCtrlScrollInterface;

typedef void (*LLUICtrlCallback)(LLUICtrl* ctrl, void* userdata);
typedef BOOL (*LLUICtrlValidate)(LLUICtrl* ctrl, void* userdata);

class LLUICtrl
: public LLView
{
public:
	LLUICtrl();
	LLUICtrl( const LLString& name, const LLRect& rect, BOOL mouse_opaque,
		LLUICtrlCallback callback,
		void* callback_userdata,
		U32 reshape=FOLLOWS_NONE);
	virtual ~LLUICtrl();

	// LLView interface
	//virtual BOOL	handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect);
	virtual void	initFromXML(LLXMLNodePtr node, LLView* parent);
	virtual LLXMLNodePtr getXML(bool save_children = true) const;

	virtual LLSD	getValue() const { return LLSD(); }

	// Defaults to no-op
	virtual BOOL	setTextArg( const LLString& key, const LLString& text );

	// Defaults to no-op
	virtual BOOL	setLabelArg( const LLString& key, const LLString& text );

	// Defaults to return NULL
	virtual LLCtrlSelectionInterface* getSelectionInterface();
	virtual LLCtrlListInterface* getListInterface();
	virtual LLCtrlScrollInterface* getScrollInterface();

	virtual void	setFocus( BOOL b );
	virtual BOOL	hasFocus() const;

	virtual void	setTabStop( BOOL b );
	virtual BOOL	hasTabStop() const;

	// Defaults to false
	virtual BOOL	acceptsTextInput() const;

	// Default to no-op
	virtual void	onTabInto();
	virtual void	clear();

	virtual void	setIsChrome(BOOL is_chrome);
	virtual BOOL	getIsChrome() const;

	virtual void	onCommit();

	virtual BOOL	isCtrl() const	{ return TRUE; }
	// "Tentative" controls have a proposed value, but haven't committed
	// it yet.  This is used when multiple objects are selected and we
	// want to display a parameter that differs between the objects.
	virtual void	setTentative(BOOL b);
	virtual BOOL	getTentative() const;

	// Returns containing panel/floater or NULL if none found.
	LLPanel*		getParentPanel() const;

	void*			getCallbackUserData() const								{ return mCallbackUserData; }
	void			setCallbackUserData( void* data )						{ mCallbackUserData = data; }
	
	void			setCommitCallback( void (*cb)(LLUICtrl*, void*) )		{ mCommitCallback = cb; }
	void			setValidateBeforeCommit( BOOL(*cb)(LLUICtrl*, void*) )	{ mValidateCallback = cb; }

	// Defaults to no-op!
	virtual	void	setDoubleClickCallback( void (*cb)(void*) );

	// Defaults to no-op
	virtual void	setColor(const LLColor4& color);

	// Defaults to no-op
	virtual void	setMinValue(LLSD min_value);
	virtual void	setMaxValue(LLSD max_value);

	// In general, only LLPanel uses these.
	void			setFocusLostCallback(void (*cb)(LLUICtrl* caller, void* user_data)) { mFocusLostCallback = cb; }
	void			setFocusReceivedCallback( void (*cb)(LLUICtrl*, void*) )	{ mFocusReceivedCallback = cb; }
	void			setFocusChangedCallback( void (*cb)(LLUICtrl*, void*) )		{ mFocusChangedCallback = cb; }

	static void		onFocusLostCallback(LLUICtrl* old_focus);

	/*virtual*/ BOOL focusFirstItem(BOOL prefer_text_fields = FALSE );

	class LLTabStopPostFilter : public LLQueryFilter, public LLSingleton<LLTabStopPostFilter>
	{
		/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const 
		{
			return filterResult_t(view->isCtrl() && static_cast<const LLUICtrl *>(view)->hasTabStop() && children.size() == 0, TRUE);
		}
	};

	class LLTextInputFilter : public LLQueryFilter, public LLSingleton<LLTextInputFilter>
	{
		/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const 
		{
			return filterResult_t(view->isCtrl() && static_cast<const LLUICtrl *>(view)->acceptsTextInput(), TRUE);
		}
	};

	// Returns TRUE if the user has modified this control.   Editable controls should override this.
	virtual BOOL	 isDirty()			{ return FALSE;		};

protected:
	virtual void	onFocusReceived();
	virtual void	onFocusLost();
	void			onChangeFocus( S32 direction );

protected:

	void			(*mCommitCallback)( LLUICtrl* ctrl, void* userdata );
	void			(*mFocusLostCallback)( LLUICtrl* caller, void* userdata );
	void			(*mFocusReceivedCallback)( LLUICtrl* ctrl, void* userdata );
	void			(*mFocusChangedCallback)( LLUICtrl* ctrl, void* userdata );
	BOOL			(*mValidateCallback)( LLUICtrl* ctrl, void* userdata );

	void*			mCallbackUserData;
	BOOL			mTentative;
	BOOL			mTabStop;

private:
	BOOL			mIsChrome;


};

#endif  // LL_LLUICTRL_H
