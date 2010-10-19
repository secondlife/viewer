/** 
 * @file llfloaterimportcollada.cpp
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


#include "llviewerprecompiledheaders.h"

#if 0 //DEPRECATED

#include "llfloaterimportcollada.h"

#include "dae.h"
//#include "dom.h"
#include "dom/domAsset.h"
#include "dom/domBind_material.h"
#include "dom/domConstants.h"
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

#include "llagent.h"
#include "llassetuploadresponders.h"
#include "lleconomy.h"
#include "llfloaterperms.h"
#include "llfloaterreg.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "llselectmgr.h"
#include "llvfile.h"
#include "llvfs.h"
#include "llviewermenufile.h"
#include "llviewerregion.h"
#include "llvolumemessage.h"
#include "llmodel.h"
#include "llmeshreduction.h"
#include "material_codes.h"

//
// floater
//

LLFloaterImportCollada::LLFloaterImportCollada(const LLSD& key)
	: LLFloater(key)
{
}


BOOL LLFloaterImportCollada::postBuild()
{
	if (!LLFloater::postBuild())
	{
		return FALSE;
	}

	childSetCommitCallback("ok", LLImportCollada::onCommitOK, this);
	childSetCommitCallback("cancel", LLImportCollada::onCommitCancel, this);
	
	setStatusIdle();
	setAssetCount(0,0);
	enableOK(TRUE);

	return TRUE;
}


void LLFloaterImportCollada::setAssetCount(S32 mesh_count, S32 texture_count)
{
	childSetTextArg("mesh count", "[COUNT]", llformat("%d", mesh_count));
	childSetTextArg("texture count", "[COUNT]", llformat("%d", texture_count));
}

void LLFloaterImportCollada::setStatusAssetUploading(std::string asset_name)
{
	LLUIString uploading = getString("status_uploading");
	uploading.setArg("[NAME]", asset_name);
	childSetTextArg("status", "[STATUS]", uploading.getString());
}

void LLFloaterImportCollada::setStatusCreatingPrim(std::string prim_name)
{
	LLUIString creating = getString("status_creating");
	creating.setArg("[NAME]", prim_name);
	childSetTextArg("status", "[STATUS]", creating.getString());
}

void LLFloaterImportCollada::setStatusIdle()
{
	childSetTextArg("status", "[STATUS]", getString("status_idle"));
}

void LLFloaterImportCollada::enableOK(BOOL enable)
{
	childSetEnabled("ok", enable);
}


//
// misc helpers
//


// why oh why do forbid matrix multiplication in our llmath library?
LLMatrix4 matrix_multiply(LLMatrix4 a, LLMatrix4 b)
{
	a *= b;

	return a;
}

// why oh why does colladadom not provide such things?
daeElement* getFirstChild(daeElement* parent)
{
	daeTArray< daeSmartRef<daeElement> > children = parent->getChildren();

	if (children.getCount() > 0)
	{
		return children[0];
	}
	else
	{
		return NULL;
	}
}

// why oh why does colladadom not provide such things?
daeElement* getNextSibling(daeElement* child)
{
	daeElement* parent = child->getParent();

	if (parent == NULL)
	{
		// must be root, root has no siblings
		return NULL;
	}

	daeElement* sibling = NULL;

	daeTArray< daeSmartRef<daeElement> > children = parent->getChildren();
	for (S32 i = 0; i < children.getCount(); i++)
	{
		if (child == children[i])
		{
			if ((i+1) < children.getCount())
			{
				sibling = children[i+1];
			}
		}
	}

	return sibling;
}

// try to get a decent label for this element
std::string getElementLabel(daeElement *element)
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

LLColor4 getDaeColor(daeElement* element)
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

LLTextureEntry profileToTextureEntry(domProfile_COMMON* material)
{
	LLTextureEntry te;

	te.setID(LLUUID("5748decc-f629-461c-9a36-a35a221fe21f")); // blank texture
	daeElement* diffuse = material->getDescendant("diffuse");
	if (diffuse)
	{
		te.setColor(LLColor3(0.1f, 0.9f, 1.0f));
					
		domCommon_color_or_texture_type_complexType::domTexture* texture =
			daeSafeCast<domCommon_color_or_texture_type_complexType::domTexture>(diffuse->getDescendant("texture"));
		if (texture)
		{
			domCommon_newparam_type_Array newparams = material->getNewparam_array();
			for (S32 i = 0; i < newparams.getCount(); i++)
			{
				domFx_surface_common* surface = newparams[i]->getSurface();
				if (surface)
				{
					domFx_surface_init_common* init = surface->getFx_surface_init_common();
					if (init)
					{
						domFx_surface_init_from_common_Array init_from = init->getInit_from_array();

						if (init_from.getCount() > 0)
						{
							daeElement* image = init_from[0]->getValue().getElement();
							if (image)
							{
								LLUUID texture_asset = LLImportColladaAssetCache::getInstance()->getAssetForDaeElement(image);

								if (texture_asset.notNull())
								{
									te.setID(texture_asset);
									te.setColor(LLColor3(1,1,1));
								}
							}
						}
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
			te.setColor(value);
		}
	}

	daeElement* emission = material->getDescendant("emission");
	if (emission)
	{
		LLColor4 emission_color = getDaeColor(emission);
		if (((emission_color[0] + emission_color[1] + emission_color[2]) / 3.0) > 0.25)
		{
			te.setFullbright(TRUE);
		}
	}

	return te;
}

std::vector<LLTextureEntry> getMaterials(LLModel* model, domInstance_geometry* instance_geo)
{
	std::vector<LLTextureEntry> texture_entries;
	for (int i = 0; i < model->mMaterialList.size(); i++)
	{
		LLTextureEntry texture_entry;

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
							texture_entry = profileToTextureEntry(profile);
						}
					}
				}
			}
		}
		
		texture_entries.push_back(texture_entry);
	}

	return texture_entries;
}

LLTextureEntry instanceGeoToTextureEntry(domInstance_geometry* instance_geo)
{
	LLTextureEntry te;
	domInstance_material* instance_mat = 
		daeSafeCast<domInstance_material>(instance_geo->getDescendant(daeElement::matchType(domInstance_material::ID())));
	if (instance_mat)
	{
	}

	return te;
}



// responder for asset uploads
// does all the normal stuff followed by a notification to continue importing
// WARNING - currently unused - TODO
class LLColladaNewAgentInventoryResponder : public LLNewAgentInventoryResponder
{
	LLColladaNewAgentInventoryResponder(const LLSD& post_data,
										const LLUUID& vfile_id,
										LLAssetType::EType asset_type)
	    : LLNewAgentInventoryResponder(post_data, vfile_id, asset_type)
	{
	}
	LLColladaNewAgentInventoryResponder(const LLSD& post_data,
										const std::string& file_name,
										LLAssetType::EType asset_type)
	    : LLNewAgentInventoryResponder(post_data, file_name, asset_type)
	{
	}

	virtual void uploadComplete(const LLSD& content)
	{
		LLNewAgentInventoryResponder::uploadComplete(content);
	}
	
};

BOOL LLImportColladaAssetCache::uploadImageAsset(domImage* image)
{
	// we only support init_from now - embedded data will come later
	domImage::domInit_from* init = image->getInit_from();
	if (!init)
	{
		return FALSE;
	}

	std::string filename = cdom::uriToNativePath(init->getValue().str());

	std::string name = getElementLabel(image);
	
	LLUUID transaction_id = upload_new_resource(filename, name, std::string(),
												0, LLFolderType::FT_TEXTURE, LLInventoryType::IT_TEXTURE,
												LLFloaterPerms::getNextOwnerPerms(), LLFloaterPerms::getGroupPerms(),
												LLFloaterPerms::getEveryonePerms(),
												name, NULL,
												LLGlobalEconomy::Singleton::getInstance()->getPriceUpload(), NULL);

	if (transaction_id.isNull())
	{
		llwarns << "cannot upload " << filename << llendl;
		
		return FALSE;
	}

	mTransactionMap[transaction_id] = image;

	LLFloaterReg::findTypedInstance<LLFloaterImportCollada>("import_collada")->setStatusAssetUploading(name);
	
	return TRUE;
}



//
// asset cache -
// uploads assets and provides a map from collada element to asset
//



BOOL LLImportColladaAssetCache::uploadMeshAsset(domMesh* mesh)
{
	LLPointer<LLModel> model = LLModel::loadModelFromDomMesh(mesh);

	if (model->getNumVolumeFaces() == 0)
	{
		return FALSE;
	}

	// generate LODs
	
	
	std::vector<LLPointer<LLModel> > lods;
	lods.push_back(model);
	S32 triangle_count = model->getNumTriangles();

	for (S32 i = 0; i < 4; i++)
	{
		LLPointer<LLModel> last_model = lods.back();

		S32 triangle_target = (S32)(triangle_count / pow(3.f, i + 1));
		if (triangle_target > 16)
		{
			LLMeshReduction reduction;
			LLPointer<LLModel> new_model = reduction.reduce(model, triangle_target, LLMeshReduction::TRIANGLE_BUDGET);
			lods.push_back(new_model);
		}
		else
		{
			lods.push_back(last_model);
		}
	}
	
    // write model to temp file

	std::string filename = gDirUtilp->getTempFilename();
	LLModel::writeModel(
		filename,
		lods[4],
		lods[0],
		lods[1],
		lods[2],
		lods[3],
		lods[4]->mConvexHullDecomp);


	// copy file to VFS

	LLTransactionID tid;
	tid.generate();
	LLAssetID uuid = tid.makeAssetID(gAgent.getSecureSessionID());  // create asset uuid

	S32 file_size;
	LLAPRFile infile ;
	infile.open(filename, LL_APR_RB, NULL, &file_size);

	if (infile.getFileHandle())
	{
		LLVFile file(gVFS, uuid, LLAssetType::AT_MESH, LLVFile::WRITE);

		file.setMaxSize(file_size);

		const S32 buf_size = 65536;
		U8 copy_buf[buf_size];
		while ((file_size = infile.read(copy_buf, buf_size)))
		{
			file.write(copy_buf, file_size);
		}
	}

	
	std::string name = getElementLabel(mesh);
	
	upload_new_resource(tid, LLAssetType::AT_MESH, name, std::string(), 0,LLFolderType::FT_MESH, LLInventoryType::IT_MESH,
						LLFloaterPerms::getNextOwnerPerms(), LLFloaterPerms::getGroupPerms(), LLFloaterPerms::getEveryonePerms(),
						name, NULL,
						LLGlobalEconomy::Singleton::getInstance()->getPriceUpload(), NULL);

	LLFile::remove(filename);

	mTransactionMap[uuid] = mesh;

	LLFloaterReg::findTypedInstance<LLFloaterImportCollada>("import_collada")->setStatusAssetUploading(name);

	return TRUE;
}


// called by the mesh asset upload responder to indicate the mesh asset has been uploaded
void LLImportColladaAssetCache::assetUploaded(LLUUID transaction_uuid, LLUUID asset_uuid, BOOL success)
{
	std::map<LLUUID, daeElement*>::iterator i = mTransactionMap.find(transaction_uuid);

	if (i != mTransactionMap.end())
	{
		daeElement* element = i->second;
			

		if (success)
		{
			mAssetMap[element] = asset_uuid;
		}
		else // failure
		{
			// if failed, put back on end of queue
			mUploadsPending.push_back(element);
		}
		
		mUploads--;
		uploadNextAsset();
	}
}

const S32 MAX_CONCURRENT_UPLOADS = 5;

void LLImportColladaAssetCache::uploadNextAsset()
{
	while ((mUploadsPending.size() > 0) && (mUploads < MAX_CONCURRENT_UPLOADS))
	{
		BOOL upload_started = FALSE;

		daeElement* element = mUploadsPending.back();
		mUploadsPending.pop_back();

		domImage* image = daeSafeCast<domImage>(element);
		if (image)
		{
			upload_started = uploadImageAsset(image);
		}
		
		domMesh* mesh = daeSafeCast<domMesh>(element);
		if (mesh)
		{
			upload_started = uploadMeshAsset(mesh);
		}

		if (upload_started)
		{
			mUploads++;
		}

	}

	if ((mUploadsPending.size() == 0) && (mUploads == 0))
	{
		// we're done! notify the importer
		LLImportCollada::getInstance()->assetsUploaded();
	}

	updateCount();
}


void LLImportColladaAssetCache::clear()
{
	mDAE = NULL;
	mTransactionMap.clear();
	mAssetMap.clear();
	mUploadsPending.clear();
	mUploads = 0;
}

void LLImportColladaAssetCache::endImport()
{
	clear();
}

void LLImportColladaAssetCache::updateCount()
{
	S32 mesh_count = 0;
	S32 image_count = 0;

	for (S32 i = 0; i < mUploadsPending.size(); i++)
	{
		daeElement* element = mUploadsPending[i];

		if (daeSafeCast<domMesh>(element))
		{
			mesh_count++;
		}

		if (daeSafeCast<domImage>(element))
		{
			image_count++;
		}
	}
	
	LLFloaterReg::findTypedInstance<LLFloaterImportCollada>("import_collada")->setAssetCount(mesh_count, image_count);
}

void LLImportColladaAssetCache::prepareForUpload(DAE* dae)
{
	clear();
	mDAE = dae;

	daeDatabase* db = mDAE->getDatabase();

	S32 mesh_count = db->getElementCount(NULL, COLLADA_TYPE_MESH);
	for (S32 i = 0; i < mesh_count; i++)
	{
		domMesh* mesh = NULL;

		db->getElement((daeElement**) &mesh, i, NULL, COLLADA_TYPE_MESH);

		mUploadsPending.push_back(mesh);
	}

	
	S32 image_count = db->getElementCount(NULL, COLLADA_TYPE_IMAGE);
	for (S32 i = 0; i < image_count; i++)
	{
		domImage* image = NULL;
		db->getElement((daeElement**) &image, i, NULL, COLLADA_TYPE_IMAGE);
		
		mUploadsPending.push_back(image);
	}

	updateCount();
}


void LLImportColladaAssetCache::uploadAssets()
{
	uploadNextAsset();
}


LLUUID LLImportColladaAssetCache::getAssetForDaeElement(daeElement* element)
{
	LLUUID id;
	
	std::map<daeElement*, LLUUID>::iterator i = mAssetMap.find(element);
	if (i != mAssetMap.end())
	{
		id = i->second;
	}

	return id;
}


//
// importer
//



LLImportCollada::LLImportCollada()
{
	mIsImporting = FALSE;
}




void LLImportCollada::appendObjectAsset(domInstance_geometry* instance_geo)
{
	domGeometry* geo = daeSafeCast<domGeometry>(instance_geo->getUrl().getElement());
	if (!geo)
	{
		llwarns << "cannot find geometry" << llendl;
		return;
	}
	
	domMesh* mesh = daeSafeCast<domMesh>(geo->getDescendant(daeElement::matchType(domMesh::ID())));
	if (!mesh)
	{
		llwarns << "could not find mesh" << llendl;
		return;
	}
	
	LLUUID mesh_asset = LLImportColladaAssetCache::getInstance()->getAssetForDaeElement(mesh);
	if (mesh_asset.isNull())
	{
		llwarns << "no mesh asset, skipping" << llendl;
		return;
	}

	// load the model
	LLModel* model = LLModel::loadModelFromDomMesh(mesh);



	
	// get our local transformation
	LLMatrix4 transformation = mStack.front().transformation;

	// adjust the transformation to compensate for mesh normalization
	LLVector3 mesh_scale_vector;
	LLVector3 mesh_translation_vector;
	model->getNormalizedScaleTranslation(mesh_scale_vector, mesh_translation_vector);
	LLMatrix4 mesh_translation;
	mesh_translation.setTranslation(mesh_translation_vector);
	transformation = matrix_multiply(mesh_translation, transformation);
	LLMatrix4 mesh_scale;
	mesh_scale.initScale(mesh_scale_vector);
	transformation = matrix_multiply(mesh_scale, transformation);

	// check for reflection
	BOOL reflected = (transformation.determinant() < 0);
	
	// compute position
	LLVector3 position = LLVector3(0, 0, 0) * transformation;

	// compute scale
	LLVector3 x_transformed = LLVector3(1, 0, 0) * transformation - position;
	LLVector3 y_transformed = LLVector3(0, 1, 0) * transformation - position;
	LLVector3 z_transformed = LLVector3(0, 0, 1) * transformation - position;
	F32 x_length = x_transformed.normalize();
	F32 y_length = y_transformed.normalize();
	F32 z_length = z_transformed.normalize();
	LLVector3 scale = LLVector3(x_length, y_length, z_length);

	// adjust for "reflected" geometry
	LLVector3 x_transformed_reflected = x_transformed;
	if (reflected)
	{
		x_transformed_reflected *= -1.0;
	}
	
	// compute rotation
	LLMatrix3 rotation_matrix;
	rotation_matrix.setRows(x_transformed_reflected, y_transformed, z_transformed);
	LLQuaternion quat_rotation = rotation_matrix.quaternion();
	quat_rotation.normalize(); // the rotation_matrix might not have been orthoginal.  make it so here.
	LLVector3 euler_rotation;
	quat_rotation.getEulerAngles(&euler_rotation.mV[VX], &euler_rotation.mV[VY], &euler_rotation.mV[VZ]);

	
	//
	// build parameter block to construct this prim
	//
	
	LLSD object_params;

    // create prim

	// set volume params
	LLVolumeParams  volume_params;
	volume_params.setType( LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE );
	volume_params.setBeginAndEndS( 0.f, 1.f );
	volume_params.setBeginAndEndT( 0.f, 1.f );
	volume_params.setRatio  ( 1, 1 );
	volume_params.setShear  ( 0, 0 );
	U8 sculpt_type = LL_SCULPT_TYPE_MESH;
	if (reflected)
	{
		sculpt_type |= LL_SCULPT_FLAG_MIRROR;
	}
	volume_params.setSculptID(mesh_asset, sculpt_type);
	object_params["shape"] = volume_params.asLLSD();

	object_params["material"] = LL_MCODE_WOOD;

	object_params["group-id"] = gAgent.getGroupID();
	object_params["pos"] = ll_sd_from_vector3(position);
	object_params["rotation"] = ll_sd_from_quaternion(quat_rotation);
	object_params["scale"] = ll_sd_from_vector3(scale);
	object_params["name"] = mStack.front().name;

	// load material from dae file
	std::vector<LLTextureEntry> texture_entries = getMaterials(model, instance_geo);
	object_params["facelist"] = LLSD::emptyArray();
	for (int i = 0; i < texture_entries.size(); i++)
	{
		object_params["facelist"][i] = texture_entries[i].asLLSD();
	}

	// set extra parameters
	LLSculptParams sculpt_params;
	sculpt_params.setSculptTexture(mesh_asset);
	sculpt_params.setSculptType(sculpt_type);
	U8 buffer[MAX_OBJECT_PARAMS_SIZE+1];
	LLDataPackerBinaryBuffer dp(buffer, MAX_OBJECT_PARAMS_SIZE);
	sculpt_params.pack(dp);
	std::vector<U8> v(dp.getCurrentSize());
	memcpy(&v[0], buffer, dp.getCurrentSize());
	LLSD extra_parameter;
	extra_parameter["extra_parameter"] = sculpt_params.mType;
	extra_parameter["param_data"] = v;
	object_params["extra_parameters"].append(extra_parameter);

	mObjectList.append(object_params);
	
	delete model;

	LLFloaterReg::findTypedInstance<LLFloaterImportCollada>("import_collada")->setStatusCreatingPrim(mStack.front().name);
	
	return;
}

void LLImportCollada::uploadObjectAsset()
{
	LLSD request;
	request["objects"] = mObjectList;
	
	std::string url = gAgent.getRegion()->getCapability("UploadObjectAsset");
	LLHTTPClient::post(url, request, new LLHTTPClient::Responder());
}



void LLImportCollada::importFile(std::string filename)
{
	if (mIsImporting)
	{
		llwarns << "Importer already running, import command for " << filename << " ignored" << llendl;
		return;
	}

	LLFloaterReg::showInstance("import_collada");
	LLFloaterReg::findTypedInstance<LLFloaterImportCollada>("import_collada")->enableOK(TRUE);

	
	mIsImporting = TRUE;
	mDAE = new DAE;
	mImportOrigin = gAgent.getPositionAgent() + LLVector3(0, 0, 2);
	mSceneTransformation = LLMatrix4();  // identity
	mFilename = filename;
	mCreates = 0;
	mObjectList = LLSD::emptyArray();
	
	if (mDAE->open(mFilename) == NULL)
	{
		llwarns << "cannot open file " << mFilename << llendl;
		endImport();
		return;
	}

	LLImportColladaAssetCache::getInstance()->prepareForUpload(mDAE);

	return;
}


void LLImportCollada::assetsUploaded()
{
	if (!mIsImporting)
	{
		// weird, we got a callback while not importing.
		return;
	}
	
	daeDocument* doc = mDAE->getDoc(mFilename);
	if (!doc)
	{
		llwarns << "can't find internal doc" << llendl;
		endImport();
	}
	
	daeElement* root = doc->getDomRoot();
	if (!root)
	{
		llwarns << "document has no root" << llendl;
		endImport();
	}

	domAsset::domUnit* unit = daeSafeCast<domAsset::domUnit>(root->getDescendant(daeElement::matchType(domAsset::domUnit::ID())));
	if (unit)
	{
		mSceneTransformation *= unit->getMeter();
	}

	domUpAxisType up = UPAXISTYPE_Y_UP;  // default is Y_UP
	domAsset::domUp_axis* up_axis =
		daeSafeCast<domAsset::domUp_axis>(root->getDescendant(daeElement::matchType(domAsset::domUp_axis::ID())));
	if (up_axis)
	{
		up = up_axis->getValue();
	}
	
	if (up == UPAXISTYPE_X_UP)
	{
		LLMatrix4 rotation;
		rotation.initRotation(0.0f, 90.0f * DEG_TO_RAD, 0.0f);

		mSceneTransformation = matrix_multiply(rotation, mSceneTransformation);
	}
	else if (up == UPAXISTYPE_Y_UP)
	{
		LLMatrix4 rotation;
		rotation.initRotation(90.0f * DEG_TO_RAD, 0.0f, 0.0f);

		mSceneTransformation = matrix_multiply(rotation, mSceneTransformation);
	}
    // else Z_UP, which is our behavior

	

	daeElement* scene = root->getDescendant("visual_scene");
	if (!scene)
	{
		llwarns << "document has no visual_scene" << llendl;
		endImport();
	}

	processElement(scene);
	processNextElement();
}

void LLImportCollada::pushStack(daeElement* next_element, std::string name, LLMatrix4 transformation)
{
	struct StackState new_state;

	new_state.next_element = next_element;
	new_state.name = name;
	new_state.transformation = transformation;

	mStack.push_front(new_state);
}



void LLImportCollada::popStack()
{
	mStack.pop_front();
}



BOOL LLImportCollada::processElement(daeElement* element)
{
	if (mStack.size() > 0)
	{
		mStack.front().next_element = getNextSibling(element);
	}
	
	domTranslate* translate = daeSafeCast<domTranslate>(element);
	if (translate)
	{
		domFloat3 dom_value = translate->getValue();

		LLMatrix4 translation;
		translation.setTranslation(LLVector3(dom_value[0], dom_value[1], dom_value[2]));

		mStack.front().transformation = matrix_multiply(translation, mStack.front().transformation);
	}

	domRotate* rotate = daeSafeCast<domRotate>(element);
	if (rotate)
	{
		domFloat4 dom_value = rotate->getValue();

		LLMatrix4 rotation;
		rotation.initRotTrans(dom_value[3] * DEG_TO_RAD, LLVector3(dom_value[0], dom_value[1], dom_value[2]), LLVector3(0, 0, 0));

		mStack.front().transformation = matrix_multiply(rotation, mStack.front().transformation);
	}

	domScale* scale = daeSafeCast<domScale>(element);
	if (scale)
	{
		domFloat3 dom_value = scale->getValue();

		LLMatrix4 scaling;
		scaling.initScale(LLVector3(dom_value[0], dom_value[1], dom_value[2]));

		mStack.front().transformation = matrix_multiply(scaling, mStack.front().transformation);
	}

	domMatrix* matrix = daeSafeCast<domMatrix>(element);
	if (matrix)
	{
		domFloat4x4 dom_value = matrix->getValue();

		LLMatrix4 matrix_transform;

		for (int i = 0; i < 4; i++)
			for(int j = 0; j < 4; j++)
			{
				matrix_transform.mMatrix[i][j] = dom_value[i + j*4];
			}
		
		mStack.front().transformation = matrix_multiply(matrix_transform, mStack.front().transformation);
	}

	domInstance_geometry* instance_geo = daeSafeCast<domInstance_geometry>(element);
	if (instance_geo)
	{
		appendObjectAsset(instance_geo);
	}

	domNode* node = daeSafeCast<domNode>(element);
	if (node)
	{
		pushStack(getFirstChild(element), getElementLabel(element), mStack.front().transformation);
	}

	domInstance_node* instance_node = daeSafeCast<domInstance_node>(element);
	if (instance_node)
	{
		daeElement* instance = instance_node->getUrl().getElement();
		if (instance)
		{
			pushStack(getFirstChild(instance), getElementLabel(instance), mStack.front().transformation);
		}
	}

	domVisual_scene* scene = daeSafeCast<domVisual_scene>(element);
	if (scene)
	{
		pushStack(getFirstChild(element), std::string("scene"), mSceneTransformation);
	}

	return FALSE;
}


void LLImportCollada::processNextElement()
{
	while(1)
	{
		if (mStack.size() == 0)
		{
			uploadObjectAsset();
			endImport();
			return;
		}

		daeElement *element = mStack.front().next_element;

		if (element == NULL)
		{
			popStack();
		}
		else
		{
			processElement(element);
		}
	}
}


void LLImportCollada::endImport()
{
	LLFloaterReg::hideInstance("import_collada");

	LLImportColladaAssetCache::getInstance()->endImport();
	
	if (mDAE)
	{
		delete mDAE;
		mDAE = NULL;
	}
	
	mIsImporting = FALSE;
}


/* static */
void LLImportCollada::onCommitOK(LLUICtrl*, void*)
{
	LLFloaterReg::findTypedInstance<LLFloaterImportCollada>("import_collada")->enableOK(FALSE);

	LLImportColladaAssetCache::getInstance()->uploadAssets();
}


/* static */
void LLImportCollada::onCommitCancel(LLUICtrl*, void*)
{
	getInstance()->endImport();
}

#endif
