/** 
 * @file lltaskname.h
 * @brief This contains the current list of valid tasks and is inluded
 * into both simulator and viewer
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLTASKNAME_H
#define LL_LLTASKNAME_H

// Current valid tasks
// If you add a taskname here you will have to 
// 1) Add an initializer to init_object() in llscript.cpp
// 1.1) Add to object_type_to_task_name() in llregion.cpp
// 2) Add display code to LLStupidObject::render2(LLAgent* agentp) in llstupidobject.cpp
// 3) Add any additional code to support new opcodes you create

typedef enum e_lltask_name
{
    LLTASK_NULL             = 0, // Not a valid task
    LLTASK_AGENT            = 1, // The player's agent in Linden World
    LLTASK_CHILD_AGENT      = 2, // Child agents sent to adjacent regions
//  LLTASK_BASIC_SHOT,      // Simple shot that moves in a straight line
//  LLTASK_BIG_SHOT,        // Big shot that uses gravity
    LLTASK_TREE             = 5, // A tree
//  LLTASK_BIRD,            // a bird
//  LLTASK_ATOR,            // a predator
//  LLTASK_SMOKE,           // Smoke poof
//  LLTASK_SPARK,           // Little spark
//  LLTASK_ROCK,            // Rock
    LLTASK_GRASS            = 11, // Grass
    LLTASK_PSYS             = 12, // particle system test example 
//  LLTASK_ORACLE,
//  LLTASK_DEMON,           // Maxwell's demon
//  LLTASK_LSL_TEST,        // Linden Scripting Language Test Object
    LLTASK_PRIMITIVE        = 16,
//  LLTASK_GHOST            = 17, // a ghost (Boo!)
    LLTASK_TREE_NEW         = 18
} ELLTaskName;
#endif
