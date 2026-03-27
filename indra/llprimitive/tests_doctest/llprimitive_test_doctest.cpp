/**
 * @file llprimitive_test.cpp
 * @brief llprimitive tests
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

#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"

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
            mVolumeTest = new LLVolume(volume_params, volume_detail, false, false);
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

LLMaterialID::LLMaterialID() {}
LLMaterialID::LLMaterialID(LLMaterialID const &m) = default;
LLMaterialID::~LLMaterialID() {}
void LLMaterialID::set(void const*) { }
U8 const * LLMaterialID::get() const { return mID; }

LLPrimTextureList::LLPrimTextureList() { }
LLPrimTextureList::~LLPrimTextureList() { }
S32 LLPrimTextureList::setBumpMap(const U8 index, const U8 bump) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setOffsetS(const U8 index, const F32 s) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setOffsetT(const U8 index, const F32 t) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::copyTexture(const U8 index, const LLTextureEntry &te) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setRotation(const U8 index, const F32 r) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setBumpShiny(const U8 index, const U8 bump_shiny) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setFullbright(const U8 index, const U8 t) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setMaterialID(const U8 index, const LLMaterialID& pMaterialID) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setMediaFlags(const U8 index, const U8 media_flags) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setMediaTexGen(const U8 index, const U8 media) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setMaterialParams(const U8 index, const LLMaterialPtr pMaterialParams) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setBumpShinyFullbright(const U8 index, const U8 bump) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setID(const U8 index, const LLUUID& id) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setGlow(const U8 index, const F32 glow) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setAlpha(const U8 index, const F32 alpha) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setColor(const U8 index, const LLColor3& color) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setColor(const U8 index, const LLColor4& color) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setScale(const U8 index, const F32 s, const F32 t) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setScaleS(const U8 index, const F32 s) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setScaleT(const U8 index, const F32 t) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setShiny(const U8 index, const U8 shiny) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setOffset(const U8 index, const F32 s, const F32 t) { return TEM_CHANGE_NONE; }
S32 LLPrimTextureList::setTexGen(const U8 index, const U8 texgen) { return TEM_CHANGE_NONE; }

LLMaterialPtr LLPrimTextureList::getMaterialParams(const U8 index) { return LLMaterialPtr(); }
void LLPrimTextureList::copy(LLPrimTextureList const & ptl) { mEntryList = ptl.mEntryList; }
void LLPrimTextureList::take(LLPrimTextureList &other_list) { }
void LLPrimTextureList::setSize(S32 new_size) { mEntryList.resize(new_size); }
void LLPrimTextureList::setAllIDs(const LLUUID &id) { }
LLTextureEntry * LLPrimTextureList::getTexture(const U8 index) const { return nullptr; }
S32 LLPrimTextureList::size() const { return static_cast<S32>(mEntryList.size()); }

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
    using tut_compat::ensure;
    using tut_compat::ensure_equals;
    using tut_compat::ensure_not_equals;
    using tut_compat::set_test_name;

    struct llprimitive
    {
        PRIMITIVE_TEST_SETUP setup_class;
    };
}

TUT_SUITE("LLPrimitive")
{
    TUT_CASE("LLPrimitive::llprimitive_object_t_test_1")
    {
        using namespace tut;
        set_test_name("Test LLPrimitive Instantiation");
        LLPrimitive test;
    }

    TUT_CASE("LLPrimitive::llprimitive_object_t_test_2")
    {
        using namespace tut;
        set_test_name("Test LLPrimitive PCode setter and getter.");
        LLPrimitive test;
        ensure_equals(test.getPCode(), 0);
        LLPCode code = 1;
        test.setPCode(code);
        ensure_equals(test.getPCode(), code);
    }

    TUT_CASE("LLPrimitive::llprimitive_object_t_test_3")
    {
        using namespace tut;
        set_test_name("Test llprimitive constructor and initer.");
        LLPCode code = 1;
        LLPrimitive primitive;
        primitive.init_primitive(code);
        ensure_equals(primitive.getPCode(), code);
    }

    TUT_CASE("LLPrimitive::llprimitive_object_t_test_4")
    {
        using namespace tut;
        set_test_name("Test Static llprimitive constructor and initer.");
        LLPCode code = 1;
        LLPrimitive * primitive = LLPrimitive::createPrimitive(code);
        ensure(primitive != NULL);
        ensure_equals(primitive->getPCode(), code);
    }

    TUT_CASE("LLPrimitive::llprimitive_object_t_test_5")
    {
        using namespace tut;
        set_test_name("Test setVolume creation of new unique volume.");
        llprimitive data;
        LLPrimitive primitive;
        LLVolumeParams params;

        ensure(primitive.getVolume() == NULL);
        ensure_equals(primitive.getNumTEs(), 0);
        ensure(!primitive.isChanged(LLXform::GEOMETRY));
        ensure(primitive.setVolume(params, 0, true) == true);
        LLVolume* new_volume = primitive.getVolume();
        ensure(new_volume != NULL);
        ensure_not_equals(primitive.getNumTEs(), 0);
        ensure_equals(new_volume->getNumFaces(), 6);
        ensure_equals(primitive.getNumTEs(), new_volume->getNumFaces());
        ensure(primitive.isChanged(LLXform::GEOMETRY));
        ensure(primitive.setVolume(params, 0, true) == false);
        ensure(new_volume == primitive.getVolume());
        params.setRevolutions(4);
        ensure(primitive.setVolume(params, 0, true) == true);
        ensure(new_volume != primitive.getVolume());
    }

    TUT_CASE("LLPrimitive::llprimitive_object_t_test_6")
    {
        using namespace tut;
        set_test_name("Test setVolume creation of new NOT-unique volume.");
        llprimitive data;
        LLPrimitive primitive;
        LLVolumeParams params;

        ensure(primitive.getVolume() == NULL);
        ensure_equals(primitive.getNumTEs(), 0);
        ensure(!primitive.isChanged(LLXform::GEOMETRY));
        ensure(primitive.setVolume(params, 0, false) == true);
        LLVolume* new_volume = primitive.getVolume();
        ensure(new_volume != NULL);
        ensure_not_equals(primitive.getNumTEs(), 0);
        ensure_equals(new_volume->getNumFaces(), 6);
        ensure_equals(primitive.getNumTEs(), new_volume->getNumFaces());
        ensure(primitive.isChanged(LLXform::GEOMETRY));
        ensure(primitive.setVolume(params, 0, false) == false);
        ensure(new_volume == primitive.getVolume());
        params.setRevolutions(4);
        ensure(primitive.setVolume(params, 0, false) == true);
        ensure(new_volume != primitive.getVolume());
    }
}

#include "../tests/llmessagesystem_stub.cpp"
