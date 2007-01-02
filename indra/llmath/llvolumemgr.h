/** 
 * @file llvolumemgr.h
 * @brief LLVolumeMgr class.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
