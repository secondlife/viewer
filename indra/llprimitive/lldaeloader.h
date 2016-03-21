/**
 * @file lldaeloader.h
 * @brief LLDAELoader class definition
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

#ifndef LL_LLDAELOADER_H
#define LL_LLDAELOADER_H

#include "llmodelloader.h"

class DAE;
class daeElement;
class domProfile_COMMON;
class domInstance_geometry;
class domNode;
class domTranslate;
class domController;
class domSkin;
class domMesh;

class LLDAELoader : public LLModelLoader
{
public:
	typedef std::map<std::string, LLImportMaterial>							material_map;
	typedef std::map<daeElement*, std::vector<LLPointer<LLModel> > >	dae_model_map;
	dae_model_map	mModelsMap;

	LLDAELoader(
		std::string							filename,
		S32									lod, 
		LLModelLoader::load_callback_t		load_cb,
		LLModelLoader::joint_lookup_func_t	joint_lookup_func,
		LLModelLoader::texture_load_func_t	texture_load_func,
		LLModelLoader::state_callback_t		state_cb,
		void*								opaque_userdata,
		JointTransformMap&					jointTransformMap,
		JointNameSet&						jointsFromNodes,
        std::map<std::string, std::string>& jointAliasMap,
        U32									maxJointsPerMesh,
		U32									modelLimit,
        bool								preprocess);
	virtual ~LLDAELoader() ;

	virtual bool OpenFile(const std::string& filename);

protected:

	void processElement(daeElement* element, bool& badElement, DAE* dae);
	void processDomModel(LLModel* model, DAE* dae, daeElement* pRoot, domMesh* mesh, domSkin* skin);

	material_map getMaterials(LLModel* model, domInstance_geometry* instance_geo, DAE* dae);
	LLImportMaterial profileToMaterial(domProfile_COMMON* material, DAE* dae);	
	LLColor4 getDaeColor(daeElement* element);
	
	daeElement* getChildFromElement( daeElement* pElement, std::string const & name );
	
	bool isNodeAJoint( domNode* pNode );
	void processJointNode( domNode* pNode, std::map<std::string,LLMatrix4>& jointTransforms );
	void extractTranslation( domTranslate* pTranslate, LLMatrix4& transform );
	void extractTranslationViaElement( daeElement* pTranslateElement, LLMatrix4& transform );
	void extractTranslationViaSID( daeElement* pElement, LLMatrix4& transform );
	void buildJointToNodeMappingFromScene( daeElement* pRoot );
	void processJointToNodeMapping( domNode* pNode );
	void processChildJoints( domNode* pParentNode );

	bool verifyCount( int expected, int result );

	//Verify that a controller matches vertex counts
	bool verifyController( domController* pController );

	static bool addVolumeFacesFromDomMesh(LLModel* model, domMesh* mesh);
	static bool createVolumeFacesFromDomMesh(LLModel* model, domMesh *mesh);

	static LLModel* loadModelFromDomMesh(domMesh* mesh);

	// Loads a mesh breaking it into one or more models as necessary
	// to get around volume face limitations while retaining >8 materials
	//
	bool loadModelsFromDomMesh(domMesh* mesh, std::vector<LLModel*>& models_out, U32 submodel_limit);

	static std::string getElementLabel(daeElement *element);
	static size_t getSuffixPosition(std::string label);
	static std::string getLodlessLabel(daeElement *element);

	static std::string preprocessDAE(std::string filename);

private:
	U32 mGeneratedModelLimit; // Attempt to limit amount of generated submodels
	bool mPreprocessDAE;

};
#endif  // LL_LLDAELLOADER_H
