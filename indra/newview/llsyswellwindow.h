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

#include "lldockablefloater.h"
#include "llbutton.h"
#include "llscreenchannel.h"
#include "llscrollcontainer.h"
#include "llchiclet.h"
#include "llimview.h"

#include "boost/shared_ptr.hpp"

class LLFlatListView;

class LLSysWellWindow : public LLDockableFloater, LLIMSessionObserver
{
public:
    LLSysWellWindow(const LLSD& key);
    ~LLSysWellWindow();
	BOOL postBuild();

	// other interface functions
	// check is window empty
	bool isWindowEmpty();

	// Operating with items
	void addItem(LLSysWellItem::Params p);
    void clear( void );
	void removeItemByID(const LLUUID& id);

	// Operating with outfit
	virtual void setVisible(BOOL visible);
	void adjustWindowPosition();
	void toggleWindow();
	/*virtual*/ BOOL	canClose() { return FALSE; }
	/*virtual*/ void	setDocked(bool docked, bool pop_on_undock = true);
	// override LLFloater's minimization according to EXT-1216
	/*virtual*/ void	setMinimized(BOOL minimize);

	// Handlers
	void onItemClick(LLSysWellItem* item);
	void onItemClose(LLSysWellItem* item);
	void onStoreToast(LLPanel* info_panel, LLUUID id);
	void onChicletClick();

	// size constants for the window and for its elements
	static const S32 MAX_WINDOW_HEIGHT		= 200;
	static const S32 MIN_WINDOW_WIDTH		= 318;

private:

	typedef enum{
		IT_NOTIFICATION,
		IT_INSTANT_MESSAGE
	}EItemType; 

	// gets a rect that bounds possible positions for the SysWellWindow on a screen (EXT-1111)
	void getAllowedRect(LLRect& rect);
	// connect counter and list updaters to the corresponding signals
	void connectListUpdaterToSignal(std::string notification_type);
	// init Window's channel
	void initChannel();
	void handleItemAdded(EItemType added_item_type);
	void handleItemRemoved(EItemType removed_item_type);
	bool anotherTypeExists(EItemType item_type) ;



	class RowPanel;
	void reshapeWindow();
	LLChiclet * findIMChiclet(const LLUUID& sessionId);
	void addIMRow(const LLUUID& sessionId, S32 chicletCounter, const std::string& name, const LLUUID& otherParticipantId);
	void delIMRow(const LLUUID& sessionId);
	// LLIMSessionObserver observe triggers
	virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	virtual void sessionRemoved(const LLUUID& session_id);

	// pointer to a corresponding channel's instance
	LLNotificationsUI::LLScreenChannel*	mChannel;
	LLFlatListView*	mMessageList;

	/**
	 *	Special panel which is used as separator of Notifications & IM Rows.
	 *	It is always presents in the list and shown when it is necessary.
	 *	It should be taken into account when reshaping and checking list size
	 */
	LLPanel* mSeparator;

	typedef std::map<EItemType, S32> typed_items_count_t;
	typed_items_count_t mTypedItemsCount;

private:
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
	private:
		void onClose();
	public:
		LLIMChiclet* mChiclet;
	private:
		LLButton*	mCloseBtn;
		const LLSysWellWindow* mParent;
	};
};

#endif // LL_LLSYSWELLWINDOW_H



