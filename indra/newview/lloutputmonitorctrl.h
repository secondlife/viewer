/** 
 * @file lloutputmonitorctrl.h
 * @brief LLOutputMonitorCtrl base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLOUTPUTMONITORCTRL_H
#define LL_LLOUTPUTMONITORCTRL_H

#include "v4color.h"
#include "llview.h"
#include "llmutelist.h"
#include "llspeakingindicatormanager.h"
#include "lluiimage.h"

class LLTextBox;
class LLUICtrlFactory;

//
// Classes
//

class LLOutputMonitorCtrl
: public LLView, public LLSpeakingIndicator, LLMuteListObserver
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<bool>	draw_border;
		Mandatory<LLUIImage*>	image_mute,
								image_off,
								image_on,
								image_level_1,
								image_level_2,
								image_level_3;
		Optional<bool>		auto_update;
		Optional<LLUUID>	speaker_id;

		Params();
	};
protected:
	bool	mBorder;
	LLOutputMonitorCtrl(const Params&);
	friend class LLUICtrlFactory;

public:
	virtual ~LLOutputMonitorCtrl();

	// llview overrides
	virtual void	draw();

	void			setPower(F32 val);
	F32				getPower(F32 val) const { return mPower; }
	
	bool			getIsMuted() const { return mIsMuted; }
	void			setIsMuted(bool val) { mIsMuted = val; }

	// For the current user, need to know the PTT state to show
	// correct button image.
	void			setIsAgentControl(bool val) { mIsAgentControl = val; }

	void			setIsTalking(bool val) { mIsTalking = val; }

	/**
	 * Sets avatar UUID to interact with voice channel.
	 *
	 * @param speaker_id LLUUID of an avatar whose voice level is displayed.
	 * @param session_id session UUID for which indicator should be shown only. Passed to LLSpeakingIndicatorManager
	 *		If this parameter is set registered indicator will be shown only in voice channel
	 *		which has the same session id (EXT-5562).
	 */
	void			setSpeakerId(const LLUUID& speaker_id, const LLUUID& session_id = LLUUID::null);

	//called by mute list
	virtual void onChange();

	/**
	 * Implementation of LLSpeakingIndicator interface.
	 * Behavior is implemented via changing visibility.
	 *
	 * If instance is in visible chain now (all parents are visible) it changes visibility 
	 * and notify parent about this.
	 *
	 * Otherwise it marks an instance as dirty and stores necessary visibility.
	 * It will be applied in next draw and parent will be notified.
	 */
	virtual void	switchIndicator(bool switch_on);

private:

	/**
	 * Notifies parent about changed visibility.
	 *
	 * Passes LLSD with "visibility_changed" => <current visibility> value.
	 * For now it is processed by LLAvatarListItem to update (reshape) its children.
	 * Implemented fo complete EXT-3976
	 */
	void			notifyParentVisibilityChanged();

	//static LLColor4	sColorMuted;
	//static LLColor4	sColorNormal;
	//static LLColor4	sColorOverdriven;
	static LLColor4	sColorBound;
	//static S32		sRectsNumber;
	//static F32		sRectWidthRatio;
	//static F32		sRectHeightRatio;
	
	

	F32				mPower;
	bool			mIsAgentControl;
	bool			mIsMuted;
	bool			mIsTalking;
	LLPointer<LLUIImage> mImageMute;
	LLPointer<LLUIImage> mImageOff;
	LLPointer<LLUIImage> mImageOn;
	LLPointer<LLUIImage> mImageLevel1;
	LLPointer<LLUIImage> mImageLevel2;
	LLPointer<LLUIImage> mImageLevel3;

	/** whether to deal with LLVoiceClient::getInstance() directly */
	bool			mAutoUpdate;

	/** uuid of a speaker being monitored */
	LLUUID			mSpeakerId;

	/** indicates if the instance is dirty and should notify parent */
	bool			mIsSwitchDirty;
	bool			mShouldSwitchOn;
};

#endif
