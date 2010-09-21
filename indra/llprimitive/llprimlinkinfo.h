/** 
 * @file llprimlinkinfo.h
 * @author andrew@lindenlab.com
 * @brief A template for determining which prims in a set are linkable
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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


#ifndef LL_PRIM_LINK_INFO_H
#define LL_PRIM_LINK_INFO_H

// system includes
#include <iostream>
#include <map>
#include <list>
#include <vector>

// common includes
#include "stdtypes.h"
#include "v3math.h"
#include "llquaternion.h"
#include "llsphere.h"


const F32 MAX_OBJECT_SPAN = 54.f;		// max distance from outside edge of an object to the farthest edge
const F32 OBJECT_SPAN_BONUS = 2.f;		// infinitesimally small prims can always link up to this distance
const S32 MAX_PRIMS_PER_OBJECT = 256;


template < typename DATA_TYPE >
class LLPrimLinkInfo
{
public:
	LLPrimLinkInfo();
	LLPrimLinkInfo( DATA_TYPE data, const LLSphere& sphere );
	~LLPrimLinkInfo();

	void set( DATA_TYPE data, const LLSphere& sphere );
	void append( DATA_TYPE data, const LLSphere& sphere );
	void getData( std::list< DATA_TYPE >& data_list ) const;
	F32 getDiameter() const;
	LLVector3 getCenter() const;

	// returns 'true' if this info can link with other_info
	bool canLink( const LLPrimLinkInfo< DATA_TYPE >& other_info );

	S32 getPrimCount() const { return mDataMap.size(); }

	void mergeLinkableSet( typename std::list< LLPrimLinkInfo < DATA_TYPE > >& unlinked );

	void transform(const LLVector3& position, const LLQuaternion& rotation);

private:
	// returns number of merges made
	S32 merge(LLPrimLinkInfo< DATA_TYPE >& other_info);

	// returns number of collapses made
	static S32 collapse(typename std::list< LLPrimLinkInfo < DATA_TYPE > >& unlinked );

	void computeBoundingSphere();

	// Internal utility to encapsulate the link rules
	F32 get_max_linkable_span(const LLSphere& first, const LLSphere& second);
	F32 get_span(const LLSphere& first, const LLSphere& second);

private:
	std::map< DATA_TYPE, LLSphere > mDataMap;
	LLSphere mBoundingSphere;
};



template < typename DATA_TYPE >
LLPrimLinkInfo< DATA_TYPE >::LLPrimLinkInfo()
:	mBoundingSphere( LLVector3(0.f, 0.f, 0.f), 0.f )
{
}

template < typename DATA_TYPE >
LLPrimLinkInfo< DATA_TYPE >::LLPrimLinkInfo( DATA_TYPE data, const LLSphere& sphere)
:	mBoundingSphere(sphere)
{
	mDataMap[data] = sphere;
}

template < typename DATA_TYPE >
LLPrimLinkInfo< DATA_TYPE >::~LLPrimLinkInfo()
{
	mDataMap.clear();
}

template < typename DATA_TYPE >
void LLPrimLinkInfo< DATA_TYPE>::set( DATA_TYPE data, const LLSphere& sphere )
{
	if (!mDataMap.empty())
	{
		mDataMap.clear();
	}
	mDataMap[data] = sphere;
	mBoundingSphere = sphere;
}

template < typename DATA_TYPE >
void LLPrimLinkInfo< DATA_TYPE>::append( DATA_TYPE data, const LLSphere& sphere )
{
	mDataMap[data] = sphere;
	if (!mBoundingSphere.contains(sphere))
	{
		computeBoundingSphere();
	}
}

template < typename DATA_TYPE >
void LLPrimLinkInfo< DATA_TYPE >::getData( std::list< DATA_TYPE >& data_list) const
{
	typename std::map< DATA_TYPE, LLSphere >::const_iterator map_itr;
	for (map_itr = mDataMap.begin(); map_itr != mDataMap.end(); ++map_itr)
	{
		data_list.push_back(map_itr->first);
	}
}

template < typename DATA_TYPE >
F32 LLPrimLinkInfo< DATA_TYPE >::getDiameter() const
{ 
	return 2.f * mBoundingSphere.getRadius();
}

template < typename DATA_TYPE >
LLVector3 LLPrimLinkInfo< DATA_TYPE >::getCenter() const
{ 
	return mBoundingSphere.getCenter(); 
}

template < typename DATA_TYPE >
F32 LLPrimLinkInfo< DATA_TYPE >::get_max_linkable_span(const LLSphere& first, const LLSphere& second)
{
	F32 max_span = 3.f * (first.getRadius() + second.getRadius()) + OBJECT_SPAN_BONUS;
	if (max_span > MAX_OBJECT_SPAN)
	{
		max_span = MAX_OBJECT_SPAN;
	}

	return max_span;
}

template < typename DATA_TYPE >
F32 LLPrimLinkInfo< DATA_TYPE >::get_span(const LLSphere& first, const LLSphere& second)
{
	F32 span = (first.getCenter() - second.getCenter()).length()
				+ first.getRadius() + second.getRadius();
	return span;
}

// static
// returns 'true' if this info can link with any part of other_info
template < typename DATA_TYPE >
bool LLPrimLinkInfo< DATA_TYPE >::canLink(const LLPrimLinkInfo& other_info)
{
	F32 max_span = get_max_linkable_span(mBoundingSphere, other_info.mBoundingSphere);

	F32 span = get_span(mBoundingSphere, other_info.mBoundingSphere);
	
	if (span <= max_span)
	{
		// The entire other_info fits inside the max span.
		return TRUE;
	}
	else if (span > max_span + 2.f * other_info.mBoundingSphere.getRadius())
	{
		// there is no way any piece of other_info could link with this one
		return FALSE;
	}

	// there may be a piece of other_info that is linkable
	typename std::map< DATA_TYPE, LLSphere >::const_iterator map_itr;
	for (map_itr = other_info.mDataMap.begin(); map_itr != other_info.mDataMap.end(); ++map_itr)
	{
		const LLSphere& other_sphere = (*map_itr).second;
		max_span = get_max_linkable_span(mBoundingSphere, other_sphere);

		span = get_span(mBoundingSphere, other_sphere);

		if (span <= max_span)
		{
			// found one piece that is linkable
			return TRUE;
		}
	}
	return FALSE;
}

// merges elements of 'unlinked' 
// returns number of links made (NOT final prim count, NOR linked prim count)
// and removes any linkable infos from 'unlinked' 
template < typename DATA_TYPE >
void LLPrimLinkInfo< DATA_TYPE >::mergeLinkableSet(std::list< LLPrimLinkInfo< DATA_TYPE > > & unlinked)
{
	bool linked_something = true;
	while (linked_something)
	{
		linked_something = false;

		typename std::list< LLPrimLinkInfo< DATA_TYPE > >::iterator other_itr = unlinked.begin();
		while ( other_itr != unlinked.end()
			   && getPrimCount() < MAX_PRIMS_PER_OBJECT )
		{
			S32 merge_count = merge(*other_itr);
			if (merge_count > 0)
			{
				linked_something = true;
			}
			if (0 == (*other_itr).getPrimCount())
			{
				unlinked.erase(other_itr++);
			}
			else
			{
				++other_itr;
			}
		}
		if (!linked_something
			&& unlinked.size() > 1)
		{
			S32 collapse_count = collapse(unlinked);
			if (collapse_count > 0)
			{
				linked_something = true;
			}
		}
	}
}

// transforms all of the spheres into a new reference frame
template < typename DATA_TYPE >
void LLPrimLinkInfo< DATA_TYPE >::transform(const LLVector3& position, const LLQuaternion& rotation)
{
	typename std::map< DATA_TYPE, LLSphere >::iterator map_itr;
	for (map_itr = mDataMap.begin(); map_itr != mDataMap.end(); ++map_itr)
	{
		(*map_itr).second.setCenter((*map_itr).second.getCenter() * rotation + position);
	}
	mBoundingSphere.setCenter(mBoundingSphere.getCenter() * rotation + position);
}

// private
// returns number of links made
template < typename DATA_TYPE >
S32 LLPrimLinkInfo< DATA_TYPE >::merge(LLPrimLinkInfo& other_info)
{
	S32 link_count = 0;

//	F32 other_radius = other_info.mBoundingSphere.getRadius();
//	other_info.computeBoundingSphere();
//	if ( other_radius != other_info.mBoundingSphere.getRadius() )
//	{
//		llinfos << "Other bounding sphere changed!!" << llendl;
//	}

//	F32 this_radius = mBoundingSphere.getRadius();
//	computeBoundingSphere();
//	if ( this_radius != mBoundingSphere.getRadius() )
//	{
//		llinfos << "This bounding sphere changed!!" << llendl;
//	}


	F32 max_span = get_max_linkable_span(mBoundingSphere, other_info.mBoundingSphere);

	//  F32 center_dist = (mBoundingSphere.getCenter() - other_info.mBoundingSphere.getCenter()).length();
	//	llinfos << "objects are " << center_dist << "m apart" << llendl;
	F32 span = get_span(mBoundingSphere, other_info.mBoundingSphere);

	F32 span_limit = max_span + (2.f * other_info.mBoundingSphere.getRadius());
	if (span > span_limit)
	{
		// there is no way any piece of other_info could link with this one
		// llinfos << "span too large:  " << span << " vs. " << span_limit << llendl;
		return 0;
	}

	bool completely_linkable = (span <= max_span) ? true : false;

	typename std::map< DATA_TYPE, LLSphere >::iterator map_itr = other_info.mDataMap.begin();
	while (map_itr != other_info.mDataMap.end()
			&& getPrimCount() < MAX_PRIMS_PER_OBJECT )
	{
		DATA_TYPE other_data = (*map_itr).first;
		LLSphere& other_sphere = (*map_itr).second;

		if (!completely_linkable)
		{
			max_span = get_max_linkable_span(mBoundingSphere, other_sphere);
	
			F32 span = get_span(mBoundingSphere, other_sphere);

			if (span > max_span)
			{
				++map_itr;
				continue;
			}
		}

		mDataMap[other_data] = other_sphere;
		++link_count;

		if (!mBoundingSphere.contains(other_sphere) )
		{
			computeBoundingSphere();
		}

		// remove from the other info
		other_info.mDataMap.erase(map_itr++);
	}

	if (link_count > 0 && other_info.getPrimCount() > 0)
	{
		other_info.computeBoundingSphere();
	}
	return link_count;
}

// links any linkable elements of unlinked
template < typename DATA_TYPE >
S32 LLPrimLinkInfo< DATA_TYPE >::collapse(std::list< LLPrimLinkInfo< DATA_TYPE > > & unlinked)
{ 
	S32 link_count = 0;
	bool linked_something = true;
	while (linked_something)
	{
		linked_something = false;

		typename std::list< LLPrimLinkInfo< DATA_TYPE > >::iterator this_itr = unlinked.begin();
		typename std::list< LLPrimLinkInfo< DATA_TYPE > >::iterator other_itr = this_itr;
		++other_itr;
		while ( other_itr != unlinked.end() )
			   
		{
			S32 merge_count = (*this_itr).merge(*other_itr);
			if (merge_count > 0)
			{
				linked_something = true;
				link_count += merge_count;
			}
			if (0 == (*other_itr).getPrimCount())
			{
				unlinked.erase(other_itr++);
			}
			else
			{
				++other_itr;
			}
		}
	}
	return link_count;
}


template < typename DATA_TYPE >
void LLPrimLinkInfo< DATA_TYPE >::computeBoundingSphere()
{ 
	std::vector< LLSphere > sphere_list;
	typename std::map< DATA_TYPE, LLSphere >::const_iterator map_itr;
	for (map_itr = mDataMap.begin(); map_itr != mDataMap.end(); ++map_itr)
	{
		sphere_list.push_back(map_itr->second);
	}
	mBoundingSphere = LLSphere::getBoundingSphere(sphere_list);
}


#endif

