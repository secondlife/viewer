/** 
 * @file legacy_object_types.h
 * @brief Byte codes for basic object and primitive types
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

#ifndef LL_LEGACY_OBJECT_TYPES_H
#define LL_LEGACY_OBJECT_TYPES_H

const	S8		PLAYER			= 'c';
//const	S8		BASIC_SHOT		= 's';
//const	S8		BIG_SHOT		= 'S';
//const	S8		TREE_SHOT		= 'g';
//const	S8		PHYSICAL_BALL	= 'b';

const	S8		TREE		= 'T';
const	S8		TREE_NEW	= 'R';
//const	S8		SPARK		= 'p';
//const	S8		SMOKE		= 'q';
//const	S8		BOX			= 'x';
//const	S8		CYLINDER	= 'y';
//const	S8		CONE		= 'o';
//const	S8		SPHERE		= 'h';
//const S8		BIRD		= 'r';			// ascii 114
//const S8		ATOR		= 'a';
//const S8      ROCK		= 'k';

const	S8		GRASS		= 'd';			
const   S8      PART_SYS    = 'P';

//const	S8		ORACLE		= 'O';
//const	S8		TEXTBUBBLE	= 't';			//  Text bubble to show communication
//const	S8		DEMON		= 'M';			// Maxwell's demon for scarfing legacy_object_types.h
//const	S8		CUBE		= 'f';
//const	S8		LSL_TEST	= 'L';
//const	S8		PRISM			= '1';
//const	S8		PYRAMID			= '2';
//const	S8		TETRAHEDRON		= '3';
//const	S8		HALF_CYLINDER	= '4';
//const	S8		HALF_CONE		= '5';
//const	S8		HALF_SPHERE		= '6';

const	S8		PRIMITIVE_VOLUME = 'v';

// Misc constants 

//const   F32		AVATAR_RADIUS			= 0.5f;
//const   F32		SHOT_RADIUS				= 0.05f;
//const   F32		BIG_SHOT_RADIUS			= 0.05f;
//const   F32		TREE_SIZE				= 5.f;
//const   F32		BALL_SIZE				= 4.f;

//const	F32		SHOT_VELOCITY			= 100.f;
//const	F32		GRENADE_BLAST_RADIUS	= 5.f;

#endif

