/** 
 * @file llfloaterlagmeter.cpp
 * @brief The "Lag-o-Meter" floater used to tell users what is causing lag.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llfloaterlagmeter.h"

#include "lluictrlfactory.h"
#include "llviewerstats.h"
#include "llviewertexture.h"
#include "llviewercontrol.h"
#include "llappviewer.h"

#include "lltexturefetch.h"

#include "llbutton.h"
#include "llfocusmgr.h"
#include "lltextbox.h"

const std::string LAG_CRITICAL_IMAGE_NAME = "lag_status_critical.tga";
const std::string LAG_WARNING_IMAGE_NAME  = "lag_status_warning.tga";
const std::string LAG_GOOD_IMAGE_NAME     = "lag_status_good.tga";

LLFloaterLagMeter::LLFloaterLagMeter(const LLSD& key)
	:	LLFloater(key)
{
	mCommitCallbackRegistrar.add("LagMeter.ClickShrink",  boost::bind(&LLFloaterLagMeter::onClickShrink, this));	
}

BOOL LLFloaterLagMeter::postBuild()
{
	// Don't let this window take keyboard focus -- it's confusing to
	// lose arrow-key driving when testing lag.
	setIsChrome(TRUE);
	
	// were we shrunk last time?
	if (isShrunk())
	{
		onClickShrink();
	}
	
	mClientButton = getChild<LLButton>("client_lagmeter");
	mClientText = getChild<LLTextBox>("client_text");
	mClientCause = getChild<LLTextBox>("client_lag_cause");

	mNetworkButton = getChild<LLButton>("network_lagmeter");
	mNetworkText = getChild<LLTextBox>("network_text");
	mNetworkCause = getChild<LLTextBox>("network_lag_cause");

	mServerButton = getChild<LLButton>("server_lagmeter");
	mServerText = getChild<LLTextBox>("server_text");
	mServerCause = getChild<LLTextBox>("server_lag_cause");

	std::string config_string = getString("client_frame_rate_critical_fps", mStringArgs);
	mClientFrameTimeCritical = 1.0f / (float)atof( config_string.c_str() );
	config_string = getString("client_frame_rate_warning_fps", mStringArgs);
	mClientFrameTimeWarning = 1.0f / (float)atof( config_string.c_str() );

	config_string = getString("network_packet_loss_critical_pct", mStringArgs);
	mNetworkPacketLossCritical = (float)atof( config_string.c_str() );
	config_string = getString("network_packet_loss_warning_pct", mStringArgs);
	mNetworkPacketLossWarning = (float)atof( config_string.c_str() );

	config_string = getString("network_ping_critical_ms", mStringArgs);
	mNetworkPingCritical = (float)atof( config_string.c_str() );
	config_string = getString("network_ping_warning_ms", mStringArgs);
	mNetworkPingWarning = (float)atof( config_string.c_str() );
	config_string = getString("server_frame_rate_critical_fps", mStringArgs);

	mServerFrameTimeCritical = 1000.0f / (float)atof( config_string.c_str() );
	config_string = getString("server_frame_rate_warning_fps", mStringArgs);
	mServerFrameTimeWarning = 1000.0f / (float)atof( config_string.c_str() );
	config_string = getString("server_single_process_max_time_ms", mStringArgs);
	mServerSingleProcessMaxTime = (float)atof( config_string.c_str() );

//	mShrunk = false;
	config_string = getString("max_width_px", mStringArgs);
	mMaxWidth = atoi( config_string.c_str() );
	config_string = getString("min_width_px", mStringArgs);
	mMinWidth = atoi( config_string.c_str() );

	mStringArgs["[CLIENT_FRAME_RATE_CRITICAL]"] = getString("client_frame_rate_critical_fps");
	mStringArgs["[CLIENT_FRAME_RATE_WARNING]"] = getString("client_frame_rate_warning_fps");

	mStringArgs["[NETWORK_PACKET_LOSS_CRITICAL]"] = getString("network_packet_loss_critical_pct");
	mStringArgs["[NETWORK_PACKET_LOSS_WARNING]"] = getString("network_packet_loss_warning_pct");

	mStringArgs["[NETWORK_PING_CRITICAL]"] = getString("network_ping_critical_ms");
	mStringArgs["[NETWORK_PING_WARNING]"] = getString("network_ping_warning_ms");

	mStringArgs["[SERVER_FRAME_RATE_CRITICAL]"] = getString("server_frame_rate_critical_fps");
	mStringArgs["[SERVER_FRAME_RATE_WARNING]"] = getString("server_frame_rate_warning_fps");

//	childSetAction("minimize", onClickShrink, this);
	updateControls(isShrunk()); // if expanded append colon to the labels (EXT-4079)

	return TRUE;
}
LLFloaterLagMeter::~LLFloaterLagMeter()
{
	// save shrunk status for next time
//	gSavedSettings.setBOOL("LagMeterShrunk", mShrunk);
	// expand so we save the large window rectangle
	if (isShrunk())
	{
		onClickShrink();
	}
}

void LLFloaterLagMeter::draw()
{
	determineClient();
	determineNetwork();
	determineServer();

	LLFloater::draw();
}

void LLFloaterLagMeter::determineClient()
{
	F32 client_frame_time = LLViewerStats::getInstance()->mFPSStat.getMeanDuration();
	bool find_cause = false;

	if (!gFocusMgr.getAppHasFocus())
	{
		mClientButton->setImageUnselected(LLUI::getUIImage(LAG_GOOD_IMAGE_NAME));
		mClientText->setText( getString("client_frame_time_window_bg_msg", mStringArgs) );
		mClientCause->setText( LLStringUtil::null );
	}
	else if(client_frame_time >= mClientFrameTimeCritical)
	{
		mClientButton->setImageUnselected(LLUI::getUIImage(LAG_CRITICAL_IMAGE_NAME));
		mClientText->setText( getString("client_frame_time_critical_msg", mStringArgs) );
		find_cause = true;
	}
	else if(client_frame_time >= mClientFrameTimeWarning)
	{
		mClientButton->setImageUnselected(LLUI::getUIImage(LAG_WARNING_IMAGE_NAME));
		mClientText->setText( getString("client_frame_time_warning_msg", mStringArgs) );
		find_cause = true;
	}
	else
	{
		mClientButton->setImageUnselected(LLUI::getUIImage(LAG_GOOD_IMAGE_NAME));
		mClientText->setText( getString("client_frame_time_normal_msg", mStringArgs) );
		mClientCause->setText( LLStringUtil::null );
	}	

	if(find_cause)
	{
		if(gSavedSettings.getF32("RenderFarClip") > 128)
		{
			mClientCause->setText( getString("client_draw_distance_cause_msg", mStringArgs) );
		}
		else if(LLAppViewer::instance()->getTextureFetch()->getNumRequests() > 2)
		{
			mClientCause->setText( getString("client_texture_loading_cause_msg", mStringArgs) );
		}
		else if((BYTES_TO_MEGA_BYTES(LLViewerTexture::sBoundTextureMemoryInBytes)) > LLViewerTexture::sMaxBoundTextureMemInMegaBytes)
		{
			mClientCause->setText( getString("client_texture_memory_cause_msg", mStringArgs) );
		}
		else 
		{
			mClientCause->setText( getString("client_complex_objects_cause_msg", mStringArgs) );
		}
	}
}

void LLFloaterLagMeter::determineNetwork()
{
	F32 packet_loss = LLViewerStats::getInstance()->mPacketsLostPercentStat.getMean();
	F32 ping_time = LLViewerStats::getInstance()->mSimPingStat.getMean();
	bool find_cause_loss = false;
	bool find_cause_ping = false;

	// *FIXME: We can't blame a large ping time on anything in
	// particular if the frame rate is low, because a low frame
	// rate is a sure recipe for bad ping times right now until
	// the network handlers are de-synched from the rendering.
	F32 client_frame_time_ms = 1000.0f * LLViewerStats::getInstance()->mFPSStat.getMeanDuration();
	
	if(packet_loss >= mNetworkPacketLossCritical)
	{
		mNetworkButton->setImageUnselected(LLUI::getUIImage(LAG_CRITICAL_IMAGE_NAME));
		mNetworkText->setText( getString("network_packet_loss_critical_msg", mStringArgs) );
		find_cause_loss = true;
	}
	else if(ping_time >= mNetworkPingCritical)
	{
		mNetworkButton->setImageUnselected(LLUI::getUIImage(LAG_CRITICAL_IMAGE_NAME));
		if (client_frame_time_ms < mNetworkPingCritical)
		{
			mNetworkText->setText( getString("network_ping_critical_msg", mStringArgs) );
			find_cause_ping = true;
		}
	}
	else if(packet_loss >= mNetworkPacketLossWarning)
	{
		mNetworkButton->setImageUnselected(LLUI::getUIImage(LAG_WARNING_IMAGE_NAME));
		mNetworkText->setText( getString("network_packet_loss_warning_msg", mStringArgs) );
		find_cause_loss = true;
	}
	else if(ping_time >= mNetworkPingWarning)
	{
		mNetworkButton->setImageUnselected(LLUI::getUIImage(LAG_WARNING_IMAGE_NAME));
		if (client_frame_time_ms < mNetworkPingWarning)
		{
			mNetworkText->setText( getString("network_ping_warning_msg", mStringArgs) );
			find_cause_ping = true;
		}
	}
	else
	{
		mNetworkButton->setImageUnselected(LLUI::getUIImage(LAG_GOOD_IMAGE_NAME));
		mNetworkText->setText( getString("network_performance_normal_msg", mStringArgs) );
	}

	if(find_cause_loss)
 	{
		mNetworkCause->setText( getString("network_packet_loss_cause_msg", mStringArgs) );
 	}
	else if(find_cause_ping)
	{
		mNetworkCause->setText( getString("network_ping_cause_msg", mStringArgs) );
	}
	else
	{
		mNetworkCause->setText( LLStringUtil::null );
	}
}

void LLFloaterLagMeter::determineServer()
{
	F32 sim_frame_time = LLViewerStats::getInstance()->mSimFrameMsec.getCurrent();
	bool find_cause = false;

	if(sim_frame_time >= mServerFrameTimeCritical)
	{
		mServerButton->setImageUnselected(LLUI::getUIImage(LAG_CRITICAL_IMAGE_NAME));
		mServerText->setText( getString("server_frame_time_critical_msg", mStringArgs) );
		find_cause = true;
	}
	else if(sim_frame_time >= mServerFrameTimeWarning)
	{
		mServerButton->setImageUnselected(LLUI::getUIImage(LAG_WARNING_IMAGE_NAME));
		mServerText->setText( getString("server_frame_time_warning_msg", mStringArgs) );
		find_cause = true;
	}
	else
	{
		mServerButton->setImageUnselected(LLUI::getUIImage(LAG_GOOD_IMAGE_NAME));
		mServerText->setText( getString("server_frame_time_normal_msg", mStringArgs) );
		mServerCause->setText( LLStringUtil::null );
	}	

	if(find_cause)
	{
		if(LLViewerStats::getInstance()->mSimSimPhysicsMsec.getCurrent() > mServerSingleProcessMaxTime)
		{
			mServerCause->setText( getString("server_physics_cause_msg", mStringArgs) );
		}
		else if(LLViewerStats::getInstance()->mSimScriptMsec.getCurrent() > mServerSingleProcessMaxTime)
		{
			mServerCause->setText( getString("server_scripts_cause_msg", mStringArgs) );
		}
		else if(LLViewerStats::getInstance()->mSimNetMsec.getCurrent() > mServerSingleProcessMaxTime)
		{
			mServerCause->setText( getString("server_net_cause_msg", mStringArgs) );
		}
		else if(LLViewerStats::getInstance()->mSimAgentMsec.getCurrent() > mServerSingleProcessMaxTime)
		{
			mServerCause->setText( getString("server_agent_cause_msg", mStringArgs) );
		}
		else if(LLViewerStats::getInstance()->mSimImagesMsec.getCurrent() > mServerSingleProcessMaxTime)
		{
			mServerCause->setText( getString("server_images_cause_msg", mStringArgs) );
		}
		else
		{
			mServerCause->setText( getString("server_generic_cause_msg", mStringArgs) );
		}
	}
}

void LLFloaterLagMeter::updateControls(bool shrink)
{
//	LLFloaterLagMeter * self = (LLFloaterLagMeter*)data;

	LLButton * button = getChild<LLButton>("minimize");
	S32 delta_width = mMaxWidth -mMinWidth;
	LLRect r = getRect();

	if(!shrink)
	{
		setTitle(getString("max_title_msg", mStringArgs) );
		// make left edge appear to expand
		r.translate(-delta_width, 0);
		setRect(r);
		reshape(mMaxWidth, getRect().getHeight());
		
		getChild<LLUICtrl>("client")->setValue(getString("client_text_msg", mStringArgs) + ":");
		getChild<LLUICtrl>("network")->setValue(getString("network_text_msg",mStringArgs) + ":");
		getChild<LLUICtrl>("server")->setValue(getString("server_text_msg", mStringArgs) + ":");

		// usually "<<"
		button->setLabel( getString("smaller_label", mStringArgs) );
	}
	else
	{
		setTitle( getString("min_title_msg", mStringArgs) );
		// make left edge appear to collapse
		r.translate(delta_width, 0);
		setRect(r);
		reshape(mMinWidth, getRect().getHeight());
		
		getChild<LLUICtrl>("client")->setValue(getString("client_text_msg", mStringArgs) );
		getChild<LLUICtrl>("network")->setValue(getString("network_text_msg",mStringArgs) );
		getChild<LLUICtrl>("server")->setValue(getString("server_text_msg", mStringArgs) );

		// usually ">>"
		button->setLabel( getString("bigger_label", mStringArgs) );
	}
	// Don't put keyboard focus on the button
	button->setFocus(FALSE);

//	self->mClientText->setVisible(self->mShrunk);
//	self->mClientCause->setVisible(self->mShrunk);
//	self->getChildView("client_help")->setVisible( self->mShrunk);

//	self->mNetworkText->setVisible(self->mShrunk);
//	self->mNetworkCause->setVisible(self->mShrunk);
//	self->getChildView("network_help")->setVisible( self->mShrunk);

//	self->mServerText->setVisible(self->mShrunk);
//	self->mServerCause->setVisible(self->mShrunk);
//	self->getChildView("server_help")->setVisible( self->mShrunk);

//	self->mShrunk = !self->mShrunk;
}

BOOL LLFloaterLagMeter::isShrunk()
{
	return gSavedSettings.getBOOL("LagMeterShrunk");
}

void LLFloaterLagMeter::onClickShrink()  // toggle "LagMeterShrunk"
{
	bool shrunk = isShrunk();
	updateControls(!shrunk);
	gSavedSettings.setBOOL("LagMeterShrunk", !shrunk);
}
