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
 *  the GPL can be found in doc/GPL-license.txt in this distribution, or
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

#include "llavatariconctrl.h"
#include "llbutton.h"
#include "llpanel.h"
#include "lltextbox.h"
#include "lloutputmonitorctrl.h"
#include "llgroupmgr.h"
#include "llimview.h"

class LLVoiceControlPanel;
class LLMenuGL;
class LLIMFloater;

/**
 * Class for displaying amount of messages/notifications(unread).
 */
class LLChicletNotificationCounterCtrl : public LLTextBox
{
public:

	struct Params :	public LLInitParam::Block<Params, LLTextBox::Params>
	{
		/**
		* Contains maximum displayed count of unread messages. Default value is 9.
		*
		* If count is less than "max_unread_count" will be displayed as is.
		* Otherwise 9+ will be shown (for default value).
		*/
		Optional<S32> max_displayed_count;

		Params();
	};

	/**
	 * Sets number of notifications
	 */
	virtual void setCounter(S32 counter);

	/**
	 * Returns number of notifications
	 */
	virtual S32 getCounter() const { return mCounter; }

	/**
	 * Returns width, required to display amount of notifications in text form.
	 * Width is the only valid value.
	 */
	/*virtual*/ LLRect getRequiredRect();

	/**
	 * Sets number of notifications using LLSD
	 */
	/*virtual*/ void setValue(const LLSD& value);

	/**
	 * Returns number of notifications wrapped in LLSD
	 */
	/*virtual*/ LLSD getValue() const;

protected:

	LLChicletNotificationCounterCtrl(const Params& p);
	friend class LLUICtrlFactory;

private:

	S32 mCounter;
	S32 mInitialWidth;
	S32 mMaxDisplayedCount;
};

/**
 * Class for displaying avatar's icon in P2P chiclet.
 */
class LLChicletAvatarIconCtrl : public LLAvatarIconCtrl
{
public:

	struct Params :	public LLInitParam::Block<Params, LLAvatarIconCtrl::Params>
	{
		Params()
		{
			draw_tooltip(FALSE);
			mouse_opaque(FALSE);
			default_icon_name("Generic_Person");
		};
	};

protected:

	LLChicletAvatarIconCtrl(const Params& p);
	friend class LLUICtrlFactory;
};

/**
 * Class for displaying group's icon in Group chiclet.
 */
class LLChicletGroupIconCtrl : public LLIconCtrl
{
public:

	struct Params :	public LLInitParam::Block<Params, LLIconCtrl::Params>
	{
		Optional<std::string> default_icon;

		Params()
		 : default_icon("default_icon", "Generic_Group")
		{
		};
	};

	/**
	 * Sets icon, if value is LLUUID::null - default icon will be set.
	 */
	virtual void setValue(const LLSD& value );

protected:

	LLChicletGroupIconCtrl(const Params& p);
	friend class LLUICtrlFactory;

	std::string mDefaultIcon;
};

/**
 * Class for displaying icon in inventory offer chiclet.
 */
class LLChicletInvOfferIconCtrl : public LLChicletAvatarIconCtrl
{
public:

	struct Params :
		public LLInitParam::Block<Params, LLChicletAvatarIconCtrl::Params>
	{
		Optional<std::string> default_icon;

		Params()
		 : default_icon("default_icon", "Generic_Object_Small")
		{
			avatar_id = LLUUID::null;
		};
	};

	/**
	 * Sets icon, if value is LLUUID::null - default icon will be set.
	 */
	virtual void setValue(const LLSD& value );

protected:

	LLChicletInvOfferIconCtrl(const Params& p);
	friend class LLUICtrlFactory;

private:
	std::string mDefaultIcon;
};

/**
 * Class for displaying of speaker's voice indicator 
 */
class LLChicletSpeakerCtrl : public LLOutputMonitorCtrl
{
public:

	struct Params : public LLInitParam::Block<Params, LLOutputMonitorCtrl::Params>
	{
		Params(){};
	};
protected:

	LLChicletSpeakerCtrl(const Params&p);
	friend class LLUICtrlFactory;
};

/**
 * Base class for all chiclets.
 */
class LLChiclet : public LLUICtrl
{
public:

	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<bool> show_counter,
					   enable_counter;

		Params();
	};

	/*virtual*/ ~LLChiclet();

	/**
	 * Associates chat session id with chiclet.
	 */
	virtual void setSessionId(const LLUUID& session_id) { mSessionId = session_id; }

	/**
	 * Returns associated chat session.
	 */
	virtual const LLUUID& getSessionId() const { return mSessionId; }

	/**
	 * Sets number of unread notifications.
	 */
	virtual void setCounter(S32 counter) = 0;

	/**
	 * Returns number of unread notifications.
	 */
	virtual S32 getCounter() = 0;

	/**
	 * Sets show counter state.
	 */
	virtual void setShowCounter(bool show) { mShowCounter = show; }

	/**
	 * Returns show counter state.
	 */
	virtual bool getShowCounter() {return mShowCounter;};

	/**
	 * Connects chiclet clicked event with callback.
	 */
	/*virtual*/ boost::signals2::connection setLeftButtonClickCallback(
		const commit_callback_t& cb);

	typedef boost::function<void (LLChiclet* ctrl, const LLSD& param)> 
		chiclet_size_changed_callback_t;

	/**
	 * Connects chiclets size changed event with callback.
	 */
	virtual boost::signals2::connection setChicletSizeChangedCallback(
		const chiclet_size_changed_callback_t& cb);

	/**
	 * Sets IM Session id using LLSD
	 */
	/*virtual*/ LLSD getValue() const;

	/**
	 * Returns IM Session id using LLSD
	 */
	/*virtual*/ void setValue(const LLSD& value);

protected:

	friend class LLUICtrlFactory;
	LLChiclet(const Params& p);

	/**
	 * Notifies subscribers about click on chiclet.
	 */
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);

	/**
	 * Notifies subscribers about chiclet size changed event.
	 */
	virtual void onChicletSizeChanged();

private:

	LLUUID mSessionId;

	bool mShowCounter;

	typedef boost::signals2::signal<void (LLChiclet* ctrl, const LLSD& param)> 
		chiclet_size_changed_signal_t;

	chiclet_size_changed_signal_t mChicletSizeChangedSignal;
};


/**
 * Base class for Instant Message chiclets.
 * IMChiclet displays icon, number of unread messages(optional)
 * and voice chat status(optional).
 */
class LLIMChiclet : public LLChiclet
{
public:
	enum EType {
		TYPE_UNKNOWN,
		TYPE_IM,
		TYPE_GROUP,
		TYPE_AD_HOC
	};
	struct Params : public LLInitParam::Block<Params, LLChiclet::Params>
	{
		Params(){}
	};

	
	/*virtual*/ ~LLIMChiclet() {};

	/**
	 * Sets IM session name. This name will be displayed in chiclet tooltip.
	 */
	virtual void setIMSessionName(const std::string& name) { setToolTip(name); }

	/**
	 * Sets id of person/group user is chatting with.
	 * Session id should be set before calling this
	 */
	virtual void setOtherParticipantId(const LLUUID& other_participant_id) { mOtherParticipantId = other_participant_id; }

	/**
	 * Gets id of person/group user is chatting with.
	 */
	virtual LLUUID getOtherParticipantId() { return mOtherParticipantId; }

	/**
	 * Init Speaker Control with speaker's ID
	 */
	virtual void initSpeakerControl();

	/**
	 * set status (Shows/Hide) for voice control.
	 */
	virtual void setShowSpeaker(bool show);

	/**
	 * Returns voice chat status control visibility.
	 */
	virtual bool getShowSpeaker() {return mShowSpeaker;};

	/**
	 * Shows/Hides for voice control for a chiclet.
	 */
	virtual void toggleSpeakerControl();

	/**
	* Sets number of unread messages. Will update chiclet's width if number text 
	* exceeds size of counter and notify it's parent about size change.
	*/
	virtual void setCounter(S32);

	/**
	* Enables/disables the counter control for a chiclet.
	*/
	virtual void enableCounterControl(bool enable);

	/**
	* Sets show counter state.
	*/
	virtual void setShowCounter(bool show);

	/**
	* Shows/Hides for counter control for a chiclet.
	*/
	virtual void toggleCounterControl();

	/**
	* Sets required width for a chiclet according to visible controls.
	*/
	virtual void setRequiredWidth();

	/**
	 * Shows/hides overlay icon concerning new unread messages.
	 */
	virtual void setShowNewMessagesIcon(bool show);

	/**
	 * Returns visibility of overlay icon concerning new unread messages.
	 */
	virtual bool getShowNewMessagesIcon();

	virtual void draw();

	/**
	 * Determine whether given ID refers to a group or an IM chat session.
	 * 
	 * This is used when we need to chose what IM chiclet (P2P/group)
	 * class to instantiate.
	 * 
	 * @param session_id session ID.
	 * @return TYPE_GROUP in case of group chat session,
	 *         TYPE_IM in case of P2P session,
	 *         TYPE_UNKNOWN otherwise.
	 */
	static EType getIMSessionType(const LLUUID& session_id);

	/**
	 * The action taken on mouse down event.
	 * 
	 * Made public so that it can be triggered from outside
	 * (more specifically, from the Active IM window).
	 */
	virtual void onMouseDown();

protected:

	LLIMChiclet(const LLIMChiclet::Params& p);

	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);

protected:

	bool mShowSpeaker;
	bool mCounterEnabled;
	/* initial width of chiclet, should not include counter or speaker width */
	S32 mDefaultWidth;

	LLIconCtrl* mNewMessagesIcon;
	LLChicletNotificationCounterCtrl* mCounterCtrl;
	LLChicletSpeakerCtrl* mSpeakerCtrl;


	/** the id of another participant, either an avatar id or a group id*/
	LLUUID mOtherParticipantId;

	template<typename Container>
	struct CollectChicletCombiner {
		typedef Container result_type;

		template<typename InputIterator>
		Container operator()(InputIterator first, InputIterator last) const {
			Container c = Container();
			for (InputIterator iter = first; iter != last; iter++) {
				if (*iter != NULL) {
					c.push_back(*iter);
				}
			}
			return c;
		}
	};

public:
	static boost::signals2::signal<LLChiclet* (const LLUUID&),
			CollectChicletCombiner<std::list<LLChiclet*> > >
			sFindChicletsSignal;
};

/**
 * Implements P2P chiclet.
 */
class LLIMP2PChiclet : public LLIMChiclet
{
public:
	struct Params : public LLInitParam::Block<Params, LLIMChiclet::Params>
	{
		Optional<LLChicletAvatarIconCtrl::Params> avatar_icon;

		Optional<LLChicletNotificationCounterCtrl::Params> unread_notifications;

		Optional<LLChicletSpeakerCtrl::Params> speaker;

		Optional<LLIconCtrl::Params> new_message_icon;

		Optional<bool>	show_speaker;

		Params();
	};

	/* virtual */ void setOtherParticipantId(const LLUUID& other_participant_id);

	/**
	 * Init Speaker Control with speaker's ID
	 */
	/*virtual*/ void initSpeakerControl();

	/**
	 * Returns number of unread messages.
	 */
	/*virtual*/ S32 getCounter() { return mCounterCtrl->getCounter(); }

protected:
	LLIMP2PChiclet(const Params& p);
	friend class LLUICtrlFactory;

	/**
	 * Creates chiclet popup menu. Will create P2P or Group IM Chat menu 
	 * based on other participant's id.
	 */
	virtual void createPopupMenu();

	/**
	 * Processes clicks on chiclet popup menu.
	 */
	virtual void onMenuItemClicked(const LLSD& user_data);

	/**
	 * Displays popup menu.
	 */
	/*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	/** 
	 * Enables/disables menus based on relationship with other participant.
	 * Enables/disables "show session" menu item depending on visible IM floater existence.
	 */
	virtual void updateMenuItems();

private:

	LLChicletAvatarIconCtrl* mChicletIconCtrl;
	LLMenuGL* mPopupMenu;
};

/**
 * Implements AD-HOC chiclet.
 */
class LLAdHocChiclet : public LLIMChiclet
{
public:
	struct Params : public LLInitParam::Block<Params, LLIMChiclet::Params>
	{
		Optional<LLChicletAvatarIconCtrl::Params> avatar_icon;

		Optional<LLChicletNotificationCounterCtrl::Params> unread_notifications;

		Optional<LLChicletSpeakerCtrl::Params> speaker;

		Optional<LLIconCtrl::Params> new_message_icon;

		Optional<bool>	show_speaker;

		Optional<LLColor4>	avatar_icon_color;

		Params();
	};

	/**
	 * Sets session id.
	 * Session ID for group chat is actually Group ID.
	 */
	/*virtual*/ void setSessionId(const LLUUID& session_id);

	/**
	 * Keep Speaker Control with actual speaker's ID
	 */
	/*virtual*/ void draw();

	/**
	 * Init Speaker Control with speaker's ID
	 */
	/*virtual*/ void initSpeakerControl();

	/**
	 * Returns number of unread messages.
	 */
	/*virtual*/ S32 getCounter() { return mCounterCtrl->getCounter(); }

protected:
	LLAdHocChiclet(const Params& p);
	friend class LLUICtrlFactory;

	/**
	 * Creates chiclet popup menu. Will create AdHoc Chat menu 
	 * based on other participant's id.
	 */
	virtual void createPopupMenu();

	/**
	 * Processes clicks on chiclet popup menu.
	 */
	virtual void onMenuItemClicked(const LLSD& user_data);

	/**
	 * Displays popup menu.
	 */
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	/**
	 * Finds a current speaker and resets the SpeakerControl with speaker's ID
	 */
	/*virtual*/ void switchToCurrentSpeaker();

private:

	LLChicletAvatarIconCtrl* mChicletIconCtrl;
	LLMenuGL* mPopupMenu;
};

/**
 * Chiclet for script floaters.
 */
class LLScriptChiclet : public LLIMChiclet
{
public:

	struct Params : public LLInitParam::Block<Params, LLIMChiclet::Params>
	{
		Optional<LLIconCtrl::Params> icon;

		Optional<LLIconCtrl::Params> new_message_icon;

		Params();
	};

	/*virtual*/ void setSessionId(const LLUUID& session_id);

	/*virtual*/ void setCounter(S32 counter);

	/*virtual*/ S32 getCounter() { return 0; }

	/**
	 * Toggle script floater
	 */
	/*virtual*/ void onMouseDown();

	/**
	 * Override default handler
	 */
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);

protected:

	LLScriptChiclet(const Params&);
	friend class LLUICtrlFactory;

private:

	LLIconCtrl* mChicletIconCtrl;
};

/**
 * Chiclet for inventory offer script floaters.
 */
class LLInvOfferChiclet: public LLIMChiclet
{
public:

	struct Params : public LLInitParam::Block<Params, LLIMChiclet::Params>
	{
		Optional<LLChicletInvOfferIconCtrl::Params> icon;

		Optional<LLIconCtrl::Params> new_message_icon;

		Params();
	};

	/*virtual*/ void setSessionId(const LLUUID& session_id);

	/*virtual*/ void setCounter(S32 counter);

	/*virtual*/ S32 getCounter() { return 0; }

	/**
	 * Toggle script floater
	 */
	/*virtual*/ void onMouseDown();

	/**
	 * Override default handler
	 */
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);


protected:
	LLInvOfferChiclet(const Params&);
	friend class LLUICtrlFactory;

private:
	LLChicletInvOfferIconCtrl* mChicletIconCtrl;
};

/**
 * Implements Group chat chiclet.
 */
class LLIMGroupChiclet : public LLIMChiclet, public LLGroupMgrObserver
{
public:

	struct Params : public LLInitParam::Block<Params, LLIMChiclet::Params>
	{
		Optional<LLChicletGroupIconCtrl::Params> group_icon;

		Optional<LLChicletNotificationCounterCtrl::Params> unread_notifications;

		Optional<LLChicletSpeakerCtrl::Params> speaker;

		Optional<LLIconCtrl::Params> new_message_icon;

		Optional<bool>	show_speaker;

		Params();
	};

	/**
	 * Sets session id.
	 * Session ID for group chat is actually Group ID.
	 */
	/*virtual*/ void setSessionId(const LLUUID& session_id);

	/**
	 * Keep Speaker Control with actual speaker's ID
	 */
	/*virtual*/ void draw();

	/**
	 * Callback for LLGroupMgrObserver, we get this when group data is available or changed.
	 * Sets group icon.
	 */
	/*virtual*/ void changed(LLGroupChange gc);

	/**
	 * Init Speaker Control with speaker's ID
	 */
	/*virtual*/ void initSpeakerControl();

	/**
	 * Returns number of unread messages.
	 */
	/*virtual*/ S32 getCounter() { return mCounterCtrl->getCounter(); }

	~LLIMGroupChiclet();

protected:
	LLIMGroupChiclet(const Params& p);
	friend class LLUICtrlFactory;

	/**
	 * Finds a current speaker and resets the SpeakerControl with speaker's ID
	 */
	/*virtual*/ void switchToCurrentSpeaker();

	/**
	 * Creates chiclet popup menu. Will create P2P or Group IM Chat menu 
	 * based on other participant's id.
	 */
	virtual void createPopupMenu();

	/**
	 * Processes clicks on chiclet popup menu.
	 */
	virtual void onMenuItemClicked(const LLSD& user_data);

	/**
	 * Enables/disables "show session" menu item depending on visible IM floater existence.
	 */
	virtual void updateMenuItems();

	/**
	 * Displays popup menu.
	 */
	/*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

private:

	LLChicletGroupIconCtrl* mChicletIconCtrl;
	LLMenuGL* mPopupMenu;
};

/**
 * Implements notification chiclet. Used to display total amount of unread messages 
 * across all IM sessions, total amount of system notifications. See EXT-3147 for details
 */
class LLSysWellChiclet : public LLChiclet
{
public:

	struct Params : public LLInitParam::Block<Params, LLChiclet::Params>
	{
		Optional<LLButton::Params> button;

		Optional<LLChicletNotificationCounterCtrl::Params> unread_notifications;

		/**
		 * Contains maximum displayed count of unread messages. Default value is 9.
		 *
		 * If count is less than "max_unread_count" will be displayed as is.
		 * Otherwise 9+ will be shown (for default value).
		 */
		Optional<S32> max_displayed_count;

		/**
		 * How many time chiclet should flash before set "Lit" state. Default value is 3.
		 */
		Optional<S32> flash_to_lit_count;

		/**
		 * Period of flashing while setting "Lit" state, in seconds. Default value is 0.5.
		 */
		Optional<F32> flash_period;

		Params();
	};

	/*virtual*/ void setCounter(S32 counter);

	// *TODO: mantipov: seems getCounter is not necessary for LLNotificationChiclet
	// but inherited interface requires it to implement. 
	// Probably it can be safe removed.
	/*virtual*/S32 getCounter() { return mCounter; }

	boost::signals2::connection setClickCallback(const commit_callback_t& cb);

	/*virtual*/ ~LLSysWellChiclet();

	void setToggleState(BOOL toggled);

	void setNewMessagesState(bool new_messages);

protected:

	LLSysWellChiclet(const Params& p);
	friend class LLUICtrlFactory;

	/**
	 * Change Well 'Lit' state from 'Lit' to 'Unlit' and vice-versa.
	 *
	 * There is an assumption that it will be called 2*N times to do not change its start state.
	 * @see FlashToLitTimer
	 */
	void changeLitState();

	/**
	 * Displays menu.
	 */
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	virtual void createMenu() = 0;

protected:
	class FlashToLitTimer;
	LLButton* mButton;
	S32 mCounter;
	S32 mMaxDisplayedCount;
	bool mIsNewMessagesState;

	FlashToLitTimer* mFlashToLitTimer;
	LLContextMenu* mContextMenu;
};

/**
 * Class represented a chiclet for IM Well Icon.
 *
 * It displays a count of unread messages from other participants in all IM sessions.
 */
class LLIMWellChiclet : public LLSysWellChiclet, LLIMSessionObserver
{
	friend class LLUICtrlFactory;
public:
	virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id) {}
	virtual void sessionRemoved(const LLUUID& session_id) { messageCountChanged(LLSD()); }
	virtual void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id) {}

	~LLIMWellChiclet();
protected:
	LLIMWellChiclet(const Params& p);

	/**
	 * Processes clicks on chiclet popup menu.
	 */
	virtual void onMenuItemClicked(const LLSD& user_data);

	/**
	 * Enables chiclet menu items.
	 */
	bool enableMenuItem(const LLSD& user_data);

	/**
	 * Creates menu.
	 */
	/*virtual*/ void createMenu();

	/**
	 * Handles changes in a session (message was added, messages were read, etc.)
	 *
	 * It get total count of unread messages from a LLIMMgr in all opened sessions and display it.
	 *
	 * @param[in] session_data contains session related data, is not used now
	 *		["session_id"] - id of an appropriate session
	 *		["participant_unread"] - count of unread messages from "real" participants.
	 *
	 * @see LLIMMgr::getNumberOfUnreadParticipantMessages()
	 */
	void messageCountChanged(const LLSD& session_data);
};

class LLNotificationChiclet : public LLSysWellChiclet
{
	friend class LLUICtrlFactory;
protected:
	LLNotificationChiclet(const Params& p);

	/**
	 * Processes clicks on chiclet menu.
	 */
	void onMenuItemClicked(const LLSD& user_data);

	/**
	 * Enables chiclet menu items.
	 */
	bool enableMenuItem(const LLSD& user_data);

	/**
	 * Creates menu.
	 */
	/*virtual*/ void createMenu();

	// connect counter updaters to the corresponding signals
	void connectCounterUpdatersToSignal(const std::string& notification_type);

	// methods for updating a number of unread System notifications
	void incUreadSystemNotifications() { setCounter(++mUreadSystemNotifications); }
	void decUreadSystemNotifications() { setCounter(--mUreadSystemNotifications); }

	S32 mUreadSystemNotifications;
};

/**
 * Storage class for all IM chiclets. Provides mechanism to display, 
 * scroll, create, remove chiclets.
 */
class LLChicletPanel : public LLPanel
{
public:

	struct Params :	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<S32> chiclet_padding,
					  scrolling_offset;

		Optional<S32> min_width;

		Params();
	};

	virtual ~LLChicletPanel();

	/**
	 * Creates chiclet and adds it to chiclet list at specified index.
	 */
	template<class T> T* createChiclet(const LLUUID& session_id, S32 index);

	/**
	 * Creates chiclet and adds it to chiclet list at right.
	 */
	template<class T> T* createChiclet(const LLUUID& session_id);

	/**
	 * Returns pointer to chiclet of specified type at specified index.
	 */
	template<class T> T* getChiclet(S32 index);

	/**
	 * Returns pointer to LLChiclet at specified index.
	 */
	LLChiclet* getChiclet(S32 index) { return getChiclet<LLChiclet>(index); }

	/**
	 * Searches a chiclet using IM session id.
	 */
	template<class T> T* findChiclet(const LLUUID& im_session_id);

	/**
	 * Returns number of hosted chiclets.
	 */
	S32 getChicletCount() {return mChicletList.size();};

	/**
	 * Returns index of chiclet in list.
	 */
	S32 getChicletIndex(const LLChiclet* chiclet);

	/**
	 * Removes chiclet by index.
	 */
	void removeChiclet(S32 index);

	/**
	 * Removes chiclet by pointer.
	 */
	void removeChiclet(LLChiclet* chiclet);

	/**
	 * Removes chiclet by IM session id.
	 */
	void removeChiclet(const LLUUID& im_session_id);

	/**
	 * Removes all chiclets.
	 */
	void removeAll();

	/**
	 * Scrolls the panel to the specified chiclet
	 */
	void scrollToChiclet(const LLChiclet* chiclet);

	boost::signals2::connection setChicletClickedCallback(
		const commit_callback_t& cb);

	/*virtual*/ BOOL postBuild();

	/**
	 * Handler for the Voice Client's signal. Finds a corresponding chiclet and toggles its SpeakerControl
	 */
	void onCurrentVoiceChannelChanged(const LLUUID& session_id);

	/**
	 * Reshapes controls and rearranges chiclets if needed.
	 */
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE );

	/*virtual*/ void draw();

	S32 getMinWidth() const { return mMinWidth; }

	S32 getTotalUnreadIMCount();

	S32	notifyParent(const LLSD& info);

protected:
	LLChicletPanel(const Params&p);
	friend class LLUICtrlFactory;

	/**
	 * Adds chiclet to list and rearranges all chiclets. 
	 * They should be right aligned, most recent right. See EXT-1293
	 *
	 * It calculates position of the first chiclet in the list. Other chiclets are placed in arrange().
	 *
	 * @see arrange()
	 */
	bool addChiclet(LLChiclet*, S32 index);

	/**
	 * Arranges chiclets to have them in correct positions.
	 *
	 * Method bases on assumption that first chiclet has correct rect and starts from the its position.
	 *
	 * @see addChiclet()
	 */
	void arrange();

	/**
	 * Returns true if chiclets can be scrolled right.
	 */
	bool canScrollRight();

	/**
	 * Returns true if we need to show scroll buttons
	 */
	bool needShowScroll();

	/**
	 * Returns true if chiclets can be scrolled left.
	 */
	bool canScrollLeft();

	/**
	 * Shows or hides chiclet scroll buttons if chiclets can or can not be scrolled.
	 */
	void showScrollButtonsIfNeeded();

	/**
	 * Shifts chiclets left or right.
	 */
	void shiftChiclets(S32 offset, S32 start_index = 0);

	/**
	 * Removes gaps between first chiclet and scroll area left side,
	 * last chiclet and scroll area right side.
	 */
	void trimChiclets();

	/**
	 * Scrolls chiclets to right or left.
	 */
	void scroll(S32 offset);

	/**
	 * Verifies that chiclets can be scrolled left, then calls scroll()
	 */
	void scrollLeft();

	/**
	 * Verifies that chiclets can be scrolled right, then calls scroll()
	 */
	void scrollRight();

	/**
	 * Callback for left scroll button clicked
	 */
	void onLeftScrollClick();

	/**
	 * Callback for right scroll button clicked
	 */
	void onRightScrollClick();

	/**
	 * Callback for right scroll button held down event
	 */
	void onLeftScrollHeldDown();

	/**
	 * Callback for left scroll button held down event
	 */
	void onRightScrollHeldDown();

	/**
	 * Callback for mouse wheel scrolled, calls scrollRight() or scrollLeft()
	 */
	BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);

	/**
	 * Notifies subscribers about click on chiclet.
	 * Do not place any code here, instead subscribe on event (see setChicletClickedCallback).
	 */
	void onChicletClick(LLUICtrl*ctrl,const LLSD&param);

	/**
	 * Callback for chiclet size changed event, rearranges chiclets.
	 */
	void onChicletSizeChanged(LLChiclet* ctrl, const LLSD& param);

	typedef std::vector<LLChiclet*> chiclet_list_t;

	/**
	 * Removes chiclet from scroll area and chiclet list.
	 */
	void removeChiclet(chiclet_list_t::iterator it);

	S32 getChicletPadding() { return mChicletPadding; }

	S32 getScrollingOffset() { return mScrollingOffset; }

	bool isAnyIMFloaterDoked();

protected:

	chiclet_list_t mChicletList;
	LLButton* mLeftScrollButton;
	LLButton* mRightScrollButton;
	LLPanel* mScrollArea;

	S32 mChicletPadding;
	S32 mScrollingOffset;
	S32 mMinWidth;
	bool mShowControls;
	static const S32 s_scroll_ratio;
};

template<class T> 
T* LLChicletPanel::createChiclet(const LLUUID& session_id, S32 index)
{
	typename T::Params params;
	T* chiclet = LLUICtrlFactory::create<T>(params);
	if(!chiclet)
	{
		llwarns << "Could not create chiclet" << llendl;
		return NULL;
	}
	if(!addChiclet(chiclet, index))
	{
		delete chiclet;
		llwarns << "Could not add chiclet to chiclet panel" << llendl;
		return NULL;
	}

	if (!isAnyIMFloaterDoked())
	{
		scrollToChiclet(chiclet);
	}

	chiclet->setSessionId(session_id);

	return chiclet;
}

template<class T>
T* LLChicletPanel::createChiclet(const LLUUID& session_id)
{
	return createChiclet<T>(session_id, mChicletList.size());
}

template<class T>
T* LLChicletPanel::findChiclet(const LLUUID& im_session_id)
{
	if(im_session_id.isNull())
	{
		return NULL;
	}

	chiclet_list_t::const_iterator it = mChicletList.begin();
	for( ; mChicletList.end() != it; ++it)
	{
		LLChiclet* chiclet = *it;

		if(chiclet->getSessionId() == im_session_id)
		{
			T* result = dynamic_cast<T*>(chiclet);
			if(!result && chiclet)
			{
				llwarns << "Found chiclet but of wrong type " << llendl;
			}
			return result;
		}
	}
	return NULL;
}

template<class T> T* LLChicletPanel::getChiclet(S32 index)
{
	if(index < 0 || index >= getChicletCount())
	{
		return NULL;
	}

	LLChiclet* chiclet = mChicletList[index];
	T*result = dynamic_cast<T*>(chiclet);
	if(!result && chiclet)
	{
		llwarns << "Found chiclet but of wrong type " << llendl;
	}
	return result;
}

#endif // LL_LLCHICLET_H
