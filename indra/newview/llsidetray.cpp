/** 
 * @file llsidetray.cpp
 * @brief SideBar implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "lltextbox.h"

#include "llagentcamera.h"
#include "llappviewer.h"
#include "llbottomtray.h"
#include "llfloaterreg.h"
#include "llfirstuse.h"
#include "llhints.h"
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
#include "llscreenchannel.h"
#include "llchannelmanager.h"
#include "llwindow.h"//for SetCursor
#include "lltransientfloatermgr.h"

#include "llsidepanelappearance.h"

//#include "llscrollcontainer.h"

using namespace std;
using namespace LLNotificationsUI;

static LLRootViewRegistry::Register<LLSideTray>	t1("side_tray");
static LLDefaultChildRegistry::Register<LLSideTrayTab>	t2("sidetray_tab");

static const S32 BOTTOM_BAR_PAD = 5;

static const std::string COLLAPSED_NAME = "<<";
static const std::string EXPANDED_NAME  = ">>";

static const std::string TAB_PANEL_CAPTION_NAME = "sidetray_tab_panel";
static const std::string TAB_PANEL_CAPTION_TITLE_BOX = "sidetray_tab_title";

LLSideTray* LLSideTray::sInstance = 0;

// static
LLSideTray* LLSideTray::getInstance()
{
	if (!sInstance)
	{
		sInstance = LLUICtrlFactory::createFromFile<LLSideTray>("panel_side_tray.xml",NULL, LLRootView::child_registry_t::instance());
		sInstance->setXMLFilename("panel_side_tray.xml");
	}

	return sInstance;
}

// static
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
	LOG_CLASS(LLSideTrayTab);
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
	
	void			dock(LLFloater* floater_tab);
	void			undock(LLFloater* floater_tab);

	LLSideTray*		getSideTray();

	void			onFloaterClose(LLSD::Boolean app_quitting);
	
public:
	virtual ~LLSideTrayTab();
	
    /*virtual*/ BOOL	postBuild	();
	/*virtual*/ bool	addChild	(LLView* view, S32 tab_group);
	
	
	void			reshape		(S32 width, S32 height, BOOL called_from_parent = TRUE);
	
	static LLSideTrayTab*  createInstance	();
	
	const std::string& getDescription () const { return mDescription;}
	const std::string& getTabTitle() const { return mTabTitle;}
	
	void			onOpen		(const LLSD& key);
	
	void			toggleTabDocked();
	void			setDocked(bool dock);
	bool			isDocked() const;

	BOOL			handleScrollWheel(S32 x, S32 y, S32 clicks);

	LLPanel *getPanel();
private:
	std::string mTabTitle;
	std::string mImage;
	std::string mImageSelected;
	std::string	mDescription;
	
	LLView*	mMainPanel;
	boost::signals2::connection mFloaterCloseConn;
};

LLSideTrayTab::LLSideTrayTab(const Params& p)
:	LLPanel(),
	mTabTitle(p.tab_title),
	mImage(p.image),
	mImageSelected(p.image_selected),
	mDescription(p.description),
	mMainPanel(NULL)
{
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

	getChild<LLButton>("undock")->setCommitCallback(boost::bind(&LLSideTrayTab::toggleTabDocked, this));
	getChild<LLButton>("dock")->setCommitCallback(boost::bind(&LLSideTrayTab::toggleTabDocked, this));

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

// Attempts to get the existing side tray instance.
// Needed to avoid recursive calls of LLSideTray::getInstance().
LLSideTray* LLSideTrayTab::getSideTray()
{
	// First, check if the side tray is our parent (i.e. we're attached).
	LLSideTray* side_tray = dynamic_cast<LLSideTray*>(getParent());
	if (!side_tray)
	{
		// Detached? Ok, check if the instance exists at all/
		if (LLSideTray::instanceCreated())
		{
			side_tray = LLSideTray::getInstance();
		}
		else
		{
			llerrs << "No safe way to get the side tray instance" << llendl;
		}
	}

	return side_tray;
}

void LLSideTrayTab::toggleTabDocked()
{
	std::string tab_name = getName();

	LLFloater* floater_tab = LLFloaterReg::getInstance("side_bar_tab", tab_name);
	if (!floater_tab) return;

	bool docking = LLFloater::isShown(floater_tab);

	// Hide the "Tear Off" button when a tab gets undocked
	// and show "Dock" button instead.
	getChild<LLButton>("undock")->setVisible(docking);
	getChild<LLButton>("dock")->setVisible(!docking);

	if (docking)
	{
		dock(floater_tab);
	}
	else
	{
		undock(floater_tab);
	}

	// Open/close the floater *after* we reparent the tab panel,
	// so that it doesn't receive redundant visibility change notifications.
	LLFloaterReg::toggleInstance("side_bar_tab", tab_name);
}

// Same as toggleTabDocked() apart from making sure that we do exactly what we want.
void LLSideTrayTab::setDocked(bool dock)
{
	if (isDocked() == dock)
	{
		llwarns << "Tab " << getName() << " is already " << (dock ? "docked" : "undocked") << llendl;
		return;
	}

	toggleTabDocked();
}

bool LLSideTrayTab::isDocked() const
{
	return dynamic_cast<LLSideTray*>(getParent()) != NULL;
}

void LLSideTrayTab::onFloaterClose(LLSD::Boolean app_quitting)
{
	// If user presses Ctrl-Shift-W, handle that gracefully by docking all
	// undocked tabs before their floaters get destroyed (STORM-1016).

	// Don't dock on quit for the current dock state to be correctly saved.
	if (app_quitting) return;

	lldebugs << "Forcibly docking tab " << getName() << llendl;
	setDocked(true);
}

BOOL LLSideTrayTab::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	// Let children handle the event
	LLUICtrl::handleScrollWheel(x, y, clicks);

	// and then eat it to prevent in-world scrolling (STORM-351).
	return TRUE;
}

void LLSideTrayTab::dock(LLFloater* floater_tab)
{
	LLSideTray* side_tray = getSideTray();
	if (!side_tray) return;

	// Before docking the tab, reset its (and its children's) transparency to default (STORM-688).
	floater_tab->updateTransparency(TT_DEFAULT);

	if (!side_tray->addTab(this))
	{
		llwarns << "Failed to add tab " << getName() << " to side tray" << llendl;
		return;
	}

	mFloaterCloseConn.disconnect();
	setRect(side_tray->getLocalRect());
	reshape(getRect().getWidth(), getRect().getHeight());

	// Select the re-docked tab.
	side_tray->selectTabByName(getName());

	if (side_tray->getCollapsed())
	{
		side_tray->expandSideBar(false);
	}
}

static void on_minimize(LLSidepanelAppearance* panel, LLSD minimized)
{
	if (!panel) return;
	bool visible = !minimized.asBoolean();
	LLSD visibility;
	visibility["visible"] = visible;
	// Do not reset accordion state on minimize (STORM-375)
	visibility["reset_accordion"] = false;
	panel->updateToVisibility(visibility);
}

void LLSideTrayTab::undock(LLFloater* floater_tab)
{
	LLSideTray* side_tray = getSideTray();
	if (!side_tray) return;

	// Remember whether the tab have been active before detaching
	// because removeTab() will change active tab.
	bool was_active = side_tray->getActiveTab() == this;

	// Remove the tab from Side Tray's tabs list.
	// We have to do it despite removing the tab from Side Tray's child view tree
	// by addChild(). Otherwise the tab could be accessed by the pointer in LLSideTray::mTabs.
	if (!side_tray->removeTab(this))
	{
		llwarns << "Failed to remove tab " << getName() << " from side tray" << llendl;
		return;
	}

	// If we're undocking while side tray is collapsed we need to explicitly show the panel.
	if (!getVisible())
	{
		setVisible(true);
	}

	floater_tab->addChild(this);
	mFloaterCloseConn = floater_tab->setCloseCallback(boost::bind(&LLSideTrayTab::onFloaterClose, this, _2));
	floater_tab->setTitle(mTabTitle);
	floater_tab->setName(getName());

	// Resize handles get obscured by added panel so move them to front.
	floater_tab->moveResizeHandlesToFront();

	// Reshape the floater if needed.
	LLRect floater_rect;
	if (floater_tab->hasSavedRect())
	{
		// We've got saved rect for the floater, hence no need to reshape it.
		floater_rect = floater_tab->getLocalRect();
	}
	else
	{
		// Detaching for the first time. Reshape the floater.
		floater_rect = side_tray->getLocalRect();

		// Reduce detached floater height by small BOTTOM_BAR_PAD not to make it flush with the bottom bar.
		floater_rect.mBottom += LLBottomTray::getInstance()->getRect().getHeight() + BOTTOM_BAR_PAD;
		floater_rect.makeValid();
		floater_tab->reshape(floater_rect.getWidth(), floater_rect.getHeight());
	}

	// Reshape the panel.
	{
		LLRect panel_rect = floater_tab->getLocalRect();
		panel_rect.mTop -= floater_tab->getHeaderHeight();
		panel_rect.makeValid();
		setRect(panel_rect);
		reshape(panel_rect.getWidth(), panel_rect.getHeight());
	}

	// Set FOLLOWS_ALL flag for the tab to follow floater dimensions upon resizing.
	setFollowsAll();

	// Camera view may need to be changed for appearance panel(STORM-301) on minimize of floater,
	// so setting callback here. 
	if (getName() == "sidebar_appearance")
	{
		LLSidepanelAppearance* panel_appearance = dynamic_cast<LLSidepanelAppearance*>(getPanel());
		if(panel_appearance)
		{
			floater_tab->setMinimizeCallback(boost::bind(&on_minimize, panel_appearance, _2));
		}
	}

	if (!side_tray->getCollapsed())
	{
		side_tray->collapseSideBar();
	}

	if (!was_active)
	{
		// When a tab other then current active tab is detached from Side Tray
		// onOpen() should be called as tab visibility is changed.
		onOpen(LLSD());
	}
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
// LLSideTrayButton
// Side Tray tab button with "tear off" handling.
//////////////////////////////////////////////////////////////////////////////

class LLSideTrayButton : public LLButton
{
public:
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask)
	{
		// Route future Mouse messages here preemptively.  (Release on mouse up.)
		// No handler needed for focus lost since this class has no state that depends on it.
		gFocusMgr.setMouseCapture(this);

		localPointToScreen(x, y, &mDragLastScreenX, &mDragLastScreenY);

		// Note: don't pass on to children
		return TRUE;
	}

	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask)
	{
		// We only handle the click if the click both started and ended within us
		if( !hasMouseCapture() ) return FALSE;

		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y);

		S32 delta_x = screen_x - mDragLastScreenX;
		S32 delta_y = screen_y - mDragLastScreenY;

		LLSideTray* side_tray = LLSideTray::getInstance();

		// Check if the tab we are dragging is docked.
		if (!side_tray->isTabAttached(getName())) return FALSE;

		// Same value is hardcoded in LLDragHandle::handleHover().
		const S32 undock_threshold = 12;

		// Detach a tab if it has been pulled further than undock_threshold.
		if (delta_x <= -undock_threshold ||	delta_x >= undock_threshold	||
			delta_y <= -undock_threshold ||	delta_y >= undock_threshold)
		{
			LLSideTrayTab* tab = side_tray->getTab(getName());
			if (!tab) return FALSE;

			tab->toggleTabDocked();

			LLFloater* floater_tab = LLFloaterReg::getInstance("side_bar_tab", tab->getName());
			if (!floater_tab) return FALSE;

			LLRect original_rect = floater_tab->getRect();
			S32 header_snap_y = floater_tab->getHeaderHeight() / 2;
			S32 snap_x = screen_x - original_rect.mLeft - original_rect.getWidth() / 2;
			S32 snap_y = screen_y - original_rect.mTop + header_snap_y;

			// Move the floater to appear "under" the mouse pointer.
			floater_tab->setRect(original_rect.translate(snap_x, snap_y));

			// Snap the mouse pointer to the center of the floater header
			// and call 'mouse down' event handler to begin dragging.
			floater_tab->handleMouseDown(original_rect.getWidth() / 2,
										 original_rect.getHeight() - header_snap_y,
										 mask);

			return TRUE;
		}

		return FALSE;
	}

protected:
	LLSideTrayButton(const LLButton::Params& p)
	: LLButton(p)
	, mDragLastScreenX(0)
	, mDragLastScreenY(0)
	{}

	friend class LLUICtrlFactory;

private:
	S32		mDragLastScreenX;
	S32		mDragLastScreenY;
};

//////////////////////////////////////////////////////////////////////////////
// LLSideTray
//////////////////////////////////////////////////////////////////////////////

LLSideTray::Params::Params()
:	collapsed("collapsed",false),
	tab_btn_image_normal("tab_btn_image",LLUI::getUIImage("taskpanel/TaskPanel_Tab_Off.png")),
	tab_btn_image_selected("tab_btn_image_selected",LLUI::getUIImage("taskpanel/TaskPanel_Tab_Selected.png")),
	default_button_width("tab_btn_width",32),
	default_button_height("tab_btn_height",32),
	default_button_margin("tab_btn_margin",0)
{}

//virtual 
LLSideTray::LLSideTray(const Params& params)
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
	LLView* side_bar_tabs  = gViewerWindow->getRootView()->getChildView("side_bar_tabs");
	if (side_bar_tabs != NULL)
	{
		LLTransientFloaterMgr::getInstance()->addControlView(side_bar_tabs);
	}

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

	LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLSideTray::handleLoginComplete, this));

	// Remember original tabs order, so that we can restore it if user detaches and then re-attaches a tab.
	for (child_vector_const_iter_t it = mTabs.begin(); it != mTabs.end(); ++it)
	{
		std::string tab_name = (*it)->getName();
		mOriginalTabOrder.push_back(tab_name);
	}

	//EXT-8045
	//connect all already created channels to reflect sidetray collapse/expand
	std::vector<LLChannelManager::ChannelElem>& channels = LLChannelManager::getInstance()->getChannelList();
	for(std::vector<LLChannelManager::ChannelElem>::iterator it = channels.begin();it!=channels.end();++it)
	{
		if ((*it).channel)
		{
			setVisibleWidthChangeCallback(boost::bind(&LLScreenChannelBase::resetPositionAndSize, (*it).channel));
		}
	}

	return true;
}

void LLSideTray::handleLoginComplete()
{
	//reset tab to "home" tab if it was changesd during login process
	selectTabByName("sidebar_home");

	detachTabs();
}

LLSideTrayTab* LLSideTray::getTab(const std::string& name)
{
	return findChild<LLSideTrayTab>(name,false);
}

bool LLSideTray::isTabAttached(const std::string& name)
{
	LLSideTrayTab* tab = getTab(name);
	if (!tab) return false;

	return std::find(mTabs.begin(), mTabs.end(), tab) != mTabs.end();
}

bool LLSideTray::hasTabs()
{
	// The open/close tab doesn't count.
	return mTabs.size() > 1;
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
		// Only highlight the tab if side tray is expanded (STORM-157).
		btn->setImageOverlay( new_state && !getCollapsed() ? tab->mImageSelected : tab->mImage );
	}
}

LLPanel* LLSideTray::openChildPanel(LLSideTrayTab* tab, const std::string& panel_name, const LLSD& params)
{
	LLView* view = tab->findChildView(panel_name, true);
	if (!view) return NULL;

	std::string tab_name = tab->getName();

	bool tab_attached = isTabAttached(tab_name);

	if (tab_attached && LLUI::sSettingGroups["config"]->getBOOL("OpenSidePanelsInFloaters"))
	{
		tab->toggleTabDocked();
		tab_attached = false;
	}

	// Select tab and expand Side Tray only when a tab is attached.
	if (tab_attached)
	{
		selectTabByName(tab_name);
		if (mCollapsed)
			expandSideBar();
	}
	else
	{
		LLFloater* floater_tab = LLFloaterReg::getInstance("side_bar_tab", tab_name);
		if (!floater_tab) return NULL;

		floater_tab->openFloater(panel_name);
	}

	LLSideTrayPanelContainer* container = dynamic_cast<LLSideTrayPanelContainer*>(view->getParent());
	if (container)
	{
		LLSD new_params = params;
		new_params[LLSideTrayPanelContainer::PARAM_SUB_PANEL_NAME] = panel_name;
		container->onOpen(new_params);

		return container->getCurrentPanel();
	}

	LLPanel* panel = dynamic_cast<LLPanel*>(view);
	if (panel)
	{
		panel->onOpen(params);
	}

	return panel;
}

bool LLSideTray::selectTabByIndex(size_t index)
{
	if(index>=mTabs.size())
		return false;

	LLSideTrayTab* sidebar_tab = mTabs[index];
	return selectTabByName(sidebar_tab->getName());
}

bool LLSideTray::selectTabByName(const std::string& name, bool keep_prev_visible)
{
	LLSideTrayTab* tab_to_keep_visible = NULL;
	LLSideTrayTab* new_tab = getTab(name);
	if (!new_tab) return false;

	// Bail out if already selected.
	if (new_tab == mActiveTab)
		return false;

	//deselect old tab
	if (mActiveTab)
	{
		// Keep previously active tab visible if requested.
		if (keep_prev_visible) tab_to_keep_visible = mActiveTab;
	toggleTabButton(mActiveTab);
	}

	//select new tab
	mActiveTab = new_tab;

	if (mActiveTab)
	{
	toggleTabButton(mActiveTab);
	LLSD key;//empty
	mActiveTab->onOpen(key);
	}

	//arrange();
	
	//hide all tabs - show active tab
	child_vector_const_iter_t child_it;
	for ( child_it = mTabs.begin(); child_it != mTabs.end(); ++child_it)
	{
		LLSideTrayTab* sidebar_tab = *child_it;

		bool vis = sidebar_tab == mActiveTab;

		// Force keeping the tab visible if requested.
		vis |= sidebar_tab == tab_to_keep_visible;

		// When the last tab gets detached, for a short moment the "Toggle Sidebar" pseudo-tab
		// is shown. So, to avoid the flicker we make sure it never gets visible.
		vis &= (*child_it)->getName() != "sidebar_openclose";

		sidebar_tab->setVisible(vis);
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
	bparams.image_unselected(sidetray_params.tab_btn_image_normal);
	bparams.image_selected(sidetray_params.tab_btn_image_selected);
	bparams.image_disabled(sidetray_params.tab_btn_image_normal);
	bparams.image_disabled_selected(sidetray_params.tab_btn_image_selected);

	LLButton* button;
	if (name == "sidebar_openclose")
	{
		// "Open/Close" button shouldn't allow "tear off"
		// hence it is created as LLButton instance.
		button = LLUICtrlFactory::create<LLButton>(bparams);
	}
	else
	{
		button = LLUICtrlFactory::create<LLSideTrayButton>(bparams);
	}

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

bool LLSideTray::removeTab(LLSideTrayTab* tab)
{
	if (!tab) return false;
	std::string tab_name = tab->getName();

	// Look up the tab in the list of known tabs.
	child_vector_iter_t tab_it = std::find(mTabs.begin(), mTabs.end(), tab);
	if (tab_it == mTabs.end())
	{
		llwarns << "Cannot find tab named " << tab_name << llendl;
		return false;
	}

	// Find the button corresponding to the tab.
	button_map_t::iterator btn_it = mTabButtons.find(tab_name);
	if (btn_it == mTabButtons.end())
	{
		llwarns << "Cannot find button for tab named " << tab_name << llendl;
		return false;
	}
	LLButton* btn = btn_it->second;

	// Deselect the tab.
	if (mActiveTab == tab)
	{
		// Select the next tab (or first one, if we're removing the last tab),
		// skipping the fake open/close tab (STORM-155).
		child_vector_iter_t next_tab_it = tab_it;
		do
		{
			next_tab_it = (next_tab_it < (mTabs.end() - 1)) ? next_tab_it + 1 : mTabs.begin();
		}
		while ((*next_tab_it)->getName() == "sidebar_openclose");

		selectTabByName((*next_tab_it)->getName(), true); // Don't hide the tab being removed.
	}

	// Remove the tab.
	removeChild(tab);
	mTabs.erase(tab_it);

	// Add the tab to detached tabs list.
	mDetachedTabs.push_back(tab);

	// Remove the button from the buttons panel so that it isn't drawn anymore.
	mButtonsPanel->removeChild(btn);

	// Re-arrange remaining tabs.
	arrange();

	return true;
}

bool LLSideTray::addTab(LLSideTrayTab* tab)
{
	if (tab == NULL) return false;

	std::string tab_name = tab->getName();

	// Make sure the tab isn't already in the list.
	if (std::find(mTabs.begin(), mTabs.end(), tab) != mTabs.end())
	{
		llwarns << "Attempt to re-add existing tab " << tab_name << llendl;
		return false;
	}

	// Look up the corresponding button.
	button_map_t::const_iterator btn_it = mTabButtons.find(tab_name);
	if (btn_it == mTabButtons.end())
	{
		llwarns << "Tab " << tab_name << " has no associated button" << llendl;
		return false;
	}
	LLButton* btn = btn_it->second;

	// Insert the tab at its original position.
	LLUICtrl::addChild(tab);
	{
		tab_order_vector_const_iter_t new_tab_orig_pos =
			std::find(mOriginalTabOrder.begin(), mOriginalTabOrder.end(), tab_name);
		llassert(new_tab_orig_pos != mOriginalTabOrder.end());
		child_vector_iter_t insert_pos = mTabs.end();

		for (child_vector_iter_t tab_it = mTabs.begin(); tab_it != mTabs.end(); ++tab_it)
		{
			tab_order_vector_const_iter_t cur_tab_orig_pos =
				std::find(mOriginalTabOrder.begin(), mOriginalTabOrder.end(), (*tab_it)->getName());
			llassert(cur_tab_orig_pos != mOriginalTabOrder.end());

			if (new_tab_orig_pos < cur_tab_orig_pos)
			{
				insert_pos = tab_it;
				break;
			}
		}

		mTabs.insert(insert_pos, tab);
	}

	// Add the button to the buttons panel so that it's drawn again.
	mButtonsPanel->addChildInBack(btn);

	// Arrange tabs after inserting a new one.
	arrange();

	// Remove the tab from the list of detached tabs.
	child_vector_iter_t tab_it = std::find(mDetachedTabs.begin(), mDetachedTabs.end(), tab);
	if (tab_it != mDetachedTabs.end())
	{
		mDetachedTabs.erase(tab_it);
	}

	return true;
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
			mCollapseButton = createButton(name,sidebar_tab->mImage,sidebar_tab->getTabTitle(),
				boost::bind(&LLSideTray::onToggleCollapse, this));
			LLHints::registerHintTarget("side_panel_btn", mCollapseButton->getHandle());
		}
		else
		{
			LLButton* button = createButton(name,sidebar_tab->mImage,sidebar_tab->getTabTitle(),
				boost::bind(&LLSideTray::onTabButtonClick, this, name));
			mTabButtons[name] = button;
		}
	}
	LLHints::registerHintTarget("inventory_btn", mTabButtons["sidebar_inventory"]->getHandle());
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
	LLSideTrayTab* tab = getTab(name);
	if (!tab) return;

	if(tab == mActiveTab)
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
	LLFirstUse::notUsingSidePanel(false);
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

	setFocus(!mCollapsed);

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

	// The tab buttons should be shown only if there is at least one non-detached tab.
	// Also hide them in mouse-look mode.
	mButtonsPanel->setVisible(hasTabs() && !gAgentCamera.cameraMouselook());
}

// Detach those tabs that were detached when the viewer exited last time.
void LLSideTray::detachTabs()
{
	// copy mTabs because LLSideTray::toggleTabDocked() modifies it.
	child_vector_t tabs = mTabs;

	for (child_vector_const_iter_t it = tabs.begin(); it != tabs.end(); ++it)
	{
		LLSideTrayTab* tab = *it;

		std::string floater_ctrl_name = LLFloater::getControlName("side_bar_tab", LLSD(tab->getName()));
		std::string vis_ctrl_name = LLFloaterReg::getVisibilityControlName(floater_ctrl_name);
		if (!LLFloater::getControlGroup()->controlExists(vis_ctrl_name)) continue;

		bool is_visible = LLFloater::getControlGroup()->getBOOL(vis_ctrl_name);
		if (!is_visible) continue;

		llassert(isTabAttached(tab->getName()));
		tab->toggleTabDocked();
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

void LLSideTray::expandSideBar(bool open_active)
{
	mCollapsed = false;
	LLSideTrayTab* openclose_tab = getTab("sidebar_openclose");
	if (openclose_tab)
	{
		mCollapseButton->setImageOverlay( openclose_tab->mImageSelected );
	}

	if (open_active)
	{
		mActiveTab->onOpen(LLSD());
	}

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
	LLPanel* new_panel = NULL;

	// Look up the tab in the list of detached tabs.
	child_vector_const_iter_t child_it;
	for ( child_it = mDetachedTabs.begin(); child_it != mDetachedTabs.end(); ++child_it)
	{
		new_panel = openChildPanel(*child_it, panel_name, params);
		if (new_panel) break;
	}

	// Look up the tab in the list of attached tabs.
	for ( child_it = mTabs.begin(); child_it != mTabs.end(); ++child_it)
	{
		new_panel = openChildPanel(*child_it, panel_name, params);
		if (new_panel) break;
	}

	return new_panel;
}

void LLSideTray::hidePanel(const std::string& panel_name)
{
	LLPanel* panelp = getPanel(panel_name);
	if (panelp)
	{
		if(isTabAttached(panel_name))
		{
			collapseSideBar();
		}
		else
		{
			LLFloaterReg::hideInstance("side_bar_tab", panel_name);
		}
	}
}


void LLSideTray::togglePanel(LLPanel* &sub_panel, const std::string& panel_name, const LLSD& params)
{
	if(!sub_panel)
		return;

	// If a panel is visible and attached to Side Tray (has LLSideTray among its ancestors)
	// it should be toggled off by collapsing Side Tray.
	if (sub_panel->isInVisibleChain() && sub_panel->hasAncestor(this))
	{
		LLSideTray::getInstance()->collapseSideBar();
	}
	else
	{
		LLSideTray::getInstance()->showPanel(panel_name, params);
	}
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
	// Look up the panel in the list of detached tabs.
	for ( child_vector_const_iter_t child_it = mDetachedTabs.begin(); child_it != mDetachedTabs.end(); ++child_it)
	{
		LLPanel *panel = findChildPanel(*child_it,panel_name,true);
		if(panel)
		{
			return panel;
		}
	}

	// Look up the panel in the list of attached tabs.
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

void LLSideTray::setTabDocked(const std::string& tab_name, bool dock)
{
	LLSideTrayTab* tab = getTab(tab_name);
	if (!tab)
	{	// not a docked tab, look through detached tabs
		for(child_vector_iter_t tab_it = mDetachedTabs.begin(), tab_end_it = mDetachedTabs.end();
			tab_it != tab_end_it;
			++tab_it)
		{
			if ((*tab_it)->getName() == tab_name)
			{
				tab = *tab_it;
				break;
			}
		}

	}

	if (tab)
	{
		bool tab_attached = isTabAttached(tab_name);
		LLFloater* floater_tab = LLFloaterReg::getInstance("side_bar_tab", tab_name);
		if (!floater_tab) return;

		if (dock && !tab_attached)
		{
			tab->dock(floater_tab);
		}
		else if (!dock && tab_attached)
		{
			tab->undock(floater_tab);
		}
	}
}


void	LLSideTray::updateSidetrayVisibility()
{
	// set visibility of parent container based on collapsed state
	LLView* parent = getParent();
	if (parent)
	{
		bool old_visibility = parent->getVisible();
		bool new_visibility = !mCollapsed && !gAgentCamera.cameraMouselook();

		if (old_visibility != new_visibility)
		{
			parent->setVisible(new_visibility);

			// Signal change of visible width.
			llinfos << "Visible: " << new_visibility << llendl;
			mVisibleWidthChangeSignal(this, new_visibility);
		}
	}
}

S32 LLSideTray::getVisibleWidth()
{
	return (isInVisibleChain() && !mCollapsed) ? getRect().getWidth() : 0;
}

void LLSideTray::setVisibleWidthChangeCallback(const commit_signal_t::slot_type& cb)
{
	mVisibleWidthChangeSignal.connect(cb);
}
