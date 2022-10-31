/** 
 * @file lltree_common.h
 * @brief LLTree_gene_0 base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

//  Common data for trees shared between simulator and viewer

#ifndef LL_LLTREE_COMMON_H
#define LL_LLTREE_COMMON_H

struct LLTree_gene_0 
{
    LLTree_gene_0()
    :   scale(0),
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
    U8  scale;              //  Scales size of the tree ( / 50 = meter)
    U8  branches;           //  When tree forks, how many branches emerge?
    U8  twist;              //  twist about old branch axis for each branch (convert to degrees by dividing/255 * 180)
    U8  droop;              //  Droop away from old branch axis (convert to degrees by dividing/255 * 180)
    U8  species;            //  Branch coloring index
    U8  trunk_depth;        //  max recursions in the main trunk
    U8  branch_thickness;   //  Scales thickness of trunk ( / 50 = meter) 
    U8  max_depth;          //  Branch Recursions to flower 
    U8  scale_step;         //  How much to multiply scale size at each recursion 0-1.f to convert to float
}; 

#endif
