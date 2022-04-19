# -*- cmake -*-
include(Prebuilt)
include(Linking)

include_guard()

add_library( ll::googlemock INTERFACE IMPORTED )
if(USE_CONAN)
  target_link_libraries( ll::googlemock INTERFACE  CONAN_PKG::gtest )

  #Not very nice, but for the moment we need this for tut.hpp
  target_include_directories( ll::googlemock SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include ) 
  return()
endif()

use_prebuilt_binary(googlemock)

target_include_directories( ll::googlemock SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include )

if (LINUX)
    # VWR-24366: gmock is underlinked, it needs gtest.
    target_link_libraries( ll::googlemock INTERFACE gmock gtest)
elseif(WINDOWS)
    target_link_libraries( ll::googlemock INTERFACE gmock)
    target_include_directories( ll::googlemock SYSTEM INTERFACE
            ${LIBS_PREBUILT_DIR}/include
            ${LIBS_PREBUILT_DIR}/include/gmock)
elseif(DARWIN)
    target_link_libraries( ll::googlemock INTERFACE gmock gtest)
endif(LINUX)


