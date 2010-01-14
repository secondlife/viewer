/** 
 * @file llsidetray.cpp
 * @brief SideBar implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
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

#include "llviewerprecompiledheaders.h"

#include "lltextbox.h"

#include "llagent.h"
#include "llbottomtray.h"
#include "llsidetray.h"
#include "llviewerwindow.h"
#include "llaccordionctrl.h"
#include "llfocusmgr.h"
#include "llrootview.h"
#include "llnavigationbar.h"

#include "llaccordionctrltab.h"

#include "llfloater.h" //for gFloaterView
#include "lliconctrl.h"//for OpenClose tab icon
#include "llsidetraypanelcontainer.h"
#include "llwindow.h"//for SetCursor
#include "lltransientfloatermgr.h"

//#include "llscrollcontainer.h"

using namespace std;

static LLRootViewRegistry::Register<LLSideTray>	t1("side_tray");
static LLDefaultChildRegistry::Register<LLSideTrayTab>	t2("sidetray_tab");

static const std::string COLLAPSED_NAME = "<<";
static const std::string EXPANDED_NAME  = ">>";

static const std::string TAB_PANEL_CAPTION_NAME = "sidetray_tab_panel";
static const std::string TAB_PANEL_CAPTION_TITLE_BOX = "sidetray_tab_title";

LLSideTray* LLSideTray::sInstance = 0;

LLSideTray* LLSideTray::getInstance()
{
	if (!sInstance)
	{
		sInstance = LLUICtrlFactory::createFromFile<LLSideTray>("panel_side_tray.xml",NULL, LLRootView::child_registry_t::instance());
		sInstance->setXMLFilename("panel_side_tray.xml");
	}

	return sInstance;
}

bool	LLSideTray::instanceCreated	()
{
	return sInstance!=0;
}

//////////////////////////////////////////////////////////////////////////////
// LLSideTrayTab
// Represents a single tab in the side tray, only used by LLSideTray
//////////////////////////////////////////////////////////////////////////////

class LLSideTrayTab: public LLPanel
{
	friend class LLUICtrlFactory;
	friend class LLSideTray;
public:
	
	struct Params 
	:	public LLInitParam::Block<Params, LLPanel::Params>
	{
		// image name
		Optional<std::string>		image;
		Optional<std::string>		image_selected;
		Optional<std::string>		tab_title;
		Optional<std::string>		description;
		Params()
		:	image("image"),
			image_selected("image_selected"),
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
	
	
	void			reshape		(S32 width, S32 height, BOOL called_from_parent = TRUE);
	
	static LLSideTrayTab*  createInstance	();
	
	const std::string& getDescription () const { return mDescription;}
	const std::string& getTabTitle() const { return mTabTitle;}
	
	void			onOpen		(const LLSD& key);
	
	LLPanel *getPanel();
private:
	std::string mTabTitle;
	std::string mImage;
	std::string mImageSelected;
	std::string	mDescription;
	
	LLView*	mMainPanel;
};

LLSideTrayTab::LLSideTrayTab(const Params& p)
:	LLPanel(),
	mTabTitle(p.tab_title),
	mImage(p.image),
	mImageSelected(p.image_selected),
	mDescription(p.description),
	mMainPanel(NULL)
{
	// Necessary for focus movement among child controls
	setFocusRoot(TRUE);
}

LLSideTrayTab::~LLSideTrayTab()
{
}

bool LLSideTrayTab::addChild(LLView* view, S32 tab_group)
{
	if(mMainPanel == 0 && TAB_PANEL_CAPTION_NAME != view->getName())//skip our caption panel
		mMainPanel = view;
	return LLPanel::addChild(view,tab_group);
	//return res;
}



//virtual 
BOOL LLSideTrayTab::postBuild()
{
	LLPanel* title_panel = LLUICtrlFactory::getInstance()->createFromFile<LLPanel>("panel_side_tray_tab_caption.xml",this, child_registry_t::instance());
	string name = title_panel->getName();
	LLPanel::addChild(title_panel);
	
	title_panel->getChild<LLTextBox>(TAB_PANEL_CAPTION_TITLE_BOX)->setValue(mTabTitle);

	return true;
}

static const S32 splitter_margin = 1;

void LLSideTrayTab::reshape		(S32 width, S32 height, BOOL called_from_parent )
{
	LLPanel::reshape(width, height, called_from_parent);
	LLView* title_panel = findChildView(TAB_PANEL_CAPTION_NAME, true);
	if (!title_panel)
	{
		// not fully constructed yet
		return;
	}

	S32 title_height = title_panel->getRect().getHeight();
	title_panel->setOrigin( 0, height - title_height );
	title_panel->reshape(width,title_height);

	LLRect sRect;
	sRect.setLeftTopAndSize( splitter_margin, height - title_height - splitter_margin, 
							width - 2*splitter_margin, height - title_height - 2*splitter_margin);
	mMainPanel->setShape(sRect);
}

void	LLSideTrayTab::onOpen		(const LLSD& key)
{
	LLPanel *panel = getPanel();
	if(panel)
		panel->onOpen(key);
}

LLPanel*	LLSideTrayTab::getPanel()
{
	LLPanel* panel = dynamic_cast<LLPanel*>(mMainPanel);
	return panel;
}

LLSideTrayTab*  LLSideTrayTab::createInstance	()
{
	LLSideTrayTab::Params tab_params; 
	tab_params.tab_title("openclose");

	LLSideTrayTab* tab = LLUICtrlFactory::create<LLSideTrayTab>(tab_params);
	return tab;
}

//////////////////////////////////////////////////////////////////////////////
// LLSideTray
//////////////////////////////////////////////////////////////////////////////

LLSideTray::Params::Params()
:	collapsed("collapsed",false),
	tab_btn_image_normal("tab_btn_image","sidebar_tab_left.tga"),
	tab_btn_image_selected("tab_btn_image_selected","button_enabled_selected_32x128.tga"),
	default_button_width("tab_btn_width",32),
	default_button_height("tab_btn_height",32),
	default_button_margin("tab_btn_margin",0)
{}

//virtual 
LLSideTray::LLSideTray(Params& params)
	   : LLPanel(params)
	    ,mActiveTab(0)
		,mCollapsed(false)
		,mCollapseButton(0)
{
	mCollapsed=params.collapsed;

	LLUICtrl::CommitCallbackRegistry::Registrar& commit = LLUICtrl::CommitCallbackRegistry::currentRegistrar();

	// register handler function to process data from the xml. 
	// panel_name should be specified via "parameter" attribute.
	commit.add("SideTray.ShowPanel", boost::bind(&LLSideTray::showPanel, this, _2, LLUUID::null));
	LLTransientFloaterMgr::getInstance()->addControlView(this);

	LLPanel::Params p;
	p.name = "buttons_panel";
	p.mouse_opaque = false;
	mButtonsPanel = LLUICtrlFactory::create<LLPanel>(p);
}


BOOL LLSideTray::postBuild()
{
	createButtons();

	arrange();
	selectTabByName("sidebar_home");

	if(mCollapsed)
		collapseSideBar();

	setMouseOpaque(false);
	return true;
}

LLSideTrayTab* LLSideTray::getTab(const std::string& name)
{
	return getChild<LLSideTrayTab>(name,false);
}


void LLSideTray::toggleTabButton(LLSideTrayTab* tab)
{
	if(tab == NULL)
		return;
	std::string name = tab->getName();
	std::map<std::string,LLButton*>::iterator it = mTabButtons.find(name);
	if(it != mTabButtons.end())
	{
		LLButton* btn = it->second;
		bool new_state = !btn->getToggleState();
		btn->setToggleState(new_state); 
		btn->setImageOverlay( new_state ? tab->mImageSelected : tab->mImage );
	}
}

bool LLSideTray::selectTabByIndex(size_t index)
{
	if(index>=mTabs.size())
		return false;

	LLSideTrayTab* sidebar_tab = mTabs[index];
	return selectTabByName(sidebar_tab->getName());
}

bool LLSideTray::selectTabByName	(const std::string& name)
{
	LLSideTrayTab* side_bar = getTab(name);

	if(side_bar == mActiveTab)
		return false;
	//deselect old tab
	toggleTabButton(mActiveTab);
	if(mActiveTab)
		mActiveTab->setVisible(false);

	//select new tab
	mActiveTab = side_bar;
	toggleTabButton(mActiveTab);
	LLSD key;//empty
	mActiveTab->onOpen(key);

	mActiveTab->setVisible(true);

	//arrange();
	
	//hide all tabs - show active tab
	child_vector_const_iter_t child_it;
	for ( child_it = mTabs.begin(); child_it != mTabs.end(); ++child_it)
	{
		LLSideTrayTab* sidebar_tab = *child_it;
		sidebar_tab->setVisible(sidebar_tab  == mActiveTab);
	}
	return true;
}

LLButton* LLSideTray::createButton	(const std::string& name,const std::string& image,const std::string& tooltip,
									 LLUICtrl::commit_callback_t callback)
{
	static LLSideTray::Params sidetray_params(LLUICtrlFactory::getDefaultParams<LLSideTray>());	
	
	LLButton::Params bparams;

	LLRect rect;
	rect.setOriginAndSize(0, 0, sidetray_params.default_button_width, sidetray_params.default_button_height); 

	bparams.name(name);
	bparams.follows.flags (FOLLOWS_LEFT | FOLLOWS_TOP);
	bparams.rect (rect);
	bparams.tab_stop(false);
	bparams.image_unselected.name(sidetray_params.tab_btn_image_normal);
	bparams.image_selected.name(sidetray_params.tab_btn_image_selected);
	bparams.image_disabled.name(sidetray_params.tab_btn_image_normal);
	bparams.image_disabled_selected.name(sidetray_params.tab_btn_image_selected);

	LLButton* button = LLUICtrlFactory::create<LLButton> (bparams);
	button->setLabel(name);
	button->setClickedCallback(callback);

	button->setToolTip(tooltip);
	
	if(image.length())
	{
		button->setImageOverlay(image);
	}

	mButtonsPanel->addChildInBack(button);

	return button;
}

bool LLSideTray::addChild(LLView* view, S32 tab_group)
{
	LLSideTrayTab* tab_panel = dynamic_cast<LLSideTrayTab*>(view);

	if (tab_panel)
	{
		mTabs.push_back(tab_panel);
	}
	
	return LLUICtrl::addChild(view, tab_group);
}


void	LLSideTray::createButtons	()
{
	//create buttons for tabs
	child_vector_const_iter_t child_it = mTabs.begin();
	for ( ; child_it != mTabs.end(); ++child_it)
	{
		LLSideTrayTab* sidebar_tab = *child_it;

		std::string name = sidebar_tab->getName();
		
		// The "OpenClose" button will open/close the whole panel
		if (name == "sidebar_openclose")
		{
			mCollapseButton = createButton("",sidebar_tab->mImage,sidebar_tab->getTabTitle(),
				boost::bind(&LLSideTray::onToggleCollapse, this));
		}
		else
		{
			LLButton* button = createButton("",sidebar_tab->mImage,sidebar_tab->getTabTitle(),
				boost::bind(&LLSideTray::onTabButtonClick, this, name));
			mTabButtons[name] = button;
		}
	}
}

void		LLSideTray::processTriState ()
{
	if(mCollapsed)
		expandSideBar();
	else
	{
#if 0 // *TODO: EXT-2092
		
		// Tell the active task panel to switch to its default view
		// or collapse side tray if already on the default view.
		LLSD info;
		info["task-panel-action"] = "handle-tri-state";
		mActiveTab->notifyChildren(info);
#else
		collapseSideBar();
#endif
	}
}

void		LLSideTray::onTabButtonClick(string name)
{
	LLSideTrayTab* side_bar = getTab(name);
	if(side_bar == mActiveTab)
	{
		processTriState ();
		return;
	}
	selectTabByName	(name);
	if(mCollapsed)
		expandSideBar();
}

void		LLSideTray::onToggleCollapse()
{
	if(mCollapsed)
	{
		expandSideBar();
		//selectTabByName("sidebar_openclose");
	}
	else
		collapseSideBar();
}


void LLSideTray::reflectCollapseChange()
{
	updateSidetrayVisibility();

	if(mCollapsed)
	{
		gFloaterView->setSnapOffsetRight(0);
		setFocus(FALSE);
	}
	else
	{
		gFloaterView->setSnapOffsetRight(getRect().getWidth());
		setFocus(TRUE);
	}

	gFloaterView->refresh();
}

void LLSideTray::arrange()
{
	static LLSideTray::Params sidetray_params(LLUICtrlFactory::getDefaultParams<LLSideTray>());	

	updateSidetrayVisibility();
	
	LLRect ctrl_rect;
	ctrl_rect.setLeftTopAndSize(0,
								mButtonsPanel->getRect().getHeight() - sidetray_params.default_button_width,
								sidetray_params.default_button_width,
								sidetray_params.default_button_height);

	mCollapseButton->setRect(ctrl_rect);

	//arrange tab buttons
	//arrange tab buttons
	child_vector_const_iter_t child_it;
	int offset = (sidetray_params.default_button_height+sidetray_params.default_button_margin)*2;
	for ( child_it = mTabs.begin(); child_it != mTabs.end(); ++child_it)	
	{
		LLSideTrayTab* sidebar_tab = *child_it;
		
		ctrl_rect.setLeftTopAndSize(0,
									mButtonsPanel->getRect().getHeight()-offset,
									sidetray_params.default_button_width,
									sidetray_params.default_button_height);

		if(mTabButtons.find(sidebar_tab->getName()) == mTabButtons.end())
			continue;

		LLButton* btn = mTabButtons[sidebar_tab->getName()];

		btn->setRect(ctrl_rect);
		offset+=sidetray_params.default_button_height;
		offset+=sidetray_params.default_button_margin;

		btn->setVisible(ctrl_rect.mBottom > 0);
	}

	//arrange tabs
	for ( child_vector_t::iterator child_it = mTabs.begin(); child_it != mTabs.end(); ++child_it)
	{
		LLSideTrayTab* sidebar_tab = *child_it;
		sidebar_tab->setShape(getLocalRect());
	}
}

void LLSideTray::collapseSideBar()
{
	mCollapsed = true;
	// Reset all overlay images, because there is no "selected" tab when the
	// whole side tray is hidden.
	child_vector_const_iter_t it = mTabs.begin();
	for ( ; it != mTabs.end(); ++it )
	{
		LLSideTrayTab* tab = *it;
		std::string name = tab->getName();
		std::map<std::string,LLButton*>::const_iterator btn_it =
			mTabButtons.find(name);
		if (btn_it != mTabButtons.end())
		{
			LLButton* btn = btn_it->second;
			btn->setImageOverlay( tab->mImage );
		}
	}
		
	// OpenClose tab doesn't put its button in mTabButtons
	LLSideTrayTab* openclose_tab = getTab("sidebar_openclose");
	if (openclose_tab)
	{
		mCollapseButton->setImageOverlay( openclose_tab->mImage );
	}
	//mActiveTab->setVisible(FALSE);
	reflectCollapseChange();
	setFocus( FALSE );

}

void LLSideTray::expandSideBar()
{
	mCollapsed = false;
	LLSideTrayTab* openclose_tab = getTab("sidebar_openclose");
	if (openclose_tab)
	{
		mCollapseButton->setImageOverlay( openclose_tab->mImageSelected );
	}
	LLSD key;//empty
	mActiveTab->onOpen(key);

	reflectCollapseChange();


	std::string name = mActiveTab->getName();
	std::map<std::string,LLButton*>::const_iterator btn_it =
		mTabButtons.find(name);
	if (btn_it != mTabButtons.end())
	{
		LLButton* btn = btn_it->second;
		btn->setImageOverlay( mActiveTab->mImageSelected  );
	}

}

void LLSideTray::highlightFocused()
{
	/* uncomment in case something change
	if(!mActiveTab)
		return;
	BOOL dependent_has_focus = gFocusMgr.childHasKeyboardFocus(this);
	setBackgroundOpaque( dependent_has_focus ); 
	mActiveTab->setBackgroundOpaque( dependent_has_focus ); 
	*/
}

//virtual
BOOL		LLSideTray::handleMouseDown	(S32 x, S32 y, MASK mask)
{
	BOOL ret = LLPanel::handleMouseDown(x,y,mask);
	if(ret)
		setFocus(true);	
	return ret;
}

void LLSideTray::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);
	if(!mActiveTab)
		return;
	
	arrange();
}

/**
 * Activate tab with "panel_name" panel
 * if no such tab - return false, otherwise true.
 * TODO* In some cases a pointer to a panel of
 * a specific class may be needed so this method
 * would need to use templates.
 */
LLPanel*	LLSideTray::showPanel		(const std::string& panel_name, const LLSD& params)
{
	//arrange tabs
	child_vector_const_iter_t child_it;
	for ( child_it = mTabs.begin(); child_it != mTabs.end(); ++child_it)
	{
		LLView* view = (*child_it)->findChildView(panel_name,true);
		if(view)
		{
			selectTabByName	((*child_it)->getName());
			if(mCollapsed)
				expandSideBar();

			LLSideTrayPanelContainer* container = dynamic_cast<LLSideTrayPanelContainer*>(view->getParent());
			if(container)
			{
				LLSD new_params = params;
				new_params[LLSideTrayPanelContainer::PARAM_SUB_PANEL_NAME] = panel_name;
				container->onOpen(new_params);

				return container->getCurrentPanel();
			}

			LLPanel* panel = dynamic_cast<LLPanel*>(view);
			if(panel)
			{
				panel->onOpen(params);
			}
			return panel;
		}
	}
	return NULL;
}

// This is just LLView::findChildView specialized to restrict the search to LLPanels.
// Optimization for EXT-4068 to avoid searching down to the individual item level
// when inventories are large.
LLPanel *findChildPanel(LLPanel *panel, const std::string& name, bool recurse)
{
	for (LLView::child_list_const_iter_t child_it = panel->beginChild();
		 child_it != panel->endChild(); ++child_it)
	{
		LLPanel *child_panel = dynamic_cast<LLPanel*>(*child_it);
		if (!child_panel)
			continue;
		if (child_panel->getName() == name)
			return child_panel;
	}
	if (recurse)
	{
		for (LLView::child_list_const_iter_t child_it = panel->beginChild();
			 child_it != panel->endChild(); ++child_it)
		{
			LLPanel *child_panel = dynamic_cast<LLPanel*>(*child_it);
			if (!child_panel)
				continue;
			LLPanel *found_panel = findChildPanel(child_panel,name,recurse);
			if (found_panel)
			{
				return found_panel;
			}
		}
	}
	return NULL;
}

LLPanel* LLSideTray::getPanel(const std::string& panel_name)
{
	for ( child_vector_const_iter_t child_it = mTabs.begin(); child_it != mTabs.end(); ++child_it)
	{
		LLPanel *panel = findChildPanel(*child_it,panel_name,true);
		if(panel)
		{
			return panel;
		}
	}
	return NULL;
}

LLPanel*	LLSideTray::getActivePanel()
{
	if (mActiveTab && !mCollapsed)
	{
		return mActiveTab->getPanel();
	}
	return NULL;
}

bool		LLSideTray::isPanelActive(const std::string& panel_name)
{
	LLPanel *panel = getActivePanel();
	if (!panel) return false;
	return (panel->getName() == panel_name);
}


// *TODO: Eliminate magic constants.
static const S32	fake_offset = 132;
static const S32	fake_top_offset = 18;

void	LLSideTray::updateSidetrayVisibility()
{
	// set visibility of parent container based on collapsed state
	if (getParent())
	{
		getParent()->setVisible(!mCollapsed && !gAgent.cameraMouselook());
	}
}

