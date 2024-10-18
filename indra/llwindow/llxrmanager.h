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
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "llrender.h"
#include "llsingleton.h"

glm::quat quatFromXrQuaternion(XrQuaternionf quat);
glm::vec3    vec3FromXrVector3(XrVector3f vec);

class LLRenderTarget;
class LLSwapchainXR;

class LLXRManager : public LLSimpleton<LLXRManager>
{

    XrInstance                                          mXRInstance = XR_NULL_HANDLE;
    XrSystemId                                          mSystemID = XR_NULL_SYSTEM_ID;
    XrSession                                           mSession = XR_NULL_HANDLE;
    XrSessionState                                      mSessionState = XR_SESSION_STATE_UNKNOWN;
    XrViewConfigurationType                             mViewConfig = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    // This will be the space that we're using from OpenXR.
    // - Local is akin to a "sitting" mode.
    // - Stage is akin to a "standing" or "roomscale" mode.
    // Should probably be set based upon the HMD's capabilities.
    XrReferenceSpaceType                                mAppSpace     = XR_REFERENCE_SPACE_TYPE_STAGE;
    XrFrameState                                        mFrameState     = { XR_TYPE_FRAME_STATE, nullptr };
    XrViewState                                         mViewState      = { XR_TYPE_VIEW_STATE, nullptr };
    XrSpace                                             mReferenceSpace = XR_NULL_HANDLE;
    XrSpace                                             mViewSpace      = XR_NULL_HANDLE;
    std::vector<XrViewConfigurationView>                mViewConfigViews;
    std::vector<XrView>                                 mViews;
    std::vector<XrCompositionLayerProjectionView>       mProjectionViews;
    std::vector<const char*>                            mActiveAPILayers;
    std::vector<const char*>                            mActiveInstanceExtensions;
    std::vector<std::string>                            mRequestedAPILayers;
    std::vector<std::string>                            mRequestedInstanceExtensions;
    std::vector<XrEnvironmentBlendMode>                 mSupportedBlendModes;
    std::vector<XrEnvironmentBlendMode>                 mApplicationEnvironmentBlendModes = { XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
                                                                                               XR_ENVIRONMENT_BLEND_MODE_ADDITIVE };
    XrEnvironmentBlendMode                              mEnvironmentBlendMode             = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    bool                                                mSessionRunning = false;
    enum class SwapchainType : uint8_t
    {
        COLOR,
        DEPTH
    };

    std::vector<LLSwapchainXR*> mSwapchains;

    std::vector<XrViewConfigurationType> mAppViewConfigurations = { XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                                             XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO };
    std::vector<XrViewConfigurationType> mViewConfigs;

    XrDebugUtilsMessengerEXT                mDebugMessenger;

    // There isn't really a reason to change this given I doubt we'll be bringing the desktop viewer to handhelds any time soon.
    // Keeping it here just in case.
    XrFormFactor                            mFormFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemProperties                      mSystemProperties = { XR_TYPE_SYSTEM_PROPERTIES };

    U32 mSwapchainLength = 1;

    bool mSwapchainInitialized = false;

  public:
    typedef enum
    {
        XR_STATE_UNINITIALIZED = 0,
        XR_STATE_INSTANCE_CREATED,
        XR_STATE_SESSION_CREATED,
        XR_STATE_RUNNING,
        XR_STATE_PAUSED,
        XR_STATE_DESTROYED = -1
    } LLXRState;

  private:
    LLXRState mXRState = XR_STATE_UNINITIALIZED;

    glm::vec3 mHeadPosition;
    glm::quat mHeadOrientation;

    std::vector<glm::quat>  mEyeRotations;
    std::vector<glm::vec3>  mEyePositions;
    std::vector<glm::mat4>  mEyeProjections;
    std::vector<glm::mat4>  mEyeViews;

    U32 mCurSwapTarget = 0;


  public:
    LLXRManager();
    ~LLXRManager();
    // This should always be called prior to attempting to create a session.
    void initInstance();

    void getInstanceProperties();

    void getSystemID();

    void getEnvironmentBlendModes();

    void getConfigurationViews();

    // This should only be called after initializing an instance.
    // Note that you will need platform specific bindings. OpenXR generally makes this pretty eays to do.
#ifdef LL_WINDOWS
    void createSession(XrGraphicsBindingOpenGLWin32KHR bindings);
#elif LL_LINUX
    void createSession(XrGraphicsBindingOpenGLXlibKHR bindings);
#elif LL_DARWIN
    void createSession(XrGraphicsRequirementsMetalKHR bindings);
#endif

    void createSwapchains();

    void destroySwapchains();
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

    void bindSwapTarget();

    void flushSwapTarget();

    typedef enum
    {
        XR_EYE_LEFT = 0,
        XR_EYE_RIGHT
    } LLXREye;

    // This will update the framebuffer for the given eye.
    void updateFrame();

    void endFrame();

    XrSession getXRSession() { return mSession; }
    XrInstance getXRInstance() { return mXRInstance; }

    LLXRState xrState() { return mXRState; }
    glm::vec3 getHeadPosition() { return mHeadPosition; }
    glm::quat getHeadOrientation() { return mHeadOrientation; }
    std::vector<glm::quat> getEyeRotations() { return mEyeRotations; }
    std::vector<glm::vec3>    getEyePositions() { return mEyePositions; }
    std::vector<glm::mat4>    getEyeProjections() { return mEyeProjections; }
    std::vector<glm::mat4>    getEyeViews() { return mEyeViews; }
    U32                       getSwapchainLength() { return mSwapchainLength; }

    U32 mCurrentEye = 0;
    F32 mZNear      = 0.1f;
    F32 mZFar       = 1000.0f;
};
