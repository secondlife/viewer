/** 
 * @file llstatusbar.cpp
 * @brief LLStatusBar class implementation
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

#include "llstatusbar.h"

#include <iomanip>

#include "imageids.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llparcel.h"
#include "llstring.h"
#include "message.h"

#include "llagent.h"
#include "llbutton.h"
#include "llviewercontrol.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterchat.h"
#include "llfloaterdirectory.h"		// to spawn search
#include "llfloaterland.h"
#include "llfloaterregioninfo.h"
#include "llfloaterscriptdebug.h"
#include "llhudicon.h"
#include "llinventoryview.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llnotify.h"
#include "llimview.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llframetimer.h"
#include "llvoavatar.h"
#include "llresmgr.h"
#include "llworld.h"
#include "llstatgraph.h"
#include "llviewermenu.h"	// for gMenuBarView
#include "llviewerparcelmgr.h"
#include "llviewerthrottle.h"
#include "llvieweruictrlfactory.h"
#include "llvoiceclient.h"	// for gVoiceClient

#include "lltoolmgr.h"
#include "llfocusmgr.h"
#include "llappviewer.h"

//#include "llfirstuse.h"

//
// Globals
//
LLStatusBar *gStatusBar = NULL;
S32 STATUS_BAR_HEIGHT = 0;
extern S32 MENU_BAR_HEIGHT;


// TODO: these values ought to be in the XML too
const S32 MENU_PARCEL_SPACING = 1;	// Distance from right of menu item to parcel information
const S32 SIM_STAT_WIDTH = 8;
const F32 SIM_WARN_FRACTION = 0.75f;
const F32 SIM_FULL_FRACTION = 0.98f;
const LLColor4 SIM_OK_COLOR(0.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_WARN_COLOR(1.f, 1.f, 0.f, 1.f);
const LLColor4 SIM_FULL_COLOR(1.f, 0.f, 0.f, 1.f);
const F32 ICON_TIMER_EXPIRY		= 3.f; // How long the balance and health icons should flash after a change.
const F32 ICON_FLASH_FREQUENCY	= 2.f;
const S32 GRAPHIC_FUDGE = 4;
const S32 TEXT_HEIGHT = 18;

static void onClickParcelInfo(void*);
static void onClickBalance(void*);
static void onClickBuyCurrency(void*);
static void onClickHealth(void*);
static void onClickFly(void*);
static void onClickPush(void*);
static void onClickVoice(void*);
static void onClickBuild(void*);
static void onClickScripts(void*);
static void onClickBuyLand(void*);
static void onClickScriptDebug(void*);

std::vector<std::string> LLStatusBar::sDays;
std::vector<std::string> LLStatusBar::sMonths;
const U32 LLStatusBar::MAX_DATE_STRING_LENGTH = 2000;

LLStatusBar::LLStatusBar(const std::string& name, const LLRect& rect)
:	LLPanel(name, LLRect(), FALSE),		// not mouse opaque
	mBalance(0),
	mHealth(100),
	mSquareMetersCredit(0),
	mSquareMetersCommitted(0)
{
	// status bar can possible overlay menus?
	mMouseOpaque = FALSE;
	setIsChrome(TRUE);

	// size of day of the weeks and year
	sDays.reserve(7);
	sMonths.reserve(12);

	mBalanceTimer = new LLFrameTimer();
	mHealthTimer = new LLFrameTimer();

	gUICtrlFactory->buildPanel(this,"panel_status_bar.xml");

	// status bar can never get a tab
	setFocusRoot(FALSE);

	// build date necessary data (must do after panel built)
	setupDate();

	mTextParcelName = LLUICtrlFactory::getTextBoxByName( this, "ParcelNameText" );
	mTextBalance = LLUICtrlFactory::getTextBoxByName( this, "BalanceText" );

	mTextHealth = LLUICtrlFactory::getTextBoxByName( this, "HealthText" );
	mTextTime = LLUICtrlFactory::getTextBoxByName( this, "TimeText" );
	
	childSetAction("scriptout", onClickScriptDebug, this);
	childSetAction("health", onClickHealth, this);
	childSetAction("fly", onClickFly, this);
	childSetAction("buyland", onClickBuyLand, this );
	childSetAction("buycurrency", onClickBuyCurrency, this );
	childSetAction("build", onClickBuild, this );
	childSetAction("scripts", onClickScripts, this );
	childSetAction("restrictpush", onClickPush, this );
	childSetAction("status_voice", onClickVoice, this );

	childSetCommitCallback("search_editor", onCommitSearch, this);
	childSetAction("search_btn", onClickSearch, this);

	childSetActionTextbox("ParcelNameText", onClickParcelInfo );
	childSetActionTextbox("BalanceText", onClickBalance );
}

LLStatusBar::~LLStatusBar()
{
	delete mBalanceTimer;
	mBalanceTimer = NULL;

	delete mHealthTimer;
	mHealthTimer = NULL;

	// LLView destructor cleans up children
}

//virtual
EWidgetType LLStatusBar::getWidgetType() const
{
	return WIDGET_TYPE_STATUS_BAR;
}

//virtual
LLString LLStatusBar::getWidgetTag() const
{
	return LL_STATUS_BAR_TAG;
}

//-----------------------------------------------------------------------
// Overrides
//-----------------------------------------------------------------------

// virtual
void LLStatusBar::draw()
{
	refresh();

	if (mBgVisible)
	{
		gl_drop_shadow(0, mRect.getHeight(), mRect.getWidth(), 0, 
				LLUI::sColorsGroup->getColor("ColorDropShadow"), 
				LLUI::sConfigGroup->getS32("DropShadowFloater") );
	}
	LLPanel::draw();
}


// Per-frame updates of visibility
void LLStatusBar::refresh()
{
	// *TODO: Localize / translate time
	
	// Get current UTC time, adjusted for the user's clock
	// being off.
	U32 utc_time;
	utc_time = time_corrected();

	// There's only one internal tm buffer.
	struct tm* internal_time;

	// Convert to Pacific, based on server's opinion of whether
	// it's daylight savings time there.
	internal_time = utc_to_pacific_time(utc_time, gPacificDaylightTime);

	S32 hour = internal_time->tm_hour;
	S32 min  = internal_time->tm_min;

	std::string am_pm = "AM";
	if (hour > 11)
	{
		hour -= 12;
		am_pm = "PM";
	}

	std::string tz = "PST";
	if (gPacificDaylightTime)
	{
		tz = "PDT";
	}
	// Zero hour is 12 AM
	if (hour == 0) hour = 12;
	std::ostringstream t;
	t << std::setfill(' ') << std::setw(2) << hour << ":" 
	  << std::setfill('0') << std::setw(2) << min 
	  << " " << am_pm << " " << tz;
	mTextTime->setText(t.str());

	// Year starts at 1900, set the tooltip to have the date
	std::ostringstream date;
	date	<< sDays[internal_time->tm_wday] << ", "
			<< std::setfill('0') << std::setw(2) << internal_time->tm_mday << " "
			<< sMonths[internal_time->tm_mon] << " "
			<< internal_time->tm_year + 1900;
	mTextTime->setToolTip(date.str());

	LLRect r;
	const S32 MENU_RIGHT = gMenuBarView->getRightmostMenuEdge();
	S32 x = MENU_RIGHT + MENU_PARCEL_SPACING;
	S32 y = 0;
	
	// reshape menu bar to its content's width
	if (MENU_RIGHT != gMenuBarView->getRect().getWidth())
	{
		gMenuBarView->reshape(MENU_RIGHT, gMenuBarView->getRect().getHeight());
	}

	LLViewerRegion *region = gAgent.getRegion();
	LLParcel *parcel = gParcelMgr->getAgentParcel();

	LLRect buttonRect;

	if (LLHUDIcon::iconsNearby())
	{
		childGetRect( "scriptout", buttonRect );
		r.setOriginAndSize( x, y, buttonRect.getWidth(), buttonRect.getHeight());
		childSetRect("scriptout",r);
		childSetVisible("scriptout", true);
		x += buttonRect.getWidth();
	}
	else
	{
		childSetVisible("scriptout", false);
	}

	if ((region && region->getAllowDamage()) ||
		(parcel && parcel->getAllowDamage()) )
	{
		// set visibility based on flashing
		if( mHealthTimer->hasExpired() )
		{
			childSetVisible("health", true);
		}
		else
		{
			BOOL flash = S32(mHealthTimer->getElapsedSeconds() * ICON_FLASH_FREQUENCY) & 1;
			childSetVisible("health", flash);
		}
		mTextHealth->setVisible(TRUE);

		// Health
		childGetRect( "health", buttonRect );
		r.setOriginAndSize( x, y-GRAPHIC_FUDGE, buttonRect.getWidth(), buttonRect.getHeight());
		childSetRect("health", r);
		x += buttonRect.getWidth();

		const S32 health_width = S32( LLFontGL::sSansSerifSmall->getWidth("100%") );
		r.set(x, y+TEXT_HEIGHT - 2, x+health_width, y);
		mTextHealth->setRect(r);
		x += health_width;
	}
	else
	{
		// invisible if region doesn't allow damage
		childSetVisible("health", false);
		mTextHealth->setVisible(FALSE);
	}

	if ((region && region->getBlockFly()) ||
		(parcel && !parcel->getAllowFly()) )
	{
		// No Fly Zone
		childGetRect( "fly", buttonRect );
		childSetVisible( "fly", true );
		r.setOriginAndSize( x, y-GRAPHIC_FUDGE, buttonRect.getWidth(), buttonRect.getHeight());
		childSetRect( "fly", r );
		x += buttonRect.getWidth();
	}
	else
	{
		childSetVisible("fly", false);
	}

	BOOL no_build = parcel && !parcel->getAllowModify();
	childSetVisible("build", no_build);
	if (no_build)
	{
		childGetRect( "build", buttonRect );
		// No Build Zone
		r.setOriginAndSize( x, y-GRAPHIC_FUDGE, buttonRect.getWidth(), buttonRect.getHeight());
		childSetRect( "build", r );
		x += buttonRect.getWidth();
	}

	BOOL no_scripts = FALSE;
	if((region
		&& ((region->getRegionFlags() & REGION_FLAGS_SKIP_SCRIPTS)
			|| (region->getRegionFlags() & REGION_FLAGS_ESTATE_SKIP_SCRIPTS)))
	   || (parcel && !parcel->getAllowOtherScripts()))
	{
		no_scripts = TRUE;
	}
	childSetVisible("scripts", no_scripts);
	if (no_scripts)
	{
		// No scripts
		childGetRect( "scripts", buttonRect );
		r.setOriginAndSize( x, y-GRAPHIC_FUDGE, buttonRect.getWidth(), buttonRect.getHeight());
		childSetRect( "scripts", r );
		x += buttonRect.getWidth();
	}

	BOOL no_region_push = (region && region->getRestrictPushObject());
	BOOL no_push = no_region_push || (parcel && parcel->getRestrictPushObject());
	childSetVisible("restrictpush", no_push);
	if (no_push)
	{
		childGetRect( "restrictpush", buttonRect );
		r.setOriginAndSize( x, y-GRAPHIC_FUDGE, buttonRect.getWidth(), buttonRect.getHeight());
		childSetRect( "restrictpush", r );
		x += buttonRect.getWidth();
	}

	BOOL have_voice = gVoiceClient->getAreaVoiceDisabled() ? FALSE : TRUE;
	childSetVisible("status_voice", have_voice);
	if (have_voice)
	{
		childGetRect( "status_voice", buttonRect );
		r.setOriginAndSize( x, y-GRAPHIC_FUDGE, buttonRect.getWidth(), buttonRect.getHeight());
		childSetRect( "status_voice", r );
		x += buttonRect.getWidth();
	}

	BOOL canBuyLand = parcel
		&& !parcel->isPublic()
		&& gParcelMgr->canAgentBuyParcel(parcel, false);
	childSetVisible("buyland", canBuyLand);
	if (canBuyLand)
	{
		childGetRect( "buyland", buttonRect );
		r.setOriginAndSize( x, y, buttonRect.getWidth(), buttonRect.getHeight());
		childSetRect( "buyland", r );
		x += buttonRect.getWidth();
	}

	LLString location_name;
	if (region)
	{
		const LLVector3& agent_pos_region = gAgent.getPositionAgent();
		S32 pos_x = lltrunc( agent_pos_region.mV[VX] );
		S32 pos_y = lltrunc( agent_pos_region.mV[VY] );
		S32 pos_z = lltrunc( agent_pos_region.mV[VZ] );

		// Round the numbers based on the velocity
		LLVector3 agent_velocity = gAgent.getVelocity();
		F32 velocity_mag_sq = agent_velocity.magVecSquared();

		const F32 FLY_CUTOFF = 6.f;		// meters/sec
		const F32 FLY_CUTOFF_SQ = FLY_CUTOFF * FLY_CUTOFF;
		const F32 WALK_CUTOFF = 1.5f;	// meters/sec
		const F32 WALK_CUTOFF_SQ = WALK_CUTOFF * WALK_CUTOFF;

		if (velocity_mag_sq > FLY_CUTOFF_SQ)
		{
			pos_x -= pos_x % 4;
			pos_y -= pos_y % 4;
		}
		else if (velocity_mag_sq > WALK_CUTOFF_SQ)
		{
			pos_x -= pos_x % 2;
			pos_y -= pos_y % 2;
		}

		mRegionDetails.mTime = mTextTime->getText();
		mRegionDetails.mBalance = mBalance;
		mRegionDetails.mAccesString = (char *)region->getSimAccessString();
		mRegionDetails.mPing = region->getNetDetailsForLCD();
		if (parcel && !parcel->getName().empty())
		{
			location_name = region->getName()
				+ llformat(" %d, %d, %d (%s) - %s", 
						   pos_x, pos_y, pos_z,
						   region->getSimAccessString(),
						   parcel->getName().c_str());

			// keep these around for the LCD to use
			mRegionDetails.mRegionName = region->getName();
			mRegionDetails.mParcelName = (char *)parcel->getName().c_str();
			mRegionDetails.mX = pos_x;
			mRegionDetails.mY = pos_y;
			mRegionDetails.mZ = pos_z;
			mRegionDetails.mArea = parcel->getArea();
			mRegionDetails.mForSale = parcel->getForSale();
			mRegionDetails.mTraffic = gParcelMgr->getDwelling();
			
			if (parcel->isPublic())
			{
				snprintf(mRegionDetails.mOwner, MAX_STRING, "Public");
			}
			else
			{
				if (parcel->getIsGroupOwned())
				{
					if(!parcel->getGroupID().isNull())
					{
						gCacheName->getGroupName(parcel->getGroupID(), mRegionDetails.mOwner);
					}
					else
					{
						snprintf(mRegionDetails.mOwner, MAX_STRING, "Group Owned");
					}
				}
				else
				{
					// Figure out the owner's name
					char owner_first[MAX_STRING];	/*Flawfinder: ignore*/
					char owner_last[MAX_STRING];	/*Flawfinder: ignore*/
					gCacheName->getName(parcel->getOwnerID(), owner_first, owner_last);
					snprintf(mRegionDetails.mOwner, MAX_STRING, "%s %s", owner_first, owner_last); 		/* Flawfinder: ignore */
				}
			}
		}
		else
		{
			location_name = region->getName()
				+ llformat(" %d, %d, %d (%s)", 
						   pos_x, pos_y, pos_z,
						   region->getSimAccessString());
			// keep these around for the LCD to use
			mRegionDetails.mRegionName = region->getName();
			mRegionDetails.mParcelName = "Unknown";
			
			mRegionDetails.mX = pos_x;
			mRegionDetails.mY = pos_y;
			mRegionDetails.mZ = pos_z;
			mRegionDetails.mArea = 0;
			mRegionDetails.mForSale = FALSE;
			snprintf(mRegionDetails.mOwner, MAX_STRING, "Unknown");
			mRegionDetails.mTraffic = 0.0f;
		}
	}
	else
	{
		// no region
		location_name = "(Unknown)";
		// keep these around for the LCD to use
		mRegionDetails.mRegionName = LLString("Unknown");
		mRegionDetails.mParcelName = "Unknown";
		mRegionDetails.mAccesString = "Unknown";
		mRegionDetails.mX = 0;
		mRegionDetails.mY = 0;
		mRegionDetails.mZ = 0;
		mRegionDetails.mArea = 0;
		mRegionDetails.mForSale = FALSE;
		snprintf(mRegionDetails.mOwner, MAX_STRING, "Unknown");
		mRegionDetails.mTraffic = 0.0f;
	}
	mTextParcelName->setText(location_name);

	// Adjust region name and parcel name
	x += 4;

	const S32 PARCEL_RIGHT =  llmin(mTextTime->getRect().mLeft, mTextParcelName->getTextPixelWidth() + x + 5);
	r.set(x+4, mRect.getHeight() - 2, PARCEL_RIGHT, 0);
	mTextParcelName->setRect(r);
}

void LLStatusBar::setVisibleForMouselook(bool visible)
{
	mTextBalance->setVisible(visible);
	mTextTime->setVisible(visible);
	childSetVisible("buycurrency", visible);
	childSetVisible("search_editor", visible);
	childSetVisible("search_btn", visible);
	setBackgroundVisible(visible);
}

void LLStatusBar::debitBalance(S32 debit)
{
	setBalance(getBalance() - debit);
}

void LLStatusBar::creditBalance(S32 credit)
{
	setBalance(getBalance() + credit);
}

void LLStatusBar::setBalance(S32 balance)
{
	LLString money_str = gResMgr->getMonetaryString( balance );
	LLString balance_str = "L$";
	balance_str += money_str;
	mTextBalance->setText( balance_str );

	if (mBalance && (fabs((F32)(mBalance - balance)) > gSavedSettings.getF32("UISndMoneyChangeThreshold")))
	{
		if (mBalance > balance)
			make_ui_sound("UISndMoneyChangeDown");
		else
			make_ui_sound("UISndMoneyChangeUp");
	}

	if( balance != mBalance )
	{
		mBalanceTimer->reset();
		mBalanceTimer->setTimerExpirySec( ICON_TIMER_EXPIRY );
		mBalance = balance;
	}
}

void LLStatusBar::setHealth(S32 health)
{
	//llinfos << "Setting health to: " << buffer << llendl;
	mTextHealth->setText(llformat("%d%%", health));

	if( mHealth > health )
	{
		if (mHealth > (health + gSavedSettings.getF32("UISndHealthReductionThreshold")))
		{
			LLVOAvatar *me;

			if ((me = gAgent.getAvatarObject()))
			{
				if (me->getSex() == SEX_FEMALE)
				{
					make_ui_sound("UISndHealthReductionF");
				}
				else
				{
					make_ui_sound("UISndHealthReductionM");
				}
			}
		}

		mHealthTimer->reset();
		mHealthTimer->setTimerExpirySec( ICON_TIMER_EXPIRY );
	}

	mHealth = health;
}

S32 LLStatusBar::getBalance() const
{
	return mBalance;
}


S32 LLStatusBar::getHealth() const
{
	return mHealth;
}

void LLStatusBar::setLandCredit(S32 credit)
{
	mSquareMetersCredit = credit;
}
void LLStatusBar::setLandCommitted(S32 committed)
{
	mSquareMetersCommitted = committed;
}

BOOL LLStatusBar::isUserTiered() const
{
	return (mSquareMetersCredit > 0);
}

S32 LLStatusBar::getSquareMetersCredit() const
{
	return mSquareMetersCredit;
}

S32 LLStatusBar::getSquareMetersCommitted() const
{
	return mSquareMetersCommitted;
}

S32 LLStatusBar::getSquareMetersLeft() const
{
	return mSquareMetersCredit - mSquareMetersCommitted;
}

static void onClickParcelInfo(void* data)
{
	gParcelMgr->selectParcelAt(gAgent.getPositionGlobal());

	LLFloaterLand::show();
}

static void onClickBalance(void* data)
{
	LLFloaterBuyCurrency::buyCurrency();
}

static void onClickBuyCurrency(void* data)
{
	LLFloaterBuyCurrency::buyCurrency();
}

static void onClickHealth(void* )
{
	LLNotifyBox::showXml("NotSafe");
}

static void onClickScriptDebug(void*)
{
	LLFloaterScriptDebug::show(LLUUID::null);
}

static void onClickFly(void* )
{
	LLNotifyBox::showXml("NoFly");
}

static void onClickPush(void* )
{
	LLNotifyBox::showXml("PushRestricted");
}

static void onClickVoice(void* )
{
	LLNotifyBox::showXml("VoiceAvailablity");
	//LLFirstUse::useVoice();
}

static void onClickBuild(void*)
{
	LLNotifyBox::showXml("NoBuild");
}

static void onClickScripts(void*)
{
	LLViewerRegion* region = gAgent.getRegion();
	if(region && region->getRegionFlags() & REGION_FLAGS_ESTATE_SKIP_SCRIPTS)
	{
		LLNotifyBox::showXml("ScriptsStopped");
	}
	else if(region && region->getRegionFlags() & REGION_FLAGS_SKIP_SCRIPTS)
	{
		LLNotifyBox::showXml("ScriptsNotRunning");
	}
	else
	{
		LLNotifyBox::showXml("NoOutsideScripts");
	}
}

static void onClickBuyLand(void*)
{
	gParcelMgr->selectParcelAt(gAgent.getPositionGlobal());
	gParcelMgr->startBuyLand();
}

// sets the static variables necessary for the date
void LLStatusBar::setupDate()
{
	// fill the day array with what's in the xui
	LLString day_list = childGetText("StatBarDaysOfWeek");
	size_t length = day_list.size();
	
	// quick input check
	if(length < MAX_DATE_STRING_LENGTH)
	{
		// tokenize it and put it in the array
		LLString cur_word;
		for(size_t i = 0; i < length; ++i)
		{
			if(day_list[i] == ':')
			{
				sDays.push_back(cur_word);
				cur_word.clear();
			}
			else
			{
				cur_word.append(1, day_list.c_str()[i]);
			}
		}
		sDays.push_back(cur_word);
	}
	
	// fill the day array with what's in the xui	
	LLString month_list = childGetText( "StatBarMonthsOfYear" );
	length = month_list.size();
	
	// quick input check
	if(length < MAX_DATE_STRING_LENGTH)
	{
		// tokenize it and put it in the array
		LLString cur_word;
		for(size_t i = 0; i < length; ++i)
		{
			if(month_list[i] == ':')
			{
				sMonths.push_back(cur_word);
				cur_word.clear();
			}
			else
			{
				cur_word.append(1, month_list.c_str()[i]);
			}
		}
		sMonths.push_back(cur_word);
	}
	
	// make sure we have at least 7 days and 12 months
	if(sDays.size() < 7)
	{
		sDays.resize(7);
	}
	
	if(sMonths.size() < 12)
	{
		sMonths.resize(12);
	}
}

// static
void LLStatusBar::onCommitSearch(LLUICtrl*, void* data)
{
	// committing is the same as clicking "search"
	onClickSearch(data);
}

// static
void LLStatusBar::onClickSearch(void* data)
{
	LLStatusBar* self = (LLStatusBar*)data;
	LLString search_text = self->childGetText("search_editor");
	LLFloaterDirectory::showFindAll(search_text);
}

BOOL can_afford_transaction(S32 cost)
{
	return((cost <= 0)||((gStatusBar) && (gStatusBar->getBalance() >=cost)));
}
