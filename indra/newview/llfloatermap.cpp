/** 
 * @file llfloatermap.cpp
 * @brief The "mini-map" or radar in the upper right part of the screen.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

// self include
#include "llfloatermap.h"

// Library includes
#include "llfontgl.h"
#include "llinventory.h"
#include "message.h"

// Viewer includes
#include "llagent.h"
#include "llcolorscheme.h"
#include "llviewercontrol.h"
#include "lldraghandle.h"
#include "lleconomy.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "llnetmap.h"
#include "llregionhandle.h"
#include "llresizebar.h"
#include "llresizehandle.h"
#include "llresmgr.h"
#include "llsky.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "llstatgraph.h"
#include "llstatusbar.h"
//#include "lltextbox.h"
#include "llui.h"
#include "llviewermenu.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llurlsimstring.h"

#include "llglheaders.h"

//
// Constants
//
const S32 LEGEND_SIZE = 16;

const S32 HPAD = 4;

const S32 MAP_COMBOBOX_WIDTH = 128;
const S32 MAP_COMBOBOX_HEIGHT = 20;
const S32 GO_BTN_WIDTH = 20;
const S32 SLIDER_WIDTH = LEGEND_SIZE + MAP_COMBOBOX_WIDTH + HPAD + GO_BTN_WIDTH - HPAD;
const S32 SLIDER_HEIGHT = 16;

const S32 SIM_STAT_WIDTH = 8;

const S32 MAP_SCALE_MIN = 35;	// in pixels per sim
const S32 MAP_SCALE_MAX = 180;	// in pixels per sim
const S32 MAP_SCALE_INCREMENT = 5;

const char MAP_TITLE[] = "";

const S32 NETMAP_MIN_WIDTH = 128;
const S32 NETMAP_MIN_HEIGHT = 128;

const S32 FLOATERMAP_MIN_WIDTH = 
	SLIDER_WIDTH + 
	2 * (HPAD + LLPANEL_BORDER_WIDTH);

const S32 MORE_BTN_VPAD = 2;

const S32 SPIN_VPAD = 4;
  
const S32 TRACKING_LABEL_HEIGHT = 16;

const S32 FLOATERMAP_MIN_HEIGHT_LARGE = 60;

//
// Globals
//
LLFloaterMap *gFloaterMap = NULL;




//
// Member functions
//

LLFloaterMap::LLFloaterMap(const std::string& name)
	:
	LLFloater(name, 
				"FloaterMiniMapRect", 
				MAP_TITLE, 
				TRUE, 
				FLOATERMAP_MIN_WIDTH, 
				FLOATERMAP_MIN_HEIGHT_LARGE, 
				FALSE, 
				FALSE, 
				TRUE)	// close button
{
	const S32 LEFT = LLPANEL_BORDER_WIDTH;
	const S32 TOP = getRect().getHeight();

	S32 y = 0;

	// Map itself
	LLRect map_rect(
		LEFT, 
		TOP - LLPANEL_BORDER_WIDTH,
		getRect().getWidth() - LLPANEL_BORDER_WIDTH,
		y );
	LLColor4 bg_color = gColors.getColor( "NetMapBackgroundColor" );
	mMap = new LLNetMap("Net Map", map_rect, bg_color);
	mMap->setFollowsAll();
	addChildAtEnd(mMap);

	// Get the drag handle all the way in back
	sendChildToBack(getDragHandle());

	setIsChrome(TRUE);
}


LLFloaterMap::~LLFloaterMap()
{
}


// virtual 
void LLFloaterMap::setVisible(BOOL visible)
{
	LLFloater::setVisible(visible);

	gSavedSettings.setBOOL("ShowMiniMap", visible);
}


// virtual
void LLFloaterMap::onClose(bool app_quitting)
{
	LLFloater::setVisible(FALSE);

	if (!app_quitting)
	{
		gSavedSettings.setBOOL("ShowMiniMap", FALSE);
	}
}

BOOL LLFloaterMap::canClose()
{
	return !LLApp::isExiting();
}


// virtual
void LLFloaterMap::draw()
{
	// Note: we can't just gAgent.check cameraMouselook() because the transition states are wrong.
	if( gAgent.cameraMouselook())
	{
		setMouseOpaque(FALSE);
		getDragHandle()->setMouseOpaque(FALSE);

		drawChild(mMap);
	}
	else
	{
		setMouseOpaque(TRUE);
		getDragHandle()->setMouseOpaque(TRUE);

		LLFloater::draw();
	}
}

// static
void LLFloaterMap::toggle(void*)
{
	if( gFloaterMap )
	{
		if (gFloaterMap->getVisible())
		{
			gFloaterMap->close();
		}
		else
		{
			gFloaterMap->open();		/* Flawfinder: ignore */
		}
	}
}
