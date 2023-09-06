# -*- cmake -*-
include(CMakeCopyIfDifferent)

include(Linking)

include_guard()

set(WEBRTC_ROOT ${CMAKE_BINARY_DIR}/../../webrtc/src)
file(COPY ${WEBRTC_ROOT}/out/Default/obj/webrtc.lib
    DESTINATION ${CMAKE_BINARY_DIR}/packages/lib/release
)
set(WEBRTC_INCLUDE_DIR ${CMAKE_BINARY_DIR}/packages/include/webrtc)
file(MAKE_DIRECTORY ${WEBRTC_INCLUDE_DIR})

file(COPY ${WEBRTC_ROOT}/api
          ${WEBRTC_ROOT}/media/base 
          ${WEBRTC_ROOT}/media/engine
          ${WEBRTC_ROOT}/rtc_base
          ${WEBRTC_ROOT}/pc
          ${WEBRTC_ROOT}/p2p
          ${WEBRTC_ROOT}/call
          ${WEBRTC_ROOT}/media
          ${WEBRTC_ROOT}/system_wrappers
          ${WEBRTC_ROOT}/common_video
          ${WEBRTC_ROOT}/video
          ${WEBRTC_ROOT}/common_audio
          ${WEBRTC_ROOT}/logging
          ${WEBRTC_ROOT}/third_party/abseil-cpp/absl
    DESTINATION ${WEBRTC_INCLUDE_DIR}
    FILES_MATCHING PATTERN "*.h"
)

add_library(ll::webrtc STATIC IMPORTED)

if (LINUX)
  target_link_libraries( ll::webrtc INTERFACE ../webrtc/src/obj/Default/webrtc)
elseif (DARWIN)
  target_link_libraries( ll::webrtc INTERFACE ../webrtc/src/obj/Default/webrtc)
elseif (WINDOWS)
  set_target_properties( ll::webrtc PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/packages/lib/release/webrtc.lib)
  target_link_libraries( ll::webrtc INTERFACE ${CMAKE_BINARY_DIR}/packages/lib/release/webrtc.lib)
endif (LINUX)
target_include_directories( ll::webrtc INTERFACE "${WEBRTC_INCLUDE_DIR}")
