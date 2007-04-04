/** 
 * @file llfloatermap.cpp
 * @brief The "mini-map" or radar in the upper right part of the screen.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
#include "lltextbox.h"
#include "llui.h"
#include "llviewermenu.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "viewer.h"

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
				"FloaterMapRect", 
				MAP_TITLE, 
				TRUE, 
				FLOATERMAP_MIN_WIDTH, 
				FLOATERMAP_MIN_HEIGHT_LARGE, 
				FALSE, 
				FALSE, 
				TRUE)	// close button
{
	const S32 LEFT = LLPANEL_BORDER_WIDTH;
	const S32 TOP = mRect.getHeight();

	S32 y = 0;

	// Map itself
	LLRect map_rect(
		LEFT, 
		TOP - LLPANEL_BORDER_WIDTH,
		mRect.getWidth() - LLPANEL_BORDER_WIDTH,
		y );
	LLColor4 bg_color = gColors.getColor( "NetMapBackgroundColor" );
	mMap = new LLNetMap("Net Map", map_rect, bg_color);
	mMap->setFollowsAll();
	addChildAtEnd(mMap);

	// Get the drag handle all the way in back
	removeChild(mDragHandle);
	addChildAtEnd(mDragHandle);

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
	return !gQuit;
}


// virtual
void LLFloaterMap::draw()
{
	if( getVisible() )
	{
		// Note: we can't just gAgent.check cameraMouselook() because the transition states are wrong.
		if( gAgent.cameraMouselook())
		{
			setMouseOpaque(FALSE);
			mDragHandle->setMouseOpaque(FALSE);

			drawChild(mMap);
		}
		else
		{
			setMouseOpaque(TRUE);
			mDragHandle->setMouseOpaque(TRUE);

			LLFloater::draw();
		}
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


BOOL process_secondlife_url(LLString url)
{
	S32 strpos, strpos2;

	LLString slurlID = "slurl.com/secondlife/";
	strpos = url.find(slurlID);
	
	if (strpos < 0)
	{
		slurlID="secondlife://";
		strpos = url.find(slurlID);
	}
	
	if (strpos >= 0) 
	{
		LLString simname;

		strpos+=slurlID.length();
		strpos2=url.find("/",strpos);
		if (strpos2 < strpos) strpos2=url.length();
		simname="secondlife://" + url.substr(strpos,url.length() - strpos);

		LLURLSimString::setString( simname );
		LLURLSimString::parse();

		// if there is a world map
		if ( gFloaterWorldMap )
		{
			// mark where the destination is
			gFloaterWorldMap->trackURL( LLURLSimString::sInstance.mSimName.c_str(),
										LLURLSimString::sInstance.mX,
										LLURLSimString::sInstance.mY,
										LLURLSimString::sInstance.mZ );

			// display map
			LLFloaterWorldMap::show( NULL, TRUE );
		};

		return TRUE;
	}
	return FALSE;
}
