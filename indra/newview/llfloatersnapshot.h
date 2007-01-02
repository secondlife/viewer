/** 
 * @file llfloatersnapshot.h
 * @brief Snapshot preview window, allowing saving, e-mailing, etc.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERSNAPSHOT_H
#define LL_LLFLOATERSNAPSHOT_H

#include "llfloater.h"

#include "llmemory.h"
#include "llimagegl.h"
#include "llcharacter.h"

class LLFloaterSnapshot : public LLFloater
{
public:
    LLFloaterSnapshot();
	virtual ~LLFloaterSnapshot();

	virtual BOOL postBuild();
	virtual void draw();
	virtual void onClose(bool app_quitting);

	static void show(void*);
	static void hide(void*);

	static void update();
	
private:
	class Impl;
	Impl& impl;

	static LLFloaterSnapshot* sInstance;
};

class LLSnapshotFloaterView : public LLFloaterView
{
public:
	LLSnapshotFloaterView( const LLString& name, const LLRect& rect );
	virtual ~LLSnapshotFloaterView();

	/*virtual*/	BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	/*virtual*/	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/	BOOL handleHover(S32 x, S32 y, MASK mask);
};

extern LLSnapshotFloaterView* gSnapshotFloaterView;
#endif // LL_LLFLOATERSNAPSHOT_H
