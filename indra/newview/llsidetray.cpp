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

#include "llsidetray.h"
#include "llviewerwindow.h"
#include "llaccordionctrl.h"
#include "llfocusmgr.h"
#include "llrootview.h"

#include "llaccordionctrltab.h"

#include "llfloater.h" //for gFloaterView
#include "lliconctrl.h"//for Home tab icon
#include "llsidetraypanelcontainer.h"
#include "llwindow.h"//for SetCursor

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

LLSideTrayTab::LLSideTrayTab(const Params& params):mMainPanel(0)
												
{
	mImagePath = params.image_path;
	mTabTitle = params.tab_title;
	mDescription = params.description;
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

	static LLUIColor default_background_color = LLUIColorTable::instance().getColor("FloaterDefaultBackgroundColor");
	static LLUIColor focus_background_color = LLUIColorTable::instance().getColor("FloaterFocusBackgroundColor");
	
	setTransparentColor(default_background_color);
	setBackgroundColor(focus_background_color);
	
	return true;
}

static const S32 splitter_margin = 1;

//virtual 
void	LLSideTrayTab::arrange(S32 width, S32 height )
{
	if(!mMainPanel)
		return;
	
	S32 offset = 0;

	LLView* title_panel = getChildView(TAB_PANEL_CAPTION_NAME, true, false);

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

	LLView* title_panel = getChildView(TAB_PANEL_CAPTION_NAME, true, false);

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

	//border
	gl_rect_2d(0,0,getRect().getWidth() - 1,getRect().getHeight() - 1,LLColor4::black,false);


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


//virtual 
LLSideTray::LLSideTray(Params& params)
	   : LLPanel(params)
	    ,mActiveTab(0)
		,mCollapsed(false)
		,mCollapseButton(0)
	    ,mMaxBarWidth(params.rect.width)
{
	mCollapsed=params.collapsed;
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
    
/**
 * add new panel to tab with tab_name name
 * @param tab_name - name of sidebar tab to add new panel
 * @param panel - pointer to panel 
 */
bool        LLSideTray::addPanel        ( const std::string& tab_name
										,LLPanel* panel )
{
	return false;
}
/**
 * Add new tab to side bar
 * @param tab_name - name of the new tab
 * @param image - image for new sidebar button
 * @param title -  title for new tab
 */
bool        LLSideTray::addTab          ( const std::string& tab_name
										,const std::string& image
										,const std::string& title)
{
	LLSideTrayTab::Params params;
	params.image_path = image;
	params.tab_title = title;
	LLSideTrayTab* tab = LLUICtrlFactory::create<LLSideTrayTab> (params);
	addChild(tab,1);
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
	//create show/hide button
	mCollapseButton = createButton(EXPANDED_NAME,"",boost::bind(&LLSideTray::onToggleCollapse, this));

	//create buttons for tabs
	child_vector_const_iter_t child_it = mTabs.begin();
	++child_it;

	for ( ; child_it != mTabs.end(); ++child_it)
	{
		LLSideTrayTab* sidebar_tab = dynamic_cast<LLSideTrayTab*>(*child_it);
		if(sidebar_tab == NULL)
			continue;

		string name = sidebar_tab->getName();
		
		LLButton* button = createButton("",sidebar_tab->mImagePath,boost::bind(&LLSideTray::onTabButtonClick, this, sidebar_tab->getName()));
		mTabButtons[sidebar_tab->getName()] = button;
	}
	
}

void		LLSideTray::onTabButtonClick(string name)
{
	
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
		gFloaterView->setSnapOffsetRight(0);
	else
		gFloaterView->setSnapOffsetRight(mMaxBarWidth);

	gFloaterView->refresh();

	setFocus( FALSE );
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

void LLSideTray::collapseSideBar	()
{
	mCollapsed = true;
	mCollapseButton->setLabel(COLLAPSED_NAME);
	mActiveTab->setVisible(FALSE);
	reflectCollapseChange();
	setFocus( FALSE );

}
void LLSideTray::expandSideBar	()
{
	mCollapsed = false;
	mCollapseButton->setLabel(EXPANDED_NAME);
	LLSD key;//empty
	mActiveTab->onOpen(key);
	mActiveTab->setVisible(TRUE);

	reflectCollapseChange();

}

void LLSideTray::highlightFocused()
{
	if(!mActiveTab)
		return;
	/* uncomment in case something change
	BOOL dependent_has_focus = gFocusMgr.childHasKeyboardFocus(this);
	setBackgroundOpaque( dependent_has_focus ); 
	mActiveTab->setBackgroundOpaque( dependent_has_focus ); 
	*/
	mActiveTab->setBackgroundOpaque( true ); 

	
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
		LLView* view = (*child_it)->getChildView(panel_name,true,false);
		if(view)
		{
			onTabButtonClick((*child_it)->getName());

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

static const S32	fake_offset = 132;
static const S32	fake_top_offset = 78;

void	LLSideTray::setPanelRect	()
{
	static LLSideTray::Params sidetray_params(LLUICtrlFactory::getDefaultParams<LLSideTray>());	

	const LLRect& parent_rect = gViewerWindow->getRootView()->getRect();

	S32 panel_width = sidetray_params.default_button_width+sidetray_params.default_button_margin;
	if(!mCollapsed)
		panel_width+=mMaxBarWidth;

	S32 panel_height = parent_rect.getHeight()-fake_top_offset;
	LLRect panel_rect;
	panel_rect.setLeftTopAndSize( parent_rect.mRight-panel_width, parent_rect.mTop-fake_top_offset, panel_width, panel_height);
	setRect(panel_rect);
}

S32	LLSideTray::getTrayWidth()
{
	static LLSideTray::Params sidetray_params(LLUICtrlFactory::getDefaultParams<LLSideTray>());	
	return getRect().getWidth() - (sidetray_params.default_button_width + sidetray_params.default_button_margin);
}
