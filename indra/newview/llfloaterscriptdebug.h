/** 
 * @file llfloaterscriptdebug.h
 * @brief Shows error and warning output from scripts
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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

	virtual void		init(const LLString& title, BOOL resizable, 
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
