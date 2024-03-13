#pragma once

#include <openxr/openxr.h>
#include "llcommon.h"
#include <vector>

struct LLXRView
{

};

class LLXRManager
{
    XrInstance mXRInstance;
    XrSystemId mSystemID;
    XrSession  mSession;
    std::vector<XrViewConfigurationView> mViewConfigs;

    bool       mInitialized = false;
  public:
    void initOpenXR();


};
