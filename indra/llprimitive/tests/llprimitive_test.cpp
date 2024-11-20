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

#include "linden_common.h"

#include "../test/lltut.h"

#include "../llprimitive.h"

#include "../../llmath/llvolumemgr.h"

#include "../llmaterialid.cpp"
#include "lltextureentry_stub.cpp"
#include "llprimtexturelist_stub.cpp"

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
    tut::llprimitive_t tut_llprimitive("LLPrimitive");

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
        ensure(primitive.setVolume(params, 0, true) == true);
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
        ensure(primitive.setVolume(params, 0, true) == false);
        ensure(new_volume == primitive.getVolume());

        // Change the param definition and try setting it again.
        params.setRevolutions(4);
        ensure(primitive.setVolume(params, 0, true) == true);

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
        ensure(primitive.setVolume(params, 0, false) == true);

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
        ensure(primitive.setVolume(params, 0, false) == false);
        ensure(new_volume == primitive.getVolume());

        // Change the param definition and try setting it again.
        params.setRevolutions(4);
        ensure(primitive.setVolume(params, 0, false) == true);

        // Ensure that we now have a different volume
        ensure(new_volume != primitive.getVolume());
    }

    template<> template<>
    void llprimitive_object_t::test<7>()
    {
        set_test_name("Test LLPrimitive pack/unpackTEMessageBuffer().");

        // init some values
        LLUUID image_id;
        LLUUID material_uuid;
        LLColor4 color(0.0f, 0.0f, 0.0f, 0.0f);
        F32 scale_s = 1.0;
        F32 scale_t = 1.0;
        S16 offset_s = 0;
        S16 offset_t = 0;
        S16 rot = 0;
        U8 bump = 0;
        U8 media_flags = 0;
        U8 glow = 0;
        U8 alpha_gamma = 31;

        // init some deltas
        LLColor4 d_color(0.05f, 0.07f, 0.11f, 0.13f);
        F32 d_scale_s = 0.1f;
        F32 d_scale_t = 0.3f;
        S16 d_offset_s = 5;
        S16 d_offset_t = 7;
        S16 d_rot = 11;
        U8 d_bump = 3;
        U8 d_media_flags = 5;
        U8 d_glow = 7;
        U8 d_alpha_gamma = 11;

        // prep the containers
        U8 num_textures = 5;
        LLPrimitive primitive_A;
        primitive_A.setNumTEs(num_textures);
        LLTextureEntry texture_entry;
        LLTEContents contents_A(num_textures);
        LLTEContents contents_B(num_textures);

        // fill contents_A and primitive_A
        for (U8 i = 0; i < num_textures; ++i)
        {
            // generate fake texture data
            image_id.generate();
            material_uuid.generate();
            color += d_color;
            scale_s += d_scale_s;
            scale_t -= d_scale_t;
            offset_s += d_offset_s;
            offset_t -= d_offset_t;
            rot += d_rot;
            bump += d_bump;
            media_flags += d_media_flags;
            glow += d_glow;
            alpha_gamma += d_alpha_gamma;

            // store the fake texture data in contents
            contents_A.image_ids[i] = image_id;

            LLMaterialID material_id;
            material_id.set(material_uuid.mData);
            contents_A.material_ids[i] = material_id;

            contents_A.colors[i].setVecScaleClamp(color);

            contents_A.scale_s[i] = scale_s;
            contents_A.scale_t[i] = scale_t;
            contents_A.offset_s[i] = offset_s;
            contents_A.offset_t[i] = offset_t;
            contents_A.rot[i] = rot;
            contents_A.bump[i] = bump;
            contents_A.glow[i] = glow;
            contents_A.media_flags[i] = media_flags & TEM_MEDIA_MASK;
            contents_A.alpha_gamma[i] = alpha_gamma;

            // store the fake texture data in texture_entry
            F32 f_offset_s = (F32)offset_s / (F32)0x7FFF;
            F32 f_offset_t = (F32)offset_t / (F32)0x7FFF;

            // Texture rotations are sent over the wire as a S16.  This is used to scale the actual float
            // value to a S16.   Don't use 7FFF as it introduces some odd rounding with 180 since it
            // can't be divided by 2.   See DEV-19108
            constexpr F32   TEXTURE_ROTATION_PACK_FACTOR = ((F32) 0x08000);
            F32 f_rotation = ((F32)rot / TEXTURE_ROTATION_PACK_FACTOR) * F_TWO_PI;

            F32 f_glow = (F32)glow / (F32)0xFF;

            texture_entry.init(image_id, scale_s, scale_t, f_offset_s, f_offset_t, f_rotation, bump, alpha_gamma);
            texture_entry.setMaterialID(material_id);
            texture_entry.setColor(color);
            texture_entry.setMediaFlags(media_flags);
            texture_entry.setGlow(f_glow);
            texture_entry.setAlphaGamma(alpha_gamma);

            // store texture_entry in primitive_A
            primitive_A.setTE(i, texture_entry);
        }

        // pack buffer from primitive_A
        constexpr size_t MAX_TE_BUFFER = 4096;
        U8 buffer[MAX_TE_BUFFER];
        S32 num_bytes = primitive_A.packTEMessageBuffer(buffer);
        ensure_not_equals(num_bytes, 0);

        // unpack buffer into contents_B
        // but first manually null-terminate the buffer as required by LLPrimitive::parseTEMessage()
        // because last TEField is not null-terminated in the message,
        // but it needs to be null-terminated for unpacking
        buffer[num_bytes] = 0;
        ++num_bytes;
        bool success = LLPrimitive::parseTEMessage(buffer, num_bytes, contents_B);
        ensure(success);

        // compare contents
        for (U8 i = 0; i < num_textures; ++i)
        {
            ensure_equals(contents_A.image_ids[i], contents_B.image_ids[i]);
            ensure_equals(contents_A.material_ids[i], contents_B.material_ids[i]);
            ensure_equals(contents_A.colors[i], contents_B.colors[i]);
            ensure_equals(contents_A.scale_s[i], contents_B.scale_s[i]);
            ensure_equals(contents_A.scale_t[i], contents_B.scale_t[i]);
            ensure_equals(contents_A.offset_s[i], contents_B.offset_s[i]);
            ensure_equals(contents_A.offset_t[i], contents_B.offset_t[i]);
            ensure_equals(contents_A.rot[i], contents_B.rot[i]);
            ensure_equals(contents_A.bump[i], contents_B.bump[i]);
            ensure_equals(contents_A.media_flags[i], contents_B.media_flags[i]);
            ensure_equals(contents_A.glow[i], contents_B.glow[i]);
            ensure_equals(contents_A.alpha_gamma[i], contents_B.alpha_gamma[i]);
        }

        // create primitive_B
        LLPrimitive primitive_B;
        primitive_B.setNumTEs(num_textures);

        // apply contents_B
        primitive_B.applyParsedTEMessage(contents_B);

        // compare primitives
        for (U8 i = 0; i < num_textures; ++i)
        {
            LLTextureEntry* te_A = primitive_A.getTE(i);
            LLTextureEntry* te_B = primitive_B.getTE(i);

            ensure_equals(te_A->getID(), te_B->getID());
            ensure_equals(te_A->getMaterialID(), te_B->getMaterialID());

            // color can experience quantization error after pack/unpack, so we check for proximity
            ensure(distVec(te_A->getColor(), te_B->getColor()) < 0.005f);

            // Note: ;scale, offset, and rotation can also experience a little quantization error
            // however it happens to be zero for the values we use in this test
            ensure_equals(te_A->getScaleS(), te_B->getScaleS());
            ensure_equals(te_A->getScaleT(), te_B->getScaleT());
            ensure_equals(te_A->getOffsetS(), te_B->getOffsetS());
            ensure_equals(te_A->getOffsetT(), te_B->getOffsetT());
            ensure_equals(te_A->getRotation(), te_B->getRotation());

            ensure_equals(te_A->getBumpShinyFullbright(), te_B->getBumpShinyFullbright());
            ensure_equals(te_A->getMediaFlags(), te_B->getMediaFlags());
            ensure_equals(te_A->getGlow(), te_B->getGlow());
            ensure_equals(te_A->getAlphaGamma(), te_B->getAlphaGamma());
        }
    }
}

#include "llmessagesystem_stub.cpp"
