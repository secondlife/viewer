# -*- cmake -*-

# some webrtc headers require C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#set(CMAKE_GENERATOR_TOOLSET "clang-cl")

include(00-Common)
include(Linking)
include(WebRTC)

project(llwebrtc)

message(STATUS "C Compiler executable: ${CMAKE_C_COMPILER}")
message(STATUS "CXX Compiler executable: ${CMAKE_CXX_COMPILER}")
message(STATUS "Linker executable: ${CMAKE_LINKER}")
message(STATUS "SharedLib: ${SHARED_LIB_STAGING_DIR}")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")


set(llwebrtc_SOURCE_FILES
    llwebrtc.cpp
    )

set(llwebrtc_HEADER_FILES
    CMakeLists.txt
    llwebrtc.h
    llwebrtc_impl.h
    )

list(APPEND llwebrtc_SOURCE_FILES ${llwebrtc_HEADER_FILES})

add_library (llwebrtc SHARED ${llwebrtc_SOURCE_FILES})

set_target_properties(llwebrtc PROPERTIES PUBLIC_HEADER llwebrtc.h)

target_link_libraries(llwebrtc PRIVATE ll::webrtc 
                                       secur32
                                       winmm
                                       dmoguids
                                       wmcodecdspuuid
                                       msdmo
                                       strmiids
                                       iphlpapi)
target_include_directories( llwebrtc  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET llwebrtc PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")

install(TARGETS llwebrtc RUNTIME DESTINATION "${CMAKE_BINARY_DIR}/sharedlibs/RelWithDebInfo")

# Add tests
if (LL_TESTS)
endif (LL_TESTS)
