/** 
 * @file llpanelavatar.h
 * @brief LLPanelAvatar and related class definitions
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLPANELAVATAR_H
#define LL_LLPANELAVATAR_H

#include "llpanel.h"
#include "llavatarpropertiesprocessor.h"
#include "llcallingcard.h"
#include "llvoiceclient.h"
#include "llavatarnamecache.h"

class LLComboBox;
class LLLineEditor;

/**
* Base class for any Profile View.
*/
class LLPanelProfileTab
	: public LLPanel
	, public LLAvatarPropertiesObserver
{
public:

	/**
	 * Sets avatar ID, sets panel as observer of avatar related info replies from server.
	 */
	virtual void setAvatarId(const LLUUID& id);

	/**
	 * Returns avatar ID.
	 */
	virtual const LLUUID& getAvatarId() { return mAvatarId; }

	/**
	 * Sends update data request to server.
	 */
	virtual void updateData() = 0;

	/**
	 * Clears panel data if viewing avatar info for first time and sends update data request.
	 */
	virtual void onOpen(const LLSD& key);

	/**
	 * Profile tabs should close any opened panels here.
	 *
	 * Called from LLPanelProfile::onOpen() before opening new profile.
	 * See LLPanelPicks::onClosePanel for example. LLPanelPicks closes picture info panel
	 * before new profile is displayed, otherwise new profile will 
	 * be hidden behind picture info panel.
	 */
	virtual void onClosePanel() {}

	/**
	 * Resets controls visibility, state, etc.
	 */
	virtual void resetControls(){};

	/**
	 * Clears all data received from server.
	 */
	virtual void resetData(){};

	/*virtual*/ ~LLPanelProfileTab();

protected:

	LLPanelProfileTab();

	/**
	 * Scrolls panel to top when viewing avatar info for first time.
	 */
	void scrollToTop();

	virtual void onMapButtonClick();

	virtual void updateButtons();

private:

	LLUUID mAvatarId;
};

#endif // LL_LLPANELAVATAR_H
