/** 
 * @file llstatusbar.cpp
 * @brief LLStatusBar class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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

#include "lltoolmgr.h"
#include "llfocusmgr.h"
#include "viewer.h"

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

	mBalanceTimer = new LLFrameTimer();
	mHealthTimer = new LLFrameTimer();

	gUICtrlFactory->buildPanel(this,"panel_status_bar.xml");

	// status bar can never get a tab
	setFocusRoot(FALSE);

	mBtnScriptOut = LLUICtrlFactory::getButtonByName( this, "scriptout" );
	mBtnHealth = LLUICtrlFactory::getButtonByName( this, "health" );
	mBtnFly = LLUICtrlFactory::getButtonByName( this, "fly" );
	mBtnBuild = LLUICtrlFactory::getButtonByName( this, "build" );
	mBtnScripts = LLUICtrlFactory::getButtonByName( this, "scripts" );
	mBtnPush = LLUICtrlFactory::getButtonByName( this, "restrictpush" );
	mBtnBuyLand = LLUICtrlFactory::getButtonByName( this, "buyland" );
	mBtnBuyCurrency = LLUICtrlFactory::getButtonByName( this, "buycurrency" );

	mTextParcelName = LLUICtrlFactory::getTextBoxByName( this, "ParcelNameText" );
	mTextBalance = LLUICtrlFactory::getTextBoxByName( this, "BalanceText" );

	mTextHealth = LLUICtrlFactory::getTextBoxByName( this, "HealthText" );
	mTextTime = LLUICtrlFactory::getTextBoxByName( this, "TimeText" );

	S32 x = mRect.getWidth() - 2;
	S32 y = 0;
	LLRect r;
	r.set( x-SIM_STAT_WIDTH, y+MENU_BAR_HEIGHT-1, x, y+1);
	mSGBandwidth = new LLStatGraph("BandwidthGraph", r);
	mSGBandwidth->setFollows(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	mSGBandwidth->setStat(&gViewerStats->mKBitStat);
	LLString text = childGetText("bandwidth_tooltip") + " ";
	LLUIString bandwidth_tooltip = text;	// get the text from XML until this widget is XML driven
	mSGBandwidth->setLabel(bandwidth_tooltip.getString().c_str());
	mSGBandwidth->setUnits("Kbps");
	mSGBandwidth->setPrecision(0);
	addChild(mSGBandwidth);
	x -= SIM_STAT_WIDTH + 2;

	r.set( x-SIM_STAT_WIDTH, y+MENU_BAR_HEIGHT-1, x, y+1);
	mSGPacketLoss = new LLStatGraph("PacketLossPercent", r);
	mSGPacketLoss->setFollows(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	mSGPacketLoss->setStat(&gViewerStats->mPacketsLostPercentStat);
	text = childGetText("packet_loss_tooltip") + " ";
	LLUIString packet_loss_tooltip = text;	// get the text from XML until this widget is XML driven
	mSGPacketLoss->setLabel(packet_loss_tooltip.getString().c_str());
	mSGPacketLoss->setUnits("%");
	mSGPacketLoss->setMin(0.f);
	mSGPacketLoss->setMax(5.f);
	mSGPacketLoss->setThreshold(0, 0.5f);
	mSGPacketLoss->setThreshold(1, 1.f);
	mSGPacketLoss->setThreshold(2, 3.f);
	mSGPacketLoss->setPrecision(1);
	mSGPacketLoss->mPerSec = FALSE;
	addChild(mSGPacketLoss);

}

LLStatusBar::~LLStatusBar()
{
	delete mBalanceTimer;
	mBalanceTimer = NULL;

	delete mHealthTimer;
	mHealthTimer = NULL;

	// LLView destructor cleans up children
}

BOOL LLStatusBar::postBuild()
{
	childSetAction("scriptout", onClickScriptDebug, this);
	childSetAction("health", onClickHealth, this);
	childSetAction("fly", onClickFly, this);
	childSetAction("buyland", onClickBuyLand, this );
	childSetAction("buycurrency", onClickBuyCurrency, this );
	childSetAction("build", onClickBuild, this );
	childSetAction("scripts", onClickScripts, this );
	childSetAction("restrictpush", onClickPush, this );

	childSetActionTextbox("ParcelNameText", onClickParcelInfo );
	childSetActionTextbox("BalanceText", onClickBalance );

	return TRUE;
}

EWidgetType LLStatusBar::getWidgetType() const
{
	return WIDGET_TYPE_STATUS_BAR;
}

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

	LLView::draw();
}


// Per-frame updates of visibility
void LLStatusBar::refresh()
{
	F32 bwtotal = gViewerThrottle.getMaxBandwidth() / 1000.f;
	mSGBandwidth->setMin(0.f);
	mSGBandwidth->setMax(bwtotal*1.25f);
	mSGBandwidth->setThreshold(0, bwtotal*0.75f);
	mSGBandwidth->setThreshold(1, bwtotal);
	mSGBandwidth->setThreshold(2, bwtotal);

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
	mTextTime->setText(t.str().c_str());

	LLRect r;
	const S32 MENU_RIGHT = gMenuBarView->getRightmostMenuEdge();
	S32 x = MENU_RIGHT + MENU_PARCEL_SPACING;
	S32 y = 0;

	LLViewerRegion *region = gAgent.getRegion();
	LLParcel *parcel = gParcelMgr->getAgentParcel();

	LLRect buttonRect;

	if (LLHUDIcon::iconsNearby())
	{
		childGetRect( "scriptout", buttonRect );
		r.setOriginAndSize( x, y, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnScriptOut->setRect(r);
		mBtnScriptOut->setVisible(TRUE);
		x += buttonRect.getWidth();
	}
	else
	{
		mBtnScriptOut->setVisible(FALSE);
	}

	if ((region && region->getAllowDamage()) ||
		(parcel && parcel->getAllowDamage()) )
	{
		// set visibility based on flashing
		if( mHealthTimer->hasExpired() )
		{
			mBtnHealth->setVisible( TRUE );
		}
		else
		{
			BOOL flash = S32(mHealthTimer->getElapsedSeconds() * ICON_FLASH_FREQUENCY) & 1;
			mBtnHealth->setVisible( flash );
		}
		mTextHealth->setVisible(TRUE);

		// Health
		childGetRect( "health", buttonRect );
		r.setOriginAndSize( x, y-GRAPHIC_FUDGE, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnHealth->setRect(r);
		x += buttonRect.getWidth();

		const S32 health_width = S32( LLFontGL::sSansSerifSmall->getWidth("100%") );
		r.set(x, y+TEXT_HEIGHT - 2, x+health_width, y);
		mTextHealth->setRect(r);
		x += health_width;
	}
	else
	{
		// invisible if region doesn't allow damage
		mBtnHealth->setVisible(FALSE);
		mTextHealth->setVisible(FALSE);
	}

	if ((region && region->getBlockFly()) ||
		(parcel && !parcel->getAllowFly()) )
	{
		// No Fly Zone
		childGetRect( "fly", buttonRect );
		mBtnFly->setVisible(TRUE);
		r.setOriginAndSize( x, y-GRAPHIC_FUDGE, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnFly->setRect(r);
		x += buttonRect.getWidth();
	}
	else
	{
		mBtnFly->setVisible(FALSE);
	}

	BOOL no_build = parcel && !parcel->getAllowModify();
	mBtnBuild->setVisible( no_build );
	if (no_build)
	{
		childGetRect( "build", buttonRect );
		// No Build Zone
		r.setOriginAndSize( x, y-GRAPHIC_FUDGE, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnBuild->setRect(r);
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
	mBtnScripts->setVisible( no_scripts );
	if (no_scripts)
	{
		// No scripts
		childGetRect( "scripts", buttonRect );
		r.setOriginAndSize( x, y-GRAPHIC_FUDGE, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnScripts->setRect(r);
		x += buttonRect.getWidth();
	}

	BOOL no_region_push = (region && region->getRestrictPushObject());
	BOOL no_push = no_region_push || (parcel && parcel->getRestrictPushObject());
	mBtnPush->setVisible( no_push );
	if (no_push)
	{
		childGetRect( "restrictpush", buttonRect );
		// No Push Zone
		r.setOriginAndSize( x, y-GRAPHIC_FUDGE, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnPush->setRect(r);
		x += buttonRect.getWidth();
	}

	BOOL canBuyLand = parcel
		&& !parcel->isPublic()
		&& gParcelMgr->canAgentBuyParcel(parcel, false);
	mBtnBuyLand->setVisible(canBuyLand);
	if (canBuyLand)
	{
		childGetRect( "buyland", buttonRect );
		r.setOriginAndSize( x, y, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnBuyLand->setRect(r);
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

		if (parcel && parcel->getName())
		{
			location_name = region->getName()
				+ llformat(" %d, %d, %d (%s) - %s", 
						   pos_x, pos_y, pos_z,
						   region->getSimAccessString(),
						   parcel->getName());
		}
		else
		{
			location_name = region->getName()
				+ llformat(" %d, %d, %d (%s)", 
						   pos_x, pos_y, pos_z,
						   region->getSimAccessString());
		}
	}
	else
	{
		// no region
		location_name = "(Unknown)";
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
	mSGBandwidth->setVisible(visible);
	mSGPacketLoss->setVisible(visible);
	mBtnBuyCurrency->setVisible(visible);
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
	LLString balance_str;
	gResMgr->getMonetaryString( balance_str, balance );
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
	char buffer[MAX_STRING];		/* Flawfinder: ignore */
	snprintf(buffer, MAX_STRING, "%d%%", health);		/* Flawfinder: ignore */
	//llinfos << "Setting health to: " << buffer << llendl;
	mTextHealth->setText(buffer);

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

// static
void LLStatusBar::onClickParcelInfo(void* data)
{
	gParcelMgr->selectParcelAt(gAgent.getPositionGlobal());

	LLFloaterLand::show();
}

// static
void LLStatusBar::onClickBalance(void* data)
{
	LLFloaterBuyCurrency::buyCurrency();
}

// static
void LLStatusBar::onClickBuyCurrency(void* data)
{
	LLFloaterBuyCurrency::buyCurrency();
}

// static
void LLStatusBar::onClickHealth(void* )
{
	LLNotifyBox::showXml("NotSafe");
}

// static
void LLStatusBar::onClickScriptDebug(void*)
{
	LLFloaterScriptDebug::show(LLUUID::null);
}

// static
void LLStatusBar::onClickFly(void* )
{
	LLNotifyBox::showXml("NoFly");
}

// static
void LLStatusBar::onClickPush(void* )
{
	LLNotifyBox::showXml("PushRestricted");
}

// static
void LLStatusBar::onClickBuild(void*)
{
	LLNotifyBox::showXml("NoBuild");
}

// static
void LLStatusBar::onClickScripts(void*)
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

// static
void LLStatusBar::onClickBuyLand(void*)
{
	gParcelMgr->selectParcelAt(gAgent.getPositionGlobal());
	gParcelMgr->startBuyLand();
}
