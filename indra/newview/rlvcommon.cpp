#include "llviewerprecompiledheaders.h"
#include "llversioninfo.h"

#include "rlvdefines.h"
#include "rlvcommon.h"

using namespace Rlv;

// ============================================================================
// RlvStrings
//

std::string RlvStrings::getVersion(bool wants_legacy)
{
    return llformat("%s viewer v%d.%d.%d (RLVa %d.%d.%d)",
        !wants_legacy ? "RestrainedLove" : "RestrainedLife",
        SpecVersion::Major, SpecVersion::Minor, SpecVersion::Patch,
        ImplVersion::Major, ImplVersion::Minor, ImplVersion::Patch);
}

std::string RlvStrings::getVersionAbout()
{
    return llformat("RLV v%d.%d.%d / RLVa v%d.%d.%d.%d",
        SpecVersion::Major, SpecVersion::Minor, SpecVersion::Patch,
        ImplVersion::Major, ImplVersion::Minor, ImplVersion::Patch, LLVersionInfo::instance().getBuild());
}

std::string RlvStrings::getVersionNum()
{
    return llformat("%d%02d%02d%02d",
        SpecVersion::Major, SpecVersion::Minor, SpecVersion::Patch, SpecVersion::Build);
}

std::string RlvStrings::getVersionImplNum()
{
    return llformat("%d%02d%02d%02d",
        ImplVersion::Major, ImplVersion::Minor, ImplVersion::Patch, ImplVersion::ImplId);
}

// ============================================================================
