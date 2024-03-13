#include "llxrmanager.h"
#include "linden_common.h"
#include "llgl.h"

#ifdef LL_WINDOWS

#define XR_USE_PLATFORM_WIN32
#include <windows.h>
#include <Unknwn.h>  // ...
#endif

#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

#define LL_HAND_COUNT 2
#define LL_HAND_LEFT_INDEX  0
#define LL_HAND_RIGHT_INDEX 1


static PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;

void LLXRManager::initOpenXR()
{
    // A lot of this uses Monado's OpenGL example which can be found here: https://gitlab.freedesktop.org/monado/demos/openxr-simple-example/

    // For the time being, OpenXR support is only really relevant on Windows and Linux.
    // macOS/visionOS does not have any support for OpenXR at present.  This may change via Monado or a compatibility initiative.
    #ifdef LL_WINDOWS || LL_LINUX
    XrFormFactor form_factor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrViewConfigurationType view_type   = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    XrReferenceSpaceType play_space_type = XR_REFERENCE_SPACE_TYPE_LOCAL;
    XrSpace              play_space      = XR_NULL_HANDLE;

    // the instance handle can be thought of as the basic connection to the OpenXR runtime
    mXRInstance = XR_NULL_HANDLE;
    // the system represents an (opaque) set of XR devices in use, managed by the runtime
    mSystemID = XR_NULL_SYSTEM_ID;
    // the session deals with the renderloop submitting frames to the runtime
    mSession = XR_NULL_HANDLE;

    XrGraphicsBindingOpenGLWin32KHR graphics_binding_gl = {};

    U32 view_count = 0;
    XrCompositionLayerProjectionView *projection_views = nullptr;

    XrView *views = nullptr;

    XrPath hand_paths[LL_HAND_COUNT];

    // reuse this variable for all our OpenXR return codes
    XrResult result = XR_SUCCESS;

    U32 ext_count = 0;
    result             = xrEnumerateInstanceExtensionProperties(nullptr, 0, &ext_count, nullptr);

    if (result != XR_SUCCESS)
    {
        LL_ERRS("XR Manager") << "Failed to enumerate the OpenXR runtime's extensions." << LL_ENDL;
        return;
    }

    // Allocate some extension properties against the number of reported extensions.  We'll iterate through these later.
    XrExtensionProperties *ext_props = new XrExtensionProperties[ext_count];
    for (U16 i = 0; i < ext_count; i++)
    {
        // we usually have to fill in the type (for validation) and set
        // next to NULL (or a pointer to an extension specific struct)
        ext_props[i].type = XR_TYPE_EXTENSION_PROPERTIES;
        ext_props[i].next = nullptr;
    }

    bool opengl_supported = false;

    LL_INFOS() << "Runtime supports " << ext_count << " extensions" << LL_ENDL;
    for (uint32_t i = 0; i < ext_count; i++)
    {
        LL_INFOS() << "Extension: " << ext_props[i].extensionName << " " << ext_props[i].extensionVersion << LL_ENDL;
        if (strcmp(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME, ext_props[i].extensionName) == 0)
        {
            opengl_supported = true;
        }
    }
    delete[] ext_props;

    if (!opengl_supported)
    {
        LL_ERRS("XR Manager") << "OpenGL is not supported by this OpenXR runtime.  Aborting OpenXR initialization." << LL_ENDL;
        return;
    }

    XrInstanceCreateInfo instCreateInfo    = {};
    S32         enabled_ext_count = 1;
    const char *enabled_exts[1]   = {XR_KHR_OPENGL_ENABLE_EXTENSION_NAME};

    instCreateInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
    instCreateInfo.next = NULL;
    instCreateInfo.createFlags = 0;
    instCreateInfo.enabledExtensionCount = enabled_ext_count;
    instCreateInfo.enabledExtensionNames = enabled_exts;
    instCreateInfo.enabledApiLayerCount  = 0;
    instCreateInfo.enabledApiLayerNames  = NULL;
    instCreateInfo.applicationInfo       = {
        "Second Life",
        1,
        "Second Life",
        0,
        XR_CURRENT_API_VERSION,
    };

    result = xrCreateInstance(&instCreateInfo, &mXRInstance);

    result = xrGetInstanceProcAddr(mXRInstance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction *) &pfnGetOpenGLGraphicsRequirementsKHR);

    if (result != XR_SUCCESS)
    {
        LL_ERRS("XR Manager") << "Unable to get OpenGL graphics requirements extension function pointer.  Aborting OpenXR initialization."
                              << LL_ENDL;
        return;
    }

    XrSystemGetInfo system_get_info = {
        XR_TYPE_SYSTEM_GET_INFO,
        nullptr,
        form_factor
    };

    result = xrGetSystem(mXRInstance, &system_get_info, &mSystemID);

    if (result != XR_SUCCESS)
    {
        LL_ERRS("XR Manager") << "Unable to get system information from OpenXR.  Aborting OpenXR initialization." << LL_ENDL;
        return;
    }

    {
        XrSystemProperties system_props = {
            XR_TYPE_SYSTEM_PROPERTIES,
            nullptr
        };

        result = xrGetSystemProperties(mXRInstance, mSystemID, &system_props);
        if (result != XR_SUCCESS)
        {
            LL_ERRS("XR Manager") << "Unable to retrieve system properties from OpenXR.  Aborting OpenXR initialization." << LL_ENDL;
            return;
        }
    }

    result = xrEnumerateViewConfigurationViews(mXRInstance, mSystemID, view_type, 0, &view_count, nullptr);
    if (result != XR_SUCCESS) {
        LL_ERRS("XR Manager") << "Unable to retrieve the view configuration for the system.  Aborting OpenXR initialization." << LL_ENDL;
        return;
    }

    for (U32 i = 0; i < view_count; i++)
    {
        XrViewConfigurationView viewconfig = {};
        viewconfig.type                    = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        viewconfig.next                    = nullptr;

        mViewConfigs.push_back(viewconfig);
    }

    result = xrEnumerateViewConfigurationViews(mXRInstance, mSystemID, view_type, view_count, &view_count, mViewConfigs.data());

    if (result != XR_SUCCESS)
    {
        LL_ERRS("XR Manager") << "Unable to enumerate the view configuration.  Aborting OpenXR initialization." << LL_ENDL;
        return;
    }

    XrGraphicsRequirementsOpenGLKHR opengl_reqs = 
    {
        XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR,
        nullptr
    };

    result = pfnGetOpenGLGraphicsRequirementsKHR(mXRInstance, mSystemID, &opengl_reqs);

    if (result != XR_SUCCESS)
    {
        LL_ERRS("XR Manager") << "Unable to get OpenGL graphics requirements.  Aborting OpenXR initialization." << LL_ENDL;
        return;
    }

    #ifdef LL_WINDOWS
    graphics_binding_gl = XrGraphicsBindingOpenGLWin32KHR {
        XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
    };
    #endif


    mInitialized = true;

    #elif LL_DARWIN
    LL_ERRS("XR Manager") << "Apple platforms, such as visionOS and macOS, are not presently supported.  Aborting XR initialization." << LL_ENDL;
    return;
    #endif
}
