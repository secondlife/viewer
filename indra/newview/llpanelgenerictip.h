/** 
 * @file llpanelgenerictip.h
 * @brief Represents a generic panel for a notifytip notifications. As example:
 * "SystemMessageTip", "Cancelled", "UploadWebSnapshotDone".
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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

#ifndef LL_PANELGENERICTIP_H
#define LL_PANELGENERICTIP_H

#include "llpaneltiptoast.h"

/**
 * Represents tip toast panel that contains only one child element - message text.
 * This panel can be used for different cases of tip notifications.
 */
class LLPanelGenericTip: public LLPanelTipToast
{
	// disallow instantiation of this class
private:
	// grant privileges to instantiate this class to LLToastPanel
	friend class LLToastPanel;
	/**
	 * Generic toast tip panel.
	 * This is particular case of toast panel that decoupled from LLToastNotifyPanel.
	 * From now LLToastNotifyPanel is deprecated and will be removed after all  panel
	 * types are represented in separate classes.
	 */
	LLPanelGenericTip(const LLNotificationPtr& notification);
};
#endif /* LL_PANELGENERICTIP_H */
