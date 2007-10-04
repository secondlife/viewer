/** 
 * @file lluictrlfactory.h
 * @brief Factory class for creating UI controls
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#ifndef LLUICTRLFACTORY_H
#define LLUICTRLFACTORY_H

#include <iosfwd>
#include <stack>

#include "llcallbackmap.h"
#include "llfloater.h"

class LLControlGroup;
class LLView;
class LLFontGL;

class LLFloater;
class LLPanel;
class LLButton;
class LLCheckBoxCtrl;
class LLComboBox;
class LLIconCtrl;
class LLLineEditor;
class LLMenuGL;
class LLMenuBarGL;
class LLMenuItemCallGL;
class LLNameListCtrl;
class LLPieMenu;
class LLRadioGroup;
class LLSearchEditor;
class LLScrollableContainerView;
class LLScrollListCtrl;
class LLSlider;
class LLSliderCtrl;
class LLSpinCtrl;
class LLTextBox;
class LLTextEditor;
class LLTextureCtrl;
class LLWebBrowserCtrl;
class LLViewBorder;
class LLColorSwatchCtrl;
class LLScrollingPanelList;
class LLCtrlListInterface;
class LLCtrlSelectionInterface;
class LLCtrlScrollInterface;

// Widget 

class LLUICtrlFactory
{
public:
	LLUICtrlFactory();
	// do not call!  needs to be public so run-time can clean up the singleton
	virtual ~LLUICtrlFactory();

	void setupPaths();
	
	void buildFloater(LLFloater* floaterp, const LLString &filename, 
						const LLCallbackMap::map_t* factory_map = NULL, BOOL open = TRUE);

	BOOL buildPanel(LLPanel* panelp, const LLString &filename,
					const LLCallbackMap::map_t* factory_map = NULL);

	LLMenuGL *buildMenu(const LLString &filename, LLView* parentp);

	LLPieMenu *buildPieMenu(const LLString &filename, LLView* parentp);

	// Does what you want for LLFloaters and LLPanels
	// Returns 0 on success
	S32 saveToXML(LLView* viewp, const LLString& filename);

	void removePanel(LLPanel* panelp);
	void removeFloater(LLFloater* floaterp);

	void rebuild();

	static EWidgetType getWidgetType(const LLString& ctrl_type);
	static LLString	getWidgetType(EWidgetType ctrl_type);
	static BOOL getAttributeColor(LLXMLNodePtr node, const LLString& name, LLColor4& color);

	// specific typed getters
	static LLButton*			getButtonByName(		LLPanel* panelp, const LLString& name);
	static LLCheckBoxCtrl*		getCheckBoxByName(		LLPanel* panelp, const LLString& name);
	static LLComboBox*			getComboBoxByName(		LLPanel* panelp, const LLString& name);
	static LLIconCtrl*			getIconByName(			LLPanel* panelp, const LLString& name);
	static LLLineEditor*		getLineEditorByName(	LLPanel* panelp, const LLString& name);
	static LLNameListCtrl*		getNameListByName(		LLPanel* panelp, const LLString& name);
	static LLRadioGroup*		getRadioGroupByName(	LLPanel* panelp, const LLString& name);
	static LLScrollListCtrl*	getScrollListByName(	LLPanel* panelp, const LLString& name);
	static LLSliderCtrl*		getSliderByName(		LLPanel* panelp, const LLString& name);
	static LLSlider*			getSliderBarByName(		LLPanel* panelp, const LLString& name);
	static LLSpinCtrl*			getSpinnerByName(		LLPanel* panelp, const LLString& name);
	static LLTextBox*			getTextBoxByName(		LLPanel* panelp, const LLString& name);
	static LLTextEditor*		getTextEditorByName(	LLPanel* panelp, const LLString& name);
	static LLTabContainerCommon* getTabContainerByName(	LLPanel* panelp, const LLString& name);
	static LLScrollableContainerView*	getScrollableContainerByName(LLPanel* panelp, const LLString& name);
	static LLTextureCtrl*		getTexturePickerByName(	LLPanel* panelp, const LLString& name);
	static LLPanel* 			getPanelByName(LLPanel* panelp, const LLString& name);
	static LLColorSwatchCtrl*	getColorSwatchByName(LLPanel* panelp, const LLString& name);
	static LLWebBrowserCtrl*	getWebBrowserCtrlByName(LLPanel* panelp, const LLString& name);
	static LLMenuItemCallGL*	getMenuItemCallByName(LLPanel* panelp, const LLString& name);
	static LLScrollingPanelList* getScrollingPanelList(LLPanel* panelp, const LLString& name);

	// interface getters
	static LLCtrlListInterface* getListInterfaceByName(LLPanel* panelp, const LLString& name);
	static LLCtrlSelectionInterface* getSelectionInterfaceByName(LLPanel* panelp, const LLString& name);
	static LLCtrlScrollInterface* getScrollInterfaceByName(LLPanel* panelp, const LLString& name);

	LLPanel* createFactoryPanel(LLString name);

	virtual LLView* createCtrlWidget(LLPanel *parent, LLXMLNodePtr node);
	virtual void createWidget(LLPanel *parent, LLXMLNodePtr node);

	// Creator library
	typedef LLView* (*creator_function_t)(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	void registerCreator(LLString ctrlname, creator_function_t function);

	static bool getLayeredXMLNode(const LLString &filename, LLXMLNodePtr& root);

protected:


	typedef std::map<LLViewHandle, LLString> built_panel_t;
	built_panel_t mBuiltPanels;
	typedef std::map<LLViewHandle, LLString> built_floater_t;
	built_floater_t mBuiltFloaters;

	std::deque<const LLCallbackMap::map_t*> mFactoryStack;

	static const LLString sUICtrlNames[];

	typedef std::map<LLString, creator_function_t> creator_list_t;
	creator_list_t mCreatorFunctions;
	static std::vector<LLString> mXUIPaths;
};

template<class T>
class LLUICtrlCreator
{
public:
	static void registerCreator(LLString name, LLUICtrlFactory *factory)
	{
		factory->registerCreator(name, T::fromXML);
	}
};

#endif //LL_LLWIDGETFACTORY_H
