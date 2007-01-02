/** 
 * @file llhudeffect.h
 * @brief LLHUDEffect class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLHUDEFFECT_H
#define LL_LLHUDEFFECT_H

#include "llhudobject.h"

#include "lluuid.h"
#include "v4coloru.h"
#include "llinterp.h"
#include "llframetimer.h"
#include "llmemory.h"

const F32 LL_HUD_DUR_SHORT = 1.f;

class LLMessageSystem;


class LLHUDEffect : public LLHUDObject
{
public:
	void setNeedsSendToSim(const BOOL send_to_sim);
	BOOL getNeedsSendToSim() const;
	void setOriginatedHere(const BOOL orig_here);
	BOOL getOriginatedHere() const;

	void setDuration(const F32 duration);
	void setColor(const LLColor4U &color);
	void setID(const LLUUID &id);
	const LLUUID &getID() const;

	BOOL isDead() const;

	friend class LLHUDManager;
protected:
	LLHUDEffect(const U8 type);
	~LLHUDEffect();

	/*virtual*/ void render();

	virtual void packData(LLMessageSystem *mesgsys);
	virtual void unpackData(LLMessageSystem *mesgsys, S32 blocknum);
	virtual void update();

	static void getIDType(LLMessageSystem *mesgsys, S32 blocknum, LLUUID &uuid, U8 &type);

protected:
	LLUUID		mID;
	F32			mDuration;
	LLColor4U	mColor;

	BOOL		mNeedsSendToSim;
	BOOL		mOriginatedHere;
};

#endif // LL_LLHUDEFFECT_H
