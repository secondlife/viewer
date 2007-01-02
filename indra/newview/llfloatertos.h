/** 
 * @file llfloatertos.h
 * @brief Terms of Service Agreement dialog
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERTOS_H
#define LL_LLFLOATERTOS_H

#include "llmodaldialog.h"
#include "llassetstorage.h"
#include "llmozlib.h"

class LLButton;
class LLRadioGroup;
class LLVFS;
class LLTextEditor;
class LLUUID;

class LLFloaterTOS : 
	public LLModalDialog,
	public LLEmbeddedBrowserWindowObserver
{
public:
	virtual ~LLFloaterTOS();

	// Types of dialog.
	enum ETOSType
	{
		TOS_TOS = 0,
		TOS_CRITICAL_MESSAGE = 1
	};

	// Asset_id is overwritten with LLUUID::null when agree is clicked.
	static LLFloaterTOS* show(ETOSType type, const std::string & message);

	BOOL postBuild();
	
	virtual void draw();

	static void		updateAgree( LLUICtrl *, void* userdata );
	static void		onContinue( void* userdata );
	static void		onCancel( void* userdata );

	void			setSiteIsAlive( bool alive );

	virtual void onNavigateComplete( const EventType& eventIn );

private:
	// Asset_id is overwritten with LLUUID::null when agree is clicked.
	LLFloaterTOS(ETOSType type, const std::string & message);

private:
	ETOSType		mType;
	LLString		mMessage;
	int				mWebBrowserWindowId;
	int				mLoadCompleteCount;

	static LLFloaterTOS* sInstance;
};

#endif // LL_LLFLOATERTOS_H
