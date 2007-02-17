/** 
 * @file llfloatertelehub.h
 * @author James Cook
 * @brief LLFloaterTelehub class definition
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERTELEHUB_H
#define LL_LLFLOATERTELEHUB_H

#include "llfloater.h"

class LLMessageSystem;
class LLObjectSelection;

const S32 MAX_SPAWNPOINTS_PER_TELEHUB = 16;

class LLFloaterTelehub : public LLFloater
{
public:
	// Opens the floater on screen.
	static void show();

	virtual void draw();

	static BOOL renderBeacons();
	static void addBeacons();

private:
	LLFloaterTelehub();
	~LLFloaterTelehub();

	void refresh();
	void sendTelehubInfoRequest();

	static void onClickConnect(void* data);
	static void onClickDisconnect(void* data);
	static void onClickAddSpawnPoint(void* data);
	static void onClickRemoveSpawnPoint(void* data);

	static void processTelehubInfo(LLMessageSystem* msg, void**);
	void unpackTelehubInfo(LLMessageSystem* msg);

private:
	LLUUID mTelehubObjectID;	// null if no telehub
	LLString mTelehubObjectName;
	LLVector3 mTelehubPos;	// region local, fallback if viewer can't see the object
	LLQuaternion mTelehubRot;

	S32 mNumSpawn;
	LLVector3 mSpawnPointPos[MAX_SPAWNPOINTS_PER_TELEHUB];
	
	LLHandle<LLObjectSelection> mObjectSelection;

	static LLFloaterTelehub* sInstance;
};

#endif
