/** 
 * @file llfloaterhud.h
 * @brief The HUD floater
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * Copyright (c) 2008, Linden Research, Inc.
 * $/LicenseInfo$
 */

#ifndef LL_LLFLOATERHUD_H
#define LL_LLFLOATERHUD_H

#include "llfloater.h"

class LLWebBrowserCtrl;

//=============================================================================
//
//	CLASS		LLFloaterHUD

class LLFloaterHUD : public LLFloater

/*!	@brief		A floater showing the HUD tutorial
*/
{
public:
	static LLFloaterHUD* getInstance(); ///< get instance creating if necessary
	virtual ~LLFloaterHUD(); ///< virtual destructor

	static std::string sTutorialUrl;

	static void show(); ///< show the HUD
	static void close(); ///< close the HUD (destroys floater)

protected:
	LLWebBrowserCtrl* mWebBrowser; ///< the actual web browser control

	LLFloaterHUD(); ///< default constructor
	virtual void onFocusReceived(); ///< called when we get the focus
private:
	static LLFloaterHUD* sInstance;
};

#endif // LL_LLFLOATERHUD_H
