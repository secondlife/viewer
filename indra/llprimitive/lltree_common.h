/** 
 * @file lltree_common.h
 * @brief LLTree_gene_0 base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

//	Common data for trees shared between simulator and viewer

#ifndef LL_LLTREE_COMMON_H
#define LL_LLTREE_COMMON_H

struct LLTree_gene_0 
{
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
