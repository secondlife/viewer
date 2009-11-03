/** 
 * @file llmemtype.h
 * @brief Runtime memory usage debugging utilities.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#ifndef LL_MEMTYPE_H
#define LL_MEMTYPE_H

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

#include "linden_common.h"
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// WARNING: Never commit with MEM_TRACK_MEM == 1
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define MEM_TRACK_MEM (0 && LL_WINDOWS)

#include <vector>

#define MEM_TYPE_NEW(T)

class LL_COMMON_API LLMemType
{
public:

	// class we'll initialize all instances of as
	// static members of MemType.  Then use
	// to construct any new mem type.
	class LL_COMMON_API DeclareMemType
	{
	public:
		DeclareMemType(char const * st);
		~DeclareMemType();
	
		S32 mID;
		char const * mName;
		
		// array so we can map an index ID to Name
		static std::vector<char const *> mNameList;
	};

	LLMemType(DeclareMemType& dt);
	~LLMemType();

	static char const * getNameFromID(S32 id);

	static DeclareMemType MTYPE_INIT;
	static DeclareMemType MTYPE_STARTUP;
	static DeclareMemType MTYPE_MAIN;
	static DeclareMemType MTYPE_FRAME;

	static DeclareMemType MTYPE_GATHER_INPUT;
	static DeclareMemType MTYPE_JOY_KEY;

	static DeclareMemType MTYPE_IDLE;
	static DeclareMemType MTYPE_IDLE_PUMP;
	static DeclareMemType MTYPE_IDLE_NETWORK;
	static DeclareMemType MTYPE_IDLE_UPDATE_REGIONS;
	static DeclareMemType MTYPE_IDLE_UPDATE_VIEWER_REGION;
	static DeclareMemType MTYPE_IDLE_UPDATE_SURFACE;
	static DeclareMemType MTYPE_IDLE_UPDATE_PARCEL_OVERLAY;
	static DeclareMemType MTYPE_IDLE_AUDIO;

	static DeclareMemType MTYPE_CACHE_PROCESS_PENDING;
	static DeclareMemType MTYPE_CACHE_PROCESS_PENDING_ASKS;
	static DeclareMemType MTYPE_CACHE_PROCESS_PENDING_REPLIES;

	static DeclareMemType MTYPE_MESSAGE_CHECK_ALL;
	static DeclareMemType MTYPE_MESSAGE_PROCESS_ACKS;

	static DeclareMemType MTYPE_RENDER;
	static DeclareMemType MTYPE_SLEEP;

	static DeclareMemType MTYPE_NETWORK;
	static DeclareMemType MTYPE_PHYSICS;
	static DeclareMemType MTYPE_INTERESTLIST;

	static DeclareMemType MTYPE_IMAGEBASE;
	static DeclareMemType MTYPE_IMAGERAW;
	static DeclareMemType MTYPE_IMAGEFORMATTED;
	
	static DeclareMemType MTYPE_APPFMTIMAGE;
	static DeclareMemType MTYPE_APPRAWIMAGE;
	static DeclareMemType MTYPE_APPAUXRAWIMAGE;
	
	static DeclareMemType MTYPE_DRAWABLE;
	
	static DeclareMemType MTYPE_OBJECT;
	static DeclareMemType MTYPE_OBJECT_PROCESS_UPDATE;
	static DeclareMemType MTYPE_OBJECT_PROCESS_UPDATE_CORE;

	static DeclareMemType MTYPE_DISPLAY;
	static DeclareMemType MTYPE_DISPLAY_UPDATE;
	static DeclareMemType MTYPE_DISPLAY_UPDATE_CAMERA;
	static DeclareMemType MTYPE_DISPLAY_UPDATE_GEOM;
	static DeclareMemType MTYPE_DISPLAY_SWAP;
	static DeclareMemType MTYPE_DISPLAY_UPDATE_HUD;
	static DeclareMemType MTYPE_DISPLAY_GEN_REFLECTION;
	static DeclareMemType MTYPE_DISPLAY_IMAGE_UPDATE;
	static DeclareMemType MTYPE_DISPLAY_STATE_SORT;
	static DeclareMemType MTYPE_DISPLAY_SKY;
	static DeclareMemType MTYPE_DISPLAY_RENDER_GEOM;
	static DeclareMemType MTYPE_DISPLAY_RENDER_FLUSH;
	static DeclareMemType MTYPE_DISPLAY_RENDER_UI;
	static DeclareMemType MTYPE_DISPLAY_RENDER_ATTACHMENTS;

	static DeclareMemType MTYPE_VERTEX_DATA;
	static DeclareMemType MTYPE_VERTEX_CONSTRUCTOR;
	static DeclareMemType MTYPE_VERTEX_DESTRUCTOR;
	static DeclareMemType MTYPE_VERTEX_CREATE_VERTICES;
	static DeclareMemType MTYPE_VERTEX_CREATE_INDICES;
	static DeclareMemType MTYPE_VERTEX_DESTROY_BUFFER;	
	static DeclareMemType MTYPE_VERTEX_DESTROY_INDICES;
	static DeclareMemType MTYPE_VERTEX_UPDATE_VERTS;
	static DeclareMemType MTYPE_VERTEX_UPDATE_INDICES;
	static DeclareMemType MTYPE_VERTEX_ALLOCATE_BUFFER;
	static DeclareMemType MTYPE_VERTEX_RESIZE_BUFFER;
	static DeclareMemType MTYPE_VERTEX_MAP_BUFFER;
	static DeclareMemType MTYPE_VERTEX_MAP_BUFFER_VERTICES;
	static DeclareMemType MTYPE_VERTEX_MAP_BUFFER_INDICES;
	static DeclareMemType MTYPE_VERTEX_UNMAP_BUFFER;
	static DeclareMemType MTYPE_VERTEX_SET_STRIDE;
	static DeclareMemType MTYPE_VERTEX_SET_BUFFER;
	static DeclareMemType MTYPE_VERTEX_SETUP_VERTEX_BUFFER;
	static DeclareMemType MTYPE_VERTEX_CLEANUP_CLASS;

	static DeclareMemType MTYPE_SPACE_PARTITION;

	static DeclareMemType MTYPE_PIPELINE;
	static DeclareMemType MTYPE_PIPELINE_INIT;
	static DeclareMemType MTYPE_PIPELINE_CREATE_BUFFERS;
	static DeclareMemType MTYPE_PIPELINE_RESTORE_GL;
	static DeclareMemType MTYPE_PIPELINE_UNLOAD_SHADERS;
	static DeclareMemType MTYPE_PIPELINE_LIGHTING_DETAIL;
	static DeclareMemType MTYPE_PIPELINE_GET_POOL_TYPE;
	static DeclareMemType MTYPE_PIPELINE_ADD_POOL;
	static DeclareMemType MTYPE_PIPELINE_ALLOCATE_DRAWABLE;
	static DeclareMemType MTYPE_PIPELINE_ADD_OBJECT;
	static DeclareMemType MTYPE_PIPELINE_CREATE_OBJECTS;
	static DeclareMemType MTYPE_PIPELINE_UPDATE_MOVE;
	static DeclareMemType MTYPE_PIPELINE_UPDATE_GEOM;
	static DeclareMemType MTYPE_PIPELINE_MARK_VISIBLE;
	static DeclareMemType MTYPE_PIPELINE_MARK_MOVED;
	static DeclareMemType MTYPE_PIPELINE_MARK_SHIFT;
	static DeclareMemType MTYPE_PIPELINE_SHIFT_OBJECTS;
	static DeclareMemType MTYPE_PIPELINE_MARK_TEXTURED;
	static DeclareMemType MTYPE_PIPELINE_MARK_REBUILD;
	static DeclareMemType MTYPE_PIPELINE_UPDATE_CULL;
	static DeclareMemType MTYPE_PIPELINE_STATE_SORT;
	static DeclareMemType MTYPE_PIPELINE_POST_SORT;
	
	static DeclareMemType MTYPE_PIPELINE_RENDER_HUD_ELS;
	static DeclareMemType MTYPE_PIPELINE_RENDER_HL;
	static DeclareMemType MTYPE_PIPELINE_RENDER_GEOM;
	static DeclareMemType MTYPE_PIPELINE_RENDER_GEOM_DEFFERRED;
	static DeclareMemType MTYPE_PIPELINE_RENDER_GEOM_POST_DEF;
	static DeclareMemType MTYPE_PIPELINE_RENDER_GEOM_SHADOW;
	static DeclareMemType MTYPE_PIPELINE_RENDER_SELECT;
	static DeclareMemType MTYPE_PIPELINE_REBUILD_POOLS;
	static DeclareMemType MTYPE_PIPELINE_QUICK_LOOKUP;
	static DeclareMemType MTYPE_PIPELINE_RENDER_OBJECTS;
	static DeclareMemType MTYPE_PIPELINE_GENERATE_IMPOSTOR;
	static DeclareMemType MTYPE_PIPELINE_RENDER_BLOOM;

	static DeclareMemType MTYPE_UPKEEP_POOLS;

	static DeclareMemType MTYPE_AVATAR;
	static DeclareMemType MTYPE_AVATAR_MESH;
	static DeclareMemType MTYPE_PARTICLES;
	static DeclareMemType MTYPE_REGIONS;

	static DeclareMemType MTYPE_INVENTORY;
	static DeclareMemType MTYPE_INVENTORY_DRAW;
	static DeclareMemType MTYPE_INVENTORY_BUILD_NEW_VIEWS;
	static DeclareMemType MTYPE_INVENTORY_DO_FOLDER;
	static DeclareMemType MTYPE_INVENTORY_POST_BUILD;
	static DeclareMemType MTYPE_INVENTORY_FROM_XML;
	static DeclareMemType MTYPE_INVENTORY_CREATE_NEW_ITEM;
	static DeclareMemType MTYPE_INVENTORY_VIEW_INIT;
	static DeclareMemType MTYPE_INVENTORY_VIEW_SHOW;
	static DeclareMemType MTYPE_INVENTORY_VIEW_TOGGLE;

	static DeclareMemType MTYPE_ANIMATION;
	static DeclareMemType MTYPE_VOLUME;
	static DeclareMemType MTYPE_PRIMITIVE;
	
	static DeclareMemType MTYPE_SCRIPT;
	static DeclareMemType MTYPE_SCRIPT_RUN;
	static DeclareMemType MTYPE_SCRIPT_BYTECODE;
	
	static DeclareMemType MTYPE_IO_PUMP;
	static DeclareMemType MTYPE_IO_TCP;
	static DeclareMemType MTYPE_IO_BUFFER;
	static DeclareMemType MTYPE_IO_HTTP_SERVER;
	static DeclareMemType MTYPE_IO_SD_SERVER;
	static DeclareMemType MTYPE_IO_SD_CLIENT;
	static DeclareMemType MTYPE_IO_URL_REQUEST;

	static DeclareMemType MTYPE_DIRECTX_INIT;

	static DeclareMemType MTYPE_TEMP1;
	static DeclareMemType MTYPE_TEMP2;
	static DeclareMemType MTYPE_TEMP3;
	static DeclareMemType MTYPE_TEMP4;
	static DeclareMemType MTYPE_TEMP5;
	static DeclareMemType MTYPE_TEMP6;
	static DeclareMemType MTYPE_TEMP7;
	static DeclareMemType MTYPE_TEMP8;
	static DeclareMemType MTYPE_TEMP9;

	static DeclareMemType MTYPE_OTHER; // Special; used by display code

	S32 mTypeIndex;
};

//----------------------------------------------------------------------------

#endif

