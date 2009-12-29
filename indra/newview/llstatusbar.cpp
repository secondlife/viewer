/** 
* @file llstatusbar.cpp
* @brief LLStatusBar class implementation
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

#include "llstatusbar.h"

// viewer includes
#include "llagent.h"
#include "llbutton.h"
#include "llcommandhandler.h"
#include "llviewercontrol.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterchat.h"
#include "llfloaterlagmeter.h"
#include "llfloatervolumepulldown.h"
#include "llfloaterregioninfo.h"
#include "llfloaterscriptdebug.h"
#include "llhudicon.h"
#include "llnavigationbar.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llsd.h"
#include "lltextbox.h"
#include "llui.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llframetimer.h"
#include "llvoavatarself.h"
#include "llresmgr.h"
#include "llworld.h"
#include "llstatgraph.h"
#include "llviewermenu.h"	// for gMenuBarView
#include "llviewerparcelmgr.h"
#include "llviewerthrottle.h"
#include "lluictrlfactory.h"

#include "lltoolmgr.h"
#include "llfocusmgr.h"
#include "llappviewer.h"
#include "lltrans.h"

// library includes
#include "imageids.h"
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llstring.h"
#include "message.h"

// system includes
#include <iomanip>


//
// Globals
//
LLStatusBar *gStatusBar = NULL;
S32 STATUS_BAR_HEIGHT = 26;
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
const S32 TEXT_HEIGHT = 18;

static void onClickBuyCurrency(void*);
static void onClickHealth(void*);
static void onClickScriptDebug(void*);
static void onClickVolume(void*);

std::vector<std::string> LLStatusBar::sDays;
std::vector<std::string> LLStatusBar::sMonths;
const U32 LLStatusBar::MAX_DATE_STRING_LENGTH = 2000;

LLStatusBar::LLStatusBar(const LLRect& rect)
:	LLPanel(),
	mTextHealth(NULL),
	mTextTime(NULL),
	mSGBandwidth(NULL),
	mSGPacketLoss(NULL),
	mBtnBuyCurrency(NULL),
	mBtnVolume(NULL),
	mBalance(0),
	mHealth(100),
	mSquareMetersCredit(0),
	mSquareMetersCommitted(0)
{
	setRect(rect);
	
	// status bar can possible overlay menus?
	setMouseOpaque(FALSE);
	setIsChrome(TRUE);

	// size of day of the weeks and year
	sDays.reserve(7);
	sMonths.reserve(12);

	mBalanceTimer = new LLFrameTimer();
	mHealthTimer = new LLFrameTimer();

	LLUICtrlFactory::getInstance()->buildPanel(this,"panel_status_bar.xml");

	// status bar can never get a tab
	setFocusRoot(FALSE);

	// build date necessary data (must do after panel built)
	setupDate();

	mTextHealth = getChild<LLTextBox>("HealthText" );
	mTextTime = getChild<LLTextBox>("TimeText" );
	
	mBtnBuyCurrency = getChild<LLButton>( "buycurrency" );
	mBtnBuyCurrency->setClickedCallback( onClickBuyCurrency, this );

	mBtnVolume = getChild<LLButton>( "volume_btn" );
	mBtnVolume->setClickedCallback( onClickVolume, this );
	mBtnVolume->setMouseEnterCallback(boost::bind(&LLStatusBar::onMouseEnterVolume, this));
	mBtnVolume->setIsChrome(TRUE);

	gSavedSettings.getControl("MuteAudio")->getSignal()->connect(boost::bind(&LLStatusBar::onVolumeChanged, this, _2));

	childSetAction("scriptout", onClickScriptDebug, this);
	childSetAction("health", onClickHealth, this);

	// Adding Net Stat Graph
	S32 x = getRect().getWidth() - 2;
	S32 y = 0;
	LLRect r;
	r.set( x-SIM_STAT_WIDTH, y+MENU_BAR_HEIGHT-1, x, y+1);
	LLStatGraph::Params sgp;
	sgp.name("BandwidthGraph");
	sgp.rect(r);
	sgp.follows.flags(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	sgp.mouse_opaque(false);
	mSGBandwidth = LLUICtrlFactory::create<LLStatGraph>(sgp);
	mSGBandwidth->setStat(&LLViewerStats::getInstance()->mKBitStat);
	mSGBandwidth->setUnits("Kbps");
	mSGBandwidth->setPrecision(0);
	addChild(mSGBandwidth);
	x -= SIM_STAT_WIDTH + 2;

	r.set( x-SIM_STAT_WIDTH, y+MENU_BAR_HEIGHT-1, x, y+1);
	//these don't seem to like being reused
	LLStatGraph::Params pgp;
	pgp.name("PacketLossPercent");
	pgp.rect(r);
	pgp.follows.flags(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	pgp.mouse_opaque(false);

	mSGPacketLoss = LLUICtrlFactory::create<LLStatGraph>(pgp);
	mSGPacketLoss->setStat(&LLViewerStats::getInstance()->mPacketsLostPercentStat);
	mSGPacketLoss->setUnits("%");
	mSGPacketLoss->setMin(0.f);
	mSGPacketLoss->setMax(5.f);
	mSGPacketLoss->setThreshold(0, 0.5f);
	mSGPacketLoss->setThreshold(1, 1.f);
	mSGPacketLoss->setThreshold(2, 3.f);
	mSGPacketLoss->setPrecision(1);
	mSGPacketLoss->mPerSec = FALSE;
	addChild(mSGPacketLoss);

	childSetActionTextbox("stat_btn", onClickStatGraph);
}

LLStatusBar::~LLStatusBar()
{
	delete mBalanceTimer;
	mBalanceTimer = NULL;

	delete mHealthTimer;
	mHealthTimer = NULL;

	// LLView destructor cleans up children
}

//-----------------------------------------------------------------------
// Overrides
//-----------------------------------------------------------------------

// virtual
void LLStatusBar::draw()
{
	refresh();
	LLPanel::draw();
}

BOOL LLStatusBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	show_navbar_context_menu(this,x,y);
	return TRUE;
}

BOOL LLStatusBar::postBuild()
{
	gMenuBarView->setRightMouseDownCallback(boost::bind(&show_navbar_context_menu, _1, _2, _3));

	return TRUE;
}

// Per-frame updates of visibility
void LLStatusBar::refresh()
{
	bool net_stats_visible = gSavedSettings.getBOOL("ShowNetStats");
	
	if (net_stats_visible)
	{
		// Adding Net Stat Meter back in
		F32 bwtotal = gViewerThrottle.getMaxBandwidth() / 1000.f;
		mSGBandwidth->setMin(0.f);
		mSGBandwidth->setMax(bwtotal*1.25f);
		mSGBandwidth->setThreshold(0, bwtotal*0.75f);
		mSGBandwidth->setThreshold(1, bwtotal);
		mSGBandwidth->setThreshold(2, bwtotal);
	}
	
	// Get current UTC time, adjusted for the user's clock
	// being off.
	time_t utc_time;
	utc_time = time_corrected();

	std::string timeStr = getString("time");
	LLSD substitution;
	substitution["datetime"] = (S32) utc_time;
	LLStringUtil::format (timeStr, substitution);
	mTextTime->setText(timeStr);

	// set the tooltip to have the date
	std::string dtStr = getString("timeTooltip");
	LLStringUtil::format (dtStr, substitution);
	mTextTime->setToolTip (dtStr);

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
	LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

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

	if (gAgent.getCameraMode() == CAMERA_MODE_MOUSELOOK &&
		((region && region->getAllowDamage()) || (parcel && parcel->getAllowDamage())))
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
		r.setOriginAndSize( x, y, buttonRect.getWidth(), buttonRect.getHeight());
		childSetRect("health", r);
		x += buttonRect.getWidth();

		const S32 health_width = S32( LLFontGL::getFontSansSerifSmall()->getWidth(std::string("100%")) );
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

	mSGBandwidth->setVisible(net_stats_visible);
	mSGPacketLoss->setVisible(net_stats_visible);
	childSetEnabled("stat_btn", net_stats_visible);

	// update the master volume button state
	BOOL mute_audio = gSavedSettings.getBOOL("MuteAudio");
	mBtnVolume->setToggleState(mute_audio);
}

void LLStatusBar::setVisibleForMouselook(bool visible)
{
	mTextTime->setVisible(visible);
	mBtnBuyCurrency->setVisible(visible);
	mSGBandwidth->setVisible(visible);
	mSGPacketLoss->setVisible(visible);
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
	std::string money_str = LLResMgr::getInstance()->getMonetaryString( balance );

	LLStringUtil::format_map_t string_args;
	string_args["[AMT]"] = llformat("%s", money_str.c_str());
	std::string labe_str = getString("buycurrencylabel", string_args);
	mBtnBuyCurrency->setLabel(labe_str);

	// Resize the balance button so that the label fits it, and the button expands to the left.
	// *TODO: LLButton should have an option where to expand.
	{
		S32 saved_right = mBtnBuyCurrency->getRect().mRight;
		mBtnBuyCurrency->autoResize();
		mBtnBuyCurrency->translate(saved_right - mBtnBuyCurrency->getRect().mRight, 0);
	}

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


// static
void LLStatusBar::sendMoneyBalanceRequest()
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MoneyBalanceRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MoneyData);
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null );
	gAgent.sendReliableMessage();
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

static void onClickBuyCurrency(void* data)
{
	LLFloaterBuyCurrency::buyCurrency();
}

static void onClickHealth(void* )
{
	LLNotificationsUtil::add("NotSafe");
}

static void onClickScriptDebug(void*)
{
	LLFloaterScriptDebug::show(LLUUID::null);
}

//static
void LLStatusBar::onMouseEnterVolume(LLUICtrl* ctrl)
{
	// show the master volume pull-down
	LLFloaterReg::showInstance("volume_pulldown");
}

static void onClickVolume(void* data)
{
	// toggle the master mute setting
	BOOL mute_audio = gSavedSettings.getBOOL("MuteAudio");
	gSavedSettings.setBOOL("MuteAudio", !mute_audio);
}

// sets the static variables necessary for the date
void LLStatusBar::setupDate()
{
	// fill the day array with what's in the xui
	std::string day_list = getString("StatBarDaysOfWeek");
	size_t length = day_list.size();
	
	// quick input check
	if(length < MAX_DATE_STRING_LENGTH)
	{
		// tokenize it and put it in the array
		std::string cur_word;
		for(size_t i = 0; i < length; ++i)
		{
			if(day_list[i] == ':')
			{
				sDays.push_back(cur_word);
				cur_word.clear();
			}
			else
			{
				cur_word.append(1, day_list[i]);
			}
		}
		sDays.push_back(cur_word);
	}
	
	// fill the day array with what's in the xui	
	std::string month_list = getString( "StatBarMonthsOfYear" );
	length = month_list.size();
	
	// quick input check
	if(length < MAX_DATE_STRING_LENGTH)
	{
		// tokenize it and put it in the array
		std::string cur_word;
		for(size_t i = 0; i < length; ++i)
		{
			if(month_list[i] == ':')
			{
				sMonths.push_back(cur_word);
				cur_word.clear();
			}
			else
			{
				cur_word.append(1, month_list[i]);
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
void LLStatusBar::onClickStatGraph(void* data)
{
	LLFloaterReg::showInstance("lagmeter");
}

BOOL can_afford_transaction(S32 cost)
{
	return((cost <= 0)||((gStatusBar) && (gStatusBar->getBalance() >=cost)));
}

void LLStatusBar::onVolumeChanged(const LLSD& newvalue)
{
	refresh();
}

// Implements secondlife:///app/balance/request to request a L$ balance
// update via UDP message system. JC
class LLBalanceHandler : public LLCommandHandler
{
public:
	// Requires "trusted" browser/URL source
	LLBalanceHandler() : LLCommandHandler("balance", UNTRUSTED_BLOCK) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (tokens.size() == 1
			&& tokens[0].asString() == "request")
		{
			LLStatusBar::sendMoneyBalanceRequest();
			return true;
		}
		return false;
	}
};
// register with command dispatch system
LLBalanceHandler gBalanceHandler;
