/** 
 * @file lltoolbar.cpp
 * @author James Cook, Richard Nelson
 * @brief Large friendly buttons at bottom of screen.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "lltoolbar.h"

#include "imageids.h"
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llflyoutbutton.h"
#include "llrect.h"
#include "llparcel.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llbutton.h"
#include "llfocusmgr.h"
#include "llviewercontrol.h"
#include "llmenucommands.h"
#include "llimview.h"
#include "lluiconstants.h"
#include "llvoavatarself.h"
#include "lltooldraganddrop.h"
#include "llfloaterinventory.h"
#include "llfloaterchatterbox.h"
#include "llfloatersnapshot.h"
#include "llinventorypanel.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llviewermenu.h"
//#include "llfirstuse.h"
#include "llpanelblockedlist.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "llviewerparcelmgr.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "lltoolgrab.h"
#include "llcombobox.h"
#include "lllayoutstack.h"

#if LL_DARWIN

	#include "llresizehandle.h"

#endif // LL_DARWIN

//
// Globals
//

LLToolBar *gToolBar = NULL;

//
// Statics
//
F32	LLToolBar::sInventoryAutoOpenTime = 1.f;

//
// Functions
//

LLToolBar::LLToolBar()
	: LLPanel(),

	mInventoryAutoOpen(FALSE),
	mNumUnreadIMs(0)	
#if LL_DARWIN
	, mResizeHandle(NULL)
#endif // LL_DARWIN
{
	setIsChrome(TRUE);
	setFocusRoot(TRUE);
	
	mCommitCallbackRegistrar.add("HandleCommunicate", &LLToolBar::onClickCommunicate);
}


BOOL LLToolBar::postBuild()
{
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
		LLResizeHandle::Params p;
		p.name("");
		p.rect(rect);
		p.min_width(RESIZE_HANDLE_WIDTH);
		p.min_height(RESIZE_HANDLE_HEIGHT);
		p.enabled(false);
		mResizeHandle = LLUICtrlFactory::create<LLResizeHandle>(p);
		addChildInBack(mResizeHandle);
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

	LLRect button_screen_rect;
	inventory_btn->localRectToScreen(inventory_btn->getRect(),&button_screen_rect);
	
	const LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel();
	if(active_panel)
	{
		mInventoryAutoOpen = FALSE;
	}
	else if (button_screen_rect.pointInRect(x, y))
	{
		if (mInventoryAutoOpen)
		{
			if (!active_panel && 
				mInventoryAutoOpenTimer.getElapsedTimeF32() > sInventoryAutoOpenTime)
			{
				LLFloaterInventory::showAgentInventory();
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
	S32 width = gViewerWindow->getWindowWidthScaled() + FUDGE_WIDTH_OF_SCREEN;   
	S32 pad = 2;

	// this function may be called before postBuild(), in which case mResizeHandle won't have been set up yet.
	if(mResizeHandle != NULL)
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

	//LLFloater* frontmost_floater = LLFloaterChatterBox::getInstance()->getActiveFloater();
	LLScrollListItem* itemp = NULL;

	LLSD contact_sd;
	contact_sd["value"] = "contacts";
	/*contact_sd["columns"][0]["value"] = LLFloaterMyFriends::getInstance()->getShortTitle(); 
	if (LLFloaterMyFriends::getInstance() == frontmost_floater)
	{
		contact_sd["columns"][0]["font"]["name"] = "SANSSERIF_SMALL"; 
		contact_sd["columns"][0]["font"]["style"] = "BOLD"; 
		// make sure current tab is selected in list
		if (selected.isUndefined())
		{
			selected = "contacts";
		}
	}*/
	itemp = communicate_button->addElement(contact_sd, ADD_TOP);

	communicate_button->addSeparator(ADD_TOP);
	communicate_button->add(getString("Redock Windows"), LLSD("redock"), ADD_TOP);
	communicate_button->addSeparator(ADD_TOP);
	communicate_button->add(getString("Blocked List"), LLSD("mute list"), ADD_TOP);

	std::set<LLHandle<LLFloater> >::const_iterator floater_handle_it;

	/*if (gIMMgr->getIMFloaterHandles().size() > 0)
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
			LLSD im_sd;
			im_sd["value"] = im_floaterp->getSessionID();
			im_sd["columns"][0]["value"] = floater_title;
			if (im_floaterp  == frontmost_floater)
			{
				im_sd["columns"][0]["font"]["name"] = "SANSSERIF_SMALL";
				im_sd["columns"][0]["font"]["style"] = "BOLD";
				if (selected.isUndefined())
				{
					selected = im_floaterp->getSessionID();
				}
			}
			itemp = communicate_button->addElement(im_sd, ADD_TOP);
		}
	}*/

	communicate_button->setValue(selected);
}


// static
void LLToolBar::onClickCommunicate(LLUICtrl* ctrl, const LLSD& user_data)
{
	LLFlyoutButton* communicate_button = dynamic_cast<LLFlyoutButton*>(ctrl);
	llassert_always(communicate_button);
	
	LLSD selected_option = communicate_button->getValue();
    
	if (selected_option.asString() == "contacts")
	{
		LLFloaterReg::showInstance("contacts", "friends");
	}
	else if (selected_option.asString() == "local chat")
	{
		LLFloaterReg::showInstance("communicate", "local");
	}
	else if (selected_option.asString() == "redock")
	{
		/*LLFloaterChatterBox* chatterbox_instance = LLFloaterChatterBox::getInstance();
		if(chatterbox_instance)
		{
			chatterbox_instance->addFloater(LLFloaterMyFriends::getInstance(), FALSE);
			
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
					chatterbox_instance->addFloater(im_floaterp, FALSE);
				}
			}
			LLFloaterReg::showInstance("communicate", session_to_show);
		}*/
	}
	else if (selected_option.asString() == "mute list")
	{
		LLPanelBlockedList::showPanelAndSelect(LLUUID::null);
	}
	else if (selected_option.isUndefined()) // user just clicked the communicate button, treat as toggle
	{
		/*LLFloaterReg::toggleInstance("communicate");*/
		}
	else // otherwise selection_option is undifined or a specific IM session id
	{
		/*LLFloaterReg::showInstance("communicate", selected_option);*/
	}
}


