/** 
 * @file LLSideTray.h
 * @brief SideBar header file
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLSIDETRAY_H_
#define LL_LLSIDETRAY_H_

#include "llpanel.h"
#include "string"

class LLAccordionCtrl;
class LLSideTrayTab;

// Deal with LLSideTrayTab being opaque. Generic do-nothing cast...
template <class T>
T tab_cast(LLSideTrayTab* tab) { return tab; }
// specialized for implementation in presence of LLSideTrayTab definition
template <>
LLPanel* tab_cast<LLPanel*>(LLSideTrayTab* tab);

// added inheritance from LLDestroyClass<LLSideTray> to enable Side Tray perform necessary actions 
// while disconnecting viewer in LLAppViewer::disconnectViewer().
// LLDestroyClassList::instance().fireCallbacks() calls destroyClass method. See EXT-245.
class LLSideTray : public LLPanel, private LLDestroyClass<LLSideTray>
{
	friend class LLUICtrlFactory;
	friend class LLDestroyClass<LLSideTray>;
	friend class LLSideTrayTab;
	friend class LLSideTrayButton;
public:

	LOG_CLASS(LLSideTray);

	struct Params 
	:	public LLInitParam::Block<Params, LLPanel::Params>
	{
		// initial state
		Optional<bool>				collapsed;
		Optional<LLUIImage*>		tab_btn_image_normal,
									tab_btn_image_selected;
		
		Optional<S32>				default_button_width,
									default_button_height,
									default_button_margin;
		
		Params();
	};

	static LLSideTray*	getInstance		();
	static bool			instanceCreated	();
protected:
	LLSideTray(const Params& params);
	typedef std::vector<LLSideTrayTab*> child_vector_t;
	typedef child_vector_t::iterator					child_vector_iter_t;
	typedef child_vector_t::const_iterator  			child_vector_const_iter_t;
	typedef child_vector_t::reverse_iterator 			child_vector_reverse_iter_t;
	typedef child_vector_t::const_reverse_iterator 		child_vector_const_reverse_iter_t;
	typedef std::vector<std::string>					tab_order_vector_t;
	typedef tab_order_vector_t::const_iterator			tab_order_vector_const_iter_t;

public:

	// interface functions
	    
	/**
	 * Select tab with specific name and set it active
	 *
	 * @param name				Tab to switch to.
	 * @param keep_prev_visible	Whether to keep the previously selected tab visible.
	 */
	bool 		selectTabByName	(const std::string& name, bool keep_prev_visible = false);
	
	/**
     * Select tab with specific index and set it active    
     */
	bool		selectTabByIndex(size_t index);

	/**
	 * Activate tab with "panel_name" panel
	 * if no such tab - return NULL, otherwise a pointer to the panel
	 * Pass params as array, or they may be overwritten(example - params["name"]="nearby")
	 */
	LLPanel*	showPanel		(const std::string& panel_name, const LLSD& params = LLSD());

	/**
	 * Toggling Side Tray tab which contains "sub_panel" child of "panel_name" panel.
	 * If "sub_panel" is not visible Side Tray is opened to display it,
	 * otherwise Side Tray is collapsed.
	 * params are passed to "panel_name" panel onOpen().
	 */
	void		togglePanel		(LLPanel* &sub_panel, const std::string& panel_name, const LLSD& params = LLSD());

	/*
	 * get the panel (don't show it or do anything else with it)
	 */
    LLPanel*	getPanel		(const std::string& panel_name);
    LLPanel*	getActivePanel	();
    bool		isPanelActive	(const std::string& panel_name);

	/*
	 * get the panel of given type T (don't show it or do anything else with it)
	 */
	template <typename T>
	T* getPanel(const std::string& panel_name)
	{
		T* panel = dynamic_cast<T*>(getPanel(panel_name));
		if (!panel)
		{
			llwarns << "Child named \"" << panel_name << "\" of type " << typeid(T*).name() << " not found" << llendl;
			return NULL;
		}
		return panel;
	}

	/*
     * collapse SideBar, hiding visible tab and moving tab buttons
     * to the right corner of the screen
     */
	void		collapseSideBar	();
    
	/*
     * expand SideBar
     *
     * @param open_active Whether to call onOpen() for the active tab.
     */
	void		expandSideBar(bool open_active = true);


	/**
	 *hightlight if focused. manly copypaste from highlightFocusedFloater
	 */
	void		highlightFocused();

	void		setVisible(BOOL visible)
	{
		if (getParent()) getParent()->setVisible(visible);
	}

	LLPanel*	getButtonsPanel() { return mButtonsPanel; }

	bool		getCollapsed() { return mCollapsed; }

public:
	virtual ~LLSideTray(){};

    virtual BOOL postBuild();

	BOOL		handleMouseDown	(S32 x, S32 y, MASK mask);
	
	void		reshape			(S32 width, S32 height, BOOL called_from_parent = TRUE);


	/**
	 * @return side tray width if it's visible and expanded, 0 otherwise.
	 *
	 * Not that width of the tab buttons is not included.
	 *
	 * @see setVisibleWidthChangeCallback()
	 */
	S32			getVisibleWidth();

	void		setVisibleWidthChangeCallback(const commit_signal_t::slot_type& cb);

	void		updateSidetrayVisibility();

	void		handleLoginComplete();

	bool 		isTabAttached	(const std::string& name);

protected:
	bool		addChild		(LLView* view, S32 tab_group);
	bool		removeTab		(LLSideTrayTab* tab); // Used to detach tabs temporarily
	bool		addTab			(LLSideTrayTab* tab); // Used to re-attach tabs
	bool		hasTabs			();

	const LLSideTrayTab*	getActiveTab() const { return mActiveTab; }
	LLSideTrayTab* 			getTab(const std::string& name);

	void		createButtons	();

	LLButton*	createButton	(const std::string& name,const std::string& image,const std::string& tooltip,
									LLUICtrl::commit_callback_t callback);
	void		arrange			();
	void		detachTabs		();
	void		reflectCollapseChange();
	void		processTriState ();

	void		toggleTabButton	(LLSideTrayTab* tab);

	LLPanel*	openChildPanel	(LLSideTrayTab* tab, const std::string& panel_name, const LLSD& params);

	void		onTabButtonClick(std::string name);
	void		onToggleCollapse();

private:
	// Implementation of LLDestroyClass<LLSideTray>
	static void destroyClass()
	{
		// Disable SideTray to avoid crashes. EXT-245
		if (LLSideTray::instanceCreated())
			LLSideTray::getInstance()->setEnabled(FALSE);
	}
	
private:
	// Since we provide no public way to query mTabs and mDetachedTabs, give
	// LLSideTrayListener friend access.
	friend class LLSideTrayListener;
	LLPanel*						mButtonsPanel;
	typedef std::map<std::string,LLButton*> button_map_t;
	button_map_t					mTabButtons;
	child_vector_t					mTabs;
	child_vector_t					mDetachedTabs;
	tab_order_vector_t				mOriginalTabOrder;
	LLSideTrayTab*					mActiveTab;	
	
	commit_signal_t					mVisibleWidthChangeSignal;

	LLButton*						mCollapseButton;
	bool							mCollapsed;
	
	static LLSideTray*				sInstance;
};

#endif

