/** 
 * @file llsavedsettingsglue.h
 * @author James Cook
 * @brief LLSavedSettingsGlue class definition
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSAVEDSETTINGSGLUE_H
#define LL_LLSAVEDSETTINGSGLUE_H

class LLUICtrl;

// Helper to change gSavedSettings from UI widget commit callbacks.
// Set the widget callback to be one of the setFoo() calls below,
// and assign the control name as a const char* to the userdata.
class LLSavedSettingsGlue
{
public:
	static void setBOOL(LLUICtrl* ctrl, void* name);
	static void setS32(LLUICtrl* ctrl, void* name);
	static void setF32(LLUICtrl* ctrl, void* name);
	static void setU32(LLUICtrl* ctrl, void* name);
	static void setString(LLUICtrl* ctrl, void* name);
};

#endif
