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
#include "llmediactrl.h"	// LLMediaCtrlObserver
#include <boost/scoped_ptr.hpp>

class LLLineEditor;
class LLUIImage;
class LLPanelLoginListener;
class LLSLURL;
class LLCredential;

class LLPanelLogin:	
	public LLPanel,
	public LLViewerMediaObserver
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

	// Show the XUI first name, last name, and password widgets.  They are
	// hidden on startup for reg-in-client
	static void showLoginWidgets();

	static void show(const LLRect &rect, BOOL show_server, 
		void (*callback)(S32 option, void* user_data), 
		void* callback_data);

	static void setFields(LLPointer<LLCredential> credential, BOOL remember);

	static void getFields(LLPointer<LLCredential>& credential, BOOL remember);

	static BOOL isGridComboDirty();
	static BOOL areCredentialFieldsDirty();
	static void setLocation(const LLSLURL& slurl);
	
	static void updateLocationCombo(bool force_visible);  // simply update the combo box
	static void closePanel();

	void setSiteIsAlive( bool alive );

	static void loadLoginPage();	
	static void giveFocus();
	static void setAlwaysRefresh(bool refresh); 
	
	// inherited from LLViewerMediaObserver
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);
	static void updateServer();  // update the combo box, change the login page to the new server, clear the combo

private:
	friend class LLPanelLoginListener;
	void reshapeBrowser();
	static void onClickConnect(void*);
	static void onClickNewAccount(void*);
//	static bool newAccountAlertCallback(const LLSD& notification, const LLSD& response);
	static void onClickVersion(void*);
	static void onClickForgotPassword(void*);
	static void onClickHelp(void*);
	static void onPassKey(LLLineEditor* caller, void* user_data);
	static void onSelectServer(LLUICtrl*, void*);
	static void onServerComboLostFocus(LLFocusableElement*);
	static void updateServerCombo();
	static void updateStartSLURL();
	
	static void updateLoginPanelLinks();

private:
	LLPointer<LLUIImage> mLogoImage;
	boost::scoped_ptr<LLPanelLoginListener> mListener;

	void			(*mCallback)(S32 option, void *userdata);
	void*			mCallbackData;

	BOOL            mPasswordModified;

	static LLPanelLogin* sInstance;
	static BOOL		sCapslockDidNotification;
	BOOL			mHtmlAvailable;
};

std::string load_password_from_disk(void);
void save_password_to_disk(const char* hashed_password);

#endif
