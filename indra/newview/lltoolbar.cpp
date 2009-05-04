/** 
 * @file lltoolbar.cpp
 * @author James Cook, Richard Nelson
 * @brief Large friendly buttons at bottom of screen.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "lltoolbar.h"

#include "imageids.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llparcel.h"

#include "llagent.h"
#include "llbutton.h"
#include "llfocusmgr.h"
#include "llviewercontrol.h"
#include "llmenucommands.h"
#include "llimview.h"
#include "lluiconstants.h"
#include "llvoavatar.h"
#include "lltooldraganddrop.h"
#include "llinventoryview.h"
#include "llfloaterchatterbox.h"
#include "llfloaterfriends.h"
#include "llfloatersnapshot.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llviewermenu.h"
#include "llfirstuse.h"
#include "llviewerparcelmgr.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "lltoolgrab.h"
#include "llcombobox.h"
#include "llfloaterchat.h"
#include "llfloatermute.h"
#include "llimpanel.h"
#include "llscrolllistctrl.h"

#if LL_DARWIN

	#include "llresizehandle.h"

	// This class draws like an LLResizeHandle but has no interactivity.
	// It's just there to provide a cue to the user that the lower right corner of the window functions as a resize handle.
	class LLFakeResizeHandle : public LLResizeHandle
	{
	public:
		LLFakeResizeHandle(const std::string& name, const LLRect& rect, S32 min_width, S32 min_height, ECorner corner = RIGHT_BOTTOM )
		: LLResizeHandle(name, rect, min_width, min_height, corner )
		{
			
		}

		virtual BOOL	handleHover(S32 x, S32 y, MASK mask)   { return FALSE; };
		virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask)  { return FALSE; };
		virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask)   { return FALSE; };

	};

#endif // LL_DARWIN

//
// Globals
//

LLToolBar *gToolBar = NULL;
S32 TOOL_BAR_HEIGHT = 20;

//
// Statics
//
F32	LLToolBar::sInventoryAutoOpenTime = 1.f;

//
// Functions
//

LLToolBar::LLToolBar()
:	LLPanel()
#if LL_DARWIN
	, mResizeHandle(NULL)
#endif // LL_DARWIN
{
	setIsChrome(TRUE);
	setFocusRoot(TRUE);
}


BOOL LLToolBar::postBuild()
{
	childSetCommitCallback("communicate_btn", onClickCommunicate, this);
	childSetControlName("communicate_btn", "ShowCommunicate");

	childSetAction("chat_btn", onClickChat, this);
	childSetControlName("chat_btn", "ChatVisible");

	childSetAction("appearance_btn", onClickAppearance, this);
	childSetControlName("appearance_btn", "");

	childSetAction("fly_btn", onClickFly, this);
	childSetControlName("fly_btn", "FlyBtnState");

	childSetAction("sit_btn", onClickSit, this);
	childSetControlName("sit_btn", "SitBtnState");

	childSetAction("snapshot_btn", onClickSnapshot, this);
	childSetControlName("snapshot_btn", "");

	childSetAction("directory_btn", onClickDirectory, this);
	childSetControlName("directory_btn", "ShowDirectory");

	childSetAction("build_btn", onClickBuild, this);
	childSetControlName("build_btn", "BuildBtnState");

	childSetAction("radar_btn", onClickRadar, this);
	childSetControlName("radar_btn", "ShowMiniMap");

	childSetAction("map_btn", onClickMap, this);
	childSetControlName("map_btn", "ShowWorldMap");

	childSetAction("inventory_btn", onClickInventory, this);
	childSetControlName("inventory_btn", "ShowInventory");

	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *view = *child_iter;
		LLButton* buttonp = dynamic_cast<LLButton*>(view);
		if(buttonp)
		{
			buttonp->setSoundFlags(LLView::SILENT);
		}
	}

#if LL_DARWIN
	if(mResizeHandle == NULL)
	{
		LLRect rect(0, 0, RESIZE_HANDLE_WIDTH, RESIZE_HANDLE_HEIGHT);
		mResizeHandle = new LLFakeResizeHandle(std::string(""), rect, RESIZE_HANDLE_WIDTH, RESIZE_HANDLE_HEIGHT);
		this->addChildAtEnd(mResizeHandle);
		LLLayoutStack* toolbar_stack = getChild<LLLayoutStack>("toolbar_stack");
		toolbar_stack->reshape(toolbar_stack->getRect().getWidth() - RESIZE_HANDLE_WIDTH, toolbar_stack->getRect().getHeight());
	}
#endif // LL_DARWIN

	layoutButtons();

	return TRUE;
}

LLToolBar::~LLToolBar()
{
	// LLView destructor cleans up children
}


BOOL LLToolBar::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	LLButton* inventory_btn = getChild<LLButton>("inventory_btn");
	if (!inventory_btn) return FALSE;

	LLInventoryView* active_inventory = LLInventoryView::getActiveInventory();

	if(active_inventory && active_inventory->getVisible())
	{
		mInventoryAutoOpen = FALSE;
	}
	else if (inventory_btn->getRect().pointInRect(x, y))
	{
		if (mInventoryAutoOpen)
		{
			if (!(active_inventory && active_inventory->getVisible()) && 
			mInventoryAutoOpenTimer.getElapsedTimeF32() > sInventoryAutoOpenTime)
			{
				LLInventoryView::showAgentInventory();
			}
		}
		else
		{
			mInventoryAutoOpen = TRUE;
			mInventoryAutoOpenTimer.reset();
		}
	}

	return LLPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
}

// static
void LLToolBar::toggle(void*)
{
	BOOL show = gSavedSettings.getBOOL("ShowToolBar");                      
	gSavedSettings.setBOOL("ShowToolBar", !show);                           
	gToolBar->setVisible(!show);
}


// static
BOOL LLToolBar::visible(void*)
{
	return gToolBar->getVisible();
}


void LLToolBar::layoutButtons()
{
#if LL_DARWIN
	const S32 FUDGE_WIDTH_OF_SCREEN = 4;                                    
	S32 width = gViewerWindow->getWindowWidth() + FUDGE_WIDTH_OF_SCREEN;   
	S32 pad = 2;

	// this function may be called before postBuild(), in which case mResizeHandle won't have been set up yet.
	if(mResizeHandle != NULL)
	{
		if(!gViewerWindow->getWindow()->getFullscreen())
		{
			// Only when running in windowed mode on the Mac, leave room for a resize widget on the right edge of the bar.
			width -= RESIZE_HANDLE_WIDTH;

			LLRect r;
			r.mLeft = width - pad;
			r.mBottom = 0;
			r.mRight = r.mLeft + RESIZE_HANDLE_WIDTH;
			r.mTop = r.mBottom + RESIZE_HANDLE_HEIGHT;
			mResizeHandle->setRect(r);
			mResizeHandle->setVisible(TRUE);
		}
		else
		{
			mResizeHandle->setVisible(FALSE);
		}
	}
#endif // LL_DARWIN
}


// virtual
void LLToolBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);

	layoutButtons();
}


// Per-frame updates of visibility
void LLToolBar::refresh()
{
	BOOL show = gSavedSettings.getBOOL("ShowToolBar");
	BOOL mouselook = gAgent.cameraMouselook();
	setVisible(show && !mouselook);

	BOOL sitting = FALSE;
	if (gAgent.getAvatarObject())
	{
		sitting = gAgent.getAvatarObject()->mIsSitting;
	}

	childSetEnabled("fly_btn", (gAgent.canFly() || gAgent.getFlying()) && !sitting );

	childSetEnabled("build_btn", LLViewerParcelMgr::getInstance()->agentCanBuild() );

	// Check to see if we're in build mode
	BOOL build_mode = LLToolMgr::getInstance()->inEdit();
	// And not just clicking on a scripted object
	if (LLToolGrab::getInstance()->getHideBuildHighlight())
	{
		build_mode = FALSE;
	}
	gSavedSettings.setBOOL("BuildBtnState", build_mode);

	if (isInVisibleChain())
	{
		updateCommunicateList();
	}
}

void LLToolBar::updateCommunicateList()
{
	LLFlyoutButton* communicate_button = getChild<LLFlyoutButton>("communicate_btn");
	LLSD selected = communicate_button->getValue();

	communicate_button->removeall();

	LLFloater* frontmost_floater = LLFloaterChatterBox::getInstance()->getActiveFloater();
	LLScrollListItem* itemp = NULL;

	itemp = communicate_button->add(LLFloaterMyFriends::getInstance()->getShortTitle(), LLSD("contacts"), ADD_TOP);
	if (LLFloaterMyFriends::getInstance() == frontmost_floater)
	{
		((LLScrollListText*)itemp->getColumn(0))->setFontStyle(LLFontGL::BOLD);
		// make sure current tab is selected in list
		if (selected.isUndefined())
		{
			selected = itemp->getValue();
		}
	}
	itemp = communicate_button->add(LLFloaterChat::getInstance()->getShortTitle(), LLSD("local chat"), ADD_TOP);
	if (LLFloaterChat::getInstance() == frontmost_floater)
	{
		((LLScrollListText*)itemp->getColumn(0))->setFontStyle(LLFontGL::BOLD);
		if (selected.isUndefined())
		{
			selected = itemp->getValue();
		}
	}
	communicate_button->addSeparator(ADD_TOP);
	communicate_button->add(getString("Redock Windows"), LLSD("redock"), ADD_TOP);
	communicate_button->addSeparator(ADD_TOP);
	communicate_button->add(LLFloaterMute::getInstance()->getShortTitle(), LLSD("mute list"), ADD_TOP);
	
	std::set<LLHandle<LLFloater> >::const_iterator floater_handle_it;

	if (gIMMgr->getIMFloaterHandles().size() > 0)
	{
		communicate_button->addSeparator(ADD_TOP);
	}

	for(floater_handle_it = gIMMgr->getIMFloaterHandles().begin(); floater_handle_it != gIMMgr->getIMFloaterHandles().end(); ++floater_handle_it)
	{
		LLFloaterIMPanel* im_floaterp = (LLFloaterIMPanel*)floater_handle_it->get();
		if (im_floaterp)
		{
			std::string floater_title = im_floaterp->getNumUnreadMessages() > 0 ? "*" : "";
			floater_title.append(im_floaterp->getShortTitle());
			itemp = communicate_button->add(floater_title, im_floaterp->getSessionID(), ADD_TOP);
			if (im_floaterp  == frontmost_floater)
			{
				((LLScrollListText*)itemp->getColumn(0))->setFontStyle(LLFontGL::BOLD);
				if (selected.isUndefined())
				{
					selected = itemp->getValue();
				}
			}
		}
	}

	communicate_button->setToggleState(gSavedSettings.getBOOL("ShowCommunicate"));
	communicate_button->setValue(selected);
}


// static
void LLToolBar::onClickCommunicate(LLUICtrl* ctrl, void* user_data)
{
	LLToolBar* toolbar = (LLToolBar*)user_data;
	LLFlyoutButton* communicate_button = toolbar->getChild<LLFlyoutButton>("communicate_btn");
	
	LLSD selected_option = communicate_button->getValue();
    
	if (selected_option.asString() == "contacts")
	{
		LLFloaterMyFriends::showInstance();
	}
	else if (selected_option.asString() == "local chat")
	{
		LLFloaterChat::showInstance();
	}
	else if (selected_option.asString() == "redock")
	{
		LLFloaterChatterBox::getInstance()->addFloater(LLFloaterMyFriends::getInstance(), FALSE);
		LLFloaterChatterBox::getInstance()->addFloater(LLFloaterChat::getInstance(), FALSE);
		LLUUID session_to_show;
		
		std::set<LLHandle<LLFloater> >::const_iterator floater_handle_it;
		for(floater_handle_it = gIMMgr->getIMFloaterHandles().begin(); floater_handle_it != gIMMgr->getIMFloaterHandles().end(); ++floater_handle_it)
		{
			LLFloater* im_floaterp = floater_handle_it->get();
			if (im_floaterp)
			{
				if (im_floaterp->isFrontmost())
				{
					session_to_show = ((LLFloaterIMPanel*)im_floaterp)->getSessionID();
				}
				LLFloaterChatterBox::getInstance()->addFloater(im_floaterp, FALSE);
			}
		}

		LLFloaterChatterBox::showInstance(session_to_show);
	}
	else if (selected_option.asString() == "mute list")
	{
		LLFloaterMute::showInstance();
	}
	else if (selected_option.isUndefined()) // user just clicked the communicate button, treat as toggle
	{
		if (LLFloaterChatterBox::getInstance()->getFloaterCount() == 0)
		{
			LLFloaterMyFriends::toggleInstance();
		}
		else
		{
			LLFloaterChatterBox::toggleInstance();
		}
	}
	else // otherwise selection_option is a specific IM session id
	{
		LLFloaterChatterBox::showInstance(selected_option);
	}
}


// static
void LLToolBar::onClickChat(void* user_data)
{
	handle_chat(NULL);
}

// static
void LLToolBar::onClickAppearance(void*)
{
	if (gAgent.areWearablesLoaded())
	{
		gAgent.changeCameraToCustomizeAvatar();
	}
}


// static
void LLToolBar::onClickFly(void*)
{
	gAgent.toggleFlying();
}


// static
void LLToolBar::onClickSit(void*)
{
	if (!(gAgent.getControlFlags() & AGENT_CONTROL_SIT_ON_GROUND))
	{
		// sit down
		gAgent.setFlying(FALSE);
		gAgent.setControlFlags(AGENT_CONTROL_SIT_ON_GROUND);

		// Might be first sit
		LLFirstUse::useSit();
	}
	else
	{
		// stand up
		gAgent.setFlying(FALSE);
		gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
	}
}


// static
void LLToolBar::onClickSnapshot(void*)
{
	LLFloaterSnapshot::show (0);
}


// static
void LLToolBar::onClickDirectory(void*)
{
	handle_find(NULL);
}


// static
void LLToolBar::onClickBuild(void*)
{
	toggle_build_mode();
}


// static
void LLToolBar::onClickRadar(void*)
{
	handle_mini_map(NULL);
}


// static
void LLToolBar::onClickMap(void*)
{
	handle_map(NULL);
}


// static
void LLToolBar::onClickInventory(void*)
{
	handle_inventory(NULL);
}

