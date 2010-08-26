/** 
 * @file llfloaterimportcollada.h
 * @brief The about box from Help -> About
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLFLOATERIMPORTCOLLADA_H
#define LL_LLFLOATERIMPORTCOLLADA_H

#include "llfloater.h"
#include "llvolume.h" //for LL_MESH_ENABLED

#if LL_MESH_ENABLED

class LLFloaterImportCollada : public LLFloater
{
public:
	LLFloaterImportCollada(const LLSD& key);
	/* virtual */ BOOL postBuild();

	void setAssetCount(S32 mesh_count, S32 texture_count);
	void setStatusAssetUploading(std::string asset_name);
	void setStatusCreatingPrim(std::string prim_name);
	void setStatusIdle();
	void enableOK(BOOL enable);
};

class LLViewerObject;
class DAE;
class daeElement;
class domMesh;
class domImage;
class domInstance_geometry;
class LLModel;
class LLImportCollada;

class LLImportColladaAssetCache : public LLSingleton<LLImportColladaAssetCache>
{
public:
	// called first to initialize 
	void prepareForUpload(DAE* dae);

	// upload the assets in this collada file
	void uploadAssets();

	// get the uploaded assets which corresponds to this element
	LLUUID getAssetForDaeElement(daeElement* element);

	// stop the upload
	void endImport();
	
	// reset
	void clear();

	// callback for notification when an asset has been uploaded
	void assetUploaded(LLUUID transaction_uuid, LLUUID asset_uuid, BOOL success);

private:
	void uploadNextAsset();
	BOOL uploadMeshAsset(domMesh* mesh);
	BOOL uploadImageAsset(domImage* image);
	void updateCount();
	
	std::vector<daeElement*> mUploadsPending;
	std::map<LLUUID, daeElement*> mTransactionMap;
	std::map<daeElement*, LLUUID> mAssetMap;
	
	DAE* mDAE;
	S32 mUploads;
};


class LLImportCollada 
: public LLSingleton<LLImportCollada>
{
public:
	LLImportCollada();
	void importFile(std::string filename);

	// callback when all assets have been uploaded
	void assetsUploaded();

	// callback when buttons pressed
	static void onCommitOK(LLUICtrl*, void*);
	static void onCommitCancel(LLUICtrl*, void*);
	
	
private:
	void endImport();
	void processNextElement();
	BOOL processElement(daeElement* element);
	void pushStack(daeElement* next_element, std::string name, LLMatrix4 transformation);
	void popStack();
	void appendObjectAsset(domInstance_geometry* instance_geo);
	void uploadObjectAsset();
	
	struct StackState
	{
		daeElement* next_element;
		std::string name;
		LLMatrix4 transformation;
	};

	std::list<struct StackState> mStack;
	S32 mCreates;
	LLVector3 mImportOrigin;
	std::string mFilename;
	BOOL mIsImporting;
	DAE  *mDAE;
	LLSD mObjectList;
	
	LLMatrix4   mSceneTransformation;
};

#endif

#endif // LL_LLFLOATERIMPORTCOLLADA_H
