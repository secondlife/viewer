/** 
 * @file llfloaterscriptdebug.h
 * @brief Shows error and warning output from scripts
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#ifndef LL_LLFLOATERSCRIPTDEBUG_H
#define LL_LLFLOATERSCRIPTDEBUG_H

#include "llfloater.h"

class LLTextEditor;
class LLUUID;

class LLFloaterScriptDebug : public LLMultiFloater
{
public:
	virtual ~LLFloaterScriptDebug();
	virtual void onClose(bool app_quitting) { setVisible(FALSE); }
	virtual BOOL postBuild();
    static void show(const LLUUID& object_id);
	static void addScriptLine(const std::string &utf8mesg, const std::string &user_name, const LLColor4& color, const LLUUID& source_id);

protected:
	LLFloaterScriptDebug();

	static LLFloater* addOutputWindow(const LLUUID& object_id);

protected:
	static LLFloaterScriptDebug*	sInstance;
};

class LLFloaterScriptDebugOutput : public LLFloater
{
public:
	LLFloaterScriptDebugOutput();
	LLFloaterScriptDebugOutput(const LLUUID& object_id);
	~LLFloaterScriptDebugOutput();

	virtual void		initFloater(const std::string& title, BOOL resizable, 
						S32 min_width, S32 min_height, BOOL drag_on_left,
						BOOL minimizable, BOOL close_btn);

	void addLine(const std::string &utf8mesg, const std::string &user_name, const LLColor4& color);

	static LLFloaterScriptDebugOutput* show(const LLUUID& object_id);
	static LLFloaterScriptDebugOutput* getFloaterByID(const LLUUID& id);

protected:
	LLTextEditor*			mHistoryEditor;

	typedef std::map<LLUUID, LLFloaterScriptDebugOutput*> instance_map_t;
	static instance_map_t sInstanceMap;

	LLUUID		mObjectID;
};

#endif // LL_LLFLOATERSCRIPTDEBUG_H
