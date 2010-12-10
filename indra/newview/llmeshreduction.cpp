/** 
 * @file llmeshreduction.cpp
 * @brief LLMeshReduction class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llmeshreduction.h"
#include "llgl.h"
#include "llvertexbuffer.h"

#include "glod/glod.h"


#if 0 //not used ?

void create_vertex_buffers_from_model(LLModel* model, std::vector<LLPointer <LLVertexBuffer> >& vertex_buffers)
{
#if 0 //VECTORIZE THIS ?
	vertex_buffers.clear();
	
	for (S32 i = 0; i < model->getNumVolumeFaces(); ++i)
	{
		const LLVolumeFace &vf = model->getVolumeFace(i);
		U32 num_vertices = vf.mNumVertices;
		U32 num_indices = vf.mNumIndices;

		if (!num_vertices || ! num_indices)
		{
			continue;
		}

		LLVertexBuffer* vb =
			new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0, 0);
		
		vb->allocateBuffer(num_vertices, num_indices, TRUE);

		LLStrider<LLVector3> vertex_strider;
		LLStrider<LLVector3> normal_strider;
		LLStrider<LLVector2> tc_strider;
		LLStrider<U16> index_strider;

		vb->getVertexStrider(vertex_strider);
		vb->getNormalStrider(normal_strider);
		vb->getTexCoord0Strider(tc_strider);

		vb->getIndexStrider(index_strider);

		// build vertices and normals
		for (U32 i = 0; (S32)i < num_vertices; i++)
		{
			*(vertex_strider++) = vf.mVertices[i].mPosition;
			*(tc_strider++) = vf.mVertices[i].mTexCoord;
			LLVector3 normal = vf.mVertices[i].mNormal;
			normal.normalize();
			*(normal_strider++) = normal;
		}

		// build indices
		for (U32 i = 0; i < num_indices; i++)
		{
			*(index_strider++) = vf.mIndices[i];
		}


		vertex_buffers.push_back(vb);
	}
#endif
}

void create_glod_object_from_vertex_buffers(S32 object, S32 group, std::vector<LLPointer <LLVertexBuffer> >& vertex_buffers)
{
	glodNewGroup(group);
	stop_gloderror();
	glodNewObject(object, group, GLOD_DISCRETE);
	stop_gloderror();

	for (U32 i = 0; i < vertex_buffers.size(); ++i)
	{
		vertex_buffers[i]->setBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0);
		
		U32 num_indices = vertex_buffers[i]->getNumIndices();
		
		if (num_indices > 2)
		{
			glodInsertElements(object, i, GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT,
							   vertex_buffers[i]->getIndicesPointer(), 0, 0.f);
		}
		stop_gloderror();
	}
	
	glodBuildObject(object);
	stop_gloderror();
}

// extract the GLOD data into vertex buffers
void create_vertex_buffers_from_glod_object(S32 object, S32 group, std::vector<LLPointer <LLVertexBuffer> >& vertex_buffers)
{
	vertex_buffers.clear();
	
	GLint patch_count = 0;
	glodGetObjectParameteriv(object, GLOD_NUM_PATCHES, &patch_count);
	stop_gloderror();

	GLint* sizes = new GLint[patch_count*2];
	glodGetObjectParameteriv(object, GLOD_PATCH_SIZES, sizes);
	stop_gloderror();

	GLint* names = new GLint[patch_count];
	glodGetObjectParameteriv(object, GLOD_PATCH_NAMES, names);
	stop_gloderror();

	for (S32 i = 0; i < patch_count; i++)
	{
		LLPointer<LLVertexBuffer> buff =
			new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0, 0);
			
		if (sizes[i*2+1] > 0 && sizes[i*2] > 0)
		{
			buff->allocateBuffer(sizes[i*2+1], sizes[i*2], true);
			buff->setBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0);
			glodFillElements(object, names[i], GL_UNSIGNED_SHORT, buff->getIndicesPointer());
			stop_gloderror();
		}
		else
		{
            // this face was eliminated, create a dummy triangle (one vertex, 3 indices, all 0)
			buff->allocateBuffer(1, 3, true);
		}

		vertex_buffers.push_back(buff);
	}
	
	glodDeleteObject(object);
	stop_gloderror();
	glodDeleteGroup(group);
	stop_gloderror();
	
	delete [] sizes;
	delete [] names;
}


LLPointer<LLModel> create_model_from_vertex_buffers(std::vector<LLPointer <LLVertexBuffer> >& vertex_buffers)
{
	// extract the newly reduced mesh

	// create our output model
	LLVolumeParams volume_params;
	volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);
	LLPointer<LLModel> out_model = new LLModel(volume_params, 0.f);

	out_model->setNumVolumeFaces(vertex_buffers.size());

	// build new faces from each vertex buffer
	for (GLint i = 0; i < vertex_buffers.size(); ++i)
	{
		LLStrider<LLVector3> pos;
		LLStrider<LLVector3> norm;
		LLStrider<LLVector2> tc;
		LLStrider<U16> index;

		vertex_buffers[i]->getVertexStrider(pos);
		vertex_buffers[i]->getNormalStrider(norm);
		vertex_buffers[i]->getTexCoord0Strider(tc);
		vertex_buffers[i]->getIndexStrider(index);

		out_model->setVolumeFaceData(i, pos, norm, tc, index,
									 vertex_buffers[i]->getNumVerts(), vertex_buffers[i]->getNumIndices());
	}
	
	return out_model;
}



LLMeshReduction::LLMeshReduction()
{
	mCounter = 1;

	glodInit();
}

LLMeshReduction::~LLMeshReduction()
{
	glodShutdown();
}


LLPointer<LLModel> LLMeshReduction::reduce(LLModel* in_model, F32 limit, S32 mode)
{
	LLVertexBuffer::unbind();

	// create vertex buffers from model
	std::vector<LLPointer<LLVertexBuffer> > in_vertex_buffers;
	create_vertex_buffers_from_model(in_model, in_vertex_buffers);

	// create glod object from vertex buffers
	stop_gloderror();
	S32 glod_group = mCounter++;
	S32 glod_object = mCounter++;
	create_glod_object_from_vertex_buffers(glod_object, glod_group, in_vertex_buffers);

	
	// set reduction parameters
	stop_gloderror();

	if (mode == TRIANGLE_BUDGET)
	{
		// triangle budget mode
		glodGroupParameteri(glod_group, GLOD_ADAPT_MODE, GLOD_TRIANGLE_BUDGET);
		stop_gloderror();		
		glodGroupParameteri(glod_group, GLOD_ERROR_MODE, GLOD_OBJECT_SPACE_ERROR);
		stop_gloderror();		
		S32 triangle_count = (S32)limit;
		glodGroupParameteri(glod_group, GLOD_MAX_TRIANGLES, triangle_count);
		stop_gloderror();		
	}
	else if (mode == ERROR_THRESHOLD)
	{ 
		// error threshold mode
		glodGroupParameteri(glod_group, GLOD_ADAPT_MODE, GLOD_ERROR_THRESHOLD);
		glodGroupParameteri(glod_group, GLOD_ERROR_MODE, GLOD_OBJECT_SPACE_ERROR);
		F32 error_threshold = limit;
		glodGroupParameterf(glod_group, GLOD_OBJECT_SPACE_ERROR_THRESHOLD, error_threshold);
		stop_gloderror();
	}
	
	else
	{
		// not a legal mode
		return NULL;
	}


	// do the reduction
	glodAdaptGroup(glod_group);
	stop_gloderror();

	// convert glod object into vertex buffers
	std::vector<LLPointer<LLVertexBuffer> > out_vertex_buffers;
	create_vertex_buffers_from_glod_object(glod_object, glod_group, out_vertex_buffers);

	// convert vertex buffers into a model
	LLPointer<LLModel> out_model = create_model_from_vertex_buffers(out_vertex_buffers);

	
	return out_model;
}

#endif
