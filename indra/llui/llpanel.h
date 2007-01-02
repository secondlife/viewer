/** 
 * @file llpanel.h
 * @author James Cook, Tom Yedwab
 * @brief LLPanel base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPANEL_H
#define LL_LLPANEL_H

// Opaque view with a background and a border.  Can contain LLUICtrls.

#include "llcallbackmap.h"
#include "lluictrl.h"
#include "llviewborder.h"
#include "v4color.h"
#include <list>

const S32 LLPANEL_BORDER_WIDTH = 1;
const BOOL BORDER_YES = TRUE;
const BOOL BORDER_NO = FALSE;

class LLViewerImage;
class LLUUID;
class LLCheckBoxCtrl;
class LLComboBox;
class LLIconCtrl;
class LLLineEditor;
class LLRadioGroup;
class LLScrollListCtrl;
class LLSliderCtrl;
class LLSpinCtrl;
class LLTextBox;
class LLTextEditor;

class LLAlertInfo
{
public:
	LLString mLabel;
	LLString::format_map_t mArgs;

	LLAlertInfo(LLString label, LLString::format_map_t args)
		: mLabel(label), mArgs(args) { }

		LLAlertInfo() { }
};

class LLPanel : public LLUICtrl
{
public:
	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	// defaults to TRUE
	virtual BOOL isPanel();

	// minimal constructor for data-driven initialization
	LLPanel();
	LLPanel(const LLString& name);

	// Position and size not saved
	LLPanel(const LLString& name, const LLRect& rect, BOOL bordered = TRUE);

	// Position and size are saved to rect_control
	LLPanel(const LLString& name, const LLString& rect_control, BOOL bordered = TRUE);

	void addBorder( LLViewBorder::EBevel border_bevel = LLViewBorder::BEVEL_OUT,
					LLViewBorder::EStyle border_style = LLViewBorder::STYLE_LINE,
					S32 border_thickness = LLPANEL_BORDER_WIDTH );

	virtual ~LLPanel();
	virtual void	draw();	
	virtual void 	refresh();	// called in setFocus()
	virtual void	setFocus( BOOL b );
	void			setFocusRoot(BOOL b) { mIsFocusRoot = b; }
	virtual BOOL	handleKeyHere( KEY key, MASK mask, BOOL called_from_parent );
	virtual BOOL	handleKey( KEY key, MASK mask, BOOL called_from_parent );
	virtual BOOL	postBuild();

	void			requires(LLString name, EWidgetType type = WIDGET_TYPE_DONTCARE);
	BOOL			checkRequirements();

	static void		alertXml(LLString label, LLString::format_map_t args = LLString::format_map_t());
	static BOOL		nextAlert(LLAlertInfo &alert);

	void			setBackgroundColor( const LLColor4& color );
	void			setTransparentColor(const LLColor4& color);
	void			setBackgroundVisible( BOOL b )	{ mBgVisible = b; }
	void			setBackgroundOpaque(BOOL b)		{ mBgOpaque = b; }
	void			setDefaultBtn(LLButton* btn = NULL);
	void			setDefaultBtn(const LLString& id);
	void			setLabel(LLString label) { mLabel = label; }
	LLString		getLabel() const { return mLabel; }
	
	void            setRectControl(const LLString& rect_control) { mRectControl.assign(rect_control); }
	
	void			setBorderVisible( BOOL b );

	void			setCtrlsEnabled(BOOL b);
	virtual void	clearCtrls();

	LLViewHandle	getHandle() { return mViewHandle; }

	S32				getLastTabGroup() { return mLastTabGroup; }

	LLView*			getCtrlByNameAndType(const LLString& name, EWidgetType type);

	static LLPanel*	getPanelByHandle(LLViewHandle handle);

	virtual const LLCallbackMap::map_t& getFactoryMap() const { return mFactoryMap; }

	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	void initPanelXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	void setPanelParameters(LLXMLNodePtr node, LLView *parentp);

	// ** Wrappers for setting child properties by name ** -TomY

	// Override to set not found list
	virtual LLView* getChildByName(const LLString& name, BOOL recurse = FALSE) const;
	
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
	void childSetFocusChangedCallback(const LLString& id, void (*cb)(LLUICtrl*, void*));
	
	void childSetCommitCallback(const LLString& id, void (*cb)(LLUICtrl*, void*), void* userdata = NULL );
	void childSetDoubleClickCallback(const LLString& id, void (*cb)(void*), void* userdata = NULL );
	void childSetValidate(const LLString& id, BOOL (*cb)(LLUICtrl*, void*) );
	void childSetUserData(const LLString& id, void* userdata);

	void childSetColor(const LLString& id, const LLColor4& color);

	LLCtrlSelectionInterface* childGetSelectionInterface(const LLString& id);
	LLCtrlListInterface* childGetListInterface(const LLString& id);
	LLCtrlScrollInterface* childGetScrollInterface(const LLString& id);

	// This is the magic bullet for data-driven UI
	void childSetValue(const LLString& id, LLSD value);
	LLSD childGetValue(const LLString& id) const;

	// For setting text / label replacement params, e.g. "Hello [NAME]"
	// Not implemented for all types, defaults to noop, returns FALSE if not applicaple
	BOOL childSetTextArg(const LLString& id, const LLString& key, const LLString& text);
	BOOL childSetLabelArg(const LLString& id, const LLString& key, const LLString& text);
	
	// LLSlider / LLSpinCtrl
	void childSetMinValue(const LLString& id, LLSD min_value);
	void childSetMaxValue(const LLString& id, LLSD max_value);

	// LLTabContainer
	void childShowTab(const LLString& id, const LLString& tabname, bool visible = true);
	LLPanel *childGetVisibleTab(const LLString& id);
	void childSetTabChangeCallback(const LLString& id, const LLString& tabname, void (*on_tab_clicked)(void*, bool), void *userdata);

	// LLTextBox
	void childSetWrappedText(const LLString& id, const LLString& text, bool visible = true);

	// LLTextBox/LLTextEditor/LLLineEditor
	void childSetText(const LLString& id, const LLString& text);
	LLString childGetText(const LLString& id);

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

	typedef std::queue<LLAlertInfo> alert_queue_t;
	static alert_queue_t sAlertQueue;

private:
	// common constructor
	void init();
	
protected:
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
	LLCallbackMap::map_t mFactoryMap;
	LLString		mLabel;
	S32				mLastTabGroup;

	typedef std::map<LLString, EWidgetType> requirements_map_t;
	requirements_map_t mRequirements;

	typedef std::map<LLViewHandle, LLPanel*> panel_map_t;
	static panel_map_t sPanelMap;
};

#endif
