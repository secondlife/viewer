/** 
 * @file llfloaternamedesc.h
 * @brief LLFloaterNameDesc class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERNAMEDESC_H
#define LL_LLFLOATERNAMEDESC_H

#include "llfloater.h"
#include "llresizehandle.h"
#include "llstring.h"

class LLLineEditor;
class LLButton;
class LLRadioGroup;

class LLFloaterNameDesc : public LLFloater
{
public:
	LLFloaterNameDesc(const std::string& filename);
	virtual ~LLFloaterNameDesc();
	virtual BOOL postBuild();

	static void			doCommit(class LLUICtrl *, void* userdata);
protected:
	virtual void		onCommit();
	virtual void	    centerWindow();

protected:
	BOOL        mIsAudio;

	LLString		mFilenameAndPath;
	LLString		mFilename;

	static void		onBtnOK(void*);
	static void		onBtnCancel(void*);
};

#endif  // LL_LLFLOATERNAMEDESC_H
