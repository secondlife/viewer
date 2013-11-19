/** 
 * @file llvolumemgr.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llvolumemgr.h"
#include "llvolume.h"


const F32 BASE_THRESHOLD = 0.03f;

//static
F32 LLVolumeLODGroup::mDetailThresholds[NUM_LODS] = {BASE_THRESHOLD,
													 2*BASE_THRESHOLD,
													 8*BASE_THRESHOLD,
													 100*BASE_THRESHOLD};

//static
F32 LLVolumeLODGroup::mDetailScales[NUM_LODS] = {1.f, 1.5f, 2.5f, 4.f};


//============================================================================

LLVolumeMgr::LLVolumeMgr()
:	mDataMutex(NULL)
{
	// the LLMutex magic interferes with easy unit testing,
	// so you now must manually call useMutex() to use it
	//mDataMutex = new LLMutex(gAPRPoolp);
}

LLVolumeMgr::~LLVolumeMgr()
{
	cleanup();

	delete mDataMutex;
	mDataMutex = NULL;
}

BOOL LLVolumeMgr::cleanup()
{
	BOOL no_refs = TRUE;
	if (mDataMutex)
	{
		mDataMutex->lock();
	}
	for (volume_lod_group_map_t::iterator iter = mVolumeLODGroups.begin(),
			 end = mVolumeLODGroups.end();
		 iter != end; iter++)
	{
		LLVolumeLODGroup *volgroupp = iter->second;
		if (volgroupp->cleanupRefs() == false)
		{
			no_refs = FALSE;
		}
 		delete volgroupp;
	}
	mVolumeLODGroups.clear();
	if (mDataMutex)
	{
		mDataMutex->unlock();
	}
	return no_refs;
}

// Always only ever store the results of refVolume in a LLPointer
// Note however that LLVolumeLODGroup that contains the volume
//  also holds a LLPointer so the volume will only go away after
//  anything holding the volume and the LODGroup are destroyed
LLVolume* LLVolumeMgr::refVolume(const LLVolumeParams &volume_params, const S32 detail)
{
	LLVolumeLODGroup* volgroupp;
	if (mDataMutex)
	{
		mDataMutex->lock();
	}
	volume_lod_group_map_t::iterator iter = mVolumeLODGroups.find(&volume_params);
	if( iter == mVolumeLODGroups.end() )
	{
		volgroupp = createNewGroup(volume_params);
	}
	else
	{
		volgroupp = iter->second;
	}
	if (mDataMutex)
	{
		mDataMutex->unlock();
	}
	return volgroupp->refLOD(detail);
}

// virtual
LLVolumeLODGroup* LLVolumeMgr::getGroup( const LLVolumeParams& volume_params ) const
{
	LLVolumeLODGroup* volgroupp = NULL;
	if (mDataMutex)
	{
		mDataMutex->lock();
	}
	volume_lod_group_map_t::const_iterator iter = mVolumeLODGroups.find(&volume_params);
	if( iter != mVolumeLODGroups.end() )
	{
		volgroupp = iter->second;
	}
	if (mDataMutex)
	{
		mDataMutex->unlock();
	}
	return volgroupp;
}

void LLVolumeMgr::unrefVolume(LLVolume *volumep)
{
	if (volumep->isUnique())
	{
		// TomY: Don't need to manage this volume. It is a unique instance.
		return;
	}
	const LLVolumeParams* params = &(volumep->getParams());
	if (mDataMutex)
	{
		mDataMutex->lock();
	}
	volume_lod_group_map_t::iterator iter = mVolumeLODGroups.find(params);
	if( iter == mVolumeLODGroups.end() )
	{
		llerrs << "Warning! Tried to cleanup unknown volume type! " << *params << llendl;
		if (mDataMutex)
		{
			mDataMutex->unlock();
		}
		return;
	}
	else
	{
		LLVolumeLODGroup* volgroupp = iter->second;

		volgroupp->derefLOD(volumep);
		if (volgroupp->getNumRefs() == 0)
		{
			mVolumeLODGroups.erase(params);
			delete volgroupp;
		}
	}
	if (mDataMutex)
	{
		mDataMutex->unlock();
	}

}

// protected
void LLVolumeMgr::insertGroup(LLVolumeLODGroup* volgroup)
{
	mVolumeLODGroups[volgroup->getVolumeParams()] = volgroup;
}

// protected
LLVolumeLODGroup* LLVolumeMgr::createNewGroup(const LLVolumeParams& volume_params)
{
	LLVolumeLODGroup* volgroup = new LLVolumeLODGroup(volume_params);
	insertGroup(volgroup);
	return volgroup;
}

// virtual
void LLVolumeMgr::dump()
{
	F32 avg = 0.f;
	if (mDataMutex)
	{
		mDataMutex->lock();
	}
	for (volume_lod_group_map_t::iterator iter = mVolumeLODGroups.begin(),
			 end = mVolumeLODGroups.end();
		 iter != end; iter++)
	{
		LLVolumeLODGroup *volgroupp = iter->second;
		avg += volgroupp->dump();
	}
	int count = (int)mVolumeLODGroups.size();
	avg = count ? avg / (F32)count : 0.0f;
	if (mDataMutex)
	{
		mDataMutex->unlock();
	}
	llinfos << "Average usage of LODs " << avg << llendl;
}

void LLVolumeMgr::useMutex()
{ 
	if (!mDataMutex)
	{
		mDataMutex = new LLMutex(gAPRPoolp);
	}
}

std::ostream& operator<<(std::ostream& s, const LLVolumeMgr& volume_mgr)
{
	s << "{ numLODgroups=" << volume_mgr.mVolumeLODGroups.size() << ", ";

	S32 total_refs = 0;
	if (volume_mgr.mDataMutex)
	{
		volume_mgr.mDataMutex->lock();
	}

	for (LLVolumeMgr::volume_lod_group_map_t::const_iterator iter = volume_mgr.mVolumeLODGroups.begin();
		 iter != volume_mgr.mVolumeLODGroups.end(); ++iter)
	{
		LLVolumeLODGroup *volgroupp = iter->second;
		total_refs += volgroupp->getNumRefs();
		s << ", " << (*volgroupp);
	}

	if (volume_mgr.mDataMutex)
	{
		volume_mgr.mDataMutex->unlock();
	}

	s << ", total_refs=" << total_refs << " }";
	return s;
}

LLVolumeLODGroup::LLVolumeLODGroup(const LLVolumeParams &params)
	: mVolumeParams(params),
	  mRefs(0)
{
	for (S32 i = 0; i < NUM_LODS; i++)
	{
		mLODRefs[i] = 0;
		mAccessCount[i] = 0;
	}
}

LLVolumeLODGroup::~LLVolumeLODGroup()
{
	for (S32 i = 0; i < NUM_LODS; i++)
	{
		llassert_always(mLODRefs[i] == 0);
	}
}

// Called from LLVolumeMgr::cleanup
bool LLVolumeLODGroup::cleanupRefs()
{
	bool res = true;
	if (mRefs != 0)
	{
		llwarns << "Volume group has remaining refs:" << getNumRefs() << llendl;
		mRefs = 0;
		for (S32 i = 0; i < NUM_LODS; i++)
		{
			if (mLODRefs[i] > 0)
			{
				llwarns << " LOD " << i << " refs = " << mLODRefs[i] << llendl;
				mLODRefs[i] = 0;
				mVolumeLODs[i] = NULL;
			}
		}
		llwarns << *getVolumeParams() << llendl;
		res = false;
	}
	return res;
}

LLVolume* LLVolumeLODGroup::refLOD(const S32 detail)
{
	llassert(detail >=0 && detail < NUM_LODS);
	mAccessCount[detail]++;
	
	mRefs++;
	if (mVolumeLODs[detail].isNull())
	{
		mVolumeLODs[detail] = new LLVolume(mVolumeParams, mDetailScales[detail]);
	}
	mLODRefs[detail]++;
	return mVolumeLODs[detail];
}

BOOL LLVolumeLODGroup::derefLOD(LLVolume *volumep)
{
	llassert_always(mRefs > 0);
	mRefs--;
	for (S32 i = 0; i < NUM_LODS; i++)
	{
		if (mVolumeLODs[i] == volumep)
		{
			llassert_always(mLODRefs[i] > 0);
			mLODRefs[i]--;
#if 0 // SJB: Possible opt: keep other lods around
			if (!mLODRefs[i])
			{
				mVolumeLODs[i] = NULL;
			}
#endif
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

void LLVolumeLODGroup::getDetailProximity(const F32 tan_angle, F32 &to_lower, F32& to_higher)
{
	S32 detail = getDetailFromTan(tan_angle);
	
	if (detail > 0)
	{
		to_lower = tan_angle - mDetailThresholds[detail];
	}
	else
	{
		to_lower = 1024.f*1024.f;
	}

	if (detail < NUM_LODS-1)
	{
		to_higher = mDetailThresholds[detail+1] - tan_angle;
	}
	else
	{
		to_higher = 1024.f*1024.f;
	}
}

F32 LLVolumeLODGroup::getVolumeScaleFromDetail(const S32 detail)
{
	return mDetailScales[detail];
}

S32 LLVolumeLODGroup::getVolumeDetailFromScale(const F32 detail)
{
	for (S32 i = 1; i < 4; i++)
	{
		if (mDetailScales[i] > detail)
		{
			return i-1;
		}
	}

	return 3;
}

F32 LLVolumeLODGroup::dump()
{
	F32 usage = 0.f;
	for (S32 i = 0; i < NUM_LODS; i++)
	{
		if (mAccessCount[i] > 0)
		{
			usage += 1.f;
		}
	}
	usage = usage / (F32)NUM_LODS;

	std::string dump_str = llformat("%.3f %d %d %d %d", usage, mAccessCount[0], mAccessCount[1], mAccessCount[2], mAccessCount[3]);

	llinfos << dump_str << llendl;
	return usage;
}

std::ostream& operator<<(std::ostream& s, const LLVolumeLODGroup& volgroup)
{
	s << "{ numRefs=" << volgroup.getNumRefs();
	s << ", mParams=" << volgroup.getVolumeParams();
	s << " }";
	 
	return s;
}

