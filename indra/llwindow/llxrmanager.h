#pragma once

#ifdef LL_WINDOWS
#define XR_USE_PLATFORM_WIN32
#include <windows.h>
#include <Unknwn.h>
#endif

#define XR_USE_GRAPHICS_API_OPENGL

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include "llcommon.h"
#include "llmath.h"
#include "v3math.h"
#include "llquaternion.h"
#include <vector>

class LLXRManager
{
    XrInstance                              mXRInstance = XR_NULL_HANDLE;
    XrSystemId                              mSystemID = XR_NULL_SYSTEM_ID;
    XrSession                               mSession = XR_NULL_HANDLE;
    XrSessionState                          mSessionState = XR_SESSION_STATE_UNKNOWN;

    // This will be the space that we're using from OpenXR.
    // - Local is akin to a "sitting" mode.
    // - Stage is akin to a "standing" or "roomscale" mode.
    // Should probably be set based upon the HMD's capabilities.
    XrReferenceSpaceType                    mAppSpace     = XR_REFERENCE_SPACE_TYPE_STAGE;

    XrSpace                                 mReferenceSpace = XR_NULL_HANDLE;
    XrSpace                                 mViewSpace      = XR_NULL_HANDLE;
    std::vector<XrViewConfigurationView>    mViewConfigs;
    std::vector<XrView>                     mViews;
    std::vector<XrCompositionLayerProjectionView> mProjectionViews;
    std::vector<const char*>                mActiveAPILayers;
    std::vector<const char*>                mActiveInstanceExtensions;
    std::vector<const char*>                mRequestedAPILayers;
    std::vector<const char*>                mRequestedInstanceExtensions;
    bool                                    mSessionRunning = false;

    XrDebugUtilsMessengerEXT                mDebugMessenger;

    // There isn't really a reason to change this given I doubt we'll be bringing the desktop viewer to handhelds any time soon.
    // Keeping it here just in case.
    XrFormFactor                            mFormFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemProperties                      mSystemProperties = { XR_TYPE_SYSTEM_PROPERTIES };

    // Change this to go between mono and stereo rendering.  In the future we'll likely have this change based upon what your HMD supports.
    // Some examples of this are:
    // - Foveated rendering
    // - Varjo's weird quad display system
    // - Generally future display tech in HMDs
    XrViewConfigurationType                 mViewConfigType   = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

  public:
    typedef enum
    {
        XR_STATE_UNINITIALIZED,
        XR_STATE_INSTANCE_CREATED,
        XR_STATE_SESSION_CREATED,
        XR_STATE_RUNNING,
        XR_STATE_PAUSED,
        XR_STATE_DESTROYED
    } LLXRState;

  private:
    LLXRState mInitialized = XR_STATE_UNINITIALIZED;

    LLVector3 mHeadPosition;
    LLQuaternion mHeadOrientation;

  public:
    // This should always be called prior to attempting to create a session.
    void initInstance();

    // This should only be called after initializing an instance.
    // Note that you will need platform specific bindings. OpenXR generally makes this pretty eays to do.
#ifdef LL_WINDOWS
    void createSession(XrGraphicsBindingOpenGLWin32KHR bindings);
#elif LL_LINUX
    void createSession(XrGraphicsBindingOpenGLXlibKHR bindings);
#elif LL_DARWIN
    void createSession(XrGraphicsRequirementsMetalKHR bindings);
#endif
    // This should be called when the viewer is shutting down.
    // This will destroy the session and instance, and set the XR manager to an uninitialized state.
    void shutdown();

    // This sets up the play space.  See mAppSpace. Should be called after creating a session.
    void setupPlaySpace();

    void startFrame();

    // This pretty much goes through and polls all of our events, and handles any session state changes.
    void handleSessionState();

    // This should be called every frame to update the session.
    // This is where we get pose data, etc.
    void updateXRSession();

    void endFrame();

    LLXRState xrState() { return mInitialized; }
    LLVector3 getHeadPosition() { return mHeadPosition; }
    LLQuaternion getHeadOrientation() { return mHeadOrientation; }
};
