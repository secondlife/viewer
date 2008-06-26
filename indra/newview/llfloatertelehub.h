/** 
 * @file llfloatertelehub.h
 * @author James Cook
 * @brief LLFloaterTelehub class definition
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
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
	std::string mTelehubObjectName;
	LLVector3 mTelehubPos;	// region local, fallback if viewer can't see the object
	LLQuaternion mTelehubRot;

	S32 mNumSpawn;
	LLVector3 mSpawnPointPos[MAX_SPAWNPOINTS_PER_TELEHUB];
	
	LLSafeHandle<LLObjectSelection> mObjectSelection;

	static LLFloaterTelehub* sInstance;
};

#endif
