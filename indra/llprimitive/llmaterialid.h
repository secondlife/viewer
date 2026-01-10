/**
* @file   llmaterialid.h
* @brief  Header file for llmaterialid
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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
#ifndef LL_LLMATERIALID_H
#define LL_LLMATERIALID_H

#define MATERIAL_ID_SIZE 16

#include <string>
#include "llsd.h"

class LLMaterialID
{
public:
    LLMaterialID();
    LLMaterialID(const LLSD& pMaterialID);
    LLMaterialID(const LLSD::Binary& pMaterialID);
    LLMaterialID(const void* pMemory);
    LLMaterialID(const LLMaterialID& pOtherMaterialID);
    LLMaterialID(const LLUUID& lluid);
    ~LLMaterialID();

    bool          operator == (const LLMaterialID& pOtherMaterialID) const;
    bool          operator != (const LLMaterialID& pOtherMaterialID) const;

    bool          operator < (const LLMaterialID& pOtherMaterialID) const;
    bool          operator <= (const LLMaterialID& pOtherMaterialID) const;
    bool          operator > (const LLMaterialID& pOtherMaterialID) const;
    bool          operator >= (const LLMaterialID& pOtherMaterialID) const;

    LLMaterialID& operator = (const LLMaterialID& pOtherMaterialID);

    bool          isNull() const;

    const U8*     get() const;
    void          set(const void* pMemory);
    void          clear();

    LLSD          asLLSD() const;
    std::string   asString() const;
    LLUUID        asUUID() const;

    friend std::ostream& operator<<(std::ostream& s, const LLMaterialID &material_id);

    static const LLMaterialID null;

private:
    // definitions follow class
    friend std::hash<LLMaterialID>;
    friend size_t hash_value(const LLMaterialID&) noexcept;

    void parseFromBinary(const LLSD::Binary& pMaterialID);
    void copyFromOtherMaterialID(const LLMaterialID& pOtherMaterialID);
    int  compareToOtherMaterialID(const LLMaterialID& pOtherMaterialID) const;

    U8 mID[MATERIAL_ID_SIZE];
} ;

// std::hash implementation for LLMaterialID
namespace std
{
    template<> struct hash<LLMaterialID>
    {
        inline size_t operator()(const LLMaterialID& id) const noexcept
        {
            size_t h = 0;
            // Golden ratio hash with avalanche mixing
            // Process 8 bytes at a time by manually constructing 64-bit values
            // Shift by 31: mixes upper half into lower half for better bit distribution
            // Shift by 47: ensures highest bits influence final hash output
            for (int i = 0; i < MATERIAL_ID_SIZE; i += 8) {
                size_t chunk = (size_t)id.mID[i] | ((size_t)id.mID[i + 1] << 8) |
                               ((size_t)id.mID[i+2] << 16) | ((size_t)id.mID[i+3] << 24) |
                               ((size_t)id.mID[i+4] << 32) | ((size_t)id.mID[i+5] << 40) |
                               ((size_t)id.mID[i + 6] << 48) | ((size_t)id.mID[i + 7] << 56);
                h ^= (chunk * 0x9e3779b97f4a7c15ULL) ^ (h >> 31) ^ (h >> 47);
            }
            return h;
        }
    };
}

// For use with boost::container_hash
inline size_t hash_value(const LLMaterialID& id) noexcept
{
    return std::hash<LLMaterialID>{}(id);
}

#endif // LL_LLMATERIALID_H

