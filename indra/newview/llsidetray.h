/** 
 * @file LLSideTray.h
 * @brief SideBar header file
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLSIDETRAY_H_
#define LL_LLSIDETRAY_H_

#include "llpanel.h"
#include "string"

class LLAccordionCtrl;
class LLSideTrayTab;

// added inheritance from LLDestroyClass<LLSideTray> to enable Side Tray perform necessary actions 
// while disconnecting viewer in LLAppViewer::disconnectViewer().
// LLDestroyClassList::instance().fireCallbacks() calls destroyClass method. See EXT-245.
class LLSideTray : public LLPanel, private LLDestroyClass<LLSideTray>
{
	friend class LLUICtrlFactory;
	friend class LLDestroyClass<LLSideTray>;
public:

	LOG_CLASS(LLSideTray);

	struct Params 
	:	public LLInitParam::Block<Params, LLPanel::Params>
	{
		// initial state
		Optional<bool>		collapsed;
		Optional<std::string>		tab_btn_image_normal;
		Optional<std::string>		tab_btn_image_selected;
		
		Optional<S32>				default_button_width;
		Optional<S32>				default_button_height;
		Optional<S32>				default_button_margin;
		
		Params();
	};

	static LLSideTray*	getInstance		();
	static bool			instanceCreated	();
protected:
	LLSideTray(Params& params);
	typedef std::vector<LLSideTrayTab*> child_vector_t;
	typedef child_vector_t::iterator					child_vector_iter_t;
	typedef child_vector_t::const_iterator  			child_vector_const_iter_t;
	typedef child_vector_t::reverse_iterator 			child_vector_reverse_iter_t;
	typedef child_vector_t::const_reverse_iterator 		child_vector_const_reverse_iter_t;

public:

	// interface functions
	    
	/**
     * Select tab with specific name and set it active    
     */
	bool 		selectTabByName	(const std::string& name);
	
	/**
     * Select tab with specific index and set it active    
     */
	bool		selectTabByIndex(size_t index);

	/**
	 * Activate tab with "panel_name" panel
	 * if no such tab - return NULL, otherwise a pointer to the panel
	 * Pass params as array, or they may be overwritten(example - params["name"]="nearby")
	 */
	LLPanel*	showPanel		(const std::string& panel_name, const LLSD& params);

	/**
	 * Toggling Side Tray tab which contains "sub_panel" child of "panel_name" panel.
	 * If "sub_panel" is not visible Side Tray is opened to display it,
	 * otherwise Side Tray is collapsed.
	 * params are passed to "panel_name" panel onOpen().
	 */
	void		togglePanel		(LLPanel* &sub_panel, const std::string& panel_name, const LLSD& params);

	/*
	 * get the panel (don't show it or do anything else with it)
	 */
    LLPanel*	getPanel		(const std::string& panel_name);
    LLPanel*	getActivePanel	();
    bool		isPanelActive	(const std::string& panel_name);
	/*
	 * get currently active tab
	 */
    const LLSideTrayTab*	getActiveTab() const { return mActiveTab; }

	/*
     * collapse SideBar, hiding visible tab and moving tab buttons
     * to the right corner of the screen
     */
	void		collapseSideBar	();
    
	/*
     * expand SideBar
     */
	void		expandSideBar	();


	/**
	 *hightlight if focused. manly copypaste from highlightFocusedFloater
	 */
	void		highlightFocused();

	void		setVisible(BOOL visible)
	{
		if (getParent()) getParent()->setVisible(visible);
	}

	LLPanel*	getButtonsPanel() { return mButtonsPanel; }

public:
	virtual ~LLSideTray(){};

    virtual BOOL postBuild();

	void		onTabButtonClick(std::string name);
	void		onToggleCollapse();

	bool		addChild		(LLView* view, S32 tab_group);

	BOOL		handleMouseDown	(S32 x, S32 y, MASK mask);
	
	void		reshape			(S32 width, S32 height, BOOL called_from_parent = TRUE);

	void		processTriState ();
	
	void		updateSidetrayVisibility();

protected:
	LLSideTrayTab* getTab		(const std::string& name);

	void		createButtons	();
	LLButton*	createButton	(const std::string& name,const std::string& image,const std::string& tooltip,
									LLUICtrl::commit_callback_t callback);
	void		arrange			();
	void		reflectCollapseChange();

	void		toggleTabButton	(LLSideTrayTab* tab);

private:
	// Implementation of LLDestroyClass<LLSideTray>
	static void destroyClass()
	{
		// Disable SideTray to avoid crashes. EXT-245
		if (LLSideTray::instanceCreated())
			LLSideTray::getInstance()->setEnabled(FALSE);
	}
	
private:

	LLPanel*						mButtonsPanel;
	typedef std::map<std::string,LLButton*> button_map_t;
	button_map_t					mTabButtons;
	child_vector_t					mTabs;
	LLSideTrayTab*					mActiveTab;	
	
	LLButton*						mCollapseButton;
	bool							mCollapsed;
	
	static LLSideTray*				sInstance;
};

#endif

