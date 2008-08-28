/** 
 * @file llfloaterhud.h
 * @brief A floater showing the HUD tutorial
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * Copyright (c) 2008, Linden Research, Inc.
 * $/LicenseInfo$
 */

#ifndef LL_LLFLOATERHUD_H
#define LL_LLFLOATERHUD_H

#include "llfloater.h"

class LLWebBrowserCtrl;

class LLFloaterHUD : public LLFloater
{
public:
	static LLFloaterHUD* getInstance(); ///< get instance creating if necessary

	static void showHUD(); ///< show the HUD

	// Save our visibility state during close
	/*virtual*/ void onClose(bool app_quitting);

private:
	// Handles its own construction and destruction, so private.
	LLFloaterHUD();
	/*virtual*/ ~LLFloaterHUD();

private:
	LLWebBrowserCtrl* mWebBrowser; ///< the actual web browser control
	static LLFloaterHUD* sInstance;
};

#endif // LL_LLFLOATERHUD_H
