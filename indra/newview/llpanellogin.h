/** 
 * @file llpanellogin.h
 * @brief Login username entry fields.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "llcommandhandler.h"
#include "lldbstrings.h"
#include "llmemory.h"
#include "llviewerimage.h"
#include "llstring.h"
#include "llmd5.h"
#include "llwebbrowserctrl.h"

class LLTextBox;
class LLLineEditor;
class LLCheckBoxCtrl;
class LLButton;
class LLComboBox;


class LLLoginHandler : public LLCommandHandler
{
 public:
	LLLoginHandler() : LLCommandHandler("login") { }
	bool handle(const LLSD& tokens, const LLSD& queryMap);
	bool parseDirectLogin(std::string url);
	void parse(const LLSD& queryMap);

	LLUUID mWebLoginKey;
	LLString mFirstName;
	LLString mLastName;
};

extern LLLoginHandler gLoginHandler;

class LLPanelLogin
:	public LLPanel
#if LL_LIBXUL_ENABLED
	, public LLWebBrowserCtrlObserver
#endif
{
public:
	LLPanelLogin(const LLRect &rect, BOOL show_server, 
				void (*callback)(S32 option, void* user_data),
				void *callback_data);
	~LLPanelLogin();

	virtual BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	virtual void draw();

	static void show(const LLRect &rect, BOOL show_server, 
		void (*callback)(S32 option, void* user_data), 
		void* callback_data);

	static void close();

	void setSiteIsAlive( bool alive );

	static void loadLoginPage();	
	static void giveFocus();
	static void setAlwaysRefresh(bool refresh); 

private:
	static void onClickQuit(void*);
	static void onClickVersion(void*);

#if LL_LIBXUL_ENABLED
	// browser observer impls
	virtual void onNavigateComplete( const EventType& eventIn );
#endif
	
private:
	LLPointer<LLViewerImage> mLogoImage;

	void			(*mCallback)(S32 option, void *userdata);
	void*			mCallbackData;

	static LLPanelLogin* sInstance;
	BOOL			mHtmlAvailable;
};

#endif
