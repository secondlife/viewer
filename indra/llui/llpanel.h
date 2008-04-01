/** 
 * @file llpanel.h
 * @author James Cook, Tom Yedwab
 * @brief LLPanel base class
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

#ifndef LL_LLPANEL_H
#define LL_LLPANEL_H


#include "llcallbackmap.h"
#include "lluictrl.h"
#include "llbutton.h"
#include "lllineeditor.h"
#include "llviewborder.h"
#include "lluistring.h"
#include "v4color.h"
#include <list>
#include <queue>

const S32 LLPANEL_BORDER_WIDTH = 1;
const BOOL BORDER_YES = TRUE;
const BOOL BORDER_NO = FALSE;


struct LLAlertInfo
{
	LLString mLabel;
	LLString::format_map_t mArgs;

	LLAlertInfo(LLString label, LLString::format_map_t args) : mLabel(label), mArgs(args) { }
	LLAlertInfo(){}
};


/*
 * General purpose concrete view base class.
 * Transparent or opaque,
 * With or without border,
 * Can contain LLUICtrls.
 */
class LLPanel : public LLUICtrl 
{
public:

	// minimal constructor for data-driven initialization
	LLPanel();
	LLPanel(const LLString& name);

	// Position and size not saved
	LLPanel(const LLString& name, const LLRect& rect, BOOL bordered = TRUE);

	// Position and size are saved to rect_control
	LLPanel(const LLString& name, const LLString& rect_control, BOOL bordered = TRUE);	
	
	/*virtual*/ ~LLPanel();

	// LLView interface
	/*virtual*/ BOOL 	isPanel() const;
	/*virtual*/ void	draw();	
	/*virtual*/ BOOL	handleKeyHere( KEY key, MASK mask );
	/*virtual*/ LLXMLNodePtr getXML(bool save_children = true) const;
	// Override to set not found list:
	virtual LLView* getChildView(const LLString& name, BOOL recurse = TRUE, BOOL create_if_missing = TRUE) const;

	// From LLFocusableElement
	/*virtual*/ void	setFocus( BOOL b );
	
	// New virtuals
	virtual 	void	refresh();	// called in setFocus()
	virtual 	BOOL	postBuild();
	virtual 	void	clearCtrls(); // overridden in LLPanelObject and LLPanelVolume

	// Border controls
	void addBorder( LLViewBorder::EBevel border_bevel = LLViewBorder::BEVEL_OUT,
					LLViewBorder::EStyle border_style = LLViewBorder::STYLE_LINE,
					S32 border_thickness = LLPANEL_BORDER_WIDTH );
	void			removeBorder();
	BOOL			hasBorder() const { return mBorder != NULL; }
	void			setBorderVisible( BOOL b );

	template <class T> void requires(LLString name)
	{
		// check for widget with matching type and name
		if (LLView::getChild<T>(name) == NULL)
		{
			mRequirementsError += name + "\n";
		}
	}
	
	// requires LLView by default
	void requires(LLString name)
	{
		requires<LLView>(name);
	}
	BOOL			checkRequirements();

	void			setBackgroundColor( const LLColor4& color ) { mBgColorOpaque = color; }
	const LLColor4&	getBackgroundColor() const { return mBgColorOpaque; }
	void			setTransparentColor(const LLColor4& color) { mBgColorAlpha = color; }
	const LLColor4& getTransparentColor() const { return mBgColorAlpha; }
	void			setBackgroundVisible( BOOL b )	{ mBgVisible = b; }
	BOOL			isBackgroundVisible() const { return mBgVisible; }
	void			setBackgroundOpaque(BOOL b)		{ mBgOpaque = b; }
	BOOL			isBackgroundOpaque() const { return mBgOpaque; }
	void			setDefaultBtn(LLButton* btn = NULL);
	void			setDefaultBtn(const LLString& id);
	void			updateDefaultBtn();
	void			setLabel(const LLStringExplicit& label) { mLabel = label; }
	LLString		getLabel() const { return mLabel; }
	
	void            setRectControl(const LLString& rect_control) { mRectControl.assign(rect_control); }
	const LLString&	getRectControl() const { return mRectControl; }
	void			storeRectControl();

	void			setCtrlsEnabled(BOOL b);

	LLHandle<LLPanel>	getHandle() const { return mPanelHandle; }

	S32				getLastTabGroup() const { return mLastTabGroup; }

	const LLCallbackMap::map_t& getFactoryMap() const { return mFactoryMap; }

	BOOL initPanelXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	void initChildrenXML(LLXMLNodePtr node, LLUICtrlFactory* factory);
	void setPanelParameters(LLXMLNodePtr node, LLView *parentp);

	LLString getString(const LLString& name, const LLString::format_map_t& args = LLUIString::sNullArgs) const;
	LLUIString getUIString(const LLString& name) const;

	// ** Wrappers for setting child properties by name ** -TomY

	// LLView
	void childSetVisible(const LLString& name, bool visible);
	void childShow(const LLString& name) { childSetVisible(name, true); }
	void childHide(const LLString& name) { childSetVisible(name, false); }
	bool childIsVisible(const LLString& id) const;
	void childSetTentative(const LLString& name, bool tentative);

	void childSetEnabled(const LLString& name, bool enabled);
	void childEnable(const LLString& name)	{ childSetEnabled(name, true); }
	void childDisable(const LLString& name) { childSetEnabled(name, false); };
	bool childIsEnabled(const LLString& id) const;

	void childSetToolTip(const LLString& id, const LLString& msg);
	void childSetRect(const LLString& id, const LLRect &rect);
	bool childGetRect(const LLString& id, LLRect& rect) const;

	// LLUICtrl
	void childSetFocus(const LLString& id, BOOL focus = TRUE);
	BOOL childHasFocus(const LLString& id);
	void childSetFocusChangedCallback(const LLString& id, void (*cb)(LLFocusableElement*, void*), void* user_data = NULL);
	
	void childSetCommitCallback(const LLString& id, void (*cb)(LLUICtrl*, void*), void* userdata = NULL );
	void childSetDoubleClickCallback(const LLString& id, void (*cb)(void*), void* userdata = NULL );
	void childSetValidate(const LLString& id, BOOL (*cb)(LLUICtrl*, void*) );
	void childSetUserData(const LLString& id, void* userdata);

	void childSetColor(const LLString& id, const LLColor4& color);

	LLCtrlSelectionInterface* childGetSelectionInterface(const LLString& id) const;
	LLCtrlListInterface* childGetListInterface(const LLString& id) const;
	LLCtrlScrollInterface* childGetScrollInterface(const LLString& id) const;

	// This is the magic bullet for data-driven UI
	void childSetValue(const LLString& id, LLSD value);
	LLSD childGetValue(const LLString& id) const;

	// For setting text / label replacement params, e.g. "Hello [NAME]"
	// Not implemented for all types, defaults to noop, returns FALSE if not applicaple
	BOOL childSetTextArg(const LLString& id, const LLString& key, const LLStringExplicit& text);
	BOOL childSetLabelArg(const LLString& id, const LLString& key, const LLStringExplicit& text);
	BOOL childSetToolTipArg(const LLString& id, const LLString& key, const LLStringExplicit& text);
	
	// LLSlider / LLMultiSlider / LLSpinCtrl
	void childSetMinValue(const LLString& id, LLSD min_value);
	void childSetMaxValue(const LLString& id, LLSD max_value);

	// LLTabContainer
	void childShowTab(const LLString& id, const LLString& tabname, bool visible = true);
	LLPanel *childGetVisibleTab(const LLString& id) const;
	void childSetTabChangeCallback(const LLString& id, const LLString& tabname, void (*on_tab_clicked)(void*, bool), void *userdata);

	// LLTextBox
	void childSetWrappedText(const LLString& id, const LLString& text, bool visible = true);

	// LLTextBox/LLTextEditor/LLLineEditor
	void childSetText(const LLString& id, const LLStringExplicit& text) { childSetValue(id, LLSD(text)); }
	LLString childGetText(const LLString& id) const { return childGetValue(id).asString(); }

	// LLLineEditor
	void childSetKeystrokeCallback(const LLString& id, void (*keystroke_callback)(LLLineEditor* caller, void* user_data), void *user_data);
	void childSetPrevalidate(const LLString& id, BOOL (*func)(const LLWString &) );

	// LLButton
	void childSetAction(const LLString& id, void(*function)(void*), void* value);
	void childSetActionTextbox(const LLString& id, void(*function)(void*));
	void childSetControlName(const LLString& id, const LLString& control_name);

	// Error reporting
	void childNotFound(const LLString& id) const;
	void childDisplayNotFound();

	static void		alertXml(LLString label, LLString::format_map_t args = LLString::format_map_t());
	static BOOL		nextAlert(LLAlertInfo &alert);
	static LLView*	fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	
protected:
	// Override to set not found list
	LLButton*		getDefaultButton() { return mDefaultBtn; }
	LLCallbackMap::map_t mFactoryMap;

private:
	// common construction logic
	void init();

	// From LLView
	virtual void	addCtrl( LLUICtrl* ctrl, S32 tab_group );
	virtual void	addCtrlAtEnd( LLUICtrl* ctrl, S32 tab_group);

	// Unified error reporting for the child* functions
	typedef std::set<LLString> expected_members_list_t;
	mutable expected_members_list_t mExpectedMembers;
	mutable expected_members_list_t mNewExpectedMembers;

	LLString		mRectControl;
	LLColor4		mBgColorAlpha;
	LLColor4		mBgColorOpaque;
	LLColor4		mDefaultBtnHighlight;
	BOOL			mBgVisible;
	BOOL			mBgOpaque;
	LLViewBorder*	mBorder;
	LLButton*		mDefaultBtn;
	LLString		mLabel;
	S32				mLastTabGroup;
	LLRootHandle<LLPanel> mPanelHandle;

	typedef std::map<LLString, LLUIString> ui_string_map_t;
	ui_string_map_t	mUIStrings;

	LLString		mRequirementsError;

	typedef std::queue<LLAlertInfo> alert_queue_t;
	static alert_queue_t sAlertQueue;
}; // end class LLPanel


class LLLayoutStack : public LLView
{
public:
	typedef enum e_layout_orientation
	{
		HORIZONTAL,
		VERTICAL
	} eLayoutOrientation;

	LLLayoutStack(eLayoutOrientation orientation);
	virtual ~LLLayoutStack();

	/*virtual*/ void draw();
	/*virtual*/ LLXMLNodePtr getXML(bool save_children = true) const;
	/*virtual*/ void removeCtrl(LLUICtrl* ctrl);

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	S32 getMinWidth() const { return mMinWidth; }
	S32 getMinHeight() const { return mMinHeight; }
	
	void addPanel(LLPanel* panel, S32 min_width, S32 min_height, BOOL auto_resize, BOOL user_resize, S32 index = S32_MAX);
	void removePanel(LLPanel* panel);

private:
	struct LLEmbeddedPanel;

	void updateLayout(BOOL force_resize = FALSE);
	void calcMinExtents();
	S32 getDefaultHeight(S32 cur_height);
	S32 getDefaultWidth(S32 cur_width);

	const eLayoutOrientation mOrientation;

	typedef std::vector<LLEmbeddedPanel*> e_panel_list_t;
	e_panel_list_t mPanels;
	LLEmbeddedPanel* findEmbeddedPanel(LLPanel* panelp) const;

	S32 mMinWidth;
	S32 mMinHeight;
	S32 mPanelSpacing;
}; // end class LLLayoutStack

#endif
