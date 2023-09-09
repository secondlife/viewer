# -*- cmake -*-

include(FetchContent)

if (WINDOWS)
FetchContent_Declare(
  webrtc
  URL      http://localhost:8000/webrtc.windows_x86_64.tar.bz2
  URL_HASH MD5=dfb692562770dc8c877ebfe4302e2881
  FIND_PACKAGE_ARGS NAMES webrtc
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  DOWNLOAD_DIR "${LIBS_PREBUILT_DIR}/webrtc/"
)
elseif (DARWIN)
FetchContent_Declare(
  webrtc
  URL      http://localhost:8000/webrtc.macos_x86_64.tar.bz2
  URL_HASH MD5=cfbcac7da897a862f9791ea29330b814
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
  target_link_libraries( ll::webrtc INTERFACE "${WEBRTC_PATH}/lib/webrtc.a" )
elseif (LINUX)
  target_link_libraries( ll::webrtc INTERFACE "${WEBRTC_PATH}/lib/webrtc.a" )
endif (WINDOWS)
target_include_directories( ll::webrtc SYSTEM INTERFACE "${WEBRTC_PATH}/include" "${WEBRTC_PATH}/include/third_party/abseil-cpp")

