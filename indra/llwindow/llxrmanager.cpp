#include "llxrmanager.h"
#include "linden_common.h"
#include "llgl.h"
#include "llrender.h"

LLMatrix4 OXR_TO_SFR = LLMatrix4(OGL_TO_CFR_ROTATION);

#define LL_HAND_COUNT 2
#define LL_HAND_LEFT_INDEX  0
#define LL_HAND_RIGHT_INDEX 1


static PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;

void LLXRManager::initInstance()
{
    // A lot of this uses Monado's OpenGL example which can be found here: https://gitlab.freedesktop.org/monado/demos/openxr-simple-example/

    // For the time being, OpenXR support is only really relevant on Windows and Linux.
    // macOS/visionOS does not have any support for OpenXR at present.  This may change via Monado or a compatibility initiative.
#if defined(LL_WINDOWS) || defined(LL_LINUX)

    XrApplicationInfo app_info = {
        "Second Life Viewer",
        1,
        "Second Life",
        0,
        XR_API_VERSION_1_0, // Note: SteamVR likes OpenXR 1.0.  It may support additional versions, but don't use XR_CURRENT_API_VERSION.  It doesn't work as of 2024-09-30.
    };

    // Setup our instance extensions and API layers.
    // We have two lists you should push to - the code below will check if they're available and enable them if they are.

    // mRequestedInstanceExtensions are the extensions that we want to use.
    // mRequestedAPILayers are the API layers that we want to use.

    // If the extensions are found in the current OpenXR runtime, they will be enabled.  Same applies for API layers.
    // Please note that you should check if the extensions/API layers were found and error handle as needed.

    #if defined(LL_DEBUG)
    mRequestedInstanceExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif
    mRequestedInstanceExtensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);

    U32 apiLayerCount = 0;
    std::vector<XrApiLayerProperties> apiLayerProperties;

    S32 err = xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr);
    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to retrieve API layers.  Error code: " << err << LL_ENDL;
        return;
    }

    // Get our API layer properties.

    apiLayerProperties.resize(apiLayerCount, { XR_TYPE_API_LAYER_PROPERTIES });

    err = xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount, apiLayerProperties.data());

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to enumerate API layers.  Error code: " << err << LL_ENDL;
        return;
    }

    for (auto& requestedLayer : mRequestedAPILayers)
    {
        bool found = false;
        for (auto& layer : apiLayerProperties)
        {
            if (strcmp(requestedLayer, layer.layerName) == 0)
            {
                found = true;
                mActiveAPILayers.push_back(requestedLayer);
                break;
            }
        }

        if (!found)
        {
            LL_WARNS("XRManager") << "Requested API layer " << requestedLayer << " was not found." << LL_ENDL;
        }
    }

    U32 extensionCount = 0;
    std::vector<XrExtensionProperties> extensionProperties;
    err = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to retrieve instance extensions.  Error code: " << err << LL_ENDL;
        return;
    }

    // Get our extension properties for this instance.

    extensionProperties.resize(extensionCount, { XR_TYPE_EXTENSION_PROPERTIES });

    err = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProperties.data());

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to enumerate instance extensions.  Error code: " << err << LL_ENDL;
        return;
    }

    for (auto &requestedInstanceExt : mRequestedInstanceExtensions)
    {
        bool found = false;
        for (auto& extProper : extensionProperties)
        {
            if (strcmp(requestedInstanceExt,extProper.extensionName) == 0)
            {
                found = true;
                mActiveInstanceExtensions.push_back(requestedInstanceExt);
                break;
            }
        }

        if (!found)
        {
            LL_WARNS("XRManager") << "Requested instance extension " << requestedInstanceExt << " was not found." << LL_ENDL;
        }
    }

    // Create our instance.

    XrInstanceCreateInfo instanceCreateInfo = {
        XR_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        0,
        app_info,
        static_cast<U32>(mActiveAPILayers.size()),
        mActiveAPILayers.data(),
        static_cast<U32>(mActiveInstanceExtensions.size()),
        mActiveInstanceExtensions.data()
    };

    err = xrCreateInstance(&instanceCreateInfo, &mXRInstance);

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to create OpenXR instance.  Error code: " << err << LL_ENDL;
        return;
    }

    // Get our instance properties.

    XrInstanceProperties instanceProperties = { XR_TYPE_INSTANCE_PROPERTIES };

    err = xrGetInstanceProperties(mXRInstance, &instanceProperties);

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to retrieve instance properties.  Error code: " << err << LL_ENDL;
        return;
    }
    else
    {
        LL_INFOS("XRManager") << "OpenXR runtime: " << instanceProperties.runtimeName << " version " << instanceProperties.runtimeVersion
                               << LL_ENDL;
    }

    // Get our system info.

    XrSystemGetInfo systemGI{ XR_TYPE_SYSTEM_GET_INFO };
    systemGI.formFactor = mFormFactor;

    err = xrGetSystem(mXRInstance, &systemGI, &mSystemID);

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to retrieve system ID.  Error code: " << err << LL_ENDL;
        return;
    }

    err = xrGetSystemProperties(mXRInstance, mSystemID, &mSystemProperties);

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to retrieve system properties.  Error code: " << err << LL_ENDL;
        return;
    }
    else
    {
        LL_INFOS("XRManager") << "System properties: " << LL_ENDL;
        LL_INFOS("XRManager") << " - Vendor ID: " << mSystemProperties.vendorId << LL_ENDL;
        LL_INFOS("XRManager") << " - System ID: " << mSystemProperties.systemId << LL_ENDL;
        LL_INFOS("XRManager") << " - Max layer count: " << mSystemProperties.graphicsProperties.maxLayerCount << LL_ENDL;
        LL_INFOS("XRManager") << " - Max swapchain image height: " << mSystemProperties.graphicsProperties.maxSwapchainImageHeight << LL_ENDL;
        LL_INFOS("XRManager") << " - Max swapchain image width: " << mSystemProperties.graphicsProperties.maxSwapchainImageWidth << LL_ENDL;
        LL_INFOS("XRManager") << " - Supports orientation tracking: " << mSystemProperties.trackingProperties.orientationTracking
                              << LL_ENDL;
        LL_INFOS("XRManager") << " - Supports position tracking: " << mSystemProperties.trackingProperties.positionTracking
                              << LL_ENDL;
    }

    // Get our view configurations, and setup our views.
    U32 viewCount;
    err = xrEnumerateViewConfigurationViews(mXRInstance, mSystemID, mViewConfigType, 0, &viewCount, nullptr);

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to enumerate view configuration views.  Error code: " << err << LL_ENDL;
        return;
    }

    mViewConfigs.resize(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW, nullptr });

    mViews.resize(viewCount, { XR_TYPE_VIEW, nullptr });

    mProjectionViews.resize(viewCount, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW, nullptr });

    err = xrEnumerateViewConfigurationViews(mXRInstance, mSystemID, mViewConfigType, viewCount, &viewCount, mViewConfigs.data());

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to enumerate view configuration views.  Error code: " << err << LL_ENDL;
        return;
    }

#elif LL_DARWIN
    LL_ERRS("XRManager") << "Apple platforms, such as visionOS and macOS, are not presently supported.  Aborting XR initialization." << LL_ENDL;
    return;
#endif
    
    mInitialized = XR_STATE_INSTANCE_CREATED;
}

#ifdef LL_WINDOWS
void LLXRManager::createSession(XrGraphicsBindingOpenGLWin32KHR graphicsBinding)
#elif LL_LINUX
void LLXRManager::createSession(XrGraphicsBindingOpenGLXlibKHR graphicsBinding)
#elif LL_DARWIN
void LLXRManager::createSession(XrGraphicsRequirementsMetalKHR graphicsBinding)
#endif
{
#ifdef LL_DARWIN
    LL_WARNS("XRManager") << "Metal is not presently supported.  Aborting XR initialization." << LL_ENDL;
    return;
#endif

    if (mInitialized != XR_STATE_INSTANCE_CREATED)
    {
        LL_ERRS("XRManager") << "Cannot create session without an instance." << LL_ENDL;
        return;
    }

    XrResult result =
        xrGetInstanceProcAddr(mXRInstance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&pfnGetOpenGLGraphicsRequirementsKHR);
    if (!XR_SUCCEEDED(result))
    {
        LL_ERRS("XRManager") << "Failed to get xrGetOpenGLGraphicsRequirementsKHR function pointer." << LL_ENDL;
        return;
    }

    XrGraphicsRequirementsOpenGLKHR graphicsRequirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };

    S32 err = pfnGetOpenGLGraphicsRequirementsKHR(mXRInstance, mSystemID, &graphicsRequirements);

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to retrieve OpenGL graphics requirements.  Error code: " << err << LL_ENDL;
        return;
    }
    else
    {
        LL_INFOS("XRManager") << "OpenGL graphics requirements: " << LL_ENDL;
        LL_INFOS("XRManager") << " - Min API version: " << graphicsRequirements.minApiVersionSupported << LL_ENDL;
        LL_INFOS("XRManager") << " - Max API version: " << graphicsRequirements.maxApiVersionSupported << LL_ENDL;
    }

    XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
    sessionInfo.systemId            = mSystemID;
    sessionInfo.next                = &graphicsBinding;

    err = xrCreateSession(mXRInstance, &sessionInfo, &mSession);

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to create OpenXR session.  Error code: " << err << LL_ENDL;
        return;
    }

    mInitialized = XR_STATE_SESSION_CREATED;
}

void LLXRManager::shutdown()
{
    if (mInitialized != XR_STATE_DESTROYED && mInitialized != XR_STATE_UNINITIALIZED)
    {
        xrDestroySession(mSession);
        xrDestroyInstance(mXRInstance);
        mInitialized = XR_STATE_DESTROYED;
    }
}

static XrPosef identity_pose = {
    { 0, 0, 0, 1.0 },
    { 0, 0, 0 }
};

void LLXRManager::setupPlaySpace()
{
    if (mInitialized != XR_STATE_SESSION_CREATED)
    {
        LL_ERRS("XRManager") << "Cannot setup reference space without a session." << LL_ENDL;
        return;
    }

    XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    referenceSpaceCreateInfo.next                       = nullptr;
    referenceSpaceCreateInfo.poseInReferenceSpace       = identity_pose;
    referenceSpaceCreateInfo.referenceSpaceType         = mAppSpace;

    S32 err = xrCreateReferenceSpace(mSession, &referenceSpaceCreateInfo, &mReferenceSpace);

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to create reference space.  Error code: " << err << LL_ENDL;
        return;
    }

    // We also need to create a view space.  This is useful for getting view space positions relative to the reference space.

    XrReferenceSpaceCreateInfo view_space_info = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    view_space_info.next                       = nullptr;
    view_space_info.referenceSpaceType         = XR_REFERENCE_SPACE_TYPE_VIEW;
    view_space_info.poseInReferenceSpace       = identity_pose;

    err = xrCreateReferenceSpace(mSession, &view_space_info, &mViewSpace);

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to create view space.  Error code: " << err << LL_ENDL;
        return;
    }
}

void LLXRManager::startFrame()
{
    if (mInitialized != XR_STATE_RUNNING)
    {
        return;
    }

    XrFrameBeginInfo frameBeginInfo = { XR_TYPE_FRAME_BEGIN_INFO };
    xrBeginFrame(mSession, &frameBeginInfo);
}

void LLXRManager::handleSessionState()
{
    if (mInitialized != XR_STATE_RUNNING &&
        mInitialized != XR_STATE_SESSION_CREATED &&
        mInitialized != XR_STATE_PAUSED)
    {
        return;
    }

    XrEventDataBuffer runtime_event = { XR_TYPE_EVENT_DATA_BUFFER };
    XrResult          poll_result   = xrPollEvent(mXRInstance, &runtime_event);
    while (poll_result == XR_SUCCESS)
    {
        switch (runtime_event.type)
        {
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            {
                LL_ERRS("XRManager") << "Instance loss pending." << LL_ENDL;
                break;
            }
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
            {
                XrEventDataSessionStateChanged* event = (XrEventDataSessionStateChanged*)&runtime_event;
                mSessionState = event->state;

                switch (mSessionState)
                {
                    case XR_SESSION_STATE_READY:
                    {
                        LL_INFOS("XRManager") << "Session state changed to READY." << LL_ENDL;
                        if (mInitialized != XR_STATE_RUNNING &&
                            mInitialized != XR_STATE_PAUSED)
                        {
                            XrSessionBeginInfo beginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
                            beginInfo.next                         = nullptr;
                            beginInfo.primaryViewConfigurationType = mViewConfigType;

                            S32 err = xrBeginSession(mSession, &beginInfo);

                            if (XR_FAILED(err))
                            {
                                LL_ERRS("XRManager") << "Failed to begin session.  Error code: " << err << LL_ENDL;
                                return;
                            }

                            mInitialized = XR_STATE_RUNNING;
                        }

                        break;
                    }
                    case XR_SESSION_STATE_SYNCHRONIZED:
                    case XR_SESSION_STATE_VISIBLE:
                    case XR_SESSION_STATE_FOCUSED:
                    {
                        if (mInitialized == XR_STATE_PAUSED)
                        {
                            mInitialized = XR_STATE_RUNNING;
                        }

                        LL_INFOS("XRManager") << "Session state changed to FOCUSED." << LL_ENDL;
                        break;
                    }
                    case XR_SESSION_STATE_STOPPING:
                    {
                        LL_INFOS("XRManager") << "Session state changed to STOPPING." << LL_ENDL;

                        if (mInitialized == XR_STATE_RUNNING)
                        {
                            S32 err = xrEndSession(mSession);
                            if (XR_FAILED(err))
                            {
                                LL_ERRS("XRManager") << "Failed to end session.  Error code: " << err << LL_ENDL;
                                return;
                            }
                        }
                        break;
                    }
                    case XR_SESSION_STATE_LOSS_PENDING:
                    case XR_SESSION_STATE_EXITING:
                    {
                        LL_INFOS("XRManager") << "Session state changed to EXITING." << LL_ENDL;
                        shutdown();
                        break;
                    }

                    case XR_SESSION_STATE_IDLE:
                    case XR_SESSION_STATE_UNKNOWN:
                    {
                        // When the state is idle, there's a good chance the user does not have their headset on.
                        // We can use this to redirect rendering to the desktop camera.
                        // Eventually: have the user's avatar pose sw
                        LL_INFOS("XRManager") << "Session state changed to UNKNOWN or IDLE." << LL_ENDL;
                        break;
                    }
                    default:
                    {
                        LL_WARNS("XRManager") << "Unhandled session state: " << mSessionState << LL_ENDL;
                        break;
                    }
                }
                break;
            }

            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
            {
                LL_INFOS("XRManager") << "Interaction profile changed." << LL_ENDL;
                break;
            }
            default:
            {
                LL_WARNS("XRManager") << "Unhandled event type: " << runtime_event.type << LL_ENDL;
                break;
            }
        }

        runtime_event.type = XR_TYPE_EVENT_DATA_BUFFER;
        poll_result        = xrPollEvent(mXRInstance, &runtime_event);
    }

    if (poll_result == XR_EVENT_UNAVAILABLE)
    {

    }
    else
    {
        LL_WARNS ("XRManager") << "Failed to poll event.  Error code: " << poll_result << LL_ENDL;
    }
}

void LLXRManager::updateXRSession()
{
    if (mInitialized != XR_STATE_RUNNING)
    {
        return;
    }

    XrFrameWaitInfo frameWaitInfo = { XR_TYPE_FRAME_WAIT_INFO };
    XrFrameState    frameState    = { XR_TYPE_FRAME_STATE };

    S32 err = xrWaitFrame(mSession, &frameWaitInfo, &frameState);

    XrViewLocateInfo viewLocateInfo      = { XR_TYPE_VIEW_LOCATE_INFO };
    viewLocateInfo.next                  = nullptr;
    viewLocateInfo.viewConfigurationType = mViewConfigType;
    viewLocateInfo.displayTime           = frameState.predictedDisplayTime;
    viewLocateInfo.space                 = mReferenceSpace;

    XrViewState viewState = { XR_TYPE_VIEW_STATE };
    U32         viewCount = (U32)mViews.size();
    err                   = xrLocateViews(mSession, &viewLocateInfo, &viewState, viewCount, &viewCount, mViews.data());

    if (XR_FAILED(err))
    {
        LL_WARNS("XRManager") << "Failed to locate views.  Error code: " << err << LL_ENDL;
        return;
    }

    XrSpaceLocation spaceLocation = { XR_TYPE_SPACE_LOCATION };
    spaceLocation.next            = nullptr;
    spaceLocation.pose            = identity_pose;

    xrLocateSpace(mViewSpace, mReferenceSpace, frameState.predictedDisplayTime, &spaceLocation);
    // OpenXR assumes Y up.
    // We need Z up.
    // Just swap Y with Z.
    mHeadPosition = LLVector3(spaceLocation.pose.position.x, spaceLocation.pose.position.z, spaceLocation.pose.position.y);

    LLMatrix4 hmdMatrix;

    // Convert to CFR.
    mHeadOrientation = LLQuaternion(-mViews[0].pose.orientation.z,
                                    -mViews[0].pose.orientation.x,
                                    mViews[0].pose.orientation.y,
                                    mViews[0].pose.orientation.w);
}

void LLXRManager::endFrame()
{
    if (mInitialized != XR_STATE_RUNNING)
    {
        return;
    }

    XrFrameEndInfo frameEndInfo = { XR_TYPE_FRAME_END_INFO };
    xrEndFrame(mSession, &frameEndInfo);
}
