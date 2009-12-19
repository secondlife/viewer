/**
 * @file lltoastalertpanel.h
 * @brief Panel for alert toasts.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

// *NOTE: this module is a copy-paste of llui/llalertdialog.h
// Can we re-implement this as a subclass of LLAlertDialog and
// avoid all this code duplication? It already caused EXT-2232.

#ifndef LL_TOASTALERTPANEL_H
#define LL_TOASTALERTPANEL_H

#include "lltoastpanel.h"
#include "llfloater.h"
#include "llui.h"
#include "llnotificationptr.h"
#include "llalertdialog.h"
#include "llerror.h"

class LLButton;
class LLCheckBoxCtrl;
class LLAlertDialogTemplate;
class LLLineEditor;

/**
 * Toast panel for alert notification.
 * Alerts notifications doesn't require user interaction.
 *
 * Replaces class LLAlertDialog.
 * https://wiki.lindenlab.com/mediawiki/index.php?title=LLAlertDialog&oldid=81388
 */

class LLToastAlertPanel
	: public LLToastPanel
{
	LOG_CLASS(LLToastAlertPanel);
public:
	typedef bool (*display_callback_t)(S32 modal);

	static void setURLLoader(LLAlertURLLoader* loader)
	{
		sURLLoader = loader;
	}
	
public:
	// User's responsibility to call show() after creating these.
	LLToastAlertPanel( LLNotificationPtr notep, bool is_modal );

	virtual BOOL	handleKeyHere(KEY key, MASK mask );

	virtual void	draw();
	virtual void	setVisible( BOOL visible );

	bool 			setCheckBox( const std::string&, const std::string& );	
	void			setCaution(BOOL val = TRUE) { mCaution = val; }
	// If mUnique==TRUE only one copy of this message should exist
	void			setUnique(BOOL val = TRUE) { mUnique = val; }
	void			setEditTextArgs(const LLSD& edit_args);
	
	void onClickIgnore(LLUICtrl* ctrl);
	void onButtonPressed(const LLSD& data, S32 button);
	
private:
	static std::map<std::string, LLToastAlertPanel*> sUniqueActiveMap;

	virtual ~LLToastAlertPanel();
	// No you can't kill it.  It can only kill itself.

	// Does it have a readable title label, or minimize or close buttons?
	BOOL hasTitleBar() const;

private:
	static LLAlertURLLoader* sURLLoader;
	static LLControlGroup* sSettings;

	struct ButtonData
	{
		LLButton* mButton;
		std::string mURL;
		U32 mURLExternal;
	};
	std::vector<ButtonData> mButtonData;

	S32				mDefaultOption;
	LLCheckBoxCtrl* mCheck;
	BOOL			mCaution;
	BOOL			mUnique;
	LLUIString		mLabel;
	LLFrameTimer	mDefaultBtnTimer;
	// For Dialogs that take a line as text as input:
	LLLineEditor* mLineEditor;

};

#endif  // LL_TOASTALERTPANEL_H
