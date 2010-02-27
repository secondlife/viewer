/** 
 * @file llsyswellwindow.h
 * @brief                                    // TODO
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LL_LLSYSWELLWINDOW_H
#define LL_LLSYSWELLWINDOW_H

#include "llsyswellitem.h"

#include "lltransientdockablefloater.h"
#include "llbutton.h"
#include "llscreenchannel.h"
#include "llscrollcontainer.h"
#include "llimview.h"

#include "boost/shared_ptr.hpp"

class LLFlatListView;
class LLChiclet;
class LLIMChiclet;
class LLScriptChiclet;
class LLSysWellChiclet;


class LLSysWellWindow : public LLTransientDockableFloater
{
public:
    LLSysWellWindow(const LLSD& key);
    ~LLSysWellWindow();
	BOOL postBuild();

	// other interface functions
	// check is window empty
	bool isWindowEmpty();

	// Operating with items
	void removeItemByID(const LLUUID& id);

	// Operating with outfit
	virtual void setVisible(BOOL visible);
	void adjustWindowPosition();
	/*virtual*/ void	setDocked(bool docked, bool pop_on_undock = true);
	// override LLFloater's minimization according to EXT-1216
	/*virtual*/ void	setMinimized(BOOL minimize);

	void onStartUpToastClick(S32 x, S32 y, MASK mask);

	void setSysWellChiclet(LLSysWellChiclet* chiclet);

	// size constants for the window and for its elements
	static const S32 MAX_WINDOW_HEIGHT		= 200;
	static const S32 MIN_WINDOW_WIDTH		= 318;

protected:

	typedef enum{
		IT_NOTIFICATION,
		IT_INSTANT_MESSAGE
	}EItemType; 

	// gets a rect that bounds possible positions for the SysWellWindow on a screen (EXT-1111)
	void getAllowedRect(LLRect& rect);


	// init Window's channel
	virtual void initChannel();
	void handleItemAdded(EItemType added_item_type);
	void handleItemRemoved(EItemType removed_item_type);
	bool anotherTypeExists(EItemType item_type) ;

	const std::string NOTIFICATION_WELL_ANCHOR_NAME;
	const std::string IM_WELL_ANCHOR_NAME;
	virtual const std::string& getAnchorViewName() = 0;

	void reshapeWindow();
	void releaseNewMessagesState();

	// pointer to a corresponding channel's instance
	LLNotificationsUI::LLScreenChannel*	mChannel;
	LLFlatListView*	mMessageList;

	/**
	 * Reference to an appropriate Well chiclet to release "new message" state. EXT-3147
	 */
	LLSysWellChiclet* mSysWellChiclet;

	/**
	 *	Special panel which is used as separator of Notifications & IM Rows.
	 *	It is always presents in the list and shown when it is necessary.
	 *	It should be taken into account when reshaping and checking list size
	 */
	LLPanel* mSeparator;

	typedef std::map<EItemType, S32> typed_items_count_t;
	typed_items_count_t mTypedItemsCount;

};

/**
 * Class intended to manage incoming notifications.
 * 
 * It contains a list of notifications that have not been responded to.
 */
class LLNotificationWellWindow : public LLSysWellWindow
{
public:
	LLNotificationWellWindow(const LLSD& key);
	static LLNotificationWellWindow* getInstance(const LLSD& key = LLSD());

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setVisible(BOOL visible);

	// Operating with items
	void addItem(LLSysWellItem::Params p);

	// Closes all notifications and removes them from the Notification Well
	void closeAll();

protected:
	/*virtual*/ const std::string& getAnchorViewName() { return NOTIFICATION_WELL_ANCHOR_NAME; }

private:
	// init Window's channel
	void initChannel();
	void clearScreenChannels();


	void onStoreToast(LLPanel* info_panel, LLUUID id);

	// connect counter and list updaters to the corresponding signals
	void connectListUpdaterToSignal(std::string notification_type);

	// Handlers
	void onItemClick(LLSysWellItem* item);
	void onItemClose(LLSysWellItem* item);

	// ID of a toast loaded by user (by clicking notification well item)
	LLUUID mLoadedToastId;

};

/**
 * Class intended to manage incoming messages in IM chats.
 * 
 * It contains a list list of all active IM sessions.
 */
class LLIMWellWindow : public LLSysWellWindow, LLIMSessionObserver, LLInitClass<LLIMWellWindow>
{
public:
	LLIMWellWindow(const LLSD& key);
	~LLIMWellWindow();

	static LLIMWellWindow* getInstance(const LLSD& key = LLSD());
	static void initClass() { getInstance(); }

	/*virtual*/ BOOL postBuild();

	// LLIMSessionObserver observe triggers
	/*virtual*/ void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	/*virtual*/ void sessionRemoved(const LLUUID& session_id);
	/*virtual*/ void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id);

	void addObjectRow(const LLUUID& notification_id, bool new_message = false);
	void removeObjectRow(const LLUUID& notification_id);

	void addIMRow(const LLUUID& session_id);
	bool hasIMRow(const LLUUID& session_id);

	void closeAll();

protected:
	/*virtual*/ const std::string& getAnchorViewName() { return IM_WELL_ANCHOR_NAME; }

private:
	LLChiclet * findIMChiclet(const LLUUID& sessionId);
	LLChiclet* findObjectChiclet(const LLUUID& notification_id);

	void addIMRow(const LLUUID& sessionId, S32 chicletCounter, const std::string& name, const LLUUID& otherParticipantId);
	void delIMRow(const LLUUID& sessionId);
	bool confirmCloseAll(const LLSD& notification, const LLSD& response);
	void closeAllImpl();

	/**
	 * Scrolling row panel.
	 */
	class RowPanel: public LLPanel
	{
	public:
		RowPanel(const LLSysWellWindow* parent, const LLUUID& sessionId, S32 chicletCounter,
				const std::string& name, const LLUUID& otherParticipantId);
		virtual ~RowPanel();
		void onMouseEnter(S32 x, S32 y, MASK mask);
		void onMouseLeave(S32 x, S32 y, MASK mask);
		BOOL handleMouseDown(S32 x, S32 y, MASK mask);
		BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	private:
		static const S32 CHICLET_HPAD = 10;
		void onChicletSizeChanged(LLChiclet* ctrl, const LLSD& param);
		void onClosePanel();
	public:
		LLIMChiclet* mChiclet;
	private:
		LLButton*	mCloseBtn;
		const LLSysWellWindow* mParent;
	};

	class ObjectRowPanel: public LLPanel
	{
	public:
		ObjectRowPanel(const LLUUID& notification_id, bool new_message = false);
		virtual ~ObjectRowPanel();
		/*virtual*/ void onMouseEnter(S32 x, S32 y, MASK mask);
		/*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);
		/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
		/*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	private:
		void onClosePanel();
		void initChiclet(const LLUUID& notification_id, bool new_message = false);

	public:
		LLIMChiclet* mChiclet;
	private:
		LLButton*	mCloseBtn;
	};
};

#endif // LL_LLSYSWELLWINDOW_H



