/** 
 * @file llfloaterlagmeter.cpp
 * @brief The "Lag-o-Meter" floater used to tell users what is causing lag.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
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

#include "llfloaterlagmeter.h"

#include "llvieweruictrlfactory.h"
#include "llviewerstats.h"
#include "llviewerimage.h"
#include "llviewercontrol.h"
#include "llappviewer.h"

#include "lltexturefetch.h"

#include "llbutton.h"
#include "llfocusmgr.h"
#include "lltextbox.h"

const LLString LAG_CRITICAL_IMAGE_NAME = "lag_status_critical.tga";
const LLString LAG_WARNING_IMAGE_NAME  = "lag_status_warning.tga";
const LLString LAG_GOOD_IMAGE_NAME     = "lag_status_good.tga";

LLFloaterLagMeter * LLFloaterLagMeter::sInstance = NULL;

LLFloaterLagMeter::LLFloaterLagMeter()
	:	LLFloater("floater_lagmeter")
{
	gUICtrlFactory->buildFloater(this, "floater_lagmeter.xml");

	// Don't let this window take keyboard focus -- it's confusing to
	// lose arrow-key driving when testing lag.
	setIsChrome(TRUE);

	mClientButton = LLUICtrlFactory::getButtonByName(this, "client_lagmeter");
	mClientText = LLUICtrlFactory::getTextBoxByName(this, "client_text");
	mClientCause = LLUICtrlFactory::getTextBoxByName(this, "client_lag_cause");

	mNetworkButton = LLUICtrlFactory::getButtonByName(this, "network_lagmeter");
	mNetworkText = LLUICtrlFactory::getTextBoxByName(this, "network_text");
	mNetworkCause = LLUICtrlFactory::getTextBoxByName(this, "network_lag_cause");

	mServerButton = LLUICtrlFactory::getButtonByName(this, "server_lagmeter");
	mServerText = LLUICtrlFactory::getTextBoxByName(this, "server_text");
	mServerCause = LLUICtrlFactory::getTextBoxByName(this, "server_lag_cause");

	LLString config_string = childGetText("client_frame_rate_critical_fps");
	mClientFrameTimeCritical = 1.0f / (float)atof( config_string.c_str() );
	config_string = childGetText("client_frame_rate_warning_fps");
	mClientFrameTimeWarning = 1.0f / (float)atof( config_string.c_str() );

	config_string = childGetText("network_packet_loss_critical_pct");
	mNetworkPacketLossCritical = (float)atof( config_string.c_str() );
	config_string = childGetText("network_packet_loss_warning_pct");
	mNetworkPacketLossWarning = (float)atof( config_string.c_str() );

	config_string = childGetText("network_ping_critical_ms");
	mNetworkPingCritical = (float)atof( config_string.c_str() );
	config_string = childGetText("network_ping_warning_ms");
	mNetworkPingWarning = (float)atof( config_string.c_str() );
	config_string = childGetText("server_frame_rate_critical_fps");

	mServerFrameTimeCritical = 1000.0f / (float)atof( config_string.c_str() );
	config_string = childGetText("server_frame_rate_warning_fps");
	mServerFrameTimeWarning = 1000.0f / (float)atof( config_string.c_str() );
	config_string = childGetText("server_single_process_max_time_ms");
	mServerSingleProcessMaxTime = (float)atof( config_string.c_str() );

	mShrunk = false;
	config_string = childGetText("max_width_px");
	mMaxWidth = atoi( config_string.c_str() );
	config_string = childGetText("min_width_px");
	mMinWidth = atoi( config_string.c_str() );

	childSetTextArg("client_frame_time_critical_msg", "[CLIENT_FRAME_RATE_CRITICAL]", childGetText("client_frame_rate_critical_fps"));
	childSetTextArg("client_frame_time_warning_msg", "[CLIENT_FRAME_RATE_CRITICAL]", childGetText("client_frame_rate_critical_fps"));
	childSetTextArg("client_frame_time_warning_msg", "[CLIENT_FRAME_RATE_WARNING]", childGetText("client_frame_rate_warning_fps"));

	childSetTextArg("network_packet_loss_critical_msg", "[NETWORK_PACKET_LOSS_CRITICAL]", childGetText("network_packet_loss_critical_pct"));
	childSetTextArg("network_packet_loss_warning_msg", "[NETWORK_PACKET_LOSS_CRITICAL]", childGetText("network_packet_loss_critical_pct"));
	childSetTextArg("network_packet_loss_warning_msg", "[NETWORK_PACKET_LOSS_WARNING]", childGetText("network_packet_loss_warning_pct"));

	childSetTextArg("network_ping_critical_msg", "[NETWORK_PING_CRITICAL]", childGetText("network_ping_critical_ms"));
	childSetTextArg("network_ping_warning_msg", "[NETWORK_PING_CRITICAL]", childGetText("network_ping_critical_ms"));
	childSetTextArg("network_ping_warning_msg", "[NETWORK_PING_WARNING]", childGetText("network_ping_warning_ms"));

	childSetTextArg("server_frame_time_critical_msg", "[SERVER_FRAME_RATE_CRITICAL]", childGetText("server_frame_rate_critical_fps"));
	childSetTextArg("server_frame_time_warning_msg", "[SERVER_FRAME_RATE_CRITICAL]", childGetText("server_frame_rate_critical_fps"));
	childSetTextArg("server_frame_time_warning_msg", "[SERVER_FRAME_RATE_WARNING]", childGetText("server_frame_rate_warning_fps"));

	childSetAction("minimize", onClickShrink, this);

	// were we shrunk last time?
	if (gSavedSettings.getBOOL("LagMeterShrunk"))
	{
		onClickShrink(this);
	}
}

LLFloaterLagMeter::~LLFloaterLagMeter()
{
	sInstance = NULL;

	// save shrunk status for next time
	gSavedSettings.setBOOL("LagMeterShrunk", mShrunk);
	// expand so we save the large window rectangle
	if (mShrunk)
	{
		onClickShrink(this);
	}
}

void LLFloaterLagMeter::draw()
{
	determineClient();
	determineNetwork();
	determineServer();

	LLFloater::draw();
}

//static
void LLFloaterLagMeter::show(void *data)
{
	if(!sInstance) sInstance = new LLFloaterLagMeter();
	sInstance->open();
}

void LLFloaterLagMeter::determineClient()
{
	F32 client_frame_time = gViewerStats->mFPSStat.getMeanDuration();
	bool find_cause = false;

	if (!gFocusMgr.getAppHasFocus())
	{
		mClientButton->setImageUnselected(LAG_GOOD_IMAGE_NAME);
		mClientText->setText( childGetText("client_frame_time_window_bg_msg") );
		mClientCause->setText( LLString::null );
	}
	else if(client_frame_time >= mClientFrameTimeCritical)
	{
		mClientButton->setImageUnselected(LAG_CRITICAL_IMAGE_NAME);
		mClientText->setText( childGetText("client_frame_time_critical_msg") );
		find_cause = true;
	}
	else if(client_frame_time >= mClientFrameTimeWarning)
	{
		mClientButton->setImageUnselected(LAG_WARNING_IMAGE_NAME);
		mClientText->setText( childGetText("client_frame_time_warning_msg") );
		find_cause = true;
	}
	else
	{
		mClientButton->setImageUnselected(LAG_GOOD_IMAGE_NAME);
		mClientText->setText( childGetText("client_frame_time_normal_msg") );
		mClientCause->setText( LLString::null );
	}	

	if(find_cause)
	{
		if(gSavedSettings.getF32("RenderFarClip") > 128)
		{
			mClientCause->setText( childGetText("client_draw_distance_cause_msg") );
		}
		else if(LLAppViewer::instance()->getTextureFetch()->getNumRequests() > 2)
		{
			mClientCause->setText( childGetText("client_texture_loading_cause_msg") );
		}
		else if(LLViewerImage::sBoundTextureMemory > LLViewerImage::sMaxBoundTextureMem)
		{
			mClientCause->setText( childGetText("client_texture_memory_cause_msg") );
		}
		else 
		{
			mClientCause->setText( childGetText("client_complex_objects_cause_msg") );
		}
	}
}

void LLFloaterLagMeter::determineNetwork()
{
	F32 packet_loss = gViewerStats->mPacketsLostPercentStat.getMean();
	F32 ping_time = gViewerStats->mSimPingStat.getMean();
	bool find_cause_loss = false;
	bool find_cause_ping = false;

	if(packet_loss >= mNetworkPacketLossCritical)
	{
		mNetworkButton->setImageUnselected(LAG_CRITICAL_IMAGE_NAME);
		mNetworkText->setText( childGetText("network_packet_loss_critical_msg") );
		find_cause_loss = true;
	}
	else if(ping_time >= mNetworkPingCritical)
	{
		mNetworkButton->setImageUnselected(LAG_CRITICAL_IMAGE_NAME);
		mNetworkText->setText( childGetText("network_ping_critical_msg") );
		find_cause_ping = true;
	}
	else if(packet_loss >= mNetworkPacketLossWarning)
	{
		mNetworkButton->setImageUnselected(LAG_WARNING_IMAGE_NAME);
		mNetworkText->setText( childGetText("network_packet_loss_warning_msg") );
		find_cause_loss = true;
	}
	else if(ping_time >= mNetworkPingWarning)
	{
		mNetworkButton->setImageUnselected(LAG_WARNING_IMAGE_NAME);
		mNetworkText->setText( childGetText("network_ping_warning_msg") );
		find_cause_ping = true;
	}
	else
	{
		mNetworkButton->setImageUnselected(LAG_GOOD_IMAGE_NAME);
		mNetworkText->setText( childGetText("network_performance_normal_msg") );
	}

	if(find_cause_loss)
 	{
		mNetworkCause->setText( childGetText("network_packet_loss_cause_msg") );
 	}
	else if(find_cause_ping)
	{
		mNetworkCause->setText( childGetText("network_ping_cause_msg") );
	}
	else
	{
		mNetworkCause->setText( LLString::null );
	}
}

void LLFloaterLagMeter::determineServer()
{
	F32 sim_frame_time = gViewerStats->mSimFrameMsec.getCurrent();
	bool find_cause = false;

	if(sim_frame_time >= mServerFrameTimeCritical)
	{
		mServerButton->setImageUnselected(LAG_CRITICAL_IMAGE_NAME);
		mServerText->setText( childGetText("server_frame_time_critical_msg") );
		find_cause = true;
	}
	else if(sim_frame_time >= mServerFrameTimeWarning)
	{
		mServerButton->setImageUnselected(LAG_WARNING_IMAGE_NAME);
		mServerText->setText( childGetText("server_frame_time_warning_msg") );
		find_cause = true;
	}
	else
	{
		mServerButton->setImageUnselected(LAG_GOOD_IMAGE_NAME);
		mServerText->setText( childGetText("server_frame_time_normal_msg") );
		mServerCause->setText( LLString::null );
	}	

	if(find_cause)
	{
		if(gViewerStats->mSimSimPhysicsMsec.getCurrent() > mServerSingleProcessMaxTime)
		{
			mServerCause->setText( childGetText("server_physics_cause_msg") );
		}
		else if(gViewerStats->mSimScriptMsec.getCurrent() > mServerSingleProcessMaxTime)
		{
			mServerCause->setText( childGetText("server_scripts_cause_msg") );
		}
		else if(gViewerStats->mSimNetMsec.getCurrent() > mServerSingleProcessMaxTime)
		{
			mServerCause->setText( childGetText("server_net_cause_msg") );
		}
		else if(gViewerStats->mSimAgentMsec.getCurrent() > mServerSingleProcessMaxTime)
		{
			mServerCause->setText( childGetText("server_agent_cause_msg") );
		}
		else if(gViewerStats->mSimImagesMsec.getCurrent() > mServerSingleProcessMaxTime)
		{
			mServerCause->setText( childGetText("server_images_cause_msg") );
		}
		else
		{
			mServerCause->setText( childGetText("server_generic_cause_msg") );
		}
	}
}

//static
void LLFloaterLagMeter::onClickShrink(void * data)
{
	LLFloaterLagMeter * self = (LLFloaterLagMeter*)data;

	LLButton * button = (LLButton*)self->getChildByName("minimize");
	S32 delta_width = self->mMaxWidth - self->mMinWidth;
	LLRect r = self->getRect();
	if(self->mShrunk)
	{
		self->setTitle( self->childGetText("max_title_msg") );
		// make left edge appear to expand
		r.translate(-delta_width, 0);
		self->setRect(r);
		self->reshape(self->mMaxWidth, self->getRect().getHeight());
		
		self->childSetText("client", self->childGetText("client_text_msg") + ":");
		self->childSetText("network", self->childGetText("network_text_msg") + ":");
		self->childSetText("server", self->childGetText("server_text_msg") + ":");

		// usually "<<"
		button->setLabel( self->childGetText("smaller_label") );
	}
	else
	{
		self->setTitle( self->childGetText("min_title_msg") );
		// make left edge appear to collapse
		r.translate(delta_width, 0);
		self->setRect(r);
		self->reshape(self->mMinWidth, self->getRect().getHeight());
		
		self->childSetText("client", self->childGetText("client_text_msg") );
		self->childSetText("network", self->childGetText("network_text_msg") );
		self->childSetText("server", self->childGetText("server_text_msg") );

		// usually ">>"
		button->setLabel( self->childGetText("bigger_label") );
	}
	// Don't put keyboard focus on the button
	button->setFocus(FALSE);

	self->mClientText->setVisible(self->mShrunk);
	self->mClientCause->setVisible(self->mShrunk);
	self->childSetVisible("client_help", self->mShrunk);

	self->mNetworkText->setVisible(self->mShrunk);
	self->mNetworkCause->setVisible(self->mShrunk);
	self->childSetVisible("network_help", self->mShrunk);

	self->mServerText->setVisible(self->mShrunk);
	self->mServerCause->setVisible(self->mShrunk);
	self->childSetVisible("server_help", self->mShrunk);

	self->mShrunk = !self->mShrunk;
}
