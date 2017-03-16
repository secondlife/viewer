/** 
 * @file llfloaterscriptdebug.h
 * @brief Shows error and warning output from scripts
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLFLOATERSCRIPTDEBUG_H
#define LL_LLFLOATERSCRIPTDEBUG_H

#include "llmultifloater.h"

class LLTextEditor;
class LLUUID;

class LLFloaterScriptDebug : public LLMultiFloater
{
public:
	LLFloaterScriptDebug(const LLSD& key);
	virtual ~LLFloaterScriptDebug();
	virtual BOOL postBuild();
	virtual void setVisible(BOOL visible);
    static void show(const LLUUID& object_id);

    /*virtual*/ void closeFloater(bool app_quitting = false);
	static void addScriptLine(const std::string &utf8mesg, const std::string &user_name, const LLColor4& color, const LLUUID& source_id);

protected:
	static LLFloater* addOutputWindow(const LLUUID& object_id);

protected:
	static LLFloaterScriptDebug*	sInstance;
};

class LLFloaterScriptDebugOutput : public LLFloater
{
public:
	LLFloaterScriptDebugOutput(const LLSD& object_id);
	~LLFloaterScriptDebugOutput();

	void addLine(const std::string &utf8mesg, const std::string &user_name, const LLColor4& color);

	virtual BOOL postBuild();
	
protected:
	LLTextEditor* mHistoryEditor;

	LLUUID mObjectID;
};

#endif // LL_LLFLOATERSCRIPTDEBUG_H
