# -*- cmake -*-

include(FetchContent)

if (WINDOWS)
    if( ADDRESS_SIZE EQUAL 32 )
        FetchContent_Declare(
              webrtc
              URL "https://webrtc-build-releases.s3.us-west-2.amazonaws.com/webrtc.windows_x86.tar.bz2"
              URL_HASH "MD5=cefbd446b1b152ac08217fc78648fb99"
              FIND_PACKAGE_ARGS NAMES webrtc
              DOWNLOAD_EXTRACT_TIMESTAMP TRUE
              DOWNLOAD_DIR "${LIBS_PREBUILT_DIR}/webrtc/"
            )
    else ( ADDRESS_SIZE EQUAL 32 )
        FetchContent_Declare(
              webrtc
              URL "https://webrtc-build-releases.s3.us-west-2.amazonaws.com/webrtc.windows_x86_64.tar.bz2"
              URL_HASH "MD5=6e25b99284215933ec1571b6e60c1030"
              FIND_PACKAGE_ARGS NAMES webrtc
              DOWNLOAD_EXTRACT_TIMESTAMP TRUE
              DOWNLOAD_DIR "${LIBS_PREBUILT_DIR}/webrtc/"
            )
    endif ( ADDRESS_SIZE EQUAL 32 )
elseif (DARWIN)
    FetchContent_Declare(
      webrtc
      URL "https://webrtc-build-releases.s3.us-west-2.amazonaws.com/webrtc.macos_x86_64.tar.bz2"
      URL_HASH "MD5=6849e81773135290f5c70cbf679992fb"
      FIND_PACKAGE_ARGS NAMES webrtc
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
      DOWNLOAD_DIR "${LIBS_PREBUILT_DIR}/webrtc/"
    )
endif (WINDOWS)

FetchContent_MakeAvailable(webrtc)

set(WEBRTC_PATH ${webrtc_SOURCE_DIR})

add_library( ll::webrtc INTERFACE IMPORTED )

if (WINDOWS)
    target_link_libraries( ll::webrtc INTERFACE "${WEBRTC_PATH}/lib/webrtc.lib" )
elseif (DARWIN)
    FIND_LIBRARY(COREAUDIO_LIBRARY CoreAudio)
    FIND_LIBRARY(COREGRAPHICS_LIBRARY CoreGraphics)
    FIND_LIBRARY(AUDIOTOOLBOX_LIBRARY AudioToolbox)
    FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation)
    FIND_LIBRARY(COCOA_LIBRARY Cocoa)
    
    target_link_libraries( ll::webrtc INTERFACE
        "${WEBRTC_PATH}/lib/libwebrtc.a"
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREGRAPHICS_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${COCOA_LIBRARY}
    )
elseif (LINUX)
    target_link_libraries( ll::webrtc INTERFACE "${WEBRTC_PATH}/lib/libwebrtc.a" )
endif (WINDOWS)

message("PATH: ${WEBRTC_PATH}/include")
    
target_include_directories( ll::webrtc INTERFACE "${WEBRTC_PATH}/include" "${WEBRTC_PATH}/include/third_party/abseil-cpp")

