/** 
 * @file lluictrl.h
 * @author James Cook, Richard Nelson, Tom Yedwab
 * @brief Abstract base class for UI controls
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llview.h"
#include "llrect.h"
#include "llsd.h"


class LLFocusableElement
{
	friend class LLFocusMgr; // allow access to focus change handlers
public:
	LLFocusableElement();
	virtual ~LLFocusableElement();

	virtual void	setFocus( BOOL b );
	virtual BOOL	hasFocus() const;

	void			setFocusLostCallback(void (*cb)(LLFocusableElement* caller, void*), void* user_data = NULL) { mFocusLostCallback = cb; mFocusCallbackUserData = user_data; }
	void			setFocusReceivedCallback( void (*cb)(LLFocusableElement*, void*), void* user_data = NULL)	{ mFocusReceivedCallback = cb; mFocusCallbackUserData = user_data; }
	void			setFocusChangedCallback( void (*cb)(LLFocusableElement*, void*), void* user_data = NULL )		{ mFocusChangedCallback = cb; mFocusCallbackUserData = user_data; }

protected:
	virtual void	onFocusReceived();
	virtual void	onFocusLost();
	void			(*mFocusLostCallback)( LLFocusableElement* caller, void* userdata );
	void			(*mFocusReceivedCallback)( LLFocusableElement* ctrl, void* userdata );
	void			(*mFocusChangedCallback)( LLFocusableElement* ctrl, void* userdata );
	void*			mFocusCallbackUserData;
};

class LLUICtrl
: public LLView, public LLFocusableElement
{
public:
	typedef void (*LLUICtrlCallback)(LLUICtrl* ctrl, void* userdata);
	typedef BOOL (*LLUICtrlValidate)(LLUICtrl* ctrl, void* userdata);

	LLUICtrl();
	LLUICtrl( const LLString& name, const LLRect& rect, BOOL mouse_opaque,
		LLUICtrlCallback callback,
		void* callback_userdata,
		U32 reshape=FOLLOWS_NONE);
	/*virtual*/ ~LLUICtrl();

	// LLView interface
	/*virtual*/ void	initFromXML(LLXMLNodePtr node, LLView* parent);
	/*virtual*/ LLXMLNodePtr getXML(bool save_children = true) const;
	/*virtual*/ BOOL	setLabelArg( const LLString& key, const LLStringExplicit& text );
	/*virtual*/ void	onFocusReceived();
	/*virtual*/ void	onFocusLost();
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

	virtual LLSD	getValue() const;
	virtual BOOL	setTextArg(  const LLString& key, const LLStringExplicit& text );
	virtual void	setIsChrome(BOOL is_chrome);

	virtual BOOL	acceptsTextInput() const; // Defaults to false

	// A control is dirty if the user has modified its value.
	// Editable controls should override this.
	virtual BOOL	isDirty() const; // Defauls to false
	virtual void	resetDirty(); //Defaults to no-op
	
	// Call appropriate callbacks
	virtual void	onLostTop();	// called when registered as top ctrl and user clicks elsewhere
	virtual void	onCommit();
	
	// Default to no-op:
	virtual void	onTabInto();
	virtual void	clear();
	virtual	void	setDoubleClickCallback( void (*cb)(void*) );
	virtual void	setColor(const LLColor4& color);
	virtual void	setMinValue(LLSD min_value);
	virtual void	setMaxValue(LLSD max_value);

	BOOL	focusNextItem(BOOL text_entry_only);
	BOOL	focusPrevItem(BOOL text_entry_only);
	BOOL 	focusFirstItem(BOOL prefer_text_fields = FALSE, BOOL focus_flash = TRUE );
	BOOL	focusLastItem(BOOL prefer_text_fields = FALSE);

	// Non Virtuals
	BOOL			getIsChrome() const;
	
	void			setTabStop( BOOL b );
	BOOL			hasTabStop() const;

	// Returns containing panel/floater or NULL if none found.
	class LLPanel*	getParentPanel() const;
	class LLUICtrl*	getParentUICtrl() const;

	void*			getCallbackUserData() const								{ return mCallbackUserData; }
	void			setCallbackUserData( void* data )						{ mCallbackUserData = data; }
	
	void			setCommitCallback( void (*cb)(LLUICtrl*, void*) )		{ mCommitCallback = cb; }
	void			setValidateBeforeCommit( BOOL(*cb)(LLUICtrl*, void*) )	{ mValidateCallback = cb; }
	void			setLostTopCallback( void (*cb)(LLUICtrl*, void*) )		{ mLostTopCallback = cb; }
	
	const LLUICtrl* findRootMostFocusRoot() const;

	class LLTextInputFilter : public LLQueryFilter, public LLSingleton<LLTextInputFilter>
	{
		/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const 
		{
			return filterResult_t(view->isCtrl() && static_cast<const LLUICtrl *>(view)->acceptsTextInput(), TRUE);
		}
	};

protected:

	void			(*mCommitCallback)( LLUICtrl* ctrl, void* userdata );
	void			(*mLostTopCallback)( LLUICtrl* ctrl, void* userdata );
	BOOL			(*mValidateCallback)( LLUICtrl* ctrl, void* userdata );

	void*			mCallbackUserData;

private:

	BOOL			mTabStop;
	BOOL			mIsChrome;
	BOOL			mTentative;

	class DefaultTabGroupFirstSorter;
};

#endif  // LL_LLUICTRL_H
