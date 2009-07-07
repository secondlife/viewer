/** 
* @file llchiclet.h
* @brief LLChiclet class header file
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

#ifndef LL_LLCHICLET_H
#define LL_LLCHICLET_H

#include "llpanel.h"

class LLTextBox;
class LLIconCtrl;
class LLAvatarIconCtrl;
class LLVoiceControlPanel;
class LLOutputMonitorCtrl;

class LLChiclet : public LLUICtrl
{
public:

	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Params(){};
	};

	virtual ~LLChiclet();

	virtual void setCounter(S32 counter) = 0;

	virtual S32 getCounter() = 0;

	virtual void setShowCounter(bool show) {mShowCounter = show;};

	virtual bool getShowCounter() {return mShowCounter;};

	virtual boost::signals2::connection setLeftButtonClickCallback(
		const commit_callback_t& cb);

protected:

	friend class LLUICtrlFactory;
	LLChiclet(const Params& p);

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);

protected:
	S32 mCounter;
	bool mShowCounter;
};

class LLIMChiclet : public LLChiclet
{
public:
	static LLChiclet* create(LLSD* imSessionId = NULL);

	void setCounter(S32);

	S32 getCounter() {return mCounter;};

	const LLSD& getIMSessionId() const {return mIMSessionId;};

	void setIMSessionId(LLSD* imSessionId) {if (imSessionId) mIMSessionId = *imSessionId;};
	void setIMSessionName(const std::string& name);
	void setOtherParticipantId(const LLUUID& other_participant_id);

	void setShowSpeaker(bool show);

	bool getShowSpeaker() {return mShowSpeaker;};

	enum SpeakerStatus
	{
		SPREAKER_ACTIVE,
		SPEAKER_IDLE
	};

	void setSpeakerStatus(SpeakerStatus status);

	SpeakerStatus getSpeakerStatus() {return mSpeakerStatus;};

	~LLIMChiclet();

protected:
	LLIMChiclet(const LLChiclet::Params& p);
	friend class LLUICtrlFactory;

	S32 calcCounterWidth();

	//overrides
public:

	void setShowCounter(bool show);

	void draw();

	LLRect getRequiredRect();

protected:
	LLAvatarIconCtrl* mAvatar;
	LLTextBox* mCounterText;
	LLIconCtrl* mSpeaker;

	LLSD mIMSessionId;
	bool mShowSpeaker;
	SpeakerStatus mSpeakerStatus;
};

class LLNotificationChiclet : public LLChiclet
{
public:

	struct Params : public LLInitParam::Block<Params, LLChiclet::Params>
	{
		Optional<LLUIImage*>	
			image_unselected,
			image_selected,
			image_hover_selected,
			image_hover_unselected,
			image_disabled_selected,
			image_disabled,
			image_overlay;

		Optional<S32>			
			label_left;

		Params();
	};

	static LLChiclet* create(const Params& p);

	void setCounter(S32 counter);

	S32 getCounter() {return mCounter;};

	boost::signals2::connection setClickCallback(const commit_callback_t& cb);

	virtual ~ LLNotificationChiclet();

protected:
	LLNotificationChiclet(const Params& p);
	friend class LLUICtrlFactory;

protected:
	LLButton* mButton;
	LLTextBox* mCounterText;
};

class LLChicletPanel : public LLPanel
{
public:

	struct Params :	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Params(){};
	};

	~LLChicletPanel();

	LLChiclet* createChiclet(LLSD* imSessionId = NULL, S32 pos = 0);

	bool addChiclet(LLChiclet*, S32 pos);

	LLChiclet* getChiclet(S32 pos);

	LLChiclet* findIMChiclet(const LLSD* imSessionId);

	S32 getChicletCount() {return mChicletList.size();};

	void removeChiclet(S32 pos);

	void removeChiclet(LLChiclet*);

	void removeIMChiclet(const LLSD* imSessionId);

	void removeAll();

	void scrollLeft();

	void scrollRight();

	void onLeftScrollClick();

	void onRightScrollClick();

	boost::signals2::connection setChicletClickCallback(
		const commit_callback_t& cb);

	void onChicletClick(LLUICtrl*ctrl,const LLSD&param);

	//overrides
public:
	/*virtual*/ BOOL postBuild();

	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE );

	void draw();

protected:
	LLChicletPanel(const Params&p);
	friend class LLUICtrlFactory;

	void arrange();

	bool canScrollRight();

	bool canScrollLeft();

	void showScrollButtonsIfNeeded();

	S32 getFirstVisibleChiclet();

	void reshapeScrollArea(S32 delta_width);

	enum ScrollDirection
	{
		SCROLL_LEFT = 1,
		SCROLL_RIGHT = -1
	};

	void scroll(ScrollDirection direction);

	typedef std::vector<LLChiclet*> chiclet_list_t;

	void removeChiclet(chiclet_list_t::iterator it);

	BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);

protected:

	chiclet_list_t mChicletList;
	LLButton* mLeftScroll;
	LLButton* mRightScroll;
	LLPanel* mScrollArea;
};


class LLTalkButton : public LLUICtrl
{
public:

	virtual ~LLTalkButton();

	void onClick_SpeakBtn();
	void onClick_ShowBtn();

	void draw();

protected:
	friend class LLUICtrlFactory;
	LLTalkButton(const LLUICtrl::Params& p);

private:
	LLButton*	mSpeakBtn;
	LLButton*	mShowBtn;
	LLVoiceControlPanel* mPrivateCallPanel;
	LLOutputMonitorCtrl* mOutputMonitor;
};

#endif // LL_LLCHICLET_H
