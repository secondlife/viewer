/**
 * @file llvvmquery.cpp
 * @brief Query the Viewer Version Manager (VVM) for update information
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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
#include "llvvmquery.h"

#include "llcorehttputil.h"
#include "llcoros.h"
#include "llevents.h"
#include "llviewernetwork.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"
#include "llhasheduniqueid.h"
#include "lluri.h"
#include "llsys.h"

#if LL_VELOPACK
#include "llvelopack.h"
#endif

namespace
{
    std::string get_platform_string()
    {
#if LL_WINDOWS
        return "win64";
#elif LL_DARWIN
        return "mac64";
#elif LL_LINUX
        return "lnx64";
#else
        return "unknown";
#endif
    }

    std::string get_platform_version()
    {
        return LLOSInfo::instance().getOSVersionString();
    }

    std::string get_machine_id()
    {
        unsigned char id[MD5HEX_STR_SIZE];
        if (llHashedUniqueID(id))
        {
            return std::string(reinterpret_cast<char*>(id));
        }
        return "unknown";
    }

    void query_vvm_coro()
    {
        // Get base URL from grid manager
        std::string base_url = LLGridManager::getInstance()->getUpdateServiceURL();

        // We use this for dev testing when working with VVM and working on the updater.  Not advisable to uncomment it.
        //std::string base_url = "https://update.qa.secondlife.io/update";

        if (base_url.empty())
        {
            LL_WARNS("VVM") << "No update service URL configured" << LL_ENDL;
            return;
        }

        // Gather parameters for VVM query
        std::string channel = LLVersionInfo::instance().getChannel();

        // We use this for dev testing when working with VVM and working on the updater.  Not advisable to uncomment it.
        // std::string channel = "QA Target for Velopack";

        std::string version = LLVersionInfo::instance().getVersion();
        std::string platform = get_platform_string();
        std::string platform_version = get_platform_version();
        std::string test_ok = gSavedSettings.getBOOL("UpdaterWillingToTest") ? "testok" : "testno";
        std::string machine_id = get_machine_id();

        // Build URL: {base}/v1.2/{channel}/{version}/{platform}/{platform_version}/{testok}/{uuid}
        std::string url = base_url + "/v1.2/" +
            LLURI::escape(channel) + "/" +
            LLURI::escape(version) + "/" +
            platform + "/" +
            LLURI::escape(platform_version) + "/" +
            test_ok + "/" +
            machine_id;

        LL_INFOS("VVM") << "Querying VVM: " << url << LL_ENDL;

        // Make HTTP GET request
        LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
        LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t adapter =
            std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("VVMQuery", httpPolicy);
        LLCore::HttpRequest::ptr_t request = std::make_shared<LLCore::HttpRequest>();

        LLSD result = adapter->getAndSuspend(request, url);

        // Check HTTP status
        LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

        if (!status)
        {
            if (status.getType() == 404)
            {
                LL_INFOS("VVM") << "Unmanaged channel, no updates available" << LL_ENDL;
                return;
            }
            LL_WARNS("VVM") << "VVM query failed: " << status.toString() << LL_ENDL;
            return;
        }

        // Read whether this update is required or optional
        bool update_required = result["required"].asBoolean();
        std::string relnotes = result["more_info"].asString();

        // Extract update URL for current platform
        LLSD platforms = result["platforms"];
        if (platforms.has(platform))
        {
            std::string update_url = platforms[platform]["url"].asString();
#if LL_VELOPACK
            std::string velopack_url = platforms[platform]["velopack_url"].asString();
            if (!velopack_url.empty())
            {
                LL_INFOS("VVM") << "Velopack update URL: " << velopack_url
                                << " required: " << update_required << LL_ENDL;
                velopack_set_update_url(velopack_url);

                std::string update_version = result["version"].asString();
                LLCoros::instance().launch("VelopackUpdateCheck",
                    [update_required, update_version, relnotes]()
                    {
                        velopack_check_for_updates(update_required, update_version, relnotes);
                    });
            }
            else
#endif
            if (!update_url.empty())
            {
                LL_INFOS("VVM") << "Update available at: " << update_url << LL_ENDL;
            }
        }
        else
        {
            LL_INFOS("VVM") << "No update available for platform: " << platform << LL_ENDL;
        }

        // Post release notes URL to the relnotes event pump
        if (!relnotes.empty())
        {
            LL_INFOS("VVM") << "Release notes URL: " << relnotes << LL_ENDL;
            LLEventPumps::instance().obtain("relnotes").post(relnotes);
        }
    }
}

void initVVMUpdateCheck()
{
    LL_INFOS("VVM") << "Initializing VVM update check" << LL_ENDL;
    LLCoros::instance().launch("VVMUpdateCheck", &query_vvm_coro);
}
