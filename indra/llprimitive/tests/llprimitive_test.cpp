/** 
 * @file llprimitive_test.cpp
 * @brief llprimitive tests
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "../test/lltut.h"

#include "../llprimitive.h"

#include "../../llmath/llvolumemgr.h"

class DummyVolumeMgr : public LLVolumeMgr
{
public:
	DummyVolumeMgr() : LLVolumeMgr(), mVolumeTest(NULL), mCurrDetailTest(0) {}
	~DummyVolumeMgr() 
	{
	}
	
	
	virtual LLVolume *refVolume(const LLVolumeParams &volume_params, const S32 detail) 
	{
		if (mVolumeTest.isNull() || volume_params != mCurrParamsTest || detail != mCurrDetailTest)
		{
			F32 volume_detail = LLVolumeLODGroup::getVolumeScaleFromDetail(detail);
			mVolumeTest = new LLVolume(volume_params, volume_detail, FALSE, FALSE);
			mCurrParamsTest = volume_params;
			mCurrDetailTest = detail;
			return mVolumeTest;
		} 
		else 
		{
			return mVolumeTest;
		}
	}
	
	virtual void unrefVolume(LLVolume *volumep) 
	{
		if (mVolumeTest == volumep)
		{
			mVolumeTest = NULL;
		}
	}
	
private:
	LLPointer<LLVolume> mVolumeTest;
	LLVolumeParams mCurrParamsTest;
	S32 mCurrDetailTest;
};

class PRIMITIVE_TEST_SETUP
{
public:
	PRIMITIVE_TEST_SETUP()
	{
		volume_manager_test = new DummyVolumeMgr();
		LLPrimitive::setVolumeManager(volume_manager_test);
	}
	
	~PRIMITIVE_TEST_SETUP()
	{
		LLPrimitive::cleanupVolumeManager();
	}
	DummyVolumeMgr * volume_manager_test;
}; 

namespace tut
{
	struct llprimitive
	{
		PRIMITIVE_TEST_SETUP setup_class;
	};
    
	typedef test_group<llprimitive> llprimitive_t;
	typedef llprimitive_t::object llprimitive_object_t;
	tut::llprimitive_t tut_llprimitive("llprimitive");

	template<> template<>
	void llprimitive_object_t::test<1>()
	{
		set_test_name("Test LLPrimitive Instantiation");
		LLPrimitive test;
	}
	
	template<> template<>
	void llprimitive_object_t::test<2>()
	{
		set_test_name("Test LLPrimitive PCode setter and getter.");
		LLPrimitive test;
		ensure_equals(test.getPCode(), 0);
		LLPCode code = 1;
		test.setPCode(code);
		ensure_equals(test.getPCode(), code);
	}
	
	template<> template<>
	void llprimitive_object_t::test<3>()
	{
		set_test_name("Test llprimitive constructor and initer.");
		LLPCode code = 1;
		LLPrimitive primitive;
		primitive.init_primitive(code);
		ensure_equals(primitive.getPCode(), code);
	}
	
	template<> template<>
	void llprimitive_object_t::test<4>()
	{
		set_test_name("Test Static llprimitive constructor and initer.");
		LLPCode code = 1;
		LLPrimitive * primitive = LLPrimitive::createPrimitive(code);
		ensure(primitive != NULL);
		ensure_equals(primitive->getPCode(), code);
	}

	template<> template<>
	void llprimitive_object_t::test<5>()
	{
		set_test_name("Test setVolume creation of new unique volume.");
		LLPrimitive primitive;
		LLVolumeParams params;
		
		// Make sure volume starts off null
		ensure(primitive.getVolume() == NULL);
		
		// Make sure we have no texture entries before setting the volume
		ensure_equals(primitive.getNumTEs(), 0);
		
		// Test that GEOMETRY has not been flagged as changed.
		ensure(!primitive.isChanged(LLXform::GEOMETRY));
		
		// Make sure setVolume returns true
		ensure(primitive.setVolume(params, 0, true) == TRUE);
		LLVolume* new_volume = primitive.getVolume();
		
		// make sure new volume was actually created
		ensure(new_volume != NULL);
		
		// Make sure that now that we've set the volume we have texture entries
		ensure_not_equals(primitive.getNumTEs(), 0);
		
		// Make sure that the number of texture entries equals the number of faces in the volume (should be 6)
		ensure_equals(new_volume->getNumFaces(), 6);
		ensure_equals(primitive.getNumTEs(), new_volume->getNumFaces());
		
		// Test that GEOMETRY has been flagged as changed.
		ensure(primitive.isChanged(LLXform::GEOMETRY));
		
		// Run it twice to make sure it doesn't create a different one if params are the same
		ensure(primitive.setVolume(params, 0, true) == FALSE);
		ensure(new_volume == primitive.getVolume());
		
		// Change the param definition and try setting it again.
		params.setRevolutions(4);
		ensure(primitive.setVolume(params, 0, true) == TRUE);
		
		// Ensure that we now have a different volume
		ensure(new_volume != primitive.getVolume());
	}
	
	template<> template<>
	void llprimitive_object_t::test<6>()
	{
		set_test_name("Test setVolume creation of new NOT-unique volume.");
		LLPrimitive primitive;
		LLVolumeParams params;
		
		// Make sure volume starts off null
		ensure(primitive.getVolume() == NULL);
		
		// Make sure we have no texture entries before setting the volume
		ensure_equals(primitive.getNumTEs(), 0);
		
		// Test that GEOMETRY has not been flagged as changed.
		ensure(!primitive.isChanged(LLXform::GEOMETRY));
		
		// Make sure setVolume returns true
		ensure(primitive.setVolume(params, 0, false) == TRUE);
		
		LLVolume* new_volume = primitive.getVolume();
		
		// make sure new volume was actually created
		ensure(new_volume != NULL);
		
		// Make sure that now that we've set the volume we have texture entries
		ensure_not_equals(primitive.getNumTEs(), 0);
		
		// Make sure that the number of texture entries equals the number of faces in the volume (should be 6)
		ensure_equals(new_volume->getNumFaces(), 6);
		ensure_equals(primitive.getNumTEs(), new_volume->getNumFaces());
		
		// Test that GEOMETRY has been flagged as changed.
		ensure(primitive.isChanged(LLXform::GEOMETRY));
		
		// Run it twice to make sure it doesn't create a different one if params are the same
		ensure(primitive.setVolume(params, 0, false) == FALSE);
		ensure(new_volume == primitive.getVolume());
		
		// Change the param definition and try setting it again.
		params.setRevolutions(4);
		ensure(primitive.setVolume(params, 0, false) == TRUE); 
		
		// Ensure that we now have a different volume
		ensure(new_volume != primitive.getVolume());
	}
}

#include "llmessagesystem_stub.cpp"
