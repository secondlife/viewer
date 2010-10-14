/** 
 * @file llfloaterimportcollada.h
 * @brief The about box from Help -> About
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

#if 0 //DEPRECATED

#ifndef LL_LLFLOATERIMPORTCOLLADA_H
#define LL_LLFLOATERIMPORTCOLLADA_H

#include "llfloater.h"

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

#endif // LL_LLFLOATERIMPORTCOLLADA_H

#endif //DEPRECATED
