/** 
 * @file llvolumemgr.cpp
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

#include "linden_common.h"

#include "llvolumemgr.h"
#include "llvolume.h"


//#define DEBUG_VOLUME

LLVolumeMgr* gVolumeMgr = 0;

const F32 BASE_THRESHOLD = 0.03f;

//static
F32 LLVolumeLODGroup::mDetailThresholds[NUM_LODS] = {BASE_THRESHOLD,
													 2*BASE_THRESHOLD,
													 8*BASE_THRESHOLD,
													 100*BASE_THRESHOLD};

//static
F32 LLVolumeLODGroup::mDetailScales[NUM_LODS] = {1.f, 1.5f, 2.5f, 4.f};

//============================================================================
//static
void LLVolumeMgr::initClass()
{
	gVolumeMgr = new LLVolumeMgr();
}

//static
BOOL LLVolumeMgr::cleanupClass()
{
	BOOL res = FALSE;
	if (gVolumeMgr) {
		res = gVolumeMgr->cleanup();
		delete gVolumeMgr;
		gVolumeMgr = 0;
	}
	return res;
}

//============================================================================

LLVolumeMgr::LLVolumeMgr()
{
	mDataMutex = new LLMutex(gAPRPoolp);
//	mNumVolumes = 0;
}

LLVolumeMgr::~LLVolumeMgr()
{
	cleanup();
	delete mDataMutex;
}

BOOL LLVolumeMgr::cleanup()
{
	#ifdef DEBUG_VOLUME
	{
		lldebugs << "LLVolumeMgr::cleanup()" << llendl;
	}
	#endif
	BOOL no_refs = TRUE;
	mDataMutex->lock();
	for (volume_lod_group_map_t::iterator iter = mVolumeLODGroups.begin(),
			 end = mVolumeLODGroups.end();
		 iter != end; iter++)
	{
		LLVolumeLODGroup *volgroupp = iter->second;
		if (volgroupp->getNumRefs() != 1)
		{
			llwarns << "Volume group " << volgroupp << " has " 
					<< volgroupp->getNumRefs() << " remaining refs" << llendl;
			llwarns << volgroupp->getParams() << llendl;
			no_refs = FALSE;
		}
 		volgroupp->unref();// this );
	}
	mVolumeLODGroups.clear();
	mDataMutex->unlock();
	return no_refs;
}

LLVolume *LLVolumeMgr::getVolume(const LLVolumeParams &volume_params, const S32 detail)
{
	LLVolumeLODGroup* volgroupp;
	mDataMutex->lock();
	volume_lod_group_map_t::iterator iter = mVolumeLODGroups.find(&volume_params);
	if( iter == mVolumeLODGroups.end() )
	{
		volgroupp = new LLVolumeLODGroup(volume_params);
		const LLVolumeParams* params = &(volgroupp->getParams());
		mVolumeLODGroups[params] = volgroupp;
 		volgroupp->ref(); // initial reference
	}
	else
	{
		volgroupp = iter->second;
	}
	volgroupp->ref();// this );
	mDataMutex->unlock();
	//	mNumVolumes++;
	#ifdef DEBUG_VOLUME
	{
		lldebugs << "LLVolumeMgr::getVolume() " << (*this) << llendl;
	}
	#endif
	return volgroupp->getLOD(detail);
}

void LLVolumeMgr::cleanupVolume(LLVolume *volumep)
{
	if (volumep->isUnique())
	{
		// TomY: Don't need to manage this volume. It is a unique instance.
		return;
	}
	LLVolumeParams* params = (LLVolumeParams*) &(volumep->getParams());
	mDataMutex->lock();
	volume_lod_group_map_t::iterator iter = mVolumeLODGroups.find(params);
	if( iter == mVolumeLODGroups.end() )
	{
		llerrs << "Warning! Tried to cleanup unknown volume type! " << *params << llendl;
		mDataMutex->unlock();
		return;
	}
	else
	{
		LLVolumeLODGroup* volgroupp = iter->second;

		volgroupp->derefLOD(volumep);
		volgroupp->unref();// this );
		if (volgroupp->getNumRefs() == 1)
		{
			mVolumeLODGroups.erase(params);
			volgroupp->unref();// this );
		}
		//	mNumVolumes--;
	}
	mDataMutex->unlock();

	#ifdef DEBUG_VOLUME
	{
		lldebugs << "LLVolumeMgr::cleanupVolume() " << (*this) << llendl;
	}
	#endif
}

void LLVolumeMgr::dump()
{
	F32 avg = 0.f;
	mDataMutex->lock();
	for (volume_lod_group_map_t::iterator iter = mVolumeLODGroups.begin(),
			 end = mVolumeLODGroups.end();
		 iter != end; iter++)
	{
		LLVolumeLODGroup *volgroupp = iter->second;
		avg += volgroupp->dump();
	}
	int count = (int)mVolumeLODGroups.size();
	avg = count ? avg / (F32)count : 0.0f;
	mDataMutex->unlock();
	llinfos << "Average usage of LODs " << avg << llendl;
}

std::ostream& operator<<(std::ostream& s, const LLVolumeMgr& volume_mgr)
{
	s << "{ numLODgroups=" << volume_mgr.mVolumeLODGroups.size() << ", ";

	S32 total_refs = 0;
	volume_mgr.mDataMutex->lock();

	LLVolumeMgr::volume_lod_group_map_iter iter = volume_mgr.mVolumeLODGroups.begin();
	LLVolumeMgr::volume_lod_group_map_iter end  = volume_mgr.mVolumeLODGroups.end();
	for ( ; iter != end; ++iter)
	{
		LLVolumeLODGroup *volgroupp = iter->second;
		total_refs += volgroupp->getNumRefs();
		s << ", " << (*volgroupp);
	}

	volume_mgr.mDataMutex->unlock();

	s << ", total_refs=" << total_refs << " }";
	return s;
}

LLVolumeLODGroup::LLVolumeLODGroup(const LLVolumeParams &params)
{
	S32 i;
	mParams = params;

	for (i = 0; i < NUM_LODS; i++)
	{
		mLODRefs[i] = 0;
		mVolumeLODs[i] = NULL;
		mAccessCount[i] = 0;
	}
}

LLVolumeLODGroup::~LLVolumeLODGroup()
{
	S32 i;
	for (i = 0; i < NUM_LODS; i++)
	{
		delete mVolumeLODs[i];
		mVolumeLODs[i] = NULL;
	}
}


LLVolume * LLVolumeLODGroup::getLOD(const S32 detail)
{
	llassert(detail >=0 && detail < NUM_LODS);
	mAccessCount[detail]++;
	mLODRefs[detail]++;
	if (!mVolumeLODs[detail])
	{
		mVolumeLODs[detail] = new LLVolume(mParams, mDetailScales[detail]);
	}
	return mVolumeLODs[detail];
}

BOOL LLVolumeLODGroup::derefLOD(LLVolume *volumep)
{
	S32 i;
	for (i = 0; i < NUM_LODS; i++)
	{
		if (mVolumeLODs[i] == volumep)
		{
			mLODRefs[i]--;
			if (!mLODRefs[i])
			{
				mVolumeLODs[i] = NULL;
			}
			return TRUE;
		}
	}
	llerrs << "Deref of non-matching LOD in volume LOD group" << llendl;
	return FALSE;
}

S32 LLVolumeLODGroup::getDetailFromTan(const F32 tan_angle)
{
	S32 i = 0;
	while (i < (NUM_LODS - 1))
	{
		if (tan_angle <= mDetailThresholds[i])
		{
			return i;
		}
		i++;
	}
	return NUM_LODS - 1;
}

F32 LLVolumeLODGroup::getVolumeScaleFromDetail(const S32 detail)
{
	return mDetailScales[detail];
}

F32 LLVolumeLODGroup::dump()
{
	char dump_str[255];		/* Flawfinder: ignore */
	F32 usage = 0.f;
	for (S32 i = 0; i < NUM_LODS; i++)
	{
		if (mAccessCount[i] > 0)
		{
			usage += 1.f;
		}
	}
	usage = usage / (F32)NUM_LODS;

	snprintf(dump_str, sizeof(dump_str), "%.3f %d %d %d %d", usage, mAccessCount[0], mAccessCount[1], mAccessCount[2], mAccessCount[3]);	/* Flawfinder: ignore */

	llinfos << dump_str << llendl;
	return usage;
}

std::ostream& operator<<(std::ostream& s, const LLVolumeLODGroup& volgroup)
{
	s << "{ numRefs=" << volgroup.getNumRefs();
	s << ", mParams=" << volgroup.mParams;
	s << " }";
	 
	return s;
}

