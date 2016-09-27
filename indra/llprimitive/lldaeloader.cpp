/**
 * @file lldaeloader.cpp
 * @brief LLDAELoader class implementation
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#if LL_MSVC
#pragma warning (disable : 4263)
#pragma warning (disable : 4264)
#endif
#include "dae.h"
#include "dom/domAsset.h"
#include "dom/domBind_material.h"
#include "dom/domCOLLADA.h"
#include "dom/domConstants.h"
#include "dom/domController.h"
#include "dom/domEffect.h"
#include "dom/domGeometry.h"
#include "dom/domInstance_geometry.h"
#include "dom/domInstance_material.h"
#include "dom/domInstance_node.h"
#include "dom/domInstance_effect.h"
#include "dom/domMaterial.h"
#include "dom/domMatrix.h"
#include "dom/domNode.h"
#include "dom/domProfile_COMMON.h"
#include "dom/domRotate.h"
#include "dom/domScale.h"
#include "dom/domTranslate.h"
#include "dom/domVisual_scene.h"
#if LL_MSVC
#pragma warning (default : 4263)
#pragma warning (default : 4264)
#endif

#include <boost/lexical_cast.hpp>

#include "lldaeloader.h"
#include "llsdserialize.h"
#include "lljoint.h"

#include "glh/glh_linear.h"
#include "llmatrix4a.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>

std::string colladaVersion[VERSIONTYPE_COUNT+1] = 
{
	"1.4.0",
	"1.4.1",
	"Unsupported"
};

static const std::string lod_suffix[LLModel::NUM_LODS] =
{
	"_LOD0",
	"_LOD1",
	"_LOD2",
	"",
	"_PHYS",
};

const U32 LIMIT_MATERIALS_OUTPUT = 12;

bool get_dom_sources(const domInputLocalOffset_Array& inputs, S32& pos_offset, S32& tc_offset, S32& norm_offset, S32 &idx_stride,
	domSource* &pos_source, domSource* &tc_source, domSource* &norm_source)
{
	idx_stride = 0;

	for (U32 j = 0; j < inputs.getCount(); ++j)
	{
		idx_stride = llmax((S32) inputs[j]->getOffset(), idx_stride);

		if (strcmp(COMMON_PROFILE_INPUT_VERTEX, inputs[j]->getSemantic()) == 0)
		{ //found vertex array
			const domURIFragmentType& uri = inputs[j]->getSource();
			daeElementRef elem = uri.getElement();
			domVertices* vertices = (domVertices*) elem.cast();
			if ( !vertices )
			{
				return false;
			}

			domInputLocal_Array& v_inp = vertices->getInput_array();


			for (U32 k = 0; k < v_inp.getCount(); ++k)
			{
				if (strcmp(COMMON_PROFILE_INPUT_POSITION, v_inp[k]->getSemantic()) == 0)
				{
					pos_offset = inputs[j]->getOffset();

					const domURIFragmentType& uri = v_inp[k]->getSource();
					daeElementRef elem = uri.getElement();
					pos_source = (domSource*) elem.cast();
				}

				if (strcmp(COMMON_PROFILE_INPUT_NORMAL, v_inp[k]->getSemantic()) == 0)
				{
					norm_offset = inputs[j]->getOffset();

					const domURIFragmentType& uri = v_inp[k]->getSource();
					daeElementRef elem = uri.getElement();
					norm_source = (domSource*) elem.cast();
				}
			}
		}

		if (strcmp(COMMON_PROFILE_INPUT_NORMAL, inputs[j]->getSemantic()) == 0)
		{
			//found normal array for this triangle list
			norm_offset = inputs[j]->getOffset();
			const domURIFragmentType& uri = inputs[j]->getSource();
			daeElementRef elem = uri.getElement();
			norm_source = (domSource*) elem.cast();
		}
		else if (strcmp(COMMON_PROFILE_INPUT_TEXCOORD, inputs[j]->getSemantic()) == 0)
		{ //found texCoords
			tc_offset = inputs[j]->getOffset();
			const domURIFragmentType& uri = inputs[j]->getSource();
			daeElementRef elem = uri.getElement();
			tc_source = (domSource*) elem.cast();
		}
	}

	idx_stride += 1;

	return true;
}

LLModel::EModelStatus load_face_from_dom_triangles(std::vector<LLVolumeFace>& face_list, std::vector<std::string>& materials, domTrianglesRef& tri)
{
	LLVolumeFace face;
	std::vector<LLVolumeFace::VertexData> verts;
	std::vector<U16> indices;
	
	const domInputLocalOffset_Array& inputs = tri->getInput_array();

	S32 pos_offset = -1;
	S32 tc_offset = -1;
	S32 norm_offset = -1;

	domSource* pos_source = NULL;
	domSource* tc_source = NULL;
	domSource* norm_source = NULL;

	S32 idx_stride = 0;

	if ( !get_dom_sources(inputs, pos_offset, tc_offset, norm_offset, idx_stride, pos_source, tc_source, norm_source))
	{
		return LLModel::BAD_ELEMENT;
	}

	if (!pos_source || !pos_source->getFloat_array())
	{
		LL_WARNS() << "Unable to process mesh without position data; invalid model;  invalid model." << LL_ENDL;
		return LLModel::BAD_ELEMENT;
	}

	domPRef p = tri->getP();
	domListOfUInts& idx = p->getValue();
	
	domListOfFloats  dummy ;
	domListOfFloats& v = pos_source ? pos_source->getFloat_array()->getValue() : dummy ;
	domListOfFloats& tc = tc_source ? tc_source->getFloat_array()->getValue() : dummy ;
	domListOfFloats& n = norm_source ? norm_source->getFloat_array()->getValue() : dummy ;

	if (pos_source)
	{
		if(v.getCount() == 0)
		{
			return LLModel::BAD_ELEMENT;
		}

		face.mExtents[0].set(v[0], v[1], v[2]);
		face.mExtents[1].set(v[0], v[1], v[2]);
	}
	
	LLVolumeFace::VertexMapData::PointMap point_map;
	
	for (U32 i = 0; i < idx.getCount(); i += idx_stride)
	{
		LLVolumeFace::VertexData cv;
		if (pos_source)
		{
			cv.setPosition(LLVector4a(v[idx[i+pos_offset]*3+0],
								v[idx[i+pos_offset]*3+1],
								v[idx[i+pos_offset]*3+2]));
		}

		if (tc_source)
		{
			cv.mTexCoord.setVec(tc[idx[i+tc_offset]*2+0],
								tc[idx[i+tc_offset]*2+1]);
		}
		
		if (norm_source)
		{
			cv.setNormal(LLVector4a(n[idx[i+norm_offset]*3+0],
								n[idx[i+norm_offset]*3+1],
								n[idx[i+norm_offset]*3+2]));
		}
		
		BOOL found = FALSE;
			
		LLVolumeFace::VertexMapData::PointMap::iterator point_iter;
		point_iter = point_map.find(LLVector3(cv.getPosition().getF32ptr()));
		
		if (point_iter != point_map.end())
		{
			for (U32 j = 0; j < point_iter->second.size(); ++j)
			{
				// We have a matching loc
				//
				if ((point_iter->second)[j] == cv)
				{
					U16 shared_index	= (point_iter->second)[j].mIndex;

					// Don't share verts within the same tri, degenerate
					//
                    U32 indx_size = indices.size();
                    U32 verts_new_tri = indx_size % 3;
                    if ((verts_new_tri < 1 || indices[indx_size - 1] != shared_index)
                        && (verts_new_tri < 2 || indices[indx_size - 2] != shared_index))
					{
						found = true;
						indices.push_back(shared_index);
					}
					break;
				}
			}
		}

		if (!found)
		{
			update_min_max(face.mExtents[0], face.mExtents[1], cv.getPosition());
			verts.push_back(cv);
			if (verts.size() >= 65535)
			{
				//llerrs << "Attempted to write model exceeding 16-bit index buffer limitation." << LL_ENDL;
				return LLModel::VERTEX_NUMBER_OVERFLOW ;
			}
			U16 index = (U16) (verts.size()-1);
			indices.push_back(index);

			LLVolumeFace::VertexMapData d;
			d.setPosition(cv.getPosition());
			d.mTexCoord = cv.mTexCoord;
			d.setNormal(cv.getNormal());
			d.mIndex = index;
			if (point_iter != point_map.end())
			{
				point_iter->second.push_back(d);
			}
			else
			{
				point_map[LLVector3(d.getPosition().getF32ptr())].push_back(d);
			}
		}

		if (indices.size()%3 == 0 && verts.size() >= 65532)
		{
			std::string material;

			if (tri->getMaterial())
			{
				material = std::string(tri->getMaterial());
			}

			materials.push_back(material);
			face_list.push_back(face);
			face_list.rbegin()->fillFromLegacyData(verts, indices);
			LLVolumeFace& new_face = *face_list.rbegin();
			if (!norm_source)
			{
				//ll_aligned_free_16(new_face.mNormals);
				new_face.mNormals = NULL;
			}

			if (!tc_source)
			{
				//ll_aligned_free_16(new_face.mTexCoords);
				new_face.mTexCoords = NULL;
			}

			face = LLVolumeFace();
			point_map.clear();
		}
	}

	if (!verts.empty())
	{
		std::string material;

		if (tri->getMaterial())
		{
			material = std::string(tri->getMaterial());
		}
		
		materials.push_back(material);
		face_list.push_back(face);

		face_list.rbegin()->fillFromLegacyData(verts, indices);
		LLVolumeFace& new_face = *face_list.rbegin();
		if (!norm_source)
		{
			//ll_aligned_free_16(new_face.mNormals);
			new_face.mNormals = NULL;
		}

		if (!tc_source)
		{
			//ll_aligned_free_16(new_face.mTexCoords);
			new_face.mTexCoords = NULL;
		}
	}

	return LLModel::NO_ERRORS ;
}

LLModel::EModelStatus load_face_from_dom_polylist(std::vector<LLVolumeFace>& face_list, std::vector<std::string>& materials, domPolylistRef& poly)
{
	domPRef p = poly->getP();
	domListOfUInts& idx = p->getValue();

	if (idx.getCount() == 0)
	{
		return LLModel::NO_ERRORS ;
	}

	const domInputLocalOffset_Array& inputs = poly->getInput_array();


	domListOfUInts& vcount = poly->getVcount()->getValue();
	
	S32 pos_offset = -1;
	S32 tc_offset = -1;
	S32 norm_offset = -1;

	domSource* pos_source = NULL;
	domSource* tc_source = NULL;
	domSource* norm_source = NULL;

	S32 idx_stride = 0;

	if (!get_dom_sources(inputs, pos_offset, tc_offset, norm_offset, idx_stride, pos_source, tc_source, norm_source))
	{
		return LLModel::BAD_ELEMENT;
	}

	LLVolumeFace face;

	std::vector<U16> indices;
	std::vector<LLVolumeFace::VertexData> verts;

	domListOfFloats v;
	domListOfFloats tc;
	domListOfFloats n;

	if (pos_source)
	{
		v = pos_source->getFloat_array()->getValue();
		face.mExtents[0].set(v[0], v[1], v[2]);
		face.mExtents[1].set(v[0], v[1], v[2]);
	}

	if (tc_source)
	{
		tc = tc_source->getFloat_array()->getValue();
	}

	if (norm_source)
	{
		n = norm_source->getFloat_array()->getValue();
	}
	
	LLVolumeFace::VertexMapData::PointMap point_map;

	U32 cur_idx = 0;
	for (U32 i = 0; i < vcount.getCount(); ++i)
	{ //for each polygon
		U32 first_index = 0;
		U32 last_index = 0;
		for (U32 j = 0; j < vcount[i]; ++j)
		{ //for each vertex

			LLVolumeFace::VertexData cv;

			if (pos_source)
			{
				cv.getPosition().set(v[idx[cur_idx+pos_offset]*3+0],
									v[idx[cur_idx+pos_offset]*3+1],
									v[idx[cur_idx+pos_offset]*3+2]);
				if (!cv.getPosition().isFinite3())
				{
					LL_WARNS() << "Found NaN while loading position data from DAE-Model, invalid model." << LL_ENDL;
					return LLModel::BAD_ELEMENT;
				}
			}

			if (tc_source)
			{
				cv.mTexCoord.setVec(tc[idx[cur_idx+tc_offset]*2+0],
									tc[idx[cur_idx+tc_offset]*2+1]);
			}
			
			if (norm_source)
			{
				cv.getNormal().set(n[idx[cur_idx+norm_offset]*3+0],
									n[idx[cur_idx+norm_offset]*3+1],
									n[idx[cur_idx+norm_offset]*3+2]);

				if (!cv.getNormal().isFinite3())
				{
					LL_WARNS() << "Found NaN while loading normals from DAE-Model, invalid model." << LL_ENDL;
					return LLModel::BAD_ELEMENT;
				}
			}
			
			cur_idx += idx_stride;
			
			BOOL found = FALSE;
				
			LLVolumeFace::VertexMapData::PointMap::iterator point_iter;
			LLVector3 pos3(cv.getPosition().getF32ptr());
			point_iter = point_map.find(pos3);
			
			if (point_iter != point_map.end())
			{
				for (U32 k = 0; k < point_iter->second.size(); ++k)
				{
					if ((point_iter->second)[k] == cv)
					{
						found = TRUE;
						U32 index = (point_iter->second)[k].mIndex;
						if (j == 0)
						{
							first_index = index;
						}
						else if (j == 1)
						{
							last_index = index;
						}
						else
						{
							// if these are the same, we have a very, very skinny triangle (coincident verts on one or more edges)
							//
							llassert((first_index != last_index) && (last_index != index) && (first_index != index));
							indices.push_back(first_index);
							indices.push_back(last_index);
							indices.push_back(index);
							last_index = index;
						}

						break;
					}
				}
			}

			if (!found)
			{
				update_min_max(face.mExtents[0], face.mExtents[1], cv.getPosition());
				verts.push_back(cv);
				if (verts.size() >= 65535)
				{
					//llerrs << "Attempted to write model exceeding 16-bit index buffer limitation." << LL_ENDL;
					return LLModel::VERTEX_NUMBER_OVERFLOW ;
				}
				U16 index = (U16) (verts.size()-1);
			
				if (j == 0)
				{
					first_index = index;
				}
				else if (j == 1)
				{
					last_index = index;
				}
				else
				{
					// detect very skinny degenerate triangles with collapsed edges
					//
					llassert((first_index != last_index) && (last_index != index) && (first_index != index));
					indices.push_back(first_index);
					indices.push_back(last_index);
					indices.push_back(index);
					last_index = index;
				}	

				LLVolumeFace::VertexMapData d;
				d.setPosition(cv.getPosition());
				d.mTexCoord = cv.mTexCoord;
				d.setNormal(cv.getNormal());
				d.mIndex = index;
				if (point_iter != point_map.end())
				{
					point_iter->second.push_back(d);
				}
				else
				{
					point_map[pos3].push_back(d);
				}
			}

			if (indices.size()%3 == 0 && indices.size() >= 65532)
			{
				std::string material;

				if (poly->getMaterial())
				{
					material = std::string(poly->getMaterial());
				}

				materials.push_back(material);
				face_list.push_back(face);
				face_list.rbegin()->fillFromLegacyData(verts, indices);
				LLVolumeFace& new_face = *face_list.rbegin();
				if (!norm_source)
				{
					//ll_aligned_free_16(new_face.mNormals);
					new_face.mNormals = NULL;
				}

				if (!tc_source)
				{
					//ll_aligned_free_16(new_face.mTexCoords);
					new_face.mTexCoords = NULL;
				}

				face = LLVolumeFace();
				verts.clear();
				indices.clear();
				point_map.clear();
			}
		}
	}

	if (!verts.empty())
	{
		std::string material;

		if (poly->getMaterial())
		{
			material = std::string(poly->getMaterial());
		}
	
		materials.push_back(material);
		face_list.push_back(face);
		face_list.rbegin()->fillFromLegacyData(verts, indices);

		LLVolumeFace& new_face = *face_list.rbegin();
		if (!norm_source)
		{
			//ll_aligned_free_16(new_face.mNormals);
			new_face.mNormals = NULL;
		}

		if (!tc_source)
		{
			//ll_aligned_free_16(new_face.mTexCoords);
			new_face.mTexCoords = NULL;
		}
	}

	return LLModel::NO_ERRORS ;
}

LLModel::EModelStatus load_face_from_dom_polygons(std::vector<LLVolumeFace>& face_list, std::vector<std::string>& materials, domPolygonsRef& poly)
{
	LLVolumeFace face;
	std::vector<U16> indices;
	std::vector<LLVolumeFace::VertexData> verts;

	const domInputLocalOffset_Array& inputs = poly->getInput_array();

	S32 v_offset = -1;
	S32 n_offset = -1;
	S32 t_offset = -1;

	domListOfFloats* v = NULL;
	domListOfFloats* n = NULL;
	domListOfFloats* t = NULL;
	
	U32 stride = 0;
	for (U32 i = 0; i < inputs.getCount(); ++i)
	{
		stride = llmax((U32) inputs[i]->getOffset()+1, stride);

		if (strcmp(COMMON_PROFILE_INPUT_VERTEX, inputs[i]->getSemantic()) == 0)
		{ //found vertex array
			v_offset = inputs[i]->getOffset();

			const domURIFragmentType& uri = inputs[i]->getSource();
			daeElementRef elem = uri.getElement();
			domVertices* vertices = (domVertices*) elem.cast();
			if (!vertices)
			{
				return LLModel::BAD_ELEMENT;
			}
			domInputLocal_Array& v_inp = vertices->getInput_array();

			for (U32 k = 0; k < v_inp.getCount(); ++k)
			{
				if (strcmp(COMMON_PROFILE_INPUT_POSITION, v_inp[k]->getSemantic()) == 0)
				{
					const domURIFragmentType& uri = v_inp[k]->getSource();
					daeElementRef elem = uri.getElement();
					domSource* src = (domSource*) elem.cast();
					if (!src)
					{
						return LLModel::BAD_ELEMENT;
					}
					v = &(src->getFloat_array()->getValue());
				}
			}
		}
		else if (strcmp(COMMON_PROFILE_INPUT_NORMAL, inputs[i]->getSemantic()) == 0)
		{
			n_offset = inputs[i]->getOffset();
			//found normal array for this triangle list
			const domURIFragmentType& uri = inputs[i]->getSource();
			daeElementRef elem = uri.getElement();
			domSource* src = (domSource*) elem.cast();
			if (!src)
			{
				return LLModel::BAD_ELEMENT;
			}
			n = &(src->getFloat_array()->getValue());
		}
		else if (strcmp(COMMON_PROFILE_INPUT_TEXCOORD, inputs[i]->getSemantic()) == 0 && inputs[i]->getSet() == 0)
		{ //found texCoords
			t_offset = inputs[i]->getOffset();
			const domURIFragmentType& uri = inputs[i]->getSource();
			daeElementRef elem = uri.getElement();
			domSource* src = (domSource*) elem.cast();
			if (!src)
			{
				return LLModel::BAD_ELEMENT;
			}
			t = &(src->getFloat_array()->getValue());
		}
	}

	domP_Array& ps = poly->getP_array();

	//make a triangle list in <verts>
	for (U32 i = 0; i < ps.getCount(); ++i)
	{ //for each polygon
		domListOfUInts& idx = ps[i]->getValue();
		for (U32 j = 0; j < idx.getCount()/stride; ++j)
		{ //for each vertex
			if (j > 2)
			{
				U32 size = verts.size();
				LLVolumeFace::VertexData v0 = verts[size-3];
				LLVolumeFace::VertexData v1 = verts[size-1];

				verts.push_back(v0);
				verts.push_back(v1);
			}

			LLVolumeFace::VertexData vert;


			if (v)
			{
				U32 v_idx = idx[j*stride+v_offset]*3;
				v_idx = llclamp(v_idx, (U32) 0, (U32) v->getCount());
				vert.getPosition().set(v->get(v_idx),
								v->get(v_idx+1),
								v->get(v_idx+2));
			}
			
			//bounds check n and t lookups because some FBX to DAE converters
			//use negative indices and empty arrays to indicate data does not exist
			//for a particular channel
			if (n && n->getCount() > 0)
			{
				U32 n_idx = idx[j*stride+n_offset]*3;
				n_idx = llclamp(n_idx, (U32) 0, (U32) n->getCount());
				vert.getNormal().set(n->get(n_idx),
								n->get(n_idx+1),
								n->get(n_idx+2));
			}
			else
			{
				vert.getNormal().clear();
			}

			
			if (t && t->getCount() > 0)
			{
				U32 t_idx = idx[j*stride+t_offset]*2;
				t_idx = llclamp(t_idx, (U32) 0, (U32) t->getCount());
				vert.mTexCoord.setVec(t->get(t_idx),
								t->get(t_idx+1));								
			}
			else
			{
				vert.mTexCoord.clear();
			}

						
			verts.push_back(vert);
		}
	}

	if (verts.empty())
	{
		return LLModel::NO_ERRORS;
	}

	face.mExtents[0] = verts[0].getPosition();
	face.mExtents[1] = verts[0].getPosition();
	
	//create a map of unique vertices to indices
	std::map<LLVolumeFace::VertexData, U32> vert_idx;

	U32 cur_idx = 0;
	for (U32 i = 0; i < verts.size(); ++i)
	{
		std::map<LLVolumeFace::VertexData, U32>::iterator iter = vert_idx.find(verts[i]);
		if (iter == vert_idx.end())
		{
			vert_idx[verts[i]] = cur_idx++;
		}
	}

	//build vertex array from map
	std::vector<LLVolumeFace::VertexData> new_verts;
	new_verts.resize(vert_idx.size());

	for (std::map<LLVolumeFace::VertexData, U32>::iterator iter = vert_idx.begin(); iter != vert_idx.end(); ++iter)
	{
		new_verts[iter->second] = iter->first;
		update_min_max(face.mExtents[0], face.mExtents[1], iter->first.getPosition());
	}

	//build index array from map
	indices.resize(verts.size());

	for (U32 i = 0; i < verts.size(); ++i)
	{
		indices[i] = vert_idx[verts[i]];
		llassert(!i || (indices[i-1] != indices[i]));
	}

	// DEBUG just build an expanded triangle list
	/*for (U32 i = 0; i < verts.size(); ++i)
	{
		indices.push_back((U16) i);
		update_min_max(face.mExtents[0], face.mExtents[1], verts[i].getPosition());
	}*/

    if (!new_verts.empty())
	{
		std::string material;

		if (poly->getMaterial())
		{
			material = std::string(poly->getMaterial());
		}

		materials.push_back(material);
		face_list.push_back(face);
		face_list.rbegin()->fillFromLegacyData(new_verts, indices);

		LLVolumeFace& new_face = *face_list.rbegin();
		if (!n)
		{
			//ll_aligned_free_16(new_face.mNormals);
			new_face.mNormals = NULL;
		}

		if (!t)
		{
			//ll_aligned_free_16(new_face.mTexCoords);
			new_face.mTexCoords = NULL;
		}
	}

	return LLModel::NO_ERRORS ;
}

//-----------------------------------------------------------------------------
// LLDAELoader
//-----------------------------------------------------------------------------
LLDAELoader::LLDAELoader(
	std::string				filename,
	S32						lod,
	load_callback_t		load_cb,
	joint_lookup_func_t	joint_lookup_func,
	texture_load_func_t	texture_load_func,
	state_callback_t		state_cb,
	void*						opaque_userdata,
	JointTransformMap&	jointMap,
	JointSet&				jointsFromNodes,
	U32					modelLimit,
	bool					preprocess)
: LLModelLoader(
		filename,
		lod,
		load_cb,
		joint_lookup_func,
		texture_load_func,
		state_cb,
		opaque_userdata,
		jointMap,
		jointsFromNodes),
mGeneratedModelLimit(modelLimit),
mPreprocessDAE(preprocess)
{
}

LLDAELoader::~LLDAELoader()
{
}

struct ModelSort
{
	bool operator()(const LLPointer< LLModel >& lhs, const LLPointer< LLModel >& rhs)
	{
        if (lhs->mSubmodelID < rhs->mSubmodelID)
        {
            return true;
        }
		return LLStringUtil::compareInsensitive(lhs->mLabel, rhs->mLabel) < 0;
	}
};

bool LLDAELoader::OpenFile(const std::string& filename)
{
	//no suitable slm exists, load from the .dae file
	DAE dae;
	domCOLLADA* dom;
	if (mPreprocessDAE)
	{
		dom = dae.openFromMemory(filename, preprocessDAE(filename).c_str());
	}
	else
	{
		LL_INFOS() << "Skipping dae preprocessing" << LL_ENDL;
		dom = dae.open(filename);
	}
	
	if (!dom)
	{
		LL_INFOS() <<" Error with dae - traditionally indicates a corrupt file."<<LL_ENDL;
		setLoadState( ERROR_PARSING );
		return false;
	}
	//Dom version
	daeString domVersion = dae.getDomVersion();
	std::string sldom(domVersion);
	LL_INFOS()<<"Collada Importer Version: "<<sldom<<LL_ENDL;
	//Dae version
	domVersionType docVersion = dom->getVersion();
	//0=1.4
	//1=1.4.1
	//2=Currently unsupported, however may work
	if (docVersion > 1 ) 
	{ 
		docVersion = VERSIONTYPE_COUNT;
	}
	LL_INFOS()<<"Dae version "<<colladaVersion[docVersion]<<LL_ENDL;
	
	
	daeDatabase* db = dae.getDatabase();
	
	daeInt count = db->getElementCount(NULL, COLLADA_TYPE_MESH);
	
	daeDocument* doc = dae.getDoc(filename);
	if (!doc)
	{
		LL_WARNS() << "can't find internal doc" << LL_ENDL;
		return false;
	}
	
	daeElement* root = doc->getDomRoot();
	if (!root)
	{
		LL_WARNS() << "document has no root" << LL_ENDL;
		return false;
	}
	
	//Verify some basic properties of the dae
	//1. Basic validity check on controller 
	U32 controllerCount = (int) db->getElementCount( NULL, "controller" );
	bool result = false;
	for ( int i=0; i<controllerCount; ++i )
	{
		domController* pController = NULL;
		db->getElement( (daeElement**) &pController, i , NULL, "controller" );
		result = verifyController( pController );
		if (!result)
		{
			LL_INFOS() << "Could not verify controller" << LL_ENDL;
			setLoadState( ERROR_PARSING );
			return true;
		}
	}


	//get unit scale
	mTransform.setIdentity();
	
	domAsset::domUnit* unit = daeSafeCast<domAsset::domUnit>(root->getDescendant(daeElement::matchType(domAsset::domUnit::ID())));

	if (unit)
	{
		F32 meter = unit->getMeter();
		mTransform.mMatrix[0][0] = meter;
		mTransform.mMatrix[1][1] = meter;
		mTransform.mMatrix[2][2] = meter;
	}
	
	//get up axis rotation
	LLMatrix4 rotation;
	
	domUpAxisType up = UPAXISTYPE_Y_UP;  // default is Y_UP
	domAsset::domUp_axis* up_axis =
	daeSafeCast<domAsset::domUp_axis>(root->getDescendant(daeElement::matchType(domAsset::domUp_axis::ID())));
	
	if (up_axis)
	{
		up = up_axis->getValue();
	}
	
	if (up == UPAXISTYPE_X_UP)
	{
		rotation.initRotation(0.0f, 90.0f * DEG_TO_RAD, 0.0f);
	}
	else if (up == UPAXISTYPE_Y_UP)
	{
		rotation.initRotation(90.0f * DEG_TO_RAD, 0.0f, 0.0f);
	}
	
	rotation *= mTransform;
	mTransform = rotation;

	mTransform.condition();	

	U32 submodel_limit = count > 0 ? mGeneratedModelLimit/count : 0;
	for (daeInt idx = 0; idx < count; ++idx)
	{ //build map of domEntities to LLModel
		domMesh* mesh = NULL;
		db->getElement((daeElement**) &mesh, idx, NULL, COLLADA_TYPE_MESH);
		
		if (mesh)
		{

			std::vector<LLModel*> models;

			loadModelsFromDomMesh(mesh, models, submodel_limit);

			std::vector<LLModel*>::iterator i;
			i = models.begin();
			while (i != models.end())
			{
				LLModel* mdl = *i;
				if(mdl->getStatus() != LLModel::NO_ERRORS)
				{
					setLoadState(ERROR_MODEL + mdl->getStatus()) ;
					return false; //abort
				}

				if (mdl && validate_model(mdl))
				{
					mModelList.push_back(mdl);
					mModelsMap[mesh].push_back(mdl);
				}
				i++;
			}
		}
	}

	std::sort(mModelList.begin(), mModelList.end(), ModelSort());

	model_list::iterator model_iter = mModelList.begin();
	while (model_iter != mModelList.end())
	{
		LLModel* mdl = *model_iter;
		U32 material_count = mdl->mMaterialList.size();
		LL_INFOS() << "Importing " << mdl->mLabel << " model with " << material_count << " material references" << LL_ENDL;
		std::vector<std::string>::iterator mat_iter = mdl->mMaterialList.begin();
		std::vector<std::string>::iterator end_iter = material_count > LIMIT_MATERIALS_OUTPUT
														? mat_iter + LIMIT_MATERIALS_OUTPUT
														: mdl->mMaterialList.end();
		while (mat_iter != end_iter)
		{
			LL_INFOS() << mdl->mLabel << " references " << (*mat_iter) << LL_ENDL;
			mat_iter++;
		}
		model_iter++;
	}

	count = db->getElementCount(NULL, COLLADA_TYPE_SKIN);
	for (daeInt idx = 0; idx < count; ++idx)
	{ //add skinned meshes as instances
		domSkin* skin = NULL;
		db->getElement((daeElement**) &skin, idx, NULL, COLLADA_TYPE_SKIN);
		
		if (skin)
		{
			domGeometry* geom = daeSafeCast<domGeometry>(skin->getSource().getElement());
			
			if (geom)
			{
				domMesh* mesh = geom->getMesh();
				if (mesh)
				{
					std::vector< LLPointer< LLModel > >::iterator i = mModelsMap[mesh].begin();
					while (i != mModelsMap[mesh].end())
					{
						LLPointer<LLModel> mdl = *i;
						LLDAELoader::processDomModel(mdl, &dae, root, mesh, skin);
						i++;
					}
				}
			}
		}
	}

	LL_INFOS()<< "Collada skins processed: " << count <<LL_ENDL;

	daeElement* scene = root->getDescendant("visual_scene");
	
	if (!scene)
	{
		LL_WARNS() << "document has no visual_scene" << LL_ENDL;
		setLoadState( ERROR_PARSING );
		return true;
	}
	
	setLoadState( DONE );

	bool badElement = false;
	
	processElement( scene, badElement, &dae );
	
	if ( badElement )
	{
		LL_INFOS()<<"Scene could not be parsed"<<LL_ENDL;
		setLoadState( ERROR_PARSING );
	}
	
	return true;
}

std::string LLDAELoader::preprocessDAE(std::string filename)
{
	// Open a DAE file for some preprocessing (like removing space characters in IDs), see MAINT-5678
	llifstream inFile;
	inFile.open(filename.c_str(), std::ios_base::in);
	std::stringstream strStream;
	strStream << inFile.rdbuf();
	std::string buffer = strStream.str();

	LL_INFOS() << "Preprocessing dae file to remove spaces from the names, ids, etc." << LL_ENDL;

	try
	{
		boost::regex re("\"[\\w\\.@#$-]*(\\s[\\w\\.@#$-]*)+\"");
		boost::sregex_iterator next(buffer.begin(), buffer.end(), re);
		boost::sregex_iterator end;
		while (next != end)
		{
			boost::smatch match = *next;
			std::string s = match.str();
			LL_INFOS() << s << " found" << LL_ENDL;
			boost::replace_all(s, " ", "_");
			LL_INFOS() << "Replacing with " << s << LL_ENDL;
			boost::replace_all(buffer, match.str(), s);
			next++;
		}
	}
	catch (boost::regex_error &)
	{
		LL_INFOS() << "Regex error" << LL_ENDL;
	}

	return buffer;
}

void LLDAELoader::processDomModel(LLModel* model, DAE* dae, daeElement* root, domMesh* mesh, domSkin* skin)
{
	llassert(model && dae && mesh && skin);

	if (model)
	{
		LLVector3 mesh_scale_vector;
		LLVector3 mesh_translation_vector;
		model->getNormalizedScaleTranslation(mesh_scale_vector, mesh_translation_vector);

		LLMatrix4 normalized_transformation;
		normalized_transformation.setTranslation(mesh_translation_vector);

		LLMatrix4 mesh_scale;
		mesh_scale.initScale(mesh_scale_vector);
		mesh_scale *= normalized_transformation;
		normalized_transformation = mesh_scale;

		glh::matrix4f inv_mat((F32*) normalized_transformation.mMatrix);
		inv_mat = inv_mat.inverse();
		LLMatrix4 inverse_normalized_transformation(inv_mat.m);

		domSkin::domBind_shape_matrix* bind_mat = skin->getBind_shape_matrix();

		if (bind_mat)
		{ //get bind shape matrix
			domFloat4x4& dom_value = bind_mat->getValue();

			LLMeshSkinInfo& skin_info = model->mSkinInfo;

			for (int i = 0; i < 4; i++)
			{
				for(int j = 0; j < 4; j++)
				{
					skin_info.mBindShapeMatrix.mMatrix[i][j] = dom_value[i + j*4];
				}
			}

			LLMatrix4 trans = normalized_transformation;
			trans *= skin_info.mBindShapeMatrix;
			skin_info.mBindShapeMatrix = trans;							
		}


		//Some collada setup for accessing the skeleton
		daeElement* pElement = 0;
		dae->getDatabase()->getElement( &pElement, 0, 0, "skeleton" );

		//Try to get at the skeletal instance controller
		domInstance_controller::domSkeleton* pSkeleton = daeSafeCast<domInstance_controller::domSkeleton>( pElement );
		bool missingSkeletonOrScene = false;

		//If no skeleton, do a breadth-first search to get at specific joints
		bool rootNode = false;

		//Need to test for a skeleton that does not have a root node
		//This occurs when your instance controller does not have an associated scene 
		if ( pSkeleton )
		{
			daeElement* pSkeletonRootNode = pSkeleton->getValue().getElement();
			if ( pSkeletonRootNode )
			{
				rootNode = true;
			}

		}
		if ( !pSkeleton || !rootNode )
		{
			daeElement* pScene = root->getDescendant("visual_scene");
			if ( !pScene )
			{
				LL_WARNS()<<"No visual scene - unable to parse bone offsets "<<LL_ENDL;
				missingSkeletonOrScene = true;
			}
			else
			{
				//Get the children at this level
				daeTArray< daeSmartRef<daeElement> > children = pScene->getChildren();
				S32 childCount = children.getCount();

				//Process any children that are joints
				//Not all children are joints, some code be ambient lights, cameras, geometry etc..
				for (S32 i = 0; i < childCount; ++i)
				{
					domNode* pNode = daeSafeCast<domNode>(children[i]);
					if ( isNodeAJoint( pNode ) )
					{
						processJointNode( pNode, mJointList );
					}
				}
			}
		}
		else
			//Has Skeleton
		{
			//Get the root node of the skeleton
			daeElement* pSkeletonRootNode = pSkeleton->getValue().getElement();
			if ( pSkeletonRootNode )
			{
				//Once we have the root node - start acccessing it's joint components
				const int jointCnt = mJointMap.size();
				JointMap :: const_iterator jointIt = mJointMap.begin();

				//Loop over all the possible joints within the .dae - using the allowed joint list in the ctor.
				for ( int i=0; i<jointCnt; ++i, ++jointIt )
				{
					//Build a joint for the resolver to work with
					char str[64]={0};
					sprintf(str,"./%s",(*jointIt).first.c_str() );
					//LL_WARNS()<<"Joint "<< str <<LL_ENDL;

					//Setup the resolver
					daeSIDResolver resolver( pSkeletonRootNode, str );

					//Look for the joint
					domNode* pJoint = daeSafeCast<domNode>( resolver.getElement() );
					if ( pJoint )
					{
						//Pull out the translate id and store it in the jointTranslations map
						daeSIDResolver jointResolverA( pJoint, "./translate" );
						domTranslate* pTranslateA = daeSafeCast<domTranslate>( jointResolverA.getElement() );
						daeSIDResolver jointResolverB( pJoint, "./location" );
						domTranslate* pTranslateB = daeSafeCast<domTranslate>( jointResolverB.getElement() );

						LLMatrix4 workingTransform;

						//Translation via SID
						if ( pTranslateA )
						{
							extractTranslation( pTranslateA, workingTransform );
						}
						else
							if ( pTranslateB )
							{
								extractTranslation( pTranslateB, workingTransform );
							}
							else
							{
								//Translation via child from element
								daeElement* pTranslateElement = getChildFromElement( pJoint, "translate" );
								if ( pTranslateElement && pTranslateElement->typeID() != domTranslate::ID() )
								{
									LL_WARNS()<< "The found element is not a translate node" <<LL_ENDL;
									missingSkeletonOrScene = true;
								}
								else
									if ( pTranslateElement )
									{
										extractTranslationViaElement( pTranslateElement, workingTransform );
									}
									else
									{
										extractTranslationViaSID( pJoint, workingTransform );
									}

							}

							//Store the joint transform w/respect to it's name.
							mJointList[(*jointIt).second.c_str()] = workingTransform;
					}
				}

				//If anything failed in regards to extracting the skeleton, joints or translation id,
				//mention it
				if ( missingSkeletonOrScene  )
				{
					LL_WARNS()<< "Partial jointmap found in asset - did you mean to just have a partial map?" << LL_ENDL;
				}
			}//got skeleton?
		}


		domSkin::domJoints* joints = skin->getJoints();

		domInputLocal_Array& joint_input = joints->getInput_array();

		for (size_t i = 0; i < joint_input.getCount(); ++i)
		{
			domInputLocal* input = joint_input.get(i);
			xsNMTOKEN semantic = input->getSemantic();

			if (strcmp(semantic, COMMON_PROFILE_INPUT_JOINT) == 0)
			{ //found joint source, fill model->mJointMap and model->mSkinInfo.mJointNames
				daeElement* elem = input->getSource().getElement();

				domSource* source = daeSafeCast<domSource>(elem);
				if (source)
				{


					domName_array* names_source = source->getName_array();

					if (names_source)
					{
						domListOfNames &names = names_source->getValue();

						for (size_t j = 0; j < names.getCount(); ++j)
						{
							std::string name(names.get(j));
							if (mJointMap.find(name) != mJointMap.end())
							{
								name = mJointMap[name];
							}
							model->mSkinInfo.mJointNames.push_back(name);
							model->mSkinInfo.mJointMap[name] = j;
						}
					}
					else
					{
						domIDREF_array* names_source = source->getIDREF_array();
						if (names_source)
						{
							xsIDREFS& names = names_source->getValue();

							for (size_t j = 0; j < names.getCount(); ++j)
							{
								std::string name(names.get(j).getID());
								if (mJointMap.find(name) != mJointMap.end())
								{
									name = mJointMap[name];
								}
								model->mSkinInfo.mJointNames.push_back(name);
								model->mSkinInfo.mJointMap[name] = j;
							}
						}
					}
				}
			}
			else if (strcmp(semantic, COMMON_PROFILE_INPUT_INV_BIND_MATRIX) == 0)
			{ //found inv_bind_matrix array, fill model->mInvBindMatrix
				domSource* source = daeSafeCast<domSource>(input->getSource().getElement());
				if (source)
				{
					domFloat_array* t = source->getFloat_array();
					if (t)
					{
						domListOfFloats& transform = t->getValue();
						S32 count = transform.getCount()/16;

						for (S32 k = 0; k < count; ++k)
						{
							LLMatrix4 mat;

							for (int i = 0; i < 4; i++)
							{
								for(int j = 0; j < 4; j++)
								{
									mat.mMatrix[i][j] = transform[k*16 + i + j*4];
								}
							}

							model->mSkinInfo.mInvBindMatrix.push_back(mat);											
						}
					}
				}
			}
		}

		//Now that we've parsed the joint array, let's determine if we have a full rig
		//(which means we have all the joint sthat are required for an avatar versus
		//a skinned asset attached to a node in a file that contains an entire skeleton,
		//but does not use the skeleton).						
		buildJointToNodeMappingFromScene( root );
		critiqueRigForUploadApplicability( model->mSkinInfo.mJointNames );

		if ( !missingSkeletonOrScene )
		{
			//Set the joint translations on the avatar - if it's a full mapping
			//The joints are reset in the dtor
			if ( getRigWithSceneParity() )
			{	
				JointMap :: const_iterator masterJointIt = mJointMap.begin();
				JointMap :: const_iterator masterJointItEnd = mJointMap.end();
				for (;masterJointIt!=masterJointItEnd;++masterJointIt )
				{
					std::string lookingForJoint = (*masterJointIt).first.c_str();

					if ( mJointList.find( lookingForJoint ) != mJointList.end() )
					{
						//LL_INFOS()<<"joint "<<lookingForJoint.c_str()<<LL_ENDL;
						LLMatrix4 jointTransform = mJointList[lookingForJoint];
						LLJoint* pJoint = mJointLookupFunc(lookingForJoint,mOpaqueData);
						if ( pJoint )
						{   
							LLUUID fake_mesh_id;
							fake_mesh_id.generate();
							pJoint->addAttachmentPosOverride( jointTransform.getTranslation(), fake_mesh_id, "");
						}
						else
						{
							//Most likely an error in the asset.
							LL_WARNS()<<"Tried to apply joint position from .dae, but it did not exist in the avatar rig." << LL_ENDL;
						}
					}
				}
			}
		} //missingSkeletonOrScene

		//We need to construct the alternate bind matrix (which contains the new joint positions)
		//in the same order as they were stored in the joint buffer. The joints associated
		//with the skeleton are not stored in the same order as they are in the exported joint buffer.
		//This remaps the skeletal joints to be in the same order as the joints stored in the model.
		std::vector<std::string> :: const_iterator jointIt  = model->mSkinInfo.mJointNames.begin();
		const int jointCnt = model->mSkinInfo.mJointNames.size();
		for ( int i=0; i<jointCnt; ++i, ++jointIt )
		{
			std::string lookingForJoint = (*jointIt).c_str();
			//Look for the joint xform that we extracted from the skeleton, using the jointIt as the key
			//and store it in the alternate bind matrix
			if ( mJointList.find( lookingForJoint ) != mJointList.end() )
			{
				LLMatrix4 jointTransform = mJointList[lookingForJoint];
				LLMatrix4 newInverse = model->mSkinInfo.mInvBindMatrix[i];
				newInverse.setTranslation( mJointList[lookingForJoint].getTranslation() );
				model->mSkinInfo.mAlternateBindMatrix.push_back( newInverse );
			}
			else
			{
				LL_WARNS()<<"Possibly misnamed/missing joint [" <<lookingForJoint.c_str()<<" ] "<<LL_ENDL;
			}
		}

		//grab raw position array

		domVertices* verts = mesh->getVertices();
		if (verts)
		{
			domInputLocal_Array& inputs = verts->getInput_array();
			for (size_t i = 0; i < inputs.getCount() && model->mPosition.empty(); ++i)
			{
				if (strcmp(inputs[i]->getSemantic(), COMMON_PROFILE_INPUT_POSITION) == 0)
				{
					domSource* pos_source = daeSafeCast<domSource>(inputs[i]->getSource().getElement());
					if (pos_source)
					{
						domFloat_array* pos_array = pos_source->getFloat_array();
						if (pos_array)
						{
							domListOfFloats& pos = pos_array->getValue();

							for (size_t j = 0; j < pos.getCount(); j += 3)
							{
								if (pos.getCount() <= j+2)
								{
									LL_ERRS() << "Invalid position array size." << LL_ENDL;
								}

								LLVector3 v(pos[j], pos[j+1], pos[j+2]);

								//transform from COLLADA space to volume space
								v = v * inverse_normalized_transformation;

								model->mPosition.push_back(v);
							}
						}
					}
				}
			}
		}

		//grab skin weights array
		domSkin::domVertex_weights* weights = skin->getVertex_weights();
		if (weights)
		{
			domInputLocalOffset_Array& inputs = weights->getInput_array();
			domFloat_array* vertex_weights = NULL;
			for (size_t i = 0; i < inputs.getCount(); ++i)
			{
				if (strcmp(inputs[i]->getSemantic(), COMMON_PROFILE_INPUT_WEIGHT) == 0)
				{
					domSource* weight_source = daeSafeCast<domSource>(inputs[i]->getSource().getElement());
					if (weight_source)
					{
						vertex_weights = weight_source->getFloat_array();
					}
				}
			}

			if (vertex_weights)
			{
				domListOfFloats& w = vertex_weights->getValue();
				domListOfUInts& vcount = weights->getVcount()->getValue();
				domListOfInts& v = weights->getV()->getValue();

				U32 c_idx = 0;
				for (size_t vc_idx = 0; vc_idx < vcount.getCount(); ++vc_idx)
				{ //for each vertex
					daeUInt count = vcount[vc_idx];

					//create list of weights that influence this vertex
					LLModel::weight_list weight_list;

					for (daeUInt i = 0; i < count; ++i)
					{ //for each weight
						daeInt joint_idx = v[c_idx++];
						daeInt weight_idx = v[c_idx++];

						if (joint_idx == -1)
						{
							//ignore bindings to bind_shape_matrix
							continue;
						}

						F32 weight_value = w[weight_idx];

						weight_list.push_back(LLModel::JointWeight(joint_idx, weight_value));
					}

					//sort by joint weight
					std::sort(weight_list.begin(), weight_list.end(), LLModel::CompareWeightGreater());

					std::vector<LLModel::JointWeight> wght;

					F32 total = 0.f;

					for (U32 i = 0; i < llmin((U32) 4, (U32) weight_list.size()); ++i)
					{ //take up to 4 most significant weights
						if (weight_list[i].mWeight > 0.f)
						{
							wght.push_back( weight_list[i] );
							total += weight_list[i].mWeight;
						}
					}

					F32 scale = 1.f/total;
					if (scale != 1.f)
					{ //normalize weights
						for (U32 i = 0; i < wght.size(); ++i)
						{
							wght[i].mWeight *= scale;
						}
					}

					model->mSkinWeights[model->mPosition[vc_idx]] = wght;
				}
			}

		}

		//add instance to scene for this model

		LLMatrix4 transformation;
		transformation.initScale(mesh_scale_vector);
		transformation.setTranslation(mesh_translation_vector);
		transformation *= mTransform;

		std::map<std::string, LLImportMaterial> materials;
		for (U32 i = 0; i < model->mMaterialList.size(); ++i)
		{
			materials[model->mMaterialList[i]] = LLImportMaterial();
		}
		mScene[transformation].push_back(LLModelInstance(model, model->mLabel, transformation, materials));
		stretch_extents(model, transformation, mExtents[0], mExtents[1], mFirstTransform);
	}
}

//-----------------------------------------------------------------------------
// buildJointToNodeMappingFromScene()
//-----------------------------------------------------------------------------
void LLDAELoader::buildJointToNodeMappingFromScene( daeElement* pRoot )
{
	daeElement* pScene = pRoot->getDescendant("visual_scene");
	if ( pScene )
	{
		daeTArray< daeSmartRef<daeElement> > children = pScene->getChildren();
		S32 childCount = children.getCount();
		for (S32 i = 0; i < childCount; ++i)
		{
			domNode* pNode = daeSafeCast<domNode>(children[i]);
			processJointToNodeMapping( pNode );			
		}
	}
}
//-----------------------------------------------------------------------------
// processJointToNodeMapping()
//-----------------------------------------------------------------------------
void LLDAELoader::processJointToNodeMapping( domNode* pNode )
{
	if ( isNodeAJoint( pNode ) )
	{
		//1.Store the parent
		std::string nodeName = pNode->getName();
		if ( !nodeName.empty() )
		{
			mJointsFromNode.push_front( pNode->getName() );
		}
		//2. Handle the kiddo's
		processChildJoints( pNode );
	}
	else
	{
		//Determine if the're any children wrt to this failed node.
		//This occurs when an armature is exported and ends up being what essentially amounts to
		//as the root for the visual_scene
		if ( pNode ) 
		{
			processChildJoints( pNode );
		}
		else 
		{
			LL_INFOS()<<"Node is NULL"<<LL_ENDL;
		}

	}
}
//-----------------------------------------------------------------------------
// processChildJoint()
//-----------------------------------------------------------------------------
void LLDAELoader::processChildJoints( domNode* pParentNode )
{	
	daeTArray< daeSmartRef<daeElement> > childOfChild = pParentNode->getChildren();
	S32 childOfChildCount = childOfChild.getCount();
	for (S32 i = 0; i < childOfChildCount; ++i)
	{
		domNode* pChildNode = daeSafeCast<domNode>( childOfChild[i] );
		if ( pChildNode )
		{
			processJointToNodeMapping( pChildNode );
		}
	}
}

//-----------------------------------------------------------------------------
// isNodeAJoint()
//-----------------------------------------------------------------------------
bool LLDAELoader::isNodeAJoint( domNode* pNode )
{
    if ( !pNode || !pNode->getName() )
	{
		LL_INFOS()<<"Created node is NULL or invalid"<<LL_ENDL;
		return false;
	}
	
	return LLModelLoader::isNodeAJoint(pNode->getName());
}
//-----------------------------------------------------------------------------
// verifyCount
//-----------------------------------------------------------------------------
bool LLDAELoader::verifyCount( int expected, int result )
{
	if ( expected != result )
	{
		LL_INFOS()<< "Error: (expected/got)"<<expected<<"/"<<result<<"verts"<<LL_ENDL;
		return false;
	}
	return true;
}
//-----------------------------------------------------------------------------
// verifyController
//-----------------------------------------------------------------------------
bool LLDAELoader::verifyController( domController* pController )
{	

	bool result = true;

	domSkin* pSkin = pController->getSkin();

	if ( pSkin )
	{
		xsAnyURI & uri = pSkin->getSource();
		domElement* pElement = uri.getElement();

		if ( !pElement )
		{
			LL_INFOS()<<"Can't resolve skin source"<<LL_ENDL;
			return false;
		}

		daeString type_str = pElement->getTypeName();
		if ( stricmp(type_str, "geometry") == 0 )
		{	
			//Skin is reference directly by geometry and get the vertex count from skin
			domSkin::domVertex_weights* pVertexWeights = pSkin->getVertex_weights();
			U32 vertexWeightsCount = pVertexWeights->getCount();
			domGeometry* pGeometry = (domGeometry*) (domElement*) uri.getElement();
			domMesh* pMesh = pGeometry->getMesh();				
			
			if ( pMesh )
			{
				//Get vertex count from geometry
				domVertices* pVertices = pMesh->getVertices();
				if ( !pVertices )
				{ 
					LL_INFOS()<<"No vertices!"<<LL_ENDL;
					return false;
				}

				if ( pVertices )
				{
					xsAnyURI src = pVertices->getInput_array()[0]->getSource();
					domSource* pSource = (domSource*) (domElement*) src.getElement();
					U32 verticesCount = pSource->getTechnique_common()->getAccessor()->getCount();
					result = verifyCount( verticesCount, vertexWeightsCount );
					if ( !result )
					{
						return result;
					}
				}
			}	

			U32 vcountCount = (U32) pVertexWeights->getVcount()->getValue().getCount();
			result = verifyCount( vcountCount, vertexWeightsCount );	
			if ( !result )
			{
				return result;
			}

			domInputLocalOffset_Array& inputs = pVertexWeights->getInput_array();
			U32 sum = 0;
			for (size_t i=0; i<vcountCount; i++)
			{
				sum += pVertexWeights->getVcount()->getValue()[i];
			}
			result = verifyCount( sum * inputs.getCount(), (domInt) pVertexWeights->getV()->getValue().getCount() );
		}
	}
	
	return result;
}

//-----------------------------------------------------------------------------
// extractTranslation()
//-----------------------------------------------------------------------------
void LLDAELoader::extractTranslation( domTranslate* pTranslate, LLMatrix4& transform )
{
	domFloat3 jointTrans = pTranslate->getValue();
	LLVector3 singleJointTranslation( jointTrans[0], jointTrans[1], jointTrans[2] );
	transform.setTranslation( singleJointTranslation );
}
//-----------------------------------------------------------------------------
// extractTranslationViaElement()
//-----------------------------------------------------------------------------
void LLDAELoader::extractTranslationViaElement( daeElement* pTranslateElement, LLMatrix4& transform )
{
	if ( pTranslateElement )
	{
		domTranslate* pTranslateChild = dynamic_cast<domTranslate*>( pTranslateElement );
		domFloat3 translateChild = pTranslateChild->getValue();
		LLVector3 singleJointTranslation( translateChild[0], translateChild[1], translateChild[2] );
		transform.setTranslation( singleJointTranslation );
	}	
}
//-----------------------------------------------------------------------------
// extractTranslationViaSID()
//-----------------------------------------------------------------------------
void LLDAELoader::extractTranslationViaSID( daeElement* pElement, LLMatrix4& transform )
{
	if ( pElement )
	{	
		daeSIDResolver resolver( pElement, "./transform" );
		domMatrix* pMatrix = daeSafeCast<domMatrix>( resolver.getElement() );
		//We are only extracting out the translational component atm
		LLMatrix4 workingTransform;
		if ( pMatrix )
		{
			domFloat4x4 domArray = pMatrix->getValue();									
			for ( int i = 0; i < 4; i++ )
			{
				for( int j = 0; j < 4; j++ )
				{
					workingTransform.mMatrix[i][j] = domArray[i + j*4];
				}
			}
			LLVector3 trans = workingTransform.getTranslation();
			transform.setTranslation( trans );	
		}
	}
	else
	{
		LL_WARNS()<<"Element is nonexistent - empty/unsupported node."<<LL_ENDL;
	}
}
//-----------------------------------------------------------------------------
// processJointNode()
//-----------------------------------------------------------------------------
void LLDAELoader::processJointNode( domNode* pNode, JointTransformMap& jointTransforms )
{
	if (pNode->getName() == NULL)
	{
		LL_WARNS() << "nameless node, can't process" << LL_ENDL;
		return;
	}

	//LL_WARNS()<<"ProcessJointNode# Node:" <<pNode->getName()<<LL_ENDL;

	//1. handle the incoming node - extract out translation via SID or element

	LLMatrix4 workingTransform;

	//Pull out the translate id and store it in the jointTranslations map
	daeSIDResolver jointResolverA( pNode, "./translate" );
	domTranslate* pTranslateA = daeSafeCast<domTranslate>( jointResolverA.getElement() );
	daeSIDResolver jointResolverB( pNode, "./location" );
	domTranslate* pTranslateB = daeSafeCast<domTranslate>( jointResolverB.getElement() );

	//Translation via SID was successful
	if ( pTranslateA )
	{
		extractTranslation( pTranslateA, workingTransform );
	}
	else
	if ( pTranslateB )
	{
		extractTranslation( pTranslateB, workingTransform );
	}
	else
	{
		//Translation via child from element
		daeElement* pTranslateElement = getChildFromElement( pNode, "translate" );
		if ( !pTranslateElement || pTranslateElement->typeID() != domTranslate::ID() )
		{
			//LL_WARNS()<< "The found element is not a translate node" <<LL_ENDL;
			daeSIDResolver jointResolver( pNode, "./matrix" );
			domMatrix* pMatrix = daeSafeCast<domMatrix>( jointResolver.getElement() );
			if ( pMatrix )
			{
				//LL_INFOS()<<"A matrix SID was however found!"<<LL_ENDL;
				domFloat4x4 domArray = pMatrix->getValue();									
				for ( int i = 0; i < 4; i++ )
				{
					for( int j = 0; j < 4; j++ )
					{
						workingTransform.mMatrix[i][j] = domArray[i + j*4];
					}
				}
			}
			else
			{
				LL_WARNS()<< "The found element is not translate or matrix node - most likely a corrupt export!" <<LL_ENDL;
			}
		}
		else
		{
			extractTranslationViaElement( pTranslateElement, workingTransform );
		}
	}

	//Store the working transform relative to the nodes name.
	jointTransforms[ pNode->getName() ] = workingTransform;

	//2. handle the nodes children

	//Gather and handle the incoming nodes children
	daeTArray< daeSmartRef<daeElement> > childOfChild = pNode->getChildren();
	S32 childOfChildCount = childOfChild.getCount();

	for (S32 i = 0; i < childOfChildCount; ++i)
	{
		domNode* pChildNode = daeSafeCast<domNode>( childOfChild[i] );
		if ( pChildNode )
		{
			processJointNode( pChildNode, jointTransforms );
		}
	}
}
//-----------------------------------------------------------------------------
// getChildFromElement()
//-----------------------------------------------------------------------------
daeElement* LLDAELoader::getChildFromElement( daeElement* pElement, std::string const & name )
{
    daeElement* pChildOfElement = pElement->getChild( name.c_str() );
	if ( pChildOfElement )
	{
		return pChildOfElement;
	}
	LL_WARNS()<< "Could not find a child [" << name << "] for the element: \"" << pElement->getAttribute("id") << "\"" << LL_ENDL;
    return NULL;
}

void LLDAELoader::processElement( daeElement* element, bool& badElement, DAE* dae )
{
	LLMatrix4 saved_transform;
	bool pushed_mat = false;

	domNode* node = daeSafeCast<domNode>(element);
	if (node)
	{
		pushed_mat = true;
		saved_transform = mTransform;
	}

	domTranslate* translate = daeSafeCast<domTranslate>(element);
	if (translate)
	{
		domFloat3 dom_value = translate->getValue();

		LLMatrix4 translation;
		translation.setTranslation(LLVector3(dom_value[0], dom_value[1], dom_value[2]));

		translation *= mTransform;
		mTransform = translation;
		mTransform.condition();
	}

	domRotate* rotate = daeSafeCast<domRotate>(element);
	if (rotate)
	{
		domFloat4 dom_value = rotate->getValue();

		LLMatrix4 rotation;
		rotation.initRotTrans(dom_value[3] * DEG_TO_RAD, LLVector3(dom_value[0], dom_value[1], dom_value[2]), LLVector3(0, 0, 0));

		rotation *= mTransform;
		mTransform = rotation;
		mTransform.condition();
	}

	domScale* scale = daeSafeCast<domScale>(element);
	if (scale)
	{
		domFloat3 dom_value = scale->getValue();


		LLVector3 scale_vector = LLVector3(dom_value[0], dom_value[1], dom_value[2]);
		scale_vector.abs(); // Set all values positive, since we don't currently support mirrored meshes
		LLMatrix4 scaling;
		scaling.initScale(scale_vector);

		scaling *= mTransform;
		mTransform = scaling;
		mTransform.condition();
	}

	domMatrix* matrix = daeSafeCast<domMatrix>(element);
	if (matrix)
	{
		domFloat4x4 dom_value = matrix->getValue();

		LLMatrix4 matrix_transform;

		for (int i = 0; i < 4; i++)
		{
			for(int j = 0; j < 4; j++)
			{
				matrix_transform.mMatrix[i][j] = dom_value[i + j*4];
			}
		}

		matrix_transform *= mTransform;
		mTransform = matrix_transform;
		mTransform.condition();
	}

	domInstance_geometry* instance_geo = daeSafeCast<domInstance_geometry>(element);
	if (instance_geo)
	{
		domGeometry* geo = daeSafeCast<domGeometry>(instance_geo->getUrl().getElement());
		if (geo)
		{
			domMesh* mesh = daeSafeCast<domMesh>(geo->getDescendant(daeElement::matchType(domMesh::ID())));
			if (mesh)
			{

				std::vector< LLPointer< LLModel > >::iterator i = mModelsMap[mesh].begin();				
				while (i != mModelsMap[mesh].end())
				{
					LLModel* model = *i;

					LLMatrix4 transformation = mTransform;
				
					if (mTransform.determinant() < 0)
					{ //negative scales are not supported
						LL_INFOS() << "Negative scale detected, unsupported transform.  domInstance_geometry: " << getElementLabel(instance_geo) << LL_ENDL;
						badElement = true;
					}

					LLModelLoader::material_map materials = getMaterials(model, instance_geo, dae);

					// adjust the transformation to compensate for mesh normalization
					LLVector3 mesh_scale_vector;
					LLVector3 mesh_translation_vector;
					model->getNormalizedScaleTranslation(mesh_scale_vector, mesh_translation_vector);

					LLMatrix4 mesh_translation;
					mesh_translation.setTranslation(mesh_translation_vector);
					mesh_translation *= transformation;
					transformation = mesh_translation;
					
					LLMatrix4 mesh_scale;
					mesh_scale.initScale(mesh_scale_vector);
					mesh_scale *= transformation;
					transformation = mesh_scale;

					if (transformation.determinant() < 0)
					{ //negative scales are not supported
						LL_INFOS() << "Negative scale detected, unsupported post-normalization transform.  domInstance_geometry: " << getElementLabel(instance_geo) << LL_ENDL;
						badElement = true;
					}

					std::string label;
					
					if (model->mLabel.empty())
					{
						label = getLodlessLabel(instance_geo);

						llassert(!label.empty());

						if (model->mSubmodelID)
						{
							label += (char)((int)'a' + model->mSubmodelID);
						}

						model->mLabel = label + lod_suffix[mLod];
					}
					else
					{
						// Don't change model's name if possible, it will play havoc with scenes that already use said model.
						size_t ext_pos = getSuffixPosition(model->mLabel);
						if (ext_pos != -1)
						{
							label = model->mLabel.substr(0, ext_pos);
						}
						else
						{
							label = model->mLabel;
						}
					}

					mScene[transformation].push_back(LLModelInstance(model, label, transformation, materials));
					stretch_extents(model, transformation, mExtents[0], mExtents[1], mFirstTransform);
					i++;
				}
			}
		}
		else 
		{
			LL_INFOS()<<"Unable to resolve geometry URL."<<LL_ENDL;
			badElement = true;			
		}

	}	

	domInstance_node* instance_node = daeSafeCast<domInstance_node>(element);
	if (instance_node)
	{
		daeElement* instance = instance_node->getUrl().getElement();
		if (instance)
		{
			processElement(instance,badElement, dae);
		}
	}

	//process children
	daeTArray< daeSmartRef<daeElement> > children = element->getChildren();
	int childCount = children.getCount();
	for (S32 i = 0; i < childCount; i++)
	{
		processElement(children[i],badElement, dae);
	}

	if (pushed_mat)
	{ //this element was a node, restore transform before processiing siblings
		mTransform = saved_transform;
	}
}

std::map<std::string, LLImportMaterial> LLDAELoader::getMaterials(LLModel* model, domInstance_geometry* instance_geo, DAE* dae)
{
	std::map<std::string, LLImportMaterial> materials;
	for (int i = 0; i < model->mMaterialList.size(); i++)
	{
		LLImportMaterial import_material;

		domInstance_material* instance_mat = NULL;

		domBind_material::domTechnique_common* technique =
		daeSafeCast<domBind_material::domTechnique_common>(instance_geo->getDescendant(daeElement::matchType(domBind_material::domTechnique_common::ID())));

		if (technique)
		{
			daeTArray< daeSmartRef<domInstance_material> > inst_materials = technique->getChildrenByType<domInstance_material>();
			for (int j = 0; j < inst_materials.getCount(); j++)
			{
				std::string symbol(inst_materials[j]->getSymbol());

				if (symbol == model->mMaterialList[i]) // found the binding
				{
					instance_mat = inst_materials[j];
					break;
				}
			}
		}

		if (instance_mat)
		{
			domMaterial* material = daeSafeCast<domMaterial>(instance_mat->getTarget().getElement());
			if (material)
			{
				domInstance_effect* instance_effect =
				daeSafeCast<domInstance_effect>(material->getDescendant(daeElement::matchType(domInstance_effect::ID())));
				if (instance_effect)
				{
					domEffect* effect = daeSafeCast<domEffect>(instance_effect->getUrl().getElement());
					if (effect)
					{
						domProfile_COMMON* profile =
						daeSafeCast<domProfile_COMMON>(effect->getDescendant(daeElement::matchType(domProfile_COMMON::ID())));
						if (profile)
						{
							import_material = profileToMaterial(profile, dae);
						}
					}
				}
			}
		}

		import_material.mBinding = model->mMaterialList[i];
		materials[model->mMaterialList[i]] = import_material;
	}

	return materials;
}

LLImportMaterial LLDAELoader::profileToMaterial(domProfile_COMMON* material, DAE* dae)
{
	LLImportMaterial mat;
	mat.mFullbright = FALSE;

	daeElement* diffuse = material->getDescendant("diffuse");
	if (diffuse)
	{
		domCommon_color_or_texture_type_complexType::domTexture* texture =
		daeSafeCast<domCommon_color_or_texture_type_complexType::domTexture>(diffuse->getDescendant("texture"));
		if (texture)
		{
			domCommon_newparam_type_Array newparams = material->getNewparam_array();
			if (newparams.getCount())
			{

				for (S32 i = 0; i < newparams.getCount(); i++)
				{
					domFx_surface_common* surface = newparams[i]->getSurface();
					if (surface)
					{
						domFx_surface_init_common* init = surface->getFx_surface_init_common();
						if (init)
						{
							domFx_surface_init_from_common_Array init_from = init->getInit_from_array();

							if (init_from.getCount() > i)
							{
								domImage* image = daeSafeCast<domImage>(init_from[i]->getValue().getElement());
								if (image)
								{
									// we only support init_from now - embedded data will come later
									domImage::domInit_from* init = image->getInit_from();
									if (init)
									{									
										mat.mDiffuseMapFilename = cdom::uriToNativePath(init->getValue().str());
										mat.mDiffuseMapLabel = getElementLabel(material);
									}
								}
							}
						}
					}
				}
			}
			else if (texture->getTexture())
			{
				domImage* image = NULL;
				dae->getDatabase()->getElement((daeElement**) &image, 0, texture->getTexture(), COLLADA_TYPE_IMAGE);
				if (image)
				{
					// we only support init_from now - embedded data will come later
					domImage::domInit_from* init = image->getInit_from();
					if (init)
					{
						std::string image_path_value = cdom::uriToNativePath(init->getValue().str());

#if LL_WINDOWS
						// Work-around DOM tendency to resort to UNC names which are only confusing for downstream...
						//
						std::string::iterator i = image_path_value.begin();
						while (*i == '\\')
							i++;
						mat.mDiffuseMapFilename.assign(i, image_path_value.end());
#else
						mat.mDiffuseMapFilename = image_path_value;
#endif
						mat.mDiffuseMapLabel = getElementLabel(material);
					}
				}
			}
		}

		domCommon_color_or_texture_type_complexType::domColor* color =
		daeSafeCast<domCommon_color_or_texture_type_complexType::domColor>(diffuse->getDescendant("color"));
		if (color)
		{
			domFx_color_common domfx_color = color->getValue();
			LLColor4 value = LLColor4(domfx_color[0], domfx_color[1], domfx_color[2], domfx_color[3]);
			mat.mDiffuseColor = value;
		}
	}

	daeElement* emission = material->getDescendant("emission");
	if (emission)
	{
		LLColor4 emission_color = getDaeColor(emission);
		if (((emission_color[0] + emission_color[1] + emission_color[2]) / 3.0) > 0.25)
		{
			mat.mFullbright = TRUE;
		}
	}

	return mat;
}

// try to get a decent label for this element
std::string LLDAELoader::getElementLabel(daeElement *element)
{
	// if we have a name attribute, use it
	std::string name = element->getAttribute("name");
	if (name.length())
	{
		return name;
	}

	// if we have an ID attribute, use it
	if (element->getID())
	{
		return std::string(element->getID());
	}

	// if we have a parent, use it
	daeElement* parent = element->getParent();
	std::string index_string;
	if (parent)
	{
		// retrieve index to distinguish items inside same parent
		size_t ind = 0;
		parent->getChildren().find(element, ind);

		if (ind > 0)
		{
			index_string = "_" + boost::lexical_cast<std::string>(ind);
		}

		// if parent has a name or ID, use it
		std::string name = parent->getAttribute("name");
		if (!name.length())
		{
			name = std::string(parent->getID());
		}

		if (name.length())
		{
			// make sure that index won't mix up with pre-named lod extensions
			size_t ext_pos = getSuffixPosition(name);

			if (ext_pos == -1)
			{
				return name + index_string;
			}
			else
			{
				return name.insert(ext_pos, index_string);
			}
		}
	}

	// try to use our type
	daeString element_name = element->getElementName();
	if (element_name)
	{
		return std::string(element_name) + index_string;
	}

	// if all else fails, use "object"
	return std::string("object") + index_string;
}

// static
size_t LLDAELoader::getSuffixPosition(std::string label)
{
	if ((label.find("_LOD") != -1) || (label.find("_PHYS") != -1))
	{
		return label.rfind('_');
	}
	return -1;
}

// static
std::string LLDAELoader::getLodlessLabel(daeElement *element)
{
	std::string label = getElementLabel(element);
	size_t ext_pos = getSuffixPosition(label);
	if (ext_pos != -1)
	{
		return label.substr(0, ext_pos);
	}
	return label;
}

LLColor4 LLDAELoader::getDaeColor(daeElement* element)
{
	LLColor4 value;
	domCommon_color_or_texture_type_complexType::domColor* color =
	daeSafeCast<domCommon_color_or_texture_type_complexType::domColor>(element->getDescendant("color"));
	if (color)
	{
		domFx_color_common domfx_color = color->getValue();
		value = LLColor4(domfx_color[0], domfx_color[1], domfx_color[2], domfx_color[3]);
	}

	return value;
}

bool LLDAELoader::addVolumeFacesFromDomMesh(LLModel* pModel,domMesh* mesh)
{
	LLModel::EModelStatus status = LLModel::NO_ERRORS;
	domTriangles_Array& tris = mesh->getTriangles_array();

	for (U32 i = 0; i < tris.getCount(); ++i)
	{
		domTrianglesRef& tri = tris.get(i);

		status = load_face_from_dom_triangles(pModel->getVolumeFaces(), pModel->getMaterialList(), tri);
		pModel->mStatus = status;
		if(status != LLModel::NO_ERRORS)
		{
			pModel->ClearFacesAndMaterials();
			return false;
		}
	}

	domPolylist_Array& polys = mesh->getPolylist_array();
	for (U32 i = 0; i < polys.getCount(); ++i)
	{
		domPolylistRef& poly = polys.get(i);
		status = load_face_from_dom_polylist(pModel->getVolumeFaces(), pModel->getMaterialList(), poly);

		if(status != LLModel::NO_ERRORS)
		{
			pModel->ClearFacesAndMaterials();
			return false;
		}
	}

	domPolygons_Array& polygons = mesh->getPolygons_array();

	for (U32 i = 0; i < polygons.getCount(); ++i)
	{
		domPolygonsRef& poly = polygons.get(i);
		status = load_face_from_dom_polygons(pModel->getVolumeFaces(), pModel->getMaterialList(), poly);

		if(status != LLModel::NO_ERRORS)
		{
			pModel->ClearFacesAndMaterials();
			return false;
		}
	}

	return (status == LLModel::NO_ERRORS);
}

//static 
LLModel* LLDAELoader::loadModelFromDomMesh(domMesh *mesh)
{
	LLVolumeParams volume_params;
	volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);
	LLModel* ret = new LLModel(volume_params, 0.f); 
	createVolumeFacesFromDomMesh(ret, mesh);
    if (ret->mLabel.empty())
    {
	    ret->mLabel = getElementLabel(mesh);
    }
    return ret;
}

//static diff version supports creating multiple models when material counts spill
// over the 8 face server-side limit
//
bool LLDAELoader::loadModelsFromDomMesh(domMesh* mesh, std::vector<LLModel*>& models_out, U32 submodel_limit)
{

	LLVolumeParams volume_params;
	volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);

	models_out.clear();

	LLModel* ret = new LLModel(volume_params, 0.f);

	std::string model_name = getLodlessLabel(mesh);
	ret->mLabel = model_name + lod_suffix[mLod];

	llassert(!ret->mLabel.empty());

	// Like a monkey, ready to be shot into space
	//
	ret->ClearFacesAndMaterials();

	// Get the whole set of volume faces
	//
	addVolumeFacesFromDomMesh(ret, mesh);

	U32 volume_faces = ret->getNumVolumeFaces();

	// Side-steps all manner of issues when splitting models
	// and matching lower LOD materials to base models
	//
	ret->sortVolumeFacesByMaterialName();

	bool normalized = false;

    int submodelID = 0;

	// remove all faces that definitely won't fit into one model and submodel limit
	U32 face_limit = (submodel_limit + 1) * LL_SCULPT_MESH_MAX_FACES;
	if (face_limit < volume_faces)
	{
		ret->setNumVolumeFaces(face_limit);
	}

	LLVolume::face_list_t remainder;
	do 
	{
		// Insure we do this once with the whole gang and not per-model
		//
		if (!normalized && !mNoNormalize)
		{			
			normalized = true;
			ret->normalizeVolumeFaces();
		}

		ret->trimVolumeFacesToSize(LL_SCULPT_MESH_MAX_FACES, &remainder);

		if (!mNoOptimize)
		{
			ret->optimizeVolumeFaces();
		}

		volume_faces = remainder.size();

		models_out.push_back(ret);

		// If we have left-over volume faces, create another model
		// to absorb them...
		//
		if (volume_faces)
		{
			LLModel* next = new LLModel(volume_params, 0.f);
			next->mSubmodelID = ++submodelID;
			next->mLabel = model_name + (char)((int)'a' + next->mSubmodelID) + lod_suffix[mLod];
			next->getVolumeFaces() = remainder;
			next->mNormalizedScale = ret->mNormalizedScale;
			next->mNormalizedTranslation = ret->mNormalizedTranslation;
			if ( ret->mMaterialList.size() > LL_SCULPT_MESH_MAX_FACES)
			{
				next->mMaterialList.assign(ret->mMaterialList.begin() + LL_SCULPT_MESH_MAX_FACES, ret->mMaterialList.end());
			}
			ret = next;
		}

		remainder.clear();

	} while (volume_faces);	

	return true;
}

bool LLDAELoader::createVolumeFacesFromDomMesh(LLModel* pModel, domMesh* mesh)
{
	if (mesh)
	{
		pModel->ClearFacesAndMaterials();

		addVolumeFacesFromDomMesh(pModel, mesh);

		if (pModel->getNumVolumeFaces() > 0)
		{
			pModel->normalizeVolumeFaces();
			pModel->optimizeVolumeFaces();

			if (pModel->getNumVolumeFaces() > 0)
			{
				return true;
			}
		}
	}
	else
	{	
		LL_WARNS() << "no mesh found" << LL_ENDL;
	}

	return false;
}
