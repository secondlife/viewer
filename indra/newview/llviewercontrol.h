/** 
 * @file llviewercontrol.h
 * @brief references to viewer-specific control files
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERCONTROL_H
#define LL_LLVIEWERCONTROL_H

#include "llcontrol.h"
#include "llfloater.h"
#include "lltexteditor.h"

class LLFloaterSettingsDebug : public LLFloater
{
public:
	LLFloaterSettingsDebug();
	virtual ~LLFloaterSettingsDebug();

	virtual BOOL postBuild();
	virtual void draw();

	void updateControl(LLControlBase* control);

	static void show(void*);
	static void onSettingSelect(LLUICtrl* ctrl, void* user_data);
	static void onCommitSettings(LLUICtrl* ctrl, void* user_data);
	static void onClickDefault(void* user_data);

protected:
	static LLFloaterSettingsDebug* sInstance;
	LLTextEditor* mComment;
};

//setting variables are declared in this function
void declare_settings();
void fixup_settings();

// saved at end of session
extern LLControlGroup gSavedSettings;
extern LLControlGroup gSavedPerAccountSettings;

// Read-only
extern LLControlGroup gViewerArt;

// Read-only
extern LLControlGroup gColors;

// Saved at end of session
extern LLControlGroup gCrashSettings;

// Set after settings loaded
extern LLString gLastRunVersion;
extern LLString gCurrentVersion;

extern LLString gSettingsFileName;
extern LLString gPerAccountSettingsFileName;

#endif // LL_LLVIEWERCONTROL_H
