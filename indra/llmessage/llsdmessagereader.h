/** 
 * @file llsdmessagereader.h
 * @brief LLSDMessageReader class Declaration
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLSDMESSAGEREADER_H
#define LL_LLSDMESSAGEREADER_H

#include "llmessagereader.h"
#include "llsd.h"

#include <map>

class LLMessageTemplate;
class LLMsgData;

class LLSDMessageReader : public LLMessageReader
{
public:

    LLSDMessageReader();
    virtual ~LLSDMessageReader();

    /** All get* methods expect pointers to canonical strings. */
    virtual void getBinaryData(const char *block, const char *var, 
                               void *datap, S32 size, S32 blocknum = 0, 
                               S32 max_size = S32_MAX);
    virtual void getBOOL(const char *block, const char *var, BOOL &data, 
                         S32 blocknum = 0);
    virtual void getS8(const char *block, const char *var, S8 &data, 
                       S32 blocknum = 0);
    virtual void getU8(const char *block, const char *var, U8 &data, 
                       S32 blocknum = 0);
    virtual void getS16(const char *block, const char *var, S16 &data, 
                        S32 blocknum = 0);
    virtual void getU16(const char *block, const char *var, U16 &data, 
                        S32 blocknum = 0);
    virtual void getS32(const char *block, const char *var, S32 &data, 
                        S32 blocknum = 0);
    virtual void getF32(const char *block, const char *var, F32 &data, 
                        S32 blocknum = 0);
    virtual void getU32(const char *block, const char *var, U32 &data, 
                        S32 blocknum = 0);
    virtual void getU64(const char *block, const char *var, U64 &data, 
                        S32 blocknum = 0);
    virtual void getF64(const char *block, const char *var, F64 &data, 
                        S32 blocknum = 0);
    virtual void getVector3(const char *block, const char *var, 
                            LLVector3 &vec, S32 blocknum = 0);
    virtual void getVector4(const char *block, const char *var, 
                            LLVector4 &vec, S32 blocknum = 0);
    virtual void getVector3d(const char *block, const char *var, 
                             LLVector3d &vec, S32 blocknum = 0);
    virtual void getQuat(const char *block, const char *var, LLQuaternion &q, 
                         S32 blocknum = 0);
    virtual void getUUID(const char *block, const char *var, LLUUID &uuid, 
                         S32 blocknum = 0);
    virtual void getIPAddr(const char *block, const char *var, U32 &ip, 
                           S32 blocknum = 0);
    virtual void getIPPort(const char *block, const char *var, U16 &port, 
                           S32 blocknum = 0);
    virtual void getString(const char *block, const char *var, 
                           S32 buffer_size, char *buffer, S32 blocknum = 0);
    virtual void getString(const char *block, const char *var, std::string& outstr,
                           S32 blocknum = 0);

    virtual S32 getNumberOfBlocks(const char *blockname);
    virtual S32 getSize(const char *blockname, const char *varname);
    virtual S32 getSize(const char *blockname, S32 blocknum, 
                        const char *varname);

    virtual void clearMessage();

    virtual const char* getMessageName() const;
    virtual S32 getMessageSize() const;

    virtual void copyToBuilder(LLMessageBuilder&) const;

    /** Expects a pointer to a canonical name string */
    void setMessage(const char* name, const LLSD& msg);

private:
    const char* mMessageName; // Canonical (prehashed) string.
    LLSD mMessage;
};

#endif // LL_LLSDMESSAGEREADER_H
