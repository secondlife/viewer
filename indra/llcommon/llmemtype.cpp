/** 
 * @file llmemtype.cpp
 * @brief Simple memory allocation/deallocation tracking stuff here
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

#include "llmemtype.h"
#include "llallocator.h"

std::vector<char const *> LLMemType::DeclareMemType::mNameList;

LLMemType::DeclareMemType LLMemType::MTYPE_INIT("Init");
LLMemType::DeclareMemType LLMemType::MTYPE_STARTUP("Startup");
LLMemType::DeclareMemType LLMemType::MTYPE_MAIN("Main");
LLMemType::DeclareMemType LLMemType::MTYPE_FRAME("Frame");

LLMemType::DeclareMemType LLMemType::MTYPE_GATHER_INPUT("GatherInput");
LLMemType::DeclareMemType LLMemType::MTYPE_JOY_KEY("JoyKey");

LLMemType::DeclareMemType LLMemType::MTYPE_IDLE("Idle");
LLMemType::DeclareMemType LLMemType::MTYPE_IDLE_PUMP("IdlePump");
LLMemType::DeclareMemType LLMemType::MTYPE_IDLE_NETWORK("IdleNetwork");
LLMemType::DeclareMemType LLMemType::MTYPE_IDLE_UPDATE_REGIONS("IdleUpdateRegions");
LLMemType::DeclareMemType LLMemType::MTYPE_IDLE_UPDATE_VIEWER_REGION("IdleUpdateViewerRegion");
LLMemType::DeclareMemType LLMemType::MTYPE_IDLE_UPDATE_SURFACE("IdleUpdateSurface");
LLMemType::DeclareMemType LLMemType::MTYPE_IDLE_UPDATE_PARCEL_OVERLAY("IdleUpdateParcelOverlay");
LLMemType::DeclareMemType LLMemType::MTYPE_IDLE_AUDIO("IdleAudio");

LLMemType::DeclareMemType LLMemType::MTYPE_CACHE_PROCESS_PENDING("CacheProcessPending");
LLMemType::DeclareMemType LLMemType::MTYPE_CACHE_PROCESS_PENDING_ASKS("CacheProcessPendingAsks");
LLMemType::DeclareMemType LLMemType::MTYPE_CACHE_PROCESS_PENDING_REPLIES("CacheProcessPendingReplies");

LLMemType::DeclareMemType LLMemType::MTYPE_MESSAGE_CHECK_ALL("MessageCheckAll");
LLMemType::DeclareMemType LLMemType::MTYPE_MESSAGE_PROCESS_ACKS("MessageProcessAcks");

LLMemType::DeclareMemType LLMemType::MTYPE_RENDER("Render");
LLMemType::DeclareMemType LLMemType::MTYPE_SLEEP("Sleep");

LLMemType::DeclareMemType LLMemType::MTYPE_NETWORK("Network");
LLMemType::DeclareMemType LLMemType::MTYPE_PHYSICS("Physics");
LLMemType::DeclareMemType LLMemType::MTYPE_INTERESTLIST("InterestList");

LLMemType::DeclareMemType LLMemType::MTYPE_IMAGEBASE("ImageBase");
LLMemType::DeclareMemType LLMemType::MTYPE_IMAGERAW("ImageRaw");
LLMemType::DeclareMemType LLMemType::MTYPE_IMAGEFORMATTED("ImageFormatted");
		
LLMemType::DeclareMemType LLMemType::MTYPE_APPFMTIMAGE("AppFmtImage");
LLMemType::DeclareMemType LLMemType::MTYPE_APPRAWIMAGE("AppRawImage");
LLMemType::DeclareMemType LLMemType::MTYPE_APPAUXRAWIMAGE("AppAuxRawImage");
		
LLMemType::DeclareMemType LLMemType::MTYPE_DRAWABLE("Drawable");

LLMemType::DeclareMemType LLMemType::MTYPE_OBJECT("Object");
LLMemType::DeclareMemType LLMemType::MTYPE_OBJECT_PROCESS_UPDATE("ObjectProcessUpdate");
LLMemType::DeclareMemType LLMemType::MTYPE_OBJECT_PROCESS_UPDATE_CORE("ObjectProcessUpdateCore");

LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY("Display");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_UPDATE("DisplayUpdate");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_UPDATE_CAMERA("DisplayUpdateCam");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_UPDATE_GEOM("DisplayUpdateGeom");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_SWAP("DisplaySwap");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_UPDATE_HUD("DisplayUpdateHud");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_GEN_REFLECTION("DisplayGenRefl");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_IMAGE_UPDATE("DisplayImageUpdate");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_STATE_SORT("DisplayStateSort");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_SKY("DisplaySky");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_RENDER_GEOM("DisplayRenderGeom");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_RENDER_FLUSH("DisplayRenderFlush");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_RENDER_UI("DisplayRenderUI");
LLMemType::DeclareMemType LLMemType::MTYPE_DISPLAY_RENDER_ATTACHMENTS("DisplayRenderAttach");

LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_DATA("VertexData");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_CONSTRUCTOR("VertexConstr");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_DESTRUCTOR("VertexDestr");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_CREATE_VERTICES("VertexCreateVerts");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_CREATE_INDICES("VertexCreateIndices");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_DESTROY_BUFFER("VertexDestroyBuff");	
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_DESTROY_INDICES("VertexDestroyIndices");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_UPDATE_VERTS("VertexUpdateVerts");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_UPDATE_INDICES("VertexUpdateIndices");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_ALLOCATE_BUFFER("VertexAllocateBuffer");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_RESIZE_BUFFER("VertexResizeBuffer");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_MAP_BUFFER("VertexMapBuffer");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_MAP_BUFFER_VERTICES("VertexMapBufferVerts");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_MAP_BUFFER_INDICES("VertexMapBufferIndices");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_UNMAP_BUFFER("VertexUnmapBuffer");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_SET_STRIDE("VertexSetStride");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_SET_BUFFER("VertexSetBuffer");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_SETUP_VERTEX_BUFFER("VertexSetupVertBuff");
LLMemType::DeclareMemType LLMemType::MTYPE_VERTEX_CLEANUP_CLASS("VertexCleanupClass");

LLMemType::DeclareMemType LLMemType::MTYPE_SPACE_PARTITION("SpacePartition");

LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE("Pipeline");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_INIT("PipelineInit");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_CREATE_BUFFERS("PipelineCreateBuffs");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_RESTORE_GL("PipelineRestroGL");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_UNLOAD_SHADERS("PipelineUnloadShaders");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_LIGHTING_DETAIL("PipelineLightingDetail");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_GET_POOL_TYPE("PipelineGetPoolType");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_ADD_POOL("PipelineAddPool");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_ALLOCATE_DRAWABLE("PipelineAllocDrawable");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_ADD_OBJECT("PipelineAddObj");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_CREATE_OBJECTS("PipelineCreateObjs");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_UPDATE_MOVE("PipelineUpdateMove");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_UPDATE_GEOM("PipelineUpdateGeom");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_MARK_VISIBLE("PipelineMarkVisible");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_MARK_MOVED("PipelineMarkMoved");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_MARK_SHIFT("PipelineMarkShift");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_SHIFT_OBJECTS("PipelineShiftObjs");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_MARK_TEXTURED("PipelineMarkTextured");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_MARK_REBUILD("PipelineMarkRebuild");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_UPDATE_CULL("PipelineUpdateCull");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_STATE_SORT("PipelineStateSort");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_POST_SORT("PipelinePostSort");
		
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_RENDER_HUD_ELS("PipelineHudEls");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_RENDER_HL("PipelineRenderHL");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_RENDER_GEOM("PipelineRenderGeom");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_RENDER_GEOM_DEFFERRED("PipelineRenderGeomDef");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_RENDER_GEOM_POST_DEF("PipelineRenderGeomPostDef");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_RENDER_GEOM_SHADOW("PipelineRenderGeomShadow");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_RENDER_SELECT("PipelineRenderSelect");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_REBUILD_POOLS("PipelineRebuildPools");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_QUICK_LOOKUP("PipelineQuickLookup");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_RENDER_OBJECTS("PipelineRenderObjs");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_GENERATE_IMPOSTOR("PipelineGenImpostors");
LLMemType::DeclareMemType LLMemType::MTYPE_PIPELINE_RENDER_BLOOM("PipelineRenderBloom");

LLMemType::DeclareMemType LLMemType::MTYPE_UPKEEP_POOLS("UpkeepPools");

LLMemType::DeclareMemType LLMemType::MTYPE_AVATAR("Avatar");
LLMemType::DeclareMemType LLMemType::MTYPE_AVATAR_MESH("AvatarMesh");
LLMemType::DeclareMemType LLMemType::MTYPE_PARTICLES("Particles");
LLMemType::DeclareMemType LLMemType::MTYPE_REGIONS("Regions");

LLMemType::DeclareMemType LLMemType::MTYPE_INVENTORY("Inventory");
LLMemType::DeclareMemType LLMemType::MTYPE_INVENTORY_DRAW("InventoryDraw");
LLMemType::DeclareMemType LLMemType::MTYPE_INVENTORY_BUILD_NEW_VIEWS("InventoryBuildNewViews");
LLMemType::DeclareMemType LLMemType::MTYPE_INVENTORY_DO_FOLDER("InventoryDoFolder");
LLMemType::DeclareMemType LLMemType::MTYPE_INVENTORY_POST_BUILD("InventoryPostBuild");
LLMemType::DeclareMemType LLMemType::MTYPE_INVENTORY_FROM_XML("InventoryFromXML");
LLMemType::DeclareMemType LLMemType::MTYPE_INVENTORY_CREATE_NEW_ITEM("InventoryCreateNewItem");
LLMemType::DeclareMemType LLMemType::MTYPE_INVENTORY_VIEW_INIT("InventoryViewInit");
LLMemType::DeclareMemType LLMemType::MTYPE_INVENTORY_VIEW_SHOW("InventoryViewShow");
LLMemType::DeclareMemType LLMemType::MTYPE_INVENTORY_VIEW_TOGGLE("InventoryViewToggle");

LLMemType::DeclareMemType LLMemType::MTYPE_ANIMATION("Animation");
LLMemType::DeclareMemType LLMemType::MTYPE_VOLUME("Volume");
LLMemType::DeclareMemType LLMemType::MTYPE_PRIMITIVE("Primitive");
		
LLMemType::DeclareMemType LLMemType::MTYPE_SCRIPT("Script");
LLMemType::DeclareMemType LLMemType::MTYPE_SCRIPT_RUN("ScriptRun");
LLMemType::DeclareMemType LLMemType::MTYPE_SCRIPT_BYTECODE("ScriptByteCode");
		
LLMemType::DeclareMemType LLMemType::MTYPE_IO_PUMP("IoPump");
LLMemType::DeclareMemType LLMemType::MTYPE_IO_TCP("IoTCP");
LLMemType::DeclareMemType LLMemType::MTYPE_IO_BUFFER("IoBuffer");
LLMemType::DeclareMemType LLMemType::MTYPE_IO_HTTP_SERVER("IoHttpServer");
LLMemType::DeclareMemType LLMemType::MTYPE_IO_SD_SERVER("IoSDServer");
LLMemType::DeclareMemType LLMemType::MTYPE_IO_SD_CLIENT("IoSDClient");
LLMemType::DeclareMemType LLMemType::MTYPE_IO_URL_REQUEST("IOUrlRequest");

LLMemType::DeclareMemType LLMemType::MTYPE_DIRECTX_INIT("DirectXInit");

LLMemType::DeclareMemType LLMemType::MTYPE_TEMP1("Temp1");
LLMemType::DeclareMemType LLMemType::MTYPE_TEMP2("Temp2");
LLMemType::DeclareMemType LLMemType::MTYPE_TEMP3("Temp3");
LLMemType::DeclareMemType LLMemType::MTYPE_TEMP4("Temp4");
LLMemType::DeclareMemType LLMemType::MTYPE_TEMP5("Temp5");
LLMemType::DeclareMemType LLMemType::MTYPE_TEMP6("Temp6");
LLMemType::DeclareMemType LLMemType::MTYPE_TEMP7("Temp7");
LLMemType::DeclareMemType LLMemType::MTYPE_TEMP8("Temp8");
LLMemType::DeclareMemType LLMemType::MTYPE_TEMP9("Temp9");

LLMemType::DeclareMemType LLMemType::MTYPE_OTHER("Other");


LLMemType::DeclareMemType::DeclareMemType(char const * st)
{
	mID = (S32)mNameList.size();
	mName = st;

	mNameList.push_back(mName);
}

LLMemType::DeclareMemType::~DeclareMemType()
{
}

LLMemType::LLMemType(LLMemType::DeclareMemType& dt)
{
	mTypeIndex = dt.mID;
	LLAllocator::pushMemType(dt.mID);
}

LLMemType::~LLMemType()
{
	LLAllocator::popMemType();
}

char const * LLMemType::getNameFromID(S32 id)
{
	if (id < 0 || id >= (S32)DeclareMemType::mNameList.size())
	{
		return "INVALID";
	}

	return DeclareMemType::mNameList[id];
}

//--------------------------------------------------------------------------------------------------
