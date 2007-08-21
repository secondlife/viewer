/** 
 * @file llpanellogin.h
 * @brief Login username entry fields.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPANELLOGIN_H
#define LL_LLPANELLOGIN_H

#include "llpanel.h"

#include "lldbstrings.h"
#include "llmemory.h"
#include "llviewerimage.h"
#include "llstring.h"
#include "llmd5.h"

class LLTextBox;
class LLLineEditor;
class LLCheckBoxCtrl;
class LLButton;
class LLComboBox;

class LLPanelLogin
:	public LLPanel
{
public:
	LLPanelLogin(const LLRect &rect, BOOL show_server, 
				void (*callback)(S32 option, void* user_data),
				void *callback_data);
	~LLPanelLogin();

	virtual BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	virtual void draw();
	virtual void setFocus( BOOL b );

	static void show(const LLRect &rect, BOOL show_server, 
		void (*callback)(S32 option, void* user_data), 
		void* callback_data);

	static void setFields(const std::string& firstname, const std::string& lastname, 
		const std::string& password, BOOL remember);

	static void addServer(const char *server, S32 domain_name);
	static void refreshLocation( bool force_visible );

	static void getFields(LLString &firstname, LLString &lastname,
		LLString &password, BOOL &remember);

	static BOOL getServer(LLString &server, S32& domain_name);
	static void getLocation(LLString &location);

	static void close();

	void setSiteIsAlive( bool alive );

	static void	giveFocus();
	static void mungePassword(LLUICtrl* caller, void* user_data);
	
private:
	static void onClickConnect(void*);
	static void onClickNewAccount(void*);
	static void newAccountAlertCallback(S32 option, void*);
	static void onClickQuit(void*);
	static void onClickVersion(void*);
	static void onPassKey(LLLineEditor* caller, void* user_data);

private:
	LLPointer<LLViewerImage> mLogoImage;

	void			(*mCallback)(S32 option, void *userdata);
	void*			mCallbackData;

	char mIncomingPassword[DB_USER_PASSWORD_BUF_SIZE];		/*Flawfinder: ignore*/
	char mMungedPassword[MD5HEX_STR_SIZE];		/*Flawfinder: ignore*/

	static LLPanelLogin* sInstance;
	static BOOL		sCapslockDidNotification;
	BOOL			mHtmlAvailable;
};

#endif
