/** 
 * @file llpanellogin.h
 * @brief Login username entry fields.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
	LLPanelLogin(const LLRect &rect,
				void (*callback)(S32 option, void* user_data),
				void *callback_data);
	~LLPanelLogin();

	virtual BOOL handleKeyHere(KEY key, MASK mask);
	virtual void draw();
	virtual void setFocus( BOOL b );

	// Show the XUI first name, last name, and password widgets.  They are
	// hidden on startup for reg-in-client
	static void showLoginWidgets();

	static void show(const LLRect &rect,
		void (*callback)(S32 option, void* user_data), 
		void* callback_data);

	static void setFields(LLPointer<LLCredential> credential, BOOL remember);

	static void getFields(LLPointer<LLCredential>& credential, BOOL& remember);

	static BOOL areCredentialFieldsDirty();
	static void setLocation(const LLSLURL& slurl);
	
	/// Call when preferences that control visibility may have changed
	static void updateLocationSelectorsVisibility();

	static void closePanel();

	void setSiteIsAlive( bool alive );

	static void loadLoginPage();	
	static void giveFocus();
	static void setAlwaysRefresh(bool refresh); 
	
	// inherited from LLViewerMediaObserver
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);
	static void updateServer();  // update the combo box, change the login page to the new server, clear the combo

	/// to be called from LLStartUp::setStartSLURL
	static void onUpdateStartSLURL(const LLSLURL& new_start_slurl);

private:
	friend class LLPanelLoginListener;
	void reshapeBrowser();
	void addFavoritesToStartLocation();
	void addUsersWithFavoritesToUsername();
	void onSelectServer();
	void onLocationSLURL();

	static void onClickConnect(void*);
	static void onClickNewAccount(void*);
	static void onClickVersion(void*);
	static void onClickForgotPassword(void*);
	static void onClickHelp(void*);
	static void onPassKey(LLLineEditor* caller, void* user_data);
	static void updateServerCombo();

private:
	LLPointer<LLUIImage> mLogoImage;
	boost::scoped_ptr<LLPanelLoginListener> mListener;

	void			(*mCallback)(S32 option, void *userdata);
	void*			mCallbackData;

	BOOL            mPasswordModified;

	static LLPanelLogin* sInstance;
	static BOOL		sCapslockDidNotification;
};

#endif
