# -*- cmake -*-
include(Prebuilt)
include(Linking)

use_prebuilt_binary(googlemock)

set(GOOGLEMOCK_INCLUDE_DIRS 
    ${LIBS_PREBUILT_DIR}/include)

if (LINUX)
    set(GOOGLEMOCK_LIBRARIES 
        gmock  
        gtest)
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


