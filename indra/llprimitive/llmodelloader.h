/**
 * @file llmodelloader.h
 * @brief LLModelLoader class definition
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

#ifndef LL_LLMODELLOADER_H
#define LL_LLMODELLOADER_H

#include "llmodel.h"
#include "llthread.h"
#include <boost/function.hpp>
#include <list>

class LLJoint;

typedef std::map<std::string, LLMatrix4>			JointTransformMap;
typedef std::map<std::string, LLMatrix4>::iterator	JointTransformMapIt;
typedef std::map<std::string, std::string>			JointMap;
typedef std::deque<std::string>						JointNameSet;

const S32 SLM_SUPPORTED_VERSION	= 3;
const S32 NUM_LOD						= 4;

class LLModelLoader : public LLThread
{
public:

	typedef std::map<std::string, LLImportMaterial>			material_map;
	typedef std::vector<LLPointer<LLModel > >					model_list;	
	typedef std::vector<LLModelInstance>						model_instance_list;	
	typedef std::map<LLMatrix4, model_instance_list >		scene;

	// Callback with loaded model data and loaded LoD
	// 
	typedef boost::function<void (scene&,model_list&,S32,void*) >		load_callback_t;

	// Function to provide joint lookup by name
	// (within preview avi skeleton, for example)
	//
	typedef boost::function<LLJoint* (const std::string&,void*) >		joint_lookup_func_t;

	// Func to load and associate material with all it's textures,
	// returned value is the number of textures loaded
	// intentionally non-const so func can modify material to
	// store platform-specific data
	//
	typedef boost::function<U32 (LLImportMaterial&,void*) >				texture_load_func_t;

	// Callback to inform client of state changes
	// during loading process (errors will be reported
	// as state changes here as well)
	//
	typedef boost::function<void (U32,void*) >								state_callback_t;

	typedef enum
	{
		STARTING = 0,
		READING_FILE,
		CREATING_FACES,
		GENERATING_VERTEX_BUFFERS,
		GENERATING_LOD,
		DONE,
		ERROR_PARSING, //basically loading failed
		ERROR_MATERIALS,
		ERROR_PASSWORD_REQUIRED,
		ERROR_NEED_MORE_MEMORY,
		ERROR_INVALID_FILE,
		ERROR_LOADER_SETUP,
		ERROR_INVALID_PARAMETERS,
		ERROR_OUT_OF_RANGE,
		ERROR_FILE_VERSION_INVALID,
		ERROR_MODEL // this error should always be last in this list, error code is passed as ERROR_MODEL+error_code
	} eLoadState;

	U32 mState;
	std::string mFilename;
	
	S32 mLod;
	
	LLMatrix4 mTransform;
	BOOL mFirstTransform;
	LLVector3 mExtents[2];
	
	bool mTrySLM;
	bool mCacheOnlyHitIfRigged; // ignore cached SLM if it does not contain rig info (and we want rig info)

	model_list		mModelList;
	scene				mScene;

	typedef std::queue<LLPointer<LLModel> > model_queue;

	//queue of models that need a physics rep
	model_queue mPhysicsQ;

	//map of avatar joints as named in COLLADA assets to internal joint names
	JointMap			mJointMap;
	JointTransformMap&	mJointList;	
	JointNameSet&		mJointsFromNode;
    U32					mMaxJointsPerMesh;

	LLModelLoader(
		std::string							filename,
		S32									lod, 
		LLModelLoader::load_callback_t		load_cb,
		LLModelLoader::joint_lookup_func_t	joint_lookup_func,
		LLModelLoader::texture_load_func_t	texture_load_func,
		LLModelLoader::state_callback_t		state_cb,
		void*								opaque_userdata,
		JointTransformMap&					jointTransformMap,
		JointNameSet&						jointsFromNodes,
        JointNameSet&						legalJointNames,
        U32									maxJointsPerMesh);
	virtual ~LLModelLoader() ;

	virtual void setNoNormalize() { mNoNormalize = true; }
	virtual void setNoOptimize() { mNoOptimize = true; }

	virtual void run();
	
	// Will try SLM or derived class OpenFile as appropriate
	//
	virtual bool doLoadModel();

	// Derived classes need to provide their parsing of files here
	//
	virtual bool OpenFile(const std::string& filename) = 0;

	bool loadFromSLM(const std::string& filename);
	
	void loadModelCallback();
	void loadTextures() ; //called in the main thread.
	void setLoadState(U32 state);

	

	S32 mNumOfFetchingTextures ; //updated in the main thread
	bool areTexturesReady() { return !mNumOfFetchingTextures; } //called in the main thread.

	bool verifyCount( int expected, int result );

	//Determines the viability of an asset to be used as an avatar rig (w or w/o joint upload caps)
	void critiqueRigForUploadApplicability( const std::vector<std::string> &jointListFromAsset );
	void critiqueJointToNodeMappingFromScene( void  );

	//Determines if a rig is a legacy from the joint list
	bool isRigLegacy( const std::vector<std::string> &jointListFromAsset );

	//Determines if a rig is suitable for upload
	bool isRigSuitableForJointPositionUpload( const std::vector<std::string> &jointListFromAsset );

	void setRigWithSceneParity( bool state ) { mRigParityWithScene = state; }
	const bool getRigWithSceneParity( void ) const { return mRigParityWithScene; }

	const bool isRigValidForJointPositionUpload( void ) const { return mRigValidJointUpload; }
	void setRigValidForJointPositionUpload( bool rigValid ) { mRigValidJointUpload = rigValid; }

	const bool isLegacyRigValid( void ) const { return mLegacyRigValid; }
	void setLegacyRigValid( bool rigValid ) { mLegacyRigValid = rigValid; }		

	//-----------------------------------------------------------------------------
	// isNodeAJoint()
	//-----------------------------------------------------------------------------
	bool isNodeAJoint(const char* name)
	{
		return mJointMap.find(name) != mJointMap.end();
	}

protected:

	LLModelLoader::load_callback_t		mLoadCallback;
	LLModelLoader::joint_lookup_func_t	mJointLookupFunc;
	LLModelLoader::texture_load_func_t	mTextureLoadFunc;
	LLModelLoader::state_callback_t		mStateCallback;
	void*								mOpaqueData;

	bool		mRigParityWithScene;
	bool		mRigValidJointUpload;
	bool		mLegacyRigValid;

	bool		mNoNormalize;
	bool		mNoOptimize;

	JointNameSet		mMasterJointList;
	JointNameSet		mMasterLegacyJointList;
	JointTransformMap	mJointTransformMap;

	static std::list<LLModelLoader*> sActiveLoaderList;
	static bool isAlive(LLModelLoader* loader) ;
};
class LLMatrix4a;
void stretch_extents(LLModel* model, LLMatrix4a& mat, LLVector4a& min, LLVector4a& max, BOOL& first_transform);
void stretch_extents(LLModel* model, LLMatrix4& mat, LLVector3& min, LLVector3& max, BOOL& first_transform);

#endif  // LL_LLMODELLOADER_H
