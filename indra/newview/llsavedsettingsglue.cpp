/** 
 * @file llsavedsettingsglue.cpp
 * @author James Cook
 * @brief LLSavedSettingsGlue class implementation
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llsavedsettingsglue.h"

#include "lluictrl.h"

#include "llviewercontrol.h"

void LLSavedSettingsGlue::setBOOL(LLUICtrl* ctrl, void* data)
{
	const char* name = (const char*)data;
	LLSD value = ctrl->getValue();
	gSavedSettings.setBOOL(name, value.asBoolean());
}

void LLSavedSettingsGlue::setS32(LLUICtrl* ctrl, void* data)
{
	const char* name = (const char*)data;
	LLSD value = ctrl->getValue();
	gSavedSettings.setS32(name, value.asInteger());
}

void LLSavedSettingsGlue::setF32(LLUICtrl* ctrl, void* data)
{
	const char* name = (const char*)data;
	LLSD value = ctrl->getValue();
	gSavedSettings.setF32(name, (F32)value.asReal());
}

void LLSavedSettingsGlue::setU32(LLUICtrl* ctrl, void* data)
{
	const char* name = (const char*)data;
	LLSD value = ctrl->getValue();
	gSavedSettings.setU32(name, (U32)value.asInteger());
}

void LLSavedSettingsGlue::setString(LLUICtrl* ctrl, void* data)
{
	const char* name = (const char*)data;
	LLSD value = ctrl->getValue();
	gSavedSettings.setString(name, value.asString());
}
