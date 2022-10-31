/** 
 * @file indra_constants.cpp
 * @brief some useful short term constants for Indra
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
#include "linden_common.h"

#include "indra_constants.h"

#include "lluuid.h"

// "agent id" for things that should be done to ALL agents
const LLUUID LL_UUID_ALL_AGENTS("44e87126-e794-4ded-05b3-7c42da3d5cdb");

// Governor Linden's agent id.
const LLUUID ALEXANDRIA_LINDEN_ID("ba2a564a-f0f1-4b82-9c61-b7520bfcd09f");
const LLUUID GOVERNOR_LINDEN_ID("3d6181b0-6a4b-97ef-18d8-722652995cf1");
// Maintenance's group id.
const LLUUID MAINTENANCE_GROUP_ID("dc7b21cd-3c89-fcaa-31c8-25f9ffd224cd");
// Grass Images
const LLUUID IMG_SMOKE          ("b4ba225c-373f-446d-9f7e-6cb7b5cf9b3d");  // VIEWER

const LLUUID IMG_DEFAULT        ("d2114404-dd59-4a4d-8e6c-49359e91bbf0");  // VIEWER

const LLUUID IMG_SUN            ("cce0f112-878f-4586-a2e2-a8f104bba271"); // dataserver
const LLUUID IMG_MOON           ("d07f6eed-b96a-47cd-b51d-400ad4a1c428"); // dataserver
const LLUUID IMG_SHOT           ("35f217a3-f618-49cf-bbca-c86d486551a9"); // dataserver
const LLUUID IMG_SPARK          ("d2e75ac1-d0fb-4532-820e-a20034ac814d"); // dataserver
const LLUUID IMG_FIRE           ("aca40aa8-44cf-44ca-a0fa-93e1a2986f82"); // dataserver
const LLUUID IMG_FACE_SELECT    ("a85ac674-cb75-4af6-9499-df7c5aaf7a28"); // face selector
const LLUUID IMG_DEFAULT_AVATAR ("c228d1cf-4b5d-4ba8-84f4-899a0796aa97"); // dataserver
const LLUUID IMG_INVISIBLE      ("3a367d1c-bef1-6d43-7595-e88c1e3aadb3"); // dataserver

const LLUUID IMG_EXPLOSION              ("68edcf47-ccd7-45b8-9f90-1649d7f12806"); // On dataserver
const LLUUID IMG_EXPLOSION_2            ("21ce046c-83fe-430a-b629-c7660ac78d7c"); // On dataserver
const LLUUID IMG_EXPLOSION_3            ("fedea30a-1be8-47a6-bc06-337a04a39c4b"); // On dataserver
const LLUUID IMG_EXPLOSION_4            ("abf0d56b-82e5-47a2-a8ad-74741bb2c29e"); // On dataserver
const LLUUID IMG_SMOKE_POOF             ("1e63e323-5fe0-452e-92f8-b98bd0f764e3"); // On dataserver

const LLUUID IMG_BIG_EXPLOSION_1        ("5e47a0dc-97bf-44e0-8b40-de06718cee9d"); // On dataserver
const LLUUID IMG_BIG_EXPLOSION_2        ("9c8eca51-53d5-42a7-bb58-cef070395db8"); // On dataserver

const LLUUID IMG_ALPHA_GRAD             ("e97cf410-8e61-7005-ec06-629eba4cd1fb"); // VIEWER
const LLUUID IMG_ALPHA_GRAD_2D          ("38b86f85-2575-52a9-a531-23108d8da837"); // VIEWER
const LLUUID IMG_TRANSPARENT            ("8dcd4a48-2d37-4909-9f78-f7a9eb4ef903"); // VIEWER

const LLUUID TERRAIN_DIRT_DETAIL        ("0bc58228-74a0-7e83-89bc-5c23464bcec5"); // VIEWER
const LLUUID TERRAIN_GRASS_DETAIL       ("63338ede-0037-c4fd-855b-015d77112fc8"); // VIEWER
const LLUUID TERRAIN_MOUNTAIN_DETAIL    ("303cd381-8560-7579-23f1-f0a880799740"); // VIEWER
const LLUUID TERRAIN_ROCK_DETAIL        ("53a2f406-4895-1d13-d541-d2e3b86bc19c"); // VIEWER

const LLUUID DEFAULT_WATER_NORMAL       ("822ded49-9a6c-f61c-cb89-6df54f42cdf4"); // VIEWER

const LLUUID IMG_USE_BAKED_HEAD  ("5a9f4a74-30f2-821c-b88d-70499d3e7183");
const LLUUID IMG_USE_BAKED_UPPER ("ae2de45c-d252-50b8-5c6e-19f39ce79317");
const LLUUID IMG_USE_BAKED_LOWER ("24daea5f-0539-cfcf-047f-fbc40b2786ba");
const LLUUID IMG_USE_BAKED_EYES  ("52cc6bb6-2ee5-e632-d3ad-50197b1dcb8a");
const LLUUID IMG_USE_BAKED_SKIRT ("43529ce8-7faa-ad92-165a-bc4078371687");
const LLUUID IMG_USE_BAKED_HAIR  ("09aac1fb-6bce-0bee-7d44-caac6dbb6c63");
const LLUUID IMG_USE_BAKED_LEFTARM  ("ff62763f-d60a-9855-890b-0c96f8f8cd98");
const LLUUID IMG_USE_BAKED_LEFTLEG  ("8e915e25-31d1-cc95-ae08-d58a47488251");
const LLUUID IMG_USE_BAKED_AUX1  ("9742065b-19b5-297c-858a-29711d539043");
const LLUUID IMG_USE_BAKED_AUX2  ("03642e83-2bd1-4eb9-34b4-4c47ed586d2d");
const LLUUID IMG_USE_BAKED_AUX3  ("edd51b77-fc10-ce7a-4b3d-011dfc349e4f");

