/** 
 * @file llpanel.h
 * @author James Cook, Tom Yedwab
 * @brief LLPanel base class
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

#ifndef LL_LLPANEL_H
#define LL_LLPANEL_H


#include "llcallbackmap.h"
#include "lluictrl.h"
#include "llviewborder.h"
#include "lluistring.h"
#include "v4color.h"
#include <list>
#include <queue>

const S32 LLPANEL_BORDER_WIDTH = 1;
const BOOL BORDER_YES = TRUE;
const BOOL BORDER_NO = FALSE;

class LLButton;

/*
 * General purpose concrete view base class.
 * Transparent or opaque,
 * With or without border,
 * Can contain LLUICtrls.
 */
class LLPanel : public LLUICtrl
{
public:
	struct LocalizedString : public LLInitParam::Block<LocalizedString>
	{
		Mandatory<std::string>	name;
		Mandatory<std::string>	value;
		
		LocalizedString();
	};

	struct Params 
	:	public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<bool>			has_border;
		Optional<LLViewBorder::Params>	border;

		Optional<LLUIColor>		bg_opaque_color,
								bg_alpha_color;

		Optional<bool>			background_visible,
								background_opaque;

		Optional<S32>			min_width,
								min_height;

		Optional<std::string>	filename;
		Optional<std::string>	class_name;
		Optional<std::string>   help_topic;

		Multiple<LocalizedString>	strings;
		
		Optional<CommitCallbackParam> visible_callback;
		
		Params();
	};

	// valid children for LLPanel are stored in this registry
	typedef LLDefaultChildRegistry child_registry_t;

protected:
	friend class LLUICtrlFactory;
	// RN: for some reason you can't just use LLUICtrlFactory::getDefaultParams as a default argument in VC8
	static const LLPanel::Params& getDefaultParams();

	// Panels can get constructed directly
	LLPanel(const LLPanel::Params& params = getDefaultParams());
	
public:
// 	LLPanel(const std::string& name, const LLRect& rect = LLRect(), BOOL bordered = TRUE);
	/*virtual*/ ~LLPanel() {}

	// LLView interface
	/*virtual*/ BOOL 	isPanel() const;
	/*virtual*/ void	draw();	
	/*virtual*/ BOOL	handleKeyHere( KEY key, MASK mask );
	/*virtual*/ void 	handleVisibilityChange ( BOOL new_visibility );

	// From LLFocusableElement
	/*virtual*/ void	setFocus( BOOL b );
	
	// New virtuals
	virtual 	void	refresh();	// called in setFocus()
	virtual 	void	clearCtrls(); // overridden in LLPanelObject and LLPanelVolume

	// Border controls
	void addBorder( LLViewBorder::Params p);
	void addBorder();
	void			removeBorder();
	BOOL			hasBorder() const { return mBorder != NULL; }
	void			setBorderVisible( BOOL b );

	void			setBackgroundColor( const LLColor4& color ) { mBgColorOpaque = color; }
	const LLColor4&	getBackgroundColor() const { return mBgColorOpaque; }
	void			setTransparentColor(const LLColor4& color) { mBgColorAlpha = color; }
	const LLColor4& getTransparentColor() const { return mBgColorAlpha; }
	void			setBackgroundVisible( BOOL b )	{ mBgVisible = b; }
	BOOL			isBackgroundVisible() const { return mBgVisible; }
	void			setBackgroundOpaque(BOOL b)		{ mBgOpaque = b; }
	BOOL			isBackgroundOpaque() const { return mBgOpaque; }
	void			setDefaultBtn(LLButton* btn = NULL);
	void			setDefaultBtn(const std::string& id);
	void			updateDefaultBtn();
	void			setLabel(const LLStringExplicit& label) { mLabel = label; }
	std::string		getLabel() const { return mLabel; }
	void			setHelpTopic(const std::string& help_topic) { mHelpTopic = help_topic; }
	std::string		getHelpTopic() const { return mHelpTopic; }
	
	void			setCtrlsEnabled(BOOL b);

	LLHandle<LLPanel>	getHandle() const { return mPanelHandle; }

	const LLCallbackMap::map_t& getFactoryMap() const { return mFactoryMap; }
	
	CommitCallbackRegistry::ScopedRegistrar& getCommitCallbackRegistrar() { return mCommitCallbackRegistrar; }
	EnableCallbackRegistry::ScopedRegistrar& getEnableCallbackRegistrar() { return mEnableCallbackRegistrar; }
	
	void initFromParams(const Params& p);
	BOOL initPanelXML(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node = NULL);
	
	bool hasString(const std::string& name);
	std::string getString(const std::string& name, const LLStringUtil::format_map_t& args) const;
	std::string getString(const std::string& name) const;

	// ** Wrappers for setting child properties by name ** -TomY

	// LLView
	void childSetVisible(const std::string& name, bool visible);
	void childShow(const std::string& name) { childSetVisible(name, true); }
	void childHide(const std::string& name) { childSetVisible(name, false); }
	bool childIsVisible(const std::string& id) const;
	void childSetTentative(const std::string& name, bool tentative);

	void childSetEnabled(const std::string& name, bool enabled);
	void childEnable(const std::string& name)	{ childSetEnabled(name, true); }
	void childDisable(const std::string& name) { childSetEnabled(name, false); };
	bool childIsEnabled(const std::string& id) const;

	void childSetToolTip(const std::string& id, const std::string& msg);
	void childSetRect(const std::string& id, const LLRect &rect);
	bool childGetRect(const std::string& id, LLRect& rect) const;

	// LLUICtrl
	void childSetFocus(const std::string& id, BOOL focus = TRUE);
	BOOL childHasFocus(const std::string& id);
	
	// *TODO: Deprecate; for backwards compatability only:
	// Prefer getChild<LLUICtrl>("foo")->setCommitCallback(boost:bind(...)),
	// which takes a generic slot.  Or use mCommitCallbackRegistrar.add() with
	// a named callback and reference it in XML.
	void childSetCommitCallback(const std::string& id, boost::function<void (LLUICtrl*,void*)> cb, void* data);	
	
	void childSetValidate(const std::string& id, boost::function<bool (const LLSD& data)> cb );

	void childSetColor(const std::string& id, const LLColor4& color);

	LLCtrlSelectionInterface* childGetSelectionInterface(const std::string& id) const;
	LLCtrlListInterface* childGetListInterface(const std::string& id) const;
	LLCtrlScrollInterface* childGetScrollInterface(const std::string& id) const;

	// This is the magic bullet for data-driven UI
	void childSetValue(const std::string& id, LLSD value);
	LLSD childGetValue(const std::string& id) const;

	// For setting text / label replacement params, e.g. "Hello [NAME]"
	// Not implemented for all types, defaults to noop, returns FALSE if not applicaple
	BOOL childSetTextArg(const std::string& id, const std::string& key, const LLStringExplicit& text);
	BOOL childSetLabelArg(const std::string& id, const std::string& key, const LLStringExplicit& text);
	BOOL childSetToolTipArg(const std::string& id, const std::string& key, const LLStringExplicit& text);
	
	// LLTabContainer
	void childShowTab(const std::string& id, const std::string& tabname, bool visible = true);
	LLPanel *childGetVisibleTab(const std::string& id) const;
	LLPanel *childGetVisibleTabWithHelp();

	// LLTextBox/LLTextEditor/LLLineEditor
	void childSetText(const std::string& id, const LLStringExplicit& text) { childSetValue(id, LLSD(text)); }

	// *NOTE: Does not return text from <string> tags, use getString()
	std::string childGetText(const std::string& id) const { return childGetValue(id).asString(); }

	// LLLineEditor
	void childSetPrevalidate(const std::string& id, BOOL (*func)(const LLWString &) );

	// LLButton
	void childSetAction(const std::string& id, boost::function<void(void*)> function, void* value = NULL);

	// LLTextBox
	void childSetActionTextbox(const std::string& id, boost::function<void(void*)> function, void* value = NULL);

	void childSetControlName(const std::string& id, const std::string& control_name);

	static LLView*	fromXML(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node = NULL);

	//call onOpen to let panel know when it's about to be shown or activated
	virtual void	onOpen(const LLSD& key) {}

	void setXMLFilename(std::string filename) { mXMLFilename = filename; };
	std::string getXMLFilename() { return mXMLFilename; };
	
protected:
	// Override to set not found list
	LLButton*		getDefaultButton() { return mDefaultBtn; }
	LLCallbackMap::map_t mFactoryMap;
	CommitCallbackRegistry::ScopedRegistrar mCommitCallbackRegistrar;
	EnableCallbackRegistry::ScopedRegistrar mEnableCallbackRegistrar;
	VisibleCallbackRegistry::ScopedRegistrar mVisibleCallbackRegistrar;

	commit_signal_t mVisibleSignal;		// Called when visibility changes, passes new visibility as LLSD()

	std::string		mHelpTopic;         // the name of this panel's help topic to display in the Help Viewer
	
private:
	LLUIColor		mBgColorAlpha;
	LLUIColor		mBgColorOpaque;
	BOOL			mBgVisible;
	BOOL			mBgOpaque;
	LLViewBorder*	mBorder;
	LLButton*		mDefaultBtn;
	LLUIString		mLabel;
	LLRootHandle<LLPanel> mPanelHandle;

	typedef std::map<std::string, std::string> ui_string_map_t;
	ui_string_map_t	mUIStrings;

	// for setting the xml filename when building panel in context dependent cases
	std::string		mXMLFilename;

}; // end class LLPanel

#endif
