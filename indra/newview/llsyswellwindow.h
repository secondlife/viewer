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

#include "llfloater.h"
#include "llbutton.h"
#include "llscreenchannel.h"
#include "llscrollcontainer.h"
#include "llchiclet.h"
#include "llimview.h"

#include "boost/shared_ptr.hpp"



class LLSysWellWindow : public LLFloater, LLIMSessionObserver
{
public:
    LLSysWellWindow(const LLSD& key);
    ~LLSysWellWindow();
	BOOL postBuild();

	// other interface functions
	bool isWindowEmpty();

	// change attributes
	void setChannel(LLNotificationsUI::LLScreenChannel*	channel) {mChannel = channel;}
	void setSysWell(LLNotificationChiclet*	sys_well) {mSysWell = sys_well;}

	// Operating with items
	void addItem(LLSysWellItem::Params p);
    void clear( void );
	void removeItemByID(const LLUUID& id);
	S32	 findItemByID(const LLUUID& id);

	// Operating with outfit
	virtual void setVisible(BOOL visible);
	void adjustWindowPosition();
	void toggleWindow();

	// Handlers
	void onItemClick(LLSysWellItem* item);
	void onItemClose(LLSysWellItem* item);

	// size constants for the window and for its elements
	static const S32 MAX_WINDOW_HEIGHT		= 200;
	static const S32 MIN_WINDOW_WIDTH		= 320;
	static const S32 MIN_PANELLIST_WIDTH	= 318;

private:
	class RowPanel;
	void reshapeWindow();
	RowPanel * findIMRow(const LLUUID& sessionId);
	LLChiclet * findIMChiclet(const LLUUID& sessionId);
	void addIMRow(const LLUUID& sessionId, S32 chicletCounter, const std::string& name, const LLUUID& otherParticipantId);
	void delIMRow(const LLUUID& sessionId);
	// LLIMSessionObserver observe triggers
	virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	virtual void sessionRemoved(const LLUUID& session_id);

	// pointer to a corresponding channel's instance
	LLNotificationsUI::LLScreenChannel*	mChannel;

	LLNotificationChiclet*	mSysWell;
	LLUIImagePtr			mDockTongue;
	LLPanel*				mTwinListPanel;
	LLScrollContainer*		mScrollContainer;
	LLScrollingPanelList*	mIMRowList;
	LLScrollingPanelList*	mNotificationList;

private:
	/**
	 * Scrolling row panel.
	 */
	class RowPanel: public LLScrollingPanel
	{
	public:
		RowPanel(const LLSysWellWindow* parent, const LLUUID& sessionId, S32 chicletCounter,
				const std::string& name, const LLUUID& otherParticipantId);
		virtual ~RowPanel();
		/*virtual*/
		void updatePanel(BOOL allow_modify);
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



