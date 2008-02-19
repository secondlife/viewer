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

class LLView;
class LLPanel;

class LLUICtrlFactory
{
public:
	LLUICtrlFactory();
	// do not call!  needs to be public so run-time can clean up the singleton
	virtual ~LLUICtrlFactory() {}

	void setupPaths();
	
	void buildFloater(LLFloater* floaterp, const LLString &filename, 
					const LLCallbackMap::map_t* factory_map = NULL, BOOL open = TRUE);
	BOOL buildPanel(LLPanel* panelp, const LLString &filename,
					const LLCallbackMap::map_t* factory_map = NULL);

	void removePanel(LLPanel* panelp) { mBuiltPanels.erase(panelp->getHandle()); }
	void removeFloater(LLFloater* floaterp) { mBuiltFloaters.erase(floaterp->getHandle()); }

	class LLMenuGL *buildMenu(const LLString &filename, LLView* parentp);
	class LLPieMenu *buildPieMenu(const LLString &filename, LLView* parentp);

	// Does what you want for LLFloaters and LLPanels
	// Returns 0 on success
	S32 saveToXML(LLView* viewp, const LLString& filename);


	// Rebuilds all currently built panels.
	void rebuild();

	static EWidgetType getWidgetType(const LLString& ctrl_type);
	static LLString	getWidgetType(EWidgetType ctrl_type);
	static BOOL getAttributeColor(LLXMLNodePtr node, const LLString& name, LLColor4& color);

	// specific typed getters
	static class LLButton*				getButtonByName(		const LLPanel* panelp, const LLString& name);
	static class LLCheckBoxCtrl*		getCheckBoxByName(		const LLPanel* panelp, const LLString& name);
	static class LLComboBox*			getComboBoxByName(		const LLPanel* panelp, const LLString& name);
	static class LLIconCtrl*			getIconByName(			const LLPanel* panelp, const LLString& name);
	static class LLLineEditor*			getLineEditorByName(	const LLPanel* panelp, const LLString& name);
	static class LLRadioGroup*			getRadioGroupByName(	const LLPanel* panelp, const LLString& name);
	static class LLScrollListCtrl*		getScrollListByName(	const LLPanel* panelp, const LLString& name);
	static class LLSliderCtrl*			getSliderByName(		const LLPanel* panelp, const LLString& name);
	static class LLSlider*				getSliderBarByName(		const LLPanel* panelp, const LLString& name);
	static class LLSpinCtrl*			getSpinnerByName(		const LLPanel* panelp, const LLString& name);
	static class LLTextBox*				getTextBoxByName(		const LLPanel* panelp, const LLString& name);
	static class LLTextEditor*			getTextEditorByName(	const LLPanel* panelp, const LLString& name);
	static class LLTabContainer*		getTabContainerByName(	const LLPanel* panelp, const LLString& name);
	static class LLScrollableContainerView*	getScrollableContainerByName(const LLPanel* panelp, const LLString& name);
	static class LLPanel* 				getPanelByName(			const LLPanel* panelp, const LLString& name);
	static class LLMenuItemCallGL*		getMenuItemCallByName(	const LLPanel* panelp, const LLString& name);
	static class LLScrollingPanelList*	getScrollingPanelList(	const LLPanel* panelp, const LLString& name);

	// interface getters
	static LLCtrlListInterface* getListInterfaceByName(	const LLPanel* panelp, const LLString& name);
	static LLCtrlSelectionInterface* getSelectionInterfaceByName(const LLPanel* panelp, const LLString& name);
	static LLCtrlScrollInterface* getScrollInterfaceByName(const LLPanel* panelp, const LLString& name);

	LLPanel* createFactoryPanel(LLString name);

	virtual LLView* createCtrlWidget(LLPanel *parent, LLXMLNodePtr node);
	virtual void createWidget(LLPanel *parent, LLXMLNodePtr node);

	template <class T> T* createDummyWidget(const LLString& name)
	{
		return NULL;
		//static LLPanel dummy_panel;
		//LLXMLNodePtr new_node_ptr = new LLXMLNode(T::getWidgetTag(), FALSE);
		//return createWidget(&dummy_panel, new_node_ptr);
	}

	// Creator library
	typedef LLView* (*creator_function_t)(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	void registerCreator(LLString ctrlname, creator_function_t function);

	static bool getLayeredXMLNode(const LLString &filename, LLXMLNodePtr& root);

protected:

	template<class T>
	class LLUICtrlCreator
	{
	public:
		static void registerCreator(LLString name, LLUICtrlFactory *factory)
		{
			factory->registerCreator(name, T::fromXML);
		}
	};

private:

	typedef std::map<LLHandle<LLPanel>, LLString> built_panel_t;
	built_panel_t mBuiltPanels;
	typedef std::map<LLHandle<LLFloater>, LLString> built_floater_t;
	built_floater_t mBuiltFloaters;

	std::deque<const LLCallbackMap::map_t*> mFactoryStack;

	static const LLString sUICtrlNames[];

	typedef std::map<LLString, creator_function_t> creator_list_t;
	creator_list_t mCreatorFunctions;
	static std::vector<LLString> mXUIPaths;
};


#endif //LLUICTRLFACTORY_H
