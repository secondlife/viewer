/**
 * @file llobjectcostmanager.cpp
 * @brief Classes for managing object-related costs (rendering, streaming, etc) across multiple versions.
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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
#include "llobjectcostmanager.h"
#include "llviewerobject.h"
#include "llvovolume.h"
#include "lldrawable.h"
#include "lldrawpool.h"
#include "llface.h"
#include "llpartdata.h"
#include "llviewerpartsource.h"
#include "llmeshrepository.h"
#include "llsd.h"

#include <set>

#define VALIDATE_COSTS

typedef std::set<LLUUID> texture_ids_t;

#pragma optimize("", off)

class LLObjectCostData
{
public:
	LLObjectCostData();
	~LLObjectCostData();

	void asLLSD( LLSD& sd ) const;

	// Face counts
	U32 m_alpha_mask_faces;
	U32 m_alpha_faces;
	U32 m_animtex_faces;
	U32 m_bumpmap_faces;
	U32 m_bump_any_faces;
	U32 m_flexi_vols;
	U32 m_full_bright_faces;
	U32 m_glow_faces;
	U32 m_invisi_faces;
	U32 m_materials_faces;
	U32 m_media_faces;
	U32 m_planar_faces;
	U32 m_shiny_faces;
	U32 m_shiny_any_faces;

	U32 m_normalmap_faces;
    U32 m_specmap_faces;

	// Volume counts
	U32 m_materials_vols;
	U32 m_mesh_vols;
	U32 m_weighted_mesh_vols;
	U32 m_particle_source_vols;
	U32 m_produces_light_vols;
	U32 m_sculpt_vols;

	// Other stats
	U32 m_num_particles;
	F32 m_part_size;
	U32 m_num_triangles_v1;
	bool m_is_animated_object;
	bool m_is_root_edit;

	// Texture ids
	texture_ids_t m_diffuse_ids;
	texture_ids_t m_normal_ids;
	texture_ids_t m_specular_ids;
	texture_ids_t m_sculpt_ids;
};

LLObjectCostData::LLObjectCostData():
	// Face counts
	m_alpha_mask_faces(0),
	m_alpha_faces(0),
	m_animtex_faces(0),
	m_bumpmap_faces(0),
	m_bump_any_faces(0),
	m_flexi_vols(0),
	m_full_bright_faces(0),
	m_glow_faces(0),
	m_invisi_faces(0),
	m_materials_faces(0),
	m_media_faces(0),
	m_planar_faces(0),
	m_shiny_faces(0),
	m_shiny_any_faces(0),
	m_normalmap_faces(0),
    m_specmap_faces(0),

	// Volume counts
	m_materials_vols(0),
	m_mesh_vols(0),
	m_weighted_mesh_vols(0),
	m_particle_source_vols(0),
	m_produces_light_vols(0),
	m_sculpt_vols(0),

	// Other stats
	m_num_particles(0),
	m_part_size(0.f),
	m_num_triangles_v1(0),
	m_is_animated_object(false),
	m_is_root_edit(false)
{
}
	
LLObjectCostData::~LLObjectCostData()
{
}

class LLObjectCostManagerImpl
{
  public:
	LLObjectCostManagerImpl() {}
	~LLObjectCostManagerImpl() {}

	F32 getStreamingCost(U32 version, const LLVOVolume *vol);
	F32 getStreamingCostV1(const LLVOVolume *vol);

	F32 getRenderCost(U32 version, const LLVOVolume *vol);
	F32 getRenderCostV1(const LLVOVolume *vol);

	F32 getRenderCostLinkset(U32 version, const LLViewerObject *root);
	F32 getRenderCostLinksetV1(const LLViewerObject *root);

	// Accumulate data for a single prim.
	void getObjectCostData(const LLVOVolume *vol, LLObjectCostData& cost_data);

	U32 textureCostsV1(const texture_ids_t& ids);
	F32 triangleCostsV1(LLObjectCostData& cost_data); 

};

F32 LLObjectCostManagerImpl::getRenderCostLinkset(U32 version, const LLViewerObject *root)
{
	F32 render_cost = 0.f;

	if (version == 0)
	{
		version = LLObjectCostManager::instance().getCurrentCostVersion();
	}
	if (version == 1)
	{
		render_cost = getRenderCostLinksetV1(root);
#ifdef VALIDATE_COSTS
		// FIXME ARC need to rework legacy code to make comparison possible
#endif
	}
	else
	{
		LL_WARNS("Arctan") << "Unrecognized version " << version << LL_ENDL;
	}

	return render_cost;
}

void get_volumes_for_linkset(const LLViewerObject *root, std::vector<const LLVOVolume*>& volumes)
{
	const LLVOVolume *root_vol = dynamic_cast<const LLVOVolume*>(root);
	if (root_vol)	
	{
		volumes.push_back(root_vol);
		
		LLViewerObject::const_child_list_t children = root_vol->getChildren();
		for (LLViewerObject::const_child_list_t::const_iterator child_iter = children.begin();
			 child_iter != children.end();
			 ++child_iter)
		{
			LLViewerObject* child_obj = *child_iter;
			LLVOVolume *child_vol = dynamic_cast<LLVOVolume*>( child_obj );
			if (child_vol)
			{
				volumes.push_back(child_vol);
			}
		}
	}
}

F32 LLObjectCostManagerImpl::getRenderCostLinksetV1(const LLViewerObject *root)
{
	F32 shame = 0.f;
	texture_ids_t all_sculpt_ids, all_diffuse_ids;

	std::vector<const LLVOVolume*> volumes;
	get_volumes_for_linkset(root,volumes);

	for (std::vector<const LLVOVolume*>::const_iterator it = volumes.begin();
		 it != volumes.end(); ++it)
	{
		const LLVOVolume *vol = *it;

		LLObjectCostData cost_data;
		getObjectCostData(vol, cost_data);

		// Charge for effective triangles
		shame += triangleCostsV1(cost_data);

		// Accumulate texture ids
		all_sculpt_ids.insert(cost_data.m_sculpt_ids.begin(), cost_data.m_sculpt_ids.end());
		all_diffuse_ids.insert(cost_data.m_diffuse_ids.begin(), cost_data.m_diffuse_ids.end());
	}
	
	// Material textures not included in V1 costs
	shame += textureCostsV1(all_sculpt_ids);
	shame += textureCostsV1(all_diffuse_ids);
	
	return shame;
}

F32 LLObjectCostManagerImpl::getStreamingCost(U32 version, const LLVOVolume *vol)
{
	F32 streaming_cost = -1.f;
	
	if (version == 0)
	{
		version = LLObjectCostManager::instance().getCurrentCostVersion();
	}
	if (version == 1)
	{
		streaming_cost = getStreamingCostV1(vol);

#ifdef VALIDATE_COSTS
		F32 streaming_cost_legacy = vol->getStreamingCostLegacy();
		if (streaming_cost != streaming_cost_legacy)
		{
			LL_WARNS("Arctan") << "streaming cost mismatch " << streaming_cost << ", " << streaming_cost_legacy << LL_ENDL;
		}
#endif
	}
	else
	{
		LL_WARNS("Arctan") << "Unrecognized version " << version << LL_ENDL;
	}

	return streaming_cost;
};

F32 LLObjectCostManagerImpl::getStreamingCostV1(const LLVOVolume *vol)
{
	F32 radius = vol->getScale().length()*0.5f;
	F32 linkset_base_cost = 0.f;

	LLMeshCostData costs;
	if (vol->getMeshCostData(costs))
	{
		if (vol->isAnimatedObject() && vol->isRootEdit())
		{
			// Root object of an animated object has this to account for skeleton overhead.
			linkset_base_cost = ANIMATED_OBJECT_BASE_COST;
		}
		if (vol->isMesh())
		{
			if (vol->isAnimatedObject() && vol->isRiggedMesh())
			{
				return linkset_base_cost + costs.getTriangleBasedStreamingCost();
			}
			else
			{
				return linkset_base_cost + costs.getRadiusBasedStreamingCost(radius);
			}
		}
		else
		{
			return linkset_base_cost + costs.getRadiusBasedStreamingCost(radius);
		}
	}
	else
	{
		return 0.f;
	}
}

F32 LLObjectCostManagerImpl::getRenderCost(U32 version, const LLVOVolume *vol)
{
	F32 render_cost = -1.f;
	
	if (version == 0)
	{
		version = LLObjectCostManager::instance().getCurrentCostVersion();
	}
	if (version == 1)
	{

		F32 render_cost = getRenderCostV1(vol);

#ifdef VALIDATE_COSTS
		LLVOVolume::texture_cost_t textures, material_textures;
		U32 render_cost_legacy = vol->getRenderCostLegacy(textures, material_textures);
		if ((U32) render_cost != render_cost_legacy)
		{
			LL_WARNS("Arctan") << "render cost mismatch " << render_cost << ", " << render_cost_legacy << LL_ENDL;
		}
#endif
	}
	else
	{
		LL_WARNS("Arctan") << "Unrecognized version " << version << LL_ENDL;
	}

	return render_cost;
};

F32 LLObjectCostManagerImpl::getRenderCostV1(const LLVOVolume *vol)
{
	LLObjectCostData cost_data;
	getObjectCostData(vol, cost_data);

	// Charge for effective triangles
	F32 shame = triangleCostsV1(cost_data);

	// Material textures not included in V1 costs
	shame += textureCostsV1(cost_data.m_sculpt_ids);
	shame += textureCostsV1(cost_data.m_diffuse_ids);
	
	return shame;
}

// Accumulate data for a single prim.
void LLObjectCostManagerImpl::getObjectCostData(const LLVOVolume *vol, LLObjectCostData& cost_data)
{
	static const U32 ARC_PARTICLE_MAX = 2048; // default values

	const LLDrawable* drawablep = vol->mDrawable;
	U32 num_faces = 0;
	if (drawablep)
	{
		num_faces = drawablep->getNumFaces();
	}

	bool has_volume = (vol->getVolume() != NULL);

    // Get access to params we'll need at various points.  
	// Skip if this is object doesn't have a volume (e.g. is an avatar).
	LLVolumeParams volume_params;
	LLPathParams path_params;
	LLProfileParams profile_params;

	cost_data.m_num_triangles_v1 = 0;
	
	if (has_volume)
	{
		volume_params = vol->getVolume()->getParams();
		path_params = volume_params.getPathParams();
		profile_params = volume_params.getProfileParams();

        LLMeshCostData costs;
		if (vol->getMeshCostData(costs))
		{
            if (vol->isAnimatedObject() && vol->isRiggedMesh())
            {
                // Scaling here is to make animated object vs
                // non-animated object ARC proportional to the
                // corresponding calculations for streaming cost.
                cost_data.m_num_triangles_v1 = (ANIMATED_OBJECT_COST_PER_KTRI * 0.001 * costs.getEstTrisForStreamingCost())/0.06;
            }
            else
            {
                F32 radius = vol->getScale().length()*0.5f;
                cost_data.m_num_triangles_v1 = costs.getRadiusWeightedTris(radius);
            }
		}

		cost_data.m_is_animated_object = vol->isAnimatedObject();
		cost_data.m_is_root_edit = vol->isRootEdit();
	}

	if (cost_data.m_num_triangles_v1 <= 0)
	{
		cost_data.m_num_triangles_v1 = 4;
	}

	if (vol->isSculpted())
	{
		if (vol->isMesh())
		{
			cost_data.m_mesh_vols++;
			if (vol->isRiggedMesh())
			{
				cost_data.m_weighted_mesh_vols++;
			}
		}
		else
		{
			// Actual sculpty, capture its id
			const LLSculptParams *sculpt_params = (LLSculptParams *) vol->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
			LLUUID sculpt_id = sculpt_params->getSculptTexture();
			cost_data.m_sculpt_ids.insert(sculpt_id);
			cost_data.m_sculpt_vols++;
		}
	}

	if (vol->isFlexible())
	{
		cost_data.m_flexi_vols++;
	}


	if (vol->isParticleSource())
	{
		cost_data.m_particle_source_vols++;
	}

	if (vol->getIsLight())
	{
		cost_data.m_produces_light_vols++;
	}

	U32 materials_faces_this_vol = 0;

	for (S32 i = 0; i < num_faces; ++i)
	{
		const LLFace* face = drawablep->getFace(i);
		if (!face) continue;
		const LLTextureEntry* te = face->getTextureEntry();
		const LLViewerTexture* img = face->getTexture();

        LLMaterial* mat = NULL;
        if (te)
        {
            mat = te->getMaterialParams();
        }
        if (mat)
        {
			materials_faces_this_vol++;
            if (mat->getNormalID().notNull())
            {
                cost_data.m_normalmap_faces++;
                LLUUID normal_id = mat->getNormalID();
				cost_data.m_normal_ids.insert(normal_id);
            }
            if (mat->getSpecularID().notNull())
            {
                cost_data.m_specmap_faces++;
                LLUUID spec_id = mat->getSpecularID();
				cost_data.m_specular_ids.insert(spec_id);
            }
        }
		if (img)
		{
			cost_data.m_diffuse_ids.insert(img->getID());
		}

		if (face->getPoolType() == LLDrawPool::POOL_ALPHA)
		{
			cost_data.m_alpha_faces++;
		}
        else if ((face->getPoolType() == LLDrawPool::POOL_ALPHA_MASK) || (face->getPoolType() == LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK))
        {
            cost_data.m_alpha_mask_faces++;
        }
        else if (face->getPoolType() == LLDrawPool::POOL_FULLBRIGHT)
        {
			// FIXME ARC: this only gets hit for full bright faces
			// with some graphics quality settings (e.g. on
			// low). Doesn't make sense since render cost should only
			// be a function of the content.
            cost_data.m_full_bright_faces++;
        }
		else if (img && img->getPrimaryFormat() == GL_ALPHA)
		{
			// ARC FIXME what is this really measuring?
			cost_data.m_invisi_faces = 1;
		}
		if (face->hasMedia())
		{
			cost_data.m_media_faces++;
		}

		if (te)
		{
            if (te->getBumpmap())
			{
				cost_data.m_bumpmap_faces++;
			}
            if (te->getBumpmap() || te->getBumpShiny() || te->getBumpShinyFullbright())
			{
				cost_data.m_bump_any_faces++;
			}
            if (te->getShiny())
			{
				cost_data.m_shiny_faces++;
			}
            if (te->getShiny() || te->getBumpShiny() || te->getBumpShinyFullbright())
			{
				cost_data.m_shiny_any_faces++;
			}
            if (te->getFullbright() || te->getBumpShinyFullbright())
            {
                cost_data.m_full_bright_faces++;
            }
			if (te->getGlow() > 0.f)
			{
				cost_data.m_glow_faces++;
			}
			if (face->mTextureMatrix != NULL)
			{
				cost_data.m_animtex_faces++;
			}
			if (te->getTexGen())
			{
				cost_data.m_planar_faces++;
			}
		}
	}

	if (materials_faces_this_vol > 0)
	{
		cost_data.m_materials_vols++;
		cost_data.m_materials_faces += materials_faces_this_vol;
	}

	if (vol->isParticleSource())
	{
		const LLPartSysData *part_sys_data = &(vol->mPartSourcep->mPartSysData);
		const LLPartData *part_data = &(part_sys_data->mPartData);
		U32 num_particles = (U32)(part_sys_data->mBurstPartCount * llceil( part_data->mMaxAge / part_sys_data->mBurstRate));
		num_particles = num_particles > ARC_PARTICLE_MAX ? ARC_PARTICLE_MAX : num_particles;
		F32 part_size = (llmax(part_data->mStartScale[0], part_data->mEndScale[0]) + llmax(part_data->mStartScale[1], part_data->mEndScale[1])) / 2.f;

		cost_data.m_num_particles += num_particles;
		cost_data.m_part_size += part_size;
		// ARC - how do we use this info? how do we aggregate it across multiple prims?
	}


}

U32 LLObjectCostManagerImpl::textureCostsV1(const texture_ids_t& ids)
{
	U32 cost = 0;

    static const U32 ARC_TEXTURE_COST    = 16; // multiplier for texture resolution - performance tested

	// FIXME ARC Media faces do not give the right dimensions. Old
	// code uses face texture directly, right value. Here we look up
	// the corresponding fetched texture, doesn't work (get 0x0 texture).
	for (texture_ids_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		const LLUUID& id = *it;
		LLViewerFetchedTexture *texture = LLViewerTextureManager::getFetchedTexture(id);
		U32 texture_cost = 0;
		if (texture)
		{
			texture_cost = 256 + (S32)(ARC_TEXTURE_COST * (texture->getFullHeight() / 128.f + texture->getFullWidth() / 128.f));
		}
		else
		{
			texture_cost = 1;
		}
		LL_DEBUGS("ARCdetail") << "texture " << id << " cost " << texture_cost << LL_ENDL;
		cost += texture_cost;
	}

	return cost;
}

F32 LLObjectCostManagerImpl::triangleCostsV1(LLObjectCostData& cost_data)
{
    /*****************************************************************
     * This calculation should not be modified by third party viewers,
     * since it is used to limit rendering and should be uniform for
     * everyone. If you have suggested improvements, submit them to
     * the official viewer for consideration.
     *****************************************************************/

	// per-prim costs
	static const U32 ARC_PARTICLE_COST = 1; // determined experimentally
	//static const U32 ARC_PARTICLE_MAX = 2048; // default values
	//static const U32 ARC_TEXTURE_COST = 16; // multiplier for texture resolution - performance tested
	static const U32 ARC_LIGHT_COST = 500; // static cost for light-producing prims 
	static const U32 ARC_MEDIA_FACE_COST = 1500; // static cost per media-enabled face 


	// per-prim multipliers
	static const F32 ARC_GLOW_MULT = 1.5f; // tested based on performance
	static const F32 ARC_BUMP_MULT = 1.25f; // tested based on performance
	static const F32 ARC_FLEXI_MULT = 5; // tested based on performance
	static const F32 ARC_SHINY_MULT = 1.6f; // tested based on performance
	static const F32 ARC_INVISI_COST = 1.2f; // tested based on performance
	static const F32 ARC_WEIGHTED_MESH = 1.2f; // tested based on performance

	static const F32 ARC_PLANAR_COST = 1.0f; // tested based on performance to have negligible impact
	static const F32 ARC_ANIM_TEX_COST = 4.f; // tested based on performance
	static const F32 ARC_ALPHA_COST = 4.f; // 4x max - based on performance

	F32 shame = 0;

	U32 invisi = (cost_data.m_invisi_faces > 0);
	U32 shiny = (cost_data.m_shiny_faces > 0);
	U32 glow = (cost_data.m_glow_faces > 0);
	U32 alpha = (cost_data.m_alpha_faces > 0);
	U32 flexi = (cost_data.m_flexi_vols > 0);
	U32 animtex = (cost_data.m_animtex_faces > 0);
	U32 particles = (cost_data.m_particle_source_vols > 0);
	U32 bump = (cost_data.m_bumpmap_faces > 0);
	U32 planar = (cost_data.m_planar_faces > 0);
	U32 weighted_mesh = (cost_data.m_weighted_mesh_vols > 0);
	U32 produces_light = (cost_data.m_produces_light_vols > 0);
	U32 media_faces = cost_data.m_media_faces;

	U32 num_triangles = cost_data.m_num_triangles_v1;
	
	// shame currently has the "base" cost of 1 point per 15 triangles, min 2.
	shame = num_triangles  * 5.f;
	shame = shame < 2.f ? 2.f : shame;

	// multiply by per-face modifiers
	if (planar)
	{
		shame *= planar * ARC_PLANAR_COST;
	}

	if (animtex)
	{
		shame *= animtex * ARC_ANIM_TEX_COST;
	}

	if (alpha)
	{
		shame *= alpha * ARC_ALPHA_COST;
	}

	if(invisi)
	{
		shame *= invisi * ARC_INVISI_COST;
	}

	if (glow)
	{
		shame *= glow * ARC_GLOW_MULT;
	}

	if (bump)
	{
		shame *= bump * ARC_BUMP_MULT;
	}

	if (shiny)
	{
		shame *= shiny * ARC_SHINY_MULT;
	}


	// multiply shame by multipliers
	if (weighted_mesh)
	{
		shame *= weighted_mesh * ARC_WEIGHTED_MESH;
	}

	if (flexi)
	{
		shame *= flexi * ARC_FLEXI_MULT;
	}


	// add additional costs
	if (particles)
	{
		shame += cost_data.m_num_particles * cost_data.m_part_size * ARC_PARTICLE_COST;
	}

	if (produces_light)
	{
		shame += ARC_LIGHT_COST;
	}

	if (media_faces)
	{
		shame += media_faces * ARC_MEDIA_FACE_COST;
	}

    // Streaming cost for animated objects includes a fixed cost
    // per linkset. Add a corresponding charge here translated into
    // triangles, but not weighted by any graphics properties.
    if (cost_data.m_is_animated_object && cost_data.m_is_root_edit) 
    {
        shame += (ANIMATED_OBJECT_BASE_COST/0.06) * 5.0f;
    }

	return shame;
}
		   
LLObjectCostManager::LLObjectCostManager()
{
	m_impl = new LLObjectCostManagerImpl;
}

LLObjectCostManager::~LLObjectCostManager()
{
	delete(m_impl);
	m_impl = NULL;
}

U32 LLObjectCostManager::getCurrentCostVersion()
{
	return 1;
}

F32 LLObjectCostManager::getStreamingCost(U32 version, const LLVOVolume *vol)
{
	F32 streaming_cost = m_impl->getStreamingCost(version, vol);
	return streaming_cost;
}

F32 LLObjectCostManager::getRenderCost(U32 version, const LLVOVolume *vol)
{
	F32 render_cost = m_impl->getRenderCost(version, vol);
	return render_cost;
}

F32 LLObjectCostManager::getRenderCostLinkset(U32 version, const LLViewerObject *root)
{
	F32 render_cost = 0.f;
	if (!root->isRootEdit())
	{
		LL_WARNS("Arctan") << "called with non-root object" << LL_ENDL;
	}
	else
	{
		render_cost = m_impl->getRenderCostLinkset(version, root);
	}
	return render_cost;
}
