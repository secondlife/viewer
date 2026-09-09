/**
 * @file llmessagejsonbuilder.cpp
 * @brief DEBUG/TESTING ONLY: build a message, or simulate receiving one,
 * from a JSON description.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#include "llmessagejsonbuilder.h"

#include <boost/json.hpp>

#include "message.h"
#include "llmessagetemplate.h"
#include "lltemplatemessagereader.h"
#include "llmessagebuilder.h"
#include "lluuid.h"
#include "llquaternion.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4math.h"
#include "net.h"

namespace
{
    // Parses an integer field value that may be a JSON number, or a JSON
    // string in decimal / "0x.." hex / "0b.." binary form, optionally
    // sign-prefixed, e.g. "12345", "0xabcd1234", "-0xabcd1234", "0b1110111".
    // The sign, if any, is applied by negating the parsed magnitude, giving
    // the correct two's-complement bit pattern once the caller truncates
    // `out` to the field's actual width.
    bool parseJsonInteger(const boost::json::value& v, U64& out)
    {
        if (v.is_int64())  { out = (U64)v.get_int64();  return true; }
        if (v.is_uint64()) { out = v.get_uint64();       return true; }
        if (v.is_double())  { out = (U64)(S64)v.get_double(); return true; }
        if (!v.is_string()) return false;

        std::string s(v.get_string().c_str());
        size_t pos = 0;
        bool negative = false;
        if (pos < s.size() && (s[pos] == '+' || s[pos] == '-'))
        {
            negative = (s[pos] == '-');
            ++pos;
        }

        int base = 10;
        if (pos + 1 < s.size() && s[pos] == '0' && (s[pos + 1] == 'x' || s[pos + 1] == 'X'))
        {
            base = 16;
            pos += 2;
        }
        else if (pos + 1 < s.size() && s[pos] == '0' && (s[pos + 1] == 'b' || s[pos + 1] == 'B'))
        {
            base = 2;
            pos += 2;
        }

        if (pos >= s.size()) return false;

        U64 magnitude = 0;
        for (; pos < s.size(); ++pos)
        {
            char c = s[pos];
            int digit;
            if (c >= '0' && c <= '9')      digit = c - '0';
            else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
            else if (c >= 'A' && c <= 'F') digit = 10 + (c - 'A');
            else return false;
            if (digit >= base) return false;
            magnitude = magnitude * (U64)base + (U64)digit;
        }

        out = negative ? (U64)(-(S64)magnitude) : magnitude;
        return true;
    }

    // Collects a JSON array of numbers as F64s. Used for vector/quaternion
    // fields, which are always "an array of floats".
    bool getNumberArray(const boost::json::value& v, std::vector<F64>& out)
    {
        if (!v.is_array()) return false;
        out.clear();
        for (const boost::json::value& e : v.get_array())
        {
            if (e.is_double())      out.push_back(e.get_double());
            else if (e.is_int64())  out.push_back((F64)e.get_int64());
            else if (e.is_uint64()) out.push_back((F64)e.get_uint64());
            else return false;
        }
        return true;
    }

    // Decodes "\xHH" escapes (produced after JSON's own string decoding,
    // so a literal backslash must already be doubled at the JSON layer) to
    // single raw bytes; every other character passes through as its raw
    // byte(s) unchanged. A plain string with no "\x" in it decodes to
    // itself, which is what makes a plain JSON string usable as-is for a
    // text field.
    void decodeBinaryEscapes(const std::string& in, std::vector<U8>& out)
    {
        out.clear();
        out.reserve(in.size());
        auto hex_val = [](char c) -> int
        {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
            if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
            return -1;
        };
        for (size_t i = 0; i < in.size(); )
        {
            if (in[i] == '\\' && i + 3 < in.size() && (in[i + 1] == 'x' || in[i + 1] == 'X'))
            {
                int hi = hex_val(in[i + 2]);
                int lo = hex_val(in[i + 3]);
                if (hi >= 0 && lo >= 0)
                {
                    out.push_back((U8)((hi << 4) | lo));
                    i += 4;
                    continue;
                }
            }
            out.push_back((U8)in[i]);
            ++i;
        }
    }

    // Every failure path below logs why and returns false. A field that
    // fails to convert must NOT be silently skipped: LLTemplateMessageBuilder
    // treats every declared field as required and hard-LL_ERRS's (aborting
    // the whole viewer) if buildMessage() finds one unset, so the only safe
    // response to a bad test payload is to abort the entire build before
    // buildMessage() is ever reached -- never partially build one.
    bool addBinaryField(LLMessageSystem* msg, const char* varname, const boost::json::value& v,
                         bool is_fixed, S32 fixed_size)
    {
        std::vector<U8> bytes;
        bool as_string = false;

        if (v.is_array())
        {
            for (const boost::json::value& e : v.get_array())
            {
                if (!e.is_int64() && !e.is_uint64())
                {
                    LL_WARNS("Messaging") << "LLMessageJsonBuilder: non-integer byte in binary array for field '"
                        << varname << "'" << LL_ENDL;
                    return false;
                }
                bytes.push_back((U8)(e.is_int64() ? e.get_int64() : (S64)e.get_uint64()));
            }
        }
        else if (v.is_string())
        {
            as_string = true;
            decodeBinaryEscapes(std::string(v.get_string().c_str()), bytes);
        }
        else
        {
            LL_WARNS("Messaging") << "LLMessageJsonBuilder: field '" << varname
                << "' needs a byte array or a string" << LL_ENDL;
            return false;
        }

        if (is_fixed)
        {
            if ((S32)bytes.size() != fixed_size)
            {
                LL_WARNS("Messaging") << "LLMessageJsonBuilder: field '" << varname << "' needs exactly "
                    << fixed_size << " bytes, got " << bytes.size() << LL_ENDL;
                return false;
            }
            msg->addBinaryDataFast(varname, bytes.data(), fixed_size);
        }
        else if (as_string)
        {
            // Plain text convention: include the trailing NUL a normal
            // string field would get, using the size-based overload so any
            // "\xHH"-decoded binary bytes still survive intact.
            msg->addStringFast(varname, std::string(bytes.begin(), bytes.end()));
        }
        else
        {
            msg->addBinaryDataFast(varname, bytes.empty() ? nullptr : bytes.data(), (S32)bytes.size());
        }
        return true;
    }

    bool addField(LLMessageSystem* msg, const char* varname, const boost::json::value& v,
                  EMsgVariableType type, S32 size)
    {
        switch (type)
        {
        case MVT_U8:
        case MVT_U16:
        case MVT_U32:
        case MVT_U64:
        case MVT_S8:
        case MVT_S16:
        case MVT_S32:
        {
            U64 n = 0;
            if (!parseJsonInteger(v, n))
            {
                LL_WARNS("Messaging") << "LLMessageJsonBuilder: bad integer for field '" << varname << "'" << LL_ENDL;
                return false;
            }
            switch (type)
            {
            case MVT_U8:  msg->addU8Fast(varname, (U8)n); break;
            case MVT_U16: msg->addU16Fast(varname, (U16)n); break;
            case MVT_U32: msg->addU32Fast(varname, (U32)n); break;
            case MVT_U64: msg->addU64Fast(varname, n); break;
            case MVT_S8:  msg->addS8Fast(varname, (S8)n); break;
            case MVT_S16: msg->addS16Fast(varname, (S16)n); break;
            case MVT_S32: msg->addS32Fast(varname, (S32)n); break;
            default: break;
            }
            return true;
        }

        case MVT_F32:
        case MVT_F64:
        {
            if (!v.is_double() && !v.is_int64() && !v.is_uint64())
            {
                LL_WARNS("Messaging") << "LLMessageJsonBuilder: bad float for field '" << varname << "'" << LL_ENDL;
                return false;
            }
            F64 f = v.is_double() ? v.get_double() : v.is_int64() ? (F64)v.get_int64() : (F64)v.get_uint64();
            if (type == MVT_F32) msg->addF32Fast(varname, (F32)f);
            else                 msg->addF64Fast(varname, f);
            return true;
        }

        case MVT_BOOL:
        {
            bool b = v.is_bool()    ? v.get_bool()
                   : v.is_int64()   ? (v.get_int64() != 0)
                   : v.is_uint64()  ? (v.get_uint64() != 0)
                   : v.is_double()  ? (v.get_double() != 0.0)
                   : false;
            msg->addBOOLFast(varname, b);
            return true;
        }

        case MVT_LLUUID:
        {
            if (!v.is_string())
            {
                LL_WARNS("Messaging") << "LLMessageJsonBuilder: field '" << varname << "' needs a UUID string" << LL_ENDL;
                return false;
            }
            LLUUID id;
            if (!LLUUID::parseUUID(std::string(v.get_string().c_str()), &id))
            {
                LL_WARNS("Messaging") << "LLMessageJsonBuilder: field '" << varname << "' is not a valid UUID" << LL_ENDL;
                return false;
            }
            msg->addUUIDFast(varname, id);
            return true;
        }

        case MVT_IP_ADDR:
        {
            U32 ip = 0;
            if (v.is_string())      ip = ip_string_to_u32(v.get_string().c_str());
            else if (v.is_int64())  ip = (U32)v.get_int64();
            else if (v.is_uint64()) ip = (U32)v.get_uint64();
            else
            {
                LL_WARNS("Messaging") << "LLMessageJsonBuilder: field '" << varname
                    << "' needs a number or a dotted-quad string" << LL_ENDL;
                return false;
            }
            msg->addIPAddrFast(varname, ip);
            return true;
        }

        case MVT_IP_PORT:
        {
            U64 n = 0;
            if (!parseJsonInteger(v, n))
            {
                LL_WARNS("Messaging") << "LLMessageJsonBuilder: bad port for field '" << varname << "'" << LL_ENDL;
                return false;
            }
            msg->addIPPortFast(varname, (U16)n);
            return true;
        }

        case MVT_LLVector3:
        case MVT_LLVector3d:
        case MVT_LLVector4:
        {
            std::vector<F64> nums;
            S32 wanted = (type == MVT_LLVector4) ? 4 : 3;
            if (!getNumberArray(v, nums) || (S32)nums.size() != wanted)
            {
                LL_WARNS("Messaging") << "LLMessageJsonBuilder: field '" << varname << "' needs an array of "
                    << wanted << " floats" << LL_ENDL;
                return false;
            }
            if (type == MVT_LLVector3)
            {
                F32 arr[3] = { (F32)nums[0], (F32)nums[1], (F32)nums[2] };
                msg->addVector3Fast(varname, LLVector3(arr));
            }
            else if (type == MVT_LLVector3d)
            {
                F64 arr[3] = { nums[0], nums[1], nums[2] };
                msg->addVector3dFast(varname, LLVector3d(arr));
            }
            else
            {
                F32 arr[4] = { (F32)nums[0], (F32)nums[1], (F32)nums[2], (F32)nums[3] };
                msg->addVector4Fast(varname, LLVector4(arr));
            }
            return true;
        }

        case MVT_LLQuaternion:
        {
            std::vector<F64> nums;
            if (!getNumberArray(v, nums) || (nums.size() != 3 && nums.size() != 4))
            {
                LL_WARNS("Messaging") << "LLMessageJsonBuilder: field '" << varname
                    << "' needs an array of 3 or 4 floats" << LL_ENDL;
                return false;
            }
            F32 x = (F32)nums[0], y = (F32)nums[1], z = (F32)nums[2];
            F32 w = (nums.size() == 4) ? (F32)nums[3]
                                        : sqrtf(llmax(0.f, 1.f - x * x - y * y - z * z));
            msg->addQuatFast(varname, LLQuaternion(x, y, z, w));
            return true;
        }

        case MVT_VARIABLE:
            return addBinaryField(msg, varname, v, false, size);

        case MVT_FIXED:
            return addBinaryField(msg, varname, v, true, size);

        default:
            LL_WARNS("Messaging") << "LLMessageJsonBuilder: field '" << varname
                << "' has unsupported wire type " << (S32)type << " -- skipped" << LL_ENDL;
            return false;
        }
    }
}

bool LLMessageJsonBuilder::build(LLMessageSystem* msg, const std::string& msg_name,
                                  const boost::json::value& body)
{
    if (!body.is_object())
    {
        LL_WARNS("Messaging") << "LLMessageJsonBuilder: body for '" << msg_name << "' is not a JSON object" << LL_ENDL;
        return false;
    }

    char* name = LLMessageStringTable::getInstance()->getString(msg_name.c_str());
    LLMessageSystem::message_template_name_map_t::const_iterator tmpl_it = msg->mMessageTemplates.find(name);
    if (tmpl_it == msg->mMessageTemplates.end())
    {
        LL_WARNS("Messaging") << "LLMessageJsonBuilder: unknown message '" << msg_name << "'" << LL_ENDL;
        return false;
    }
    const LLMessageTemplate* msg_template = tmpl_it->second;

    // Every Single/Multiple block the template declares is required by
    // LLTemplateMessageBuilder -- it hard-LL_ERRS's (aborting the whole
    // viewer, not just this call) if buildMessage() below finds one whose
    // fields were never set. A block entirely missing from `body` would
    // never even enter the main loop below to be caught there, so check for
    // that up front; per-field completeness for a block that *is* present
    // is checked in the main loop, for every instance, further down.
    for (LLMessageBlock* block_template_item : msg_template->mMemberBlocks)
    {
        if (block_template_item->mType == MBT_VARIABLE)
        {
            continue; // 0 instances is valid for a repeated/optional block
        }
        std::string block_name(block_template_item->mName);
        if (!body.as_object().if_contains(block_name))
        {
            LL_WARNS("Messaging") << "LLMessageJsonBuilder: message '" << msg_name
                << "' is missing required block '" << block_name << "'" << LL_ENDL;
            return false;
        }
    }

    msg->newMessageFast(name);

    for (const boost::json::key_value_pair& block_kv : body.as_object())
    {
        std::string block_name(block_kv.key().data(), block_kv.key().size());
        if (block_name == "Name") continue;

        char* block_name_interned = LLMessageStringTable::getInstance()->getString(block_name.c_str());
        const LLMessageBlock* block_template = msg_template->getBlock(block_name_interned);
        if (!block_template)
        {
            LL_WARNS("Messaging") << "LLMessageJsonBuilder: unknown block '" << block_name
                << "' in message '" << msg_name << "'" << LL_ENDL;
            return false;
        }

        const boost::json::value& block_val = block_kv.value();
        bool is_array = block_val.is_array();
        size_t num_instances = is_array ? block_val.get_array().size() : 1;

        if (block_template->mType == MBT_MULTIPLE && (S32)num_instances != block_template->mNumber)
        {
            LL_WARNS("Messaging") << "LLMessageJsonBuilder: block '" << block_name << "' in message '" << msg_name
                << "' needs exactly " << block_template->mNumber << " instance(s), got " << num_instances << LL_ENDL;
            return false;
        }

        for (size_t i = 0; i < num_instances; ++i)
        {
            msg->nextBlockFast(block_name_interned);
            const boost::json::value& block_data = is_array ? block_val.get_array()[i] : block_val;
            if (!block_data.is_object())
            {
                LL_WARNS("Messaging") << "LLMessageJsonBuilder: instance of block '" << block_name
                    << "' is not a JSON object" << LL_ENDL;
                return false;
            }

            for (LLMessageVariable* var_template_item : block_template->mMemberVariables)
            {
                if (!block_data.as_object().if_contains(var_template_item->getName()))
                {
                    LL_WARNS("Messaging") << "LLMessageJsonBuilder: instance of block '" << block_name
                        << "' in message '" << msg_name << "' is missing required field '"
                        << var_template_item->getName() << "'" << LL_ENDL;
                    return false;
                }
            }

            for (const boost::json::key_value_pair& field_kv : block_data.as_object())
            {
                std::string var_name(field_kv.key().data(), field_kv.key().size());
                char* var_name_interned = LLMessageStringTable::getInstance()->getString(var_name.c_str());
                const LLMessageVariable* var_template = block_template->getVariable(var_name_interned);
                if (!var_template)
                {
                    LL_WARNS("Messaging") << "LLMessageJsonBuilder: unknown field '" << var_name
                        << "' in block '" << block_name << "'" << LL_ENDL;
                    return false;
                }
                if (!addField(msg, var_name_interned, field_kv.value(), var_template->getType(), var_template->getSize()))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool LLMessageJsonBuilder::simulateReceived(LLMessageSystem* msg, const std::string& msg_name,
                                             const boost::json::value& body, const LLHost& host)
{
    if (!build(msg, msg_name, body))
    {
        return false;
    }

    U8 buffer[MAX_BUFFER_SIZE];
    U32 size = msg->mMessageBuilder->buildMessage(buffer, MAX_BUFFER_SIZE, 0);

    if (!msg->mTemplateMessageReader->validateMessage(buffer, (S32)size, host, true))
    {
        LL_WARNS("Messaging") << "LLMessageJsonBuilder: built message for '" << msg_name
            << "' failed to validate" << LL_ENDL;
        return false;
    }
    msg->mTemplateMessageReader->readMessage(buffer, host);

    LockMessageReader lock(msg->mMessageReader, msg->mTemplateMessageReader);
    return msg->callHandler(LLMessageStringTable::getInstance()->getString(msg_name.c_str()), true, msg);
}
