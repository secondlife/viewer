/** 
 * @file llpanellogin.h
 * @brief Login username entry fields.
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

#ifndef LL_LLPANELLOGIN_H
#define LL_LLPANELLOGIN_H

#include "llpanel.h"
#include "llpointer.h"			// LLPointer<>
#include "llwebbrowserctrl.h"	// LLWebBrowserCtrlObserver

class LLUIImage;


class LLPanelLogin:	
	public LLPanel,
	public LLWebBrowserCtrlObserver
{
	LOG_CLASS(LLPanelLogin);
public:
	LLPanelLogin(const LLRect &rect, BOOL show_server, 
				void (*callback)(S32 option, void* user_data),
				void *callback_data);
	~LLPanelLogin();

	virtual BOOL handleKeyHere(KEY key, MASK mask);
	virtual void draw();
	virtual void setFocus( BOOL b );

	static void show(const LLRect &rect, BOOL show_server, 
		void (*callback)(S32 option, void* user_data), 
		void* callback_data);

	// Remember password checkbox is set via gSavedSettings "RememberPassword"
	static void setFields(const std::string& firstname, const std::string& lastname, 
		const std::string& password);

	static void addServer(const std::string& server, S32 domain_name);
	static void refreshLocation( bool force_visible );

	static void getFields(std::string *firstname, std::string *lastname,
						  std::string *password);

	static BOOL isGridComboDirty();
	static void getLocation(std::string &location);

	static void closePanel();

	void setSiteIsAlive( bool alive );

	static void loadLoginPage();	
	static void giveFocus();
	static void setAlwaysRefresh(bool refresh); 
	static void mungePassword(LLUICtrl* caller, void* user_data);
	
private:
	static void onClickConnect(void*);
	static void onClickNewAccount(void*);
//	static bool newAccountAlertCallback(const LLSD& notification, const LLSD& response);
	static void onClickQuit(void*);
	static void onClickVersion(void*);
	virtual void onNavigateComplete( const EventType& eventIn );
	static void onClickForgotPassword(void*);
	static void onPassKey(LLLineEditor* caller, void* user_data);
	static void onSelectServer(LLUICtrl*, void*);
	static void onServerComboLostFocus(LLFocusableElement*, void*);
	
private:
	LLPointer<LLUIImage> mLogoImage;

	void			(*mCallback)(S32 option, void *userdata);
	void*			mCallbackData;

	std::string mIncomingPassword;
	std::string mMungedPassword;

	static LLPanelLogin* sInstance;
	static BOOL		sCapslockDidNotification;
	BOOL			mHtmlAvailable;
};

std::string load_password_from_disk(void);
void save_password_to_disk(const char* hashed_password);

#endif
