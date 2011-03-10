# -*- cmake -*-
include(Prebuilt)
include(Linking)

use_prebuilt_binary(googlemock)

set(GOOGLEMOCK_INCLUDE_DIRS 
    ${LIBS_PREBUILT_DIR}/include)

if (LINUX)
	# VWR-24366: gmock is underlinked, it needs gtest.
    set(GOOGLEMOCK_LIBRARIES 
        gmock -Wl,--no-as-needed
        gtest -Wl,--as-needed)
elseif(WINDOWS)
    set(GOOGLEMOCK_LIBRARIES 
        gmock)
    set(GOOGLEMOCK_INCLUDE_DIRS 
        ${LIBS_PREBUILT_DIR}/include
        ${LIBS_PREBUILT_DIR}/include/gmock
        ${LIBS_PREBUILT_DIR}/include/gmock/boost/tr1/tr1)
elseif(DARWIN)
    set(GOOGLEMOCK_LIBRARIES
        gmock
        gtest)
endif(LINUX)


