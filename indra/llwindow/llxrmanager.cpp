#include "llxrmanager.h"
#include "linden_common.h"
#include "llgl.h"
#include "llrender.h"
#include "llrendertarget.h"
#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/quaternion.hpp>

LLMatrix4 OXR_TO_SFR = LLMatrix4(OGL_TO_CFR_ROTATION);

#define LL_HAND_COUNT       2
#define LL_HAND_LEFT_INDEX  0
#define LL_HAND_RIGHT_INDEX 1

glm::quat quatFromXrQuaternion(XrQuaternionf quat)
{
    return glm::quat(-quat.z, -quat.x, quat.y, quat.w);
}

glm::vec3 vec3FromXrVector3(XrVector3f vec)
{
    return glm::vec3(-vec.x, vec.z, vec.y);
}

glm::mat4 projectionFov(const XrFovf fov, const float nearZ, const float farZ)
{
    glm::mat4   proj;
    const float tanAngleLeft  = tanf(fov.angleLeft);
    const float tanAngleRight = tanf(fov.angleRight);

    const float tanAngleDown = tanf(fov.angleDown);
    const float tanAngleUp   = tanf(fov.angleUp);

    const float tanAngleWidth  = tanAngleRight - tanAngleLeft;
    const float tanAngleHeight = tanAngleUp - tanAngleDown;
    const float offsetZ        = nearZ;

    if (farZ <= nearZ)
    {
        proj[0] = glm::vec4(2.f / tanAngleWidth, 0.0f, 0.0f, 0.0f);
        proj[1] = glm::vec4(0.f, 2 / tanAngleHeight, 0.0f, 0.0f);
        proj[2] = glm::vec4((tanAngleRight + tanAngleLeft) / tanAngleWidth, (tanAngleUp + tanAngleDown) / tanAngleHeight, -1.f, -1.f);
        proj[3] = glm::vec4(0.f, 0.0f, -(nearZ + offsetZ), 0.0f);
    }
    else
    {
        proj[0] = glm::vec4(2.f / tanAngleWidth, 0.0f, 0.0f, 0.0f);
        proj[1] = glm::vec4(0.f, 2 / tanAngleHeight, 0.0f, 0.0f);
        proj[2] = glm::vec4((tanAngleRight + tanAngleLeft) / tanAngleWidth,
                            (tanAngleUp + tanAngleDown) / tanAngleHeight,
                            -(farZ + offsetZ) / (farZ - nearZ),
                            -1.f);
        proj[3] = glm::vec4(0.f, 0.0f, -(farZ * (nearZ + offsetZ)) / (farZ - nearZ), 0.0f);
    }
    return proj;
}

glm::mat4 viewMatrixFromPose(XrPosef pose)
{
    glm::mat4 view;
    glm::mat4 orientation = glm::toMat4(quatFromXrQuaternion(pose.orientation));
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), vec3FromXrVector3(pose.position));
    view                  = translation * orientation;
    return view;
}

S64 getSwapchainFormat(XrInstance instance, XrSession session, S64 preferredFormat)
{
    S64 err = 0;
    U32 formatCount;
    err = xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr);

    if (err != XR_SUCCESS)
    {
        return -1;
    }

    std::vector<S64> formats(formatCount);
    err = xrEnumerateSwapchainFormats(session, formatCount, &formatCount, formats.data());

    if (err != XR_SUCCESS)
    {
        return -1;
    }

    for (int i = 0; i < formats.size(); i++)
    {
        LL_INFOS("XRManager") << "Format: " << formats[i] << LL_ENDL;
    }

    if (preferredFormat != 0)
    {
        for (auto& format : formats)
        {
            if (format == preferredFormat)
            {
                return preferredFormat;
            }
        }
    }

    // If we can't find the preferred format, just return the first one.  It's what the given OpenXR runtime prefers anyways.
    return formats[0];
}

static PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;

void LLXRManager::initInstance()
{
    // A lot of this uses Monado's OpenGL example which can be found here:
    // https://gitlab.freedesktop.org/monado/demos/openxr-simple-example/

    // For the time being, OpenXR support is only really relevant on Windows and Linux.
    // macOS/visionOS does not have any support for OpenXR at present.  This may change via Monado or a compatibility initiative.
#if defined(LL_WINDOWS) || defined(LL_LINUX)

    XrApplicationInfo AI;
    strncpy(AI.applicationName, "Second Life", XR_MAX_APPLICATION_NAME_SIZE);
    AI.applicationVersion = 1;
    strncpy(AI.engineName, "Second Life", XR_MAX_ENGINE_NAME_SIZE);
    AI.engineVersion = 1;
    AI.apiVersion    = XR_API_VERSION_1_0;

    mRequestedInstanceExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
    mRequestedInstanceExtensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);

    U32                          apiLayerCount = 0;
    std::vector<XrApiLayerProperties> apiLayerProperties;
    if (XR_FAILED(xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr)))
    {
        LL_ERRS("XRManager") << "Failed to enumerate ApiLayerProperties." << LL_ENDL;
        return;
    }

    apiLayerProperties.resize(apiLayerCount, { XR_TYPE_API_LAYER_PROPERTIES });

    if (XR_FAILED(xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount, apiLayerProperties.data())))
    {
        LL_ERRS("XRManager") << "Failed to enumerate ApiLayerProperties." << LL_ENDL;
        return;
    }

    // Check the requested API layers against the ones from the OpenXR. If found add it to the Active API Layers.
    for (auto& requestLayer : mRequestedAPILayers)
    {
        for (auto& layerProperty : apiLayerProperties)
        {
            // strcmp returns 0 if the strings match.
            if (strcmp(requestLayer.c_str(), layerProperty.layerName) != 0)
            {
                continue;
            }
            else
            {
                mActiveAPILayers.push_back(requestLayer.c_str());
                break;
            }
        }
    }

    // Get all the Instance Extensions from the OpenXR instance.
    U32                           extensionCount = 0;
    std::vector<XrExtensionProperties> extensionProperties;

    if (XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr)))
    {
        LL_ERRS("XRManager") << "Failed to enumerate InstanceExtensionProperties." << LL_ENDL;
        return;
    }

    extensionProperties.resize(extensionCount, { XR_TYPE_EXTENSION_PROPERTIES });

    if (XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProperties.data())))
    {
        LL_ERRS("XRManager") << "Failed to enumerate InstanceExtensionProperties." << LL_ENDL;
        return;
    }

    // Check the requested Instance Extensions against the ones from the OpenXR runtime.
    // If an extension is found add it to Active Instance Extensions.
    // Log error if the Instance Extension is not found.
    for (auto& requestedInstanceExtension : mRequestedInstanceExtensions)
    {
        bool found = false;
        for (auto& extensionProperty : extensionProperties)
        {
            // strcmp returns 0 if the strings match.
            if (strcmp(requestedInstanceExtension.c_str(), extensionProperty.extensionName) != 0)
            {
                continue;
            }
            else
            {
                mActiveInstanceExtensions.push_back(requestedInstanceExtension.c_str());
                found = true;
                break;
            }
        }
        if (!found)
        {
            LL_WARNS("XRManager") << "Failed to find OpenXR instance extension: " << requestedInstanceExtension << LL_ENDL;
        }
    }

    // Fill out an XrInstanceCreateInfo structure and create an XrInstance.
    // XR_DOCS_TAG_BEGIN_XrInstanceCreateInfo
    XrInstanceCreateInfo instanceCI{ XR_TYPE_INSTANCE_CREATE_INFO };
    instanceCI.createFlags           = 0;
    instanceCI.applicationInfo       = AI;
    instanceCI.enabledApiLayerCount  = static_cast<U32>(mActiveAPILayers.size());
    instanceCI.enabledApiLayerNames  = mActiveAPILayers.data();
    instanceCI.enabledExtensionCount = static_cast<U32>(mActiveInstanceExtensions.size());
    instanceCI.enabledExtensionNames = mActiveInstanceExtensions.data();

    if (XR_FAILED(xrCreateInstance(&instanceCI, &mXRInstance)))
    {
        LL_ERRS("XRManager") << "Failed to create OpenXR instance." << LL_ENDL;
        return;
    }

#elif LL_DARWIN
    LL_ERRS("XRManager") << "Apple platforms, such as visionOS and macOS, are not presently supported.  Aborting XR initialization."
                         << LL_ENDL;
    return;
#endif

    mXRState = XR_STATE_INSTANCE_CREATED;
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

    if (mXRState != XR_STATE_INSTANCE_CREATED)
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

    mXRState = XR_STATE_SESSION_CREATED;
}

GLuint LLXRManager::createImageView(LLImageViewCreateInfo& info)
{
    GLuint fbo;
    glGenFramebuffers(1, &fbo);

    return fbo;
}

void LLXRManager::createSwapchains()
{
    destroySwapchains();

    if (mSwapchains.empty())
    {
        mSwapchains.resize(mViewConfigViews.size());
    }

    if (mColorTextures.empty())
    {
        mColorTextures.resize(mViewConfigViews.size());
    }

    if (mSwapchainImages.empty())
    {
        mSwapchainImages.resize(mViewConfigViews.size());
    }

    // Per view, create a color and depth swapchain, and their associated image views.
    for (size_t i = 0; i < mViewConfigViews.size(); i++)
    {
        XrSwapchain colorSwapchain;

        XrSwapchainCreateInfo swapchainCI{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
        swapchainCI.createFlags = 0;
        swapchainCI.usageFlags  = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCI.format =
            getSwapchainFormat(mXRInstance, mSession, GL_SRGB8_ALPHA8);
        swapchainCI.sampleCount =
            mViewConfigViews[i].recommendedSwapchainSampleCount; // Use the recommended values from the XrViewConfigurationView.
        swapchainCI.width     = mViewConfigViews[i].recommendedImageRectWidth;
        swapchainCI.height    = mViewConfigViews[i].recommendedImageRectHeight;
        swapchainCI.faceCount = 1;
        swapchainCI.arraySize = 1;
        swapchainCI.mipCount  = 1;

        if (XR_FAILED(xrCreateSwapchain(mSession, &swapchainCI, &colorSwapchain)))
        {
            LL_ERRS("XRManager") << "Failed to create Color Swapchain." << LL_ENDL;
            return;
        }

        U32 colorSwapchainImageCount = 0;
        if (XR_FAILED(xrEnumerateSwapchainImages(colorSwapchain, 0, &colorSwapchainImageCount, nullptr)))
        {
            LL_ERRS("XRManager") << "Failed to enumerate Color Swapchain Images." << LL_ENDL;
            return;
        }

        mSwapchainImages[i].resize(colorSwapchainImageCount, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR, nullptr});

        if (XR_FAILED(xrEnumerateSwapchainImages(colorSwapchain, colorSwapchainImageCount, &colorSwapchainImageCount,
                                                 (XrSwapchainImageBaseHeader*)mSwapchainImages[i].data())))
        {
            LL_ERRS("XRManager") << "Failed to enumerate Color Swapchain Images." << LL_ENDL;
            return;
        }

        for (U32 j = 0; j < colorSwapchainImageCount; j++)
        {
            GLuint colorImageView;
            glGenFramebuffers(1, &colorImageView);
            mColorTextures[i].push_back(colorImageView);
        }

        mSwapchains[i] = colorSwapchain;
    }
}

void LLXRManager::destroySwapchains()
{
    if (!mColorTextures.empty())
    {
        for (auto& colorTexture : mColorTextures)
        {
            glDeleteTextures((GLsizei)colorTexture.size(), colorTexture.data());
        }
    }

    if (!mSwapchains.empty())
    {
        for (auto& swapchain : mSwapchains)
        {
            xrDestroySwapchain(swapchain);
        }
    }

    mSwapchainImages.clear();
    mColorTextures.clear();
    mSwapchains.clear();
}

void LLXRManager::getInstanceProperties()
{
    // Get the instance's properties and log the runtime name and version.
    // XR_DOCS_TAG_BEGIN_GetInstanceProperties
    XrInstanceProperties instanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };
    if (XR_FAILED(xrGetInstanceProperties(mXRInstance, &instanceProperties)))
    {
        LL_ERRS("XRManager") << "Failed to get instance properties." << LL_ENDL;
        return;
    }

    LL_WARNS("XRManager") << "OpenXR Runtime: " << instanceProperties.runtimeName << " - "
                          << XR_VERSION_MAJOR(instanceProperties.runtimeVersion) << "."
                          << XR_VERSION_MINOR(instanceProperties.runtimeVersion) << "."
                          << XR_VERSION_PATCH(instanceProperties.runtimeVersion) << LL_ENDL;
}

void LLXRManager::getSystemID()
{
    // XR_DOCS_TAG_BEGIN_GetSystemID
    // Get the XrSystemId from the instance and the supplied XrFormFactor.
    XrSystemGetInfo systemGI{ XR_TYPE_SYSTEM_GET_INFO };
    systemGI.formFactor = mFormFactor;
    if (XR_FAILED(xrGetSystem(mXRInstance, &systemGI, &mSystemID)))
    {
        LL_ERRS("XRManager") << "Failed to get SystemID." << LL_ENDL;
        return;
    }

    // Get the System's properties for some general information about the hardware and the vendor.
    if (XR_FAILED(xrGetSystemProperties(mXRInstance, mSystemID, &mSystemProperties)))
    {
        LL_ERRS("XRManager") << "Failed to get SystemProperties." << LL_ENDL;
        return;
    }
}

void LLXRManager::getEnvironmentBlendModes()
{
    U32 environmentBlendModeCount = 0;
    if (XR_FAILED(xrEnumerateEnvironmentBlendModes(mXRInstance, mSystemID, mViewConfig, 0, &environmentBlendModeCount, nullptr)))
    {
        LL_ERRS("XRManager") << "Failed to enumerate EnvironmentBlend Modes." << LL_ENDL;
        return;
    }

    mSupportedBlendModes.resize(environmentBlendModeCount);

    if (XR_FAILED(xrEnumerateEnvironmentBlendModes(mXRInstance,
                                                   mSystemID,
                                                   mViewConfig,
                                                   environmentBlendModeCount,
                                                   &environmentBlendModeCount,
                                                   mSupportedBlendModes.data())))
    {
        LL_ERRS("XRManager") << "Failed to enumerate EnvironmentBlendModes." << LL_ENDL;
    }

    // Pick the first application supported blend mode supported by the hardware.
    for (const XrEnvironmentBlendMode& environmentBlendMode : mApplicationEnvironmentBlendModes)
    {
        if (std::find(mSupportedBlendModes.begin(), mSupportedBlendModes.end(), environmentBlendMode) != mSupportedBlendModes.end())
        {
            mEnvironmentBlendMode = environmentBlendMode;
            break;
        }
    }
    if (mEnvironmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM)
    {
        mEnvironmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    }
}

void LLXRManager::getConfigurationViews()
{
    U32 viewConfigurationCount = 0;
    if (XR_FAILED(xrEnumerateViewConfigurations(mXRInstance, mSystemID, 0, &viewConfigurationCount, nullptr)))
    {
        LL_ERRS("XRManager") << "Failed to enumerate View Configurations." << LL_ENDL;
    }

    mViewConfigs.resize(viewConfigurationCount);

    if (XR_FAILED(
            xrEnumerateViewConfigurations(mXRInstance, mSystemID, viewConfigurationCount, &viewConfigurationCount, mViewConfigs.data())))
    {
        LL_ERRS("XRManager") << "Failed to enumerate View Configurations." << LL_ENDL;
    }

    // Pick the first application supported View Configuration Type con supported by the hardware.
    for (const XrViewConfigurationType& viewConfiguration : mAppViewConfigurations)
    {
        if (std::find(mViewConfigs.begin(), mViewConfigs.end(), viewConfiguration) != mViewConfigs.end())
        {
            mViewConfig = viewConfiguration;
            break;
        }
    }
    if (mViewConfig == XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM)
    {
        LL_WARNS("XRManager") << "Failed to find a view configuration type. Defaulting to XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO."
                              << LL_ENDL;
        mViewConfig = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    }

    // Gets the View Configuration Views. The first call gets the count of the array that will be returned. The next call fills out the
    // array.
    U32 viewConfigurationViewCount = 0;
    if (XR_FAILED(xrEnumerateViewConfigurationViews(mXRInstance, mSystemID, mViewConfig, 0, &viewConfigurationViewCount, nullptr)))
    {
        LL_ERRS("XRManager") << "Failed to enumerate ViewConfiguration Views." << LL_ENDL;
        return;
    }

    mViewConfigViews.resize(viewConfigurationViewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
    if (XR_FAILED(xrEnumerateViewConfigurationViews(mXRInstance,
                                                    mSystemID,
                                                    mViewConfig,
                                                    viewConfigurationViewCount,
                                                    &viewConfigurationViewCount,
                                                    mViewConfigViews.data())))
    {
        LL_ERRS("XRManager") << "Failed to enumerate ViewConfiguration Views." << LL_ENDL;
        return;
    }

    mViews.resize(viewConfigurationViewCount, { XR_TYPE_VIEW, nullptr });
    mEyeRotations.resize(viewConfigurationViewCount);
    mEyePositions.resize(viewConfigurationViewCount);
    mEyeProjections.resize(viewConfigurationViewCount);
    mEyeViews.resize(viewConfigurationViewCount);
    mProjectionViews.resize(viewConfigurationViewCount);
}

void LLXRManager::shutdown()
{
    if (mXRState != XR_STATE_DESTROYED && mXRState != XR_STATE_UNINITIALIZED)
    {
        xrDestroySession(mSession);
        xrDestroyInstance(mXRInstance);
        mXRState = XR_STATE_DESTROYED;
    }
}

static XrPosef identity_pose = { { 0, 0, 0, 1.0 }, { 0, 0, 0 } };

void LLXRManager::setupPlaySpace()
{
    if (mXRState != XR_STATE_SESSION_CREATED)
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
    if (mXRState != XR_STATE_RUNNING)
    {
        return;
    }

    XrFrameBeginInfo frameBeginInfo = { XR_TYPE_FRAME_BEGIN_INFO };
    xrBeginFrame(mSession, &frameBeginInfo);
}

void LLXRManager::handleSessionState()
{
    if (mXRState != XR_STATE_RUNNING && mXRState != XR_STATE_SESSION_CREATED && mXRState != XR_STATE_PAUSED)
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
                mSessionState                         = event->state;

                switch (mSessionState)
                {
                    case XR_SESSION_STATE_READY:
                    {
                        LL_INFOS("XRManager") << "Session state changed to READY." << LL_ENDL;
                        if (mXRState != XR_STATE_RUNNING && mXRState != XR_STATE_PAUSED)
                        {
                            XrSessionBeginInfo beginInfo           = { XR_TYPE_SESSION_BEGIN_INFO };
                            beginInfo.next                         = nullptr;
                            beginInfo.primaryViewConfigurationType = mViewConfig;

                            S32 err = xrBeginSession(mSession, &beginInfo);

                            if (XR_FAILED(err))
                            {
                                LL_ERRS("XRManager") << "Failed to begin session.  Error code: " << err << LL_ENDL;
                                return;
                            }

                            mXRState = XR_STATE_RUNNING;
                        }

                        break;
                    }
                    case XR_SESSION_STATE_SYNCHRONIZED:
                    case XR_SESSION_STATE_VISIBLE:
                    case XR_SESSION_STATE_FOCUSED:
                    {
                        if (mXRState == XR_STATE_PAUSED)
                        {
                            mXRState = XR_STATE_RUNNING;
                        }

                        LL_INFOS("XRManager") << "Session state changed to FOCUSED." << LL_ENDL;
                        break;
                    }
                    case XR_SESSION_STATE_STOPPING:
                    {
                        LL_INFOS("XRManager") << "Session state changed to STOPPING." << LL_ENDL;

                        if (mXRState == XR_STATE_RUNNING)
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

    if (poll_result == XR_EVENT_UNAVAILABLE) {}
    else
    {
        LL_WARNS("XRManager") << "Failed to poll event.  Error code: " << poll_result << LL_ENDL;
    }
}

void LLXRManager::updateXRSession()
{
    if (mXRState != XR_STATE_RUNNING)
    {
        return;
    }

    XrFrameWaitInfo frameWaitInfo = { XR_TYPE_FRAME_WAIT_INFO };

    S32 err = xrWaitFrame(mSession, &frameWaitInfo, &mFrameState);

    XrViewLocateInfo viewLocateInfo      = { XR_TYPE_VIEW_LOCATE_INFO };
    viewLocateInfo.next                  = nullptr;
    viewLocateInfo.viewConfigurationType = mViewConfig;
    viewLocateInfo.displayTime           = mFrameState.predictedDisplayTime;
    viewLocateInfo.space                 = mReferenceSpace;

    U32 viewCount = (U32)mViews.size();
    err           = xrLocateViews(mSession, &viewLocateInfo, &mViewState, viewCount, &viewCount, mViews.data());

    if (XR_FAILED(err))
    {
        LL_WARNS("XRManager") << "Failed to locate views.  Error code: " << err << LL_ENDL;
        return;
    }

    XrSpaceLocation spaceLocation = { XR_TYPE_SPACE_LOCATION };
    spaceLocation.next            = nullptr;
    spaceLocation.pose            = identity_pose;

    xrLocateSpace(mViewSpace, mReferenceSpace, mFrameState.predictedDisplayTime, &spaceLocation);
    // OpenXR assumes Y up.
    // We need Z up.
    // Just swap Y with Z.
    mHeadPosition = vec3FromXrVector3(spaceLocation.pose.position);

    LLMatrix4 hmdMatrix;

    // Convert to CFR.
    mHeadOrientation = quatFromXrQuaternion(mViews[0].pose.orientation);

    // Populate the eye poses.
    for (U32 i = 0; i < mViews.size(); i++)
    {
        mEyeRotations[i]   = quatFromXrQuaternion(mViews[i].pose.orientation);
        mEyePositions[i]   = vec3FromXrVector3(mViews[i].pose.position);
        mEyeProjections[i] = projectionFov(mViews[i].fov, mZNear, mZFar);
        mEyeViews[i]       = viewMatrixFromPose(mViews[i].pose);
    }
}

void LLXRManager::updateFrame(LLRenderTarget* target, LLXREye eye)
{
    if (mXRState != XR_STATE_RUNNING)
    {
        return;
    }

    XrSwapchain chain = mSwapchains[eye];
    auto        chainImages = mSwapchainImages[eye];
    auto        colorTextures = mColorTextures[eye];

    U32 colorImageIdx = 0;

    XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
    xrAcquireSwapchainImage(chain, &acquireInfo, &colorImageIdx);


    XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
    waitInfo.timeout                  = XR_INFINITE_DURATION;
    xrWaitSwapchainImage(chain, &waitInfo);

    U32 viewWidth   = mViewConfigViews[eye].recommendedImageRectWidth;
    U32 viewHeight  = mViewConfigViews[eye].recommendedImageRectHeight;

    mProjectionViews[eye].pose = mViews[eye].pose;
    mProjectionViews[eye].fov  = mViews[eye].fov;
    mProjectionViews[eye].subImage.imageRect.offset.x = 0;
    mProjectionViews[eye].subImage.imageRect.offset.y = 0;
    mProjectionViews[eye].subImage.imageRect.extent.width  = viewWidth;
    mProjectionViews[eye].subImage.imageRect.extent.height = viewHeight;
    mProjectionViews[eye].subImage.imageArrayIndex         = eye;
    
    // Pretty much we need to copy the texture to the swapchain.
    glBindFramebuffer(GL_FRAMEBUFFER, colorTextures[colorImageIdx]);
    glScissor(0, 0, viewWidth, viewHeight);
    glViewport(0, 0, viewWidth, viewHeight);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, colorTextures[colorImageIdx],
                           0);

    glClearColor(.0f, 0.0f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlitFramebuffer(target->getTexture(), 0, target->getWidth(), target->getHeight(), 0, 0, viewWidth, viewHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    S32                         err         = xrReleaseSwapchainImage(chain, &releaseInfo);
    
}

void LLXRManager::endFrame()
{
    if (mXRState != XR_STATE_RUNNING)
    {
        return;
    }

    XrCompositionLayerProjection layerProjection = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    layerProjection.layerFlags                   = 0;
    layerProjection.space                        = mReferenceSpace;
    layerProjection.viewCount                    = (U32)mViews.size();
    layerProjection.views                        = mProjectionViews.data();

    S32                           submittedLayerCount = 0;
    XrCompositionLayerBaseHeader* submittedLayers[]   = { (XrCompositionLayerBaseHeader*)&layerProjection };

    if (!mFrameState.shouldRender || (mViewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0)
    {
        submittedLayerCount = 0;
    }

    XrFrameEndInfo frameEndInfo = { XR_TYPE_FRAME_END_INFO };
    // frameEndInfo.layerCount           = submittedLayerCount;
    // frameEndInfo.layers               = submittedLayers;
    frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    frameEndInfo.displayTime          = mFrameState.predictedDisplayTime;
    S32 err                           = xrEndFrame(mSession, &frameEndInfo);

    if (XR_FAILED(err))
    {
        LL_ERRS("XRManager") << "Failed to end frame.  Error code: " << err << LL_ENDL;
        return;
    }
}
