/** 
 * @file llmodel.cpp
 * @brief Model handling implementation
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

#include "llmodel.h"
#include "llmemory.h"
#include "llconvexdecomposition.h"
#include "llsdserialize.h"
#include "llvector4a.h"
#if LL_MSVC
#pragma warning (disable : 4263)
#pragma warning (disable : 4264)
#endif
#include "dae.h"
#include "dae/daeErrorHandler.h"
#include "dom/domConstants.h"
#include "dom/domMesh.h"
#if LL_MSVC
#pragma warning (default : 4263)
#pragma warning (default : 4264)
#endif

#ifdef LL_STANDALONE
# include <zlib.h>
#else
# include "zlib/zlib.h"
#endif



std::string model_names[] =
{
	"lowest_lod",
	"low_lod",
	"medium_lod",
	"high_lod",
	"physics_mesh"
};

const int MODEL_NAMES_LENGTH = sizeof(model_names) / sizeof(std::string);

LLModel::LLModel(LLVolumeParams& params, F32 detail)
	: LLVolume(params, detail), mNormalizedScale(1,1,1), mNormalizedTranslation(0,0,0)
	, mPelvisOffset( 0.0f ), mStatus(NO_ERRORS)
{
	mDecompID = -1;
	mLocalID = -1;
}

LLModel::~LLModel()
{
	if (mDecompID >= 0)
	{
		LLConvexDecomposition::getInstance()->deleteDecomposition(mDecompID);
	}
}


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

	if ( !get_dom_sources(inputs, pos_offset, tc_offset, norm_offset, idx_stride, pos_source, tc_source, norm_source) || !pos_source )
	{
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
				if ((point_iter->second)[j] == cv)
				{
					found = TRUE;
					indices.push_back((point_iter->second)[j].mIndex);
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
				//llerrs << "Attempted to write model exceeding 16-bit index buffer limitation." << llendl;
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
			face_list.push_back(face);
			face_list.rbegin()->fillFromLegacyData(verts, indices);
			LLVolumeFace& new_face = *face_list.rbegin();
			if (!norm_source)
			{
				ll_aligned_free_16(new_face.mNormals);
				new_face.mNormals = NULL;
			}

			if (!tc_source)
			{
				ll_aligned_free_16(new_face.mTexCoords);
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
			ll_aligned_free_16(new_face.mNormals);
			new_face.mNormals = NULL;
		}

		if (!tc_source)
		{
			ll_aligned_free_16(new_face.mTexCoords);
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
					//llerrs << "Attempted to write model exceeding 16-bit index buffer limitation." << llendl;
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
				face_list.push_back(face);
				face_list.rbegin()->fillFromLegacyData(verts, indices);
				LLVolumeFace& new_face = *face_list.rbegin();
				if (!norm_source)
				{
					ll_aligned_free_16(new_face.mNormals);
					new_face.mNormals = NULL;
				}

				if (!tc_source)
				{
					ll_aligned_free_16(new_face.mTexCoords);
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
			ll_aligned_free_16(new_face.mNormals);
			new_face.mNormals = NULL;
		}

		if (!tc_source)
		{
			ll_aligned_free_16(new_face.mTexCoords);
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
				vert.getPosition().set(v->get(v_idx),
								v->get(v_idx+1),
								v->get(v_idx+2));
			}
			
			if (n)
			{
				U32 n_idx = idx[j*stride+n_offset]*3;
				vert.getNormal().set(n->get(n_idx),
								n->get(n_idx+1),
								n->get(n_idx+2));
			}
			
			if (t)
			{
				U32 t_idx = idx[j*stride+t_offset]*2;
				vert.mTexCoord.setVec(t->get(t_idx),
								t->get(t_idx+1));								
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
			ll_aligned_free_16(new_face.mNormals);
			new_face.mNormals = NULL;
		}

		if (!t)
		{
			ll_aligned_free_16(new_face.mTexCoords);
			new_face.mTexCoords = NULL;
		}
	}

	return LLModel::NO_ERRORS ;
}

//static
std::string LLModel::getStatusString(U32 status)
{
	const static std::string status_strings[(S32)INVALID_STATUS] = {"status_no_error", "status_vertex_number_overflow","bad_element"};

	if(status < INVALID_STATUS)
	{
		if(status_strings[status] == std::string())
		{
			llerrs << "No valid status string for this status: " << (U32)status << llendl ;
		}
		return status_strings[status] ;
	}

	llerrs << "Invalid model status: " << (U32)status << llendl ;

	return std::string() ;
}

void LLModel::addVolumeFacesFromDomMesh(domMesh* mesh)
{
	domTriangles_Array& tris = mesh->getTriangles_array();
		
	for (U32 i = 0; i < tris.getCount(); ++i)
	{
		domTrianglesRef& tri = tris.get(i);

		mStatus = load_face_from_dom_triangles(mVolumeFaces, mMaterialList, tri);
		
		if(mStatus != NO_ERRORS)
		{
			mVolumeFaces.clear() ;
			mMaterialList.clear() ;
			return ; //abort
		}
	}

	domPolylist_Array& polys = mesh->getPolylist_array();
	for (U32 i = 0; i < polys.getCount(); ++i)
	{
		domPolylistRef& poly = polys.get(i);
		mStatus = load_face_from_dom_polylist(mVolumeFaces, mMaterialList, poly);

		if(mStatus != NO_ERRORS)
		{
			mVolumeFaces.clear() ;
			mMaterialList.clear() ;
			return ; //abort
		}
	}
	
	domPolygons_Array& polygons = mesh->getPolygons_array();
	
	for (U32 i = 0; i < polygons.getCount(); ++i)
	{
		domPolygonsRef& poly = polygons.get(i);
		mStatus = load_face_from_dom_polygons(mVolumeFaces, mMaterialList, poly);

		if(mStatus != NO_ERRORS)
		{
			mVolumeFaces.clear() ;
			mMaterialList.clear() ;
			return ; //abort
		}
	}
 
}

BOOL LLModel::createVolumeFacesFromDomMesh(domMesh* mesh)
{
	if (mesh)
	{
		mVolumeFaces.clear();
		mMaterialList.clear();

		addVolumeFacesFromDomMesh(mesh);
		
		if (getNumVolumeFaces() > 0)
		{
			normalizeVolumeFaces();
			optimizeVolumeFaces();
			
			if (getNumVolumeFaces() > 0)
			{
				return TRUE;
			}
		}
	}
	else
	{	
		llwarns << "no mesh found" << llendl;
	}
	
	return FALSE;
}

void LLModel::offsetMesh( const LLVector3& pivotPoint )
{
	LLVector4a pivot( pivotPoint[VX], pivotPoint[VY], pivotPoint[VZ] );
	
	for (std::vector<LLVolumeFace>::iterator faceIt = mVolumeFaces.begin(); faceIt != mVolumeFaces.end(); )
	{
		std::vector<LLVolumeFace>:: iterator currentFaceIt = faceIt++;
		LLVolumeFace& face = *currentFaceIt;
		LLVector4a *pos = (LLVector4a*) face.mPositions;
		
		for (U32 i=0; i<face.mNumVertices; ++i )
		{
			pos[i].add( pivot );
		}
	}
}

void LLModel::optimizeVolumeFaces()
{
	for (U32 i = 0; i < getNumVolumeFaces(); ++i)
	{
		mVolumeFaces[i].optimize();
	}
}

// Shrink the model to fit
// on a 1x1x1 cube centered at the origin.
// The positions and extents
// multiplied by  mNormalizedScale
// and offset by mNormalizedTranslation
// to be the "original" extents and position.
// Also, the positions will fit
// within the unit cube.
void LLModel::normalizeVolumeFaces()
{

	// ensure we don't have too many faces
	if (mVolumeFaces.size() > LL_SCULPT_MESH_MAX_FACES)
		mVolumeFaces.resize(LL_SCULPT_MESH_MAX_FACES);
	
	if (!mVolumeFaces.empty())
	{
		LLVector4a min, max;
		
		// For all of the volume faces
		// in the model, loop over
		// them and see what the extents
		// of the volume along each axis.
		min = mVolumeFaces[0].mExtents[0];
		max = mVolumeFaces[0].mExtents[1];

		for (U32 i = 1; i < mVolumeFaces.size(); ++i)
		{
			LLVolumeFace& face = mVolumeFaces[i];

			update_min_max(min, max, face.mExtents[0]);
			update_min_max(min, max, face.mExtents[1]);

			if (face.mTexCoords)
			{
				LLVector2& min_tc = face.mTexCoordExtents[0];
				LLVector2& max_tc = face.mTexCoordExtents[1];

				min_tc = face.mTexCoords[0];
				max_tc = face.mTexCoords[0];

				for (U32 j = 1; j < face.mNumVertices; ++j)
				{
					update_min_max(min_tc, max_tc, face.mTexCoords[j]);
				}
			}
			else
			{
				face.mTexCoordExtents[0].set(0,0);
				face.mTexCoordExtents[1].set(1,1);
			}
		}

		// Now that we have the extents of the model
		// we can compute the offset needed to center
		// the model at the origin.

		// Compute center of the model
		// and make it negative to get translation
		// needed to center at origin.
		LLVector4a trans;
		trans.setAdd(min, max);
		trans.mul(-0.5f);

		// Compute the total size along all
		// axes of the model.
		LLVector4a size;
		size.setSub(max, min);

		// Prevent division by zero.
		F32 x = size[0];
		F32 y = size[1];
		F32 z = size[2];
		F32 w = size[3];
		if (fabs(x)<F_APPROXIMATELY_ZERO)
		{
			x = 1.0;
		}
		if (fabs(y)<F_APPROXIMATELY_ZERO)
		{
			y = 1.0;
		}
		if (fabs(z)<F_APPROXIMATELY_ZERO)
		{
			z = 1.0;
		}
		size.set(x,y,z,w);

		// Compute scale as reciprocal of size
		LLVector4a scale;
		scale.splat(1.f);
		scale.div(size);

		LLVector4a inv_scale(1.f);
		inv_scale.div(scale);

		for (U32 i = 0; i < mVolumeFaces.size(); ++i)
		{
			LLVolumeFace& face = mVolumeFaces[i];

			// We shrink the extents so
			// that they fall within
			// the unit cube.
			face.mExtents[0].add(trans);
			face.mExtents[0].mul(scale);

			face.mExtents[1].add(trans);
			face.mExtents[1].mul(scale);

			// For all the positions, we scale
			// the positions to fit within the unit cube.
			LLVector4a* pos = (LLVector4a*) face.mPositions;
			LLVector4a* norm = (LLVector4a*) face.mNormals;

			for (U32 j = 0; j < face.mNumVertices; ++j)
			{
			 	pos[j].add(trans);
				pos[j].mul(scale);
				if (norm && !norm[j].equals3(LLVector4a::getZero()))
				{
					norm[j].mul(inv_scale);
					norm[j].normalize3();
				}
			}
		}

		// mNormalizedScale is the scale at which
		// we would need to multiply the model
		// by to get the original size of the
		// model instead of the normalized size.
		LLVector4a normalized_scale;
		normalized_scale.splat(1.f);
		normalized_scale.div(scale);
		mNormalizedScale.set(normalized_scale.getF32ptr());
		mNormalizedTranslation.set(trans.getF32ptr());
		mNormalizedTranslation *= -1.f; 
	}
}

void LLModel::getNormalizedScaleTranslation(LLVector3& scale_out, LLVector3& translation_out)
{
	scale_out = mNormalizedScale;
	translation_out = mNormalizedTranslation;
}

void LLModel::setNumVolumeFaces(S32 count)
{
	mVolumeFaces.resize(count);
}

void LLModel::setVolumeFaceData(
	S32 f, 
	LLStrider<LLVector3> pos, 
	LLStrider<LLVector3> norm, 
	LLStrider<LLVector2> tc, 
	LLStrider<U16> ind, 
	U32 num_verts, 
	U32 num_indices)
{
	LLVolumeFace& face = mVolumeFaces[f];

	face.resizeVertices(num_verts);
	face.resizeIndices(num_indices);

	LLVector4a::memcpyNonAliased16((F32*) face.mPositions, (F32*) pos.get(), num_verts*4*sizeof(F32));
	if (norm.get())
	{
		LLVector4a::memcpyNonAliased16((F32*) face.mNormals, (F32*) norm.get(), num_verts*4*sizeof(F32));
	}
	else
	{
		ll_aligned_free_16(face.mNormals);
		face.mNormals = NULL;
	}

	if (tc.get())
	{
		U32 tex_size = (num_verts*2*sizeof(F32)+0xF)&~0xF;
		LLVector4a::memcpyNonAliased16((F32*) face.mTexCoords, (F32*) tc.get(), tex_size);
	}
	else
	{
		ll_aligned_free_16(face.mTexCoords);
		face.mTexCoords = NULL;
	}

	U32 size = (num_indices*2+0xF)&~0xF;
	LLVector4a::memcpyNonAliased16((F32*) face.mIndices, (F32*) ind.get(), size);
}

void LLModel::appendFaces(LLModel *model, LLMatrix4 &transform, LLMatrix4& norm_mat)
{
	if (mVolumeFaces.empty())
	{
		setNumVolumeFaces(1);
	}

	LLVolumeFace& face = mVolumeFaces[mVolumeFaces.size()-1];


	for (S32 i = 0; i < model->getNumFaces(); ++i)
	{
		face.appendFace(model->getVolumeFace(i), transform, norm_mat);
	}

}

void LLModel::appendFace(const LLVolumeFace& src_face, std::string src_material, LLMatrix4& mat, LLMatrix4& norm_mat)
{
	S32 rindex = getNumVolumeFaces()-1; 
	if (rindex == -1 || 
		mVolumeFaces[rindex].mNumVertices + src_face.mNumVertices >= 65536)
	{ //empty or overflow will occur, append new face
		LLVolumeFace cur_face;
		cur_face.appendFace(src_face, mat, norm_mat);
		addFace(cur_face);
		mMaterialList.push_back(src_material);
	}
	else
	{ //append to existing end face
		mVolumeFaces.rbegin()->appendFace(src_face, mat, norm_mat);
	}
}

void LLModel::addFace(const LLVolumeFace& face)
{
	if (face.mNumVertices == 0)
	{
		llerrs << "Cannot add empty face." << llendl;
	}

	mVolumeFaces.push_back(face);

	if (mVolumeFaces.size() > MAX_MODEL_FACES)
	{
		llerrs << "Model prims cannot have more than " << MAX_MODEL_FACES << " faces!" << llendl;
	}
}


void LLModel::generateNormals(F32 angle_cutoff)
{
	//generate normals for all faces by:
	// 1 - Create faceted copy of face with no texture coordinates
	// 2 - Weld vertices in faceted copy that are shared between triangles with less than "angle_cutoff" difference between normals
	// 3 - Generate smoothed set of normals based on welding results
	// 4 - Create faceted copy of face with texture coordinates
	// 5 - Copy smoothed normals to faceted copy, using closest normal to triangle normal where more than one normal exists for a given position
	// 6 - Remove redundant vertices from new faceted (now smooth) copy

	angle_cutoff = cosf(angle_cutoff);
	for (U32 j = 0; j < mVolumeFaces.size(); ++j)
	{
		LLVolumeFace& vol_face = mVolumeFaces[j];

		if (vol_face.mNumIndices > 65535)
		{
			llwarns << "Too many vertices for normal generation to work." << llendl;
			continue;
		}

		//create faceted copy of current face with no texture coordinates (step 1)
		LLVolumeFace faceted;

		LLVector4a* src_pos = (LLVector4a*) vol_face.mPositions;
		//LLVector4a* src_norm = (LLVector4a*) vol_face.mNormals;


		faceted.resizeVertices(vol_face.mNumIndices);
		faceted.resizeIndices(vol_face.mNumIndices);
		//bake out triangles into temporary face, clearing texture coordinates
		for (U32 i = 0; i < vol_face.mNumIndices; ++i)
		{
			U32 idx = vol_face.mIndices[i];
		
			faceted.mPositions[i] = src_pos[idx];
			faceted.mTexCoords[i] = LLVector2(0,0);
			faceted.mIndices[i] = i;
		}

		//generate normals for temporary face
		for (U32 i = 0; i < faceted.mNumIndices; i += 3)
		{ //for each triangle
			U16 i0 = faceted.mIndices[i+0];
			U16 i1 = faceted.mIndices[i+1];
			U16 i2 = faceted.mIndices[i+2];
			
			LLVector4a& p0 = faceted.mPositions[i0];
			LLVector4a& p1 = faceted.mPositions[i1];
			LLVector4a& p2 = faceted.mPositions[i2];

			LLVector4a& n0 = faceted.mNormals[i0];
			LLVector4a& n1 = faceted.mNormals[i1];
			LLVector4a& n2 = faceted.mNormals[i2];

			LLVector4a lhs, rhs;
			lhs.setSub(p1, p0);
			rhs.setSub(p2, p0);

			n0.setCross3(lhs, rhs);
			n0.normalize3();
			n1 = n0;
			n2 = n0;
		}

		//weld vertices in temporary face, respecting angle_cutoff (step 2)
		faceted.optimize(angle_cutoff);

		//generate normals for welded face based on new topology (step 3)

		for (U32 i = 0; i < faceted.mNumVertices; i++)
		{
			faceted.mNormals[i].clear();
		}

		for (U32 i = 0; i < faceted.mNumIndices; i += 3)
		{ //for each triangle
			U16 i0 = faceted.mIndices[i+0];
			U16 i1 = faceted.mIndices[i+1];
			U16 i2 = faceted.mIndices[i+2];
			
			LLVector4a& p0 = faceted.mPositions[i0];
			LLVector4a& p1 = faceted.mPositions[i1];
			LLVector4a& p2 = faceted.mPositions[i2];

			LLVector4a& n0 = faceted.mNormals[i0];
			LLVector4a& n1 = faceted.mNormals[i1];
			LLVector4a& n2 = faceted.mNormals[i2];

			LLVector4a lhs, rhs;
			lhs.setSub(p1, p0);
			rhs.setSub(p2, p0);

			LLVector4a n;
			n.setCross3(lhs, rhs);

			n0.add(n);
			n1.add(n);
			n2.add(n);
		}

		//normalize normals and build point map
		LLVolumeFace::VertexMapData::PointMap point_map;

		for (U32 i = 0; i < faceted.mNumVertices; ++i)
		{
			faceted.mNormals[i].normalize3();

			LLVolumeFace::VertexMapData v;
			v.setPosition(faceted.mPositions[i]);
			v.setNormal(faceted.mNormals[i]);

			point_map[LLVector3(v.getPosition().getF32ptr())].push_back(v);
		}

		//create faceted copy of current face with texture coordinates (step 4)
		LLVolumeFace new_face;

		//bake out triangles into new face
		new_face.resizeIndices(vol_face.mNumIndices);
		new_face.resizeVertices(vol_face.mNumIndices);
		
		for (U32 i = 0; i < vol_face.mNumIndices; ++i)
		{
			U32 idx = vol_face.mIndices[i];
			LLVolumeFace::VertexData v;
			new_face.mPositions[i] = vol_face.mPositions[idx];
			new_face.mNormals[i].clear();
			new_face.mIndices[i] = i;
		}

		if (vol_face.mTexCoords)
		{
			for (U32 i = 0; i < vol_face.mNumIndices; i++)
			{
				U32 idx = vol_face.mIndices[i];
				new_face.mTexCoords[i] = vol_face.mTexCoords[idx];
			}
		}
		else
		{
			ll_aligned_free_16(new_face.mTexCoords);
			new_face.mTexCoords = NULL;
		}

		//generate normals for new face
		for (U32 i = 0; i < new_face.mNumIndices; i += 3)
		{ //for each triangle
			U16 i0 = new_face.mIndices[i+0];
			U16 i1 = new_face.mIndices[i+1];
			U16 i2 = new_face.mIndices[i+2];
			
			LLVector4a& p0 = new_face.mPositions[i0];
			LLVector4a& p1 = new_face.mPositions[i1];
			LLVector4a& p2 = new_face.mPositions[i2];

			LLVector4a& n0 = new_face.mNormals[i0];
			LLVector4a& n1 = new_face.mNormals[i1];
			LLVector4a& n2 = new_face.mNormals[i2];

			LLVector4a lhs, rhs;
			lhs.setSub(p1, p0);
			rhs.setSub(p2, p0);

			n0.setCross3(lhs, rhs);
			n0.normalize3();
			n1 = n0;
			n2 = n0;
		}

		//swap out normals in new_face with best match from point map (step 5)
		for (U32 i = 0; i < new_face.mNumVertices; ++i)
		{
			//LLVolumeFace::VertexData v = new_face.mVertices[i];

			LLVector4a ref_norm = new_face.mNormals[i];

			LLVolumeFace::VertexMapData::PointMap::iterator iter = point_map.find(LLVector3(new_face.mPositions[i].getF32ptr()));

			if (iter != point_map.end())
			{
				F32 best = -2.f;
				for (U32 k = 0; k < iter->second.size(); ++k)
				{
					LLVector4a& n = iter->second[k].getNormal();

					F32 cur = n.dot3(ref_norm).getF32();

					if (cur > best)
					{
						best = cur;
						new_face.mNormals[i] = n;
					}
				}
			}
		}
		
		//remove redundant vertices from new face (step 6)
		new_face.optimize();

		mVolumeFaces[j] = new_face;
	}
}

//static
std::string LLModel::getElementLabel(daeElement *element)
{ // try to get a decent label for this element
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
	if (parent)
	{
		// if parent has a name, use it
		std::string name = parent->getAttribute("name");
		if (name.length())
		{
			return name;
		}

		// if parent has an ID, use it
		if (parent->getID())
		{
			return std::string(parent->getID());
		}
	}

	// try to use our type
	daeString element_name = element->getElementName();
	if (element_name)
	{
		return std::string(element_name);
	}

	// if all else fails, use "object"
	return std::string("object");
}

//static 
LLModel* LLModel::loadModelFromDomMesh(domMesh *mesh)
{
	LLVolumeParams volume_params;
	volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);
	LLModel* ret = new LLModel(volume_params, 0.f); 
	ret->createVolumeFacesFromDomMesh(mesh);
	ret->mLabel = getElementLabel(mesh);
	return ret;
}

std::string LLModel::getName() const
{
	if (!mRequestedLabel.empty())
		return mRequestedLabel;
	else
		return mLabel;
}

//static
LLSD LLModel::writeModel(
	std::ostream& ostr,
	LLModel* physics,
	LLModel* high,
	LLModel* medium,
	LLModel* low,
	LLModel* impostor,
	const LLModel::Decomposition& decomp,
	BOOL upload_skin,
	BOOL upload_joints,
	BOOL nowrite,
	BOOL as_slm)
{
	LLSD mdl;

	LLModel* model[] = 
	{
		impostor,
		low,
		medium,
		high,
		physics
	};

	bool skinning = upload_skin && high && !high->mSkinWeights.empty();

	if (skinning)
	{ //write skinning block
		mdl["skin"] = high->mSkinInfo.asLLSD(upload_joints);
	}

	if (!decomp.mBaseHull.empty() ||
		!decomp.mHull.empty())		
	{
		mdl["physics_convex"] = decomp.asLLSD();
		if (!decomp.mHull.empty() && !as_slm)
		{ //convex decomposition exists, physics mesh will not be used (unless this is an slm file)
			model[LLModel::LOD_PHYSICS] = NULL;
		}
	}

	if (as_slm)
	{ //save material list names
		for (U32 i = 0; i < high->mMaterialList.size(); ++i)
		{
			mdl["material_list"][i] = high->mMaterialList[i];
		}
	}

	for (U32 idx = 0; idx < MODEL_NAMES_LENGTH; ++idx)
	{
		if (model[idx] && model[idx]->getNumVolumeFaces() > 0)
		{
			LLVector3 min_pos = LLVector3(model[idx]->getVolumeFace(0).mPositions[0].getF32ptr());
			LLVector3 max_pos = min_pos;

			//find position domain
			for (S32 i = 0; i < model[idx]->getNumVolumeFaces(); ++i)
			{ //for each face
				const LLVolumeFace& face = model[idx]->getVolumeFace(i);
				for (U32 j = 0; j < face.mNumVertices; ++j)
				{
					update_min_max(min_pos, max_pos, face.mPositions[j].getF32ptr());
				}
			}

			LLVector3 pos_range = max_pos - min_pos;

			for (S32 i = 0; i < model[idx]->getNumVolumeFaces(); ++i)
			{ //for each face
				const LLVolumeFace& face = model[idx]->getVolumeFace(i);
				if (face.mNumVertices < 3)
				{ //don't export an empty face
					mdl[model_names[idx]][i]["NoGeometry"] = true;
					continue;
				}
				LLSD::Binary verts(face.mNumVertices*3*2);
				LLSD::Binary tc(face.mNumVertices*2*2);
				LLSD::Binary normals(face.mNumVertices*3*2);
				LLSD::Binary indices(face.mNumIndices*2);

				U32 vert_idx = 0;
				U32 norm_idx = 0;
				U32 tc_idx = 0;
			
				LLVector2* ftc = (LLVector2*) face.mTexCoords;
				LLVector2 min_tc;
				LLVector2 max_tc;

				if (ftc)
				{
					min_tc = ftc[0];
					max_tc = min_tc;
					
					//get texture coordinate domain
					for (U32 j = 0; j < face.mNumVertices; ++j)
					{
						update_min_max(min_tc, max_tc, ftc[j]);
					}
				}

				LLVector2 tc_range = max_tc - min_tc;

				for (U32 j = 0; j < face.mNumVertices; ++j)
				{ //for each vert
		
					F32* pos = face.mPositions[j].getF32ptr();
										
					//position
					for (U32 k = 0; k < 3; ++k)
					{ //for each component
						//convert to 16-bit normalized across domain
						U16 val = (U16) (((pos[k]-min_pos.mV[k])/pos_range.mV[k])*65535);

						U8* buff = (U8*) &val;
						//write to binary buffer
						verts[vert_idx++] = buff[0];
						verts[vert_idx++] = buff[1];
					}

					if (face.mNormals)
					{ //normals
						F32* norm = face.mNormals[j].getF32ptr();

						for (U32 k = 0; k < 3; ++k)
						{ //for each component
							//convert to 16-bit normalized
							U16 val = (U16) ((norm[k]+1.f)*0.5f*65535);
							U8* buff = (U8*) &val;

							//write to binary buffer
							normals[norm_idx++] = buff[0];
							normals[norm_idx++] = buff[1];
						}
					}
					
					F32* src_tc = (F32*) face.mTexCoords[j].mV;

					//texcoord
					if (face.mTexCoords)
					{
						for (U32 k = 0; k < 2; ++k)
						{ //for each component
							//convert to 16-bit normalized
							U16 val = (U16) ((src_tc[k]-min_tc.mV[k])/tc_range.mV[k]*65535);

							U8* buff = (U8*) &val;
							//write to binary buffer
							tc[tc_idx++] = buff[0];
							tc[tc_idx++] = buff[1];
						}
					}
				}

				U32 idx_idx = 0;
				for (U32 j = 0; j < face.mNumIndices; ++j)
				{
					U8* buff = (U8*) &(face.mIndices[j]);
					indices[idx_idx++] = buff[0];
					indices[idx_idx++] = buff[1];
				}

				//write out face data
				mdl[model_names[idx]][i]["PositionDomain"]["Min"] = min_pos.getValue();
				mdl[model_names[idx]][i]["PositionDomain"]["Max"] = max_pos.getValue();
				mdl[model_names[idx]][i]["Position"] = verts;
				
				if (face.mNormals)
				{
					mdl[model_names[idx]][i]["Normal"] = normals;
				}

				if (face.mTexCoords)
				{
					mdl[model_names[idx]][i]["TexCoord0Domain"]["Min"] = min_tc.getValue();
					mdl[model_names[idx]][i]["TexCoord0Domain"]["Max"] = max_tc.getValue();
					mdl[model_names[idx]][i]["TexCoord0"] = tc;
				}

				mdl[model_names[idx]][i]["TriangleList"] = indices;

				if (skinning)
				{
					//write out skin weights

					//each influence list entry is up to 4 24-bit values
					// first 8 bits is bone index
					// last 16 bits is bone influence weight
					// a bone index of 0xFF signifies no more influences for this vertex

					std::stringstream ostr;

					for (U32 j = 0; j < face.mNumVertices; ++j)
					{
						LLVector3 pos(face.mPositions[j].getF32ptr());

						weight_list& weights = high->getJointInfluences(pos);

						S32 count = 0;
						for (weight_list::iterator iter = weights.begin(); iter != weights.end(); ++iter)
						{
							if (iter->mJointIdx < 255 && iter->mJointIdx >= 0)
							{
								U8 idx = (U8) iter->mJointIdx;
								ostr.write((const char*) &idx, 1);

								U16 influence = (U16) (iter->mWeight*65535);
								ostr.write((const char*) &influence, 2);

								++count;
							}		
						}
						U8 end_list = 0xFF;
						if (count < 4)
						{
							ostr.write((const char*) &end_list, 1);
						}
					}

					//copy ostr to binary buffer
					std::string data = ostr.str();
					const U8* buff = (U8*) data.data();
					U32 bytes = data.size();

					LLSD::Binary w(bytes);
					for (U32 j = 0; j < bytes; ++j)
					{
						w[j] = buff[j];
					}

					mdl[model_names[idx]][i]["Weights"] = w;
				}
			}
		}
	}
	
	return writeModelToStream(ostr, mdl, nowrite, as_slm);
}

LLSD LLModel::writeModelToStream(std::ostream& ostr, LLSD& mdl, BOOL nowrite, BOOL as_slm)
{
	U32 bytes = 0;
	
	std::string::size_type cur_offset = 0;

	LLSD header;

	if (as_slm && mdl.has("material_list"))
	{ //save material binding names to header
		header["material_list"] = mdl["material_list"];
	}

	std::string skin;

	if (mdl.has("skin"))
	{ //write out skin block
		skin = zip_llsd(mdl["skin"]);

		U32 size = skin.size();
		if (size > 0)
		{
			header["skin"]["offset"] = (LLSD::Integer) cur_offset;
			header["skin"]["size"] = (LLSD::Integer) size;
			cur_offset += size;
			bytes += size;
		}
	}

	std::string decomposition;

	if (mdl.has("physics_convex"))
	{ //write out convex decomposition
		decomposition = zip_llsd(mdl["physics_convex"]);

		U32 size = decomposition.size();
		if (size > 0)
		{
			header["physics_convex"]["offset"] = (LLSD::Integer) cur_offset;
			header["physics_convex"]["size"] = (LLSD::Integer) size;
			cur_offset += size;
			bytes += size;
		}
	}

	std::string out[MODEL_NAMES_LENGTH];

	for (S32 i = 0; i < MODEL_NAMES_LENGTH; i++)
	{
		if (mdl.has(model_names[i]))
		{
			out[i] = zip_llsd(mdl[model_names[i]]);

			U32 size = out[i].size();

			header[model_names[i]]["offset"] = (LLSD::Integer) cur_offset;
			header[model_names[i]]["size"] = (LLSD::Integer) size;
			cur_offset += size;
			bytes += size;
		}
	}

	if (!nowrite)
	{
		LLSDSerialize::toBinary(header, ostr);

		if (!skin.empty())
		{ //write skin block
			ostr.write((const char*) skin.data(), header["skin"]["size"].asInteger());
		}

		if (!decomposition.empty())
		{ //write decomposition block
			ostr.write((const char*) decomposition.data(), header["physics_convex"]["size"].asInteger());
		}

		for (S32 i = 0; i < MODEL_NAMES_LENGTH; i++)
		{
			if (!out[i].empty())
			{
				ostr.write((const char*) out[i].data(), header[model_names[i]]["size"].asInteger());
			}
		}
	}
	
	return header;
}

LLModel::weight_list& LLModel::getJointInfluences(const LLVector3& pos)
{
	//1. If a vertex has been weighted then we'll find it via pos and return it's weight list
	weight_map::iterator iterPos = mSkinWeights.begin();
	weight_map::iterator iterEnd = mSkinWeights.end();
	
	for ( ; iterPos!=iterEnd; ++iterPos )
	{
		if ( jointPositionalLookup( iterPos->first, pos ) )
		{
			return iterPos->second;
		}
	}
	
	//2. Otherwise we'll use the older implementation
	weight_map::iterator iter = mSkinWeights.find(pos);
	
	if (iter != mSkinWeights.end())
	{
		if ((iter->first - pos).magVec() > 0.1f)
		{
			llerrs << "Couldn't find weight list." << llendl;
		}

		return iter->second;
	}
	else
	{  //no exact match found, get closest point
		const F32 epsilon = 1e-5f;
		weight_map::iterator iter_up = mSkinWeights.lower_bound(pos);
		weight_map::iterator iter_down = ++iter_up;

		weight_map::iterator best = iter_up;

		F32 min_dist = (iter->first - pos).magVec();

		bool done = false;
		while (!done)
		{ //search up and down mSkinWeights from lower bound of pos until a 
		  //match is found within epsilon.  If no match is found within epsilon,
		  //return closest match
			done = true;
			if (iter_up != mSkinWeights.end() && ++iter_up != mSkinWeights.end())
			{
				done = false;
				F32 dist = (iter_up->first - pos).magVec();

				if (dist < epsilon)
				{
					return iter_up->second;
				}

				if (dist < min_dist)
				{
					best = iter_up;
					min_dist = dist;
				}
			}

			if (iter_down != mSkinWeights.begin() && --iter_down != mSkinWeights.begin())
			{
				done = false;

				F32 dist = (iter_down->first - pos).magVec();

				if (dist < epsilon)
				{
					return iter_down->second;
				}

				if (dist < min_dist)
				{
					best = iter_down;
					min_dist = dist;
				}

			}
		}
		
		return best->second;
	}					
}

void LLModel::setConvexHullDecomposition(
	const LLModel::convex_hull_decomposition& decomp)
{
	mPhysics.mHull = decomp;
	mPhysics.mMesh.clear();
	updateHullCenters();
}

void LLModel::updateHullCenters()
{
	mHullCenter.resize(mPhysics.mHull.size());
	mHullPoints = 0;
	mCenterOfHullCenters.clear();

	for (U32 i = 0; i < mPhysics.mHull.size(); ++i)
	{
		LLVector3 cur_center;

		for (U32 j = 0; j < mPhysics.mHull[i].size(); ++j)
		{
			cur_center += mPhysics.mHull[i][j];
		}
		mCenterOfHullCenters += cur_center;
		cur_center *= 1.f/mPhysics.mHull[i].size();
		mHullCenter[i] = cur_center;
		mHullPoints += mPhysics.mHull[i].size();
	}

	if (mHullPoints > 0)
	{
		mCenterOfHullCenters *= 1.f / mHullPoints;
		llassert(mPhysics.hasHullList());
	}
}

bool LLModel::loadModel(std::istream& is)
{
	mSculptLevel = -1;  // default is an error occured

	LLSD header;
	{
		if (!LLSDSerialize::fromBinary(header, is, 1024*1024*1024))
		{
			llwarns << "Mesh header parse error.  Not a valid mesh asset!" << llendl;
			return false;
		}
	}
	
	if (header.has("material_list"))
	{ //load material list names
		mMaterialList.clear();
		for (U32 i = 0; i < header["material_list"].size(); ++i)
		{
			mMaterialList.push_back(header["material_list"][i].asString());
		}
	}

	std::string nm[] = 
	{
		"lowest_lod",
		"low_lod",
		"medium_lod",
		"high_lod",
		"physics_mesh",
	};

	const S32 MODEL_LODS = 5;

	S32 lod = llclamp((S32) mDetail, 0, MODEL_LODS);

	if (header[nm[lod]]["offset"].asInteger() == -1 || 
		header[nm[lod]]["size"].asInteger() == 0 )
	{ //cannot load requested LOD
		llwarns << "LoD data is invalid!" << llendl;
		return false;
	}

	bool has_skin = header["skin"]["offset"].asInteger() >=0 &&
					header["skin"]["size"].asInteger() > 0;

	if (lod == LLModel::LOD_HIGH)
	{ //try to load skin info and decomp info
		std::ios::pos_type cur_pos = is.tellg();
		loadSkinInfo(header, is);
		is.seekg(cur_pos);
	}

	if (lod == LLModel::LOD_HIGH || lod == LLModel::LOD_PHYSICS)
	{
		std::ios::pos_type cur_pos = is.tellg();
		loadDecomposition(header, is);
		is.seekg(cur_pos);
	}

	is.seekg(header[nm[lod]]["offset"].asInteger(), std::ios_base::cur);

	if (unpackVolumeFaces(is, header[nm[lod]]["size"].asInteger()))
	{
		if (has_skin)
		{ 
			//build out mSkinWeight from face info
			for (S32 i = 0; i < getNumVolumeFaces(); ++i)
			{
				const LLVolumeFace& face = getVolumeFace(i);

				if (face.mWeights)
				{
					for (S32 j = 0; j < face.mNumVertices; ++j)
					{
						LLVector4a& w = face.mWeights[j];

						std::vector<JointWeight> wght;

						for (S32 k = 0; k < 4; ++k)
						{
							S32 idx = (S32) w[k];
							F32 f = w[k] - idx;
							if (f > 0.f)
							{
								wght.push_back(JointWeight(idx, f));
							}
						}

						if (!wght.empty())
						{
							LLVector3 pos(face.mPositions[j].getF32ptr());
							mSkinWeights[pos] = wght;
						}
					}
				}
			}
		}
		return true;
	}
	else
	{
		llwarns << "unpackVolumeFaces failed!" << llendl;
	}

	return false;

}

bool LLModel::isMaterialListSubset( LLModel* ref )
{
	int refCnt = ref->mMaterialList.size();
	int modelCnt = mMaterialList.size();
	
	for (U32 src = 0; src < modelCnt; ++src)
	{				
		bool foundRef = false;
		
		for (U32 dst = 0; dst < refCnt; ++dst)
		{
			//llinfos<<mMaterialList[src]<<" "<<ref->mMaterialList[dst]<<llendl;
			foundRef = mMaterialList[src] == ref->mMaterialList[dst];									
				
			if ( foundRef )
			{	
				break;
			}										
		}
		if (!foundRef)
		{
			return false;
		}
	}
	
	return true;
}

bool LLModel::needToAddFaces( LLModel* ref, int& refFaceCnt, int& modelFaceCnt )
{
	bool changed = false;
	if ( refFaceCnt< modelFaceCnt )
	{
		refFaceCnt += modelFaceCnt - refFaceCnt;
		changed = true;
	}
	else 
	if ( modelFaceCnt < refFaceCnt )
	{
		modelFaceCnt += refFaceCnt - modelFaceCnt;
		changed = true;
	}
	
	return changed;
}

bool LLModel::matchMaterialOrder(LLModel* ref, int& refFaceCnt, int& modelFaceCnt )
{
	//Is this a subset?
	//LODs cannot currently add new materials, e.g.
	//1. ref = a,b,c lod1 = d,e => This is not permitted
	//2. ref = a,b,c lod1 = c => This would be permitted
	
	bool isASubset = isMaterialListSubset( ref );
	if ( !isASubset )
	{
		llinfos<<"Material of model is not a subset of reference."<<llendl;
		return false;
	}
	
	std::map<std::string, U32> index_map;
	
	//build a map of material slot names to face indexes
	bool reorder = false;

	std::set<std::string> base_mat;
	std::set<std::string> cur_mat;

	for (U32 i = 0; i < mMaterialList.size(); i++)
	{
		index_map[ref->mMaterialList[i]] = i;
		if (!reorder)
		{ //if any material name does not match reference, we need to reorder
			reorder = ref->mMaterialList[i] != mMaterialList[i];
		}
		base_mat.insert(ref->mMaterialList[i]);
		cur_mat.insert(mMaterialList[i]);
	}


	if (reorder && 
		base_mat == cur_mat) //don't reorder if material name sets don't match
	{
		std::vector<LLVolumeFace> new_face_list;
		new_face_list.resize(mVolumeFaces.size());

		std::vector<std::string> new_material_list;
		new_material_list.resize(mVolumeFaces.size());

		//rebuild face list so materials have the same order 
		//as the reference model
		for (U32 i = 0; i < mMaterialList.size(); ++i)
		{ 
			U32 ref_idx = index_map[mMaterialList[i]];
			new_face_list[ref_idx] = mVolumeFaces[i];

			new_material_list[ref_idx] = mMaterialList[i];
		}

		llassert(new_material_list == ref->mMaterialList);
		
		mVolumeFaces = new_face_list;
	}

	//override material list with reference model ordering
	mMaterialList = ref->mMaterialList;
	return true;
}


bool LLModel::loadSkinInfo(LLSD& header, std::istream &is)
{
	S32 offset = header["skin"]["offset"].asInteger();
	S32 size = header["skin"]["size"].asInteger();

	if (offset >= 0 && size > 0)
	{
		is.seekg(offset, std::ios_base::cur);

		LLSD skin_data;

		if (unzip_llsd(skin_data, is, size))
		{
			mSkinInfo.fromLLSD(skin_data);
			return true;
		}
	}

	return false;
}

bool LLModel::loadDecomposition(LLSD& header, std::istream& is)
{
	S32 offset = header["physics_convex"]["offset"].asInteger();
	S32 size = header["physics_convex"]["size"].asInteger();

	if (offset >= 0 && size > 0)
	{
		is.seekg(offset, std::ios_base::cur);

		LLSD data;

		if (unzip_llsd(data, is, size))
		{
			mPhysics.fromLLSD(data);
			updateHullCenters();
		}
	}

	return true;
}


LLMeshSkinInfo::LLMeshSkinInfo(LLSD& skin)
{
	fromLLSD(skin);
}

void LLMeshSkinInfo::fromLLSD(LLSD& skin)
{
	if (skin.has("joint_names"))
	{
		for (U32 i = 0; i < skin["joint_names"].size(); ++i)
		{
			mJointNames.push_back(skin["joint_names"][i]);
		}
	}

	if (skin.has("inverse_bind_matrix"))
	{
		for (U32 i = 0; i < skin["inverse_bind_matrix"].size(); ++i)
		{
			LLMatrix4 mat;
			for (U32 j = 0; j < 4; j++)
			{
				for (U32 k = 0; k < 4; k++)
				{
					mat.mMatrix[j][k] = skin["inverse_bind_matrix"][i][j*4+k].asReal();
				}
			}

			mInvBindMatrix.push_back(mat);
		}
	}

	if (skin.has("bind_shape_matrix"))
	{
		for (U32 j = 0; j < 4; j++)
		{
			for (U32 k = 0; k < 4; k++)
			{
				mBindShapeMatrix.mMatrix[j][k] = skin["bind_shape_matrix"][j*4+k].asReal();
			}
		}
	}

	if (skin.has("alt_inverse_bind_matrix"))
	{
		for (U32 i = 0; i < skin["alt_inverse_bind_matrix"].size(); ++i)
		{
			LLMatrix4 mat;
			for (U32 j = 0; j < 4; j++)
			{
				for (U32 k = 0; k < 4; k++)
				{
					mat.mMatrix[j][k] = skin["alt_inverse_bind_matrix"][i][j*4+k].asReal();
				}
			}
			
			mAlternateBindMatrix.push_back(mat);
		}
	}

	if (skin.has("pelvis_offset"))
	{
		mPelvisOffset = skin["pelvis_offset"].asReal();
	}
}

LLSD LLMeshSkinInfo::asLLSD(bool include_joints) const
{
	LLSD ret;

	for (U32 i = 0; i < mJointNames.size(); ++i)
	{
		ret["joint_names"][i] = mJointNames[i];

		for (U32 j = 0; j < 4; j++)
		{
			for (U32 k = 0; k < 4; k++)
			{
				ret["inverse_bind_matrix"][i][j*4+k] = mInvBindMatrix[i].mMatrix[j][k]; 
			}
		}
	}

	for (U32 i = 0; i < 4; i++)
	{
		for (U32 j = 0; j < 4; j++)
		{
			ret["bind_shape_matrix"][i*4+j] = mBindShapeMatrix.mMatrix[i][j];
		}
	}
		
	if ( include_joints && mAlternateBindMatrix.size() > 0 )
	{
		for (U32 i = 0; i < mJointNames.size(); ++i)
		{
			for (U32 j = 0; j < 4; j++)
			{
				for (U32 k = 0; k < 4; k++)
				{
					ret["alt_inverse_bind_matrix"][i][j*4+k] = mAlternateBindMatrix[i].mMatrix[j][k]; 
				}
			}
		}

		ret["pelvis_offset"] = mPelvisOffset;
	}

	return ret;
}

LLModel::Decomposition::Decomposition(LLSD& data)
{
	fromLLSD(data);
}

void LLModel::Decomposition::fromLLSD(LLSD& decomp)
{
	if (decomp.has("HullList") && decomp.has("Positions"))
	{
		// updated for const-correctness. gcc is picky about this type of thing - Nyx
		const LLSD::Binary& hulls = decomp["HullList"].asBinary();
		const LLSD::Binary& position = decomp["Positions"].asBinary();

		U16* p = (U16*) &position[0];

		mHull.resize(hulls.size());

		LLVector3 min;
		LLVector3 max;
		LLVector3 range;

		if (decomp.has("Min"))
		{
			min.setValue(decomp["Min"]);
			max.setValue(decomp["Max"]);
		}
		else
		{
			min.set(-0.5f, -0.5f, -0.5f);
			max.set(0.5f, 0.5f, 0.5f);
		}

		range = max-min;

		for (U32 i = 0; i < hulls.size(); ++i)
		{
			U16 count = (hulls[i] == 0) ? 256 : hulls[i];
			
			std::set<U64> valid;

			//must have at least 4 points
			//llassert(count > 3);

			for (U32 j = 0; j < count; ++j)
			{
				U64 test = (U64) p[0] | ((U64) p[1] << 16) | ((U64) p[2] << 32);
				//point must be unique
				//llassert(valid.find(test) == valid.end());
				valid.insert(test);

				mHull[i].push_back(LLVector3(
					(F32) p[0]/65535.f*range.mV[0]+min.mV[0],
					(F32) p[1]/65535.f*range.mV[1]+min.mV[1],
					(F32) p[2]/65535.f*range.mV[2]+min.mV[2]));
				p += 3;


			}

			//each hull must contain at least 4 unique points
			//llassert(valid.size() > 3);
		}
	}

	if (decomp.has("BoundingVerts"))
	{
		const LLSD::Binary& position = decomp["BoundingVerts"].asBinary();

		U16* p = (U16*) &position[0];

		LLVector3 min;
		LLVector3 max;
		LLVector3 range;

		if (decomp.has("Min"))
		{
			min.setValue(decomp["Min"]);
			max.setValue(decomp["Max"]);
		}
		else
		{
			min.set(-0.5f, -0.5f, -0.5f);
			max.set(0.5f, 0.5f, 0.5f);
		}

		range = max-min;

		U16 count = position.size()/6;
		
		for (U32 j = 0; j < count; ++j)
		{
			mBaseHull.push_back(LLVector3(
				(F32) p[0]/65535.f*range.mV[0]+min.mV[0],
				(F32) p[1]/65535.f*range.mV[1]+min.mV[1],
				(F32) p[2]/65535.f*range.mV[2]+min.mV[2]));
			p += 3;
		}		 
	}
	else
	{
		//empty base hull mesh to indicate decomposition has been loaded
		//but contains no base hull
		mBaseHullMesh.clear();
	}
}

bool LLModel::Decomposition::hasHullList() const
{
	return !mHull.empty() ;
}

LLSD LLModel::Decomposition::asLLSD() const
{
	LLSD ret;
	
	if (mBaseHull.empty() && mHull.empty())
	{ //nothing to write
		return ret;
	}

	//write decomposition block
	// ["physics_convex"]["HullList"] -- list of 8 bit integers, each entry represents a hull with specified number of points
	// ["physics_convex"]["Position"] -- list of 16-bit integers to be decoded to given domain, encoded 3D points
	// ["physics_convex"]["BoundingVerts"] -- list of 16-bit integers to be decoded to given domain, encoded 3D points representing a single hull approximation of given shape
	
	//get minimum and maximum
	LLVector3 min;
	
	if (mHull.empty())
	{  
		min = mBaseHull[0];
	}
	else
	{
		min = mHull[0][0];
	}

	LLVector3 max = min;

	LLSD::Binary hulls(mHull.size());

	U32 total = 0;

	for (U32 i = 0; i < mHull.size(); ++i)
	{
		U32 size = mHull[i].size();
		total += size;
		hulls[i] = (U8) (size);

		for (U32 j = 0; j < mHull[i].size(); ++j)
		{
			update_min_max(min, max, mHull[i][j]);
		}
	}

	for (U32 i = 0; i < mBaseHull.size(); ++i)
	{
		update_min_max(min, max, mBaseHull[i]);	
	}

	ret["Min"] = min.getValue();
	ret["Max"] = max.getValue();

	LLVector3 range = max-min;

	if (!hulls.empty())
	{
		ret["HullList"] = hulls;
	}

	if (total > 0)
	{
		LLSD::Binary p(total*3*2);

		U32 vert_idx = 0;
		
		for (U32 i = 0; i < mHull.size(); ++i)
		{
			std::set<U64> valid;

			llassert(!mHull[i].empty());

			for (U32 j = 0; j < mHull[i].size(); ++j)
			{
				U64 test = 0;
				const F32* src = mHull[i][j].mV;

				for (U32 k = 0; k < 3; k++)
				{
					//convert to 16-bit normalized across domain
					U16 val = (U16) (((src[k]-min.mV[k])/range.mV[k])*65535);

					if(valid.size() < 3)
					{
						switch (k)
						{
							case 0: test = test | (U64) val; break;
							case 1: test = test | ((U64) val << 16); break;
							case 2: test = test | ((U64) val << 32); break;
						};

						valid.insert(test);
					}
					
					U8* buff = (U8*) &val;
					//write to binary buffer
					p[vert_idx++] = buff[0];
					p[vert_idx++] = buff[1];

					//makes sure we haven't run off the end of the array
					llassert(vert_idx <= p.size());
				}
			}

			//must have at least 3 unique points
			llassert(valid.size() > 2);
		}

		ret["Positions"] = p;
	}

	//llassert(!mBaseHull.empty());

	if (!mBaseHull.empty())
	{
		LLSD::Binary p(mBaseHull.size()*3*2);

		U32 vert_idx = 0;
		for (U32 j = 0; j < mBaseHull.size(); ++j)
		{
			const F32* v = mBaseHull[j].mV;

			for (U32 k = 0; k < 3; k++)
			{
				//convert to 16-bit normalized across domain
				U16 val = (U16) (((v[k]-min.mV[k])/range.mV[k])*65535);

				U8* buff = (U8*) &val;
				//write to binary buffer
				p[vert_idx++] = buff[0];
				p[vert_idx++] = buff[1];

				if (vert_idx > p.size())
				{
					llerrs << "Index out of bounds" << llendl;
				}
			}
		}
		
		ret["BoundingVerts"] = p;
	}

	return ret;
}

void LLModel::Decomposition::merge(const LLModel::Decomposition* rhs)
{
	if (!rhs)
	{
		return;
	}

	if (mMeshID != rhs->mMeshID)
	{
		llerrs << "Attempted to merge with decomposition of some other mesh." << llendl;
	}

	if (mBaseHull.empty())
	{ //take base hull and decomposition from rhs
		mHull = rhs->mHull;
		mBaseHull = rhs->mBaseHull;
		mMesh = rhs->mMesh;
		mBaseHullMesh = rhs->mBaseHullMesh;
	}

	if (mPhysicsShapeMesh.empty())
	{ //take physics shape mesh from rhs
		mPhysicsShapeMesh = rhs->mPhysicsShapeMesh;
	}
}

