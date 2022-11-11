/**
 * @file llpuppetevent.cpp
 * @brief Implementation of LLPuppetEvent class.
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

#include "llpuppetmotion.h"
#include "llcharacter.h"
#include "llrand.h"
#include "m3math.h"
#include "v3dmath.h"
#include "llvoavatar.h"
#include "llviewerobjectlist.h" //gObjectList
#include <iomanip>

const std::string PUPPET_ROOT_JOINT_NAME("mPelvis");    //Name of the root joint
constexpr U32 PUPPET_MAX_EVENT_BYTES = 200;
U8      PUPPET_WRITE_BUFFER[PUPPET_MAX_EVENT_BYTES]; //HACK move this somewhere better.


// Helper function
// Note that the passed in vector is quantized
size_t pack_vec3(U8* wptr, LLVector3 &vec)
{
    size_t offset(0);

    // pack F32 components into 16 bits
    U16 x, y, z;
    vec.quantize16(-LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
    x = F32_to_U16(vec.mV[VX], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
    y = F32_to_U16(vec.mV[VY], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
    z = F32_to_U16(vec.mV[VZ], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);

    htolememcpy(wptr + offset, &x, MVT_U16, sizeof(U16));
    offset += sizeof(U16);
    htolememcpy(wptr + offset, &y, MVT_U16, sizeof(U16));
    offset += sizeof(U16);
    htolememcpy(wptr + offset, &z, MVT_U16, sizeof(U16));
    offset += sizeof(U16);

    return offset;
}

// Helper function
// Note that the passed in quaternion is quantized and possibly negated
size_t pack_quat(U8* wptr, LLQuaternion& quat)
{
    // A Quaternion is a 4D object but the group isomorphic with rotations is
    // limited to the surface of the unit hypersphere (radius = 1). Consequently
    // the quaternions we care about have only three degrees of freedom and
    // we can store them in three floats.  To do this we always make sure the
    // real component (W) is positive by negating the Quaternion as necessary
    // and then we store only the imaginary part (XYZ).  The real part can be
    // obtained with the formula: W = sqrt(1.0 - X*X + Y*Y + Z*Z)
    if (quat.mQ[VW] < 0.0f)
    {
        // negate the quaternion to keep its real part positive
        quat = -1.0f * quat;
    }
    // store the imaginary part
    size_t offset(0);

    // pack F32 components into 16 bits
    quat.quantize16(-LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
    U16 x = F32_to_U16(quat.mQ[VX], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
    U16 y = F32_to_U16(quat.mQ[VY], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
    U16 z = F32_to_U16(quat.mQ[VZ], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);

    htolememcpy(wptr + offset, &x, MVT_U16, sizeof(U16));
    offset += sizeof(U16);
    htolememcpy(wptr + offset, &y, MVT_U16, sizeof(U16));
    offset += sizeof(U16);
    htolememcpy(wptr + offset, &z, MVT_U16, sizeof(U16));
    offset += sizeof(U16);

    return offset;
}

// helper function - read LLVector3 from data
size_t unpack_vec3(U8* wptr, LLVector3& vec)
{
    U32 offset(0);
    U16 x, y, z;    // F32 data is packed in 16 bits

    htolememcpy(&x, wptr + offset, MVT_U16, sizeof(U16));
    offset += sizeof(U16);
    htolememcpy(&y, wptr + offset, MVT_U16, sizeof(U16));
    offset += sizeof(U16);
    htolememcpy(&z, wptr + offset, MVT_U16, sizeof(U16));
    offset += sizeof(U16);

    vec.mV[VX] = U16_to_F32(x, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
    vec.mV[VY] = U16_to_F32(y, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
    vec.mV[VZ] = U16_to_F32(z, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);

    return offset;
}

// helper function
size_t unpack_quat(U8* wptr, LLQuaternion& quat)
{
    // A packed Quaternion only includes the imaginary part (XYZ) and the
    // real part (W) is obtained with the formula:
    // W = sqrt(1.0 - X*X + Y*Y + Z*Z)
    size_t offset(0);

    U16 x, y, z;    // F32 data is packed into 16 bits
    htolememcpy(&x, wptr + offset, MVT_U16, sizeof(U16));
    offset += sizeof(U16);
    htolememcpy(&y, wptr + offset, MVT_U16, sizeof(U16));
    offset += sizeof(U16);
    htolememcpy(&z, wptr + offset, MVT_U16, sizeof(U16));
    offset += sizeof(U16);

    quat.mQ[VX] = U16_to_F32(x, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
    quat.mQ[VY] = U16_to_F32(y, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
    quat.mQ[VZ] = U16_to_F32(z, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);

    F32 imaginary_length_squared = quat.mQ[VX] * quat.mQ[VX]
            + quat.mQ[VY] * quat.mQ[VY] + quat.mQ[VZ] * quat.mQ[VZ];
    // DANGER: make sure we don't try to take the sqrt of a negative number.
    if (imaginary_length_squared > 1.0f)
    {
        quat.mQ[VW] = 0.0f;
        F32 imaginary_length = sqrtf(imaginary_length_squared);
        quat.mQ[VX] /= imaginary_length;
        quat.mQ[VY] /= imaginary_length;
        quat.mQ[VZ] /= imaginary_length;
    }
    else
    {
        quat.mQ[VW] = sqrtf(1.0f - imaginary_length_squared);
    }
    return offset;
}

void LLPuppetJointEvent::interpolate(F32 del, const LLPuppetJointEvent& A, const LLPuppetJointEvent& B)
{
    // copy all of A just in case B is incompatible
    mRotation = A.mRotation;
    mPosition = A.mPosition;
    mScale = A.mScale;
    mJointID = A.mJointID;
    mMask = A.mMask;

    // interpolate
    del = std::max(0.0f, std::min(1.0f, del)); // keep del in range [0,1]
    U8 mask = mMask & LLIK::MASK_ROT;
    if (mask && ((mMask & LLIK::MASK_ROT) == (B.mMask & LLIK::MASK_ROT)))
    {
        mRotation = slerp(del, A.mRotation, B.mRotation);
    }
    mask = mMask & LLIK::MASK_POS;
    if (mask && ((mMask & LLIK::MASK_POS) == (B.mMask & LLIK::MASK_POS)))
    {
        mPosition = (1.0f - del) * A.mPosition + del * B.mPosition;
    }
    if ((mMask & LLIK::FLAG_LOCAL_SCALE) && (B.mMask & LLIK::FLAG_LOCAL_SCALE))
    {
        mScale = (1.0f - del) * A.mScale + del * B.mScale;
    }
}

void LLPuppetJointEvent::setRotation(const LLQuaternion& rotation)
{
    mRotation = rotation;
    mRotation.normalize();
    mMask |= (mRefFrame == PARENT_FRAME ? LLIK::FLAG_LOCAL_ROT : LLIK::FLAG_TARGET_ROT);
}

void LLPuppetJointEvent::setPosition(const LLVector3& position)
{
    mPosition = position;
    mMask |= (mRefFrame == PARENT_FRAME ? LLIK::FLAG_LOCAL_POS : LLIK::FLAG_TARGET_POS);
}

void LLPuppetJointEvent::setScale(const LLVector3& scale)
{
    mScale = scale;
    mMask |= LLIK::FLAG_LOCAL_SCALE;
}

void LLPuppetJointEvent::setJointID(S32 id)
{
    mJointID = (S16)(id);
}

size_t LLPuppetJointEvent::getSize() const
{
    constexpr U32 BYTES_PER_VEC_3(3 * sizeof(F32));
    size_t num_bytes(0);
    num_bytes += sizeof(S16) + sizeof(S8);  // mJointID, mMask
    num_bytes += (mMask & LLIK::MASK_ROT) ? BYTES_PER_VEC_3 : 0;
    num_bytes += (mMask & LLIK::MASK_POS) ? BYTES_PER_VEC_3 : 0;
    num_bytes += (mMask & LLIK::FLAG_LOCAL_SCALE) ? BYTES_PER_VEC_3 : 0;
    return num_bytes;
}

size_t LLPuppetJointEvent::pack(U8* wptr)
{
    //Stuff everything into a binary blob to save overhead.
    size_t offset(0);

    htolememcpy(wptr, &mJointID, MVT_S16, sizeof(S16));
    offset += sizeof(S16);

    htolememcpy(wptr + offset, &mMask, MVT_U8, sizeof(U8));
    offset += sizeof(U8);

    //Pack these into the buffer in the same order as the flags.
    if (mMask & LLIK::MASK_ROT)
    {
        offset += pack_quat(wptr + offset, mRotation);
    }
    if (mMask & LLIK::MASK_POS)
    {
        offset += pack_vec3(wptr+offset, mPosition);
    }
    if (mMask & LLIK::FLAG_LOCAL_SCALE)
    {
        offset += pack_vec3(wptr+offset, mScale);
    }

    LL_DEBUGS("PUPPET_SPAM_PACK") << "Packed event for joint " << mJointID << " with flags 0x" << std::hex << static_cast<S32>(mMask) << std::dec << " into " << offset << " bytes.";
    if (mMask & LLIK::MASK_ROT)
        LL_CONT << " rot=" << mRotation;
    if (mMask & LLIK::MASK_POS)
        LL_CONT << " pos=" << mPosition;
    if (mMask & LLIK::FLAG_LOCAL_SCALE)
        LL_CONT << " scale=" << mScale;
    LL_CONT << " raw=" << LLError::arraylogger(wptr, offset) << " in frame " << (S32)gFrameCount << LL_ENDL;

    return offset;
}

size_t LLPuppetJointEvent::unpack(U8* wptr)
{
    htolememcpy(&mJointID, wptr, MVT_S16, sizeof(S16));
    size_t offset(sizeof(S16));

    htolememcpy(&mMask, wptr + offset, MVT_U8, sizeof(U8));
    offset += sizeof(U8);

    //Unpack in the same order as the flags.
    if (mMask & LLIK::MASK_ROT)
    {
        offset += unpack_quat(wptr+offset, mRotation);
    }
    if (mMask & LLIK::MASK_POS)
    {
        offset += unpack_vec3(wptr+offset, mPosition);
    }
    if (mMask & LLIK::FLAG_LOCAL_SCALE)
    {
        offset += unpack_vec3(wptr+offset, mScale);
    }

    LL_DEBUGS("PUPPET_SPAM_UNPACK") << "Unpacked event for joint " << mJointID << " with flags 0x" << std::hex << static_cast<S32>(mMask) << std::dec << " from " << offset << " bytes.";
    if (mMask & LLIK::MASK_ROT)
        LL_CONT << " rot=" << mRotation;
    if (mMask & LLIK::MASK_POS)
        LL_CONT << " pos=" << mPosition;
    if (mMask & LLIK::FLAG_LOCAL_SCALE)
        LL_CONT << " scale=" << mScale;
    LL_CONT << " raw=" << LLError::arraylogger(wptr, offset) << " in frame " << (S32)gFrameCount << LL_ENDL;

    return offset;
}

////////////////////////

S32 LLPuppetEvent::getMinEventSize() const
{
    // time, num and the size of the event buffer.
    S32 min_sz(sizeof(S32) + sizeof(S16) + sizeof(U32));

    if (mJointEvents.size() > 0)
    {
        min_sz = (S32)(mJointEvents.begin()->getSize());
    }
    return min_sz;
}

void LLPuppetEvent::addJointEvent(const LLPuppetJointEvent& joint_event)
{
    mJointEvents.push_back(joint_event);
}

bool  LLPuppetEvent::pack(LLDataPackerBinaryBuffer& buffer, S32& out_num_joints)
{
    //A PuppetEvent contains a timestamp and one or more joints with one or more actions applied to it.
    //Return value is true if we packed all joints into this event.    num_packed is set to number of joint events packed ok
    S16 num_joints(0);
    size_t buffer_size(buffer.getBufferSize() - buffer.getCurrentSize());
    bool result(true);

    static std::array<U8, PUPPET_MAX_EVENT_BYTES> scratch_buffer;

    // Accounting for time and num first
    size_t  len(sizeof(S32) + sizeof(S16) + sizeof(S32)); // extra S32 for binary data size.

    U8 *wptr = scratch_buffer.data();
    auto iter(mJointEvents.begin());
    size_t buf_sz(0);
    S32 total(mJointEvents.size());

    while (iter != mJointEvents.end())
    {
        if ((len + buf_sz + iter->getSize()) > buffer_size)
        {
            result = false;
            break;
        }

        size_t offset = iter->pack(wptr);
        ++num_joints;
        wptr += offset;
        buf_sz += offset;
        mJointEvents.pop_front();
        iter = mJointEvents.begin();
    }
    len += buf_sz;

    buffer.packS32(mTimestamp, "time");
    buffer.packS16(num_joints, "num");
    buffer.packBinaryData(scratch_buffer.data(), buf_sz, "data");

    out_num_joints = num_joints;
    LL_DEBUGS("PUPPET_SPAM") << "Packed " << num_joints << " joint events (of " << total << " to pack) into " << len << " byte block. " <<
        "Event data size is " << buf_sz << " in frame " << (S32) gFrameCount << LL_ENDL;

    return result;
}

bool LLPuppetEvent::unpack(LLDataPackerBinaryBuffer& buffer)
{
    U16 num_joints(0);
    if (!buffer.unpackS32(mTimestamp, "time"))
    {
        LL_DEBUGS("Puppet") << "Unable to unpack timestamp from puppetry packet." << LL_ENDL;
        return false;
    }
    if (!buffer.unpackU16(num_joints, "num"))
    {
        LL_DEBUGS("Puppet") << "Unable to unpack expected joint count from puppetry packet." << LL_ENDL;
        return false;
    }

    static std::array<U8, PUPPET_MAX_EVENT_BYTES> scratch_buffer;

    S32 buff_sz(scratch_buffer.size());

    if (!buffer.unpackBinaryData(scratch_buffer.data(), buff_sz, "data"))
    {
        LL_DEBUGS("Puppet") << "Unable to unpack puppetry payload data from puppetry packet." << LL_ENDL;
        return false;
    }
    U8* wptr(scratch_buffer.data());
    U32 offset(0);

    U32 index(0);
    for ( ; (index < num_joints) && (offset < buff_sz); ++index)
    {
        LLPuppetJointEvent jev;
        offset += jev.unpack(wptr+offset);
        mJointEvents.push_back(jev);
    }

    LL_DEBUGS_IF(index != num_joints, "Puppet") << "Unexpected joint count unpacking puppetry, expecting " << num_joints << ", only read " << index << LL_ENDL;
    LL_DEBUGS_IF(offset != buff_sz, "Puppet") << "Unread data in buffer. " << buff_sz << " bytes received, but only " << offset << " bytes used." << LL_ENDL;

    LL_DEBUGS("PUPPET_SPAM") << "Unpacked " << mJointEvents.size() << " joint events. event buffer size=" << buff_sz << " last offset=" << offset << " in frame " << (S32)gFrameCount << LL_ENDL;
    return (index == num_joints) && (offset == buff_sz);
}
