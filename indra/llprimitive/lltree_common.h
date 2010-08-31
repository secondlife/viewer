/** 
 * @file lltree_common.h
 * @brief LLTree_gene_0 base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

//	Common data for trees shared between simulator and viewer

#ifndef LL_LLTREE_COMMON_H
#define LL_LLTREE_COMMON_H

struct LLTree_gene_0 
{
	LLTree_gene_0()
	:	scale(0),
		branches(0),
		twist(0),
		droop(0),
		species(0),
		trunk_depth(0),
		branch_thickness(0),
		max_depth(0),
		scale_step(0)
	{
	}

	// 
	//  The genome for a tree, species 0
	// 
	U8  scale;				//  Scales size of the tree ( / 50 = meter)
	U8	branches;			//  When tree forks, how many branches emerge?
	U8  twist;				//  twist about old branch axis for each branch (convert to degrees by dividing/255 * 180)
	U8	droop;				//  Droop away from old branch axis (convert to degrees by dividing/255 * 180)
	U8	species;			//  Branch coloring index
	U8	trunk_depth;		//  max recursions in the main trunk
	U8  branch_thickness;	//  Scales thickness of trunk ( / 50 = meter) 
	U8  max_depth;			//  Branch Recursions to flower 
	U8  scale_step;			//  How much to multiply scale size at each recursion 0-1.f to convert to float
}; 

#endif
