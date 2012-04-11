/** 
* @file llchicletbar.h
* @brief LLChicletBar class header file
*
* $LicenseInfo:firstyear=2011&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLCHICLETBAR_H
#define LL_LLCHICLETBAR_H

#include "llpanel.h"
#include "llimview.h"

class LLChicletPanel;
class LLIMChiclet;
class LLLayoutPanel;
class LLLayoutStack;

class LLChicletBar
	: public LLSingleton<LLChicletBar>
	, public LLPanel
	, public LLIMSessionObserver
{
	LOG_CLASS(LLChicletBar);
	friend class LLSingleton<LLChicletBar>;
public:
	~LLChicletBar();

	BOOL postBuild();

	LLChicletPanel*	getChicletPanel() { return mChicletPanel; }

	// LLIMSessionObserver observe triggers
	virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	virtual void sessionRemoved(const LLUUID& session_id);
	void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id);

	S32 getTotalUnreadIMCount();

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent);

	/**
	 * Creates IM Chiclet based on session type (IM chat or Group chat)
	 */
	LLIMChiclet* createIMChiclet(const LLUUID& session_id);

	/**
	 * Shows/hides panel with specified well button (IM or Notification)
	 *
	 * @param well_name - name of the well panel to be processed.
	 * @param visible - a flag specifying whether a button should be shown or hidden.
	 */
	void showWellButton(const std::string& well_name, bool visible);

private:
	/**
	 * Updates child controls size and visibility when it is necessary to reduce total width.
	 *
	 * @param delta_width - value by which chiclet bar should be shrunk. It is a negative value.
	 * @returns positive value which chiclet bar can not process when it reaches its minimal width.
	 *		Zero if there was enough space to process delta_width.
	 */
	S32 processWidthDecreased(S32 delta_width);

	/** helper function to log debug messages */
	void log(LLView* panel, const std::string& descr);

	/**
	 * @return difference between current chiclet panel width and the minimum.
	 */
	S32 getChicletPanelShrinkHeadroom() const;

	/**
	 * function adjusts Chiclet bar width to prevent overlapping with Mini-Location bar
	 * EXP-1463
	 */
	void fitWithTopInfoBar();

protected:
	LLChicletBar(const LLSD& key = LLSD());

	LLChicletPanel* 	mChicletPanel;
	LLLayoutStack*		mToolbarStack;
};

#endif // LL_LLCHICLETBAR_H
