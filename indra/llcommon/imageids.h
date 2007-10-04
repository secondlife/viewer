/** 
 * @file imageids.h
 * @brief Temporary holder for image IDs
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_IMAGEIDS_H
#define LL_IMAGEIDS_H

#include "lluuid.h"

//
// USE OF THIS FILE IS DEPRECATED
//
// Please use viewerart.ini and the standard
// art import path.																			// indicates if file is only
			 															// on dataserver, or also
																			// pre-cached on viewer

// Grass Images
//const LLUUID IMG_GRASS1			("990c4086-46ce-49bd-8cae-afcc23a08f4e"); // dataserver
//const LLUUID IMG_GRASS2			("869e2dcf-21b9-402d-a36d-9a23365cf723"); // dataserver
//const LLUUID IMG_GRASS3			("8f97e7a7-f664-4967-9e8f-8d9e8039c1b7"); // dataserver

//const LLUUID IMG_GRASS4			("8a05131d-35b7-4812-bcfc-a989b0f954ef"); // dataserver

//const LLUUID IMG_GRASS5			("7d092acb-c69a-4122-b09b-f285e009b185"); // dataserver

const LLUUID IMG_CLEAR			("11ee27f5-43c0-414e-afd5-d7f5688c351f");  // VIEWER
const LLUUID IMG_SMOKE			("b4ba225c-373f-446d-9f7e-6cb7b5cf9b3d");  // VIEWER

const LLUUID IMG_DEFAULT		("d2114404-dd59-4a4d-8e6c-49359e91bbf0");  // VIEWER

//const LLUUID IMG_SAND			("0ff70ead-4562-45f9-9e8a-52b1a3286868"); // VIEWER 1.5k
//const LLUUID IMG_GRASS			("5ab48dd5-05d0-4f1a-ace6-efd4e2fb3508"); // VIEWER 1.2k
//const LLUUID IMG_ROCK			("402f8b24-5f9d-4905-b5f8-37baff603e88"); // VIEWER 1.2k
//const LLUUID IMG_ROCKFACE		("9c88539c-fd04-46b8-bea2-ddf1bcffe3bd"); // VIEWER 1.2k
const LLUUID IMG_SUN			("cce0f112-878f-4586-a2e2-a8f104bba271"); // dataserver
const LLUUID IMG_MOON			("d07f6eed-b96a-47cd-b51d-400ad4a1c428"); // dataserver
const LLUUID IMG_CLOUD_POOF		("fc4b9f0b-d008-45c6-96a4-01dd947ac621"); // dataserver
const LLUUID IMG_SHOT			("35f217a3-f618-49cf-bbca-c86d486551a9"); // dataserver
const LLUUID IMG_SPARK			("d2e75ac1-d0fb-4532-820e-a20034ac814d"); // dataserver
const LLUUID IMG_FIRE			("aca40aa8-44cf-44ca-a0fa-93e1a2986f82"); // dataserver
//const LLUUID IMG_WATER			("e510b068-d20d-4612-a08d-fde4d5c15789"); // VIEWER
const LLUUID IMG_FACE_SELECT    ("a85ac674-cb75-4af6-9499-df7c5aaf7a28"); // face selector

//const LLUUID IMG_SHADOW			("5e1de0a8-f9f8-4237-9396-d221126a7c4a"); // dataserver
//const LLUUID IMG_AVATARSHADOW	("c7d8bbf3-21ee-4f6e-9b20-3cf18425af1d"); // dataserver
//const LLUUID IMG_BOXSHADOW		("8d86b8cc-4889-408a-8b72-c1961bae53d7"); // dataserver
//const LLUUID IMG_EYE			("5e3551ae-9971-4814-af99-5117591e937b"); // dataserver
//const LLUUID IMG_BLUE_FLAME		("d8b62059-7b31-4511-a479-1fe45117948f"); // dataserver

const LLUUID IMG_DEFAULT_AVATAR ("c228d1cf-4b5d-4ba8-84f4-899a0796aa97"); // dataserver
//const LLUUID IMG_ENERGY_BEAM	("09e7bc54-11b9-442a-ae3d-f52e599e466a"); // dataserver
//const LLUUID IMG_ENERGY_BEAM2	("de651394-f926-48db-b666-e49d83af1bbc"); // dataserver

//const LLUUID IMG_BRICK_PATH		("a9d0019b-3783-4c7f-959c-322d301918bc"); // dataserver

const LLUUID IMG_EXPLOSION				("68edcf47-ccd7-45b8-9f90-1649d7f12806"); // On dataserver
const LLUUID IMG_EXPLOSION_2			("21ce046c-83fe-430a-b629-c7660ac78d7c"); // On dataserver
const LLUUID IMG_EXPLOSION_3			("fedea30a-1be8-47a6-bc06-337a04a39c4b"); // On dataserver
const LLUUID IMG_EXPLOSION_4			("abf0d56b-82e5-47a2-a8ad-74741bb2c29e"); // On dataserver
//const LLUUID IMG_EXPLOSION_5			("60f2dec7-675b-4950-b614-85b907d552ea"); // On dataserver
//const LLUUID IMG_SPLASH_SPRITE			("8a101f63-fe45-49e7-9f8a-e64817daa475"); // On dataserver
const LLUUID IMG_SMOKE_POOF				("1e63e323-5fe0-452e-92f8-b98bd0f764e3"); // On dataserver

const LLUUID IMG_BIG_EXPLOSION_1		("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); // On dataserver
const LLUUID IMG_BIG_EXPLOSION_2		("9c8eca51-53d5-42a7-bb58-cef070395db8"); // On dataserver
//const LLUUID IMG_BLUE_BLOOD				("8bc2e3f8-097e-4c87-b417-b0d699d07189"); // On dataserver

const LLUUID IMG_BLOOM1	  			    ("3c59f7fe-9dc8-47f9-8aaf-a9dd1fbc3bef");
//const LLUUID IMG_BLOOM2	  			    ("9fb76e81-eca0-4b6a-96e1-a6c5a685150b");
//const LLUUID IMG_BLOOM3	  			    ("fb1fecba-9585-415b-ad15-6e6e3d6c5479");

const LLUUID IMG_PTT_SPEAKER            ("89e9fc7c-0b16-457d-be4f-136270759c4d"); // On cache

#endif
