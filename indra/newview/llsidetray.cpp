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

#include "llbottomtray.h"
#include "llsidetray.h"
#include "llviewerwindow.h"
#include "llaccordionctrl.h"
#include "llfocusmgr.h"
#include "llrootview.h"
#include "llnavigationbar.h"

#include "llaccordionctrltab.h"

#include "llfloater.h" //for gFloaterView
#include "lliconctrl.h"//for Home tab icon
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

class LLSideTrayInfoPanel: public LLPanel
{
	
public:
	LLSideTrayInfoPanel():LLPanel()
	{
		setBorderVisible(true);
	}

	BOOL handleHover(S32 x, S32 y, MASK mask)
	{
		getWindow()->setCursor(UI_CURSOR_HAND);
		return TRUE;
	}

	BOOL handleMouseUp(S32 x, S32 y, MASK mask)
	{
		std::string name = getName();
		onCommit();
		LLSideTray::getInstance()->selectTabByName(name);
		return LLPanel::handleMouseUp(x,y,mask);
	}
	void reshape		(S32 width, S32 height, BOOL called_from_parent )
	{
		return LLPanel::reshape(width, height, called_from_parent);
	}

};

static LLRegisterPanelClassWrapper<LLSideTrayInfoPanel> t_people("panel_sidetray_home_info");

LLSideTray* LLSideTray::getInstance()
{
	if (!sInstance)
	{
		sInstance = LLUICtrlFactory::createFromFile<LLSideTray>("panel_side_tray.xml",gViewerWindow->getRootView(), LLRootView::child_registry_t::instance());
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
	
	
	void			arrange		(S32 width, S32 height);
	void			reshape		(S32 width, S32 height, BOOL called_from_parent = TRUE);
	
	static LLSideTrayTab*  createInstance	();
	
	const std::string& getDescription () const { return mDescription;}
	const std::string& getTabTitle() const { return mTabTitle;}
	
	void draw();
	
	void			onOpen		(const LLSD& key);
	
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

//virtual 
void	LLSideTrayTab::arrange(S32 width, S32 height )
{
	if(!mMainPanel)
		return;
	
	S32 offset = 0;

	LLView* title_panel = findChildView(TAB_PANEL_CAPTION_NAME, true);

	if(title_panel)
	{
		title_panel->setOrigin( 0, height - title_panel->getRect().getHeight() );
		offset = title_panel->getRect().getHeight();
	}

	LLRect sRect = mMainPanel->getRect();
	sRect.setLeftTopAndSize( splitter_margin, height - offset - splitter_margin, width - 2*splitter_margin, height - offset - 2*splitter_margin);
	mMainPanel->reshape(sRect.getWidth(),sRect.getHeight());
	mMainPanel->setRect(sRect);
	

	
}

void LLSideTrayTab::reshape		(S32 width, S32 height, BOOL called_from_parent )
{
	if(!mMainPanel)
		return;
	S32 offset = 0;

	LLView* title_panel = findChildView(TAB_PANEL_CAPTION_NAME, true);

	if(title_panel)
	{
		title_panel->setOrigin( 0, height - title_panel->getRect().getHeight() );
		title_panel->reshape(width,title_panel->getRect().getHeight());
		offset = title_panel->getRect().getHeight();
	}

	

	LLRect sRect = mMainPanel->getRect();
	sRect.setLeftTopAndSize( splitter_margin, height - offset - splitter_margin, width - 2*splitter_margin, height - offset - 2*splitter_margin);
	//mMainPanel->setMaxWidth(sRect.getWidth());
	mMainPanel->reshape(sRect.getWidth(), sRect.getHeight());
	
	mMainPanel->setRect(sRect);

}

void LLSideTrayTab::draw()
{
	LLPanel::draw();
}

void	LLSideTrayTab::onOpen		(const LLSD& key)
{
	LLPanel* panel = dynamic_cast<LLPanel*>(mMainPanel);
	if(panel)
		panel->onOpen(key);
}

LLSideTrayTab*  LLSideTrayTab::createInstance	()
{
	LLSideTrayTab::Params tab_params; 
	tab_params.tab_title("Home");

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
	    ,mMaxBarWidth(params.rect.width)
{
	mCollapsed=params.collapsed;


	LLUICtrl::CommitCallbackRegistry::Registrar& commit = LLUICtrl::CommitCallbackRegistry::currentRegistrar();

	// register handler function to process data from the xml. 
	// panel_name should be specified via "parameter" attribute.
	commit.add("SideTray.ShowPanel", boost::bind(&LLSideTray::showPanel, this, _2, LLUUID::null));
	LLTransientFloaterMgr::getInstance()->addControlView(this);
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


void LLSideTray::toggleTabButton	(LLSideTrayTab* tab)
{
	if(tab == NULL)
		return;
	string name = tab->getName();
	std::map<std::string,LLButton*>::iterator tIt = mTabButtons.find(name);
	if(tIt!=mTabButtons.end())
		tIt->second->setToggleState(!tIt->second->getToggleState());
}

bool LLSideTray::selectTabByIndex(size_t index)
{
	if(index>=mTabs.size())
		return false;

	LLSideTrayTab* sidebar_tab = dynamic_cast<LLSideTrayTab*>(mTabs[index]);
	if(sidebar_tab == NULL)
		return false;
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
		LLSideTrayTab* sidebar_tab = dynamic_cast<LLSideTrayTab*>(*child_it);
		if(sidebar_tab == NULL)
			continue;
		sidebar_tab->setVisible(sidebar_tab  == mActiveTab);
	}
	return true;
}

LLButton* LLSideTray::createButton	(const std::string& name,const std::string& image,LLUICtrl::commit_callback_t callback)
{
	static LLSideTray::Params sidetray_params(LLUICtrlFactory::getDefaultParams<LLSideTray>());	
	
	LLButton::Params bparams;

	LLRect rect;
	rect.setOriginAndSize(0, 0, sidetray_params.default_button_width, sidetray_params.default_button_height); 

	bparams.name(name);
	bparams.follows.flags (FOLLOWS_LEFT | FOLLOWS_BOTTOM);
	bparams.rect (rect);
	bparams.tab_stop(false);
	bparams.image_unselected.name(sidetray_params.tab_btn_image_normal);
	bparams.image_selected.name(sidetray_params.tab_btn_image_selected);
	bparams.image_disabled.name(sidetray_params.tab_btn_image_normal);
	bparams.image_disabled_selected.name(sidetray_params.tab_btn_image_selected);

	LLButton* button = LLUICtrlFactory::create<LLButton> (bparams);
	button->setLabel(name);
	button->setClickedCallback(callback);
	
	if(image.length())
	{
		button->setImageOverlay(image);
	}

	addChildInBack(button);

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
		LLSideTrayTab* sidebar_tab = dynamic_cast<LLSideTrayTab*>(*child_it);
		if(sidebar_tab == NULL)
			continue;

		std::string name = sidebar_tab->getName();
		
		// The "home" button will open/close the whole panel, this will need to
		// change if the home screen becomes its own tab.
		if (name == "sidebar_home")
		{
			mCollapseButton = createButton("",sidebar_tab->mImage,
				boost::bind(&LLSideTray::onToggleCollapse, this));
		}
		else
		{
			LLButton* button = createButton("",sidebar_tab->mImage,
				boost::bind(&LLSideTray::onTabButtonClick, this, name));
			mTabButtons[name] = button;
		}
	}
}

void		LLSideTray::onTabButtonClick(string name)
{
	LLSideTrayTab* side_bar = getTab(name);

	if(side_bar == mActiveTab)
	{
		if(mCollapsed)
			expandSideBar();
		else
			collapseSideBar();
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
		selectTabByName("sidebar_home");
	}
	else
		collapseSideBar();
}


void LLSideTray::reflectCollapseChange()
{
	setPanelRect();

	if(mCollapsed)
	{
		gFloaterView->setSnapOffsetRight(0);
		setFocus(FALSE);
	}
	else
	{
		gFloaterView->setSnapOffsetRight(mMaxBarWidth);
		setFocus(TRUE);
	}

	gFloaterView->refresh();
}

void LLSideTray::arrange			()
{
	static LLSideTray::Params sidetray_params(LLUICtrlFactory::getDefaultParams<LLSideTray>());	

	setPanelRect();
	
	LLRect ctrl_rect;
	ctrl_rect.setLeftTopAndSize(0,getRect().getHeight()-sidetray_params.default_button_width
							,sidetray_params.default_button_width
							,sidetray_params.default_button_height);

	mCollapseButton->setRect(ctrl_rect);

	//arrange tab buttons
	//arrange tab buttons
	child_vector_const_iter_t child_it;
	int offset = (sidetray_params.default_button_height+sidetray_params.default_button_margin)*2;
	for ( child_it = mTabs.begin(); child_it != mTabs.end(); ++child_it)	
	{
		LLSideTrayTab* sidebar_tab = dynamic_cast<LLSideTrayTab*>(*child_it);
		if(sidebar_tab == NULL)
			continue;
		
		ctrl_rect.setLeftTopAndSize(0,getRect().getHeight()-offset
								,sidetray_params.default_button_width
								,sidetray_params.default_button_height);

		if(mTabButtons.find(sidebar_tab->getName()) == mTabButtons.end())
			continue;

		LLButton* btn = mTabButtons[sidebar_tab->getName()];

		btn->setRect(ctrl_rect);
		offset+=sidetray_params.default_button_height;
		offset+=sidetray_params.default_button_margin;

		btn->setVisible(ctrl_rect.mBottom > 0);
	}

	ctrl_rect.setLeftTopAndSize(sidetray_params.default_button_width,getRect().getHeight(),mMaxBarWidth,getRect().getHeight());

	//arrange tabs
	for ( child_it = mTabs.begin(); child_it != mTabs.end(); ++child_it)
	{
		LLSideTrayTab* sidebar_tab = dynamic_cast<LLSideTrayTab*>(*child_it);
		if(sidebar_tab == NULL)
			continue;
		
		sidebar_tab->setRect(ctrl_rect);
		sidebar_tab->arrange(mMaxBarWidth,getRect().getHeight());
	}
}

void LLSideTray::collapseSideBar()
{
	mCollapsed = true;
	LLSideTrayTab* home_tab = getTab("sidebar_home");
	if (home_tab)
	{
		mCollapseButton->setImageOverlay( home_tab->mImage );
	}
	mActiveTab->setVisible(FALSE);
	reflectCollapseChange();
	setFocus( FALSE );

}

void LLSideTray::expandSideBar()
{
	mCollapsed = false;
	LLSideTrayTab* home_tab = getTab("sidebar_home");
	if (home_tab)
	{
		mCollapseButton->setImageOverlay( home_tab->mImageSelected );
	}
	LLSD key;//empty
	mActiveTab->onOpen(key);
	mActiveTab->setVisible(TRUE);

	reflectCollapseChange();
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

BOOL	LLSideTray::handleScrollWheel(S32 x, S32 y, S32 mask)
{
	BOOL ret = LLPanel::handleScrollWheel(x,y,mask);

	if(!ret && childFromPoint(x,y) != 0 )
		return TRUE;//mouse wheel over sidetray buttons, eat mouse wheel
	return ret;
}

//virtual
BOOL		LLSideTray::handleMouseDown	(S32 x, S32 y, MASK mask)
{
	BOOL ret = LLPanel::handleMouseDown(x,y,mask);
	if(ret)
		setFocus(true);	
	return ret;
}

void LLSideTray::reshape			(S32 width, S32 height, BOOL called_from_parent)
{
	
	LLPanel::reshape(width, height, called_from_parent);
	if(!mActiveTab)
		return;
	
	static LLSideTray::Params sidetray_params(LLUICtrlFactory::getDefaultParams<LLSideTray>());	

	setPanelRect();

	LLRect ctrl_rect;
	ctrl_rect.setLeftTopAndSize(0
							,getRect().getHeight()-sidetray_params.default_button_width
							,sidetray_params.default_button_width
							,sidetray_params.default_button_height);
	
	mCollapseButton->setRect(ctrl_rect);

	//arrange tab buttons
	child_vector_const_iter_t child_it;
	int offset = (sidetray_params.default_button_height+sidetray_params.default_button_margin)*2;
	for ( child_it = mTabs.begin(); child_it != mTabs.end(); ++child_it)	
	{
		LLSideTrayTab* sidebar_tab = dynamic_cast<LLSideTrayTab*>(*child_it);
		if(sidebar_tab == NULL)
			continue;
		
		ctrl_rect.setLeftTopAndSize(0,getRect().getHeight()-offset
								,sidetray_params.default_button_width
								,sidetray_params.default_button_height);

		if(mTabButtons.find(sidebar_tab->getName()) == mTabButtons.end())
			continue;

		LLButton* btn = mTabButtons[sidebar_tab->getName()];

		btn->setRect(ctrl_rect);
		offset+=sidetray_params.default_button_height;
		offset+=sidetray_params.default_button_margin;

		btn->setVisible(ctrl_rect.mBottom > 0);
	}

	//arrange tabs
	
	for ( child_it = mTabs.begin(); child_it != mTabs.end(); ++child_it)
	{
		LLSideTrayTab* sidebar_tab = dynamic_cast<LLSideTrayTab*>(*child_it);
		if(sidebar_tab == NULL)
			continue;
		sidebar_tab->reshape(mMaxBarWidth,getRect().getHeight());
		ctrl_rect.setLeftTopAndSize(sidetray_params.default_button_width,getRect().getHeight(),mMaxBarWidth,getRect().getHeight());
		sidebar_tab->setRect(ctrl_rect);
		
	}
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

// *TODO: Eliminate magic constants.
static const S32	fake_offset = 132;
static const S32	fake_top_offset = 18;

void LLSideTray::resetPanelRect	()
{
	const LLRect& parent_rect = gViewerWindow->getRootView()->getRect();

	static LLSideTray::Params sidetray_params(LLUICtrlFactory::getDefaultParams<LLSideTray>());	

	S32 panel_width = sidetray_params.default_button_width;
	panel_width += mCollapsed ? 0 : mMaxBarWidth;

	S32 panel_height = parent_rect.getHeight()-fake_top_offset;

	reshape(panel_width,panel_height);
}

void	LLSideTray::setPanelRect	()
{
	LLNavigationBar* nav_bar = LLNavigationBar::getInstance();
	LLRect nav_rect = nav_bar->getRect();
	
	static LLSideTray::Params sidetray_params(LLUICtrlFactory::getDefaultParams<LLSideTray>());	

	const LLRect& parent_rect = gViewerWindow->getRootView()->getRect();

	S32 panel_width = sidetray_params.default_button_width;
	panel_width += mCollapsed ? 0 : mMaxBarWidth;

	S32 panel_height = parent_rect.getHeight()-fake_top_offset - nav_rect.getHeight();
	S32 panel_top = parent_rect.mTop-fake_top_offset - nav_rect.getHeight();

	LLRect panel_rect;
	panel_rect.setLeftTopAndSize( parent_rect.mRight-panel_width, panel_top, panel_width, panel_height);
	setRect(panel_rect);
}

S32	LLSideTray::getTrayWidth()
{
	static LLSideTray::Params sidetray_params(LLUICtrlFactory::getDefaultParams<LLSideTray>());	
	return getRect().getWidth() - (sidetray_params.default_button_width + sidetray_params.default_button_margin);
}
