/** 
 * @file llpanel.h
 * @author James Cook, Tom Yedwab
 * @brief LLPanel base class
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

#ifndef LL_LLPANEL_H
#define LL_LLPANEL_H


#include "llcallbackmap.h"
#include "lluictrl.h"
#include "llviewborder.h"
#include "lluiimage.h"
#include "lluistring.h"
#include "v4color.h"
#include "llbadgeholder.h"
#include <list>
#include <queue>

const S32 LLPANEL_BORDER_WIDTH = 1;
const bool BORDER_YES = true;
const bool BORDER_NO = false;

class LLButton;
class LLUIImage;

/*
 * General purpose concrete view base class.
 * Transparent or opaque,
 * With or without border,
 * Can contain LLUICtrls.
 */
class LLPanel : public LLUICtrl, public LLBadgeHolder
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

		Optional<bool>			background_visible,
								background_opaque;

		Optional<LLUIColor>		bg_opaque_color,
								bg_alpha_color,
								bg_opaque_image_overlay,
								bg_alpha_image_overlay;
		// opaque image is for "panel in foreground" look
		Optional<LLUIImage*>	bg_opaque_image,
								bg_alpha_image;

		Optional<S32>			min_width,
								min_height;

		Optional<std::string>	filename;
		Optional<std::string>	class_name;
		Optional<std::string>   help_topic;

		Multiple<LocalizedString>	strings;
		
		Optional<CommitCallbackParam> visible_callback;

		Optional<bool>			accepts_badge;
		
		Params();
	};

protected:
	friend class LLUICtrlFactory;
	// RN: for some reason you can't just use LLUICtrlFactory::getDefaultParams as a default argument in VC8
	static const LLPanel::Params& getDefaultParams();

	// Panels can get constructed directly
	LLPanel(const LLPanel::Params& params = getDefaultParams());
	
public:
	typedef std::vector<class LLUICtrl *>				ctrl_list_t;

	bool buildFromFile(const std::string &filename, const LLPanel::Params& default_params, bool cacheable = false);
	bool buildFromFile(const std::string &filename, bool cacheable = false) { return buildFromFile(filename, getDefaultParams(), cacheable); }

	static LLPanel* createFactoryPanel(const std::string& name);

	/*virtual*/ ~LLPanel();

	// LLView interface
	/*virtual*/ bool 	isPanel() const;
	/*virtual*/ void	draw();	
	/*virtual*/ bool	handleKeyHere( KEY key, MASK mask );
	/*virtual*/ void 	onVisibilityChange ( bool new_visibility );

	// From LLFocusableElement
	/*virtual*/ void	setFocus( bool b );
	
	// New virtuals
	virtual 	void	refresh();	// called in setFocus()
	virtual 	void	clearCtrls(); // overridden in LLPanelObject and LLPanelVolume

	// Border controls
	const LLViewBorder* getBorder() const { return mBorder; }
	void addBorder( LLViewBorder::Params p);
	void addBorder();
	void			removeBorder();
	bool			hasBorder() const { return mBorder != NULL; }
	void			setBorderVisible( bool b );

	void			setBackgroundColor( const LLColor4& color ) { mBgOpaqueColor = color; }
	const LLColor4&	getBackgroundColor() const { return mBgOpaqueColor; }
	void			setTransparentColor(const LLColor4& color) { mBgAlphaColor = color; }
	const LLColor4& getTransparentColor() const { return mBgAlphaColor; }
	void			setBackgroundImage(LLUIImage* image) { mBgOpaqueImage = image; }
	void			setTransparentImage(LLUIImage* image) { mBgAlphaImage = image; }
	LLPointer<LLUIImage> getBackgroundImage() const { return mBgOpaqueImage; }
	LLPointer<LLUIImage> getTransparentImage() const { return mBgAlphaImage; }
	LLColor4		getBackgroundImageOverlay() { return mBgOpaqueImageOverlay; }
	LLColor4		getTransparentImageOverlay() { return mBgAlphaImageOverlay; }
	void			setBackgroundVisible( bool b )	{ mBgVisible = b; }
	bool			isBackgroundVisible() const { return mBgVisible; }
	void			setBackgroundOpaque(bool b)		{ mBgOpaque = b; }
	bool			isBackgroundOpaque() const { return mBgOpaque; }
	void			setDefaultBtn(LLButton* btn = NULL);
	void			setDefaultBtn(const std::string& id);
	void			updateDefaultBtn();
	void			setLabel(const LLStringExplicit& label) { mLabel = label; }
	std::string		getLabel() const { return mLabel; }
	void			setHelpTopic(const std::string& help_topic) { mHelpTopic = help_topic; }
	std::string		getHelpTopic() const { return mHelpTopic; }
	
	void			setCtrlsEnabled(bool b);
	ctrl_list_t		getCtrlList() const;

	LLHandle<LLPanel>	getHandle() const { return getDerivedHandle<LLPanel>(); }

	const LLCallbackMap::map_t& getFactoryMap() const { return mFactoryMap; }
	
	CommitCallbackRegistry::ScopedRegistrar& getCommitCallbackRegistrar() { return mCommitCallbackRegistrar; }
	EnableCallbackRegistry::ScopedRegistrar& getEnableCallbackRegistrar() { return mEnableCallbackRegistrar; }
	
	void initFromParams(const Params& p);
	bool initPanelXML(	LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node, const LLPanel::Params& default_params);
	
	bool hasString(const std::string& name);
	std::string getString(const std::string& name, const LLStringUtil::format_map_t& args) const;
	std::string getString(const std::string& name) const;

	// ** Wrappers for setting child properties by name ** -TomY
	// WARNING: These are deprecated, please use getChild<T>("name")->doStuff() idiom instead

	// LLView
	void childSetVisible(const std::string& name, bool visible);

	void childSetEnabled(const std::string& name, bool enabled);
	void childEnable(const std::string& name)	{ childSetEnabled(name, true); }
	void childDisable(const std::string& name) { childSetEnabled(name, false); };

	// LLUICtrl
	void childSetFocus(const std::string& id, bool focus = true);
	bool childHasFocus(const std::string& id);
	
	// *TODO: Deprecate; for backwards compatability only:
	// Prefer getChild<LLUICtrl>("foo")->setCommitCallback(boost:bind(...)),
	// which takes a generic slot.  Or use mCommitCallbackRegistrar.add() with
	// a named callback and reference it in XML.
	void childSetCommitCallback(const std::string& id, boost::function<void (LLUICtrl*,void*)> cb, void* data);	
	void childSetColor(const std::string& id, const LLColor4& color);

	LLCtrlSelectionInterface* childGetSelectionInterface(const std::string& id) const;
	LLCtrlListInterface* childGetListInterface(const std::string& id) const;
	LLCtrlScrollInterface* childGetScrollInterface(const std::string& id) const;

	// This is the magic bullet for data-driven UI
	void childSetValue(const std::string& id, LLSD value);
	LLSD childGetValue(const std::string& id) const;

	// For setting text / label replacement params, e.g. "Hello [NAME]"
	// Not implemented for all types, defaults to noop, returns false if not applicaple
	bool childSetTextArg(const std::string& id, const std::string& key, const LLStringExplicit& text);
	bool childSetLabelArg(const std::string& id, const std::string& key, const LLStringExplicit& text);
	
	// LLButton
	void childSetAction(const std::string& id, boost::function<void(void*)> function, void* value);
	void childSetAction(const std::string& id, const commit_signal_t::slot_type& function);

	static LLView*	fromXML(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node = NULL);

	//call onOpen to let panel know when it's about to be shown or activated
	virtual void	onOpen(const LLSD& key) {}

	void setXMLFilename(std::string filename) { mXMLFilename = filename; };
	std::string getXMLFilename() { return mXMLFilename; };
	
	boost::signals2::connection setVisibleCallback( const commit_signal_t::slot_type& cb );

protected:
	// Override to set not found list
	LLButton*		getDefaultButton() { return mDefaultBtn; }
	LLCallbackMap::map_t mFactoryMap;
	CommitCallbackRegistry::ScopedRegistrar mCommitCallbackRegistrar;
	EnableCallbackRegistry::ScopedRegistrar mEnableCallbackRegistrar;

	commit_signal_t* mVisibleSignal;		// Called when visibility changes, passes new visibility as LLSD()

	std::string		mHelpTopic;         // the name of this panel's help topic to display in the Help Viewer
	typedef std::deque<const LLCallbackMap::map_t*> factory_stack_t;
	static factory_stack_t	sFactoryStack;

	// for setting the xml filename when building panel in context dependent cases
	std::string		mXMLFilename;
	
private:
	bool			mBgVisible;				// any background at all?
	bool			mBgOpaque;				// use opaque color or image
	LLUIColor		mBgOpaqueColor;
	LLUIColor		mBgAlphaColor;
	LLUIColor		mBgOpaqueImageOverlay;
	LLUIColor		mBgAlphaImageOverlay;
	LLPointer<LLUIImage> mBgOpaqueImage;	// "panel in front" look
	LLPointer<LLUIImage> mBgAlphaImage;		// "panel in back" look
	LLViewBorder*	mBorder;
	LLButton*		mDefaultBtn;
	LLUIString		mLabel;

	typedef std::map<std::string, std::string> ui_string_map_t;
	ui_string_map_t	mUIStrings;


}; // end class LLPanel

// Build time optimization, generate once in .cpp file
#ifndef LLPANEL_CPP
extern template class LLPanel* LLView::getChild<class LLPanel>(
	const std::string& name, bool recurse) const;
#endif

typedef boost::function<LLPanel* (void)> LLPanelClassCreatorFunc;

// local static instance for registering a particular panel class

class LLRegisterPanelClass
:	public LLSingleton< LLRegisterPanelClass >
{
	LLSINGLETON_EMPTY_CTOR(LLRegisterPanelClass);
public:
	// register with either the provided builder, or the generic templated builder
	void addPanelClass(const std::string& tag,LLPanelClassCreatorFunc func)
	{
		mPanelClassesNames[tag] = func;
	}

	LLPanel* createPanelClass(const std::string& tag)
	{
		param_name_map_t::iterator iT =  mPanelClassesNames.find(tag);
		if(iT == mPanelClassesNames.end())
			return 0;
		return iT->second();
	}
	template<typename T>
	static T* defaultPanelClassBuilder()
	{
		T* pT = new T();
		return pT;
	}

private:
	typedef std::map< std::string, LLPanelClassCreatorFunc> param_name_map_t;
	
	param_name_map_t mPanelClassesNames;
};


// local static instance for registering a particular panel class
template<typename T>
	class LLPanelInjector
{
public:
	// register with either the provided builder, or the generic templated builder
	LLPanelInjector(const std::string& tag);
};


template<typename T>
	LLPanelInjector<T>::LLPanelInjector(const std::string& tag) 
{
	LLRegisterPanelClass::instance().addPanelClass(tag,&LLRegisterPanelClass::defaultPanelClassBuilder<T>);
}


#endif
