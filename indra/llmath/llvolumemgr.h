/** 
 * @file llvolumemgr.h
 * @brief LLVolumeMgr class.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#ifndef LL_LLVOLUMEMGR_H
#define LL_LLVOLUMEMGR_H

#include <map>

#include "llvolume.h"
#include "llmemory.h"
#include "llthread.h"

class LLVolumeParams;
class LLVolumeLODGroup;

class LLVolumeLODGroup : public LLThreadSafeRefCount
{
protected:
	~LLVolumeLODGroup();
	
public:
	enum
	{
		NUM_LODS = 4
	};

	LLVolumeLODGroup(const LLVolumeParams &params);

	BOOL derefLOD(LLVolume *volumep);
	static S32 getDetailFromTan(const F32 tan_angle);
	static F32 getVolumeScaleFromDetail(const S32 detail);

	LLVolume *getLOD(const S32 detail);
	const LLVolumeParams &getParams() const { return mParams; };

	F32	dump();
	friend std::ostream& operator<<(std::ostream& s, const LLVolumeLODGroup& volgroup);

protected:
	LLVolumeParams mParams;

	S32 mLODRefs[NUM_LODS];
	LLVolume *mVolumeLODs[NUM_LODS];
	static F32 mDetailThresholds[NUM_LODS];
	static F32 mDetailScales[NUM_LODS];
	S32		mAccessCount[NUM_LODS];
};

class LLVolumeMgr
{
public:
	static void initClass();
	static BOOL cleanupClass();
	
public:
	LLVolumeMgr();
	~LLVolumeMgr();
	BOOL cleanup();			// Cleanup all volumes being managed, returns TRUE if no dangling references
	LLVolume *getVolume(const LLVolumeParams &volume_params, const S32 detail);
	void cleanupVolume(LLVolume *volumep);

	void dump();
	friend std::ostream& operator<<(std::ostream& s, const LLVolumeMgr& volume_mgr);

protected:
	typedef std::map<const LLVolumeParams*, LLVolumeLODGroup*, LLVolumeParams::compare> volume_lod_group_map_t;
	typedef volume_lod_group_map_t::const_iterator volume_lod_group_map_iter;
	volume_lod_group_map_t mVolumeLODGroups;

	LLMutex* mDataMutex;
	
//	S32 mNumVolumes;
};

extern LLVolumeMgr* gVolumeMgr;

#endif // LL_LLVOLUMEMGR_H
