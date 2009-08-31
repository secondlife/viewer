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

class LLSideTray;
class LLAccordionCtrl;

class LLSideTrayTab: public LLPanel
{
	friend class LLUICtrlFactory;
	friend class LLSideTray;
public:

	struct Params 
	:	public LLInitParam::Block<Params, LLPanel::Params>
	{
		// image name
		Optional<std::string>		image_path;
		Optional<std::string>		tab_title;
		Optional<std::string>		description;
		Params()
		:	image_path("image"),
			tab_title("tab_title","no title"),
			description("description","no description")
		{};
	};
protected:
	LLSideTrayTab(const Params& params);
	

public:
	virtual ~LLSideTrayTab();

    /*virtual*/ BOOL	postBuild	();
	/*virtual*/ bool	addChild	(LLView* view, S32 tab_group);


	void			arrange		(S32 width, S32 height);
	void			reshape		(S32 width, S32 height, BOOL called_from_parent = TRUE);
	
	static LLSideTrayTab*  createInstance	();

	const std::string& getDescription () const { return mDescription;}
	const std::string& getTabTitle() const { return mTabTitle;}

	void draw();

	void			onOpen		(const LLSD& key);

private:
	std::string mTabTitle;
	std::string mImagePath;
	std::string	mDescription;

	LLView*	mMainPanel;
};

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
		
		Params()
		:	collapsed("collapsed",false),
			tab_btn_image_normal("tab_btn_image","sidebar_tab_left.tga"),
			tab_btn_image_selected("tab_btn_image_selected","button_enabled_selected_32x128.tga"),
			default_button_width("tab_btn_width",32),
			default_button_height("tab_btn_height",32),
			default_button_margin("tab_btn_margin",0)
		{};
	};

	static LLSideTray*	getInstance		();
	static bool			instanceCreated	();
protected:
	LLSideTray(Params& params);
	typedef std::vector<LLView*> child_vector_t;
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
     * add new panel to tab with tab_name name
     * @param tab_name - name of sidebar tab to add new panel
     * @param panel - pointer to panel 
     */
    bool        addPanel        ( const std::string& tab_name
                                 ,LLPanel* panel );
    /**
     * Add new tab to side bar
     * @param tab_name - name of the new tab
     * @param image - image for new sidebar button
     * @param title -  title for new tab
     */
    bool        addTab          ( const std::string& tab_name
                                 ,const std::string& image
                                 ,const std::string& title);

	/**
	 * Activate tab with "panel_name" panel
	 * if no such tab - return NULL, otherwise a pointer to the panel
	 * Pass params as array, or they may be overwritten(example - params["name"]="nearby")
	 */
    LLPanel*	showPanel		(const std::string& panel_name, const LLSD& params);

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
		LLPanel::setVisible(visible);
	}

public:
	virtual ~LLSideTray(){};

    virtual BOOL postBuild();

	void		onTabButtonClick(std::string name);
	void		onToggleCollapse();

	bool		addChild		(LLView* view, S32 tab_group);

	BOOL		handleMouseDown	(S32 x, S32 y, MASK mask);
	
	void		reshape			(S32 width, S32 height, BOOL called_from_parent = TRUE);
	S32			getTrayWidth();

	void		resetPanelRect	();
	

protected:
	LLSideTrayTab* getTab		(const std::string& name);

	void		createButtons	();
	LLButton*	createButton	(const std::string& name,const std::string& image,LLUICtrl::commit_callback_t callback);
	void		arrange			();
	void		reflectCollapseChange();

	void		toggleTabButton	(LLSideTrayTab* tab);

	void		setPanelRect	();

	

private:
	// Implementation of LLDestroyClass<LLSideTray>
	static void destroyClass()
	{
		// Disable SideTray to avoid crashes. EXT-245
		if (LLSideTray::instanceCreated())
			LLSideTray::getInstance()->setEnabled(FALSE);
	}
	

private:

	std::map<std::string,LLButton*>	mTabButtons;
	child_vector_t					mTabs;
	LLSideTrayTab*					mActiveTab;	
	
	LLButton*						mCollapseButton;
	bool							mCollapsed;
	
	S32								mMaxBarWidth;

	static LLSideTray*				sInstance;
};

#endif

