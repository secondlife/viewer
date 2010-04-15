/** 
 * @file lltoastpanel.h
 * @brief Creates a panel of a specific kind for a toast.
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

#ifndef LL_LLTOASTPANEL_H
#define LL_LLTOASTPANEL_H

#include "llpanel.h"
#include "lltextbox.h"
#include "llnotificationptr.h"

#include <string>

class LLToastPanelBase: public LLPanel 
{
public:
	virtual void init(LLSD& data){};
};

/**
 * Base class for all panels that can be added to the toast.
 * All toast panels should contain necessary logic for representing certain notification
 * but shouldn't contain logic related to this panel lifetime control and positioning
 * on the parent view.
 */
class LLToastPanel: public LLPanel {
public:
	LLToastPanel(const LLNotificationPtr&);
	virtual ~LLToastPanel() = 0;

	virtual std::string getTitle();
	virtual const LLUUID& getID();

	static const S32 MIN_PANEL_HEIGHT;

	/**
	 * Builder method for constructing notification specific panels.
	 * Normally type of created panels shouldn't be publicated and should be hidden
	 * from other functionality.
	 */
	static LLToastPanel* buidPanelFromNotification(
			const LLNotificationPtr& notification);
protected:
	LLNotificationPtr mNotification;
	void snapToMessageHeight(LLTextBase* message, S32 maxLineCount);
};

#endif /* LL_TOASTPANEL_H */
