/** 
 * @file lltoolbar.cpp
 * @author James Cook, Richard Nelson
 * @brief Large friendly buttons at bottom of screen.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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
#include "llvieweruictrlfactory.h"
#include "llviewerwindow.h"
#include "viewer.h"
#include "lltoolgrab.h"

#if LL_DARWIN

	#include "llresizehandle.h"

	// This class draws like an LLResizeHandle but has no interactivity.
	// It's just there to provide a cue to the user that the lower right corner of the window functions as a resize handle.
	class LLFakeResizeHandle : public LLResizeHandle
	{
	public:
		LLFakeResizeHandle(const LLString& name, const LLRect& rect, S32 min_width, S32 min_height, ECorner corner = RIGHT_BOTTOM )
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

LLToolBar::LLToolBar(const std::string& name, const LLRect& r)
:	LLPanel(name, r, BORDER_NO)
#if LL_DARWIN
	, mResizeHandle(NULL)
#endif // LL_DARWIN
{
	setIsChrome(TRUE);
	setFollows(	FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_BOTTOM );

	gUICtrlFactory->buildPanel(this, "panel_toolbar.xml");
	mIsFocusRoot = TRUE;

}


BOOL LLToolBar::postBuild()
{
	childSetAction("communicate_btn", onClickCommunicate, this);
	childSetControlName("communicate_btn", "ShowCommunicate");

	childSetAction("chat_btn", onClickChat, this);
	childSetControlName("chat_btn", "ChatVisible");

	childSetAction("appearance_btn", onClickAppearance, this);
	childSetControlName("appearance_btn", "");

	childSetAction("clothing_btn", onClickClothing, this);
	childSetControlName("clothing_btn", "ClothingBtnState");

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
		if(view->getWidgetType() == WIDGET_TYPE_BUTTON)
		{
			LLButton* btn = (LLButton*)view;
			btn->setSoundFlags(LLView::SILENT);
		}
	}

#if LL_DARWIN
	if(mResizeHandle == NULL)
	{
		LLRect rect(0, 0, RESIZE_HANDLE_WIDTH, RESIZE_HANDLE_HEIGHT);
		mResizeHandle = new LLFakeResizeHandle(LLString(""), rect, RESIZE_HANDLE_WIDTH, RESIZE_HANDLE_HEIGHT);
		this->addChildAtEnd(mResizeHandle);
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
									 LLString& tooltip_msg)
{
	LLButton* inventory_btn = LLUICtrlFactory::getButtonByName(this, "inventory_btn");
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
	// Always spans whole window. JC                                        
	const S32 FUDGE_WIDTH_OF_SCREEN = 4;                                    
	S32 width = gViewerWindow->getWindowWidth() + FUDGE_WIDTH_OF_SCREEN;    
	S32 count = getChildCount();
	S32 pad = 2;

#if LL_DARWIN
	// this function may be called before postBuild(), in which case mResizeHandle won't have been set up yet.
	if(mResizeHandle != NULL)
	{
		// a resize handle has been added as a child, increasing the count by one.
		count--;
		
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

	// We actually want to extend "pad" pixels off the right edge of the    
	// screen, such that the rightmost button is aligned.                   
	F32 segment_width = (F32)(width + pad) / (F32)count;                    
	S32 btn_width = lltrunc(segment_width - pad);                           
	
	// Evenly space all views
	S32 height = -1;
	S32 i = count - 1;
	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *btn_view = *child_iter;
		if(btn_view->getWidgetType() == WIDGET_TYPE_BUTTON)
		{
			if (height < 0)
			{
				height = btn_view->getRect().getHeight();
			}
			S32 x = llround(i*segment_width);                               
			S32 y = 0;                                                      
			LLRect r;                                                               
			r.setOriginAndSize(x, y, btn_width, height);                    
			btn_view->setRect(r);                                           
			i--;                                                            
		}
	}                                                                       
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

	// Clothing button updated inside LLFloaterClothing

	childSetEnabled("fly_btn", gAgent.canFly() || gAgent.getFlying() );

	childSetEnabled("build_btn", gParcelMgr->agentCanBuild() );

	// Check to see if we're in build mode
	BOOL build_mode = gToolMgr->inEdit();
	// And not just clicking on a scripted object
	if (gToolGrab->getHideBuildHighlight())
	{
		build_mode = FALSE;
	}
	gSavedSettings.setBOOL("BuildBtnState", build_mode);
}


// static
void LLToolBar::onClickCommunicate(void* user_data)
{
	LLFloaterChatterBox::toggleInstance(LLSD());
}


// static
void LLToolBar::onClickChat(void* user_data)
{
	handle_chat(NULL);
}

// static
void LLToolBar::onClickAppearance(void*)
{
	if (gAgent.getWearablesLoaded())
	{
		gAgent.changeCameraToCustomizeAvatar();
	}
}


// static
void LLToolBar::onClickClothing(void*)
{
	handle_clothing(NULL);
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

