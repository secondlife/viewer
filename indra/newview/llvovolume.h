/** 
 * @file llvovolume.h
 * @brief LLVOVolume class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVOVOLUME_H
#define LL_LLVOVOLUME_H

#include "llviewerobject.h"
#include "llspatialpartition.h"
#include "llviewerimage.h"
#include "llframetimer.h"
#include "llapr.h"
#include <map>

class LLViewerTextureAnim;
class LLDrawPool;
class LLSelectNode;

enum LLVolumeInterfaceType
{
	INTERFACE_FLEXIBLE = 1,
};

// Base class for implementations of the volume - Primitive, Flexible Object, etc.
class LLVolumeInterface
{
public:
	virtual ~LLVolumeInterface() { }
	virtual LLVolumeInterfaceType getInterfaceType() const = 0;
	virtual BOOL doIdleUpdate(LLAgent &agent, LLWorld &world, const F64 &time) = 0;
	virtual BOOL doUpdateGeometry(LLDrawable *drawable) = 0;
	virtual LLVector3 getPivotPosition() const = 0;
	virtual void onSetVolume(const LLVolumeParams &volume_params, const S32 detail) = 0;
	virtual void onSetScale(const LLVector3 &scale, BOOL damped) = 0;
	virtual void onParameterChanged(U16 param_type, LLNetworkData *data, BOOL in_use, bool local_origin) = 0;
	virtual void onShift(const LLVector3 &shift_vector) = 0;
	virtual bool isVolumeUnique() const = 0; // Do we need a unique LLVolume instance?
	virtual bool isVolumeGlobal() const = 0; // Are we in global space?
	virtual bool isActive() const = 0; // Is this object currently active?
	virtual const LLMatrix4& getWorldMatrix(LLXformMatrix* xform) const = 0;
	virtual void updateRelativeXform() = 0;
	virtual U32 getID() const = 0;
};

// Class which embodies all Volume objects (with pcode LL_PCODE_VOLUME)
class LLVOVolume : public LLViewerObject
{
public:
	static		void	initClass();
	static 		void 	preUpdateGeom();
	
	enum 
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD2) |
							(1 << LLVertexBuffer::TYPE_COLOR)
	}
	eVertexDataMask;

public:
						LLVOVolume(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	virtual				~LLVOVolume();

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);

				void	deleteFaces();

	/*virtual*/ BOOL	idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);

	/*virtual*/ BOOL	isActive() const;
	/*virtual*/ BOOL	isAttachment() const;
	/*virtual*/ BOOL	isRootEdit() const; // overridden for sake of attachments treating themselves as a root object
	/*virtual*/ BOOL	isHUDAttachment() const;

				void	generateSilhouette(LLSelectNode* nodep, const LLVector3& view_point);

				F32		getIndividualRadius()					{ return mRadius; }
				S32		getLOD() const							{ return mLOD; }
	const LLVector3		getPivotPositionAgent() const;
	const LLMatrix4&	getRelativeXform() const				{ return mRelativeXform; }
	const LLMatrix3&	getRelativeXformInvTrans() const		{ return mRelativeXformInvTrans; }
	/*virtual*/	const LLMatrix4	getRenderMatrix() const;

	/*virtual*/ BOOL	lineSegmentIntersect(const LLVector3& start, LLVector3& end) const;
				LLVector3 agentPositionToVolume(const LLVector3& pos) const;
				LLVector3 agentDirectionToVolume(const LLVector3& dir) const;
				LLVector3 volumePositionToAgent(const LLVector3& dir) const;

				
				BOOL	getVolumeChanged() const				{ return mVolumeChanged; }
				F32		getTextureVirtualSize(LLFace* face);
	/*virtual*/ F32  	getRadius() const						{ return mVObjRadius; };
				const LLMatrix4& getWorldMatrix(LLXformMatrix* xform) const;

				void	markForUpdate(BOOL priority)			{ LLViewerObject::markForUpdate(priority); mVolumeChanged = TRUE; }

	/*virtual*/ void	onShift(const LLVector3 &shift_vector); // Called when the drawable shifts

	/*virtual*/ void	parameterChanged(U16 param_type, bool local_origin);
	/*virtual*/ void	parameterChanged(U16 param_type, LLNetworkData* data, BOOL in_use, bool local_origin);

	/*virtual*/ U32		processUpdateMessage(LLMessageSystem *mesgsys,
											void **user_data,
											U32 block_num, const EObjectUpdateType update_type,
											LLDataPacker *dp);

	/*virtual*/ void	setSelected(BOOL sel);
	/*virtual*/ BOOL	setDrawableParent(LLDrawable* parentp);

	/*virtual*/ void	setScale(const LLVector3 &scale, BOOL damped);

	/*virtual*/ void	setTEImage(const U8 te, LLViewerImage *imagep);
	/*virtual*/ S32		setTETexture(const U8 te, const LLUUID &uuid);
	/*virtual*/ S32		setTEColor(const U8 te, const LLColor4 &color);
	/*virtual*/ S32		setTEBumpmap(const U8 te, const U8 bump);
	/*virtual*/ S32		setTEShiny(const U8 te, const U8 shiny);
	/*virtual*/ S32		setTEFullbright(const U8 te, const U8 fullbright);
	/*virtual*/ S32		setTEMediaFlags(const U8 te, const U8 media_flags);
	/*virtual*/ S32		setTEScale(const U8 te, const F32 s, const F32 t);
	/*virtual*/ S32		setTEScaleS(const U8 te, const F32 s);
	/*virtual*/ S32		setTEScaleT(const U8 te, const F32 t);
	/*virtual*/ S32		setTETexGen(const U8 te, const U8 texgen);
	/*virtual*/ BOOL 	setMaterial(const U8 material);

				void	setTexture(const S32 face);

	/*virtual*/ BOOL	setVolume(const LLVolumeParams &volume_params, const S32 detail, bool unique_volume = false);
				void	updateRelativeXform();
	/*virtual*/ BOOL	updateGeometry(LLDrawable *drawable);
	/*virtual*/ void	updateFaceSize(S32 idx);
	/*virtual*/ BOOL	updateLOD();
				void	updateRadius();
	/*virtual*/ void	updateTextures(LLAgent &agent);
				void	updateTextures();

				void	updateFaceFlags();
				void	regenFaces();
				BOOL	genBBoxes(BOOL force_global);
	virtual		void	updateSpatialExtents(LLVector3& min, LLVector3& max);
	virtual		F32		getBinRadius();
	virtual		void	writeCAL3D(apr_file_t* fp, 
							std::string& path,
							std::string& file_base,
							S32 joint_num, 
							LLVector3& pos, 
							LLQuaternion& rot, 
							S32& material_index, 
							S32& texture_index, 
							std::multimap<LLUUID, LLMaterialExportInfo*>& material_map);


	virtual U32 getPartitionType() const;

	// For Lights
	void setIsLight(BOOL is_light);
	void setLightColor(const LLColor3& color);
	void setLightIntensity(F32 intensity);
	void setLightRadius(F32 radius);
	void setLightFalloff(F32 falloff);
	void setLightCutoff(F32 cutoff);
	BOOL getIsLight() const;
	LLColor3 getLightBaseColor() const; // not scaled by intensity
	LLColor3 getLightColor() const; // scaled by intensity
	F32 getLightIntensity() const;
	F32 getLightRadius() const;
	F32 getLightFalloff() const;
	F32 getLightCutoff() const;
	F32 getLightDistance(const LLVector3& pos) const; // returns < 0 if inside radius

	// Flexible Objects
	U32 getVolumeInterfaceID() const;
	virtual BOOL isFlexible() const;
	BOOL isVolumeGlobal() const;
	BOOL canBeFlexible() const;
	BOOL setIsFlexible(BOOL is_flexible);
	
	// Lighting
	F32 calcLightAtPoint(const LLVector3& pos, const LLVector3& norm, LLColor4& result);
	BOOL updateLighting(BOOL do_lighting);

protected:
	S32	computeLODDetail(F32	distance, F32 radius);
	BOOL calcLOD();
	LLFace* addFace(S32 face_index);
	void updateTEData();

public:
	LLViewerTextureAnim *mTextureAnimp;
	U8 mTexAnimMode;
protected:
	friend class LLDrawable;
	
	BOOL		mFaceMappingChanged;
	BOOL		mGlobalVolume;
	BOOL		mInited;
	LLFrameTimer mTextureUpdateTimer;
	S32			mLOD;
	BOOL		mLODChanged;
	F32			mRadius;
	LLMatrix4	mRelativeXform;
	LLMatrix3	mRelativeXformInvTrans;
	BOOL		mVolumeChanged;
	F32			mVObjRadius;
	LLVolumeInterface *mVolumeImpl;
	
	// statics
public:
	static F32 sLODSlopDistanceFactor;// Changing this to zero, effectively disables the LOD transition slop 
	static F32 sLODFactor;				// LOD scale factor
	static F32 sDistanceFactor;			// LOD distance factor
		
protected:
	static S32 sNumLODChanges;
	
	friend class LLVolumeImplFlexible;
};

#endif // LL_LLVOVOLUME_H
