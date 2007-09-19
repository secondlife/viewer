/** 
 * @file llfloaterlagmeter.cpp
 * @brief The "Lag-o-Meter" floater used to tell users what is causing lag.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterlagmeter.h"

#include "llvieweruictrlfactory.h"
#include "llviewerstats.h"
#include "llviewerimage.h"
#include "llviewercontrol.h"
#include "viewer.h"
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

	mClientButton = LLUICtrlFactory::getButtonByName(this, "client_lagmeter");
	mClientText = LLUICtrlFactory::getTextBoxByName(this, "client_text");
	mClientCause = LLUICtrlFactory::getTextBoxByName(this, "client_lag_cause");

	mNetworkButton = LLUICtrlFactory::getButtonByName(this, "network_lagmeter");
	mNetworkText = LLUICtrlFactory::getTextBoxByName(this, "network_text");
	mNetworkCause = LLUICtrlFactory::getTextBoxByName(this, "network_lag_cause");

	mServerButton = LLUICtrlFactory::getButtonByName(this, "server_lagmeter");
	mServerText = LLUICtrlFactory::getTextBoxByName(this, "server_text");
	mServerCause = LLUICtrlFactory::getTextBoxByName(this, "server_lag_cause");

	childSetFocus("client_help", TRUE);

	LLString config_string = childGetText("client_frame_rate_critical_fps");
	mClientFrameTimeCritical = 1.0f / (float)atof( config_string.c_str() );
	config_string = childGetText("client_frame_rate_warning_fps");
	mClientFrameTimeWarning = 1.0f / (float)atof( config_string.c_str() );

	config_string = childGetText("network_packet_loss_critical_pct");
	mNetworkPacketLossCritical = (float)atof( config_string.c_str() );
	config_string = childGetText("network_packet_loss_warning_pct");
	mNetworkPacketLossWarning = (float)atof( config_string.c_str() );

	config_string = childGetText("server_frame_rate_critical_fps");
	mServerFrameTimeCritical = 1000.0f / (float)atof( config_string.c_str() );
	config_string = childGetText("server_frame_rate_warning_fps");
	mServerFrameTimeWarning = 1000.0f / (float)atof( config_string.c_str() );
	config_string = childGetText("server_single_process_max_time_ms");
	mServerSingleProcessMaxTime = (float)atof( config_string.c_str() );

	mMinimized = false;
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

	childSetTextArg("server_frame_time_critical_msg", "[SERVER_FRAME_RATE_CRITICAL]", childGetText("server_frame_rate_critical_fps"));
	childSetTextArg("server_frame_time_warning_msg", "[SERVER_FRAME_RATE_CRITICAL]", childGetText("server_frame_rate_critical_fps"));
	childSetTextArg("server_frame_time_warning_msg", "[SERVER_FRAME_RATE_WARNING]", childGetText("server_frame_rate_warning_fps"));

	childSetAction("minimize", onClickShrink, this);
}

LLFloaterLagMeter::~LLFloaterLagMeter()
{
	sInstance = NULL;
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
		else if(gTextureFetch->getNumRequests() > 2)
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
	bool find_cause = false;

	if(packet_loss >= mNetworkPacketLossCritical)
	{
		mNetworkButton->setImageUnselected(LAG_CRITICAL_IMAGE_NAME);
		mNetworkText->setText( childGetText("network_packet_loss_critical_msg") );
		find_cause = true;
	}
	else if(packet_loss >= mNetworkPacketLossWarning)
	{
		mNetworkButton->setImageUnselected(LAG_WARNING_IMAGE_NAME);
		mNetworkText->setText( childGetText("network_packet_loss_warning_msg") );
		find_cause = true;
	}
	else
	{
		mNetworkButton->setImageUnselected(LAG_GOOD_IMAGE_NAME);
		mNetworkText->setText( childGetText("network_packet_loss_normal_msg") );
		mNetworkCause->setText( LLString::null );
	}

	if(find_cause)
	{
		mNetworkCause->setText( childGetText("network_cause_msg") );
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

	if(self->mMinimized)
	{
		self->setTitle( self->childGetText("max_title_msg") );
		self->reshape(self->mMaxWidth, self->getRect().getHeight());
		
		self->childSetText("client", self->childGetText("client_text_msg") + ":");
		self->childSetText("network", self->childGetText("network_text_msg") + ":");
		self->childSetText("server", self->childGetText("server_text_msg") + ":");

		LLButton * button = (LLButton*)self->getChildByName("minimize");
		// usually "<<"
		button->setLabel( self->childGetText("smaller_label") );
	}
	else
	{
		self->setTitle( self->childGetText("min_title_msg") );
		self->reshape(self->mMinWidth, self->getRect().getHeight());
		
		self->childSetText("client", self->childGetText("client_text_msg") );
		self->childSetText("network", self->childGetText("network_text_msg") );
		self->childSetText("server", self->childGetText("server_text_msg") );

		LLButton * button = (LLButton*)self->getChildByName("minimize");
		// usually ">>"
		button->setLabel( self->childGetText("bigger_label") );
	}

	self->mClientText->setVisible(self->mMinimized);
	self->mClientCause->setVisible(self->mMinimized);
	self->childSetVisible("client_help", self->mMinimized);

	self->mNetworkText->setVisible(self->mMinimized);
	self->mNetworkCause->setVisible(self->mMinimized);
	self->childSetVisible("network_help", self->mMinimized);

	self->mServerText->setVisible(self->mMinimized);
	self->mServerCause->setVisible(self->mMinimized);
	self->childSetVisible("server_help", self->mMinimized);

	self->mMinimized = !self->mMinimized;
}
